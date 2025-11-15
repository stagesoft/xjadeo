#ifndef XJADEO_OSDRENDERER_H
#define XJADEO_OSDRENDERER_H

#include "OSDManager.h"
#include <string>
#include <memory>
#include <vector>
#include <cstdint>

namespace xjadeo {

/**
 * OSDRenderer - Renders OSD text using Freetype to OpenGL textures
 * 
 * This class handles the actual rendering of OSD elements onto the display.
 * It renders text using Freetype to bitmaps, then creates OpenGL textures
 * that can be composited by OpenGLRenderer.
 */
struct OSDRenderItem {
    unsigned int textureId;
    int x, y;
    int width, height;
    bool hasBox;  // Draw black box background
};

class OSDRenderer {
public:
    OSDRenderer();
    ~OSDRenderer();

    // Initialize renderer
    bool init();
    
    // Cleanup
    void cleanup();

    // Prepare OSD elements for rendering
    // Returns a list of render items that OpenGLRenderer can render
    std::vector<OSDRenderItem> prepareOSDRender(OSDManager* osd, int windowWidth, int windowHeight);

    // Check if Freetype is available
    bool isFreetypeAvailable() const { return freetypeAvailable_; }

    // Load font file
    bool loadFont(const std::string& fontFile);

private:
    bool freetypeAvailable_;
    bool initialized_;
    
    // Freetype state (if available)
    void* ftLibrary_;  // FT_Library
    void* ftFace_;     // FT_Face
    std::string currentFontFile_;
    int fontSize_;
    
    // Text rendering to texture
    unsigned int renderTextToTexture(const std::string& text, int& width, int& height, bool createBox);
    void cleanupTexture(unsigned int textureId);
    
    // Calculate text position based on alignment
    int calculateXPosition(int xAlign, int textWidth, int windowWidth);
    int calculateYPosition(int yPercent, int textHeight, int windowHeight);
    
    // Measure text dimensions
    void measureText(const std::string& text, int& width, int& height);
    
    // Default font size
    static const int DEFAULT_FONT_SIZE = 24;
};

} // namespace xjadeo

#endif // XJADEO_OSDRENDERER_H

