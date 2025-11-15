#include "OpenGLRenderer.h"
#include "../layer/VideoLayer.h"
#include "../osd/OSDRenderer.h"
#include <cstring>

extern "C" {
#ifdef __APPLE__
#include "OpenGL/glu.h"
#else
#include <GL/glu.h>
#endif
#include <GL/gl.h>
}

namespace xjadeo {

OpenGLRenderer::OpenGLRenderer()
    : textureId_(0)
    , textureWidth_(0)
    , textureHeight_(0)
    , viewportWidth_(0)
    , viewportHeight_(0)
    , letterbox_(true)
    , initialized_(false)
{
}

OpenGLRenderer::~OpenGLRenderer() {
    cleanup();
}

bool OpenGLRenderer::init() {
    if (initialized_) {
        return true;
    }

    // Initialize OpenGL state
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Generate texture
    glGenTextures(1, &textureId_);
    if (textureId_ == 0) {
        return false;
    }

    initialized_ = true;
    return true;
}

void OpenGLRenderer::cleanup() {
    if (textureId_ != 0) {
        glDeleteTextures(1, &textureId_);
        textureId_ = 0;
    }
    initialized_ = false;
}

void OpenGLRenderer::setViewport(int x, int y, int width, int height) {
    viewportWidth_ = width;
    viewportHeight_ = height;
    glViewport(x, y, width, height);
    setupOrthoProjection();
}

void OpenGLRenderer::setupOrthoProjection() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

bool OpenGLRenderer::uploadFrameToTexture(const FrameBuffer& frame) {
    if (!frame.isValid() || textureId_ == 0) {
        return false;
    }

    const FrameInfo& info = frame.info();

    // Update texture if size changed
    if (textureWidth_ != info.width || textureHeight_ != info.height) {
        textureWidth_ = info.width;
        textureHeight_ = info.height;

        glBindTexture(GL_TEXTURE_2D, textureId_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Allocate texture storage
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 
                     textureWidth_, textureHeight_, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    }

    // Upload frame data
    glBindTexture(GL_TEXTURE_2D, textureId_);
    
    // Handle different pixel formats
    const FrameInfo& info = frame.info();
    GLenum format = GL_RGB;
    GLenum type = GL_UNSIGNED_BYTE;
    
    switch (info.format) {
        case PixelFormat::YUV420P:
            // For YUV420P, we need to convert to RGB or use shader
            // For now, simplified - would need proper YUV->RGB conversion
            // This is a placeholder - full implementation would use sws_scale
            // or GL shader for YUV->RGB conversion
            format = GL_LUMINANCE; // Temporary - should be RGB after conversion
            break;
        case PixelFormat::RGB24:
            format = GL_RGB;
            break;
        case PixelFormat::RGBA32:
            format = GL_RGBA;
            break;
        case PixelFormat::BGRA32:
            format = GL_BGRA;
            break;
        default:
            format = GL_RGB;
            break;
    }
    
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    textureWidth_, textureHeight_,
                    format, type, frame.data());

    return true;
}

void OpenGLRenderer::renderQuad(float x, float y, float width, float height) {
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(x + width, y);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(x + width, y + height);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y + height);
    glEnd();
}

void OpenGLRenderer::renderQuadWithCrop(float x, float y, float width, float height,
                                       float texX, float texY, float texWidth, float texHeight) {
    glBegin(GL_QUADS);
    glTexCoord2f(texX, texY); glVertex2f(x, y);
    glTexCoord2f(texX + texWidth, texY); glVertex2f(x + width, y);
    glTexCoord2f(texX + texWidth, texY + texHeight); glVertex2f(x + width, y + height);
    glTexCoord2f(texX, texY + texHeight); glVertex2f(x, y + height);
    glEnd();
}

void OpenGLRenderer::calculateCropCoordinates(const VideoLayer* layer, float& texX, float& texY, 
                                             float& texWidth, float& texHeight) {
    if (!layer) {
        return;
    }

    const auto& props = layer->properties();
    const FrameInfo& frameInfo = layer->getFrameInfo();
    
    if (frameInfo.width == 0 || frameInfo.height == 0) {
        return;
    }

    // Panorama mode: crop to 50% width with pan offset
    if (props.panoramaMode) {
        float cropWidth = frameInfo.width / 2.0f;
        float maxOffset = frameInfo.width - cropWidth;
        
        // Clamp pan offset
        int panOffset = props.panOffset;
        if (panOffset < 0) panOffset = 0;
        if (panOffset > static_cast<int>(maxOffset)) panOffset = static_cast<int>(maxOffset);
        
        // Calculate texture coordinates
        texX = static_cast<float>(panOffset) / frameInfo.width;
        texY = 0.0f;
        texWidth = cropWidth / frameInfo.width;
        texHeight = 1.0f;
    }
    // General crop
    else if (props.crop.enabled) {
        // Calculate crop rectangle in texture coordinates (0.0 to 1.0)
        texX = static_cast<float>(props.crop.x) / frameInfo.width;
        texY = static_cast<float>(props.crop.y) / frameInfo.height;
        texWidth = static_cast<float>(props.crop.width) / frameInfo.width;
        texHeight = static_cast<float>(props.crop.height) / frameInfo.height;
        
        // Clamp to valid range
        if (texX < 0.0f) texX = 0.0f;
        if (texY < 0.0f) texY = 0.0f;
        if (texX + texWidth > 1.0f) texWidth = 1.0f - texX;
        if (texY + texHeight > 1.0f) texHeight = 1.0f - texY;
    }
}

void OpenGLRenderer::applyLayerTransform(const VideoLayer* layer) {
    if (!layer) return;

    const auto& props = layer->properties();
    
    glPushMatrix();
    
    // Apply scale
    glScalef(props.scaleX, props.scaleY, 1.0f);
    
    // Apply rotation (around center)
    if (props.rotation != 0.0f) {
        glTranslatef(0.5f, 0.5f, 0.0f);
        glRotatef(props.rotation, 0.0f, 0.0f, 1.0f);
        glTranslatef(-0.5f, -0.5f, 0.0f);
    }
}

void OpenGLRenderer::applyBlendMode(const VideoLayer* layer) {
    if (!layer) return;

    const auto& props = layer->properties();
    
    switch (props.blendMode) {
        case LayerProperties::NORMAL:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case LayerProperties::MULTIPLY:
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            break;
        case LayerProperties::SCREEN:
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
            break;
        case LayerProperties::OVERLAY:
            // Simplified overlay blend
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
    }
}

bool OpenGLRenderer::renderLayer(const VideoLayer* layer) {
    if (!layer || !layer->isReady()) {
        return false;
    }

    const auto& props = layer->properties();
    if (!props.visible) {
        return false;
    }

    // Get layer's frame buffer
    const FrameBuffer& frameBuffer = layer->getFrameBuffer();
    if (!frameBuffer.isValid()) {
        return false;
    }

    // Upload frame to texture
    if (!uploadFrameToTexture(frameBuffer)) {
        return false;
    }

    // Apply layer transform
    applyLayerTransform(layer);

    // Apply blend mode
    applyBlendMode(layer);

    // Set opacity
    glColor4f(1.0f, 1.0f, 1.0f, props.opacity);

    // Calculate normalized coordinates
    float x = -1.0f + (2.0f * props.x / viewportWidth_);
    float y = 1.0f - (2.0f * props.y / viewportHeight_);
    float w = 2.0f * props.width / viewportWidth_;
    float h = 2.0f * props.height / viewportHeight_;

    // Calculate texture coordinates for cropping/panorama
    float texX = 0.0f, texY = 0.0f, texWidth = 1.0f, texHeight = 1.0f;
    calculateCropCoordinates(layer, texX, texY, texWidth, texHeight);

    // Render quad with crop
    if (props.crop.enabled || props.panoramaMode) {
        renderQuadWithCrop(x, y, w, h, texX, texY, texWidth, texHeight);
    } else {
        renderQuad(x, y, w, h);
    }

    glPopMatrix();

    return true;
}

void OpenGLRenderer::compositeLayers(const std::vector<const VideoLayer*>& layers) {
    glClear(GL_COLOR_BUFFER_BIT);

    // Render layers in z-order (already sorted by LayerManager)
    for (const VideoLayer* layer : layers) {
        if (layer && layer->isReady()) {
            renderLayer(layer);
        }
    }
}

void OpenGLRenderer::updateTexture(int width, int height) {
    textureWidth_ = width;
    textureHeight_ = height;
    
    if (textureId_ != 0) {
        glBindTexture(GL_TEXTURE_2D, textureId_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 
                     width, height, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    }
}

void OpenGLRenderer::renderOSDItems(const std::vector<OSDRenderItem>& items) {
    if (items.empty()) {
        return;
    }

    // Save current matrix state
    glPushMatrix();
    glLoadIdentity();

    // Enable blending for OSD
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render each OSD item
    for (const auto& item : items) {
        if (item.textureId == 0) {
            continue;
        }

        glBindTexture(GL_TEXTURE_2D, item.textureId);

        // Calculate normalized coordinates
        // OpenGL coordinates: -1 to 1, with origin at center
        // Screen coordinates: 0 to viewportWidth/Height, with origin at top-left
        float x = -1.0f + (2.0f * item.x / viewportWidth_);
        float y = 1.0f - (2.0f * item.y / viewportHeight_);
        float w = 2.0f * item.width / viewportWidth_;
        float h = -2.0f * item.height / viewportHeight_; // Negative because Y is flipped

        // Render textured quad
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(x + w, y);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(x + w, y + h);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y + h);
        glEnd();
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glPopMatrix();
}

} // namespace xjadeo

