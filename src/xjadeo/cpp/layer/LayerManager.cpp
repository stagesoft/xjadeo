#include "LayerManager.h"
#include <algorithm>

namespace xjadeo {

LayerManager::LayerManager()
    : nextLayerId_(1)
{
}

LayerManager::~LayerManager() {
    layers_.clear();
}

int LayerManager::addLayer(std::unique_ptr<VideoLayer> layer) {
    if (!layer) {
        return -1;
    }
    
    int layerId = nextLayerId_++;
    layer->setLayerId(layerId);
    layers_.push_back(std::move(layer));
    
    sortLayersByZOrder();
    return layerId;
}

bool LayerManager::removeLayer(int layerId) {
    auto it = std::find_if(layers_.begin(), layers_.end(),
        [layerId](const std::unique_ptr<VideoLayer>& layer) {
            return layer->getLayerId() == layerId;
        });
    
    if (it != layers_.end()) {
        layers_.erase(it);
        return true;
    }
    
    return false;
}

VideoLayer* LayerManager::getLayer(int layerId) {
    auto it = std::find_if(layers_.begin(), layers_.end(),
        [layerId](const std::unique_ptr<VideoLayer>& layer) {
            return layer->getLayerId() == layerId;
        });
    
    if (it != layers_.end()) {
        return it->get();
    }
    
    return nullptr;
}

const VideoLayer* LayerManager::getLayer(int layerId) const {
    auto it = std::find_if(layers_.begin(), layers_.end(),
        [layerId](const std::unique_ptr<VideoLayer>& layer) {
            return layer->getLayerId() == layerId;
        });
    
    if (it != layers_.end()) {
        return it->get();
    }
    
    return nullptr;
}

std::vector<VideoLayer*> LayerManager::getLayers() {
    std::vector<VideoLayer*> result;
    result.reserve(layers_.size());
    
    for (auto& layer : layers_) {
        result.push_back(layer.get());
    }
    
    return result;
}

std::vector<const VideoLayer*> LayerManager::getLayers() const {
    std::vector<const VideoLayer*> result;
    result.reserve(layers_.size());
    
    for (const auto& layer : layers_) {
        result.push_back(layer.get());
    }
    
    return result;
}

void LayerManager::updateAll() {
    for (auto& layer : layers_) {
        if (layer && layer->isReady()) {
            layer->update();
        }
    }
}

VideoLayer* LayerManager::getLayerByIndex(size_t index) {
    if (index < layers_.size()) {
        return layers_[index].get();
    }
    return nullptr;
}

const VideoLayer* LayerManager::getLayerByIndex(size_t index) const {
    if (index < layers_.size()) {
        return layers_[index].get();
    }
    return nullptr;
}

void LayerManager::sortLayersByZOrder() {
    std::sort(layers_.begin(), layers_.end(),
        [](const std::unique_ptr<VideoLayer>& a, const std::unique_ptr<VideoLayer>& b) {
            return a->properties().zOrder < b->properties().zOrder;
        });
}

int LayerManager::getNextZOrder() {
    if (layers_.empty()) {
        return 0;
    }
    int maxZOrder = layers_[0]->properties().zOrder;
    for (const auto& layer : layers_) {
        if (layer->properties().zOrder > maxZOrder) {
            maxZOrder = layer->properties().zOrder;
        }
    }
    return maxZOrder + 1;
}

bool LayerManager::setLayerZOrder(int layerId, int zOrder) {
    VideoLayer* layer = getLayer(layerId);
    if (!layer) {
        return false;
    }
    
    layer->properties().zOrder = zOrder;
    sortLayersByZOrder();
    return true;
}

bool LayerManager::duplicateLayer(int layerId, int* newLayerId) {
    VideoLayer* sourceLayer = getLayer(layerId);
    if (!sourceLayer) {
        return false;
    }

    // Create new layer with same input source
    // Note: This is a simplified duplication - we'd need to clone the input source
    // For now, we'll just create a new layer with the same properties
    // Full implementation would require cloning InputSource and SyncSource
    
    auto newLayer = std::make_unique<VideoLayer>();
    
    // Copy properties
    auto& newProps = newLayer->properties();
    const auto& sourceProps = sourceLayer->properties();
    newProps.x = sourceProps.x + 20; // Offset slightly
    newProps.y = sourceProps.y + 20;
    newProps.width = sourceProps.width;
    newProps.height = sourceProps.height;
    newProps.opacity = sourceProps.opacity;
    newProps.zOrder = getNextZOrder();
    newProps.visible = sourceProps.visible;
    newProps.scaleX = sourceProps.scaleX;
    newProps.scaleY = sourceProps.scaleY;
    newProps.rotation = sourceProps.rotation;
    newProps.blendMode = sourceProps.blendMode;

    // Note: We can't duplicate the input source easily without cloning
    // This would require InputSource to support cloning
    // For now, the duplicated layer won't have an input source
    
    int id = addLayer(std::move(newLayer));
    if (newLayerId) {
        *newLayerId = id;
    }
    return id >= 0;
}

bool LayerManager::moveLayerToTop(int layerId) {
    VideoLayer* layer = getLayer(layerId);
    if (!layer) {
        return false;
    }
    
    int maxZOrder = getNextZOrder() - 1;
    layer->properties().zOrder = maxZOrder + 1;
    sortLayersByZOrder();
    return true;
}

bool LayerManager::moveLayerToBottom(int layerId) {
    VideoLayer* layer = getLayer(layerId);
    if (!layer) {
        return false;
    }
    
    int minZOrder = 0;
    if (!layers_.empty()) {
        minZOrder = layers_[0]->properties().zOrder;
        for (const auto& l : layers_) {
            if (l->properties().zOrder < minZOrder) {
                minZOrder = l->properties().zOrder;
            }
        }
    }
    
    layer->properties().zOrder = minZOrder - 1;
    sortLayersByZOrder();
    return true;
}

bool LayerManager::moveLayerUp(int layerId) {
    VideoLayer* layer = getLayer(layerId);
    if (!layer) {
        return false;
    }
    
    int currentZOrder = layer->properties().zOrder;
    int nextZOrder = currentZOrder + 1;
    
    // Find the next highest z-order
    for (const auto& l : layers_) {
        if (l->getLayerId() != layerId && l->properties().zOrder > currentZOrder) {
            if (l->properties().zOrder < nextZOrder || nextZOrder == currentZOrder + 1) {
                nextZOrder = l->properties().zOrder + 1;
            }
        }
    }
    
    layer->properties().zOrder = nextZOrder;
    sortLayersByZOrder();
    return true;
}

bool LayerManager::moveLayerDown(int layerId) {
    VideoLayer* layer = getLayer(layerId);
    if (!layer) {
        return false;
    }
    
    int currentZOrder = layer->properties().zOrder;
    int nextZOrder = currentZOrder - 1;
    
    // Find the next lowest z-order
    for (const auto& l : layers_) {
        if (l->getLayerId() != layerId && l->properties().zOrder < currentZOrder) {
            if (l->properties().zOrder > nextZOrder || nextZOrder == currentZOrder - 1) {
                nextZOrder = l->properties().zOrder - 1;
            }
        }
    }
    
    layer->properties().zOrder = nextZOrder;
    sortLayersByZOrder();
    return true;
}

std::vector<VideoLayer*> LayerManager::getLayersSortedByZOrder() {
    std::vector<VideoLayer*> result;
    result.reserve(layers_.size());
    
    for (auto& layer : layers_) {
        result.push_back(layer.get());
    }
    
    // Sort by z-order
    std::sort(result.begin(), result.end(),
        [](VideoLayer* a, VideoLayer* b) {
            return a->properties().zOrder < b->properties().zOrder;
        });
    
    return result;
}

std::vector<const VideoLayer*> LayerManager::getLayersSortedByZOrder() const {
    std::vector<const VideoLayer*> result;
    result.reserve(layers_.size());
    
    for (const auto& layer : layers_) {
        result.push_back(layer.get());
    }
    
    // Sort by z-order
    std::sort(result.begin(), result.end(),
        [](const VideoLayer* a, const VideoLayer* b) {
            return a->properties().zOrder < b->properties().zOrder;
        });
    
    return result;
}

} // namespace xjadeo

