#include "OSDRenderer.h"
#include "../utils/Logger.h"
#include <iostream>
#include <algorithm>
#include <cstring>

#ifdef HAVE_FT
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#endif

extern "C" {
#include <GL/gl.h>
}

namespace xjadeo {

OSDRenderer::OSDRenderer()
    : freetypeAvailable_(false)
    , initialized_(false)
    , ftLibrary_(nullptr)
    , ftFace_(nullptr)
    , fontSize_(DEFAULT_FONT_SIZE)
{
}

OSDRenderer::~OSDRenderer() {
    cleanup();
}

bool OSDRenderer::init() {
    if (initialized_) {
        return true;
    }

#ifdef HAVE_FT
    FT_Error error = FT_Init_FreeType(reinterpret_cast<FT_Library*>(&ftLibrary_));
    if (error == 0) {
        freetypeAvailable_ = true;
        initialized_ = true;
        return true;
    }
#endif

    // Fallback: no Freetype, but we can still work
    initialized_ = true;
    return true;
}

void OSDRenderer::cleanup() {
#ifdef HAVE_FT
    if (ftFace_) {
        FT_Done_Face(reinterpret_cast<FT_Face>(ftFace_));
        ftFace_ = nullptr;
    }
    if (ftLibrary_) {
        FT_Done_FreeType(reinterpret_cast<FT_Library>(ftLibrary_));
        ftLibrary_ = nullptr;
    }
#endif
    initialized_ = false;
    freetypeAvailable_ = false;
}

bool OSDRenderer::loadFont(const std::string& fontFile) {
#ifdef HAVE_FT
    if (!freetypeAvailable_ || !ftLibrary_) {
        return false;
    }

    // Don't reload if it's the same font
    if (fontFile == currentFontFile_ && ftFace_) {
        return true;
    }

    if (ftFace_) {
        FT_Done_Face(reinterpret_cast<FT_Face>(ftFace_));
        ftFace_ = nullptr;
    }

    FT_Error error;
    
    // Try to load font file
    if (fontFile.empty()) {
        // Try default font path
#ifdef SHAREDIR
        const char* defaultFont = SHAREDIR "/xjadeo/ArdourMono.ttf";
        error = FT_New_Face(reinterpret_cast<FT_Library>(ftLibrary_),
                           defaultFont, 0,
                           reinterpret_cast<FT_Face*>(&ftFace_));
        // If default fails, try common system font paths
        if (error) {
            const char* systemFonts[] = {
                "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
                "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
                "/System/Library/Fonts/Monaco.ttf",
                nullptr
            };
            for (int i = 0; systemFonts[i] && error; ++i) {
                error = FT_New_Face(reinterpret_cast<FT_Library>(ftLibrary_),
                                   systemFonts[i], 0,
                                   reinterpret_cast<FT_Face*>(&ftFace_));
            }
        }
#else
        // No default font path, return error
        return false;
#endif
    } else {
        error = FT_New_Face(reinterpret_cast<FT_Library>(ftLibrary_),
                           fontFile.c_str(), 0,
                           reinterpret_cast<FT_Face*>(&ftFace_));
    }

    if (error) {
        LOG_WARNING << "Failed to load font: " << (fontFile.empty() ? "default" : fontFile);
        return false;
    }

    // Set character size
    error = FT_Set_Char_Size(reinterpret_cast<FT_Face>(ftFace_), 0, fontSize_ * 64, 0, 72);
    if (error) {
        FT_Done_Face(reinterpret_cast<FT_Face>(ftFace_));
        ftFace_ = nullptr;
        LOG_WARNING << "Failed to set font size";
        return false;
    }

    currentFontFile_ = fontFile;
    return true;
#else
    return false;
#endif
}

void OSDRenderer::measureText(const std::string& text, int& width, int& height) {
    width = 0;
    height = 0;

#ifdef HAVE_FT
    if (!freetypeAvailable_ || !ftFace_) {
        // Fallback: estimate based on character count
        width = static_cast<int>(text.length() * fontSize_ * 0.6);
        height = fontSize_;
        return;
    }

    FT_Face face = reinterpret_cast<FT_Face>(ftFace_);
    FT_GlyphSlot slot = face->glyph;
    int maxHeight = 0;
    int currentWidth = 0;

    for (size_t n = 0; n < text.length(); ++n) {
        FT_Error error = FT_Load_Char(face, text[n], FT_LOAD_RENDER);
        if (error) continue;

        currentWidth += slot->advance.x >> 6;
        if (slot->bitmap.rows > maxHeight) {
            maxHeight = slot->bitmap.rows;
        }
    }

    width = currentWidth;
    height = maxHeight > 0 ? maxHeight : fontSize_;
#else
    // Fallback: estimate
    width = static_cast<int>(text.length() * fontSize_ * 0.6);
    height = fontSize_;
#endif
}

unsigned int OSDRenderer::renderTextToTexture(const std::string& text, int& width, int& height, bool createBox) {
    if (text.empty()) {
        width = 0;
        height = 0;
        return 0;
    }

#ifdef HAVE_FT
    if (!freetypeAvailable_ || !ftFace_) {
        // Try to load default font if not loaded
        if (!loadFont("")) {
            width = 0;
            height = 0;
            return 0;
        }
    }

    FT_Face face = reinterpret_cast<FT_Face>(ftFace_);
    FT_GlyphSlot slot = face->glyph;

    // Measure text first
    measureText(text, width, height);

    if (width == 0 || height == 0) {
        return 0;
    }

    // Add padding for box if needed
    int padding = createBox ? 8 : 4;
    int texWidth = width + padding * 2;
    int texHeight = height + padding * 2;

    // Create bitmap buffer (RGBA)
    std::vector<unsigned char> bitmap(texWidth * texHeight * 4, 0);

    // Fill with black box if requested
    if (createBox) {
        for (int y = 0; y < texHeight; ++y) {
            for (int x = 0; x < texWidth; ++x) {
                int idx = (y * texWidth + x) * 4;
                bitmap[idx] = 0;     // R
                bitmap[idx + 1] = 0; // G
                bitmap[idx + 2] = 0; // B
                bitmap[idx + 3] = 180; // A (semi-transparent black box)
            }
        }
    }

    // Render text
    int penX = padding;
    int penY = padding + height; // Baseline

    for (size_t n = 0; n < text.length(); ++n) {
        FT_Error error = FT_Load_Char(face, text[n], FT_LOAD_RENDER);
        if (error) continue;

        FT_Bitmap* bmp = &slot->bitmap;
        int x = penX + slot->bitmap_left;
        int y = penY - slot->bitmap_top;

        // Draw bitmap
        for (int row = 0; row < static_cast<int>(bmp->rows); ++row) {
            for (int col = 0; col < static_cast<int>(bmp->width); ++col) {
                int px = x + col;
                int py = y + row;

                if (px >= 0 && px < texWidth && py >= 0 && py < texHeight) {
                    unsigned char alpha = bmp->buffer[row * bmp->pitch + col];
                    int idx = (py * texWidth + px) * 4;

                    // White text
                    bitmap[idx] = 255;     // R
                    bitmap[idx + 1] = 255; // G
                    bitmap[idx + 2] = 255; // B
                    bitmap[idx + 3] = alpha; // A
                }
            }
        }

        penX += slot->advance.x >> 6;
    }

    // Create OpenGL texture
    unsigned int textureId;
    glGenTextures(1, &textureId);
    if (textureId == 0) {
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, bitmap.data());

    glBindTexture(GL_TEXTURE_2D, 0);

    return textureId;
#else
    // No Freetype - return 0
    width = 0;
    height = 0;
    return 0;
#endif
}

void OSDRenderer::cleanupTexture(unsigned int textureId) {
    if (textureId != 0) {
        glDeleteTextures(1, &textureId);
    }
}

std::vector<OSDRenderItem> OSDRenderer::prepareOSDRender(OSDManager* osd, int windowWidth, int windowHeight) {
    std::vector<OSDRenderItem> items;

    if (!osd || !initialized_ || windowWidth <= 0 || windowHeight <= 0) {
        return items;
    }

    // Load font if needed
    std::string fontFile = osd->getFontFile();
    if (!fontFile.empty() || !ftFace_) {
        loadFont(fontFile);
    }

    // Render frame number if enabled
    if (osd->isModeEnabled(OSDManager::FRAME) && !osd->getFrameText().empty()) {
        int textWidth, textHeight;
        bool hasBox = osd->isModeEnabled(OSDManager::BOX);
        unsigned int texId = renderTextToTexture(osd->getFrameText(), textWidth, textHeight, hasBox);
        
        if (texId != 0) {
            OSDRenderItem item;
            item.textureId = texId;
            item.width = textWidth;
            item.height = textHeight;
            item.hasBox = hasBox;
            item.y = calculateYPosition(osd->getFrameYPercent(), textHeight, windowHeight);
            item.x = calculateXPosition(osd->getFrameXAlign(), textWidth, windowWidth);
            items.push_back(item);
        }
    }

    // Render SMPTE timecode if enabled
    if (osd->isModeEnabled(OSDManager::SMPTE) && !osd->getSMPTETimecode().empty()) {
        int textWidth, textHeight;
        bool hasBox = osd->isModeEnabled(OSDManager::BOX);
        unsigned int texId = renderTextToTexture(osd->getSMPTETimecode(), textWidth, textHeight, hasBox);
        
        if (texId != 0) {
            OSDRenderItem item;
            item.textureId = texId;
            item.width = textWidth;
            item.height = textHeight;
            item.hasBox = hasBox;
            item.y = calculateYPosition(osd->getSMPTEYPercent(), textHeight, windowHeight);
            item.x = calculateXPosition(osd->getSMPTEXAlign(), textWidth, windowWidth);
            items.push_back(item);
        }
    }

    // Render custom text if enabled
    if (osd->isModeEnabled(OSDManager::TEXT) && !osd->getText().empty()) {
        int textWidth, textHeight;
        bool hasBox = osd->isModeEnabled(OSDManager::BOX);
        unsigned int texId = renderTextToTexture(osd->getText(), textWidth, textHeight, hasBox);
        
        if (texId != 0) {
            OSDRenderItem item;
            item.textureId = texId;
            item.width = textWidth;
            item.height = textHeight;
            item.hasBox = hasBox;
            item.y = calculateYPosition(osd->getTextYPercent(), textHeight, windowHeight);
            item.x = calculateXPosition(osd->getTextXAlign(), textWidth, windowWidth);
            items.push_back(item);
        }
    }

    // Render message if enabled
    if (osd->isModeEnabled(OSDManager::MSG) && !osd->getMessage().empty()) {
        int textWidth, textHeight;
        bool hasBox = osd->isModeEnabled(OSDManager::BOX);
        unsigned int texId = renderTextToTexture(osd->getMessage(), textWidth, textHeight, hasBox);
        
        if (texId != 0) {
            OSDRenderItem item;
            item.textureId = texId;
            item.width = textWidth;
            item.height = textHeight;
            item.hasBox = hasBox;
            item.y = calculateYPosition(50, textHeight, windowHeight); // Center
            item.x = calculateXPosition(1, textWidth, windowWidth); // Center
            items.push_back(item);
        }
    }

    return items;
}

int OSDRenderer::calculateXPosition(int xAlign, int textWidth, int windowWidth) {
    switch (xAlign) {
        case 0:  // Left
            return 10;
        case 1:  // Center
            return (windowWidth - textWidth) / 2;
        case 2:  // Right
            return windowWidth - textWidth - 10;
        default:
            return 10;
    }
}

int OSDRenderer::calculateYPosition(int yPercent, int textHeight, int windowHeight) {
    // yPercent is 0-100, where 0 is top and 100 is bottom
    int pixelY = (windowHeight * yPercent) / 100;
    // Adjust for text height (anchor at top of text)
    return pixelY;
}

} // namespace xjadeo
