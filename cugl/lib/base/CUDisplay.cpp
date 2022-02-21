//
//  CUDisplay.cpp
//  Cornell University Game Library (CUGL)
//
//  This module is a singleton providing display information about the device.
//  Originally, we had made this part of Application.  However, we discovered
//  that we needed platform specfic code for this, so we factored it out.
//
//  This singleton is also responsible for initializing (and disposing) the
//  OpenGL context.  That is because that context is tightly coupled to the
//  orientation information, which is provided by this class.
//
//  Because this is a singleton, there are no publicly accessible constructors
//  or intializers.  Use the static methods instead.
//
//  CUGL MIT License:
//      This software is provided 'as-is', without any express or implied
//      warranty.  In no event will the authors be held liable for any damages
//      arising from the use of this software.
//
//      Permission is granted to anyone to use this software for any purpose,
//      including commercial applications, and to alter it and redistribute it
//      freely, subject to the following restrictions:
//
//      1. The origin of this software must not be misrepresented; you must not
//      claim that you wrote the original software. If you use this software
//      in a product, an acknowledgment in the product documentation would be
//      appreciated but is not required.
//
//      2. Altered source versions must be plainly marked as such, and must not
//      be misrepresented as being the original software.
//
//      3. This notice may not be removed or altered from any source distribution.
//
//  Author: Walker White
//  Version: 12/12/18
#include <cugl/base/CUBase.h>
#include <cugl/base/CUDisplay.h>
#include <cugl/util/CUDebug.h>
#include "platform/CUDisplay-impl.h"
#include <SDL/SDL_ttf.h>

using namespace cugl;
using namespace cugl::impl;

/** The display singleton */
Display* Display::_thedisplay = nullptr;

// Flags for the device initialization
/** Whether this display should use the fullscreen */
Uint32 Display::INIT_FULLSCREEN   = 1;
/** Whether this display should support a High DPI screen */
Uint32 Display::INIT_HIGH_DPI     = 2;
/** Whether this display should be multisampled */
Uint32 Display::INIT_MULTISAMPLED = 4;
/** Whether this display should be centered (on windowed screens) */
Uint32 Display::INIT_CENTERED     = 8;

#pragma mark Constructors
/**
 * Creates a new, unitialized Display.
 *
 * All of the values are set to 0 or UNKNOWN, depending on their type. You
 * must initialize the Display to access its values.
 *
 * WARNING: This class is a singleton.  You should never access this
 * constructor directly.  Use the {@link start()} method instead.
 */
Display::Display() :
_window(nullptr),
_glContext(NULL),
_framebuffer(0),
_rendbuffer(0),
_initialOrientation(Orientation::UNKNOWN),
_displayOrientation(Orientation::UNKNOWN),
_deviceOrientation(Orientation::UNKNOWN) {}

/**
 * Initializes the display with the current screen information.
 *
 * This method creates a display with the given title and bounds. As part
 * of this initialization, it will create the OpenGL context, using
 * the flags provided.  The bounds are ignored if the display is fullscreen.
 * In that case, it will use the bounds of the display.
 *
 * This method gathers the native resolution bounds, pixel density, and
 * orientation  using platform-specific tools.
 *
 * WARNING: This class is a singleton.  You should never access this
 * initializer directly.  Use the {@link start()} method instead.
 *
 * @param title     The window/display title
 * @param bounds    The window/display bounds
 * @param flags     The initialization flags
 *
 * @return true if initialization was successful.
 */
bool Display::init(std::string title, Rect bounds, Uint32 flags) {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        CULogError("Could not initialize display: %s",SDL_GetError());
        return false;
    }
    
    // Initialize the TTF library
    if ( TTF_Init() < 0 ) {
        CULogError("Could not initialize TTF: %s",SDL_GetError());
        return false;
    }
    
    // We have to set the OpenGL prefs BEFORE creating window
    if (!prepareOpenGL(flags & INIT_MULTISAMPLED)) {
        return false;
    }

    Uint32 sdlflags = SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL;
    if (flags & INIT_HIGH_DPI) {
        sdlflags |= SDL_WINDOW_ALLOW_HIGHDPI;
    }

    _bounds  = DisplayBounds();
    _scale   = DisplayPixelDensity();
    if (flags & INIT_FULLSCREEN) {
        SDL_ShowCursor(0);
        sdlflags |= SDL_WINDOW_FULLSCREEN;
        bounds.origin = _bounds.origin*_scale;
        bounds.size   = _bounds.size*_scale;
    } else if (flags & INIT_CENTERED) {
        Size size = Display::get()->getBounds().size;
        bounds.origin.x = (size.width  - bounds.size.width)/2.0f;
        bounds.origin.y = (size.height - bounds.size.height)/2.0f;
    }
    
    // Make the window
    _title  = title;
    _window = SDL_CreateWindow(title.c_str(),
                               (int)bounds.origin.x,   (int)bounds.origin.y,
                               (int)bounds.size.width, (int)bounds.size.height,
                               sdlflags);
    
    if (!_window) {
        CULogError("Could not create window: %s", SDL_GetError());
        return false;
    }

    // Now we can create the OpenGL context
    if (!initOpenGL(flags & INIT_MULTISAMPLED)) {
        SDL_DestroyWindow(_window);
        _window = nullptr;
        return false;
    }
    
    // On Android, we have to call this first.
    _usable  = DisplaySafeBounds(_window);
    _notched = DisplayNotch();

// The mobile devices have viewport problems
#if CU_PLATFORM == CU_PLATFORM_ANDROID || CU_PLATFORM == CU_PLATFORM_IPHONE
    glViewport(0, 0, (int)bounds.size.width, (int)bounds.size.height);
#endif

    _initialOrientation = DisplayOrientation(true);
    _displayOrientation = _initialOrientation;
    _deviceOrientation  = DisplayOrientation(false);
    _defaultOrientation = DisplayDefaultOrientation();
    return true;
}

/**
 * Uninitializes this object, releasing all resources.
 *
 * This method quits the SDL video system and disposes the OpenGL context,
 * effectively exitting and shutting down the entire program.
 *
 * WARNING: This class is a singleton.  You should never access this
 * method directly.  Use the {@link stop()} method instead.
 */
void Display::dispose() {
    if (_window != nullptr) {
        SDL_GL_DeleteContext(_glContext);
        SDL_DestroyWindow(_window);
        _window = nullptr;
        _glContext = NULL;
    }
    _framebuffer = 0;
    _bounds.size.set(0,0);
    _usable.size.set(0,0);
    _scale.setZero();
    _initialOrientation = Orientation::UNKNOWN;
    _displayOrientation = Orientation::UNKNOWN;
    _deviceOrientation = Orientation::UNKNOWN;
    SDL_Quit();
}

#pragma mark -
#pragma mark Static Accessors
/**
 * Starts up the SDL display and video system.
 *
 * This static method needs to be the first line of any application, though
 * it is handled automatically in the {@link Application} class.
 *
 * This method creates the display with the given title and bounds. As part
 * of this initialization, it will create the OpenGL context, using
 * the flags provided.  The bounds are ignored if the display is fullscreen.
 * In that case, it will use the bounds of the display.
 *
 * Once this method is called, the {@link get()} method will no longer
 * return a null value.
 *
 * @param title     The window/display title
 * @param bounds    The window/display bounds
 * @param flags     The initialization flags
 *
 * @return true if the display was successfully initialized
 */
bool Display::start(std::string name, Rect bounds, Uint32 flags) {
    if (_thedisplay != nullptr) {
        CUAssertLog(false, "The display is already initialized");
        return false;
    }
    _thedisplay = new Display();
    return _thedisplay->init(name,bounds,flags);
}

/**
 * Shuts down the SDL display and video system.
 *
 * This static method needs to be the last line of any application, though
 * it is handled automatically in the {@link Application} class. It will
 * dipose of the display and the OpenGL context.
 *
 * Once this method is called, the {@link get()} method will return nullptr.
 * More importantly, no SDL function calls will work anymore.
 */
void Display::stop() {
    if (_thedisplay == nullptr) {
        CUAssertLog(false, "The display is not initialized");
    }
    delete _thedisplay;
    _thedisplay = nullptr;
}

#pragma mark -
#pragma mark Window Management
/**
 * Sets the title of this display
 *
 * On a desktop, the title will be displayed at the top of the window.
 *
 * @param title  The title of this display
 */
void Display::setTitle(const char* title) {
    _title = title;
    if (_window != nullptr) {
        SDL_SetWindowTitle(_window, title);
    }
}

/**
 * Shows the window for this display (assuming it was hidden).
 *
 * This method does nothing if the window was not hidden.
 */
void Display::show() {
    SDL_ShowWindow(_window);
}

/**
 * Hides the window for this display (assuming it was visible).
 *
 * This method does nothing if the window was not visible.
 */
void Display::hide() {
    SDL_HideWindow(_window);
}

#pragma mark -
#pragma mark Attributes
/**
 * Returns true if this device has a landscape orientation
 *
 * @return true if this device has a landscape orientation
 */
bool Display::isLandscape() const {
    return _bounds.size.width > _bounds.size.height;
}

/**
 * Returns true if this device has a portrait orientation
 *
 * @return true if this device has a portrait orientation
 */
bool Display::isPortrait() const {
    return _bounds.size.width < _bounds.size.height;
}


/**
 * Returns the usable full screen resolution for this display in points.
 *
 * Usable is a subjective term defined by the operating system.  In
 * general, it means the full screen minus any space used by important
 * user interface elements, like a status bar (iPhone), menu bar (OS X),
 * or task bar (Windows), or a notch (iPhone X).  In the case of the
 * latter, you can specify whether you want to use the display orientation
 * or the device orientation.
 *
 * This method computes the bounds for the current resolution, not the
 * maximum resolution.  You should never change the resolution of a display.
 * Allow the user to have their preferred resolution.  Instead, you should
 * adjust your camera to scale the viewport.
 *
 * The value returned represents points, not pixels.  If you are using a
 * traditional display, these are the same.  However, on Retina displays
 * and other high DPI monitors, these may be different.  Regardless, you
 * should always work with points, not pixels, when computing the screen
 * size.
 *
 * @param display   Whether to use the display (as opposed to the device) orientation
 *
 * @return the usable full screen resolution for this display in points.
 */
Rect Display::getSafeBounds(bool display) {
    if (display) {
        return _usable;
    } else {
        Rect result = impl::DisplaySafeBounds(_window);
        Orientation actual = impl::DisplayOrientation(true);
        Orientation device = impl::DisplayOrientation(false);
        int steps = 0; // Number of counter clockwise steps
        float swap;
        switch(actual) {
            case cugl::Display::Orientation::LANDSCAPE:
                switch (device) {
                    case cugl::Display::Orientation::LANDSCAPE_REVERSED:
                        steps = 2;
                        break;
                    case cugl::Display::Orientation::PORTRAIT:
                        steps = 3;
                        break;
                    case cugl::Display::Orientation::UPSIDE_DOWN:
                        steps= 1;
                        break;
                    default:
                        break;
                }
                break;
            case cugl::Display::Orientation::LANDSCAPE_REVERSED:
                switch (device) {
                    case cugl::Display::Orientation::LANDSCAPE:
                        steps = 2;
                        break;
                    case cugl::Display::Orientation::UPSIDE_DOWN:
                        steps = 3;
                        break;
                    case cugl::Display::Orientation::PORTRAIT:
                        steps= 1;
                        break;
                    default:
                        break;
                }
                break;
            case cugl::Display::Orientation::PORTRAIT:
                switch (device) {
                    case cugl::Display::Orientation::UPSIDE_DOWN:
                        steps = 2;
                        break;
                    case cugl::Display::Orientation::LANDSCAPE_REVERSED:
                        steps = 3;
                        break;
                    case cugl::Display::Orientation::LANDSCAPE:
                        steps= 1;
                        break;
                    default:
                        break;
                }
                break;
            case cugl::Display::Orientation::UPSIDE_DOWN:
                switch (device) {
                    case cugl::Display::Orientation::PORTRAIT:
                        steps = 2;
                        break;
                    case cugl::Display::Orientation::LANDSCAPE:
                        steps = 3;
                        break;
                    case cugl::Display::Orientation::LANDSCAPE_REVERSED:
                        steps= 1;
                        break;
                    default:
                        break;
                }
            default:
                break;
        }
        
        switch (steps) {
            case 1:
                swap = result.origin.y;
                result.origin.y = result.origin.x;
                result.origin.x = _bounds.size.height-(swap+result.size.height);
                swap = result.size.width;
                result.size.width  = result.size.height;
                result.size.height = swap;
                break;
            case 2:
                result.origin.x = _bounds.size.width-(result.origin.x+result.size.width);
                result.origin.y = _bounds.size.height-(result.origin.y+result.size.height);
                break;
            case 3:
                swap = result.origin.x;
                result.origin.x = result.origin.y;
                result.origin.y = _bounds.size.width-(swap+result.size.width);
                swap = result.size.width;
                result.size.width  = result.size.height;
                result.size.height = swap;
                break;
        }
    
        return result;
   }
}

/**
 * Removes the display orientation listener for this display.
 *
 * This listener handles changes in either the device orientation (see
 * {@link getDeviceOrientation()} or the display orientation (see
 * {@link getDeviceOrientation()}. Since the device orientation will always
 * change when the display orientation does, this callback can easily safely
 * handle both. The boolean parameter in the callback indicates whether or
 * not a display orientation change has happened as well.
 *
 * Unlike other events, this listener will be invoked at the end of an
 * animation frame, after the screen has been drawn.  So it will be
 * processed before any input events waiting for the next frame.
 *
 * A display may only have one orientation listener at a time.  If this
 * display does not have an orientation listener, this method will fail.
 *
 * @return true if the listener was succesfully removed
 */
bool Display::removeOrientationListener() {
    bool result = _orientationListener != nullptr;
    _orientationListener = nullptr;
    return result;
}


#pragma mark -
#pragma mark OpenGL Support
/**
 * Restores the default frame/render buffer.
 *
 * This is necessary when you are using a {@link RenderTarget} and want
 * to restore control the frame buffer.  It is necessary because 0 is
 * NOT necessarily the correct id of the default framebuffer (particularly
 * on iOS).
 */
void Display::restoreRenderTarget() {
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _rendbuffer);
}

/**
 * Queries the identity of the default frame/render buffer.
 *
 * This is necessary when you are using a {@link RenderTarget} and want
 * to restore control the frame buffer.  It is necessary because 0 is
 * NOT necessarily the correct id of the default framebuffer (particularly
 * on iOS).
 */
void Display::queryRenderTarget() {
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,  &_framebuffer);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &_rendbuffer);
}

/**
 * Assign the default settings for OpenGL
 *
 * This has to be done before the Window is created.
 *
 * @param mutlisample   Whether to support multisampling.
 *
 * @return true if preparation was successful
 */
bool Display::prepareOpenGL(bool multisample) {
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);    
    
#if CU_GL_PLATFORM == CU_GL_OPENGLES
    int profile = SDL_GL_CONTEXT_PROFILE_ES;
    int version = 3; // Force 3 on mobile
#else
    int profile = SDL_GL_CONTEXT_PROFILE_CORE;
    int version = 4; // Force 4 on desktop
    if (multisample) {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    }
#endif
    
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile) != 0) {
        CULogError("OpenGL is not supported on this platform: %s", SDL_GetError());
        return false;
    }
    
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, version) != 0) {
        CULogError("OpenGL %d is not supported on this platform: %s", version, SDL_GetError());
        return false;
    }
    
    // Enable stencil support for sprite batch
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    return true;
}

/**
 * Initializes the OpenGL context
 *
 * This has to be done after the Window is created.
 *
 * @param mutlisample   Whether to support multisampling.
 *
 * @return true if initialization was successful
 */
bool Display::initOpenGL(bool multisample) {
#if CU_GL_PLATFORM != CU_GL_OPENGLES
    if (multisample) {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    }
#endif

    // Create the OpenGL context
    _glContext = SDL_GL_CreateContext( _window );
    if( _glContext == NULL )  {
        CULogError("Could not create OpenGL context: %s", SDL_GetError() );
        return false;
    }
    
    // Multisampling support
#if CU_GL_PLATFORM != CU_GL_OPENGLES
    glEnable(GL_LINE_SMOOTH);
    if (multisample) {
        glEnable(GL_MULTISAMPLE);
    }
#endif
    
#if CU_PLATFORM == CU_PLATFORM_WINDOWS
    //Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        SDL_Log("Error initializing GLEW: %s", glewGetErrorString(glewError));
    }
#endif
    
    queryRenderTarget();
    return true;
}

/**
 * Refreshes the display.
 *
 * This method will swap the OpenGL framebuffers, drawing the screen.
 *
 * It will also reassess the orientation state and call the listener as
 * necessary
 */
void Display::refresh() {
    SDL_GL_SwapWindow(_window);
    Orientation oldDisplay = _displayOrientation;
    Orientation oldDevice  = _deviceOrientation;
    _displayOrientation = DisplayOrientation(true);
    _deviceOrientation  = DisplayOrientation(false);
    if (oldDisplay != _displayOrientation) {
        // Requery if necessary
        _usable = DisplaySafeBounds(_window);
    }
    if (_orientationListener &&
        (oldDevice != _deviceOrientation || oldDisplay != _displayOrientation)) {
        _orientationListener(oldDevice,_deviceOrientation,oldDisplay != _displayOrientation);
    }
}


