#ifndef XJADEO_LAYERPROPERTIES_H
#define XJADEO_LAYERPROPERTIES_H

namespace xjadeo {

/**
 * LayerProperties - Display properties for a video layer
 */
struct LayerProperties {
    int x = 0;              // Position X
    int y = 0;              // Position Y
    int width = 0;          // Width
    int height = 0;         // Height
    float opacity = 1.0f;   // Opacity (0.0 - 1.0)
    int zOrder = 0;         // Layer stacking order
    bool visible = true;    // Show/hide layer
    
    // Transform (simplified for now, can be extended)
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float rotation = 0.0f;
    
    // Crop rectangle
    struct {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        bool enabled = false;  // Enable cropping
    } crop;
    
    // Panorama mode (50% width crop with x-offset for panning)
    bool panoramaMode = false;  // Enable panorama mode (crops to 50% width)
    int panOffset = 0;          // X-offset for panning in panorama mode (0 to movie_width)
    
    // Blend mode (simplified enum for now)
    enum BlendMode {
        NORMAL = 0,
        MULTIPLY,
        SCREEN,
        OVERLAY
    };
    BlendMode blendMode = NORMAL;
};

} // namespace xjadeo

#endif // XJADEO_LAYERPROPERTIES_H

