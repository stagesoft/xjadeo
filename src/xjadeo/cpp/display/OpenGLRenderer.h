#ifndef XJADEO_OPENGLRENDERER_H
#define XJADEO_OPENGLRENDERER_H

#include "../video/FrameBuffer.h"
#include "../layer/VideoLayer.h"
#include <vector>
#include <cstdint>

namespace xjadeo {

// Forward declaration
struct OSDRenderItem;

/**
 * OpenGLRenderer - Handles OpenGL rendering for layers
 * 
 * Manages shared OpenGL context, layer compositing, blending, and transforms.
 * This class handles the actual OpenGL rendering operations.
 */
class OpenGLRenderer {
public:
    OpenGLRenderer();
    ~OpenGLRenderer();

    // Initialize OpenGL state
    bool init();

    // Cleanup OpenGL resources
    void cleanup();

    // Render a single layer
    bool renderLayer(const VideoLayer* layer);

    // Composite all layers
    void compositeLayers(const std::vector<const VideoLayer*>& layers);

    // Set viewport
    void setViewport(int x, int y, int width, int height);

    // Update texture when video source changes
    void updateTexture(int width, int height);

    // Set letterbox mode
    void setLetterbox(bool enabled) { letterbox_ = enabled; }
    bool getLetterbox() const { return letterbox_; }

    // Render OSD items
    void renderOSDItems(const std::vector<struct OSDRenderItem>& items);

private:
    // OpenGL state
    unsigned int textureId_;
    int textureWidth_;
    int textureHeight_;
    int viewportWidth_;
    int viewportHeight_;
    bool letterbox_;
    bool initialized_;

    // Internal methods
    void setupOrthoProjection();
    void renderQuad(float x, float y, float width, float height);
    void renderQuadWithCrop(float x, float y, float width, float height,
                           float texX, float texY, float texWidth, float texHeight);
    void applyLayerTransform(const VideoLayer* layer);
    void applyBlendMode(const VideoLayer* layer);
    bool uploadFrameToTexture(const FrameBuffer& frame);
    void calculateCropCoordinates(const VideoLayer* layer, float& texX, float& texY, 
                                  float& texWidth, float& texHeight);
};

} // namespace xjadeo

#endif // XJADEO_OPENGLRENDERER_H

