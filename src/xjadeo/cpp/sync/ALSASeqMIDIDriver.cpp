#include "ALSASeqMIDIDriver.h"
#include "../utils/Logger.h"
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <poll.h>
#include <unistd.h>
#include <cstring>
#include <cmath>

// No longer using C globals - all configuration passed via methods

namespace xjadeo {

ALSASeqMIDIDriver::ALSASeqMIDIDriver()
    : seq_(nullptr)
    , portId_(-1)
    , framerate_(25.0)
    , stopThread_(false)
    , connected_(false)
    , lastFrame_(-1)
    , currentFrame_(-1)
    , stopCount_(0)
    , midiClkAdj_(false)
    , delay_(0.0)
    , verbose_(false)
{
}

ALSASeqMIDIDriver::~ALSASeqMIDIDriver() {
    close();
}

bool ALSASeqMIDIDriver::isSupported() const {
    // Check if ALSA sequencer is available at compile time
#ifdef ALSA_SEQ_MIDI
    return true;
#else
    return false;
#endif
}

bool ALSASeqMIDIDriver::open(const std::string& portId) {
    if (connected_) {
        close();
    }

    // Configuration is set via setClockAdjustment(), setDelay(), setFramerate(), setVerbose()
    // These should be called before open() or after construction

    // Open ALSA sequencer
    if (!openSequencer()) {
        return false;
    }

    // Create port
    if (!createPort()) {
        closeSequencer();
        return false;
    }

    // Connect to specified port (if provided)
    if (!portId.empty() && portId != "-1") {
        if (!connectToPort(portId)) {
            LOG_WARNING << "Failed to connect to MIDI port: " << portId;
            // Continue anyway - port might be connected later
        }
    } else {
        // Autodetect - list devices if verbose
        if (verbose_) {
            detectDevices(true);
        }
    }

    // Initialize MTC decoder
    mtcDecoder_.reset();
    lastFrame_ = -1;
    currentFrame_ = -1;
    stopCount_ = 0;

    // Start background thread
    stopThread_ = false;
    try {
        thread_ = std::thread(&ALSASeqMIDIDriver::runThread, this);
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to start ALSA MIDI thread: " << e.what();
        closeSequencer();
        return false;
    }

    connected_ = true;
    return true;
}

void ALSASeqMIDIDriver::close() {
    if (!connected_) {
        return;
    }

    // Stop thread
    stopThread_ = true;
    if (thread_.joinable()) {
        thread_.join();
    }

    // Close sequencer
    closeSequencer();
    
    connected_ = false;
    currentFrame_ = -1;
    lastFrame_ = -1;
}

bool ALSASeqMIDIDriver::isConnected() const {
    return connected_ && (seq_ != nullptr);
}

int64_t ALSASeqMIDIDriver::pollFrame() {
    if (!isConnected()) {
        return -1;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get frame from MTC decoder
    int64_t frame = mtcDecoder_.timecodeToFrame(framerate_);
    
    if (frame < 0) {
        // No valid timecode yet
        return -1;
    }

    // Apply clock adjustment if enabled
    if (midiClkAdj_) {
        const auto& tc = mtcDecoder_.getTimecode();
        
        // Check if transport is stuck
        if (lastFrame_ == frame) {
            stopCount_++;
            double dly = delay_ > 0 ? delay_ : (1.0 / framerate_);
            int threshold = static_cast<int>(std::ceil(4.0 * framerate_ / dly));
            
            if (stopCount_ > threshold) {
                // Transport appears stuck - reset
                mtcDecoder_.reset();
                if (verbose_) {
                    printf("\r\t\t\t\t\t\t        -?-\r");
                    fflush(stdout);
                }
                return -1;
            }
        } else {
            stopCount_ = 0;
            lastFrame_ = frame;
        }

        // Apply quarter-frame adjustment
        double diff = tc.tick / 4.0;
        if (verbose_) {
            double adj = (diff < 0) ? std::rint(4.0 * (1.75 - diff)) :
                        (diff < 2.0) ? 0 : std::rint(4.0 * (diff - 1.75));
            printf("\r\t\t\t\t\t\t  |+%g/8\r", adj);
            fflush(stdout);
        }
        frame += static_cast<int64_t>(std::rint(diff));
    }

    currentFrame_ = frame;
    return frame;
}

bool ALSASeqMIDIDriver::openSequencer() {
    if (seq_) {
        return true; // Already open
    }

    char seqName[32];
    snprintf(seqName, sizeof(seqName), "xjadeo-%i", static_cast<int>(getpid()));

    int err = snd_seq_open(&seq_, "default", SND_SEQ_OPEN_INPUT, 0);
    if (err < 0) {
        LOG_ERROR << "Cannot open ALSA sequencer: " << snd_strerror(err);
        seq_ = nullptr;
        return false;
    }

    err = snd_seq_set_client_name(seq_, seqName);
    if (err < 0) {
        LOG_ERROR << "Cannot set client name: " << snd_strerror(err);
        snd_seq_close(seq_);
        seq_ = nullptr;
        return false;
    }

    // Set non-blocking mode
    snd_seq_nonblock(seq_, 1);

    return true;
}

void ALSASeqMIDIDriver::closeSequencer() {
    if (!seq_) {
        return;
    }

    if (verbose_) {
        printf("closing alsa midi...");
        fflush(stdout);
    }

    snd_seq_close(seq_);
    seq_ = nullptr;
    portId_ = -1;
}

bool ALSASeqMIDIDriver::createPort() {
    if (!seq_) {
        return false;
    }

    int err = snd_seq_create_simple_port(seq_, "MTC in",
                                         SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
                                         SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
    if (err < 0) {
        LOG_ERROR << "Cannot create port: " << snd_strerror(err);
        return false;
    }

    portId_ = err;
    return true;
}

bool ALSASeqMIDIDriver::connectToPort(const std::string& portName) {
    if (!seq_ || portId_ < 0) {
        return false;
    }

    snd_seq_addr_t port;
    int err = snd_seq_parse_address(seq_, &port, portName.c_str());
    if (err < 0) {
        LOG_ERROR << "Cannot find port " << portName << " - " << snd_strerror(err);
        return false;
    }

    err = snd_seq_connect_from(seq_, portId_, port.client, port.port);
    if (err < 0) {
        LOG_ERROR << "Cannot connect from port " << port.client << ":" << port.port 
                  << " - " << snd_strerror(err);
        return false;
    }

    return true;
}

void ALSASeqMIDIDriver::processEvent(void* eventPtr) {
    snd_seq_event_t* ev = static_cast<snd_seq_event_t*>(eventPtr);
    
    if (ev->type == SND_SEQ_EVENT_QFRAME) {
        // MTC quarter-frame message
        // The value contains the quarter-frame data byte (lower 7 bits)
        uint8_t data = static_cast<uint8_t>(ev->data.control.value & 0x7F);
        mtcDecoder_.processByte(data);
    } else if (ev->type == SND_SEQ_EVENT_SYSEX) {
        // System exclusive message (for full timecode)
        // Process sysex bytes
        unsigned char* sysexData = static_cast<unsigned char*>(ev->data.ext.ptr);
        for (unsigned int i = 1; i < ev->data.ext.len; ++i) {
            // TODO: Parse sysex URTM messages if needed
            // For now, we rely on quarter-frame messages
        }
    }
}

void ALSASeqMIDIDriver::runThread() {
    if (!seq_) {
        return;
    }

    int npfds = snd_seq_poll_descriptors_count(seq_, POLLIN);
    if (npfds <= 0) {
        return;
    }

    struct pollfd* pfds = static_cast<struct pollfd*>(alloca(sizeof(struct pollfd) * npfds));
    
    while (!stopThread_) {
        snd_seq_poll_descriptors(seq_, pfds, npfds, POLLIN);
        
        int pollResult = poll(pfds, npfds, 100); // 100ms timeout
        if (pollResult < 0) {
            break; // Error
        }
        
        if (pollResult == 0) {
            continue; // Timeout
        }

        // Process events
        do {
            snd_seq_event_t* event = nullptr;
            int err = snd_seq_event_input(seq_, &event);
            
            if (err < 0) {
                break; // Error or no more events
            }
            
            if (event) {
                std::lock_guard<std::mutex> lock(mutex_);
                processEvent(event);
            }
        } while (true); // Continue until no more events
    }
}

int ALSASeqMIDIDriver::detectDevices(bool verbose) {
    if (!seq_ || !verbose) {
        return 0;
    }

    snd_seq_client_info_t* cinfo;
    snd_seq_port_info_t* pinfo;

    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);

    printf(" Dumping midi seq ports: (not connecting to any)\n");
    printf("  Port    Client name                      Port name\n");

    int count = 0;
    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(seq_, cinfo) >= 0) {
        int client = snd_seq_client_info_get_client(cinfo);

        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(seq_, pinfo) >= 0) {
            // We need both READ and SUBS_READ
            unsigned int caps = snd_seq_port_info_get_capability(pinfo);
            if ((caps & (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ))
                != (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ)) {
                continue;
            }
            
            printf(" %3d:%-3d  %-32.32s %s\n",
                   snd_seq_port_info_get_client(pinfo),
                   snd_seq_port_info_get_port(pinfo),
                   snd_seq_client_info_get_name(cinfo),
                   snd_seq_port_info_get_name(pinfo));
            count++;
        }
    }

    return count;
}

} // namespace xjadeo

