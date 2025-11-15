#ifndef XJADEO_REMOTECOMMANDROUTER_H
#define XJADEO_REMOTECOMMANDROUTER_H

#include <string>
#include <functional>
#include <map>
#include <vector>

namespace xjadeo {

// Forward declarations
class XjadeoApplication;
class VideoLayer;
class LayerManager;

/**
 * RemoteCommandRouter - Routes commands to application or specific layers
 * 
 * Parses command paths and delegates to appropriate handlers:
 * - App-level: /jadeo/quit, /jadeo/layer/add, etc.
 * - Layer-level: /jadeo/layer/<id>/seek, /jadeo/layer/<id>/play, etc.
 */
class RemoteCommandRouter {
public:
    RemoteCommandRouter(XjadeoApplication* app, LayerManager* layerManager);
    ~RemoteCommandRouter();

    // Route a command (called from RemoteControl implementations)
    // Returns true if command was handled, false otherwise
    bool routeCommand(const std::string& path, const std::vector<std::string>& args);

    // Register command handlers (for extensibility)
    void registerAppCommand(const std::string& path, 
                            std::function<bool(const std::vector<std::string>&)> handler);
    void registerLayerCommand(const std::string& path,
                              std::function<bool(VideoLayer*, const std::vector<std::string>&)> handler);

private:
    XjadeoApplication* app_;
    LayerManager* layerManager_;

    // Command handler maps
    std::map<std::string, std::function<bool(const std::vector<std::string>&)>> appCommands_;
    std::map<std::string, std::function<bool(VideoLayer*, const std::vector<std::string>&)>> layerCommands_;

    // Parse command path
    bool parsePath(const std::string& path, std::string& command, int& layerId);

    // App-level command handlers
    bool handleQuit(const std::vector<std::string>& args);
    bool handleLoad(const std::vector<std::string>& args);
    bool handleSeek(const std::vector<std::string>& args);
    bool handleFPS(const std::vector<std::string>& args);
    bool handleOffset(const std::vector<std::string>& args);
    bool handleLayerAdd(const std::vector<std::string>& args);
    bool handleLayerRemove(const std::vector<std::string>& args);
    bool handleLayerDuplicate(const std::vector<std::string>& args);
    bool handleLayerReorder(const std::vector<std::string>& args);
    bool handleLayerList(const std::vector<std::string>& args);
    
    // OSD command handlers
    bool handleOSDFrame(const std::vector<std::string>& args);
    bool handleOSDSMPTE(const std::vector<std::string>& args);
    bool handleOSDText(const std::vector<std::string>& args);
    bool handleOSDBox(const std::vector<std::string>& args);
    bool handleOSDFont(const std::vector<std::string>& args);
    bool handleOSDPos(const std::vector<std::string>& args);

    // Layer-level command handlers
    bool handleLayerSeek(VideoLayer* layer, const std::vector<std::string>& args);
    bool handleLayerPlay(VideoLayer* layer, const std::vector<std::string>& args);
    bool handleLayerPause(VideoLayer* layer, const std::vector<std::string>& args);
    bool handleLayerPosition(VideoLayer* layer, const std::vector<std::string>& args);
    bool handleLayerOpacity(VideoLayer* layer, const std::vector<std::string>& args);
    bool handleLayerVisible(VideoLayer* layer, const std::vector<std::string>& args);
    bool handleLayerZOrder(VideoLayer* layer, const std::vector<std::string>& args);
    
    // Time-scaling command handlers (app-level)
    bool handleTimeScale(const std::vector<std::string>& args);
    bool handleTimeScale2(const std::vector<std::string>& args);  // timescale + offset
    bool handleLoop(const std::vector<std::string>& args);
    bool handleReverse(const std::vector<std::string>& args);
    
    // Time-scaling command handlers (layer-level)
    bool handleLayerTimeScale(VideoLayer* layer, const std::vector<std::string>& args);
    bool handleLayerTimeScale2(VideoLayer* layer, const std::vector<std::string>& args);
    bool handleLayerLoop(VideoLayer* layer, const std::vector<std::string>& args);
    bool handleLayerReverse(VideoLayer* layer, const std::vector<std::string>& args);
    
    // Crop/Panorama command handlers (app-level)
    bool handlePan(const std::vector<std::string>& args);
    
    // Crop/Panorama command handlers (layer-level)
    bool handleLayerPan(VideoLayer* layer, const std::vector<std::string>& args);
    bool handleLayerCrop(VideoLayer* layer, const std::vector<std::string>& args);
    bool handleLayerPanorama(VideoLayer* layer, const std::vector<std::string>& args);
};

} // namespace xjadeo

#endif // XJADEO_REMOTECOMMANDROUTER_H

