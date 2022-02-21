//
//  CUSpriteBatch.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides one-stop shopping for basic 2d graphics.  Despite the
//  name, it is also capable of drawing solid shapes, as well as wireframes.
//  It also has support for color gradients and (rotational) scissor masks.
//
//  While it is possible to swap out the shader for this class, the shader is
//  very peculiar in how it uses uniforms.  You should study SpriteShader.frag
//  and SpriteShader.vert before making any shader changes to this class.
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
//  Version: 7/29/21
//
#include <cugl/math/cu_math.h>
#include <cugl/util/CUDebug.h>
#include <cugl/render/CUSpriteBatch.h>
#include <cugl/render/CUVertexBuffer.h>
#include <cugl/render/CUTexture.h>
#include <cugl/render/CUShader.h>
#include <cugl/render/CUGradient.h>
#include <cugl/render/CUScissor.h>
#include <cugl/render/CUFont.h>
#include <cugl/render/CUGlyphRun.h>
#include <cugl/render/CUTextLayout.h>

/**
 * Default fragment shader
 *
 * This trick uses C++11 raw string literals to put the shader in a separate
 * file without having to guarantee its presence in the asset directory.
 * However, to work properly, the #include statement below MUST be on its
 * own separate line.
 */
const std::string oglShaderFrag =
#include "shaders/SpriteShader.frag"
;

/**
 * Default vertex shader
 *
 * This trick uses C++11 raw string literals to put the shader in a separate
 * file without having to guarantee its presence in the asset directory.
 * However, to work properly, the #include statement below MUST be on its
 * own separate line.
 */
const std::string oglShaderVert =
#include "shaders/SpriteShader.vert"
;

using namespace cugl;


#pragma mark PIPELINE FLAGS

/** The drawing type for a textured mesh */
#define TYPE_TEXTURE    1
/** The drawing type for a gradient mesh */
#define TYPE_GRADIENT   2
/** The drawing type for a scissored mesh */
#define TYPE_SCISSOR    4
/** The drawing type for a (simple) texture blur */
#define TYPE_GAUSSBLUR  8

/** The drawing command has changed */
#define DIRTY_COMMAND           0x001
/** The blending equation has changed */
#define DIRTY_BLENDEQUATION     0x002
/** The source blending functions have changed */
#define DIRTY_SRC_FUNCTION      0x004
/** The destination blending functions have changed */
#define DIRTY_DST_FUNCTION      0x008
/** The depth value has changed */
#define DIRTY_DEPTHVALUE        0x010
/** The drawing type has changed */
#define DIRTY_DRAWTYPE          0x020
/** The perspective matrix has changed */
#define DIRTY_PERSPECTIVE       0x040
/** The stencil effect has changed */
#define DIRTY_STENCIL_EFFECT    0x080
/** The stencil effect was cleared since last draw */
#define DIRTY_STENCIL_CLEAR     0x100
/** The texture has changed */
#define DIRTY_TEXTURE           0x200
/** The blur step has changed */
#define DIRTY_BLURSTEP          0x400
/** The block offset has changed */
#define DIRTY_UNIBLOCK          0x800
/** All values have changed */
#define DIRTY_ALL_VALS          0xFFF

/** Clear no buffers */
#define STENCIL_NONE            0x000
/** Clear lower buffer */
#define STENCIL_LOWER           0x001
/** Clear upper buffer */
#define STENCIL_UPPER           0x002
/** Clear both buffers */
#define STENCIL_BOTH            0x003

/**
 * Fills poly with a mesh defining the given rectangle.
 *
 * We need this method because we want to allow non-standard
 * polygons that represent a path, and not a triangulated
 * polygon.
 *
 * @param poly      The polygon to store the result
 * @param rect      The source rectangle
 * @param solid     Whether to triangulate the rectangle
 */
static void makeRect(Poly2& poly, const Rect& rect, bool solid) {
    poly.vertices.reserve(4);
    poly.vertices.push_back(rect.origin);
    poly.vertices.push_back(Vec2(rect.origin.x+rect.size.width, rect.origin.y));
    poly.vertices.push_back(rect.origin+rect.size);
    poly.vertices.push_back(Vec2(rect.origin.x, rect.origin.y+rect.size.height));
    if (solid) {
        poly.indices.reserve(6);
        poly.indices.push_back(0);
        poly.indices.push_back(1);
        poly.indices.push_back(2);
        poly.indices.push_back(0);
        poly.indices.push_back(2);
        poly.indices.push_back(3);
    } else {
        poly.indices.reserve(8);
        poly.indices.push_back(0);
        poly.indices.push_back(1);
        poly.indices.push_back(1);
        poly.indices.push_back(2);
        poly.indices.push_back(2);
        poly.indices.push_back(3);
        poly.indices.push_back(3);
        poly.indices.push_back(0);
    }
}

#pragma mark -
#pragma mark Context
/**
 * A class storing the drawing context for the associate shader.
 *
 * Because we want to minimize the number of times we load vertices
 * to the vertex buffer, all uniforms are recorded and delayed until the
 * final graphics call.  We include blending attributes as part of the
 * context, since they have similar performance characteristics to
 * other uniforms
 */
class SpriteBatch::Context {
public:
    /**
     * Creates a context of the default uniforms.
     */
    Context() {
        first = 0;
        last  = 0;
        command  = GL_TRIANGLES;
        blendEq  = GL_FUNC_ADD;
        srcRGB   = GL_SRC_ALPHA;
        srcAlpha = GL_SRC_ALPHA;
        dstRGB   = GL_ONE_MINUS_SRC_ALPHA;
        dstAlpha = GL_ONE_MINUS_SRC_ALPHA;
        perspective = std::make_shared<Mat4>();
        perspective->setIdentity();
        stencil  = StencilEffect::NATIVE;
        cleared  = STENCIL_NONE;
        texture  = nullptr;
        blockptr = -1;
        zDepth = 0;
        blur = 0;
        type = 0;
        dirty = 0;
    }
    
    /**
     * Creates a copy of the given uniforms
     *
     * @param copy  The uniforms to copy
     */
    Context(Context* copy) {
        first = copy->first;
        last  = copy->last;
        type  = copy->type;
        command  = copy->command;
        blendEq  = copy->blendEq;
        srcRGB   = copy->srcRGB;
        srcAlpha = copy->srcAlpha;
        dstRGB   = copy->dstRGB;
        dstAlpha = copy->dstAlpha;
        perspective = copy->perspective;
        stencil  = copy->stencil;
        cleared  = STENCIL_NONE; // DO NOT COPY
        texture  = copy->texture;
        blockptr = copy->blockptr;
        zDepth = copy->zDepth;
        blur  = copy->blur;
        dirty = 0;
    }
    
    /**
     * Disposes this collection of uniforms
     */
    ~Context() {
        first = 0;
        last  = 0;
        command  = GL_FALSE;
        blendEq  = GL_FALSE;
        srcRGB   = GL_FALSE;
        srcAlpha = GL_FALSE;
        dstRGB   = GL_FALSE;
        dstAlpha = GL_FALSE;
        perspective = nullptr;
        stencil  = StencilEffect::NATIVE;
        cleared  = STENCIL_NONE;
        texture  = nullptr;
        blockptr = -1;
        zDepth = 0;
        blur = 0;
        type = 0;
    }
    
    /**
     *
     * Resets this context to its default values
     */
    void reset() {
        if (texture != nullptr) {
            texture->unbind();
        }
        first = 0;
        last  = 0;
        command  = GL_TRIANGLES;
        blendEq  = GL_FUNC_ADD;
        srcRGB   = GL_SRC_ALPHA;
        srcAlpha = GL_SRC_ALPHA;
        dstRGB   = GL_ONE_MINUS_SRC_ALPHA;
        dstAlpha = GL_ONE_MINUS_SRC_ALPHA;
        perspective = std::make_shared<Mat4>();
        perspective->setIdentity();
        stencil  = StencilEffect::NATIVE;
        cleared  = STENCIL_NONE;
        texture  = nullptr;
        blockptr = -1;
        zDepth = 0;
        blur = 0;
        type = 0;
        dirty = 0;
    }
    
    /** The first vertex index position for this set of uniforms */
    GLuint first;
    /** The last vertex index position for this set of uniforms */
    GLuint last;
    /** The drawing type for the shader */
    GLint type;
    /** The stored drawing command */
    GLenum command;
    /** The stored blending equation */
    GLenum blendEq;
    /** The stored source RGB blending function */
    GLenum srcRGB;
    /** The stored source alpha blending function */
    GLenum srcAlpha;
    /** The stored destination RGB blending function */
    GLenum dstRGB;
    /** The stored destination alpha blending function */
    GLenum dstAlpha;
    /** The current stencil effect */
    StencilEffect stencil;
    /** The stencil buffer to clear */
    GLenum cleared;
    /** The stored perspective matrix */
    std::shared_ptr<Mat4> perspective;
    /** The stored texture */
    std::shared_ptr<Texture> texture;
    /** The current depth for the z-plane */
    GLfloat zDepth;
    /** The radius for our blur function */
    GLfloat blur;
    /** The stored block offset for gradient and scissor */
    GLsizei blockptr;
    /** The dirty bits relative to the previous set of uniforms */
    GLuint dirty;
};

#pragma mark -
#pragma mark Constructors
/**
 * Creates a degenerate sprite batch with no buffers.
 *
 * You must initialize the buffer before using it.
 */
SpriteBatch::SpriteBatch() :
_initialized(false),
_active(false),
_inflight(false),
_vertData(nullptr),
_indxData(nullptr),
_color(Color4f::WHITE),
_context(nullptr),
_vertMax(0),
_vertSize(0),
_indxMax(0),
_indxSize(0),
_vertTotal(0),
_callTotal(0) {
    _shader = nullptr;
    _vertbuff = nullptr;
    _unifbuff = nullptr;
    _gradient = nullptr;
    _scissor  = nullptr;
}

/**
 * Deletes the vertex buffers and resets all attributes.
 *
 * You must reinitialize the sprite batch to use it.
 */
void SpriteBatch::dispose() {
    if (_vertData) {
        delete[] _vertData; _vertData = nullptr;
    }
    if (_indxData) {
        delete[] _indxData; _indxData = nullptr;
    }
    if (_context != nullptr) {
        delete _context; _context = nullptr;
    }
    _shader = nullptr;
    _vertbuff = nullptr;
    _unifbuff = nullptr;
    _gradient = nullptr;
    _scissor  = nullptr;
    
    _vertMax  = 0;
    _vertSize = 0;
    _indxMax  = 0;
    _indxSize = 0;
    _color = Color4f::WHITE;
    
    _vertTotal = 0;
    _callTotal = 0;
    
    _initialized = false;
    _inflight = false;
    _active = false;
}

/**
 * Initializes a sprite batch with the default vertex capacity.
 *
 * The default vertex capacity is 8192 vertices and 8192*3 = 24576 indices.
 * If the mesh exceeds these values, the sprite batch will flush before
 * before continuing to draw. Similarly uniform buffer is initialized with
 * 512 buffer positions. This means that the uniform buffer is comparable
 * in memory size to the vertices, but only allows 512 gradient or scissor
 * mask context switches before the sprite batch must flush. If you wish to
 * increase (or decrease) the capacity, use the alternate initializer.
 *
 * The sprite batch begins with the default blank texture, and color white.
 * The perspective matrix is the identity.
 *
 * @return true if initialization was successful.
 */
bool SpriteBatch::init() {
    return init(DEFAULT_CAPACITY,Shader::alloc(SHADER(oglShaderVert),SHADER(oglShaderFrag)));
}

/** 
 * Initializes a sprite batch with the default vertex capacity.
 *
 * The index capacity will be 3 times the vertex capacity. The maximum
 * number of possible indices is the maximum size_t, so the vertex size
 * must be a third that.  In addition, the sprite batch will allocate
 * 1/16 of the vertex capacity for uniform blocks (for gradients and
 * scissor masks). This means that the uniform buffer is comparable
 * in memory size to the vertices while still allowing a reasonably
 * high rate of change for quads and regularly shaped sprites.
 *
 * If the mesh exceeds the capacity, the sprite batch will flush before
 * before continuing to draw. You should tune your system to have the
 * appropriate capacity.  To small a capacity will cause the system to
 * thrash.  However, too large a capacity could stall on memory transfers.
 *
 * The sprite batch begins with no active texture, and the color white.
 * The perspective matrix is the identity.
 *
 * @return true if initialization was successful.
 */
bool SpriteBatch::init(unsigned int capacity) {
    return init(capacity,Shader::alloc(SHADER(oglShaderVert),SHADER(oglShaderFrag)));
}

/**
 * Initializes a sprite batch with the default vertex capacity and given shader
 *
 * The index capacity will be 3 times the vertex capacity. The maximum
 * number of possible indices is the maximum size_t, so the vertex size
 * must be a third that.  In addition, the sprite batch will allocate
 * 1/16 of the vertex capacity for uniform blocks (for gradients and
 * scissor masks). This means that the uniform buffer is comparable
 * in memory size to the vertices while still allowing a reasonably
 * high rate of change for quads and regularly shaped sprites.
 *
 * If the mesh exceeds the capacity, the sprite batch will flush before
 * before continuing to draw. You should tune your system to have the
 * appropriate capacity.  To small a capacity will cause the system to
 * thrash.  However, too large a capacity could stall on memory transfers.
 *
 * The sprite batch begins with no active texture, and the color white.
 * The perspective matrix is the identity.
 *
 * See the class description for the properties of a valid shader.
 *
 * @param shader The shader to use for this spritebatch
 *
 * @return true if initialization was successful.
 */
bool SpriteBatch::init(unsigned int capacity, const std::shared_ptr<Shader>& shader) {
    if (_initialized) {
        CUAssertLog(false, "SpriteBatch is already initialized");
        return false; // If asserts are turned off.
    } else if (shader == nullptr) {
        CUAssertLog(false, "SpriteBatch shader cannot be null");
        return false; // If asserts are turned off.
    }
    
    _shader = shader;
    
    _vertbuff = VertexBuffer::alloc(sizeof(SpriteVertex2));
    _vertbuff->setupAttribute("aPosition", 2, GL_FLOAT, GL_FALSE,
                              offsetof(cugl::SpriteVertex2,position));
    _vertbuff->setupAttribute("aColor",    4, GL_UNSIGNED_BYTE, GL_TRUE,
                              offsetof(cugl::SpriteVertex2,color));
    _vertbuff->setupAttribute("aTexCoord", 2, GL_FLOAT, GL_FALSE,
                              offsetof(cugl::SpriteVertex2,texcoord));
    _vertbuff->setupAttribute("aGradCoord",2, GL_FLOAT, GL_FALSE,
                              offsetof(cugl::SpriteVertex2,gradcoord));
    _vertbuff->attach(_shader);
    
    // Set up data arrays;
    _vertMax = capacity;
    _vertData = new SpriteVertex2[_vertMax];
    _indxMax = capacity*3;
    _indxData = new GLuint[_indxMax];
    
    // Create uniform buffer (this has its own backing array)
    _unifbuff = UniformBuffer::alloc(40*sizeof(float),capacity/16);
    
    // Layout std140 format
    _unifbuff->setOffset("scMatrix", 0);
    _unifbuff->setOffset("scExtent", 48);
    _unifbuff->setOffset("scScale",  56);
    _unifbuff->setOffset("gdMatrix", 64);
    _unifbuff->setOffset("gdInner",  112);
    _unifbuff->setOffset("gdOuter",  128);
    _unifbuff->setOffset("gdExtent", 144);
    _unifbuff->setOffset("gdRadius", 152);
    _unifbuff->setOffset("gdFeathr", 156);

    _shader->setUniformBlock("uContext",_unifbuff);
    
    _context = new Context();
    _context->dirty = DIRTY_ALL_VALS;
    return true;
}


#pragma mark -
#pragma mark Attributes
/**
 * Sets the active color of this sprite batch 
 *
 * All subsequent shapes and outlines drawn by this sprite batch will be
 * tinted by this color.  This color is white by default.
 *
 * @param color The active color for this sprite batch
 */
void SpriteBatch::setColor(const Color4 color) {
    if (_color == color) {
        return;
    }
    _color = color;
}

/**
 * Sets the shader for this sprite batch
 *
 * This value may NOT be changed during a drawing pass.
 *
 * @param color The active color for this sprite batch
 */
void SpriteBatch::setShader(const std::shared_ptr<Shader>& shader) {
    CUAssertLog(_active, "Attempt to reassign shader while drawing is active");
    CUAssertLog(shader != nullptr, "Shader cannot be null");
    _vertbuff->detach();
    _shader = shader;
    _vertbuff->attach(_shader);
    _shader->setUniformBlock("uContext", _unifbuff);
}


/**
 * Sets the active perspective matrix of this sprite batch
 *
 * The perspective matrix is the combined modelview-projection from the
 * camera. By default, this is the identity matrix. Changing this value
 * will cause the sprite batch to flush.
 *
 * @param perspective   The active perspective matrix for this sprite batch
 */
void SpriteBatch::setPerspective(const Mat4& perspective) {
    if (_context->perspective.get() != &perspective) {
        if (_inflight) { record(); }
        auto matrix = std::make_shared<Mat4>(perspective);
        _context->perspective = matrix;
        _context->dirty = _context->dirty | DIRTY_PERSPECTIVE;
    }
}

/**
 * Returns the active perspective matrix of this sprite batch
 *
 * The perspective matrix is the combined modelview-projection from the
 * camera.  By default, this is the identity matrix.
 *
 * @return the active perspective matrix of this sprite batch
 */
const Mat4& SpriteBatch::getPerspective() const {
    return *(_context->perspective.get());
}

/**
 * Sets the active texture of this sprite batch
 *
 * All subsequent shapes and outlines drawn by this sprite batch will use
 * this texture.  If the value is nullptr, all shapes and outlines will be
 * draw with a solid color instead.  This value is nullptr by default.
 *
 * Changing this value will cause the sprite batch to flush.  However, a
 * subtexture will not cause a pipeline flush.  This is an important
 * argument for using texture atlases.
 *
 * @param color The active texture for this sprite batch
 */
void SpriteBatch::setTexture(const std::shared_ptr<Texture>& texture) {
    if (texture == _context->texture) {
        return;
    }

    if (_inflight) { record(); }
    if (texture == nullptr) {
        // Active texture is not null
        _context->dirty = _context->dirty | DIRTY_DRAWTYPE;
        _context->texture = nullptr;
        _context->type = _context->type & ~TYPE_TEXTURE;
    } else if (_context->texture == nullptr) {
        // Texture is not null
        _context->dirty = _context->dirty | DIRTY_DRAWTYPE | DIRTY_TEXTURE;
        _context->texture = texture;
        _context->type = _context->type | TYPE_TEXTURE;
    } else {
        // Both must be not nullptr
        if (_context->texture->getBuffer() != texture->getBuffer()) {
            _context->dirty = _context->dirty | DIRTY_TEXTURE;
        }
        _context->texture = texture;
        if (_context->texture->getBindPoint()) {
            _context->texture->setBindPoint(0);
        }
    }
}

/**
 * Returns the active texture of this sprite batch
 *
 * All subsequent shapes and outlines drawn by this sprite batch will use
 * this texture.  If the value is nullptr, all shapes and outlines will be
 * drawn with a solid color instead.  This value is nullptr by default.
 *
 * @return the active texture of this sprite batch
 */
const std::shared_ptr<Texture>& SpriteBatch::getTexture() const {
    return _context->texture;
}

/**
 * Returns the active gradient of this sprite batch
 *
 * Gradients may be used in the place of (and together with) colors.
 * Gradients are like applied textures, and use the gradient coordinates
 * in {@link SpriteVertex2} as their texture coordinates.
 *
 * Setting a gradient value does not guarantee that it will be used.
 * Gradients can be turned on or off by the {@link #useGradient} method.
 * By default, the gradient will not be used (as it is slower than
 * solid colors).
 *
 * All gradients are tinted by the active color. Unless you explicitly
 * want this tinting, you should set the active color to white before
 * drawing with an active gradient.
 *
 * This method returns a copy of the internal gradient. Changes to this
 * object have no effect on the sprite batch.
 *
 * @return The active gradient for this sprite batch
 */
std::shared_ptr<Gradient> SpriteBatch::getGradient() const {
    if (_gradient != nullptr) {
        return Gradient::allocCopy(_gradient);
    }
    return nullptr;
}

/**
 * Sets the active gradient of this sprite batch
 *
 * Gradients may be used in the place of (and together with) colors.
 * Gradients are like applied textures, and use the gradient coordinates
 * in {@link SpriteVertex2} as their texture coordinates.
 *
 * If this value is nullptr, then no gradient is active. In that case,
 * the color vertex attribute will be interpretted as normal (e.g. a
 * traditional color vector).  This value is nullptr by default.
 *
 * All gradients are tinted by the active color. Unless you explicitly
 * want this tinting, you should set the active color to white before
 * drawing with an active gradient.
 *
 * This method acquires a copy of the gradient. Changes to the original
 * gradient after calling this method have no effect.
 *
 * @param gradient   The active gradient for this sprite batch
 */
void SpriteBatch::setGradient(const std::shared_ptr<Gradient>& gradient) {
    if (gradient == _gradient) {
        return;
    }
    
    if (_inflight) { record(); }
    if (gradient == nullptr) {
        // Active gradient is not null
        _context->dirty = _context->dirty | DIRTY_UNIBLOCK | DIRTY_DRAWTYPE;
        _context->type = _context->type & ~TYPE_GRADIENT;
        _gradient = nullptr;
    } else {
        _context->dirty = _context->dirty | DIRTY_UNIBLOCK | DIRTY_DRAWTYPE;
        _context->type = _context->type | TYPE_GRADIENT;
        _gradient = Gradient::allocCopy(gradient);
    }
}

/**
 * Returns the active scissor mask of this sprite batch
 *
 * Scissor masks may be combined with all types of drawing (colors,
 * textures, and gradients).  They are specified in the same coordinate
 * system as {@link getPerspective}.
 *
 * If this value is nullptr, then no scissor mask is active. This value
 * is nullptr by default.
 *
 * This method returns a copy of the internal scissor. Changes to this
 * object have no effect on the sprite batch.
 *
 * @return The active scissor mask for this sprite batch
 */
std::shared_ptr<Scissor> SpriteBatch::getScissor() const {
    if (_scissor != nullptr) {
        return Scissor::alloc(_scissor);
    }
    return nullptr;
}

/**
 * Sets the active scissor mask of this sprite batch
 *
 * Scissor masks may be combined with all types of drawing (colors,
 * textures, and gradients).  They are specified in the same coordinate
 * system as {@link getPerspective}. 
 *
 * If this value is nullptr, then no scissor mask is active. This value
 * is nullptr by default.
 *
 * This method acquires a copy of the scissor. Changes to the original
 * scissor mask after calling this method have no effect.
 *
 * @param scissor   The active scissor mask for this sprite batch
 */
void SpriteBatch::setScissor(const std::shared_ptr<Scissor>& scissor) {
    if (scissor == _scissor) {
        return;
    }
    
    if (_inflight) { record(); }
    if (scissor == nullptr) {
        // Active gradient is not null
        _context->dirty = _context->dirty | DIRTY_UNIBLOCK | DIRTY_DRAWTYPE;
        _context->type = _context->type & ~TYPE_SCISSOR;
        _scissor = nullptr;
    } else {
        _context->dirty = _context->dirty | DIRTY_UNIBLOCK | DIRTY_DRAWTYPE;
        _context->type = _context->type | TYPE_SCISSOR;
        _scissor = Scissor::alloc(scissor);
    }
}

/**
 * Sets the blending functions for the source color
 *
 * The enums are the standard ones supported by OpenGL.  See
 *
 *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
 *
 * This version of the function allows you to specify different
 * blending fuctions for the RGB and alpha components of the source
 * color. This setter does not do any error checking to verify that
 * the enums are valid.
 *
 * By default both values are GL_SRC_ALPHA, as sprite batches do not
 * use premultiplied alpha.
 *
 * @param rgb       The blend function for the source RGB components
 * @param alpha     The blend function for the source alpha component
 */
void SpriteBatch::setSrcBlendFunc(GLenum rgb, GLenum alpha) {
    if (_context->srcRGB != rgb || _context->srcAlpha != alpha) {
        if (_inflight) { record(); }
        _context->srcRGB   = rgb;
        _context->srcAlpha = alpha;
        _context->dirty = _context->dirty | DIRTY_SRC_FUNCTION;
    }
}

/**
 * Returns the source blending function for the RGB components
 *
 * By default this value is GL_SRC_ALPHA, as sprite batches do not
 * use premultiplied alpha. For other options, see
 *
 *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
 *
 * @return the source blending function for the RGB components
 */
GLenum SpriteBatch::getSrcBlendRGB() const {
    return _context->srcRGB;
}

/**
 * Returns the source blending function for the alpha component
 *
 * By default this value is GL_SRC_ALPHA, as sprite batches do not
 * use premultiplied alpha. For other options, see
 *
 *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
 *
 * @return the source blending function for the alpha component
 */
GLenum SpriteBatch::getSrcBlendAlpha() const {
    return _context->srcAlpha;
}

/**
 * Sets the blending functions for the destination color
 *
 * The enums are the standard ones supported by OpenGL.  See
 *
 *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
 *
 * This version of the function allows you to specify different
 * blending fuctions for the RGB and alpha components of the
 * destiniation color. This setter does not do any error checking
 * to verify that the enums are valid.
 *
 * By default both values are GL_ONE_MINUS_SRC_ALPHA, as sprite
 * batches do not use premultiplied alpha.
 *
 * @param rgb       The blend function for the source RGB components
 * @param alpha     The blend function for the source alpha component
 */
void SpriteBatch::setDstBlendFunc(GLenum rgb, GLenum alpha) {
    if (_context->dstRGB != rgb || _context->dstAlpha != alpha) {
        if (_inflight) { record(); }
        _context->dstRGB   = rgb;
        _context->dstAlpha = alpha;
        _context->dirty = _context->dirty | DIRTY_DST_FUNCTION;
    }
}

/**
 * Returns the destination blending function for the RGB components
 *
 * By default this value is GL_ONE_MINUS_SRC_ALPHA, as sprite batches
 * do not use premultiplied alpha. For other options, see
 *
 *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
 *
 * @return the destination blending function for the RGB components
 */
GLenum SpriteBatch::getDstBlendRGB() const {
    return _context->dstRGB;
}

/**
 * Returns the destination blending function for the alpha component
 *
 * By default this value is GL_ONE_MINUS_SRC_ALPHA, as sprite batches
 * do not use premultiplied alpha. For other options, see
 *
 *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
 *
 * @return the destination blending function for the alpha component
 */
GLenum SpriteBatch::getDstBlendAlpha() const {
    return _context->dstAlpha;
}

/**
 * Sets the blending equation for this sprite batch
 *
 * The enum must be a standard ones supported by OpenGL.  See
 *
 *      https://www.opengl.org/sdk/docs/man/html/glBlendEquation.xhtml
 *
 * However, this setter does not do any error checking to verify that
 * the input is valid.  By default, the equation is GL_FUNC_ADD.
 *
 * Changing this value will cause the sprite batch to flush.
 *
 * @param equation  Specifies how source and destination colors are combined
 */
void SpriteBatch::setBlendEquation(GLenum equation) {
    if (_context->blendEq != equation) {
        if (_inflight) { record(); }
        _context->blendEq = equation;
        _context->dirty = _context->dirty | DIRTY_BLENDEQUATION;
    }
}

/**
 * Returns the blending equation for this sprite batch
 *
 * By default this value is GL_FUNC_ADD. For other options, see
 *
 *      https://www.opengl.org/sdk/docs/man/html/glBlendEquation.xhtml
 *
 * @return the blending equation for this sprite batch
 */
GLenum SpriteBatch::getBlendEquation() const {
    return _context->blendEq;
}

/**
 * Returns the current drawing command.
 *
 * The value must be one of GL_TRIANGLES or GL_LINES.  Changing this value
 * during a drawing pass will flush the buffer.
 *
 * @return the current drawing command.
 */
void SpriteBatch::setCommand(GLenum command) {
    if (_context->command != command) {
        if (_inflight) { record(); }
        _context->command = command;
        _context->dirty = _context->dirty | DIRTY_COMMAND;
    }
}

/**
 * Returns the current drawing command.
 *
 * The value must be one of GL_TRIANGLES or GL_LINES.
 *
 * @return the current drawing command.
 */
GLenum SpriteBatch::getCommand() const {
    return _context->command;
}

/**
 * Sets the current depth of this sprite batch.
 *
 * The depth value is appended to all 2d shapes drawn by this sprite
 * batch.If this value is non-zero then depth testing is turned
 * on. However, the exact depth function is up to you and should
 * be set outside of this sprite batch.
 *
 * @param depth The current depth of this sprite batch.
 */
void SpriteBatch::setDepth(float depth) {
    if (_context->zDepth != depth) {
        if (_inflight) { record(); }
        _context->zDepth = depth;
        _context->dirty  = _context->dirty | DIRTY_DEPTHVALUE;
    }
}

/**
 * Returns the current depth of this sprite batch.
 *
 * The depth value is appended to all 2d shapes drawn by this sprite
 * batch.If this value is non-zero then depth testing is turned
 * on. However, the exact depth function is up to you and should
 * be set outside of this sprite batch.
 *
 * @return the current depth of this sprite batch.
 */
float SpriteBatch::getDepth() const {
    return _context->zDepth;
}

/**
 * Sets the blur radius in pixels (0 if there is no blurring).
 *
 * This sprite batch supports a simple Gaussian blur. The blur
 * samples at 5 points along each axis. The values are essentially
 * at the radius, half-radius, and center. Because of the limited
 * sampling, large radii will start to produce a pixellation effect.
 * But it can produce acceptable blur effects with little cost to
 * performance. It is especially ideal for font-blur effects on
 * font atlases.
 *
 * When applying a blur to a {@link GlyphRun}, make sure that the
 * source {@link Font} has {@link Font#setPadding} set to at least
 * the blur radius. Otherwise, the blur will bleed into other glyphs.
 *
 * Setting this value to 0 will disable texture blurring.  This
 * value is 0 by default.
 *
 * @param radius    The blur radius in pixels
 */
void SpriteBatch::setBlur(GLfloat radius) {
    if (_context->blur == radius) {
        return;
    }
    
    if (_inflight) { record(); }
    if (radius == 0.0f) {
        // Active gradient is not null
        _context->dirty = _context->dirty | DIRTY_BLURSTEP | DIRTY_DRAWTYPE;
        _context->type = _context->type & ~TYPE_GAUSSBLUR;
    } else if (_context->blur == 0){
        _context->dirty = _context->dirty | DIRTY_BLURSTEP | DIRTY_DRAWTYPE;
        _context->type = _context->type | TYPE_GAUSSBLUR;
    } else {
        _context->dirty = _context->dirty | DIRTY_BLURSTEP;
    }
    _context->blur = radius;
}

/**
 * Returns the blur radius in pixels (0 if there is no blurring).
 *
 * This sprite batch supports a simple Gaussian blur. The blur
 * samples at 5 points along each axis. The values are essentially
 * at the radius, half-radius, and center. Because of the limited
 * sampling, large radii will start to produce a pixellation effect.
 * But it can produce acceptable blur effects with little cost to
 * performance. It is especially ideal for font-blur effects on
 * font atlases.
 *
 * When applying a blur to a {@link GlyphRun}, make sure that the
 * source {@link Font} has {@link Font#setPadding} set to at least
 * the blur radius. Otherwise, the blur will bleed into other glyphs.
 *
 * Setting this value to 0 will disable texture blurring.  This
 * value is 0 by default.
 *
 * @return the blur radius in pixels (0 if there is no blurring).
 */
GLfloat SpriteBatch::getBlur() const {
    return _context->blur;
}

/**
 * Sets the current stencil effect
 *
 * Stencil effects can be used to restrict the drawing region and
 * are generally used to speed up the processing of non-convex
 * shapes. See {@link StencilEffect} for the list of supported
 * effects, as well as a discussion of how the two halves of the
 * stencil buffer work.
 *
 * This value should be set to {@link StencilEffect#NATIVE} (the
 * default) if you wish to directly manipulate the OpenGL stencil.
 * This is sometimes necessary for more complex effects.
 *
 * @param effect    The current stencil effect
 */
void SpriteBatch::setStencilEffect(StencilEffect effect) {
    if (_context->stencil != effect) {
        if (_inflight) { record(); }
        _context->stencil = effect;
        _context->dirty = _context->dirty | DIRTY_STENCIL_EFFECT;
    }
}

/**
 * Returns the current stencil effect
 *
 * Stencil effects can be used to restrict the drawing region and
 * are generally used to speed up the processing of non-convex
 * shapes. See {@link StencilEffect} for the list of supported
 * effects, as well as a discussion of how the two halves of the
 * stencil buffer work.
 *
 * This value should be set to {@link StencilEffect#NATIVE} (the
 * default) if you wish to directly manipulate the OpenGL stencil.
 * This is sometimes necessary for more complex effects.
 *
 * @return the current stencil effect
 */
StencilEffect SpriteBatch::getStencilEffect() const {
    return _context->stencil;
}

/**
 * Clears the stencil buffer.
 *
 * This method clears both halves of the stencil buffer: both upper
 * and lower. See {@link StencilEffect} for a discussion of how the
 * two halves of the stencil buffer work.
 */
void SpriteBatch::clearStencil() {
    if (_context->cleared != STENCIL_BOTH) {
        if (_inflight) { record(); }
        _context->cleared = STENCIL_BOTH;
        _context->dirty = _context->dirty | DIRTY_STENCIL_CLEAR;
    }
}

/**
 * Clears half of the stencil buffer.
 *
 * This method clears only one of the two halves of the stencil
 * buffer. See {@link StencilEffect} for a discussion of how the
 * two halves of the stencil buffer work.
 *
 * @param lower     Whether to clear the lower stencil buffer
 */
void SpriteBatch::clearHalfStencil(bool lower) {
    GLenum state = lower ? STENCIL_LOWER : STENCIL_UPPER;
    if (_context->cleared != state) {
        if (_inflight) { record(); }
        _context->cleared = _context->cleared | state;
        _context->dirty = _context->dirty | DIRTY_STENCIL_CLEAR;
    }
}

#pragma mark -
#pragma mark Rendering
/**
 * Starts drawing with the current perspective matrix.
 *
 * This call will disable depth buffer writing. It enables blending and
 * texturing. You must call end() to complete drawing.
 *
 * Calling this method will reset the vertex and OpenGL call counters to 0.
 */
void SpriteBatch::begin() {
    glDisable(GL_CULL_FACE);
    glDepthMask(true);
    glEnable(GL_BLEND);

    // DO NOT CLEAR.  This responsibility lies elsewhere
    _shader->bind();
    _vertbuff->bind();
    _unifbuff->bind(false);
    _unifbuff->deactivate();
    _active = true;
    _callTotal = 0;
    _vertTotal = 0;
}

/**
 * Completes the drawing pass for this sprite batch, flushing the buffer.
 *
 * This method enables depth writes and disables blending and texturing. It
 * Must always be called after a call to {@link #begin()}.
 */
void SpriteBatch::end() {
    CUAssertLog(_active,"SpriteBatch is not active");
    flush();
    _context->reset();
    _context->dirty = DIRTY_ALL_VALS;

    // Undo any active stencil effects
    glDisable(GL_STENCIL_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    
    _shader->unbind();
    _active = false;
}


/**
 * Flushes the current mesh without completing the drawing pass.
 *
 * This method is called whenever you change any attribute other than color
 * mid-pass. It prevents the attribute change from retoactively affecting
 * previuosly drawn shapes.
 *
 * If you play to apply any OpenGL functionality not directly supported by
 * this sprite batch (e.g stencils), you MUST call this method first before
 * applying your effects.  In addition, you should call this again before
 * restoring the OpenGL state.
 */
void SpriteBatch::flush() {
    if (_indxSize == 0 || _vertSize == 0) {
        return;
    } else if (_context->first != _indxSize) {
        record();
    }
    
    // Load all the vertex data at once
    _vertbuff->loadVertexData(_vertData, _vertSize);
    _vertbuff->loadIndexData(_indxData, _indxSize);
    _unifbuff->activate();
    _unifbuff->flush();
    
    // Chunk the uniforms
    std::shared_ptr<Texture> previous = _context->texture;
    for(auto it = _history.begin(); it != _history.end(); ++it) {
        Context* next = *it;
        if (next->dirty & DIRTY_BLENDEQUATION) {
            glBlendEquation(next->blendEq);
        }
        if (next->dirty & DIRTY_SRC_FUNCTION || next->dirty & DIRTY_DST_FUNCTION) {
            if (next->srcRGB != next->srcAlpha || next->dstRGB != next->dstAlpha ) {
                glBlendFuncSeparate(next->srcRGB, next->srcAlpha, next->dstRGB, next->dstAlpha);
            } else {
                glBlendFunc(next->srcRGB, next->dstRGB);
            }
        }
        if (next->dirty & DIRTY_DEPTHVALUE) {
            _shader->setUniform1f("uDepth", 0);
        }
        if (next->dirty & DIRTY_DRAWTYPE) {
             _shader->setUniform1i("uType", next->type);
        }
        if (next->dirty & DIRTY_PERSPECTIVE) {
            _shader->setUniformMat4("uPerspective",*(next->perspective.get()));
        }
        if (next->dirty & DIRTY_TEXTURE) {
            previous = next->texture;
            if (previous != nullptr) {
                previous->bind();
            }
        }
        if (next->dirty & DIRTY_UNIBLOCK) {
            _unifbuff->setBlock(next->blockptr);
        }
        if (next->dirty & DIRTY_BLURSTEP) {
            blurTexture(next->texture,next->blur);
        }
        if (next->dirty & DIRTY_STENCIL_CLEAR) {
            clearStencilBuffer(next->cleared);
        }
        if (next->dirty & DIRTY_STENCIL_EFFECT) {
            applyEffect(next->stencil);
        }
        
        GLuint amt = next->last-next->first;
        _vertbuff->draw(next->command, amt, next->first);
        _callTotal++;
    }
    
    _unifbuff->deactivate();
    
    // Increment the counters
    _vertTotal += _indxSize;
    
    _vertSize = _indxSize = 0;
    unwind();
    _context->first = 0;
    _context->last  = 0;
    _context->blockptr = -1;
}


#pragma mark -
#pragma mark Solid Shapes
/**
 * Draws the given rectangle filled with the current color and texture.
 *
 * The texture will fill the entire rectangle with texture coordinate 
 * (0,1) at the bottom left corner identified by rect,origin. To draw only
 * part of a texture, use a subtexture to fill the rectangle with the
 * region [minS,maxS]x[min,maxT]. Alternatively, you can use a {@link Poly2}
 * for more fine-tuned control.
 *
 * If depth testing is on, all vertices will be the current depth.
 *
 * @param rect      The rectangle to draw
 */
void SpriteBatch::fill(const Rect rect) {
    setCommand(GL_TRIANGLES);
    prepare(rect);
}

/**
 * Draws the given rectangle filled with the current color and texture.
 *
 * The texture will fill the entire rectangle with texture coordinate
 * (0,1) at the bottom left corner identified by rect,origin. To draw only
 * part of a texture, use a subtexture to fill the rectangle with the
 * region [minS,maxS]x[min,maxT]. Alternatively, you can use a {@link Poly2}
 * for more fine-tuned control.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param rect      The rectangle to draw
 * @param offset    The rectangle offset
 */
void SpriteBatch::fill(const Rect rect, const Vec2 offset) {
    Affine2 transform;
    transform.translate(offset.x,offset.y);
    setCommand(GL_TRIANGLES);
    prepare(rect,transform);
}

/**
 * Draws the given rectangle filled with the current color and texture.
 *
 * The rectangle will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin, 
 * which is specified relative to the origin of the rectangle (not world 
 * coordinates).  So to spin about the center, the origin should be width/2, 
 * height/2 of the rectangle.
 *
 * The texture will fill the entire rectangle before being transformed. 
 * Texture coordinate (0,1) will at the bottom left corner identified by 
 * rect,origin. To draw only part of a texture, use a subtexture to fill 
 * the rectangle with the region [minS,maxS]x[min,maxT]. Alternatively, you 
 * can use a {@link Poly2} for more fine-tuned control.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param rect      The rectangle to draw
 * @param origin    The rotational offset in the rectangle
 * @param scale     The amount to scale the rectangle
 * @param angle     The amount to rotate the rectangle
 * @param offset    The rectangle offset in world coordinates
 */
void SpriteBatch::fill(const Rect rect, const Vec2 origin, const Vec2 scale,
                       float angle, const Vec2 offset) {
    Affine2 transform;
    transform.translate(-origin.x,-origin.y);
    transform.scale(scale);
    transform.rotate(angle);
    transform.translate(offset);

    setCommand(GL_TRIANGLES);
    prepare(rect,transform);
}

/**
 * Draws the given rectangle filled with the current color and texture.
 *
 * The rectangle will transformed by the given matrix. The transform will 
 * be applied assuming the given origin, which is specified relative to 
 * the origin of the rectangle (not world coordinates).  So to apply the 
 * transform to the center of the rectangle, the origin should be width/2, 
 * height/2 of the rectangle.
 *
 * The texture will fill the entire rectangle with texture coordinate
 * (0,1) at the bottom left corner identified by rect,origin. To draw only
 * part of a texture, use a subtexture to fill the rectangle with the
 * region [minS,maxS]x[min,maxT]. Alternatively, you can use a {@link Poly2}
 * for more fine-tuned control.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param rect      The rectangle to draw
 * @param origin    The rotational offset in the rectangle
 * @param transform The coordinate transform
 */
void SpriteBatch::fill(const Rect rect, const Vec2 origin, const Affine2& transform) {
    Affine2 matrix;
    matrix.translate(-origin.x,-origin.y);
    matrix *= transform;
    setCommand(GL_TRIANGLES);
    prepare(rect,matrix);
}

/**
 * Draws the given polygon filled with the current color and texture.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation 
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A 
 * vertical coordinate has texture coordinate 1-y/texture.height. As a 
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the 
 * polygon coordinates as pixel coordinates in the texture file, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param poly      The polygon to draw
 */
void SpriteBatch::fill(const Poly2& poly) {
    setCommand(GL_TRIANGLES);
    prepare(poly);
}

/**
 * Draws the given polygon filled with the current color and texture.
 *
 * The polygon will be offset by the given position.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the
 * polygon coordinates as pixel coordinates in the texture file, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param poly      The polygon to draw
 * @param offset    The polygon offset
 */
void SpriteBatch::fill(const Poly2& poly, const Vec2 offset) {
    setCommand(GL_TRIANGLES);
    prepare(poly,offset);
}

/**
 * Draws the given polygon filled with the current color and texture.
 *
 * The polygon will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin,
 * which is specified relative to the origin of the polygon (not world 
 * coordinates). Hence this origin is essentially the pixel coordinate 
 * of the texture (see below) to assign as the rotational center.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the
 * polygon coordinates as pixel coordinates in the texture file, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param poly      The polygon to draw
 * @param origin    The image origin in pixel space
 * @param scale     The amount to scale the polygon
 * @param angle     The amount to rotate the polygon
 * @param offset    The polygon offset in world coordinates
 */
void SpriteBatch::fill(const Poly2& poly, const Vec2 origin, const Vec2 scale,
                       float angle, const Vec2 offset) {
    Affine2 transform;
    Affine2::createTranslation(-origin.x,-origin.y,&transform);
    transform.scale(scale);
    transform.rotate(angle);
    transform.translate(offset);
    setCommand(GL_TRIANGLES);
    prepare(poly,transform);
}

/**
 * Draws the given polygon filled with the current color and texture.
 *
 * The polygon will transformed by the given matrix. The transform will
 * be applied assuming the given origin, which is specified relative to the 
 * origin of the polygon (not world coordinates). Hence this origin is 
 * essentially the pixel coordinate of the texture (see below) to 
 * assign as the origin of this transform.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the
 * polygon coordinates as pixel coordinates in the texture file, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param poly      The polygon to draw
 * @param origin    The image origin in pixel space
 * @param transform The coordinate transform
 */
void SpriteBatch::fill(const Poly2& poly, const Vec2 origin, const Affine2& transform) {
    Affine2 matrix;
    matrix.translate(-origin.x,-origin.y);
    matrix *= transform;
    setCommand(GL_TRIANGLES);
    prepare(poly,matrix);
}

#pragma mark -
#pragma mark Outlines
/**
 * Outlines the given rectangle with the current color and texture.
 *
 * The drawing will be a wireframe of a rectangle.  The wireframe will
 * be textured with Texture coordinate (0,1) at the bottom left corner 
 * identified by rect,origin. The remaining edges will correspond to the
 * edges of the texture. To draw only part of a texture, use a subtexture
 * to outline the edges with [minS,maxS]x[min,maxT]. Alternatively, you
 * can use a {@link Poly2} for more fine-tuned control.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param rect      The rectangle to outline
 */
void SpriteBatch::outline(const Rect rect) {
    setCommand(GL_LINES);
    prepare(rect);
}

/**
 * Outlines the given rectangle with the current color and texture.
 *
 * The drawing will be a wireframe of a rectangle.  The wireframe will
 * be textured with Texture coordinate (0,1) at the bottom left corner
 * identified by rect,origin. The remaining edges will correspond to the
 * edges of the texture. To draw only part of a texture, use a subtexture
 * to outline the edges with [minS,maxS]x[min,maxT]. Alternatively, you can
 * use a {@link Poly2} for more fine-tuned control.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param rect      The rectangle to outline
 * @param offset    The polygon offset
 */
void SpriteBatch::outline(const Rect rect, const Vec2 offset) {
    Affine2 transform;
    transform.translate(offset);
    setCommand(GL_LINES);
    prepare(rect,transform);
}

/**
 * Outlines the given rectangle with the current color and texture.
 *
 * The rectangle will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin, 
 * which is specified relative to the origin of the rectangle (not world 
 * coordinates).  So to spin about the center, the origin should be width/2, 
 * height/2 of the rectangle.
 *
 * The drawing will be a wireframe of a rectangle.  The wireframe will
 * be textured with Texture coordinate (0,1) at the bottom left corner
 * identified by rect,origin. The remaining edges will correspond to the
 * edges of the texture. To draw only part of a texture, use a subtexture
 * to outline the edges with [minS,maxS]x[min,maxT]. Alternatively, you can
 * use a {@link Poly2} for more fine-tuned control.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param rect      The rectangle to outline
 * @param origin    The rotational offset in the rectangle
 * @param scale     The amount to scale the rectangle
 * @param angle     The amount to rotate the rectangle
 * @param offset    The rectangle offset in world coordinates
 */
void SpriteBatch::outline(const Rect rect, const Vec2 origin, const Vec2 scale,
                          float angle, const Vec2 offset) {
    Affine2 transform;
    Affine2::createTranslation(-origin.x,-origin.y,&transform);
    transform.scale(scale);
    transform.rotate(angle);
    transform.translate((Vec3)offset);
    setCommand(GL_LINES);
    prepare(rect,transform);
}

/**
 * Outlines the given rectangle with the current color and texture.
 *
 * The rectangle will transformed by the given matrix. The transform will 
 * be applied assuming the given origin, which is specified relative to 
 * the origin of the rectangle (not world coordinates).  So to apply the 
 * transform to the center of the rectangle, the origin should be width/2, 
 * height/2 of the rectangle.
 *
 * The drawing will be a wireframe of a rectangle.  The wireframe will
 * be textured with Texture coordinate (0,1) at the bottom left corner
 * identified by rect,origin. The remaining edges will correspond to the
 * edges of the texture. To draw only part of a texture, use a subtexture
 * to outline the edges with [minS,maxS]x[min,maxT]. Alternatively, you can
 * use a {@link Poly2} for more fine-tuned control.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param rect      The rectangle to outline
 * @param origin    The rotational offset in the rectangle
 * @param transform The coordinate transform
 */
void SpriteBatch::outline(const Rect rect, const Vec2 origin, const Affine2& transform) {
    Affine2 matrix;
    matrix.translate(-origin.x,-origin.y);
    matrix *= transform;
    setCommand(GL_LINES);
    prepare(rect,matrix);
}

/**
 * Outlines the given path with the current color and texture.
 *
 * The drawing will be a wireframe of a path, but the lines are textured.
 * The vertex coordinates will be determined by path vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply outlining the rectangle.
 *
 * One way to think of the path is as a "cookie cutter".  Treat the
 * path coordinates as pixel coordinates in the texture file, and use
 * that to determine how the texture fills the path. This may make the
 * path larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param path      The path to outline
 */
void SpriteBatch::outline(const Path2& path) {
    // This is a violation of spirit of poly, but makes code clean
    Poly2 poly(path.vertices);
    path.getIndices(poly.indices);
    setCommand(GL_LINES);
    prepare(poly);
}

/**
 * Outlines the given path with the current color and texture.
 *
 * The path will be offset by the given position.
 *
 * The drawing will be a wireframe of a path, but the lines are textured.
 * The vertex coordinates will be determined by path vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply outlining the rectangle.
 *
 * One way to think of the path is as a "cookie cutter".  Treat the
 * path coordinates as pixel coordinates in the texture file, and use
 * that to determine how the texture fills the path. This may make the
 * path larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param path      The path to outline
 * @param offset    The path offset
 */
void SpriteBatch::outline(const Path2& path, const Vec2 offset) {
    // This is a violation of spirit of poly, but makes code clean
    Poly2 poly(path.vertices);
    path.getIndices(poly.indices);
    setCommand(GL_LINES);
    prepare(poly,offset);
}

/**
 * Outlines the given path with the current color and texture.
 *
 * The path will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin,
 * which is specified relative to the origin of the path (not world
 * coordinates). Hence this origin is essentially the pixel coordinate
 * of the texture (see below) to assign as the rotational center.
 *
 * The drawing will be a wireframe of a path, but the lines are textured.
 * The vertex coordinates will be determined by path vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply outlining the rectangle.
 *
 * One way to think of the path is as a "cookie cutter".  Treat the
 * path coordinates as pixel coordinates in the texture file, and use
 * that to determine how the texture fills the path. This may make the
 * path larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param path      The path to outline
 * @param origin    The image origin in pixel space
 * @param scale     The amount to scale the path
 * @param angle     The amount to rotate the path
 * @param offset    The path offset in world coordinates
 */
void SpriteBatch::outline(const Path2& path, const Vec2 origin, const Vec2 scale,
                          float angle, const Vec2 offset) {
    // This is a violation of spirit of poly, but makes code clean
    Poly2 poly(path.vertices);
    path.getIndices(poly.indices);
    Affine2 transform;
    Affine2::createTranslation(-origin.x,-origin.y,&transform);
    transform.scale(scale);
    transform.rotate(angle);
    transform.translate(offset);
    setCommand(GL_LINES);
    prepare(poly,transform);
}

/**
 * Outlines the given path with the current color and texture.
 *
 * The path will transformed by the given matrix. The transform will
 * be applied assuming the given origin, which is specified relative
 * to the origin of the path (not world coordinates). Hence this origin
 * is essentially the pixel coordinate of the texture (see below) to
 * assign as the origin of this transform.
 *
 * The drawing will be a wireframe of a path, but the lines are textured.
 * The vertex coordinates will be determined by path vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply outlining the rectangle.
 *
 * One way to think of the path is as a "cookie cutter".  Treat the
 * path coordinates as pixel coordinates in the texture file, and use
 * that to determine how the texture fills the path. This may make the
 * path larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param path      The path to outline
 * @param origin    The image origin in pixel space
 * @param transform The coordinate transform
 */
void SpriteBatch::outline(const Path2& path, const Vec2 origin, const Affine2& transform) {
    // This is a violation of spirit of poly, but makes code clean
    Poly2 poly(path.vertices);
    path.getIndices(poly.indices);
    Affine2 matrix;
    matrix.translate(-origin.x,-origin.y);
    matrix *= transform;
    setCommand(GL_LINES);
    prepare(poly,matrix);
}

// The methods below are to make transition from the intro class easier
#pragma mark -
#pragma mark Convenience Methods
/**
 * Draws the texture (without tint) at the given position
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws a rectangle of the size of the texture, with bottom left
 * corner at the given position.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param texture   The new active texture
 * @param position  The bottom left corner of the texture
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Vec2 position) {
    setTexture(texture);
    setColor(Color4::WHITE);
    fill(Rect(position.x,position.y, (float)texture->getWidth(), (float)texture->getHeight()));
}

/**
 * Draws the tinted texture at the given position
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws a rectangle of the size of the texture, with bottom left
 * corner at the given position.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param texture   The new active texture
 * @param color     The new active color
 * @param position  The bottom left corner of the texture
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Color4 color, const Vec2 position) {
    setTexture(texture);
    setColor(color);
    fill(Rect(position.x,position.y, (float)texture->getWidth(), (float)texture->getHeight()));
}

/**
 * Draws the tinted texture at the given position
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).  
 * It then draws a rectangle of the size of the texture, with bottom left 
 * corner at the given position.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param texture   The new active texture
 * @param color     The new active color
 * @param position  The bottom left corner of the texture
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Rect bounds) {
    setTexture(texture);
    setColor(Color4::WHITE);
    fill(bounds);
}

/**
 * Draws the texture (without tint) inside the given bounds
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws the specified rectangle filled with the texture.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param texture   The new active texture
 * @param bounds    The rectangle to texture
 */
 void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Color4 color, const Rect bounds) {
     setTexture(texture);
     setColor(color);
     fill(bounds);
 }

/**
 * Draws the texture (without tint) transformed by the given parameters
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws a texture-sized rectangle centered at the given origin, and
 * transformed by the given parameters.
 *
 * The rectangle will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin,
 * which is specified in texture pixel coordinates (e.g from the bottom
 * left corner).
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param texture   The new active texture
 * @param origin    The image origin in pixel space
 * @param scale     The amount to scale the texture
 * @param angle     The amount to rotate the texture
 * @param offset    The texture offset in world coordinates
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture,
                       const Vec2 origin, const Vec2 scale, float angle, const Vec2 offset) {
    setTexture(texture);
    setColor(Color4::WHITE);
    fill(Rect(0,0, (float)texture->getWidth(), (float)texture->getHeight()), origin, scale, angle, offset);
}

/**
 * Draws the tinted texture transformed by the given parameters
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws a texture-sized rectangle centered at the given origin, and
 * transformed by the given parameters.
 *
 * The rectangle will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin,
 * which is specified in texture pixel coordinates (e.g from the bottom
 * left corner).
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param texture   The new active texture
 * @param color     The new active color
 * @param origin    The image origin in pixel space
 * @param scale     The amount to scale the texture
 * @param angle     The amount to rotate the texture
 * @param offset    The texture offset in world coordinates
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Color4 color,
                       const Vec2 origin, const Vec2 scale, float angle, const Vec2 offset) {
    setTexture(texture);
    setColor(color);
    fill(Rect(0,0, (float)texture->getWidth(), (float)texture->getHeight()), origin, scale, angle, offset);
}

/**
 * Draws the texture (without tint) transformed by the given parameters
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It fills the specified rectangle with the texture.
 *
 * The rectangle will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin, 
 * which is specified relative to the origin of the rectangle (not world 
 * coordinates).  So to spin about the center, the origin should be width/2, 
 * height/2 of the rectangle.
 *
 * The texture will fill the entire rectangle before being transformed. 
 * Texture coordinate (0,1) will at the bottom left corner identified by 
 * rect,origin. To draw only part of a texture, use a subtexture to fill 
 * the rectangle with the region [minS,maxS]x[min,maxT]. Alternatively, you 
 * can use a {@link Poly2} for more fine-tuned control.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param texture   The new active texture
 * @param bounds    The rectangle to texture
 * @param origin    The image origin in pixel space
 * @param scale     The amount to scale the texture
 * @param angle     The amount to rotate the texture
 * @param offset    The rectangle offset in world coordinates
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Rect bounds,
                       const Vec2 origin, const Vec2 scale, float angle, const Vec2 offset) {
    setTexture(texture);
    setColor(Color4::WHITE);
    fill(bounds, origin, scale, angle, offset);
}

/**
 * Draws the tinted texture transformed by the given parameters
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It fills the specified rectangle with the texture.
 *
 * The rectangle will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin, 
 * which is specified relative to the origin of the rectangle (not world 
 * coordinates).  So to spin about the center, the origin should be width/2, 
 * height/2 of the rectangle.
 *
 * The texture will fill the entire rectangle before being transformed. 
 * Texture coordinate (0,1) will at the bottom left corner identified by 
 * rect,origin. To draw only part of a texture, use a subtexture to fill 
 * the rectangle with the region [minS,maxS]x[min,maxT]. Alternatively,
 * you can use a {@link Poly2} for more fine-tuned control.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param texture   The new active texture
 * @param color     The new active color
 * @param bounds    The rectangle to texture
 * @param origin    The image origin in pixel space
 * @param scale     The amount to scale the texture
 * @param angle     The amount to rotate the texture
 * @param offset    The rectangle offset in world coordinates
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Color4 color, const Rect bounds,
                       const Vec2 origin, const Vec2 scale, float angle, const Vec2 offset) {
    setTexture(texture);
    setColor(color);
    fill(bounds, origin, scale, angle, offset);
}

/**
 * Draws the texture (without tint) transformed by the matrix
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws a texture-sized rectangle centered at the given origin, and
 * transformed by the given matrix.
 *
 * The transform will be applied assuming the given image origin, which is
 * specified in texture pixel coordinates (e.g from the bottom left corner).
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param texture   The new active texture
 * @param origin    The image origin in pixel space
 * @param transform The coordinate transform
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Vec2 origin, const Affine2& transform) {
    setTexture(texture);
    setColor(Color4::WHITE);
    fill(Rect(0,0, (float)texture->getWidth(), (float)texture->getHeight()), origin, transform);
}

/**
 * Draws the tinted texture transformed by the matrix
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws a texture-sized rectangle centered at the given origin, and
 * transformed by the given matrix.
 *
 * The transform will be applied assuming the given image origin, which is
 * specified in texture pixel coordinates (e.g from the bottom left corner).
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param texture   The new active texture
 * @param color     The new active color
 * @param origin    The image origin in pixel space
 * @param transform The coordinate transform
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Color4 color,
                       const Vec2 origin, const Affine2& transform)  {
    setTexture(texture);
    setColor(color);
    fill(Rect(0,0, (float)texture->getWidth(), (float)texture->getHeight()), origin, transform);
}
    
/**
 * Draws the texture (without tint) transformed by the matrix
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It fills the specified rectangle with the texture, transformed by the 
 * given matrix.
 *
 * The transform will be applied assuming the given image origin, which is
 * specified in texture pixel coordinates (e.g from the bottom left corner).
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param texture   The new active texture
 * @param bounds    The rectangle to texture
 * @param origin    The image origin in pixel space
 * @param transform The coordinate transform
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Rect bounds,
                       const Vec2 origin, const Affine2& transform) {
    setTexture(texture);
    setColor(Color4::WHITE);
    fill(bounds, origin, transform);
}

/**
 * Draws the tinted texture transformed by the matrix
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It fills the specified rectangle with the texture, transformed by the 
 * given matrix.
 *
 * The transform will be applied assuming the given image origin, which is
 * specified in texture pixel coordinates (e.g from the bottom left corner).
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param texture   The new active texture
 * @param color     The new active color
 * @param bounds    The rectangle to texture
 * @param origin    The image origin in pixel space
 * @param transform The coordinate transform
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Color4 color,
                       const Rect bounds, const Vec2 origin, const Affine2& transform) {
    setTexture(texture);
    setColor(color);
    fill(bounds, origin, transform);
}

/**
 * Draws the textured polygon (without tint) at the given position
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws the polygon, offset by the given value.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the
 * polygon coordinates as pixel coordinates in the texture filed, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param texture   The new active texture
 * @param poly      The polygon to texture
 * @param offset    The polygon offset
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture,
                       const Poly2& poly, const Vec2 offset) {
    setTexture(texture);
    setColor(Color4::WHITE);
    fill(poly, offset);
}

/**
 * Draws the tinted, textured polygon at the given position
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws the polygon, offset by the given value.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the
 * polygon coordinates as pixel coordinates in the texture filed, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param texture   The new active texture
 * @param color     The new active color
 * @param poly      The polygon to texture
 * @param offset    The polygon offset
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Color4 color,
                       const Poly2& poly, const Vec2 offset) {
    setTexture(texture);
    setColor(color);
    fill(poly, offset);
}

/**
 * Draws the textured polygon (without tint) transformed by the given parameters
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws the polygon, transformed by the given parameters.
 *
 * The polygon will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin,
 * which is specified relative to the origin of the polygon (not world 
 * coordinates). Hence this origin is essentially the pixel coordinate 
 * of the texture (see below) to assign as the rotational center.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the
 * polygon coordinates as pixel coordinates in the texture filed, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param texture   The new active texture
 * @param poly      The polygon to texture
 * @param origin    The image origin in pixel space
 * @param scale     The amount to scale the polygon
 * @param angle     The amount to rotate the polygon
 * @param offset    The polygon offset in world coordinates
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Poly2& poly,
                       const Vec2 origin, const Vec2 scale, float angle,
                       const Vec2 offset) {
    setTexture(texture);
    setColor(Color4::WHITE);
    fill(poly, origin, scale, angle, offset);
}

/**
 * Draws the tinted, textured polygon transformed by the given parameters
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws the polygon, translated by the given parameters.
 *
 * The polygon will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin,
 * which is specified relative to the origin of the polygon (not world 
 * coordinates). Hence this origin is essentially the pixel coordinate 
 * of the texture (see below) to assign as the rotational center.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the
 * polygon coordinates as pixel coordinates in the texture filed, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param texture   The new active texture
 * @param color     The new active color
 * @param poly      The polygon to texture
 * @param origin    The image origin in pixel space
 * @param scale     The amount to scale the polygon
 * @param angle     The amount to rotate the polygon
 * @param offset    The polygon offset in world coordinates
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Color4 color,
                       const Poly2& poly, const Vec2 origin, const Vec2 scale,
                       float angle, const Vec2 offset) {
    setTexture(texture);
    setColor(color);
    fill(poly, origin, scale, angle, offset);
}

/**
 * Draws the textured polygon (without tint) transformed by the given matrix
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws the polygon, translated by the given matrix.
 *
 * The polygon will transformed by the given matrix. The transform will
 * be applied assuming the given origin, which is specified relative to the 
 * origin of the polygon (not world coordinates). Hence this origin is 
 * essentially the pixel coordinate of the texture (see below) to 
 * assign as the origin of this transform.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the
 * polygon coordinates as pixel coordinates in the texture filed, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param texture   The new active texture
 * @param poly      The polygon to texture
 * @param origin    The image origin in pixel space
 * @param transform The coordinate transform
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture,
                       const Poly2& poly, const Vec2 origin, const Affine2& transform) {
    setTexture(texture);
    setColor(Color4::WHITE);
    fill(poly, origin, transform);
}

/**
 * Draws the tinted, textured polygon transformed by the given matrix
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws the polygon, translated by the given matrix.
 *
 * The polygon will transformed by the given matrix. The transform will
 * be applied assuming the given origin, which is specified relative to the 
 * origin of the polygon (not world coordinates). Hence this origin is 
 * essentially the pixel coordinate of the texture (see below) to 
 * assign as the origin of this transform.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the
 * polygon coordinates as pixel coordinates in the texture filed, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param texture   The new active texture
 * @param color     The new active color
 * @param poly      The polygon to texture
 * @param origin    The image origin in pixel space
 * @param transform The coordinate transform
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Color4 color,
                       const Poly2& poly, const Vec2 origin, const Affine2& transform) {
    setTexture(texture);
    setColor(color);
    fill(poly, origin, transform);
}


#pragma mark -
#pragma mark Direct Mesh Drawing
/**
 * Draws the given mesh with the current texture and/or gradient.
 *
 * This method provides more fine tuned control over texture coordinates
 * that the other fill/outline methods.  The texture no longer needs to
 * be drawn uniformly over the shape. The offset will be applied to the
 * vertex positions directly in world space. If depth testing is on, all
 * vertices will be the current depth.
 *
 * The drawing command will be determined by the mesh, and the triangulation
 * or lines determined by the mesh indices. The mesh vertices use their own
 * color values.  However, if tint is true, these values will be tinted
 * (i.e. multiplied) by the current active color.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param mesh      The sprite mesh
 * @param offset    The coordinate offset for the mesh
 * @param tint      Whether to tint with the active color
 */
void SpriteBatch::drawMesh(const Mesh<SpriteVertex2>& mesh, const Vec2 offset, bool tint) {
    if (!mesh.indices.empty()) {
        setCommand(mesh.command);
        Affine2 transform;
        transform.translate(offset);
        prepare(mesh,transform,tint);
    }
}

/**
 * Draws the given mesh with the current texture and/or gradient.
 *
 * This method provides more fine tuned control over texture coordinates
 * that the other fill/outline methods. The texture no longer needs to be
 * drawn uniformly over the shape. The transform will be applied to the
 * vertex positions directly in world space. If depth testing is on, all
 * vertices will be the current depth.
 *
 * The drawing command will be determined by the mesh, and the triangulation
 * or lines determined by the mesh indices. The mesh vertices use their own
 * color values.  However, if tint is true, these values will be tinted
 * (i.e. multiplied) by the current active color.
 *
 * @param mesh      The sprite mesh
 * @param transform The coordinate transform
 * @param tint      Whether to tint with the active color
 */
void SpriteBatch::drawMesh(const Mesh<SpriteVertex2>& mesh, const Affine2& transform, bool tint) {
    if (!mesh.indices.empty()) {
        setCommand(mesh.command);
        prepare(mesh,transform,tint);
    }
}

/**
 * Draws the vertices in a triangle fan with the current texture and/or gradient.
 *
 * This method provides more fine tuned control over texture coordinates
 * that the other fill/outline methods.  The texture no longer needs to
 * be drawn uniformly over the shape. The offset will be applied to the
 * vertex positions directly in world space. If depth testing is on, all
 * vertices will be the current depth.
 *
 * The drawing command will be GL_TRIANGLES, and the triangulation will
 * be a mesh anchored on the first element. This method is ideal for
 * convex polygons.
 *
 * The mesh vertices use their own color values. However, if tint is true,
 * these values will be tinted (i.e. multiplied) by the current active
 * color. If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param vertices  The vertices to draw as a triangle fan
 * @param size      The number of vertices in the fan
 * @param offset    The coordinate offset for the mesh
 * @param tint      Whether to tint with the active color
 *
 * @return the number of vertices added to the drawing buffer.
 */
void SpriteBatch::drawMesh(const SpriteVertex2* vertices, size_t size, const Vec2 offset, bool tint) {
    if (size > 2) {
        setCommand(GL_TRIANGLES);
        Affine2 transform;
        transform.translate(offset);
        prepare(vertices,size,transform,tint);
    }
}

/**
 * Draws the vertices in a triangle fan with the current texture and/or gradient.
 *
 * This method provides more fine tuned control over texture coordinates
 * that the other fill/outline methods.  The texture no longer needs to be
 * drawn uniformly over the shape. The transform will be applied to the
 * vertex positions directly in world space. If depth testing is on, all
 * vertices will be the current depth.
 *
 * The drawing command will be GL_TRIANGLES, and the triangulation will
 * be a mesh anchored on the first element. This method is ideal for
 * convex polygons.
 *
 * The mesh vertices use their own color values. However, if tint is true,
 * these values will be tinted (i.e. multiplied) by the current active
 * color. If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param vertices  The vertices to draw as a triangle fan
 * @param size      The number of vertices in the fan
 * @param mat       The transform to apply to the vertices
 * @param tint      Whether to tint with the active color
 *
 * @return the number of vertices added to the drawing buffer.
 */
void SpriteBatch::drawMesh(const SpriteVertex2* vertices, size_t size, const Affine2& transform, bool tint) {
    if (size > 2) {
        setCommand(GL_TRIANGLES);
        prepare(vertices,size,transform,tint);
    }
}

#pragma mark -
#pragma mark Text Drawing
/**
 * Draws the text with the specified font at the given position.
 *
 * The position specifies the location of the left edge of the baseline of
 * the rendered text. The text will be displayed on only one line. For more
 * fine tuned control of text, you should use a {@link TextLayout}.
 *
 * By default, all text is rendered with white letters. However, this can
 * be tinted by the current sprite batch color to produce any color letters
 * required.
 *
 * If depth testing is on, the font glyphs will use the current sprite
 * batch depth.
 *
 * @param text      The text to display
 * @param font      The font to render the text
 * @param position  The left edge of the text baseline
 */
void SpriteBatch::drawText(const std::string text, const std::shared_ptr<Font>& font, const Vec2 position) {
    std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> runs;
    font->getGlyphs(runs, text, position);
    for(auto it = runs.begin(); it != runs.end(); ++it) {
        setTexture(it->second->texture);
        drawMesh(it->second->mesh,Vec2::ZERO);
    }
}

/**
 * Draws the text with the specified font and transform
 *
 * The offset is measured from the left edge of the font baseline to identify
 * the origin of the rendered text. This origin is used when applying the
 * transform to the rendered text.
 *
 * By default, all text is rendered with white letters. However, this can
 * be tinted by the current sprite batch color to produce any color letters
 * required.
 *
 * If depth testing is on, the font glyphs will use the current sprite
 * batch depth.
 *
 * @param text      The text to display
 * @param font      The font to render the text
 * @param origin    The rotational origin relative to the baseline
 * @param transform The coordinate transform
 */
void SpriteBatch::drawText(const std::string text, const std::shared_ptr<Font>& font, const Vec2 origin, const Affine2& transform) {
    std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> runs;
    font->getGlyphs(runs, text, -origin);
    for(auto it = runs.begin(); it != runs.end(); ++it) {
        setTexture(it->second->texture);
        drawMesh(it->second->mesh,transform);
    }
}

/**
 * Draws the text layout at the specified position
 *
 * The position specifies the location of the text layout origin. See the
 * specification of {@link TextLayout} for more information.
 *
 * By default, all text is rendered with white letters. However, this can
 * be tinted by the current sprite batch color to produce any color letters
 * required.
 *
 * If depth testing is on, the font glyphs will use the current sprite
 * batch depth.
 *
 * @param text      The text to display
 * @param font      The font to render the text
 * @param origin    The rotational origin relative to the baseline
 * @param transform The coordinate transform
 */
void SpriteBatch::drawText(const std::shared_ptr<TextLayout>& text, const Vec2 position) {
    std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> runs;
    text->getGlyphs(runs);
    for(auto it = runs.begin(); it != runs.end(); ++it) {
        setTexture(it->second->texture);
        drawMesh(it->second->mesh,position);
    }
}

/**
 * Draws the text layout with the given coordinate transform
 *
 * The transform is applied to the coordinate space of the {@link TextLayout}.
 *
 * By default, all text is rendered with white letters. However, this can
 * be tinted by the current sprite batch color to produce any color letters
 * required.
 *
 * If depth testing is on, the font glyphs will use the current sprite
 * batch depth.
 *
 * @param text      The text to display
 * @param font      The font to render the text
 * @param origin    The rotational origin relative to the baseline
 * @param transform The coordinate transform
 */
void SpriteBatch::drawText(const std::shared_ptr<TextLayout>& text, const Affine2& transform) {
    std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> runs;
    text->getGlyphs(runs);
    for(auto it = runs.begin(); it != runs.end(); ++it) {
        setTexture(it->second->texture);
        drawMesh(it->second->mesh,transform);
    }
}

#pragma mark -
#pragma mark Internal Helpers
/**
 * Records the current set of uniforms, freezing them.
 *
 * This method must be called whenever {@link prepare} is called for
 * a new set of uniforms.  It ensures that the vertices batched so far
 * will use the correct set of uniforms.
 */
void SpriteBatch::record() {
    Context* next = new Context(_context);
    _context->last = _indxSize;
    next->first = _indxSize;
    _history.push_back(_context);
    _context = next;
    _inflight = false;
}

/**
 * Deletes the recorded uniforms.
 *
 * This method is called upon flushing or cleanup.
 */
void SpriteBatch::unwind() {
    for(auto it = _history.begin(); it != _history.end(); ++it) {
        delete *it;
    }
    _history.clear();
}

/**
 * Sets the active uniform block to agree with the gradient and stroke.
 *
 * This method is called upon vertex preparation.
 *
 * @param context   The current uniform context
 */
void SpriteBatch::setUniformBlock(Context* context) {
    if (!(_context->dirty & DIRTY_UNIBLOCK)) {
        return;
    }
    if (_context->blockptr+1 >= _unifbuff->getBlockCount()) {
        flush();
    }
    float data[40];
    if (_scissor != nullptr) {
        _scissor->getData(data);
    } else {
        std::memset(data,0,16*sizeof(float));
    }
    if (_gradient != nullptr) {
        _gradient->getData(data+16);
    } else {
        std::memset(data+16,0,24*sizeof(float));
    }
    _context->blockptr++;
    _unifbuff->setUniformfv(_context->blockptr,0,40,data);
}

/**
 * Updates the shader with the current blur offsets
 *
 * Blur offsets depend upon the texture size. This method converts the
 * blur step into an offset in texture coordinates. It supports
 * non-square textures.
 *
 * If there is no active texture, the blur offset will be 0.
 *
 * @param texture   The texture to blur
 * @param step      The blur step in pixels
 */
void SpriteBatch::blurTexture(const std::shared_ptr<Texture>& texture, GLfloat step) {
    if (texture == nullptr) {
        _shader->setUniform2f("uBlur", 0, 0);
        return;
    }
    Size size = texture->getSize();
    size.width  = step/size.width;
    size.height = step/size.height;
    _shader->setUniform2f("uBlur",size.width,size.height);
}

/**
 * Clears the stencil buffer specified
 *
 * @param buffer    The stencil buffer (lower, upper, both)
 */
void SpriteBatch::clearStencilBuffer(GLenum buffer) {
    switch (buffer) {
        case STENCIL_NONE:
            return;
        case STENCIL_LOWER:
            glStencilMask(0xf0);
            glClear(GL_STENCIL_BUFFER_BIT);
            glStencilMask(0xff);
            return;
        case STENCIL_UPPER:
            glStencilMask(0x0f);
            glClear(GL_STENCIL_BUFFER_BIT);
            glStencilMask(0xff);
            return;
        case STENCIL_BOTH:
            glStencilMask(0xff);
            glClear(GL_STENCIL_BUFFER_BIT);
            return;
    }
}


/**
 * Configures the OpenGL settings to apply the given effect.
 *
 * @param effect    The stencil effect
 */
void SpriteBatch::applyEffect(StencilEffect effect) {
    //CULog("Applying %d",effect);
    switch(effect) {
        case NATIVE:
            // Nothing more to do
            break;
        case NONE:
            glDisable(GL_STENCIL_TEST);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case CLIP:
        case CLIP_JOIN:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xff);
            glStencilFunc(GL_NOTEQUAL, 0x00, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case MASK:
        case MASK_JOIN:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xff);
            glStencilFunc(GL_EQUAL, 0x00, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case FILL:
        case FILL_JOIN:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xff);
            glStencilFunc(GL_NOTEQUAL, 0x00, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case WIPE:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xff);
            glStencilFunc(GL_ALWAYS, 0x00, 0xff);
            glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case STAMP:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xff);
            glStencilFunc(GL_ALWAYS, 0x00, 0xff);
            glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
            glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case CARVE:
        case CARVE_NONE:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xf0);
            glStencilFunc(GL_EQUAL, 0x00, 0xf0);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case CLAMP:
        case CLAMP_NONE:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xf0);
            glStencilFunc(GL_EQUAL, 0x00, 0xf0);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case NONE_CLIP:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0x0f);
            glStencilFunc(GL_NOTEQUAL, 0x00, 0x0f);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case NONE_MASK:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0x0f);
            glStencilFunc(GL_EQUAL, 0x00, 0x0f);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case NONE_FILL:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0x0f);
            glStencilFunc(GL_NOTEQUAL, 0x00, 0x0f);
            glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case NONE_WIPE:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0x0f);
            glStencilFunc(GL_ALWAYS, 0x00, 0x0f);
            glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case NONE_STAMP:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0x0f);
            glStencilFunc(GL_ALWAYS, 0x00, 0x0f);
            glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INVERT);
            glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case NONE_CARVE:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0x0f);
            glStencilFunc(GL_EQUAL, 0x00, 0x0f);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case NONE_CLAMP:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0x0f);
            glStencilFunc(GL_EQUAL, 0x00, 0x0f);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case CLIP_NONE:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xff);
            glStencilFunc(GL_NOTEQUAL, 0x00, 0xf0);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case CLIP_MEET:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xff);
            glStencilFunc(GL_EQUAL, 0xff, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case CLIP_MASK:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xff);
            glStencilFunc(GL_EQUAL, 0xf0, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case CLIP_FILL:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0x0f);
            glStencilFunc(GL_NOTEQUAL, 0x00, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case CLIP_WIPE:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0x0f);
            glStencilFunc(GL_NOTEQUAL, 0x00, 0xf0);
            glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case CLIP_STAMP:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0x0f);
            glStencilFunc(GL_NOTEQUAL, 0x00, 0xf0);
            glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INVERT);
            glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case CLIP_CARVE:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0x0f);
            glStencilFunc(GL_EQUAL, 0xf0, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case CLIP_CLAMP:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0x0f);
            glStencilFunc(GL_EQUAL, 0xf0, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case MASK_NONE:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xff);
            glStencilFunc(GL_EQUAL, 0x00, 0xf0);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case MASK_MEET:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xff);
            glStencilFunc(GL_NOTEQUAL, 0xff, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case MASK_CLIP:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xff);
            glStencilFunc(GL_EQUAL, 0x0f, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case MASK_FILL:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0x0f);
            glStencilFunc(GL_EQUAL, 0x0f, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case MASK_WIPE:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0x0f);
            glStencilFunc(GL_EQUAL, 0x00, 0xf0);
            glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case MASK_STAMP:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0x0f);
            glStencilFunc(GL_EQUAL, 0x00, 0xf0);
            glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INVERT);
            glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case MASK_CARVE:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0x0f);
            glStencilFunc(GL_EQUAL, 0x0, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case MASK_CLAMP:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0x0f);
            glStencilFunc(GL_EQUAL, 0x00, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case FILL_NONE:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xf0);
            glStencilFunc(GL_NOTEQUAL, 0x00, 0xf0);
            glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case FILL_MEET:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xf0);
            glStencilFunc(GL_EQUAL, 0xff, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case FILL_CLIP:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xf0);
            glStencilFunc(GL_EQUAL, 0xff, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case FILL_MASK:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xf0);
            glStencilFunc(GL_EQUAL, 0xf0, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case WIPE_NONE:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xf0);
            glStencilFunc(GL_ALWAYS, 0x00, 0xf0);
            glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case WIPE_CLIP:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xf0);
            glStencilFunc(GL_NOTEQUAL, 0x00, 0x0f);
            glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case WIPE_MASK:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xf0);
            glStencilFunc(GL_EQUAL, 0x00, 0x0f);
            glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case STAMP_NONE:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xf0);
            glStencilFunc(GL_ALWAYS, 0x00, 0x0f);
            glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INVERT);
            glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case STAMP_CLIP:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xf0);
            glStencilFunc(GL_NOTEQUAL, 0x00, 0x0f);
            glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INVERT);
            glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case STAMP_MASK:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xf0);
            glStencilFunc(GL_EQUAL, 0x00, 0x0f);
            glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INVERT);
            glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case STAMP_BOTH:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xff);
            glStencilFunc(GL_ALWAYS, 0x00, 0xff);
            glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INVERT);
            glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case CARVE_CLIP:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xf0);
            glStencilFunc(GL_NOTEQUAL, 0x0f, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case CARVE_MASK:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xf0);
            glStencilFunc(GL_EQUAL, 0x0f, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case CARVE_BOTH:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xff);
            glStencilFunc(GL_EQUAL, 0x00, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case CLAMP_CLIP:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xf0);
            glStencilFunc(GL_EQUAL, 0x0f, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case CLAMP_MASK:
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xf0);
            glStencilFunc(GL_EQUAL, 0x00, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
    }
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method adds the given rectangle to the vertex buffer, but does not
 * draw it yet.  You must call {@link #flush} or {@link #end} to draw the 
 * rectangle. This method will automatically flush if the maximum number
 * of vertices is reached.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param rect  The rectangle to add to the buffer
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::prepare(const Rect rect) {
    if (_vertSize+4 >= _vertMax ||  _indxSize+8 >= _indxMax) {
        flush();
    }
    
    Texture* texture = _context->texture.get();
    float tsmax, tsmin;
    float ttmax, ttmin;
    
    if (texture != nullptr) {
        tsmax = texture->getMaxS();
        tsmin = texture->getMinS();
        ttmax = texture->getMaxT();
        ttmin = texture->getMinT();
    } else {
        tsmax = 1.0f; tsmin = 0.0f;
        ttmax = 1.0f; ttmin = 0.0f;
    }
    
    setUniformBlock(_context);
    Poly2 poly;
    makeRect(poly, rect, _context->command == GL_TRIANGLES);
    unsigned int vstart = _vertSize;
    int ii = 0;
    GLuint clr = _color.getPacked();
    for(auto it = poly.vertices.begin(); it != poly.vertices.end(); ++it) {
        Vec2 point = *it;
        _vertData[vstart+ii].position = point;
        point.x = (point.x-rect.origin.x)/rect.size.width;
        point.y = 1-(point.y-rect.origin.y)/rect.size.height;
        _vertData[vstart+ii].texcoord.x = point.x*tsmax+(1-point.x)*tsmin;
        _vertData[vstart+ii].texcoord.y = point.y*ttmax+(1-point.y)*ttmin;
        _vertData[vstart+ii].gradcoord.x = point.x+(1-point.x);
        _vertData[vstart+ii].gradcoord.y = point.y+(1-point.y);
        _vertData[vstart+ii].color = clr;
        
        ii++;
    }
    
    int jj = 0;
    unsigned int istart = _indxSize;
    for(auto it = poly.indices.begin(); it != poly.indices.end(); ++it) {
        _indxData[istart+jj] = vstart+(*it);
        jj++;
    }
    
    _vertSize += ii;
    _indxSize += jj;
    _inflight = true;
    return ii;
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method adds the given rectangle to the vertex buffer, but does not
 * draw it yet.  You must call {@link #flush} or {@link #end} to draw the 
 * rectangle. This method will automatically flush if the maximum number
 * of vertices is reached.
 *
 * All vertices will be uniformly transformed by the transform matrix.
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param rect  The rectangle to add to the buffer
 * @param mat  	The transform to apply to the vertices
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::prepare(const Rect rect, const Affine2& mat) {
    if (_vertSize+4 > _vertMax ||  _indxSize+8 > _indxMax) {
        flush();
    }

    Texture* texture = _context->texture.get();
    float tsmax, tsmin;
    float ttmax, ttmin;

    if (texture != nullptr) {
        tsmax = texture->getMaxS();
        tsmin = texture->getMinS();
        ttmax = texture->getMaxT();
        ttmin = texture->getMinT();
    } else {
        tsmax = 1.0f; tsmin = 0.0f;
        ttmax = 1.0f; ttmin = 0.0f;
    }
    
    setUniformBlock(_context);
    Poly2 poly;
    makeRect(poly, rect, _context->command == GL_TRIANGLES);

    unsigned int vstart = _vertSize;
    int ii = 0;
    GLuint clr = _color.getPacked();
    for(auto it = poly.vertices.begin(); it != poly.vertices.end(); ++it) {
        Vec2 point = *it;
        _vertData[vstart+ii].position = point*mat;
        point.x = (point.x-rect.origin.x)/rect.size.width;
        point.y = 1-(point.y-rect.origin.y)/rect.size.height;
        _vertData[vstart+ii].texcoord.x = point.x*tsmax+(1-point.x)*tsmin;
        _vertData[vstart+ii].texcoord.y = point.y*ttmax+(1-point.y)*ttmin;
        _vertData[vstart+ii].gradcoord.x = point.x+(1-point.x);
        _vertData[vstart+ii].gradcoord.y = point.y+(1-point.y);
        _vertData[vstart+ii].color = clr;

        ii++;
    }
    
    int jj = 0;
    unsigned int istart = _indxSize;
    for(auto it = poly.indices.begin(); it != poly.indices.end(); ++it) {
        _indxData[istart+jj] = vstart+(*it);
        jj++;
    }
    
    _vertSize += ii;
    _indxSize += jj;
    _inflight = true;
    return ii;
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method adds the given polygon to the vertex buffer, but does not
 * draw it yet.  You must call {@link #flush} or {@link #end} to draw the 
 * polygon. This method will automatically flush if the maximum number
 * of vertices is reached.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param poly  The polygon to add to the buffer
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::prepare(const Poly2& poly) {
    CUAssertLog(_context->command == GL_TRIANGLES ?
                 poly.indices.size() % 3 == 0 :
                 poly.indices.size() % 2 == 0,
                "Polynomial has the wrong number of indices: %d", (int)poly.indices.size());
    if (poly.vertices.size() >= _vertMax || poly.indices.size()  >= _indxMax) {
        return chunkify(poly,Mat4::IDENTITY);
    } else if (_vertSize+poly.vertices.size() > _vertMax ||
               _indxSize+poly.indices.size()  > _indxMax) {
        flush();
    }

    Texture* texture = _context->texture.get();
    float twidth, theight;
    float tsmax, tsmin;
    float ttmax, ttmin;
    
    if (texture != nullptr) {
        twidth  = texture->getWidth();
        theight = texture->getHeight();
        tsmax = texture->getMaxS();
        tsmin = texture->getMinS();
        ttmax = texture->getMaxT();
        ttmin = texture->getMinT();
    } else {
        twidth  = poly.getBounds().size.width;
        theight = poly.getBounds().size.height;
        tsmax = 1.0f; tsmin = 0.0f;
        ttmax = 1.0f; ttmin = 0.0f;
    }
    
    setUniformBlock(_context);
    unsigned int vstart = _vertSize;
    int ii = 0;
    GLuint clr = _color.getPacked();
    for(auto it = poly.vertices.begin(); it != poly.vertices.end(); ++it) {
        Vec2 point = *it;
        _vertData[vstart+ii].position = point;
        point.x /= twidth;
        point.y = 1-point.y/theight;
        _vertData[vstart+ii].texcoord.x = point.x*tsmax+(1-point.x)*tsmin;
        _vertData[vstart+ii].texcoord.y = point.y*ttmax+(1-point.y)*ttmin;
        _vertData[vstart+ii].gradcoord.x = point.x+(1-point.x);
        _vertData[vstart+ii].gradcoord.y = point.y+(1-point.y);
        _vertData[vstart+ii].color = clr;

        ii++;
    }
    
    int jj = 0;
    unsigned int istart = _indxSize;
    for(auto it = poly.indices.begin(); it != poly.indices.end(); ++it) {
        _indxData[istart+jj] = vstart+(*it);
        jj++;
    }
    
    _vertSize += ii;
    _indxSize += jj;
    _inflight = true;
    return ii;
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method adds the given polygon to the vertex buffer, but does not
 * draw it yet.  You must call {@link #flush} or {@link #end} to draw the 
 * polygon. This method will automatically flush if the maximum number
 * of vertices is reached.
 *
 * All vertices will be uniformly offset by the given vector. If depth
 * testing is on, all vertices will use the current sprite batch depth.

 *
 * @param poly  The polygon to add to the buffer
 * @param off  	The offset to apply to the vertices
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::prepare(const Poly2& poly, const Vec2 off) {
    CUAssertLog(_context->command == GL_TRIANGLES ?
                 poly.indices.size() % 3 == 0 :
                 poly.indices.size() % 2 == 0,
                "Polynomial has the wrong number of indices: %d", (int)poly.indices.size());
    if (poly.vertices.size() >= _vertMax || poly.indices.size()  >= _indxMax) {
        Mat4 matrix;
        matrix.translate((Vec3)off);
        return chunkify(poly,matrix);
    } else if (_vertSize+poly.vertices.size() > _vertMax ||
               _indxSize+poly.indices.size()  > _indxMax) {
        flush();
    }

    Texture* texture = _context->texture.get();
    float twidth, theight;
    float tsmax, tsmin;
    float ttmax, ttmin;
    
    if (texture != nullptr) {
        twidth  = texture->getWidth();
        theight = texture->getHeight();
        tsmax = texture->getMaxS();
        tsmin = texture->getMinS();
        ttmax = texture->getMaxT();
        ttmin = texture->getMinT();
    } else {
        twidth  = poly.getBounds().size.width;
        theight = poly.getBounds().size.height;
        tsmax = 1.0f; tsmin = 0.0f;
        ttmax = 1.0f; ttmin = 0.0f;
    }
    
    setUniformBlock(_context);
    unsigned int vstart = _vertSize;
    int ii = 0;
    GLuint clr = _color.getPacked();
    for(auto it = poly.vertices.begin(); it != poly.vertices.end(); ++it) {
        Vec2 point = *it;
        _vertData[vstart+ii].position = (*it)+off;
        point.x /= twidth;
        point.y = 1-point.y/theight;
        _vertData[vstart+ii].texcoord.x = point.x*tsmax+(1-point.x)*tsmin;
        _vertData[vstart+ii].texcoord.y = point.y*ttmax+(1-point.y)*ttmin;
        _vertData[vstart+ii].gradcoord.x = point.x+(1-point.x);
        _vertData[vstart+ii].gradcoord.y = point.y+(1-point.y);
        _vertData[vstart+ii].color = clr;

        ii++;
    }
    
    int jj = 0;
    unsigned int istart = _indxSize;
    for(auto it = poly.indices.begin(); it != poly.indices.end(); ++it) {
        _indxData[istart+jj] = vstart+(*it);
        jj++;
    }
    
    _vertSize += ii;
    _indxSize += jj;
    _inflight = true;
    return ii;
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method adds the given polygon to the vertex buffer, but does not
 * draw it yet.  You must call {@link #flush} or {@link #end} to draw the 
 * polygon. This method will automatically flush if the maximum number
 * of vertices is reached.
 *
 * All vertices will be uniformly transformed by the transform matrix.
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param poly  The polygon to add to the buffer
 * @param mat  	The transform to apply to the vertices
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::prepare(const Poly2& poly, const Affine2& mat) {
    CUAssertLog(_context->command == GL_TRIANGLES ?
                 poly.indices.size() % 3 == 0 :
                 poly.indices.size() % 2 == 0,
                "Polynomial has the wrong number of indices: %d", (int)poly.indices.size());
    if (poly.vertices.size() >= _vertMax || poly.indices.size()  >= _indxMax) {
        return chunkify(poly,mat);
    } else if (_vertSize+poly.vertices.size() > _vertMax ||
               _indxSize+poly.indices.size()  > _indxMax) {
        flush();
    }

    Texture* texture = _context->texture.get();
    float twidth, theight;
    float tsmax, tsmin;
    float ttmax, ttmin;
    
    if (texture != nullptr) {
        twidth  = texture->getWidth();
        theight = texture->getHeight();
        tsmax = texture->getMaxS();
        tsmin = texture->getMinS();
        ttmax = texture->getMaxT();
        ttmin = texture->getMinT();
    } else {
        twidth  = poly.getBounds().size.width;
        theight = poly.getBounds().size.height;
        tsmax = 1.0f; tsmin = 0.0f;
        ttmax = 1.0f; ttmin = 0.0f;
    }
    
    setUniformBlock(_context);
    unsigned int vstart = _vertSize;
    int ii = 0;
    GLuint clr = _color.getPacked();
    for(auto it = poly.vertices.begin(); it != poly.vertices.end(); ++it) {
        Vec2 point = *it;
        _vertData[vstart+ii].position = point*mat;
        point.x /= twidth;
        point.y = 1-point.y/theight;
        _vertData[vstart+ii].texcoord.x = point.x*tsmax+(1-point.x)*tsmin;
        _vertData[vstart+ii].texcoord.y = point.y*ttmax+(1-point.y)*ttmin;
        _vertData[vstart+ii].gradcoord.x = point.x+(1-point.x);
        _vertData[vstart+ii].gradcoord.y = point.y+(1-point.y);
        _vertData[vstart+ii].color = clr;
        ii++;
    }
    
    int jj = 0;
    unsigned int istart = _indxSize;
    for(auto it = poly.indices.begin(); it != poly.indices.end(); ++it) {
        _indxData[istart+jj] = vstart+(*it);
        jj++;
    }
    
    _vertSize += ii;
    _indxSize += jj;
    _inflight = true;
    return ii;
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method is an alternate version of {@link #prepare} for the same
 * arguments.  It runs slower (e.g. the compiler cannot easily optimize
 * the loops) but it is guaranteed to work on any size polygon.  This
 * is important for avoiding memory corruption.
 *
 * All vertices will be uniformly transformed by the transform matrix.
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param poly  The polygon to add to the buffer
 * @param mat   The transform to apply to the vertices
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::chunkify(const Poly2& poly, const Affine2& mat) {
    Texture* texture = _context->texture.get();
    std::unordered_map<Uint32, Uint32> offsets;
    const std::vector<cugl::Vec2>* vertices = &(poly.vertices);
    const std::vector<Uint32>* indices = &(poly.indices);
    
    setUniformBlock(_context);
    int chunksize = _context->command == GL_TRIANGLES ? 3 : 2;
    unsigned int start = _vertSize;

    float twidth, theight;
    float tsmax, tsmin;
    float ttmax, ttmin;
    
    if (texture != nullptr) {
        twidth  = texture->getWidth();
        theight = texture->getHeight();
        tsmax = texture->getMaxS();
        tsmin = texture->getMinS();
        ttmax = texture->getMaxT();
        ttmin = texture->getMinT();
    } else {
        twidth  = poly.getBounds().size.width;
        theight = poly.getBounds().size.height;
        tsmax = 1.0f; tsmin = 0.0f;
        ttmax = 1.0f; ttmin = 0.0f;
    }

    GLuint clr = _color.getPacked();
    for(int ii = 0;  ii < indices->size(); ii += chunksize) {
        if (_indxSize+chunksize >= _indxMax || _vertSize+chunksize >= _vertMax) {
            flush();
            offsets.clear();
        }
        
        for(int jj = 0; jj < chunksize; jj++) {
            auto search = offsets.find(indices->at(ii+jj));
            if (search != offsets.end()) {
                _indxData[_indxSize] = search->second;
            } else {
                Vec2 point = vertices->at(indices->at(ii+jj));
                _indxData[_indxSize] = _vertSize;
                _vertData[_vertSize].position = point*mat;
                
                point.x /= twidth;
                point.y = 1-point.y/theight;
                _vertData[_vertSize].texcoord.x = point.x*tsmax+(1-point.x)*tsmin;
                _vertData[_vertSize].texcoord.y = point.y*ttmax+(1-point.y)*ttmin;
                _vertData[_vertSize].gradcoord.x = point.x+(1-point.x);
                _vertData[_vertSize].gradcoord.y = point.y+(1-point.y);
                _vertData[_vertSize].color = clr;
                _vertSize++;
            }
            _indxSize++;
        }
    }

    _inflight = true;
    return (unsigned int)vertices->size()+start;
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method adds the given mesh (both vertices and indices) to the
 * vertex buffer, but does not draw it.  You must call {@link #flush}
 * or {@link #end} to draw the complete mesh. This method will automatically
 * flush if the maximum number of vertices (or uniform blocks) is reached.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param mesh  The mesh to add to the buffer
 * @param mat   The transform to apply to the vertices
 #4    0x0000000104132496 in cugl::SpriteBatch::prepare(cugl::Mesh<cugl::SpriteVertex2> const&, cugl::Affine2 const&, bool) at /Users/wmwhite/Developer/CUGL/src/render/CUSpriteBatch.cpp:3487
 * @param tint  Whether to tint with the active color
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::prepare(const Mesh<SpriteVertex2>& mesh, const Affine2& mat, bool tint) {
    CUAssertLog(mesh.isSliceable(), "Sprite batches only support sliceable meshes");
    if (mesh.vertices.size() >= _vertMax || mesh.indices.size() >= _indxMax) {
        return chunkify(mesh, mat, tint);
    } else if(_vertSize+mesh.vertices.size() > _vertMax || _indxSize+mesh.indices.size() > _indxMax) {
        flush();
    }
    
    setUniformBlock(_context);
    int ii = 0;
    tint = tint && _color != Color4::WHITE;
    for(auto it = mesh.vertices.begin(); it != mesh.vertices.end(); ++it) {
        _vertData[_vertSize+ii] = *it;
        _vertData[_vertSize+ii].position = it->position*mat;
        if (tint) {
            Uint32 c = marshall(_vertData[_vertSize+ii].color);
            Uint32 r = round(_color.r*((c >> 24)/255.0f));
            Uint32 g = round(_color.g*(((c >> 16) & 0xff)/255.0f));
            Uint32 b = round(_color.b*(((c >> 8) & 0xff)/255.0f));
            Uint32 a = round(_color.a*((c & 0xff)/255.0f));
            _vertData[_vertSize+ii].color = marshall(r << 24 | g << 16 | b << 8 | a);
        }
        ii++;
    }
    
    int jj = 0;
    for(auto it = mesh.indices.begin(); it != mesh.indices.end(); ++it) {
        _indxData[_indxSize+jj] = _vertSize+(*it);
        jj++;
    }
    
    _vertSize += ii;
    _indxSize += jj;
    _inflight = true;
    return ii;
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method is an alternate version of {@link #prepare} for the same
 * arguments.  It runs slower (e.g. the compiler cannot easily optimize
 * the loops) but it is guaranteed to work on any size mesh. This is
 * important for avoiding memory corruption.
 *
 * @param mesh  The mesh to add to the buffer
 * @param mat   The transform to apply to the vertices
 * @param tint  Whether to tint with the active color
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::chunkify(const Mesh<SpriteVertex2>& mesh, const Affine2& mat, bool tint) {
    std::unordered_map<Uint32, Uint32> offsets;
    
    setUniformBlock(_context);
    int chunksize = _context->command == GL_TRIANGLES ? 3 : 2;
    unsigned int start = _vertSize;
    
    for(int ii = 0;  ii < mesh.indices.size(); ii += chunksize) {
        if (_indxSize+chunksize >= _indxMax || _vertSize+chunksize >= _vertMax) {
            flush();
            offsets.clear();
        }
        
        for(int jj = 0; jj < chunksize; jj++) {
            auto search = offsets.find(mesh.indices[ii+jj]);
            if (search != offsets.end()) {
                _indxData[_indxSize] = search->second;
            } else {
                _indxData[_indxSize] = _vertSize;
                _vertData[_vertSize] = mesh.vertices[ii+jj];
                _vertData[_vertSize].position *= mat;
                if (tint) {
                    Color4 shade(_vertData[_vertSize].color);
                    shade *= _color;
                    _vertData[_vertSize].color = shade.getPacked();
                }
                _vertSize++;
            }
            _indxSize++;
        }
    }

    _inflight = true;
    return (unsigned int)(mesh.vertices.size()+start);
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method adds the given vertices to the vertex buffer. In addition,
 * this method adds the requisite indices to the index buffer to draw
 * these vertices as a triangle fan (anchored on the first element).
 * This method is ideal for meshes on convex polygons.
 *
 * With that said, this method does not actually draw the triangle fan.
 * You must call {@link #flush} or {@link #end} to draw the vertices.
 * This method will automatically flush if the maximum number of vertices
 * (or uniform blocks) is reached.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param vertices  The vertices to draw as a triangle fan
 * @param size      The number of vertices in the fan
 * @param mat       The transform to apply to the vertices
 * @param tint      Whether to tint with the active color
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::prepare(const SpriteVertex2* vertices, size_t size, const Affine2& mat, bool tint) {
    CUAssertLog(size >= 3, "Vertices do not form a triangle fan");
    if (size >= _vertMax || 3*(size-2) >= _indxMax) {
        return chunkify(vertices, size, mat, tint);
    } else if(_vertSize+size > _vertMax || _indxSize+3*(size-2) > _indxMax) {
        flush();
    }
    
    setUniformBlock(_context);
    int ii = 0;
    tint = tint && _color != Color4::WHITE;
    for(size_t kk = 0; kk < size; kk++) {
        _vertData[_vertSize+ii] = vertices[kk];
        _vertData[_vertSize+ii].position = vertices[kk].position*mat;
        if (tint) {
            Uint32 c = marshall(_vertData[_vertSize+ii].color);
            Uint32 r = round(_color.r*((c >> 24)/255.0f));
            Uint32 g = round(_color.g*(((c >> 16) & 0xff)/255.0f));
            Uint32 b = round(_color.b*(((c >> 8) & 0xff)/255.0f));
            Uint32 a = round(_color.a*((c & 0xff)/255.0f));
            _vertData[_vertSize+ii].color = marshall(r << 24 | g << 16 | b << 8 | a);
        }
        ii++;
    }
    
    int jj = 0;
    for(Uint32 kk = 2; kk < size; kk++) {
        _indxData[_indxSize+jj  ] = _vertSize;
        _indxData[_indxSize+jj+1] = _vertSize+kk-1;
        _indxData[_indxSize+jj+2] = _vertSize+kk;
        jj += 3;
    }
    
    _vertSize += ii;
    _indxSize += jj;
    _inflight = true;
    return ii;
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method is an alternate version of {@link #prepare} for the same
 * arguments. It runs slower (e.g. the compiler cannot easily optimize
 * the loops) but it is guaranteed to work on any number of vertices.
 * This is important for avoiding memory corruption.
 *
 * If depth testing is on, all vertices will use the current sprite
 * batch depth.
 *
 * @param vertices  The vertices to draw as a triangle fan
 * @param size      The number of vertices in the fan
 * @param mat       The transform to apply to the vertices
 * @param tint      Whether to tint with the active color
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::chunkify(const SpriteVertex2* vertices, size_t size, const Affine2& mat, bool tint) {
    setUniformBlock(_context);

    const int chunksize = 3;
    unsigned int start  = _vertSize;
    unsigned int anchor = start;

    bool fresh = true;
    for(int ii = 1;  ii < size; ii++) {
        if (_indxSize+chunksize > _indxMax || _vertSize+chunksize >= _vertMax) {
            flush();
            fresh = true;
        }
        
        if (fresh && size-ii >= 2) {
            anchor = _vertSize;
            _vertData[_vertSize++] = vertices[0];
            _vertData[_vertSize++] = vertices[ii];
        } else if (fresh) {
            anchor = _vertSize;
            _vertData[_vertSize++] = vertices[0];
            _vertData[_vertSize++] = vertices[size-2];
            _vertData[_vertSize++] = vertices[size-1];
            _indxData[_indxSize++] = anchor;
            _indxData[_indxSize++] = anchor+1;
            _indxData[_indxSize++] = anchor+2;
        } else {
            _vertData[_vertSize] = vertices[ii];
            _indxData[_indxSize++] = anchor;
            _indxData[_indxSize++] = _vertSize-1;
            _indxData[_indxSize++] = _vertSize;
            _vertSize++;
        }
    }

    _inflight = true;
    return (unsigned int)(size+start);
}
