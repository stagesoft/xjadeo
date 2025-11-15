#ifndef XJADEO_OPENGLDISPLAY_H
#define XJADEO_OPENGLDISPLAY_H

#include "DisplayBackend.h"
#include "OpenGLRenderer.h"
#include <memory>

// Forward declarations for platform-specific types
#if defined(PLATFORM_LINUX) || (!defined(PLATFORM_WINDOWS) && !defined(PLATFORM_OSX))
struct _XDisplay;
typedef struct _XDisplay Display;
typedef unsigned long Window;
typedef struct __GLXcontextRec *GLXContext;
#elif defined(PLATFORM_WINDOWS)
struct HDC__;
typedef struct HDC__* HDC;
struct HWND__;
typedef struct HWND__* HWND;
typedef void* HGLRC;
#elif defined(PLATFORM_OSX)
// OSX types would go here
typedef void* NSContext;
#endif

namespace xjadeo {

// Forward declaration
class LayerManager;

/**
 * OpenGLDisplay - OpenGL-based display backend (Linux GLX only)
 * 
 * Implements DisplayBackend using OpenGL with GLX on Linux.
 * Supports multi-layer rendering and multi-display output (Xinerama/Wayland).
 * 
 * Note: Only Linux GLX is implemented. Windows (WGL) and macOS (CGL) are not supported.
 */
class OpenGLDisplay : public DisplayBackend {
public:
    OpenGLDisplay();
    virtual ~OpenGLDisplay();

    // DisplayBackend interface
    bool openWindow() override;
    void closeWindow() override;
    bool isWindowOpen() const override;
    void render(LayerManager* layerManager, OSDManager* osdManager = nullptr) override;
    void handleEvents() override;
    void resize(unsigned int width, unsigned int height) override;
    void getWindowSize(unsigned int* width, unsigned int* height) const override;
    void setPosition(int x, int y) override;
    void getWindowPos(int* x, int* y) const override;
    void setFullscreen(int action) override;
    bool getFullscreen() const override;
    void setOnTop(int action) override;
    bool getOnTop() const override;
    bool supportsMultiDisplay() const override { return true; }
    void* getContext() override;

private:
    // Platform-specific initialization
#if defined(USE_GLX) || (!defined(PLATFORM_WINDOWS) && !defined(PLATFORM_OSX))
    bool initGLX();
    void cleanupGLX();
    void handleEventsGLX();
#elif defined(PLATFORM_WINDOWS)
    bool initWGL();
    void cleanupWGL();
    void handleEventsWGL();
#elif defined(PLATFORM_OSX)
    bool initCGL();
    void cleanupCGL();
    void handleEventsCGL();
#endif

    // OpenGL context management
    void makeCurrent();
    void clearCurrent();
    void swapBuffers();

    // Window management
    void setupWindowHints();
    void updateWindowProperties();

    // OpenGL renderer
    std::unique_ptr<OpenGLRenderer> renderer_;
    
    // OSD renderer
    std::unique_ptr<class OSDRenderer> osdRenderer_;

    // Platform-specific window/context handles
#if defined(PLATFORM_LINUX) || (!defined(PLATFORM_WINDOWS) && !defined(PLATFORM_OSX))
    Display* display_;
    Window window_;
    GLXContext context_;
    int screen_;
#elif defined(PLATFORM_WINDOWS)
    HDC hdc_;
    HWND hwnd_;
    HGLRC hglrc_;
#elif defined(PLATFORM_OSX)
    // OSX handles
    void* nsContext_;
#endif

    // Window state
    unsigned int windowWidth_;
    unsigned int windowHeight_;
    int windowX_;
    int windowY_;
    bool fullscreen_;
    bool ontop_;
    bool windowOpen_;
};

} // namespace xjadeo

#endif // XJADEO_OPENGLDISPLAY_H

