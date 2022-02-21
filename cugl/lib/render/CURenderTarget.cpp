//
//  CURenderTarget.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides support for an offscreen render target.  That is, a framebuffer
//  with (potentially multiple) attached output buffers.  This allows us to draw to a
//  texture for potential post processing.
//
//  This class uses our standard shared-pointer architecture.
//
//  1. The constructor does not perform any initialization; it just sets all
//     attributes to their defaults.
//
//  2. All initialization takes place via init methods, which can fail if an
//     object is initialized more than once.
//
//  3. All allocation takes place via static constructors which return a shared
//     pointer.
//
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
//  Version: 3/1/20

#include <cugl/render/CURenderTarget.h>
#include <cugl/render/CUTexture.h>
#include <cugl/base/CUDisplay.h>
#include <cugl/util/CUDebug.h>

using namespace cugl;

#pragma mark Setup
/**
 * Initializes the framebuffer and associated render buffer
 *
 * This method also initializes the depth/stencil buffer. It allocates the
 * arrays for the output textures and bindpoints. However, it does not
 * initialize the output textures.  That is done in {@lin #attachTexture}.
 *
 * If this method fails, it will safely clean up any allocated objects
 * before quitting.
 *
 * @return true if initialization was successful.
 */
bool RenderTarget::prepareBuffer() {
    glGetIntegerv(GL_VIEWPORT, _viewport);
    
    GLenum error;
    glGenFramebuffers(1, &_framebo);
    if (!_framebo) {
        error = glGetError();
        CULogError("Could not create frame buffer. %s", gl_error_name(error).c_str());
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, _framebo);

    // Attach the depth buffer first
    _depthst = Texture::alloc(_width,_height,Texture::PixelFormat::DEPTH_STENCIL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                           GL_TEXTURE_2D,  _depthst->getBuffer(), 0);
    if (_depthst == nullptr) {
        dispose();
        Display::get()->restoreRenderTarget();
        return false;
    }
    
    glGenRenderbuffers(1, &_renderbo);
    if (!_renderbo) {
        error = glGetError();
        CULogError("Could not create render buffer. %s", gl_error_name(error).c_str());
        dispose();
        Display::get()->restoreRenderTarget();
        return false;
    }
    
    glBindRenderbuffer(GL_RENDERBUFFER, _renderbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, _width, _height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, _renderbo);
    error = glGetError();
    if (error) {
        CULogError("Could not attach render buffer to frame buffer. %s",
                   gl_error_name(error).c_str());
        dispose();
        Display::get()->restoreRenderTarget();
        return false;
    }

    return true;
}

/**
 * Attaches an output texture with the given format to framebuffer.
 *
 * This method allocates the texture and attaches it to the correct  binds it in the correct place
 * (e.g. GL_COLOR_ATTACHMENT0+index).  The texture will be the same size
 * as this render target.
 *
 * If this method fails, it will safely clean up any previously allocated
 * objects before quitting.
 *
 * @param index        The index location to attach this output texture
 * @param index        The index location to attach this output texture
 * @param format    The texture pixel format {@see Texture}
 *
 * @return true if initialization was successful.
 */
bool RenderTarget::attachTexture(GLuint index, Texture::PixelFormat format) {
    GLenum error;
    auto texture = Texture::alloc(_width,_height,format);
    _outputs.push_back(texture);
    _bindpoints.push_back(GL_COLOR_ATTACHMENT0+(GLint)index);
    if (texture == nullptr) {
        dispose();
        Display::get()->restoreRenderTarget();
        return false;
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+(GLint)index,
                           GL_TEXTURE_2D,  texture->getBuffer(), 0);
    error = glGetError();
    if (error) {
        CULogError("Could not attach output textures to frame buffer. %s",
                   gl_error_name(error).c_str());
        dispose();
        Display::get()->restoreRenderTarget();
        return false;
    }
    return true;
}

/**
 * Completes the framebuffer after all attachments are finalized
 *
 * This sets the draw buffers and checks the framebuffer status. If
 * OpenGL says that it is complete, it returns true.
 *
 * If this method fails, it will safely clean up any previously allocated
 * objects before quitting.
 *
 * @return true if the framebuffer was successfully finalized.
 */
bool RenderTarget::completeBuffer() {
    glDrawBuffers((int)_outsize, _bindpoints.data());
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        CULogError("Could not bind frame buffer. %s",
                   gl_error_name(status).c_str());
        dispose();
        Display::get()->restoreRenderTarget();
        return false;
    }

    Display::get()->restoreRenderTarget();
    return true;
}


#pragma mark -
#pragma mark Constructors
/**
 * Creates an uninitialized render target with no output textures.
 *
 * You must initialize the render target to create an output texture.
 */
RenderTarget::RenderTarget() :
_framebo(0),
_renderbo(0),
_width(0),
_height(0),
_outsize(0) {
    _clearcol.set(0, 0, 0);
    _viewport[0] = 0; _viewport[1] = 0;
    _viewport[2] = 0; _viewport[3] = 0;
    _depthst = nullptr;
}

/**
 * Deletes this render target, disposing all resources.
 */
RenderTarget::~RenderTarget() {
    dispose();
}

/**
 * Initializes this target with multiple RGBA output textures.
 *
 * The output textures will have the given width and size. They will
 * be assigned locations 0..outputs-1.  These locations should be
 * bound with the layout keyword in any shader used with this render
 * target. Otherwise the results are not well-defined.
 *
 * If outputs is larger than the number of possible shader outputs
 * for this platform, this method will fail.  OpenGL only guarantees
 * up to 8 output textures.
 *
 * @param width        The drawing width of this render target
 * @param height    The drawing width of this render target
 * @param outputs    The number of output textures
 *
 * @return true if initialization was successful.
 */
bool RenderTarget::init(int width, int height, size_t outputs) {
    _width  = width; _height = height;
    _outsize = outputs;
    
    if (prepareBuffer()) {
        for(GLuint ii = 0; ii < _outsize; ii++) {
            if (!attachTexture(ii, Texture::PixelFormat::RGBA)) {
                return false;
            }
        }
        return completeBuffer();
    }
    
    return false;
}

/**
 * Initializes this target with multiple textures of the given format.
 *
 * The output textures will have the given width and size. They will
 * be assigned the appropriate format as specified in {@link Texture#init}.
 * They will be assigned locations 0..#outputs-1.  These locations should
 * be bound with the layout keyword in any shader used with this render
 * target. Otherwise the results are not well-defined.
 *
 * If the size of the outputs parameter is larger than the number of
 * possible shader outputs for this platform, this method will fail.
 * OpenGL only guarantees up to 8 output textures.
 *
 * @param width        The drawing width of this render target
 * @param height    The drawing width of this render target
 * @param outputs    The list of desired texture formats
 *
 * @return true if initialization was successful.
 */
bool RenderTarget::init(int width, int height,
                        std::initializer_list<Texture::PixelFormat> outputs) {
    _width  = width; _height = height;
    _outsize = outputs.size();

    if (prepareBuffer()) {
        GLuint ii = 0;
        for(auto it = outputs.begin(); it != outputs.end(); ++ii, ++it) {
            if (!attachTexture(ii, *it)) {
                return false;
            }
        }
        return completeBuffer();
    }
    
    return false;
}

/**
 * Initializes this target with multiple textures of the given format.
 *
 * The output textures will have the given width and size. They will
 * be assigned the appropriate format as specified in {@link Texture#init}.
 * They will be assigned locations matching the keys of the map outputs.
 * These locations should be bound with the layout keyword in any shader
 * used with this render target. Otherwise the results are not well-defined.
 *
 * If the size of the outputs parameter is larger than the number of
 * possible shader outputs for this platform, this method will fail.
 * OpenGL only guarantees up to 8 output textures.
 *
 * @param width        The drawing width of this render target
 * @param height    The drawing width of this render target
 * @param outputs    The map of desired texture formats for each location
 *
 * @return true if initialization was successful.
 */
bool RenderTarget::init(int width, int height,
                        std::unordered_map<GLuint,Texture::PixelFormat> outputs) {
    _width  = width; _height = height;
    _outsize = outputs.size();

    if (prepareBuffer()) {
        for(auto it = outputs.begin(); it != outputs.end(); ++it) {
            if (!attachTexture(it->first, it->second)) {
                return false;
            }
        }
        return completeBuffer();
    }
    
    return false;

    
}

/**
 * Initializes this target with multiple textures of the given format.
 *
 * The output textures will have the given width and size. They will
 * be assigned the appropriate format as specified in {@link Texture#init}.
 * They will be assigned locations 0..#outputs-1.  These locations should
 * be bound with the layout keyword in any shader used with this render
 * target. Otherwise the results are not well-defined.
 *
 * If the size of the outputs parameter is larger than the number of
 * possible shader outputs for this platform, this method will fail.
 * OpenGL only guarantees up to 8 output textures.
 *
 * @param width        The drawing width of this render target
 * @param height    The drawing width of this render target
 * @param outputs    The list of desired texture formats
 *
 * @return true if initialization was successful.
 */
bool RenderTarget::init(int width, int height,
                        std::vector<Texture::PixelFormat> outputs) {
    _width  = width; _height = height;
    _outsize = outputs.size();

    if (prepareBuffer()) {
        GLuint ii = 0;
        for(auto it = outputs.begin(); it != outputs.end(); ++ii, ++it) {
            if (!attachTexture(ii, *it)) {
                return false;
            }
        }
        return completeBuffer();
    }
    
    return false;
}

/**
 * Initializes this target with multiple textures of the given format.
 *
 * The output textures will have the given width and size. They will
 * be assigned the appropriate format as specified in {@link Texture#init}.
 * They will be assigned locations 0..outsize-1.  These locations should
 * be bound with the layout keyword in any shader used with this render
 * target. Otherwise the results are not well-defined.
 *
 * If the size of the outputs parameter is larger than the number of
 * possible shader outputs for this platform, this method will fail.
 * OpenGL only guarantees up to 8 output textures.
 *
 * @param width        The drawing width of this render target
 * @param height    The drawing width of this render target
 * @param outputs    The list of desired texture formats
 * @param outsize    The number of elements in outputs
 *
 * @return true if initialization was successful.
 */
bool RenderTarget::init(int width, int height, Texture::PixelFormat* outputs, size_t outsize) {
    _width  = width; _height = height;
    _outsize = outsize;
    
    if (prepareBuffer()) {
        for(GLuint ii = 0; ii < _outsize; ii++) {
            if (!attachTexture(ii, outputs[ii])) {
                return false;
            }
        }
        return completeBuffer();
    }
    
    return false;
}

/**
 * Deletes the render target and resets all attributes.
 *
 * You must reinitialize the render target to use it.
 */
void RenderTarget::dispose() {
    if (_framebo) {
        glDeleteFramebuffers(1, &_framebo);
        _framebo = 0;
    }
    if (_renderbo) {
        glDeleteRenderbuffers(1, &_renderbo);
        _renderbo = 0;
    }
    _outputs.clear();
    _bindpoints.clear();
    _clearcol.set(0, 0, 0);
    _viewport[0] = 0; _viewport[1] = 0;
    _viewport[2] = 0; _viewport[3] = 0;
    _width  = 0;
    _height = 0;
    _depthst = nullptr;
}

#pragma mark -
#pragma mark Drawing
/**
 * Returns the output texture for the given index.
 *
 * The index should be a value between 0..OutputSize-1.  By default it
 * is 0, the primary output texture.
 *
 * @param index    The output index
 *
 * @return the output texture for the given index.
 */
const std::shared_ptr<Texture>& RenderTarget::getTexture(size_t index) const {
    return _outputs[index];
}

/**
 * Begins sending draw commands to this render target.
 *
 * This method clears all of the output textures with the clear color of
 * this render target.  It also sets the viewpoint to match the size of
 * this render target (which may not be the same as the screen). The old
 * viewport is saved and will be restored when {@link #end} is called.
 *
 * It is NOT safe to call a begin/end pair of a render target inside of
 * another render target.  Render targets do not keep a stack.  They alway
 * return control to the default render target (the screen) when done.
 */
void RenderTarget::begin() {
    glGetIntegerv(GL_VIEWPORT, _viewport);
    glBindFramebuffer(GL_FRAMEBUFFER, _framebo);
    //glBindRenderbuffer(GL_RENDERBUFFER, _renderbo);

    glViewport(0, 0, _width, _height);
    glClearColor(_clearcol.r, _clearcol.g, _clearcol.b, _clearcol.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

/**
 * Stops sendinging draw commands to this render target.
 *
 * When this method is called, the original viewport will be restored. Future
 * draw commands will be sent directly to the screen.
 *
 * It is NOT safe to call a begin/end pair of a render target inside of
 * another render target.  Render targets do not keep a stack.  They alway
 * return control to the default render target (the screen) when done.
 */
void RenderTarget::end() {
    Display::get()->restoreRenderTarget();
    glViewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
}

