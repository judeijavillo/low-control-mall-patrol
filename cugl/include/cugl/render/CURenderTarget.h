//
//  CURenderTarget.h
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

#ifndef __CU_RENDER_TARGET_H__
#define __CU_RENDER_TARGET_H__
#include <memory>
#include <vector>
#include <unordered_map>
#include <initializer_list>
#include <cugl/math/CUColor4.h>
#include <cugl/render/CUTexture.h>

namespace cugl {

/**
 * This is a class representing an offscreen render target (framebuffer).
 *
 * A render target allows the user to draw to a texture before drawing to a screen.
 * This allows for the potential for post-processing effects.  To draw to a render
 * target simply call the {@link #begin} method before drawing.  From that point on
 * all drawing commands will be sent to the associated texture instead of the screen.
 * Call {@link #end} to resume drawing to the screen.
 *
 * Render targets should not be stacked.  It is not safe to call a begin/end pair of
 * one render target inside of another begin/end pair.  Control to the screen should
 * be resumed before using another render target.
 *
 * While render targets must have at least one output texture, they can support multiple 
 * textures as long as the active fragment shader has multiple output variables. The
 * locations of these outputs should be set explicitly and sequentially with the
 * layout keyword.
 *
 * This class greatly simplifies OpenGL framebuffers at the cost of some flexibility.
 * The only support for depth and stencil is a combined 24/8 depth and stencil buffer.
 * In addition, output textures must have one of the simplified formats defined by
 * {@link Texture::PixelFormat}. Finally, all output textures are bound sequentially
 * to output locations 0..\#outputs-1. However, we find that still allows us to handle
 * the vast majority of applications with a framebuffer.
 */
class RenderTarget {
private:
    /** The framebuffer associated with this render target */
    GLuint _framebo;
    /** The backing renderbuffer for the framebuffer */
    GLuint _renderbo;
    
    /** The render target "screen" width */
    int _width;
    /** The render target "screen" height */
    int _height;
    /** The number of output textures for this render target */
    size_t _outsize;
    
    /** The clear color for this render target */
    Color4f _clearcol;
    /** The cached viewport to restore when this target is finished */
    int _viewport[4];

    /** The combined depth and stencil buffer */
    std::shared_ptr<Texture>  _depthst;
    /** The array of output textures (must be at least one) */
    std::vector<std::shared_ptr<Texture>> _outputs;
    /** The bind points for linking up the shader output variables */
    std::vector<GLuint> _bindpoints;

#pragma mark -
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
    bool prepareBuffer();

    /**
     * Attaches an output texture with the given format to framebuffer.
     *
     * This method allocates the texture and binds it in the correct place
     * (e.g. GL_COLOR_ATTACHMENT0+index).  The texture will be the same size 
     * as this render target.
     *
     * If this method fails, it will safely clean up any previously allocated 
     * objects before quitting.
     *
     * @param index        The index location to attach this output texture
     * @param format    The texture pixel format {@see Texture}
     *
     * @return true if initialization was successful.
     */
    bool attachTexture(GLuint index, Texture::PixelFormat format);
    
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
    bool completeBuffer();

    
#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates an uninitialized render target with no output textures.
     *
     * You must initialize the render target to create an output texture.
     */
    RenderTarget();
    
    /**
     * Deletes this render target, disposing all resources.
     */
    ~RenderTarget();
    
    /**
     * Deletes the render target and resets all attributes.
     *
     * You must reinitialize the render target to use it.
     */
    void dispose();
    
    /**
     * Initializes this target with a single RGBA output texture.
     *
     * The output texture will have the given width and size.
     *
     * @param width        The drawing width of this render target   
     * @param height    The drawing width of this render target   
     *
     * @return true if initialization was successful.
     */
    bool init(int width, int height) {
        return init(width,height,1);
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
    bool init(int width, int height, size_t outputs);

    /**
     * Initializes this target with multiple textures of the given format.
     *
     * The output textures will have the given width and size. They will
     * be assigned the appropriate format as specified in {@link Texture#init}.  
     * They will be assigned locations 0..\#outputs-1.  These locations should 
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
    bool init(int width, int height, std::initializer_list<Texture::PixelFormat> outputs);

    /**
     * Initializes this shader with multiple textures of the given format.
     *
     * The output textures will have the given width and size. They will
     * be assigned the appropriate format as specified in {@link Texture#init}.  
     * They will be assigned locations 0..\#outputs-1.  These locations should 
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
    bool init(int width, int height, std::vector<Texture::PixelFormat> outputs);

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
    bool init(int width, int height,
              std::unordered_map<GLuint,Texture::PixelFormat> outputs);

    
    /**
     * Initializes this target with multiple textures of the given format.
     *
     * The output textures will have the given width and size. They will
     * be assigned the appropriate format as specified in {@link Texture#init}.  
     * They will be assigned locations 0..\#outsize-1.  These locations should 
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
    bool init(int width, int height, Texture::PixelFormat* outputs, size_t outsize);

    
#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a new render target with a single RGBA output texture.
     *
     * The output texture will have the given width and size.
     *
     * @param width        The drawing width of this render target   
     * @param height    The drawing width of this render target   
     *
     * @return a new render target with a single RGBA output texture.
     */
    static std::shared_ptr<RenderTarget> alloc(int width, int height) {
        std::shared_ptr<RenderTarget> result = std::make_shared<RenderTarget>();
        return (result->init(width,height) ? result : nullptr);
    }

    /**
     * Returns a new render target with multiple RGBA output textures.
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
     * @return a new render target with multiple RGBA output textures.
     */
    static std::shared_ptr<RenderTarget> alloc(int width, int height, size_t outputs) {
        std::shared_ptr<RenderTarget> result = std::make_shared<RenderTarget>();
        return (result->init(width,height,outputs) ? result : nullptr);
    }

    /**
     * Returns a new render target with multiple textures of the given format.
     *
     * The output textures will have the given width and size. They will
     * be assigned the appropriate format as specified in {@link Texture#init}.  
     * They will be assigned locations 0..\#outputs-1.  These locations should 
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
     * @return a new render target with multiple textures of the given format.
     */
    static std::shared_ptr<RenderTarget> alloc(int width, int height,
                                               std::initializer_list<Texture::PixelFormat> outputs) {
        std::shared_ptr<RenderTarget> result = std::make_shared<RenderTarget>();
        return (result->init(width,height,outputs) ? result : nullptr);
    }

    /**
     * Returns a new render target with multiple textures of the given format.
     *
     * The output textures will have the given width and size. They will
     * be assigned the appropriate format as specified in {@link Texture#init}.  
     * They will be assigned locations 0..\#outputs-1.  These locations should 
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
     * @return a new render target with multiple textures of the given format.
     */
    static std::shared_ptr<RenderTarget> alloc(int width, int height,
                                               std::vector<Texture::PixelFormat> outputs) {
        std::shared_ptr<RenderTarget> result = std::make_shared<RenderTarget>();
        return (result->init(width,height,outputs) ? result : nullptr);
    }
    
    /**
     * Returns a new render target with multiple textures of the given format.
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
     * @return a new render target with multiple textures of the given format.
     */
    static std::shared_ptr<RenderTarget> alloc(int width, int height,
                                               std::unordered_map<GLuint,Texture::PixelFormat> outputs) {
        std::shared_ptr<RenderTarget> result = std::make_shared<RenderTarget>();
        return (result->init(width,height,outputs) ? result : nullptr);
    }

    /**
     * Returns a new render target with multiple textures of the given format.
     *
     * The output textures will have the given width and size. They will
     * be assigned the appropriate format as specified in {@link Texture#init}.  
     * They will be assigned locations 0..\#outsize-1.  These locations should 
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
     * @return a new render target with multiple textures of the given format.
     */
    static std::shared_ptr<RenderTarget> alloc(int width, int height,
                                               Texture::PixelFormat* outputs, size_t outsize) {
        std::shared_ptr<RenderTarget> result = std::make_shared<RenderTarget>();
        return (result->init(width,height,outputs,outsize) ? result : nullptr);
    }
    
    
#pragma mark -
#pragma mark Attributes
    /**
     * Returns the width of this render target
     *
     * @return the width of this render target
     */
    int getWidth() const { return _width; }

    /**
     * Returns the height of this render target
     *
     * @return the height of this render target
     */
    int getHeight() const { return _height; }

    /**
     * Returns the clear color for this render target.
     *
     * The clear color is used to clear the texture when the method
     * {@link #begin} is called.
     *
     * @return the clear color for this render target.
     */
    Color4 getClearColor() const { return _clearcol; }
    
    /**
     * Sets the clear color for this render target.
     *
     * The clear color is used to clear the texture when the method
     * {@link #begin} is called.
     *
     * @param color    The clear color for this render target.
     */
    void setClearColor(const Color4 color) { _clearcol = color; }
    
    /**
     * Returns the number of output textures for this render target.
     *
     * If the render target has been successfully initialized, this
     * value is guaranteed to be at least 1.
     *
     * @return the number of output textures for this render target.
     */
    size_t getOutputSize() const { return _outsize; }

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
    const std::shared_ptr<Texture>& getTexture(size_t index = 0) const;

    /**
     * Returns the depth/stencil buffer for this render target
     *
     * The framebuffer for a render target always uses a combined depth
     * and stencil buffer.  It uses 24 bits for the depth and 8 bits for
     * the stencil.  This should be sufficient in most applications.
     *
     * @return the depth/stencil buffer for this render target
     */
    const std::shared_ptr<Texture>& getDepthStencil() const { return _depthst; }


#pragma mark -
#pragma mark Drawing
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
    void begin();
    
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
    void end();
};

}
#endif /* __CU_RENDER_TARGET_H__ */
