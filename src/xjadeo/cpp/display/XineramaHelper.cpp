#include "XineramaHelper.h"

#if defined(HAVE_LIBXINERAMA) && (defined(PLATFORM_LINUX) || (!defined(PLATFORM_WINDOWS) && !defined(PLATFORM_OSX)))
#include <X11/extensions/Xinerama.h>
#include <X11/Xlib.h>
#define XINERAMA_AVAILABLE 1
#else
#define XINERAMA_AVAILABLE 0
#endif

namespace xjadeo {

XineramaHelper::XineramaHelper()
    : available_(false)
{
}

XineramaHelper::~XineramaHelper() {
    clearDisplays();
}

bool XineramaHelper::detectDisplays() {
    clearDisplays();

#if XINERAMA_AVAILABLE
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) {
        return false;
    }

    int eventBase, errorBase;
    if (!XineramaQueryExtension(dpy, &eventBase, &errorBase)) {
        XCloseDisplay(dpy);
        return false;
    }

    int screenCount = 0;
    XineramaScreenInfo* screens = XineramaQueryScreens(dpy, &screenCount);

    if (screens && screenCount > 0) {
        available_ = true;
        displays_.reserve(screenCount);

        for (int i = 0; i < screenCount; ++i) {
            DisplayInfo info;
            info.x = screens[i].x_org;
            info.y = screens[i].y_org;
            info.width = screens[i].width;
            info.height = screens[i].height;
            info.screen = i;
            info.primary = (i == 0); // First screen is typically primary

            displays_.push_back(info);
        }

        XFree(screens);
    } else {
        // Fallback: single display
        DisplayInfo info;
        info.x = 0;
        info.y = 0;
        info.width = DisplayWidth(dpy, DefaultScreen(dpy));
        info.height = DisplayHeight(dpy, DefaultScreen(dpy));
        info.screen = 0;
        info.primary = true;
        displays_.push_back(info);
    }

    XCloseDisplay(dpy);
    return true;
#else
    // No Xinerama support - single display
    DisplayInfo info;
    info.x = 0;
    info.y = 0;
    info.width = 1920;  // Default fallback
    info.height = 1080;
    info.screen = 0;
    info.primary = true;
    displays_.push_back(info);
    return true;
#endif
}

const DisplayInfo& XineramaHelper::getDisplay(size_t index) const {
    static DisplayInfo defaultInfo = {0, 0, 1920, 1080, 0, true};
    
    if (index < displays_.size()) {
        return displays_[index];
    }
    return defaultInfo;
}

bool XineramaHelper::getDisplayInfo(size_t index, DisplayInfo& info) const {
    if (index < displays_.size()) {
        info = displays_[index];
        return true;
    }
    return false;
}

const DisplayInfo* XineramaHelper::getPrimaryDisplay() const {
    for (const auto& display : displays_) {
        if (display.primary) {
            return &display;
        }
    }
    if (!displays_.empty()) {
        return &displays_[0];
    }
    return nullptr;
}

void XineramaHelper::getCombinedArea(int& x, int& y, int& width, int& height) const {
    if (displays_.empty()) {
        x = y = width = height = 0;
        return;
    }

    int minX = displays_[0].x;
    int minY = displays_[0].y;
    int maxX = displays_[0].x + displays_[0].width;
    int maxY = displays_[0].y + displays_[0].height;

    for (size_t i = 1; i < displays_.size(); ++i) {
        const auto& d = displays_[i];
        if (d.x < minX) minX = d.x;
        if (d.y < minY) minY = d.y;
        int dMaxX = d.x + d.width;
        int dMaxY = d.y + d.height;
        if (dMaxX > maxX) maxX = dMaxX;
        if (dMaxY > maxY) maxY = dMaxY;
    }

    x = minX;
    y = minY;
    width = maxX - minX;
    height = maxY - minY;
}

void XineramaHelper::clearDisplays() {
    displays_.clear();
    available_ = false;
}

} // namespace xjadeo

