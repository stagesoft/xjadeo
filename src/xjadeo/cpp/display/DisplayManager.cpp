#include "DisplayManager.h"

namespace xjadeo {

DisplayManager::DisplayManager()
    : displaysDetected_(false)
{
}

DisplayManager::~DisplayManager() {
}

bool DisplayManager::detectDisplays() {
    if (xineramaHelper_.detectDisplays()) {
        displays_.clear();
        size_t count = xineramaHelper_.getDisplayCount();
        displays_.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            DisplayInfo info;
            if (xineramaHelper_.getDisplayInfo(i, info)) {
                displays_.push_back(info);
            }
        }

        displaysDetected_ = true;
        return true;
    }

    return false;
}

bool DisplayManager::createWindows(DisplayBackend* backend, int mode) {
    if (!backend || !displaysDetected_) {
        return false;
    }

    if (mode == 0) {
        // Single window spanning all displays
        int x, y, width, height;
        xineramaHelper_.getCombinedArea(x, y, width, height);
        
        // Set position and size, then open
        backend->setPosition(x, y);
        backend->resize(width, height);
        return backend->openWindow();
    } else {
        // Individual windows per display
        // For now, just create one window on primary display
        // Full implementation would create multiple windows
        const DisplayInfo* primary = getPrimaryDisplay();
        if (primary) {
            backend->setPosition(primary->x, primary->y);
            backend->resize(primary->width, primary->height);
            return backend->openWindow();
        }
    }

    return false;
}

const DisplayInfo& DisplayManager::getDisplay(size_t index) const {
    static DisplayInfo defaultInfo = {0, 0, 1920, 1080, 0, true};
    
    if (index < displays_.size()) {
        return displays_[index];
    }
    return defaultInfo;
}

bool DisplayManager::getDisplayInfo(size_t index, DisplayInfo& info) const {
    if (index < displays_.size()) {
        info = displays_[index];
        return true;
    }
    return false;
}

const DisplayInfo* DisplayManager::getPrimaryDisplay() const {
    return xineramaHelper_.getPrimaryDisplay();
}

} // namespace xjadeo

