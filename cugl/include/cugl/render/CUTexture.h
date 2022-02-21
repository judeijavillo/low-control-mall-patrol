//
//  CUTexture.h
//  Cornell University Game Library (CUGL)
//
//  This module provides the class for representing a 2D OpenGL texture. This
//  class also provides support for texture atlases.  Any non-repeating texture
//  can produce a subtexture.  A subtexture wraps the same texture data (and so
//  does not require a context switch in the rendering pipeline), but has
//  different start and end boundaries, as defined by minS, maxS, minT and maxT
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
//  Version: 2/22/20

#ifndef _CU_TEXTURE_H__
#define _CU_TEXTURE_H__
#include <cugl/math/CUMathBase.h>
#include <cugl/math/CUSize.h>

namespace cugl {

/**
 * This is a class representing an OpenGL texture.
 *
 * We enforce that all textures must be a power-of-two along each dimension
 * (though they need not be square). This is still required by some mobile
 * devices and so it is easiest to require it across the board.
 *
 * This class also provides support for texture atlases.  Any non-repeating
 * texture can produce a subtexture.  A subtexture wraps the same texture data
 * (and so does not require a context switch in the rendering pipeline), but
 * has different start and end boundaries, as defined by minS, maxS, minT and
 * maxT. See getSubtexture() for more information.
 * 
 * Shaders and textures have a many-to-many relationship. At any given time,
 * a texture may be providing data to multiple shaders, and a shader may be 
 * working with multiple textures. This many-to-many relationship is captured 
 * by bind points. A texture is bound to a specific bind point and a shader 
 * associates a bind point with a sampler variable. That sampler variable then 
 * pulls data from the appropriate texture.
 *
 * When discussing the relationship between a shader and a texture, we talk
 * about a texture being bound and a texture being active. A bound texture is 
 * one that is associated with a shader; the shader will pull from the texture 
 * in a sampler variable. An active texture is one that is capable of receiving 
 * data from the CPU. A texture must be active if the user wants to change the
 * data or filter settings of a texture.
 *
 * Ideally, bound and active should be two separate concepts, like they are in
 * {@link UniformBuffer}. However, for legacy reasons, OpenGL does not allow a
 * texture to be active without being bound.  Hence the {@link #bind} method
 * below is used for both activating and binding a texture.
 */
class Texture : public std::enable_shared_from_this<Texture> {
#pragma mark Values
public:
    /**
     * This enum lists the possible texture pixel formats.
     *
     * This enum defines the pixel formats supported by CUGL. Because of 
     * cross-platform issues (we must support both OpenGL and OpenGLES), 
     * our textures only support a small subset of formats.
     *
     * This enum also associates default internal types and data types
     * with each pixel format. This greatly simplifies texture creation
     * at the loss of some flexibility.
     */
    enum class PixelFormat : GLenum {
        /** 
         * The default format: RGB with alpha transparency.
         * 
         * This format uses GL_RGBA8 as the internal format. The data type
         * (for each component) is unsigned byte. 
         */
        RGBA = GL_RGBA,
        /** 
         * RGB with no alpha
         * 
         * All blending with this texture assumes alpha is 1.0. This format 
         * uses GL_RGB8 as the internal format. The data type (for each component) 
         * is unsigned byte. 
         */
        RGB  = GL_RGB,
        /** 
         * A single color channel.  In OpenGL that is identified as red.
         * 
         * The green and blue values will be 0.  All blending with this texture 
         * assumes alpha is 1.0. This format uses GL_RGB8 as the internal format. 
         * The data type (for each component) is unsigned byte. 
         */
        RED  = GL_RED,
        /** 
         * A dual color channel.  In OpenGL that is identified as red and green.
         * 
         * The blue values will be 0.  All blending with this texture assumes 
         * alpha is 1.0. This format uses GL_RGB8 as the internal format. 
         * The data type (for each component) is unsigned byte. 
         */
        RED_GREEN = GL_RG,
        /** 
         * A texture used to store a depth component.
         * 
         * This format uses GL_DEPTH_COMPONENT32F as the internal format. The 
         * data type (for the only component) is float. 
         */
        DEPTH = GL_DEPTH_COMPONENT,
        /** 
         * A texture used to store a combined depth and stencil component
         * 
         * This format uses GL_DEPTH24_STENCIL8 as the internal format. The 
         * data type is GL_UNSIGNED_INT_24_8, giving 24 bytes to depth and
         * 8 bits to the stencil. 
         */
        DEPTH_STENCIL = GL_DEPTH_STENCIL
    };
    
private:
    /** A reference to the allocated texture in OpenGL; 0 is not allocated. */
    GLuint _buffer;

    /** The width in pixels */
    GLuint _width;
    
    /** The height in pixels */
    GLuint _height;

    /** The pixel format of the texture */
    PixelFormat _pixelFormat;

    /** The decriptive texture name */
    std::string _name;
    
    /** The minimization algorithm */
    GLuint _minFilter;

    /** The maximization algorithm */
    GLuint _magFilter;
    
    /** The wrap-style for the horizontal texture coordinate */
    GLuint _wrapS;

    /** The wrap-style for the vertical texture coordinate */
    GLuint _wrapT;

    /** Whether or not the texture has mip maps */
    bool _hasMipmaps;

    /** An all purpose blank texture for coloring */
    static std::shared_ptr<Texture> _blank;

    /// Texture atlas support
    /** Our parent, who owns the OpenGL texture (or nullptr if we own it) */
    std::shared_ptr<Texture> _parent;
    
    /** The texture min S (used for texture atlases) */
    GLfloat _minS;

    /** The texture max S (used for texture atlases) */
    GLfloat _maxS;
    
    /** The texture min T (used for texture atlases) */
    GLfloat _minT;

    /** The texture max T (used for texture atlases) */
    GLfloat _maxT;

    /** The bind point assigned to this texture (default 0) */
    GLuint _bindpoint;
    
    /** Whether the algorithm or wrap-style has changed. */
    bool _dirty;
    
#pragma mark -
#pragma mark Constructors
public:
    /** 
     * Creates a new empty texture with no size. 
     *
     * This method performs no allocations.  You must call init to generate
     * a proper OpenGL texture.
     */
    Texture();
    
    /**
     * Deletes this texture, disposing all resources
     */
    ~Texture() { dispose(); }
    
    /**
     * Deletes the OpenGL texture and resets all attributes.
     *
     * You must reinitialize the texture to use it.
     */
    void dispose();

    /**
     * Initializes an empty texture with the given dimensions.
     *
     * Initializing a texture requires the use of the binding point at 0. Any 
     * texture bound to that point will be unbound. In addition, once 
     * initialization is done, this texture will not longer be bound as well.
     *
     * You must use the set() method to load data into the texture.
     *
     * @param width     The texture width in pixels
     * @param height    The texture height in pixels
     * @param format    The texture data format
     *
     * @return true if initialization was successful.
     */
    bool init(int width, int height, PixelFormat format = PixelFormat::RGBA) {
        return initWithData(NULL, width, height, format);
    }
    
    /**
     * Initializes an texture with the given data.
     *
     * Initializing a texture requires the use of the binding point at 0. Any 
     * texture bound to that point will be unbound. In addition, once 
     * initialization is done, this texture will not longer be bound as well.
     *
     * The data format must match the one given.
     *
     * @param data      The texture data (size width*height*format)
     * @param width     The texture width in pixels
     * @param height    The texture height in pixels
     * @param format    The texture data format
     *
     * @return true if initialization was successful.
     */
    bool initWithData(const void *data, int width, int height,
                      PixelFormat format = PixelFormat::RGBA);
    
    /**
     * Initializes an texture with the data from the given file.
     *
     * Initializing a texture requires the use of the binding point at 0. Any 
     * texture bound to that point will be unbound. In addition, once 
     * initialization is done, this texture will not longer be bound as well.
     *
     * This method can load any file format supported by SDL_Image. This
     * includes (but is not limited to) PNG, JPEG, GIF, TIFF, BMP and PCX.
     *
     * The texture will be stored in RGBA format, even if it is a file format
     * that does not support transparency (e.g. JPEG).
     *
     * IMPORTANT: In CUGL, relative path names always refer to the asset
     * directory. If you wish to load a texture from somewhere else, you must
     * use an absolute pathname.
     *
     * @param filename  The file supporting the texture file.
     *
     * @return true if initialization was successful.
     */
    bool initWithFile(const std::string filename);

    
#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a new empty texture with the given dimensions.
     *
     * Allocating a texture requires the use of the binding point at 0. Any 
     * texture bound to that point will be unbound. In addition, once 
     * allocation is done, this texture will not longer be bound as well.
     *
     * You must use the set() method to load data into the texture.
     *
     * @param width     The texture width in pixels
     * @param height    The texture height in pixels
     * @param format    The texture data format
     *
     * @return a new empty texture with the given dimensions.
     */
    static std::shared_ptr<Texture> alloc(int width, int height,
                                          PixelFormat format = PixelFormat::RGBA) {
        std::shared_ptr<Texture> result = std::make_shared<Texture>();
        return (result->init(width, height, (cugl::Texture::PixelFormat)format) ? result : nullptr);
    }

    /**
     * Returns a new texture with the given data.
     *
     * Allocating a texture requires the use of the binding point at 0. Any 
     * texture bound to that point will be unbound. In addition, once 
     * allocation is done, this texture will not longer be bound as well.
     *
     * The data format must match the one given.
     *
     * @param data      The texture data (size width*height*format)
     * @param width     The texture width in pixels
     * @param height    The texture height in pixels
     * @param format    The texture data format
     *
     * @return a new texture with the given data.
     */
    static std::shared_ptr<Texture> allocWithData(const void *data, int width, int height,
                                                  PixelFormat format = PixelFormat::RGBA) {
        std::shared_ptr<Texture> result = std::make_shared<Texture>();
        return (result->initWithData(data, width, height, (cugl::Texture::PixelFormat)format) ? result : nullptr);

    }
    
    /**
     * Returns a new texture with the data from the given file.
     *
     * Allocating a texture requires the use of the binding point at 0. Any 
     * texture bound to that point will be unbound. In addition, once 
     * allocation is done, this texture will not longer be bound as well.
     *
     * This method can load any file format supported by SDL_Image.  This
     * includes (but is not limited to) PNG, JPEG, GIF, TIFF, BMP and PCX.
     *
     * The texture will be stored in RGBA format, even if it is a file format
     * that does not support transparency (e.g. JPEG).
     *
     * @param filename  The file supporting the texture file.
     *
     * @return a new texture with the given data
     */
    static std::shared_ptr<Texture> allocWithFile(const std::string filename) {
        std::shared_ptr<Texture> result = std::make_shared<Texture>();
        return (result->initWithFile(filename) ? result : nullptr);
    }
    
    /**
     * Returns a blank texture that can be used to make solid shapes.
     *
     * Allocating a texture requires the use of the binding point at 0. Any 
     * texture bound to that point will be unbound. In addition, once 
     * allocation is done, this texture will not longer be bound as well.
     *
     * This is the texture used by {@link SpriteBatch} when the active texture 
     * is nullptr. It is a 2x2 texture of all white pixels. Using this texture 
     * means that all shapes and outlines will be drawn with a solid color.
     *
     * This texture is a singleton. There is only one of it.  All calls to
     * this method will return a reference to the same object.
     *
     * @return a blank texture that can be used to make solid shapes.
     */
    static const std::shared_ptr<Texture>& getBlank();

    
#pragma mark -
#pragma mark Setters
    /**
     * Sets this texture to have the contents of the given buffer.
     *
     * The buffer must have the correct data format. In addition, the buffer
     * must be size width*height*bytesize.  See {@link #getByteSize} for 
     * a description of the latter.
     *
     * This method is only successful if the texture is currently active.
     *
     * @param data  The buffer to read into the texture
     *
     * @return a reference to this (modified) texture for chaining.
     */
    const Texture& operator=(const void *data) {
        return set(data);
    }

    /**
     * Sets this texture to have the contents of the given buffer.
     *
     * The buffer must have the correct data format. In addition, the buffer
     * must be size width*height*bytesize.  See {@link #getByteSize} for 
     * a description of the latter.
     *
     * This method is only successful if the texture is currently active.
     *
     * @param data  The buffer to read into the texture
     *
     * @return a reference to this (modified) texture for chaining.
     */
    const Texture& set(const void *data);

    
#pragma mark -
#pragma mark Attributes
    /**
     * Sets the name of this texture.
     *
     * A name is a user-defined way of identifying a texture.  Subtextures are
     * permitted to have different names than their parents.
     *
     * @param name  The name of this texture.
     */
    void setName(std::string name) { _name = name; }
    
    /**
     * Returns the name of this texture.
     *
     * A name is a user-defined way of identifying a texture.  Subtextures are
     * permitted to have different names than their parents.
     *
     * @return the name of this texture.
     */
    const std::string& getName() const { return _name; }
    
    /**
     * Returns true if this texture has been loaded into memory.
     *
     * @return true if this texture has been loaded into memory.
     */
    bool isReady() const { return _buffer != 0; }
    
    /**
     * Returns the width of this texture in pixels.
     *
     * @return the width of this texture in pixels.
     */
    unsigned int getWidth()  const { return _width;  }
    
    /**
     * Returns the height of this texture in pixels.
     *
     * @return the height of this texture in pixels.
     */
    unsigned int getHeight() const { return _height; }
    
    /**
     * Returns the size of this texture in pixels.
     *
     * @return the size of this texture in pixels.
     */
    Size getSize() { return Size((float)_width,(float)_height); }
    
    /**
     * Returns the number of bytes in a single pixel of this texture.
     *
     * @return the number of bytes in a single pixel of this texture.
     */
    unsigned int getByteSize() const;
     
    /** 
     * Returns the data format of this texture.
     *
     * The data format determines what type of data can be assigned to this 
     * texture.
     *
     * @return the data format of this texture.
     */
    PixelFormat getFormat() const { return _pixelFormat; }

    /**
     * Returns whether this texture has generated mipmaps.
     *
     * If this texture is a subtexture of texture with mipmaps, this method will
     * also return true (and vice versa).
     *
     * @return whether this texture has generated mipmaps.
     */
    bool hasMipMaps() const {
        return (_parent != nullptr ? _parent->hasMipMaps() : _hasMipmaps);
    }

    /**
     * Builds mipmaps for the current texture.
     *
     * This method will fail if this texture is a subtexture.  Only the parent
     * texture can have mipmaps. In addition, mipmaps can only be built if the
     * texture size is a power of two.
     *
     * This method is only successful if the texture is currently active.
     */
    void buildMipMaps();
        
    /**
     * Returns the min filter of this texture.
     *
     * The min filter is the algorithm hint that OpenGL uses to make an
     * image smaller. The default is GL_NEAREST.
     *
     * @return the min filter of this texture.
     */
    GLuint getMinFilter() const {
        return (_parent != nullptr ? _parent->getMinFilter() : _minFilter);
    }

    /**
     * Returns the mag filter of this texture.
     *
     * The mag filter is the algorithm hint that OpenGL uses to make an
     * image larger. The default is GL_LINEAR.
     *
     * @return the mag filter of this texture.
     */
    GLuint getMagFilter() const {
        return (_parent != nullptr ? _parent->getMagFilter() : _magFilter);
    }

    /**
     * Sets the min filter of this texture.
     *
     * The min filter is the algorithm hint that OpenGL uses to make an
     * image smaller. The default is GL_NEAREST.
     *
     * This method may be safely called even if this texture is not active.
     * The preference will be applied once the texture is activated.
     *
     * @param minFilter The min filter of this texture.
     */
    void setMinFilter(GLuint minFilter);
    
    /**
     * Sets the mag filter of this texture.
     *
     * The mag filter is the algorithm hint that OpenGL uses to make an
     * image larger. The default is GL_LINEAR.
     *
     * This method may be safely called even if this texture is not active.
     * The preference will be applied once the texture is activated.
     *
     * @param magFilter The mag filter of this texture.
     */
    void setMagFilter(GLuint magFilter);

    /**
     * Returns the horizontal wrap of this texture.
     *
     * The default is GL_CLAMP_TO_EDGE.
     *
     * @return the horizontal wrap of this texture.
     */
    GLuint getWrapS() const {
        return (_parent != nullptr ? _parent->getWrapS() : _wrapS);
    }

    /**
     * Returns the vertical wrap of this texture.
     *
     * The default is GL_CLAMP_TO_EDGE.
     *
     * @return the vertical wrap of this texture.
     */
    GLuint getWrapT() const {
        return (_parent != nullptr ? _parent->getWrapT() : _wrapT);
    }

    /**
     * Sets the horizontal wrap of this texture.
     *
     * The default is GL_CLAMP_TO_EDGE.
     *
     * This method may be safely called even if this texture is not active.
     * The preference will be applied once the texture is activated.
     *
     * @param wrap  The horizontal wrap setting of this texture
     */
    void setWrapS(GLuint wrap);

    /**
     * Sets the vertical wrap of this texture.
     *
     * The default is GL_CLAMP_TO_EDGE.
     *
     * This method may be safely called even if this texture is not active.
     * The preference will be applied once the texture is activated.
     *
     * @param wrap  The vertical wrap setting of this texture
     */
    void setWrapT(GLuint wrap);

    
#pragma mark -
#pragma mark Atlas Support
    /** 
     * Returns the parent texture of this subtexture.
     *
     * This method will return nullptr is this is not a subtexture.
     *
     * Returns the parent texture of this subtexture.
     */
    const std::shared_ptr<Texture>& getParent() const { return _parent; }

    /**
     * Returns the parent texture of this subtexture.
     *
     * This method will return nullptr is this is not a subtexture.
     *
     * Returns the parent texture of this subtexture.
     */
    std::shared_ptr<Texture> getParent() { return _parent; }
    
    /**
     * Returns a subtexture with the given dimensions.
     *
     * The values must be 0 <= minS <= maxS <= 1 and 0 <= minT <= maxT <= 1.
     * They specify the region of the texture to extract the subtexture.
     *
     * It is the responsibility of the user to rescale the texture coordinates
     * when using subtexture.  Otherwise, the OpenGL pipeline will just use 
     * the original texture instead.  See the method internal method prepare of
     * {@link SpriteBatch} for an example of how to scale texture coordinates.
     *
     * It is possible to make a subtexture of a subtexture.  However, in that
     * case, the minS, maxS, minT and maxT values are all with respect to the
     * original root texture.  Furthermore, the parent of the new subtexture 
     * will be the original root texture.  So no tree of subtextures is more
     * than one level deep.
     *
     * Returns a subtexture with the given dimensions.
     */
    std::shared_ptr<Texture> getSubTexture(GLfloat minS, GLfloat maxS, GLfloat minT, GLfloat maxT);
    
    /**
     * Returns true if this texture is a subtexture.
     *
     * This is the same as checking if the parent is not nullptr.
     *
     * @return true if this texture is a subtexture.
     */
    bool isSubTexture() const { return _parent != nullptr; }

    /**
     * Returns the minimum S texture coordinate for this texture.
     *
     * When rescaling texture coordinates for a subtexture, this value is 
     * used in place of 0.
     *
     * @return the minimum S texture coordinate for this texture.
     */
    GLfloat getMinS() const { return _minS; }

    /**
     * Returns the minimum T texture coordinate for this texture.
     *
     * When rescaling texture coordinates for a subtexture, this value is
     * used in place of 0.
     *
     * @return the minimum T texture coordinate for this texture.
     */
    GLfloat getMinT() const { return _minT; }

    /**
     * Returns the maximum S texture coordinate for this texture.
     *
     * When rescaling texture coordinates for a subtexture, this value is
     * used in place of 0.
     *
     * @return the maximum S texture coordinate for this texture.
     */
    GLfloat getMaxS() const { return _maxS; }

    /**
     * Returns the maximum T texture coordinate for this texture.
     *
     * When rescaling texture coordinates for a subtexture, this value is
     * used in place of 0.
     *
     * @return the maximum T texture coordinate for this texture.
     */
    GLfloat getMaxT() const { return _maxT; }


#pragma mark -
#pragma mark Binding
    /**
     * Returns the OpenGL buffer for this texture.
     *
     * The buffer is a value assigned by OpenGL when the texture was allocated.
     * This method will return 0 if the texture is not initialized.
     *
     * @return the OpenGL buffer for this texture.
     */
    GLuint getBuffer() const {
        return (_parent != nullptr ? _parent->getBuffer() : _buffer);
    }
    
    /**
     * Returns the bind point for this texture.
     *
     * Textures and shaders have a many-to-many relationship. This means that 
     * connecting them requires an intermediate table. The positions in this 
     * table are called bind points. A texture is associated with a bind point 
     * and a shader associates a bind point with a sampler variable. That sampler 
     * variable then pulls data from the appropriate texture. By default this 
     * value is 0.
     *
     * @return the bind point for for this texture.
     */
    GLuint getBindPoint() const {
        return (_parent != nullptr ? _parent->getBindPoint() : _bindpoint);
    }
    
    /**
     * Sets the bind point for this texture.
     *
     * Textures and shaders have a many-to-many relationship. This means that 
     * connecting them requires an intermediate table. The positions in this 
     * table are called bind points. A texture is associated with a bind point 
     * and a shader associates a bind point with a sampler variable. That 
     * sampler variable then pulls data from the appropriate texture. By default 
     * this value is 0.
     *
     * The texture does not need to be active to call this method. This method 
     * only sets the bind point preference and does not actually {@link #bind} 
     * the texture. However, if the texture is bound to another bind point, then
     * it will be unbound from that point.
     *
     * @param point	The bind point for this texture.
     */
    void setBindPoint(GLuint point);
    
    /**
     * Binds this texture to its bind point, making
     *
     * Because of legacy issues with OpenGL, this method actually does two 
     * things. It attaches the block to the correct bind point, as defined by 
     * {@link #setBindPoint}. It also makes this the active texture, capable of
     * receiving OpenGL commands.
     *
     * Unlike {@link UniformBuffer} is not possible to bind a texture without
     * making it the active texture.  Therefore, any existing texture will 
     * be deactivated, no matter its bind point. So this texture can be unbound 
     * without a call to {@link #unbind}.
     *
     * This call is reentrant. If can be safely called multiple times.
     */
    void bind();
    
    /**
     * Unbinds this texture, making it neither bound nor active.
     *
     * If another texture is active, calling this method will not effect that
     * texture.  But once unbound, the shader will no longer receive data from
     * the bind point for this texture.  A new texture must be bound for the
     * shader to receive data.
     *
     * Unlike {@link UniformBuffer}, it is not possible to unbind a texture
     * without deactivating it.
     *
     * This call is reentrant.  If can be safely called multiple times.
     */
    void unbind();
    
    /**
     * Returns true if this texture is currently bound.
     *
     * A texture is bound if it is attached to a bind point. That means that 
     * the shader will pull sampler data for that bind point from this texture. 
     *
     * A texture can be bound without being active.  This happens when another
     * texture has subsequently been bound, but to a different bind point.
     *
     * @return true if this texture is currently bound.
     */
    bool isBound() const;

    /**
     * Returns true if this texture is currently active.
     *
     * An active uniform block is the one that receives data from OpenGL
     * calls (such as glTexImage2D). Many of the setter-like methods in
     * this class require the texture to be active to work properly (because
     * of how OpenGL calls are wrapped).
     *
     * Unlike {@link UniformBuffer}, it is not possible for a texture to be 
     * active without being bound. To activate a texture simply call the
     * {@link #bind} method.
     *
     * @return true if this texture is currently active.
     */
    bool isActive() const;


#pragma mark -
#pragma mark Conversions
    /**
     * Returns a string representation of this texture for debugging purposes.
     *
     * If verbose is true, the string will include class information.  This
     * allows us to unambiguously identify the class.
     *
     * @param verbose Whether to include class information
     *
     * @return a string representation of this texture for debugging purposes.
     */
    std::string toString(bool verbose = false) const;
    
    /** Casts from Texture to a string. */
    operator std::string() const { return toString(); }
     
    /**
     * Returns true if able to save the texture to the given file.
     *
     * The image will be saved as a PNG file.  If the suffix of file is not
     * .png, then this suffix will be added.
     *
     * This method is only successful if the texture is currently active.
     *
     * IMPORTANT: In CUGL, relative path names always refer to the asset
     * directory, which is a read-only directory.  Therefore, the file must
     * must be specified with an absolute path. Using a relative path
     * for this method will cause this method to fail.
     *
     * @param file  The name of the file to write to
     *
     * @return true if able to save the texture to the given file.
     */
    bool save(const std::string file);
      

};

}

#endif /* _CU_TEXTURE_H__ */
