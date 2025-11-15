#ifndef XJADEO_APPLICATION_H
#define XJADEO_APPLICATION_H

#include <memory>
#include <string>

namespace xjadeo {

// Forward declarations
class ConfigurationManager;
class InputSource;
class SyncSource;
class RemoteControl;
class DisplayBackend;
class LayerManager;
class VideoLayer;
class DisplayManager;
class OSDManager;

/**
 * XjadeoApplication - Main application orchestrator
 * 
 * Manages all components, coordinates the event loop, layer updates, and rendering.
 * This is the central class that ties everything together.
 */
class XjadeoApplication {
public:
    XjadeoApplication();
    ~XjadeoApplication();

    // Initialize application
    bool initialize(int argc, char** argv);

    // Run main event loop
    int run();

    // Shutdown application
    void shutdown();

    // Check if application should continue running
    bool shouldContinue() const { return running_; }

    // Quit application (called from remote control)
    void quit() { running_ = false; }

    // Configuration methods (called from remote control)
    bool setFPS(double fps);
    bool setTimeOffset(int64_t offset);
    
    // Get configuration
    ConfigurationManager* getConfig() { return config_.get(); }
    LayerManager* getLayerManager() { return layerManager_.get(); }
    OSDManager* getOSDManager() { return osdManager_.get(); }

private:
    // Component initialization
    bool initializeConfiguration(int argc, char** argv);
    bool initializeDisplay();
    bool initializeRemoteControl();
    bool initializeLayerManager();
    bool createInitialLayer();

    // Event loop
    void processEvents();
    void updateLayers();
    void render();

    // Component pointers
    std::unique_ptr<ConfigurationManager> config_;
    std::unique_ptr<RemoteControl> remoteControl_;
    std::unique_ptr<DisplayBackend> displayBackend_;
    std::unique_ptr<DisplayManager> displayManager_;
    std::unique_ptr<LayerManager> layerManager_;
    std::unique_ptr<OSDManager> osdManager_;

    // Application state
    bool running_;
    bool initialized_;
};

} // namespace xjadeo

#endif // XJADEO_APPLICATION_H

