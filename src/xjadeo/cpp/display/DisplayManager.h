#ifndef XJADEO_DISPLAYMANAGER_H
#define XJADEO_DISPLAYMANAGER_H

#include "DisplayBackend.h"
#include "XineramaHelper.h"
#include <vector>
#include <memory>

namespace xjadeo {

/**
 * DisplayManager - Manages multiple output displays
 * 
 * Supports:
 * - Single window spanning multiple monitors (Xinerama/Wayland)
 * - Individual windows per display
 */
class DisplayManager {
public:
    DisplayManager();
    ~DisplayManager();

    // Detect available displays
    bool detectDisplays();

    // Get display count
    size_t getDisplayCount() const { return displays_.size(); }

    // Create window for display
    // mode: 0 = single window spanning all, 1 = individual windows
    bool createWindows(DisplayBackend* backend, int mode = 0);

    // Get display info
    const DisplayInfo& getDisplay(size_t index) const;
    bool getDisplayInfo(size_t index, DisplayInfo& info) const;

    // Get primary display
    const DisplayInfo* getPrimaryDisplay() const;

private:
    XineramaHelper xineramaHelper_;
    std::vector<DisplayInfo> displays_;
    bool displaysDetected_;
};

} // namespace xjadeo

#endif // XJADEO_DISPLAYMANAGER_H

