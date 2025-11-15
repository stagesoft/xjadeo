#include "OpenGLDisplay.h"
#include "../layer/LayerManager.h"
#include "../osd/OSDManager.h"
#include "../osd/OSDRenderer.h"
#include "../utils/Logger.h"
#include "../../../config.h"
#include <iostream>

#if defined(PLATFORM_LINUX) || (!defined(PLATFORM_WINDOWS) && !defined(PLATFORM_OSX))
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#define USE_GLX 1
#elif defined(PLATFORM_WINDOWS)
#include <windows.h>
#include <GL/gl.h>
#define USE_WGL 1
#elif defined(PLATFORM_OSX)
// OSX includes
#define USE_CGL 1
#endif

namespace xjadeo {

OpenGLDisplay::OpenGLDisplay()
    : renderer_(std::make_unique<OpenGLRenderer>())
    , osdRenderer_(std::make_unique<OSDRenderer>())
#if defined(PLATFORM_LINUX) || (!defined(PLATFORM_WINDOWS) && !defined(PLATFORM_OSX))
    , display_(nullptr)
    , window_(0)
    , context_(nullptr)
    , screen_(0)
#elif defined(PLATFORM_WINDOWS)
    , hdc_(nullptr)
    , hwnd_(nullptr)
    , hglrc_(nullptr)
#elif defined(PLATFORM_OSX)
    , nsContext_(nullptr)
#endif
    , windowWidth_(640)
    , windowHeight_(360)
    , windowX_(0)
    , windowY_(0)
    , fullscreen_(false)
    , ontop_(false)
    , windowOpen_(false)
{
}

OpenGLDisplay::~OpenGLDisplay() {
    closeWindow();
}

bool OpenGLDisplay::openWindow() {
    if (windowOpen_) {
        return true;
    }

#if defined(USE_GLX)
    if (!initGLX()) {
        LOG_ERROR << "Failed to initialize GLX";
        return false;
    }
#elif defined(USE_WGL)
    if (!initWGL()) {
        return false;
    }
#elif defined(USE_CGL)
    if (!initCGL()) {
        return false;
    }
#else
    std::cerr << "No OpenGL implementation available for this platform" << std::endl;
    return false;
#endif

    makeCurrent();

    // Initialize renderer
    if (!renderer_->init()) {
        LOG_ERROR << "Failed to initialize OpenGL renderer";
        clearCurrent();
        closeWindow();
        return false;
    }

    // Initialize OSD renderer
    if (!osdRenderer_->init()) {
        LOG_WARNING << "Failed to initialize OSD renderer (OSD will be disabled)";
        // Don't fail window creation if OSD fails
    }

    clearCurrent();
    windowOpen_ = true;
    return true;
}

void OpenGLDisplay::closeWindow() {
    if (!windowOpen_) {
        return;
    }

    makeCurrent();
    renderer_->cleanup();
    if (osdRenderer_) {
        osdRenderer_->cleanup();
    }
    clearCurrent();

#if defined(USE_GLX)
    cleanupGLX();
#elif defined(USE_WGL)
    cleanupWGL();
#elif defined(USE_CGL)
    cleanupCGL();
#endif

    windowOpen_ = false;
}

bool OpenGLDisplay::isWindowOpen() const {
    return windowOpen_;
}

void OpenGLDisplay::render(LayerManager* layerManager, OSDManager* osdManager) {
    if (!windowOpen_ || !layerManager) {
        return;
    }

    makeCurrent();

    // Get all layers sorted by z-order
    auto layers = layerManager->getLayersSortedByZOrder();
    
    // Convert to const vector for rendering
    std::vector<const VideoLayer*> constLayers;
    constLayers.reserve(layers.size());
    for (auto* layer : layers) {
        constLayers.push_back(layer);
    }

    // Set viewport
    renderer_->setViewport(0, 0, windowWidth_, windowHeight_);

    // Composite all layers
    renderer_->compositeLayers(constLayers);

    // Render OSD if available
    if (osdManager && osdRenderer_) {
        auto osdItems = osdRenderer_->prepareOSDRender(osdManager, windowWidth_, windowHeight_);
        if (!osdItems.empty()) {
            renderer_->renderOSDItems(osdItems);
            
            // Cleanup OSD textures after rendering
            for (const auto& item : osdItems) {
                if (item.textureId != 0) {
                    glDeleteTextures(1, &item.textureId);
                }
            }
        }
    }

    // Swap buffers
    swapBuffers();
    clearCurrent();
}

void OpenGLDisplay::handleEvents() {
    if (!windowOpen_) {
        return;
    }

#if defined(USE_GLX)
    handleEventsGLX();
#elif defined(USE_WGL)
    handleEventsWGL();
#elif defined(USE_CGL)
    handleEventsCGL();
#endif
}

void OpenGLDisplay::resize(unsigned int width, unsigned int height) {
    windowWidth_ = width;
    windowHeight_ = height;

    if (windowOpen_) {
        makeCurrent();
        renderer_->setViewport(0, 0, width, height);
        clearCurrent();
    }
}

void OpenGLDisplay::getWindowSize(unsigned int* width, unsigned int* height) const {
    if (width) *width = windowWidth_;
    if (height) *height = windowHeight_;
}

void OpenGLDisplay::setPosition(int x, int y) {
    windowX_ = x;
    windowY_ = y;
    // Platform-specific implementation would move window here
}

void OpenGLDisplay::getWindowPos(int* x, int* y) const {
    if (x) *x = windowX_;
    if (y) *y = windowY_;
}

void OpenGLDisplay::setFullscreen(int action) {
    bool newState = fullscreen_;
    
    if (action == 0) {
        newState = false;
    } else if (action == 1) {
        newState = true;
    } else if (action == 2) {
        newState = !fullscreen_;
    }

    if (newState != fullscreen_) {
        fullscreen_ = newState;
        // Platform-specific fullscreen toggle would go here
    }
}

bool OpenGLDisplay::getFullscreen() const {
    return fullscreen_;
}

void OpenGLDisplay::setOnTop(int action) {
    bool newState = ontop_;
    
    if (action == 0) {
        newState = false;
    } else if (action == 1) {
        newState = true;
    } else if (action == 2) {
        newState = !ontop_;
    }

    if (newState != ontop_) {
        ontop_ = newState;
        // Platform-specific on-top toggle would go here
    }
}

bool OpenGLDisplay::getOnTop() const {
    return ontop_;
}

void* OpenGLDisplay::getContext() {
#if defined(USE_GLX)
    return context_;
#elif defined(USE_WGL)
    return hglrc_;
#elif defined(USE_CGL)
    return nsContext_;
#else
    return nullptr;
#endif
}

// Platform-specific implementations
#if defined(USE_GLX)
bool OpenGLDisplay::initGLX() {
    display_ = XOpenDisplay(nullptr);
    if (!display_) {
        std::cerr << "Cannot open X display" << std::endl;
        return false;
    }

    screen_ = DefaultScreen(display_);

    // Get GLX visual
    int attribs[] = {
        GLX_RGBA,
        GLX_DOUBLEBUFFER,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        None
    };

    XVisualInfo* vi = glXChooseVisual(display_, screen_, attribs);
    if (!vi) {
        XCloseDisplay(display_);
        display_ = nullptr;
        return false;
    }

    // Create window
    Window root = RootWindow(display_, screen_);
    Colormap cmap = XCreateColormap(display_, root, vi->visual, AllocNone);

    XSetWindowAttributes attr;
    attr.colormap = cmap;
    attr.border_pixel = 0;
    attr.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | 
                      ButtonReleaseMask | StructureNotifyMask;

    window_ = XCreateWindow(display_, root,
                           windowX_, windowY_, windowWidth_, windowHeight_,
                           0, vi->depth, InputOutput, vi->visual,
                           CWBorderPixel | CWColormap | CWEventMask, &attr);

    if (!window_) {
        XFree(vi);
        XCloseDisplay(display_);
        display_ = nullptr;
        return false;
    }

    XStoreName(display_, window_, "xjadeo");
    
    Atom wmDelete = XInternAtom(display_, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display_, window_, &wmDelete, 1);

    // Create GLX context
    context_ = glXCreateContext(display_, vi, nullptr, GL_TRUE);
    XFree(vi);

    if (!context_) {
        XDestroyWindow(display_, window_);
        XCloseDisplay(display_);
        display_ = nullptr;
        window_ = 0;
        return false;
    }

    XMapRaised(display_, window_);
    XSync(display_, False);

    return true;
}

void OpenGLDisplay::cleanupGLX() {
    if (context_) {
        glXDestroyContext(display_, context_);
        context_ = nullptr;
    }
    if (window_) {
        XDestroyWindow(display_, window_);
        window_ = 0;
    }
    if (display_) {
        XCloseDisplay(display_);
        display_ = nullptr;
    }
}

void OpenGLDisplay::handleEventsGLX() {
    if (!display_) return;

    XEvent event;
    while (XPending(display_)) {
        XNextEvent(display_, &event);

        switch (event.type) {
            case Expose:
                // Trigger redraw
                break;
            case KeyPress: {
                KeySym keysym = XLookupKeysym(&event.xkey, 0);
                if (keysym == XK_Escape) {
                    // Quit signal - would be handled by application
                }
                break;
            }
            case ClientMessage: {
                Atom wmDelete = XInternAtom(display_, "WM_DELETE_WINDOW", False);
                if (event.xclient.data.l[0] == static_cast<long>(wmDelete)) {
                    // Window close - would be handled by application
                }
                break;
            }
            case ConfigureNotify:
                windowWidth_ = event.xconfigure.width;
                windowHeight_ = event.xconfigure.height;
                break;
        }
    }
}

void OpenGLDisplay::makeCurrent() {
#if defined(USE_GLX)
    if (display_ && window_ && context_) {
        glXMakeCurrent(display_, window_, context_);
    }
#elif defined(USE_WGL)
    if (hdc_ && hglrc_) {
        wglMakeCurrent(hdc_, hglrc_);
    }
#elif defined(USE_CGL)
    // OSX make current
#endif
}

void OpenGLDisplay::clearCurrent() {
#if defined(USE_GLX)
    if (display_) {
        glXMakeCurrent(display_, None, nullptr);
    }
#elif defined(USE_WGL)
    wglMakeCurrent(nullptr, nullptr);
#elif defined(USE_CGL)
    // OSX clear current
#endif
}

void OpenGLDisplay::swapBuffers() {
#if defined(USE_GLX)
    if (display_ && window_) {
        glXSwapBuffers(display_, window_);
    }
#elif defined(USE_WGL)
    if (hdc_) {
        SwapBuffers(hdc_);
    }
#elif defined(USE_CGL)
    // OSX swap buffers
#endif
}

#elif defined(USE_WGL)
// Windows (WGL) implementation - NOT SUPPORTED
// Only Linux GLX is implemented
bool OpenGLDisplay::initWGL() {
    return false;
}

void OpenGLDisplay::cleanupWGL() {
}

void OpenGLDisplay::handleEventsWGL() {
}

#elif defined(USE_CGL)
// macOS (CGL) implementation - NOT SUPPORTED
// Only Linux GLX is implemented
bool OpenGLDisplay::initCGL() {
    return false;
}

void OpenGLDisplay::cleanupCGL() {
}

void OpenGLDisplay::handleEventsCGL() {
}

void OpenGLDisplay::makeCurrent() {
    // OSX context make current
}

void OpenGLDisplay::clearCurrent() {
    // OSX context clear
}

void OpenGLDisplay::swapBuffers() {
    // OSX swap buffers
}
#endif

} // namespace xjadeo

