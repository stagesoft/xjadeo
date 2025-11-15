#include "RemoteCommandRouter.h"
#include "../XjadeoApplication.h"
#include "../layer/LayerManager.h"
#include "../layer/VideoLayer.h"
#include "../input/VideoFileInput.h"
#include "../sync/MIDISyncSource.h"
#include "../osd/OSDManager.h"
#include <sstream>
#include <algorithm>
#include <cstdlib>

namespace xjadeo {

RemoteCommandRouter::RemoteCommandRouter(XjadeoApplication* app, LayerManager* layerManager)
    : app_(app)
    , layerManager_(layerManager)
{
    // Register app-level commands
    registerAppCommand("quit", [this](const std::vector<std::string>& args) {
        return handleQuit(args);
    });
    registerAppCommand("load", [this](const std::vector<std::string>& args) {
        return handleLoad(args);
    });
    registerAppCommand("seek", [this](const std::vector<std::string>& args) {
        return handleSeek(args);
    });
    registerAppCommand("fps", [this](const std::vector<std::string>& args) {
        return handleFPS(args);
    });
    registerAppCommand("offset", [this](const std::vector<std::string>& args) {
        return handleOffset(args);
    });
    registerAppCommand("layer/add", [this](const std::vector<std::string>& args) {
        return handleLayerAdd(args);
    });
    registerAppCommand("layer/remove", [this](const std::vector<std::string>& args) {
        return handleLayerRemove(args);
    });
    registerAppCommand("layer/duplicate", [this](const std::vector<std::string>& args) {
        return handleLayerDuplicate(args);
    });
    registerAppCommand("layer/reorder", [this](const std::vector<std::string>& args) {
        return handleLayerReorder(args);
    });
    registerAppCommand("layer/list", [this](const std::vector<std::string>& args) {
        return handleLayerList(args);
    });
    registerAppCommand("osd/frame", [this](const std::vector<std::string>& args) {
        return handleOSDFrame(args);
    });
    registerAppCommand("osd/smpte", [this](const std::vector<std::string>& args) {
        return handleOSDSMPTE(args);
    });
    registerAppCommand("osd/text", [this](const std::vector<std::string>& args) {
        return handleOSDText(args);
    });
    registerAppCommand("osd/box", [this](const std::vector<std::string>& args) {
        return handleOSDBox(args);
    });
    registerAppCommand("osd/font", [this](const std::vector<std::string>& args) {
        return handleOSDFont(args);
    });
    registerAppCommand("osd/pos", [this](const std::vector<std::string>& args) {
        return handleOSDPos(args);
    });
    registerAppCommand("art/timescale", [this](const std::vector<std::string>& args) {
        if (args.size() >= 2) {
            return handleTimeScale2(args);  // timescale + offset
        } else {
            return handleTimeScale(args);   // timescale only
        }
    });
    registerAppCommand("art/loop", [this](const std::vector<std::string>& args) {
        return handleLoop(args);
    });
    registerAppCommand("art/reverse", [this](const std::vector<std::string>& args) {
        return handleReverse(args);
    });
    registerAppCommand("art/pan", [this](const std::vector<std::string>& args) {
        return handlePan(args);
    });

    // Register layer-level commands
    registerLayerCommand("seek", [this](VideoLayer* layer, const std::vector<std::string>& args) {
        return handleLayerSeek(layer, args);
    });
    registerLayerCommand("play", [this](VideoLayer* layer, const std::vector<std::string>& args) {
        return handleLayerPlay(layer, args);
    });
    registerLayerCommand("pause", [this](VideoLayer* layer, const std::vector<std::string>& args) {
        return handleLayerPause(layer, args);
    });
    registerLayerCommand("position", [this](VideoLayer* layer, const std::vector<std::string>& args) {
        return handleLayerPosition(layer, args);
    });
    registerLayerCommand("opacity", [this](VideoLayer* layer, const std::vector<std::string>& args) {
        return handleLayerOpacity(layer, args);
    });
    registerLayerCommand("visible", [this](VideoLayer* layer, const std::vector<std::string>& args) {
        return handleLayerVisible(layer, args);
    });
    registerLayerCommand("zorder", [this](VideoLayer* layer, const std::vector<std::string>& args) {
        return handleLayerZOrder(layer, args);
    });
    registerLayerCommand("timescale", [this](VideoLayer* layer, const std::vector<std::string>& args) {
        if (args.size() >= 2) {
            return handleLayerTimeScale2(layer, args);  // timescale + offset
        } else {
            return handleLayerTimeScale(layer, args);   // timescale only
        }
    });
    registerLayerCommand("loop", [this](VideoLayer* layer, const std::vector<std::string>& args) {
        return handleLayerLoop(layer, args);
    });
    registerLayerCommand("reverse", [this](VideoLayer* layer, const std::vector<std::string>& args) {
        return handleLayerReverse(layer, args);
    });
    registerLayerCommand("pan", [this](VideoLayer* layer, const std::vector<std::string>& args) {
        return handleLayerPan(layer, args);
    });
    registerLayerCommand("crop", [this](VideoLayer* layer, const std::vector<std::string>& args) {
        return handleLayerCrop(layer, args);
    });
    registerLayerCommand("panorama", [this](VideoLayer* layer, const std::vector<std::string>& args) {
        return handleLayerPanorama(layer, args);
    });
}

RemoteCommandRouter::~RemoteCommandRouter() {
}

bool RemoteCommandRouter::routeCommand(const std::string& path, const std::vector<std::string>& args) {
    // Remove leading /jadeo/ if present
    std::string cleanPath = path;
    if (cleanPath.find("/jadeo/") == 0) {
        cleanPath = cleanPath.substr(7); // Remove "/jadeo/"
    } else if (cleanPath.find("/jadeo") == 0) {
        cleanPath = cleanPath.substr(6); // Remove "/jadeo"
    }

    // Check if it's a layer-level command
    if (cleanPath.find("layer/") == 0) {
        std::string remaining = cleanPath.substr(6); // Remove "layer/"
        
        // Parse layer ID and command
        int layerId = -1;
        std::string command;
        
        size_t slashPos = remaining.find('/');
        if (slashPos != std::string::npos) {
            std::string layerIdStr = remaining.substr(0, slashPos);
            layerId = std::atoi(layerIdStr.c_str());
            command = remaining.substr(slashPos + 1);
        } else {
            // No layer ID specified - treat as app-level layer command
            command = remaining;
        }

        if (layerId >= 0) {
            // Layer-specific command
            VideoLayer* layer = layerManager_->getLayer(layerId);
            if (!layer) {
                return false;
            }

            auto it = layerCommands_.find(command);
            if (it != layerCommands_.end()) {
                return it->second(layer, args);
            }
        } else {
            // App-level layer management command
            auto it = appCommands_.find("layer/" + command);
            if (it != appCommands_.end()) {
                return it->second(args);
            }
        }
    } else {
        // App-level command
        auto it = appCommands_.find(cleanPath);
        if (it != appCommands_.end()) {
            return it->second(args);
        }
    }

    return false;
}

void RemoteCommandRouter::registerAppCommand(const std::string& path,
                                             std::function<bool(const std::vector<std::string>&)> handler) {
    appCommands_[path] = handler;
}

void RemoteCommandRouter::registerLayerCommand(const std::string& path,
                                               std::function<bool(VideoLayer*, const std::vector<std::string>&)> handler) {
    layerCommands_[path] = handler;
}

bool RemoteCommandRouter::parsePath(const std::string& path, std::string& command, int& layerId) {
    // This is handled in routeCommand now
    return false;
}

// App-level command handlers
bool RemoteCommandRouter::handleQuit(const std::vector<std::string>& args) {
    if (app_) {
        app_->quit();
    }
    return true;
}

bool RemoteCommandRouter::handleLoad(const std::vector<std::string>& args) {
    if (args.empty()) {
        return false;
    }

    // Create new layer with video file
    // This would be implemented to create a layer via XjadeoApplication
    // For now, placeholder
    return true;
}

bool RemoteCommandRouter::handleSeek(const std::vector<std::string>& args) {
    if (args.empty()) {
        return false;
    }

    int64_t frame = std::atoll(args[0].c_str());
    
    // Seek all layers or first layer
    if (layerManager_) {
        auto layers = layerManager_->getLayers();
        if (!layers.empty() && layers[0]) {
            return layers[0]->seek(frame);
        }
    }
    
    return false;
}

bool RemoteCommandRouter::handleFPS(const std::vector<std::string>& args) {
    if (args.empty() || !app_) {
        return false;
    }
    
    double fps = std::atof(args[0].c_str());
    return app_->setFPS(fps);
}

bool RemoteCommandRouter::handleOffset(const std::vector<std::string>& args) {
    if (args.empty() || !app_) {
        return false;
    }
    
    int64_t offset = std::atoll(args[0].c_str());
    return app_->setTimeOffset(offset);
}

bool RemoteCommandRouter::handleLayerAdd(const std::vector<std::string>& args) {
    if (args.empty()) {
        return false;
    }

    // Create new layer
    // This would be implemented via XjadeoApplication
    return true;
}

bool RemoteCommandRouter::handleLayerRemove(const std::vector<std::string>& args) {
    if (args.empty()) {
        return false;
    }

    int layerId = std::atoi(args[0].c_str());
    if (layerManager_) {
        return layerManager_->removeLayer(layerId);
    }
    return false;
}

bool RemoteCommandRouter::handleLayerDuplicate(const std::vector<std::string>& args) {
    if (args.empty()) {
        return false;
    }

    int layerId = std::atoi(args[0].c_str());
    if (layerManager_) {
        int newLayerId = -1;
        if (layerManager_->duplicateLayer(layerId, &newLayerId)) {
            // Could send response with new layer ID
            return true;
        }
    }
    return false;
}

bool RemoteCommandRouter::handleLayerReorder(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        return false;
    }

    int layerId = std::atoi(args[0].c_str());
    std::string action = args[1];
    
    if (layerManager_) {
        if (action == "top" || action == "front") {
            return layerManager_->moveLayerToTop(layerId);
        } else if (action == "bottom" || action == "back") {
            return layerManager_->moveLayerToBottom(layerId);
        } else if (action == "up") {
            return layerManager_->moveLayerUp(layerId);
        } else if (action == "down") {
            return layerManager_->moveLayerDown(layerId);
        } else {
            // Try to set specific z-order
            int zOrder = std::atoi(action.c_str());
            return layerManager_->setLayerZOrder(layerId, zOrder);
        }
    }
    return false;
}

bool RemoteCommandRouter::handleLayerList(const std::vector<std::string>& args) {
    if (!layerManager_) {
        return false;
    }

    // Get all layers sorted by z-order
    auto layers = layerManager_->getLayersSortedByZOrder();
    
    // Output layer list (for now, just return success)
    // In a full implementation, this would send OSC responses or use a callback
    // to report layer information
    
    return true;
}

// Layer-level command handlers
bool RemoteCommandRouter::handleLayerSeek(VideoLayer* layer, const std::vector<std::string>& args) {
    if (!layer || args.empty()) {
        return false;
    }

    int64_t frame = std::atoll(args[0].c_str());
    return layer->seek(frame);
}

bool RemoteCommandRouter::handleLayerPlay(VideoLayer* layer, const std::vector<std::string>& args) {
    if (!layer) {
        return false;
    }
    return layer->play();
}

bool RemoteCommandRouter::handleLayerPause(VideoLayer* layer, const std::vector<std::string>& args) {
    if (!layer) {
        return false;
    }
    return layer->pause();
}

bool RemoteCommandRouter::handleLayerPosition(VideoLayer* layer, const std::vector<std::string>& args) {
    if (!layer || args.size() < 2) {
        return false;
    }

    auto& props = layer->properties();
    props.x = std::atoi(args[0].c_str());
    props.y = std::atoi(args[1].c_str());
    return true;
}

bool RemoteCommandRouter::handleLayerOpacity(VideoLayer* layer, const std::vector<std::string>& args) {
    if (!layer || args.empty()) {
        return false;
    }

    float opacity = std::atof(args[0].c_str());
    opacity = std::max(0.0f, std::min(1.0f, opacity)); // Clamp to 0-1
    layer->properties().opacity = opacity;
    return true;
}

bool RemoteCommandRouter::handleLayerVisible(VideoLayer* layer, const std::vector<std::string>& args) {
    if (!layer || args.empty()) {
        return false;
    }

    int visible = std::atoi(args[0].c_str());
    layer->properties().visible = (visible != 0);
    return true;
}

bool RemoteCommandRouter::handleLayerZOrder(VideoLayer* layer, const std::vector<std::string>& args) {
    if (!layer || args.empty()) {
        return false;
    }

    int zOrder = std::atoi(args[0].c_str());
    if (layerManager_) {
        return layerManager_->setLayerZOrder(layer->getLayerId(), zOrder);
    }
    return false;
}

bool RemoteCommandRouter::handleOSDFrame(const std::vector<std::string>& args) {
    if (!app_) {
        return false;
    }
    
    OSDManager* osd = app_->getOSDManager();
    if (!osd) {
        return false;
    }
    
    if (args.empty()) {
        // Toggle frame display
        if (osd->isModeEnabled(OSDManager::FRAME)) {
            osd->disableMode(OSDManager::FRAME);
        } else {
            osd->enableMode(OSDManager::FRAME);
        }
    } else {
        int yPos = std::atoi(args[0].c_str());
        if (yPos < 0) {
            osd->disableMode(OSDManager::FRAME);
        } else if (yPos <= 100) {
            osd->enableMode(OSDManager::FRAME);
            osd->setFramePosition(1, yPos); // Center aligned by default
        } else {
            return false;
        }
    }
    return true;
}

bool RemoteCommandRouter::handleOSDSMPTE(const std::vector<std::string>& args) {
    if (!app_) {
        return false;
    }
    
    OSDManager* osd = app_->getOSDManager();
    if (!osd) {
        return false;
    }
    
    if (args.empty()) {
        // Toggle SMPTE display
        if (osd->isModeEnabled(OSDManager::SMPTE)) {
            osd->disableMode(OSDManager::SMPTE);
        } else {
            osd->enableMode(OSDManager::SMPTE);
        }
    } else {
        int yPos = std::atoi(args[0].c_str());
        if (yPos < 0) {
            osd->disableMode(OSDManager::SMPTE);
        } else if (yPos <= 100) {
            osd->enableMode(OSDManager::SMPTE);
            osd->setSMPTEPosition(1, yPos); // Center aligned by default
        } else {
            return false;
        }
    }
    return true;
}

bool RemoteCommandRouter::handleOSDText(const std::vector<std::string>& args) {
    if (!app_) {
        return false;
    }
    
    OSDManager* osd = app_->getOSDManager();
    if (!osd) {
        return false;
    }
    
    if (args.empty()) {
        // Toggle text display
        if (osd->isModeEnabled(OSDManager::TEXT)) {
            osd->disableMode(OSDManager::TEXT);
        } else {
            osd->enableMode(OSDManager::TEXT);
        }
    } else {
        // Set text
        std::string text;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) text += " ";
            text += args[i];
        }
        osd->setText(text);
    }
    return true;
}

bool RemoteCommandRouter::handleOSDBox(const std::vector<std::string>& args) {
    if (!app_) {
        return false;
    }
    
    OSDManager* osd = app_->getOSDManager();
    if (!osd) {
        return false;
    }
    
    if (args.empty()) {
        // Toggle box
        if (osd->isModeEnabled(OSDManager::BOX)) {
            osd->disableMode(OSDManager::BOX);
        } else {
            osd->enableMode(OSDManager::BOX);
        }
    } else {
        int enable = std::atoi(args[0].c_str());
        if (enable) {
            osd->enableMode(OSDManager::BOX);
        } else {
            osd->disableMode(OSDManager::BOX);
        }
    }
    return true;
}

bool RemoteCommandRouter::handleOSDFont(const std::vector<std::string>& args) {
    if (!app_ || args.empty()) {
        return false;
    }
    
    OSDManager* osd = app_->getOSDManager();
    if (!osd) {
        return false;
    }
    
    osd->setFontFile(args[0]);
    return true;
}

bool RemoteCommandRouter::handleOSDPos(const std::vector<std::string>& args) {
    if (!app_ || args.size() < 2) {
        return false;
    }
    
    OSDManager* osd = app_->getOSDManager();
    if (!osd) {
        return false;
    }
    
    int xAlign = std::atoi(args[0].c_str()); // 0=left, 1=center, 2=right
    int yPos = std::atoi(args[1].c_str());    // 0-100
    
    if (xAlign < 0 || xAlign > 2 || yPos < 0 || yPos > 100) {
        return false;
    }
    
    osd->setTextPosition(xAlign, yPos);
    return true;
}

bool RemoteCommandRouter::handleTimeScale(const std::vector<std::string>& args) {
    if (!app_ || args.empty()) {
        return false;
    }
    
    // Apply to first layer (app-level command)
    if (layerManager_ && layerManager_->getLayerCount() > 0) {
        auto layers = layerManager_->getLayers();
        if (!layers.empty() && layers[0]) {
            double scale = std::atof(args[0].c_str());
            layers[0]->setTimeScale(scale);
            return true;
        }
    }
    return false;
}

bool RemoteCommandRouter::handleTimeScale2(const std::vector<std::string>& args) {
    if (!app_ || args.size() < 2) {
        return false;
    }
    
    // Apply to first layer (app-level command)
    if (layerManager_ && layerManager_->getLayerCount() > 0) {
        auto layers = layerManager_->getLayers();
        if (!layers.empty() && layers[0]) {
            double scale = std::atof(args[0].c_str());
            int64_t offset = std::atoll(args[1].c_str());
            layers[0]->setTimeScale(scale);
            layers[0]->setTimeOffset(offset);
            return true;
        }
    }
    return false;
}

bool RemoteCommandRouter::handleLoop(const std::vector<std::string>& args) {
    if (!app_) {
        return false;
    }
    
    // Apply to first layer (app-level command)
    if (layerManager_ && layerManager_->getLayerCount() > 0) {
        auto layers = layerManager_->getLayers();
        if (!layers.empty() && layers[0]) {
            bool enabled = false;
            if (!args.empty()) {
                int val = std::atoi(args[0].c_str());
                enabled = (val != 0);
            }
            layers[0]->setWraparound(enabled);
            return true;
        }
    }
    return false;
}

bool RemoteCommandRouter::handleReverse(const std::vector<std::string>& args) {
    if (!app_) {
        return false;
    }
    
    // Apply to first layer (app-level command)
    if (layerManager_ && layerManager_->getLayerCount() > 0) {
        auto layers = layerManager_->getLayers();
        if (!layers.empty() && layers[0]) {
            layers[0]->reverse();
            return true;
        }
    }
    return false;
}

bool RemoteCommandRouter::handleLayerTimeScale(VideoLayer* layer, const std::vector<std::string>& args) {
    if (!layer || args.empty()) {
        return false;
    }
    
    double scale = std::atof(args[0].c_str());
    layer->setTimeScale(scale);
    return true;
}

bool RemoteCommandRouter::handleLayerTimeScale2(VideoLayer* layer, const std::vector<std::string>& args) {
    if (!layer || args.size() < 2) {
        return false;
    }
    
    double scale = std::atof(args[0].c_str());
    int64_t offset = std::atoll(args[1].c_str());
    layer->setTimeScale(scale);
    layer->setTimeOffset(offset);
    return true;
}

bool RemoteCommandRouter::handleLayerLoop(VideoLayer* layer, const std::vector<std::string>& args) {
    if (!layer) {
        return false;
    }
    
    bool enabled = false;
    if (!args.empty()) {
        int val = std::atoi(args[0].c_str());
        enabled = (val != 0);
    }
    layer->setWraparound(enabled);
    return true;
}

bool RemoteCommandRouter::handleLayerReverse(VideoLayer* layer, const std::vector<std::string>& args) {
    if (!layer) {
        return false;
    }
    
    layer->reverse();
    return true;
}

bool RemoteCommandRouter::handlePan(const std::vector<std::string>& args) {
    if (!app_ || args.empty()) {
        return false;
    }
    
    // Apply to first layer (app-level command)
    if (layerManager_ && layerManager_->getLayerCount() > 0) {
        auto layers = layerManager_->getLayers();
        if (!layers.empty() && layers[0]) {
            int panOffset = std::atoi(args[0].c_str());
            auto& props = layers[0]->properties();
            
            // Enable panorama mode if not already enabled
            if (!props.panoramaMode) {
                props.panoramaMode = true;
            }
            
            // Clamp pan offset to valid range
            FrameInfo info = layers[0]->getFrameInfo();
            int maxOffset = info.width / 2; // 50% of width in panorama mode
            if (panOffset < 0) panOffset = 0;
            if (panOffset > info.width - maxOffset) panOffset = info.width - maxOffset;
            
            props.panOffset = panOffset;
            return true;
        }
    }
    return false;
}

bool RemoteCommandRouter::handleLayerPan(VideoLayer* layer, const std::vector<std::string>& args) {
    if (!layer || args.empty()) {
        return false;
    }
    
    int panOffset = std::atoi(args[0].c_str());
    auto& props = layer->properties();
    
    // Enable panorama mode if not already enabled
    if (!props.panoramaMode) {
        props.panoramaMode = true;
    }
    
    // Clamp pan offset to valid range
    FrameInfo info = layer->getFrameInfo();
    int maxOffset = info.width / 2; // 50% of width in panorama mode
    if (panOffset < 0) panOffset = 0;
    if (panOffset > info.width - maxOffset) panOffset = info.width - maxOffset;
    
    props.panOffset = panOffset;
    return true;
}

bool RemoteCommandRouter::handleLayerCrop(VideoLayer* layer, const std::vector<std::string>& args) {
    if (!layer) {
        return false;
    }
    
    auto& props = layer->properties();
    
    if (args.empty()) {
        // Toggle crop
        props.crop.enabled = !props.crop.enabled;
    } else if (args.size() >= 4) {
        // Set crop rectangle: x, y, width, height
        props.crop.x = std::atoi(args[0].c_str());
        props.crop.y = std::atoi(args[1].c_str());
        props.crop.width = std::atoi(args[2].c_str());
        props.crop.height = std::atoi(args[3].c_str());
        props.crop.enabled = true;
    } else {
        return false;
    }
    
    return true;
}

bool RemoteCommandRouter::handleLayerPanorama(VideoLayer* layer, const std::vector<std::string>& args) {
    if (!layer) {
        return false;
    }
    
    auto& props = layer->properties();
    
    if (args.empty()) {
        // Toggle panorama mode
        props.panoramaMode = !props.panoramaMode;
    } else {
        // Enable/disable based on argument
        int enabled = std::atoi(args[0].c_str());
        props.panoramaMode = (enabled != 0);
    }
    
    return true;
}

} // namespace xjadeo

