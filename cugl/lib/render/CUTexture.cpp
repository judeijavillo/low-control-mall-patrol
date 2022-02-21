//
//  CUTexture.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides the class for representing a 2D OpenGL texture. This
//  class also provides support for texture atlases.  Any non-repeating texture
//  can produce a subtexture.  A subtexture wraps the same texture data (and so
//  does not require a context switch in the rendering pipeline), but has
//  different start and end boundaries, as defined by minS, maxS, minT and maxT
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
//  Version: 2/10/20
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <sstream>
#include <cugl/util/CUDebug.h>
#include <cugl/util/CUFiletools.h>
#include <cugl/render/CUTexture.h>

using namespace cugl;

#pragma mark Internal Helpers
/**
 * This array is the data of a white image with 2 by 2 dimension.
 * It's used for creating a default texture when the texture is a nullptr.
 */
static unsigned char cu_2x2_white_image[] = {
    // RGBA8888
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF
};

/**
 * Returns the internal format for the pixel format
 *
 * We have standardized internal formats for all platforms. This may not
 * be memory efficient, but it works for 90% of all use cases.
 *
 * @param format    The explicit pixel format
 *
 * @return the internal format for the pixel format
 */
GLint internal_format(Texture::PixelFormat format) {
    switch (format) {
        case Texture::PixelFormat::RGBA:
            return GL_RGBA8;
            break;
        case Texture::PixelFormat::RGB:
            return GL_RGB8;
            break;
        case Texture::PixelFormat::RED:
            return GL_R8;
            break;
        case Texture::PixelFormat::RED_GREEN:
            return GL_RG8;
            break;
        case Texture::PixelFormat::DEPTH:
            return GL_DEPTH_COMPONENT32F;
        case Texture::PixelFormat::DEPTH_STENCIL:
            return GL_DEPTH24_STENCIL8;
    }
    
    return GL_RGBA8;
}

/**
 * Returns the data type for the pixel format
 *
 * The data type is derived from the internal format. We have standardized
 * internal formats for all platforms. This may not be memory efficient, but
 * it works for 90% of all use cases.
 *
 * @param format    The explicit pixel format
 *
 * @return the data type for the pixel format
 */
GLenum format_type(Texture::PixelFormat format) {
    switch (format) {
        case Texture::PixelFormat::RGBA:
            return GL_UNSIGNED_BYTE;
            break;
        case Texture::PixelFormat::RGB:
            return GL_UNSIGNED_BYTE;
            break;
        case Texture::PixelFormat::RED:
            return GL_UNSIGNED_BYTE;
            break;
        case Texture::PixelFormat::RED_GREEN:
            return GL_UNSIGNED_BYTE;
            break;
        case Texture::PixelFormat::DEPTH:
            return GL_FLOAT;
        case Texture::PixelFormat::DEPTH_STENCIL:
            return GL_UNSIGNED_INT_24_8;
    }
    
    return GL_UNSIGNED_BYTE;
}

/**
 * Returns a copy of buffer expanded to RGBA representation.
 *
 * PNG files must be RBG or RGBA (and it is easiest if they are RGBA). However,
 * some textures (notably GL_RED and GL_RG) do not have enough bytes per pixel
 * to be written to a PNG file.  This function creates a new buffer to fill in
 * the missing pixel channels.  Missing alpha becomes 255 (or 1.0) and all other
 * missing colors become 0.
 *
 * This function transfers ownership fo the the byte buffer it returns. It is 
 * the responsibility of the caller to free this object when done. 
 *
 * @param buffer    The original byte buffer
 * @param width     The buffer width
 * @param height    The buffer height
 * @param bsize     The byte size per pixel
 *
 * @return a copy of buffer expanded to RGBA representation.
 */
unsigned char* expand2rgba(unsigned char* buffer, int width, int height, int bsize) {
    unsigned char* result = (unsigned char*)malloc(4*width*height);
    for(int ii = 0; ii < height; ii++) {
        for(int jj = 0; jj < width; jj++) {
            for(int kk = 0; kk < bsize; kk++) {
                int dstpos = ii*width*4+jj*4+kk;
                int srcpos = ii*width*bsize+jj*bsize+kk;
                result[dstpos] = buffer[srcpos];
            }
            for(int kk = bsize; kk < 4; kk++) {
                int dstpos = ii*width*4+jj*4+kk;
                result[dstpos] = kk == 3 ? 255 : 0;
            }
        }
    }
    return result;
}

/** The blank texture corresponding to cu_2x2_white_image */
std::shared_ptr<Texture> Texture::_blank = nullptr;

#pragma mark -
#pragma mark Constructors
/**
 * Creates a new empty texture with no size.
 *
 * This method performs no allocations.  You must call init to generate
 * a proper OpenGL texture.
 */
Texture::Texture() :
_buffer(0),
_width(0),
_height(0),
_name(""),
_pixelFormat(PixelFormat::RGBA),
_minFilter(GL_LINEAR),
_magFilter(GL_LINEAR),
_wrapS(GL_CLAMP_TO_EDGE),
_wrapT(GL_CLAMP_TO_EDGE),
_hasMipmaps(false),
_parent(nullptr),
_bindpoint(0),
_minS(0),
_maxS(1),
_minT(0),
_maxT(1),
_dirty(false) {}

/**
 * Deletes the OpenGL texture and resets all attributes.
 *
 * You must reinitialize the texture to use it.
 */
void Texture::dispose() {
    if (_buffer != 0) {
        // Do we own the texture?
        if (_parent == nullptr) {
            glDeleteTextures(1, &_buffer);
        }
        _buffer = 0;
        _width = 0; _height = 0;
        _pixelFormat = PixelFormat::RGBA;
        _name = "";
        _minFilter = GL_LINEAR; _magFilter = GL_LINEAR;
        _wrapS = GL_CLAMP_TO_EDGE; _wrapT = GL_CLAMP_TO_EDGE;
        _parent = nullptr;
        _minS = _minT = 0;
        _maxS = _maxT = 1;
        _hasMipmaps = false;
        _bindpoint  = 0;
        _dirty = false;
    }
}

/**
 * Initializes an texture with the given data.
 *
 * Initializing a texture requires the use of texture offset 0.  Any texture
 * bound to that offset will be unbound.  In addition, once initialization
 * is done, this texture will not longer be bound as well.
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
bool Texture::initWithData(const void *data, int width, int height, Texture::PixelFormat format) {
    CUAssertLog(width > 0 && height > 0, "Texture size %dx%d is not valid",width,height);
    //CUAssertLog(nextPOT(width)  == width, "Width  %d is not a power of two", width);
    //CUAssertLog(nextPOT(height) == width, "Height %d is not a power of two", height);
    GLenum error;

    if (_buffer) {
        CUAssertLog(false, "Texture is already initialized");
        return false; // In case asserts are off.
    }
    
    glGenTextures(1, &_buffer);
    if (_buffer == 0) {
        error = glGetError();
        CULogError("Could not allocate texture. %s", gl_error_name(error).c_str());
        return false;
    }
    
    _width  = width;
    _height = height;
    _pixelFormat = format;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _buffer);

    GLint  internal = internal_format(format);
    GLenum datatype = format_type(format);
    glTexImage2D(GL_TEXTURE_2D, 0, internal, width, height, 0, (GLenum)format, datatype, data);
    
    error = glGetError();
    if (error) {
        CULogError("Could not initialize texture. %s", gl_error_name(error).c_str());
        glDeleteTextures(1, &_buffer);
        _buffer = 0;
        return false;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _magFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _wrapT);

    glBindTexture(GL_TEXTURE_2D, 0);
    std::stringstream ss;
    ss << "@" << data;
    setName(ss.str());
    return true;
}

/**
 * Initializes an texture with the data from the given file.
 *
 * Initializing a texture requires the use of texture offset 0.  Any texture
 * bound to that offset will be unbound.  In addition, once initialization
 * is done, this texture will not longer be bound as well.
 *
 * This method can load any file format supported by SDL_Image.  This
 * includes (but is not limited to) PNG, JPEG, GIF, TIFF, BMP and PCX.
 *
 * The texture will be stored in RGBA format, even if it is a file format
 * that does not support transparency (e.g. JPEG).
 *
 * @param filename  The file supporting the texture file.
 *
 * @return true if initialization was successful.
 */
bool Texture::initWithFile(const std::string filename) {
    std::string fullpath = filetool::normalize_path(filename);
    SDL_Surface* surface = IMG_Load(fullpath.c_str());
    if (surface == nullptr) {
        CULogError("Could not load file %s. %s", filename.c_str(), SDL_GetError());
        return false;
    }
    
    SDL_Surface* normal;
#if CU_MEMORY_ORDER == CU_ORDER_REVERSED
    normal = SDL_ConvertSurfaceFormat(surface,SDL_PIXELFORMAT_ABGR8888,0);
#else
    normal = SDL_ConvertSurfaceFormat(surface,SDL_PIXELFORMAT_RGBA8888,0);
#endif
    SDL_FreeSurface(surface);
    if (normal == nullptr) {
        CULogError("Could not process file %s. %s", filename.c_str(), SDL_GetError());
        return false;
    }

    bool result = initWithData(normal->pixels, normal->w, normal->h);
    SDL_FreeSurface(normal);
    if (result) setName(filename);
    return result;
}

/**
 * Returns a blank texture that can be used to make solid shapes.
 *
 * Allocating a texture requires the use of texture offset 0.  Any texture
 * bound to that offset will be unbound.  In addition, once initialization
 * is done, this texture will not longer be bound as well.
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
const std::shared_ptr<Texture>& Texture::getBlank() {
    if (_blank == nullptr) {
        _blank = Texture::allocWithData(cu_2x2_white_image, 2, 2);
        _blank->bind();
        _blank->setWrapS(GL_REPEAT);
        _blank->setWrapT(GL_REPEAT);
        _blank->unbind();
    }
    return _blank;
}  


#pragma mark -
#pragma mark Setters
/**
 * Sets this texture to have the contents of the given buffer.
 *
 * The buffer must have the correct data format.  In addition, the buffer
 * must be size width*height*format.
 *
 * This method is only successful if the texture is currently bound to its
 * slot.
 * If it is bound, it will make its offset the active location.
 *
 * @param data  The buffer to read into the texture
 *
 * @return a reference to this (modified) texture for chaining.
 */
const Texture& Texture::set(const void *data) {
    if (!isActive()) {
        CUAssertLog(false,"Texture %s is not currently active.",_name.c_str());
        return *this;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, (GLenum)_pixelFormat, _width, _height, 0,
                 (GLenum)_pixelFormat, GL_UNSIGNED_BYTE, data);
    return *this;
}


#pragma mark -
#pragma mark Attributes
/**
 * Returns the number of bytes in a single pixel of this texture.
 *
 * @return the number of bytes in a single pixel of this texture.
 */
unsigned int Texture::getByteSize() const {
    switch (_pixelFormat) {
        case Texture::PixelFormat::RGBA:
            return 4;
            break;
        case Texture::PixelFormat::RGB:
            return 3;
            break;
        case Texture::PixelFormat::RED:
            return 1;
            break;
        case Texture::PixelFormat::RED_GREEN:
            return 2;
            break;
        case Texture::PixelFormat::DEPTH:
            return 4;
        case Texture::PixelFormat::DEPTH_STENCIL:
            return 4;
    }
    
    return GL_RGBA8;
}

/**
 * Builds mipmaps for the current texture.
 *
 * This method will fail if this texture is a subtexture.  Only the parent
 * texture can have mipmaps.  In addition, mipmaps can only be built if the
 * texture size is a power of two.
 *
 * This method is only successful if the texture is currently active.
 */
void Texture::buildMipMaps() {
    CUAssertLog(nextPOT(_width)  == _width,  "Width  %d is not a power of two", _width);
    CUAssertLog(nextPOT(_height) == _height, "Height %d is not a power of two", _height);
    CUAssertLog(_parent == nullptr, "Cannot build mipmaps for a subtexture");
    CUAssertLog(isActive(), "Texture is not active");
    glGenerateMipmap(GL_TEXTURE_2D);
    _hasMipmaps = true;
}

/**
 * Sets the min filter of this texture.
 *
 * The min filter is the algorithm hint that OpenGL uses to make an
 * image smaller.  The default is GL_NEAREST.
 *
 * This method may be safely called even if this texture is not bound.
 * The preference will be applied once the texture is bound.
 *
 * This method may not be called on a subtexture.  It has to be set on
 * the parent texture.
 *
 * @param minFilter The min filter of this texture.
 */
void Texture::setMinFilter(GLuint minFilter) {
    CUAssertLog(_parent == nullptr, "Cannot set filters for a subtexture");
    _minFilter = minFilter;
    if (isActive()) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _minFilter);
    } else {
    	_dirty = true;
    }
}

/**
 * Sets the mag filter of this texture.
 *
 * The mag filter is the algorithm hint that OpenGL uses to make an
 * image larger.  The default is GL_LINEAR.
 *
 * This method may be safely called even if this texture is not bound.
 * The preference will be applied once the texture is bound.
 *
 * This method may not be called on a subtexture.  It has to be set on
 * the parent texture.
 *
 * @param magFilter The mag filter of this texture.
 */
void Texture::setMagFilter(GLuint magFilter) {
    CUAssertLog(_parent == nullptr, "Cannot set filters for a subtexture");
    _magFilter = magFilter;
    if (isActive()) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _magFilter);
	} else {
    	_dirty = true;
    }
}

/**
 * Sets the horizontal wrap of this texture.
 *
 * The default is GL_CLAMP_TO_EDGE.
 *
 * This method may be safely called even if this texture is not bound.
 * The preference will be applied once the texture is bound.
 *
 * This method may not be called on a subtexture.  It has to be set on
 * the parent texture.
 *
 * @param wrap	The horizontal wrap setting of this texture
 */
void Texture::setWrapS(GLuint wrap) {
    CUAssertLog(_parent == nullptr, "Cannot set wrap S for a subtexture");
    _wrapS = wrap;
    if (isActive()) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _wrapS);
	} else {
    	_dirty = true;
    }
}

/**
 * Sets the vertical wrap of this texture.
 *
 * The default is GL_CLAMP_TO_EDGE.
 *
 * This method may be safely called even if this texture is not bound.
 * The preference will be applied once the texture is bound.
 *
 * This method may not be called on a subtexture.  It has to be set on
 * the parent texture.
 *
 * @param wrap	The vertical wrap setting of this texture
 */
void Texture::setWrapT(GLuint wrap) {
    CUAssertLog(_parent == nullptr, "Cannot set wrap T for a subtexture");
    _wrapT = wrap;
    if (isActive()) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _wrapT);
	} else {
    	_dirty = true;
    }
}


#pragma mark -
#pragma mark Atlas Support
/**
 * Returns a subtexture with the given dimensions.
 *
 * The values must be 0 <= minS <= maxS <= 1 and 0 <= minT <= maxT <= 1.
 * They specify the region of the texture to extract the subtexture.
 *
 * It is the responsibility of the user to rescale the texture coordinates
 * when using subtexture.  Otherwise, the OpenGL pipeline will just use
 * the original texture instead.  See the method {@link SpriteBatch#prepare()}
 * for an example of how to scale texture coordinates.
 *
 * It is possible to make a subtexture of a subtexture.  However, in that
 * case, the minS, maxS, minT and maxT values are all with respect to the
 * original root texture.  Furthermore, the parent of the new subtexture
 * will be the original root texture.  So no tree of subtextures is more
 * than one level deep.
 *
 * Subtextures may not have different minFilter, magFilter or wrap settings
 * that their parent.
 *
 * Returns  a subtexture with the given dimensions.
 */
std::shared_ptr<Texture> Texture::getSubTexture(GLfloat minS, GLfloat maxS,
                                                GLfloat minT, GLfloat maxT) {
    CUAssertLog(_buffer, "Texture is not initialized");
    CUAssertLog(minS >= _minS && minS <= maxS, "Value minS is out of range");
    CUAssertLog(maxS <= _maxS, "Value maxS is out of range");
    CUAssertLog(minT >= _minT && minT <= maxT, "Value minT is out of range");
    CUAssertLog(maxT <= _maxT, "Value maxT is out of range");
    
    std::shared_ptr<Texture> result = std::make_shared<Texture>();

    // Make sure the tree is not deep
    std::shared_ptr<Texture> source = (_parent == nullptr ? shared_from_this() : _parent);
    
    // Shared values
    result->_buffer = source->_buffer;
    result->_parent = source;
    result->_pixelFormat = source->_pixelFormat;
    result->_name = source->_name;
    
    // Filters, wrap, and binding defer to parent.
    // These values can be left alone.
    
    // Set the size information
    result->_width  = (unsigned int)((maxS-minS)*source->_width);
    result->_height = (unsigned int)((maxT-minT)*source->_height);
    result->_minS = minS;
    result->_maxS = maxS;
    result->_minT = minT;
    result->_maxT = maxT;

    return result;
}


#pragma mark -
#pragma mark Rendering
/**
 * Sets the texture location for this texture.
 *
 * The texture location is used by a shader program to assign a sampler
 * to texture.  That is, the uniform variable for a sampler takes a texture
 * location (not a texture buffer) as its value.
 *
 * By default this value is 0.  However, for shaders that use multiple
 * textures it may be necessary to bind a texture to a non-zero slot.
 *
 * If this texture is bound, calling this method will unbind the texture
 * from its current slot before reassignment.
 *
 * @param the texture location to associate with this texture.
 */
void Texture::setBindPoint(GLuint point) {
    GLint orig;
    glGetIntegerv(GL_ACTIVE_TEXTURE,&orig);
    if (orig != _bindpoint+GL_TEXTURE0) {
        glActiveTexture(GL_TEXTURE0+_bindpoint);
    }
    GLint bind;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &bind);
    if (bind == _buffer) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    if (orig != _bindpoint+GL_TEXTURE0) {
        glActiveTexture(orig);
    }
    GLenum error = glGetError();
    CUAssertLog(error == GL_NO_ERROR, "Texture: %s", gl_error_name(error).c_str());
    _bindpoint = point;
}

/**
 * Binds this texture, making it active.
 *
 * This method does to things.  First, it switches the active texture slot to
 * the one for this texture, as specified by {@link #getSlot}.  Then it binds
 * this texture to this slot.  Both of these are required to pass a texture to a
 * shader via a {@link VertexBuffer}.
 *
 * Binding a texture to a slot replaces the texture originally there.  So this
 * texture can be unbound without a call to {@link #unbound}.  However,
 * if a texture is bound to a different slot than this texture, this texture will
 * remain bound to the slot, but will no longer be the active texture.  See
 * {@link #isactive} for more information.
 *
 * This call is reentrant.  If can be safely called multiple times.
 */
void Texture::bind() {
    if (_parent != nullptr) {
        _parent->bind();
        return;
    }
    
    glActiveTexture(GL_TEXTURE0+_bindpoint);
    glBindTexture(GL_TEXTURE_2D,_buffer);
    if (_dirty) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _magFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _wrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _wrapT);
		_dirty = false;    
    }
}

/**
 * Unbinds this texture, making it neither bound nor active.
 *
 * If another texture is active, calling this method will not effect that
 * texture.
 *
 * Once unbound, the slot of this shader will be assigned no texture.
 * So the shader will no longer receive any texture data for this slot.
 *
 * This call is reentrant.  If can be safely called multiple times.
 */
void Texture::unbind() {
    if (_parent != nullptr) {
        _parent->unbind();
        return;
    }

    GLint orig;
    glGetIntegerv(GL_ACTIVE_TEXTURE,&orig);
    if (orig != _bindpoint+GL_TEXTURE0) {
        glActiveTexture(GL_TEXTURE0+_bindpoint);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    if (orig != _bindpoint+GL_TEXTURE0) {
        glActiveTexture(orig);
    }
}

/**
 * Returns true if this texture is bound to its slot.
 *
 * To be bound, we only require that this texture be assigned to this slot
 * (so this texture will be used by the sampler attached to that slot).  We
 * do not require that it be the active texture.  See {@link #isactive} for
 * a discussion of the difference.
 *
 * Even if a texture is bound its slot, if it is not active you must call
 * {@link #bind} to reactivate it.
 *
 * @return true if this texture is bound to its specified slot.
 */
bool Texture::isBound() const {
    if (!_buffer) {
        return false;
    }
    
    GLint orig;
    glGetIntegerv(GL_ACTIVE_TEXTURE,&orig);
    if (orig != _bindpoint+GL_TEXTURE0) {
        glActiveTexture(GL_TEXTURE0+_bindpoint);
    }
    GLint bind;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &bind);
    bool result = (bind == _buffer);
    if (orig != _bindpoint+GL_TEXTURE0) {
        glActiveTexture(orig);
    }
    return result;
}

/**
 * Returns true if this texture is the active texture.
 *
 * To be an active texture, the texture has to be bound and its slot has to
 * be the active texture slot (there can only be one active texture slot at a
 * time).  A texture can be bound to a slot even if its slot is not active.
 * See {@link #isbound} for more information.
 *
 * Even if a texture is bound its slot, if it is not active you must call
 * {@link #bind} to reactivate it.
 *
 * @return true if this texture is bound to its specified slot.
 */
bool Texture::isActive() const {
    if (!_buffer) {
        return false;
    }
    GLint orig;
    glGetIntegerv(GL_ACTIVE_TEXTURE,&orig);
    if (orig != _bindpoint+GL_TEXTURE0) {
        return false;
    }
    GLint bind;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &bind);
    return (bind == _buffer);
}


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
std::string Texture::toString(bool verbose) const {
    std::stringstream ss;
    ss << (verbose ? "cugl::Texture[" : "[");
    ss << "data:" << getName() << ",";
    ss << "w:" << getWidth() << ",";
    ss << "h:" << getWidth();
    if (_parent != nullptr) {
        ss << ",";
        ss << " (" << _minS << "," << _maxS << ")";
        ss << "x(" << _minT << "," << _maxT << ")";
    }
    ss << "]";
    return ss.str();
}

/**
 * Returns true if able to save the texture to the given file.
 *
 * The image will be saved as a PNG file.  If the suffix of file is not
 * .png, then this suffix will be added.
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
bool Texture::save(const std::string file) {
#if CU_GL_PLATFORM == CU_GL_OPENGLES
    CUAssertLog(false, "Texture saving is not supported in OpenGLES");
    return false;
#else
    if (!isActive()) {
        CUAssertLog(false,"Texture %s is not currently active.",_name.c_str());
        return false;
    } else if (!filetool::is_absolute(file)) {
        CUAssertLog(false, "Data may not be saved to the asset directory.");
        return false;
    }

    // Make sure file is named properly.
    std::string fullpath = filetool::normalize_path(filetool::set_suffix(file, "png"));
    bind();
    
    // Masks appear to be necessary for alpha support
    Uint32 rmask, gmask, bmask, amask;

    // Unfortunately, masks are endian
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
    
    SDL_Surface* surface;
    unsigned int bsize = getByteSize();
    unsigned char* buffer = (unsigned char*)malloc(bsize*_width*_height);
    glGetTexImage(GL_TEXTURE_2D,0,(GLenum)_pixelFormat,format_type(_pixelFormat),buffer);
    GLenum error = glGetError();
    if (error) {
        CULogError("Could not write file %s. %s", file.c_str(), gl_error_name(error).c_str());
        free(buffer);
        return false;
    }

    if (bsize < 4) {
        unsigned char* padded = expand2rgba(buffer, _width, _height, bsize);
        free(buffer);
        buffer = padded;
    }
    
    
    surface = SDL_CreateRGBSurfaceFrom(buffer,_width,_height,32,_width*4,
                                       rmask,gmask,bmask,amask);
    if (surface == NULL) {
        CULogError("Could not write file %s. %s", file.c_str(), SDL_GetError());
        free(buffer);
        return false;
    }
    
    int success = IMG_SavePNG(surface,fullpath.c_str());
    if (success == -1) {
        CULogError("Could not write file %s. %s", file.c_str(), SDL_GetError());
        free(buffer);
        return false;
    }
    
    free(buffer);
    SDL_FreeSurface(surface);
    return true;
#endif
}    
