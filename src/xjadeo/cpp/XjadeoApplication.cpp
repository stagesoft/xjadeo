#include "XjadeoApplication.h"
#include "config/ConfigurationManager.h"
#include "input/VideoFileInput.h"
#include "sync/MIDISyncSource.h"
#include "layer/LayerManager.h"
#include "layer/VideoLayer.h"
#include "video/FrameFormat.h"
#include "display/OpenGLDisplay.h"
#include "display/DisplayManager.h"
#include "remote/OSCRemoteControl.h"
#include "osd/OSDManager.h"
#include "utils/Logger.h"
#include "utils/SMPTEUtils.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <unistd.h>

namespace xjadeo {

XjadeoApplication::XjadeoApplication()
    : running_(false)
    , initialized_(false)
{
}

XjadeoApplication::~XjadeoApplication() {
    shutdown();
}

bool XjadeoApplication::initialize(int argc, char** argv) {
    if (initialized_) {
        return true;
    }

    // Initialize logger based on configuration
    // This will be set after config is loaded, but we can set defaults
    Logger::getInstance().setQuiet(false);

    // Initialize configuration
    if (!initializeConfiguration(argc, argv)) {
        LOG_ERROR << "Failed to initialize configuration";
        return false;
    }
    
    // Update logger based on config
    bool quiet = config_->getBool("want_quiet", false);
    bool verbose = config_->getBool("want_verbose", false);
    Logger::getInstance().setQuiet(quiet);
    if (verbose) {
        Logger::getInstance().setLevel(Logger::VERBOSE);
    }

    // Initialize display
    if (!initializeDisplay()) {
        LOG_ERROR << "Failed to initialize display";
        return false;
    }

    // Initialize layer manager
    if (!initializeLayerManager()) {
        return false;
    }

    // Initialize OSD manager
    osdManager_ = std::make_unique<OSDManager>();

    // Initialize remote control
    if (!initializeRemoteControl()) {
        LOG_WARNING << "Failed to initialize remote control (continuing without it)";
        // Don't fail initialization if remote control fails
    }

    // Create initial layer if movie file provided
    std::string movieFile = config_->getMovieFile();
    if (!movieFile.empty()) {
        if (!createInitialLayer()) {
            LOG_ERROR << "Failed to create initial layer with file: " << movieFile;
            return false;
        }
        LOG_INFO << "Loaded video file: " << movieFile;
    }

    initialized_ = true;
    running_ = true;
    return true;
}

bool XjadeoApplication::initializeConfiguration(int argc, char** argv) {
    config_ = std::make_unique<ConfigurationManager>();
    
    // Load default config file (if exists)
    std::string configFile = config_->getConfigFilePath();
    config_->loadFromFile(configFile); // Ignore errors if file doesn't exist
    
    // Parse command line (overrides config file)
    config_->parseCommandLine(argc, argv);
    
    return true;
}

bool XjadeoApplication::initializeDisplay() {
    // Create display manager
    displayManager_ = std::make_unique<DisplayManager>();
    if (!displayManager_->detectDisplays()) {
        std::cerr << "Failed to detect displays" << std::endl;
        return false;
    }

    // Create OpenGL display backend
    displayBackend_ = std::make_unique<OpenGLDisplay>();

    // Create window (single window mode for now)
    if (!displayManager_->createWindows(displayBackend_.get(), 0)) {
        std::cerr << "Failed to create display window" << std::endl;
        return false;
    }

    return true;
}

bool XjadeoApplication::initializeRemoteControl() {
    // Get OSC port from config (default: 7000)
    int oscPort = config_->getInt("osc_port", 7000);
    
    // Create OSC remote control
    remoteControl_ = std::make_unique<OSCRemoteControl>(this, layerManager_.get());
    
    if (!remoteControl_->initialize(oscPort)) {
        std::cerr << "Failed to initialize OSC remote control on port " << oscPort << std::endl;
        return false;
    }
    
    return true;
}

bool XjadeoApplication::initializeLayerManager() {
    layerManager_ = std::make_unique<LayerManager>();
    return true;
}

bool XjadeoApplication::createInitialLayer() {
    std::string movieFile = config_->getMovieFile();
    if (movieFile.empty()) {
        return false;
    }

    // Create input source
    auto inputSource = std::make_unique<VideoFileInput>();
    if (!inputSource->open(movieFile)) {
        return false;
    }

    // Create sync source (optional - can be manual)
    std::unique_ptr<SyncSource> syncSource;
    std::string midiPort = config_->getString("midi_port", "-1");
    if (!midiPort.empty() && midiPort != "-1") {
        auto midiSync = std::make_unique<MIDISyncSource>();
        
        // Configure MIDI sync source from config
        bool verbose = config_->getBool("want_verbose", false);
        bool midiClkAdj = config_->getBool("midi_clkadj", false);
        double delay = config_->getDouble("delay", -1.0);
        
        // Set configuration before connecting
        midiSync->setVerbose(verbose);
        midiSync->setClockAdjustment(midiClkAdj);
        midiSync->setDelay(delay);
        
        // Connect to MIDI port
        syncSource = std::move(midiSync);
        syncSource->connect(midiPort.c_str());
    }

    // Create layer
    auto layer = std::make_unique<VideoLayer>();
    layer->setInputSource(std::move(inputSource));
    if (syncSource) {
        layer->setSyncSource(std::move(syncSource));
    }

    // Set layer properties from config
    auto& props = layer->properties();
    if (layer->isReady()) {
        FrameInfo info = layer->getFrameInfo();
        props.width = info.width;
        props.height = info.height;
    }
    props.visible = true;
    props.opacity = 1.0f;
    props.zOrder = 0;

    // Add layer to manager
    int layerId = layerManager_->addLayer(std::move(layer));
    if (layerId < 0) {
        return false;
    }

    // Start playback if sync source is connected
    VideoLayer* addedLayer = layerManager_->getLayer(layerId);
    if (addedLayer && addedLayer->getSyncSource() && addedLayer->getSyncSource()->isConnected()) {
        addedLayer->play();
    }

    return true;
}

int XjadeoApplication::run() {
    if (!initialized_) {
        LOG_ERROR << "Application not initialized";
        return 1;
    }
    
    LOG_INFO << "Starting xjadeo application";

    // Main event loop
    // Use high-resolution clock for timing
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    const auto targetFrameTime = std::chrono::microseconds(16667); // ~60 FPS default (16.67ms)
    
    while (running_ && shouldContinue()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastFrameTime);
        
        processEvents();
        updateLayers();
        render();
        
        // Frame rate limiting - sleep if we're ahead of target frame time
        // This prevents busy-waiting while still allowing sync sources to drive timing
        auto sleepTime = targetFrameTime - elapsed;
        if (sleepTime.count() > 0) {
            std::this_thread::sleep_for(sleepTime);
        }
        
        lastFrameTime = std::chrono::high_resolution_clock::now();
    }

    return 0;
}

void XjadeoApplication::processEvents() {
    // Process display window events
    if (displayBackend_) {
        displayBackend_->handleEvents();
    }
    
    // Process remote control events
    if (remoteControl_) {
        remoteControl_->process();
    }
}

void XjadeoApplication::updateLayers() {
    if (layerManager_) {
        layerManager_->updateAll();
        
        // Update OSD with current frame information from first layer
        if (osdManager_ && layerManager_->getLayerCount() > 0) {
            auto layers = layerManager_->getLayers();
            if (!layers.empty() && layers[0]) {
                VideoLayer* layer = layers[0];
                if (layer->isReady()) {
                    int64_t currentFrame = layer->getCurrentFrame();
                    if (currentFrame >= 0) {
                        osdManager_->setFrameNumber(currentFrame);
                        
                        // Update SMPTE if enabled
                        if (osdManager_->isModeEnabled(OSDManager::SMPTE)) {
                            FrameInfo info = layer->getFrameInfo();
                            if (info.framerate > 0.0) {
                                // Use SMPTEUtils for proper timecode formatting
                                std::string smpte = SMPTEUtils::frameToSmpteString(currentFrame, info.framerate);
                                osdManager_->setSMPTETimecode(smpte);
                            }
                        }
                    }
                }
            }
        }
    }
}

void XjadeoApplication::render() {
    if (displayBackend_ && layerManager_) {
        displayBackend_->render(layerManager_.get(), osdManager_.get());
    }
}

bool XjadeoApplication::setFPS(double fps) {
    if (fps <= 0.0) {
        return false;
    }
    
    // Store FPS in configuration
    if (config_) {
        config_->setDouble("fps", fps);
    }
    
    // Apply FPS to all layers (if they support it)
    // For now, FPS is typically a property of the video file itself
    // This could be used for time-scaling in the future
    // TODO: Implement time-scaling per layer
    
    return true;
}

bool XjadeoApplication::setTimeOffset(int64_t offset) {
    // Store offset in configuration
    if (config_) {
        config_->setInt("offset", static_cast<int>(offset));
    }
    
    // Apply offset to all layers
    if (layerManager_) {
        auto layers = layerManager_->getLayers();
        for (auto* layer : layers) {
            if (layer) {
                layer->setTimeOffset(offset);
            }
        }
    }
    
    return true;
}

void XjadeoApplication::shutdown() {
    running_ = false;
    
    // Shutdown layers (they will clean up their input/sync sources)
    if (layerManager_) {
        layerManager_.reset();
    }
    
    if (remoteControl_) {
        remoteControl_->shutdown();
        remoteControl_.reset();
    }
    
    if (displayBackend_) {
        displayBackend_->closeWindow();
        displayBackend_.reset();
    }
    
    if (displayManager_) {
        displayManager_.reset();
    }
    
    config_.reset();
    initialized_ = false;
}

} // namespace xjadeo

