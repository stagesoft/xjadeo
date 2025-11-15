#ifndef XJADEO_XINERAMAHELPER_H
#define XJADEO_XINERAMAHELPER_H

#include <vector>
#include <cstdint>

namespace xjadeo {

/**
 * DisplayInfo - Information about a single display
 */
struct DisplayInfo {
    int x;          // X position
    int y;          // Y position
    int width;      // Width
    int height;     // Height
    int screen;     // Screen number
    bool primary;   // Is this the primary display?
};

/**
 * XineramaHelper - Xinerama detection and management
 * 
 * Detects and manages multi-display setups using Xinerama extension.
 * Supports both single window spanning multiple monitors and
 * individual windows per display.
 */
class XineramaHelper {
public:
    XineramaHelper();
    ~XineramaHelper();

    // Detect displays using Xinerama
    bool detectDisplays();

    // Get display count
    size_t getDisplayCount() const { return displays_.size(); }

    // Get display info
    const DisplayInfo& getDisplay(size_t index) const;
    bool getDisplayInfo(size_t index, DisplayInfo& info) const;

    // Get primary display
    const DisplayInfo* getPrimaryDisplay() const;

    // Check if Xinerama is available
    bool isAvailable() const { return available_; }

    // Get combined display area (all displays)
    void getCombinedArea(int& x, int& y, int& width, int& height) const;

private:
    std::vector<DisplayInfo> displays_;
    bool available_;
    
    void clearDisplays();
};

} // namespace xjadeo

#endif // XJADEO_XINERAMAHELPER_H

