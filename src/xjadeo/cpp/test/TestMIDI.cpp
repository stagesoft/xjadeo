/**
 * TestMIDI.cpp - MIDI implementation test using libmtcmaster
 * 
 * This test program uses libmtcmaster to generate MTC (MIDI Time Code)
 * and tests the ALSASeqMIDIDriver implementation for MTC reception.
 */

#include "sync/ALSASeqMIDIDriver.h"
#include "sync/MTCDecoder.h"
#include "utils/Logger.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdlib>

#ifdef HAVE_LIBMTCMASTER
#include "interface.h"
#include "MtcMaster_class.h"
#endif

using namespace xjadeo;

// Test configuration
static const double TEST_FRAMERATE = 25.0;
static const int TEST_DURATION_SEC = 10;  // Test for 10 seconds
static const int TEST_START_FRAME = 0;

#ifdef HAVE_LIBMTCMASTER
// libmtcmaster C interface (from interface.h)
extern "C" {
    void* MTCSender_create();
    void MTCSender_release(void* mtcsender);
    void MTCSender_openPort(void* mtcsender, unsigned int portnumber, const char* portname);
    void MTCSender_play(void* mtcsender);
    void MTCSender_stop(void* mtcsender);
    void MTCSender_pause(void* mtcsender);
    void MTCSender_setTime(void* mtcsender, uint64_t nanos);
}

// We need the C++ class to access getTime() and getFrameRate()
// Include the C++ header directly (we're in C++ code)
// The include path is set by CMake to find the libmtcmaster repository
#include "MtcMaster_class.h"

/**
 * Generate MTC using libmtcmaster
 */
class MTCGenerator {
public:
    MTCGenerator() : mtc_(nullptr), running_(false), framerate_(25.0) {}
    
    ~MTCGenerator() {
        stop();
    }
    
    bool start(const std::string& portName, double framerate, int startFrame) {
        if (running_) {
            return false;
        }
        
        framerate_ = framerate;
        
        // Create MTC sender using C interface
        mtc_ = MTCSender_create();
        if (!mtc_) {
            std::cerr << "Failed to create MTC sender" << std::endl;
            return false;
        }
        
        // Convert framerate to FrameRate enum
        FrameRate fr;
        if (framerate == 24.0) {
            fr = FR_24;
        } else if (framerate == 25.0) {
            fr = FR_25;
        } else if (framerate == 29.97 || framerate == 29.0) {
            fr = FR_29;
        } else if (framerate == 30.0) {
            fr = FR_30;
        } else {
            std::cerr << "Unsupported framerate: " << framerate << std::endl;
            MTCSender_release(mtc_);
            mtc_ = nullptr;
            return false;
        }
        
        // Cast to C++ class to set framerate
        MtcMaster* master = static_cast<MtcMaster*>(mtc_);
        master->setFrameRate(fr);
        
        // Open port (port 0 = default, portname from string)
        // Parse port number from portName (format: "client:port" or just number)
        unsigned int portNum = 0;
        std::string portNameStr = portName;
        if (portName.find(':') != std::string::npos) {
            // Format: "128:0" - extract port number
            size_t colon = portName.find(':');
            portNum = std::atoi(portName.substr(colon + 1).c_str());
            portNameStr = portName;
        } else {
            portNum = std::atoi(portName.c_str());
            portNameStr = "MTCPort";
        }
        
        MTCSender_openPort(mtc_, portNum, portNameStr.c_str());
        
        // Set start time in nanoseconds
        // Convert frame to nanoseconds: frame * (1e9 / framerate)
        uint64_t startTimeNanos = static_cast<uint64_t>(startFrame * (1e9 / framerate));
        MTCSender_setTime(mtc_, startTimeNanos);
        
        // Start playing
        MTCSender_play(mtc_);
        
        running_ = true;
        std::cout << "MTC generation started on port: " << portName << " (port " << portNum << ")" << std::endl;
        std::cout << "Framerate: " << framerate << " fps, Start frame: " << startFrame << std::endl;
        return true;
    }
    
    void stop() {
        if (mtc_ && running_) {
            MTCSender_stop(mtc_);
            MTCSender_release(mtc_);
            mtc_ = nullptr;
            running_ = false;
        }
    }
    
    bool isRunning() const { return running_; }
    
    int64_t getCurrentFrame() const {
        if (mtc_) {
            // Cast to C++ class to access getTime()
            MtcMaster* master = static_cast<MtcMaster*>(mtc_);
            uint64_t timeNanos = master->getTime();
            
            // Convert nanoseconds to frame: timeNanos * framerate / 1e9
            return static_cast<int64_t>(timeNanos * framerate_ / 1e9);
        }
        return -1;
    }
    
private:
    void* mtc_;  // Opaque pointer to MtcMaster instance
    bool running_;
    double framerate_;  // Store framerate for frame conversion
};
#endif

/**
 * Test MTC reception
 */
bool test_MTC_Reception() {
    std::cout << "\n=== Testing MTC Reception ===" << std::endl;
    
#ifdef HAVE_LIBMTCMASTER
    // Create MTC generator
    MTCGenerator generator;
    
    // Create ALSA MIDI driver
    auto driver = std::make_unique<ALSASeqMIDIDriver>();
    
    if (!driver->isSupported()) {
        std::cerr << "ALSA Sequencer not supported on this system" << std::endl;
        return false;
    }
    
    // Configure driver
    driver->setFramerate(TEST_FRAMERATE);
    driver->setVerbose(true);
    driver->setClockAdjustment(false);  // Disable for testing
    
    // Open driver (autodetect port)
    std::cout << "Opening ALSA MIDI driver..." << std::endl;
    if (!driver->open("-1")) {
        std::cerr << "Failed to open ALSA MIDI driver" << std::endl;
        return false;
    }
    
    // Wait a bit for connection
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    if (!driver->isConnected()) {
        std::cerr << "MIDI driver not connected. Please connect an MTC source manually." << std::endl;
        std::cerr << "You can use: aconnect <source> <destination>" << std::endl;
        driver->close();
        return false;
    }
    
    // Start MTC generation
    std::string mtcPort = "128:0";  // Default ALSA sequencer port
    std::cout << "Starting MTC generation on port: " << mtcPort << std::endl;
    
    if (!generator.start(mtcPort, TEST_FRAMERATE, TEST_START_FRAME)) {
        std::cerr << "Failed to start MTC generation" << std::endl;
        driver->close();
        return false;
    }
    
    // Test reception
    std::cout << "Testing MTC reception for " << TEST_DURATION_SEC << " seconds..." << std::endl;
    std::cout << std::setw(10) << "Time" << std::setw(15) << "Generated" 
              << std::setw(15) << "Received" << std::setw(15) << "Diff" << std::endl;
    
    auto startTime = std::chrono::steady_clock::now();
    int64_t lastReceivedFrame = -1;
    int frameCount = 0;
    int errorCount = 0;
    
    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
        
        if (elapsed >= TEST_DURATION_SEC) {
            break;
        }
        
        // Poll for frame
        int64_t receivedFrame = driver->pollFrame();
        int64_t generatedFrame = generator.getCurrentFrame();
        
        if (receivedFrame >= 0) {
            if (receivedFrame != lastReceivedFrame) {
                frameCount++;
                int64_t diff = receivedFrame - generatedFrame;
                
                std::cout << std::setw(10) << elapsed << "s"
                          << std::setw(15) << generatedFrame
                          << std::setw(15) << receivedFrame
                          << std::setw(15) << diff << std::endl;
                
                // Check for large differences (allow small timing differences)
                if (std::abs(diff) > 2) {
                    errorCount++;
                    std::cerr << "Warning: Large frame difference detected: " << diff << std::endl;
                }
                
                lastReceivedFrame = receivedFrame;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Stop generator
    generator.stop();
    driver->close();
    
    // Print results
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Frames received: " << frameCount << std::endl;
    std::cout << "Errors: " << errorCount << std::endl;
    
    if (frameCount == 0) {
        std::cerr << "ERROR: No frames received!" << std::endl;
        return false;
    }
    
    if (errorCount > frameCount / 10) {  // Allow up to 10% errors
        std::cerr << "ERROR: Too many frame errors!" << std::endl;
        return false;
    }
    
    std::cout << "Test PASSED" << std::endl;
    return true;
    
#else
    std::cerr << "libmtcmaster not available. Cannot run MTC test." << std::endl;
    std::cerr << "Install libmtcmaster and rebuild with ENABLE_MIDI=ON" << std::endl;
    return false;
#endif
}

/**
 * Test MTC decoder directly
 */
bool test_MTCDecoder() {
    std::cout << "\n=== Testing MTCDecoder ===" << std::endl;
    
    MTCDecoder decoder;
    
    // Test quarter-frame messages for frame 1234 at 25fps
    // Frame 1234 = 00:00:49:09 (25fps)
    // Hour: 0, Min: 0, Sec: 49, Frame: 9
    
    // Quarter-frame messages (in order 0-7):
    // 0: Frame low nibble (0x09 & 0x0F = 0x09)
    // 1: Frame high nibble (0x09 >> 4 = 0x00)
    // 2: Seconds low nibble (0x31 & 0x0F = 0x01)
    // 3: Seconds high nibble (0x31 >> 4 = 0x03)
    // 4: Minutes low nibble (0x00 & 0x0F = 0x00)
    // 5: Minutes high nibble (0x00 >> 4 = 0x00)
    // 6: Hours low nibble + type (0x00)
    // 7: Hours high nibble (0x00)
    
    // Test quarter-frame messages for frame 1234 at 25fps
    // Frame 1234 = 00:00:49:09 (25fps)
    // MTC format: upper nibble = quarter-frame index (0-7), lower nibble = data
    uint8_t quarterFrames[] = {
        0x09,  // Quarter-frame 0: Frame low nibble (9)
        0x10,  // Quarter-frame 1: Frame high nibble (0)
        0x21,  // Quarter-frame 2: Seconds low nibble (1)
        0x33,  // Quarter-frame 3: Seconds high nibble (3) -> 49 seconds (0x31)
        0x40,  // Quarter-frame 4: Minutes low nibble (0)
        0x50,  // Quarter-frame 5: Minutes high nibble (0)
        0x60,  // Quarter-frame 6: Hours low nibble (0)
        0x72   // Quarter-frame 7: Hours high nibble bit 0 (0), type bits 1-2 (type 1 = 01)
    };
    
    // Process all quarter-frames
    bool complete = false;
    for (int i = 0; i < 8; i++) {
        complete = decoder.processByte(quarterFrames[i]);
    }
    
    // Verify complete timecode was received
    if (!complete) {
        std::cerr << "MTCDecoder test FAILED: Complete timecode not received" << std::endl;
        return false;
    }
    
    // Check result
    int64_t frame = decoder.timecodeToFrame(25.0);
    
    std::cout << "Decoded frame: " << frame << std::endl;
    std::cout << "Expected frame: 1234" << std::endl;
    
    if (frame == 1234) {
        std::cout << "MTCDecoder test PASSED" << std::endl;
        return true;
    } else {
        std::cerr << "MTCDecoder test FAILED: expected 1234, got " << frame << std::endl;
        return false;
    }
}

int main(int argc, char** argv) {
    std::cout << "xjadeo MIDI Test Suite" << std::endl;
    std::cout << "======================" << std::endl;
    
    bool allPassed = true;
    
    // Test 1: MTCDecoder
    if (!test_MTCDecoder()) {
        allPassed = false;
    }
    
    // Test 2: MTC Reception (requires libmtcmaster and ALSA)
    if (argc == 1 || (argc > 1 && strcmp(argv[1], "--skip-mtc") != 0)) {
        if (!test_MTC_Reception()) {
            allPassed = false;
        }
    } else {
        std::cout << "\nSkipping MTC reception test (use without --skip-mtc to run)" << std::endl;
    }
    
    if (allPassed) {
        std::cout << "\n=== All tests PASSED ===" << std::endl;
        return 0;
    } else {
        std::cerr << "\n=== Some tests FAILED ===" << std::endl;
        return 1;
    }
}

