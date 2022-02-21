//
//  CUCanvasNode.cpp
//  Cornell University Game Library (CUGL)
//
//  This module is a near-complete recreation of NanoVG by Mikko Mononen
//  (memon@inside.org). The goal is to give users an easy way to create
//  scalable vector graphics in the engine, so that they do not have to
//  do everything with textures. In addition, the API is designed so that
//  it can support a significant subset of SVG, thus allowing artist to
//  import this file type.
//
//  For those familiar with NanoVG, this interface does have some important
//  differences. First of all, non-drawing commands are factored out into
//  other classes, like Font, TextLayout, or Affine2.  Those classes already
//  provide a lot of the support functionality present in NanVG. This class
//  provides only the drawing context, which is the new functionality.
//
//  In addition, this class places the origin in the bottom left corner with
//  an increasing y-axis, as is consistent with the scene graph framework.
//  On the other hand, NanoVG uses a top left origin with decreasing y-axis.
//  This does change the order of some commands but does not affect the
//  functionality of the module.
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
//  Version: 8/6/21
//
#include <cugl/scene2/graph/CUCanvasNode.h>
#include <cugl/util/CUTimestamp.h>
#include <cugl/math/cu_math.h>
#include <cugl/render/cu_render.h>

using namespace cugl;
using namespace cugl::scene2;

#define MIN_TOLERANCE   0.005f
#define MAX_TOLERANCE   10000.0f
#define KAPPA90         0.5522847493f    // Length proportional to radius of a cubic bezier handle for 90deg arcs.
/**
 * Returns the sign (1, -1, or 0) of the given number
 *
 * @param a     The number to test
 *
 * @return the sign (1, -1, or 0) of the given number
 */
static float signf(float a) { return a >= 0.0f ? 1.0f : -1.0f; }

#pragma mark -
#pragma mark Paint

/**
 * Creates an uninitialized paint.
 *
 * You must initialize this paint before use. Otherwise it will not do
 * anything do anything when applied.
 *
 * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate a Node on
 * the heap, use one of the static constructors instead.
 */
CanvasNode::Paint::Paint() :
_type(UNKNOWN),
_factor1(0.0f),
_factor2(0.0f),
_texture(nullptr) {
}

/**
 * Initializes a linear gradient with the given start and end positions.
 *
 * In a linear gradient, the inner starts at position start, and
 * transitions to the outer color at position end. The transition
 * is along the vector end-start.
 *
 * This initializer is very similar to {@link Gradient#initLinear},
 * except that the positions are given in the coordinate system of the
 * canvas node, and not using texture coordinates.
 *
 * @param inner The inner gradient color
 * @param outer The outer gradient color
 * @param start The start position of the inner color
 * @param end   The start position of the outer color
 *
 * @return true if initialization was successful.
 */
bool CanvasNode::Paint::initLinearGradient(Color4 inner, Color4 outer, const Vec2 start, const Vec2 end) {
    _type = LINEAR;
    _inner = inner;
    _outer = outer;
    _start = start;
    _extent = end;
    _factor1 = 0.0f;
    _factor2 = 0.0f;
    _transform.setIdentity();
    return true;
}

/**
 * Initializes a general radial gradient of the two colors.
 *
 * In a general radial gradient, the inner color starts at the center
 * and continues to the inner radius.  It then transitions smoothly
 * to the outer color at the outer radius.
 *
 * This initializer is very similar to {@link Gradient#initRadial},
 * except that the positions are given in the coordinate system of the
 * canvas node, and not using texture coordinates.
 *
 * @param inner     The inner gradient color
 * @param outer     The outer gradient color
 * @param center    The center of the radial gradient
 * @param iradius   The radius for the inner color
 * @param oradius   The radius for the outer color
 *
 * @return true if initialization was successful.
 */
bool CanvasNode::Paint::initRadialGradient(Color4 inner, Color4 outer, const Vec2 center,
                                           float iradius, float oradius) {
    _type = RADIAL;
    _inner = inner;
    _outer = outer;
    _start = center;
    _factor1 = iradius;
    _factor2 = oradius;
    _transform.setIdentity();
    return true;
}

/**
 * Initializes a box gradient of the two colors.
 *
 * Box gradients paint the inner color in a rounded rectangle, and
 * then use a feather setting to transition to the outer color. To be
 * well-defined, the corner radius should be no larger than half the
 * width and height (at which point it defines a capsule). Shapes
 * with abnormally large radii are undefined.
 *
 * The feather value acts like the inner and outer radius of a radial
 * gradient. If a line is drawn from the center of the round rectangle
 * to a corner, consider two segments. The first starts at the corner
 * and moves towards the center of the rectangle half-feather in
 * distance.  The end of this segment is the end of the inner color
 * The second second starts at the corner and moves in the opposite
 * direction the same amount. The end of this segement is the other
 * color. In between, the colors are smoothly interpolated. Hence the
 * feather effectively defines the pixel size of the transition zone.
 *
 * This initializer is very similar to {@link Gradient#initBox}, except
 * that the positions are given in the coordinate system of the canvas
 * node, and not using texture coordinates.
 *
 * @param inner     The inner gradient color
 * @param outer     The outer gradient color
 * @param box       The bounds of the rounded rectangle.
 * @param radius    The corner radius of the rounded rectangle.
 * @param feather   The feather value for color interpolation
 *
 * @return true if initialization was successful.
 */
bool CanvasNode::Paint::initBoxGradient(Color4 inner, Color4 outer, const Vec2 origin, const Size size,
                                        float radius, float feather) {
    _type = BOX;
    _inner = inner;
    _outer = outer;
    _start = origin;
    _extent = size;
    _factor1 = radius;
    _factor2 = feather;
    _transform.setIdentity();
    return true;
}

/**
 * Initializes a texture pattern with the given bounds.
 *
 * When painting a texture pattern, the texture is sized and offset
 * to fit within the given bounds. The texture will either be
 * clamped or repeated outside of these bounds, depending upon the
 * texture settings. As with all paints, these bounds are specified
 * in the coordinate system of the canvas node, and not using
 * texture coordinates.
 *
 * Typically a texture pattern is applied to a shape that fully
 * fits within the bounds. For example, if these bounds are the
 * bounding box of a polygon, and this paint is applied to the
 * polygon, the effect is the same as for a {@link PolygonNode}.
 *
 * @param texture   The texture to paint with
 * @param origin    The position of the texture bottom left corner
 * @param size      The size to scale the texture
 *
 * @return true if initialization was successful.
 */
bool CanvasNode::Paint::initPattern(std::shared_ptr<Texture> texture, const Vec2 origin, const Size size) {
    _type = PATTERN;
    _texture = texture;
    _start  = origin;
    _extent = size;
    _factor1 = 0;
    _factor2 = 0;
    _transform.setIdentity();
    return true;

    return false;
}

/**
 * Returns true if the paint is a gradient.
 *
 * Note that only gradient paints may be applied to text. Images
 * patterns applied to text will be ignored.
 *
 * @return true if the paint is a gradient.
 */
bool CanvasNode::Paint::isGradient() const {
    return _type != Type::PATTERN && _type != Type::UNKNOWN;
}

#pragma mark -
#pragma mark Commands

/**
 * An enum representing the command type
 *
 * The purpose of this enum is to convert the command into a sequence of
 * calls to the {@link SpriteBatch}. Each command type uses the sprite
 * batch differently.
 */
enum CommandType {
    /**
     * An undefined command.
     *
     * This is for uninitialized commands only.
     */
    UNDEFINED,
    /**
     * A command for an extruded path.
     *
     * This is a temporary command. Before execution it will be converted
     * into the appropriate stroke type according to the fill rule (which
     * applies to solid strokes).
     */
    STROKE,
    /**
     * A command for a normal extruded path.
     *
     * This command will not be subject to the stencil buffer, and will
     * in fact erase any active stencil.
     */
    NORMAL_STROKE,
    /**
     * A command for an extruded path drawn to the stencil buffer.
     *
     * The stroke will not be visible. This command is purely for clipping
     * and masking effects.
     */
    STENCIL_STROKE,
    /**
     * A command for an extruded path clipped to the stencil buffer.
     *
     * This command will draw a normal stroke, but it will be clipped to
     * the stencil buffer.  There must be a previous use of the STENCIL
     * fill rule for this to have an effect.
     */
    CLIP_STROKE,
    /**
     * A command for an extruded path masked by the stencil buffer.
     *
     * This command will draw a normal stroke, but it will be masked by
     * the stencil buffer.  There must be a previous use of the STENCIL
     * fill rule for this to have an effect.
     */
    MASK_STROKE,
    /**
     * A command for a filled path.
     *
     * This is a temporary command. Before execution it will be converted
     * into the appropriate fill type according to the fill rule.
     */
    FILL,
    /**
     * A command for a filled convex path.
     */
    CONVEX_FILL,
    /**
     * A command for a filled concave path (using non-zero rule).
     *
     * This command will not be subject to the stencil buffer, and will
     * in fact erase any active stencil.
     */
    CONCAVE_FILL,
    /**
     * A command for a filled concave path (using even-odd rule).
     *
     * This command will not be subject to the stencil buffer, and will
     * in fact erase any active stencil.
     */
    EVENODD_FILL,
    /**
     * A command for a filled path drawn to the stencil buffer.
     *
     * The solid shape will be drawn using the even-odd rule. The shape
     * will not be visible. This command is purely for clipping and
     * masking effects.
     */
    STENCIL_FILL,
    /**
     * A command for a filled path clipped to the stencil buffer.
     *
     * The solid shape will be drawn using the even-odd rule. The shape
     * will be clipped to the stencil buffer. There must be a previous use
     * of the STENCIL fill rule for this to have an effect.
     */
    CLIP_FILL,
    /**
     * A command for a filled path masked by the stencil buffer.
     *
     * The solid shape will be drawn using the even-odd rule. The shape
     * will be clipped to the stencil buffer. There must be a previous use
     * of the STENCIL fill rule for this to have an effect.
     */
    MASK_FILL,
    /**
     * A command for rendering text.
     *
     * This is a temporary command. Before execution it will be converted
     * into the appropriate fill type according to the fill rule.
     */
    TEXT,
    /**
     * A command for rendering text.
     *
     * This command will not be subject to the stencil buffer, and will
     * in fact erase any active stencil.
     */
    NORMAL_TEXT,
    /**
     * A command for rendering text to the stencil buffer.
     *
     * The rendered text will be drawn using the even-odd rule. The text
     * will not be visible. This command is purely for clipping and
     * masking effects.
     */
    STENCIL_TEXT,
    /**
     * A command for rendering text clipped to the stencil.
     *
     * The rendered text will be drawn using the even-odd rule. The text
     * will be clipped to the stencil buffer. There must be a previous use
     * of the STENCIL fill rule for this to have an effect.
     */
    CLIP_TEXT,
    /**
     * A command for rendering text masked by the stencil.
     *
     * The rendered text will be drawn using the even-odd rule. The text
     * will be clipped to the stencil buffer. There must be a previous use
     * of the STENCIL fill rule for this to have an effect.
     */
    MASK_TEXT
};


/**
 * A drawing command to send to the {@link SpriteBatch}
 *
 * Each command encapsulates a single mesh gradient/texture pairing
 * to send to the {@link SpriteBatch}. A command represents a drawing
 * that has been completed and saved.
 */
class CanvasNode::Command {
public:

    /** The command type, for interpretting the meshes */
    CommandType type;
    /** The mesh with the drawing information */
    Mesh<SpriteVertex2> mesh;
    /** The mesh with the fringe border (if applicable) */
    Mesh<SpriteVertex2> border;
    /** The mesh gradient (may be nullptr) */
    std::shared_ptr<Gradient> gradient;
    /** The mesh texture (may be nullptr) */
    std::shared_ptr<Texture> texture;
    /** The mesh scissor (may be nullptr) */
    std::shared_ptr<Scissor> scissor;
    /** The blur step (reserved for fonts) */
    float blurStep;
    /** The current blend equation */
    GLenum blendEquation;
    /** The current src blend function for the RGB values */
    GLenum blendSrcRGB;
    /** The current src blend function for the alpha value */
    GLenum blendSrcAlpha;
    /** The current dst blend function for the RGB values */
    GLenum blendDstRGB;
    /** The current dst blend function for the alpha value */
    GLenum blendDstAlpha;
    
    /**
     * Creates a new drawing command.
     *
     * This only initializes the values with the defaults. Drawing
     * commands are intended to be manipulated directly through their
     * fields (like a struct).
     */
    Command() :
    type(UNDEFINED),
    gradient(nullptr),
    texture(nullptr),
    scissor(nullptr),
    blurStep(0),
    blendEquation(GL_FUNC_ADD),
    blendSrcRGB(GL_SRC_ALPHA),
    blendSrcAlpha(GL_SRC_ALPHA),
    blendDstRGB(GL_ONE_MINUS_SRC_ALPHA),
    blendDstAlpha(GL_ONE_MINUS_SRC_ALPHA) {
    }
    
    /**
     * Disposes this drawing command.
     */
    ~Command() {
        gradient = nullptr;
        texture = nullptr;
        scissor = nullptr;
        mesh.clear();
    }
    
    /**
     * Resets a drawing command to the default values
     */
    void reset() {
        mesh.clear();
        type = UNDEFINED;
        gradient = nullptr;
        texture = nullptr;
        scissor = nullptr;
        blurStep = 0;
        blendEquation = GL_FUNC_ADD;
        blendSrcRGB = GL_SRC_ALPHA;
        blendSrcAlpha = GL_SRC_ALPHA;
        blendDstRGB = GL_ONE_MINUS_SRC_ALPHA;
        blendDstAlpha = GL_ONE_MINUS_SRC_ALPHA;
    }
    
    /**
     * Applies the given paint to this drawing command
     *
     * The bounds are necessary to compute the texture coordinates for
     * gradients and textures. It should always be the content size of
     * the canvas node.
     *
     * @param paint     The paint to apply
     * @param bounds    The content size of the canvas node
     */
    void applyPaint(CanvasNode::Paint* paint, Size bounds) {
        float scale = std::min(bounds.width,bounds.height);
        switch(paint->_type) {
        case CanvasNode::Paint::Type::LINEAR:
                gradient = Gradient::allocLinear(paint->_inner,paint->_outer,paint->_start/scale,paint->_extent/scale);
                for(auto it = mesh.vertices.begin(); it != mesh.vertices.end(); ++it) {
                    it->gradcoord.set(it->position);
                    it->gradcoord *= paint->_transform;
                    it->gradcoord /= scale;
                }
                for(auto it = border.vertices.begin(); it != border.vertices.end(); ++it) {
                    it->gradcoord.set(it->position);
                    it->gradcoord *= paint->_transform;
                    it->gradcoord /= scale;
                }
                break;
            case CanvasNode::Paint::Type::RADIAL:
                gradient = Gradient::allocRadial(paint->_inner, paint->_outer, paint->_start/scale, paint->_factor1/scale, paint->_factor2/scale);
                for(auto it = mesh.vertices.begin(); it != mesh.vertices.end(); ++it) {
                    it->gradcoord.set(it->position);
                    it->gradcoord *= paint->_transform;
                    it->gradcoord /= scale;
                }
                for(auto it = border.vertices.begin(); it != border.vertices.end(); ++it) {
                    it->gradcoord.set(it->position);
                    it->gradcoord *= paint->_transform;
                    it->gradcoord /= scale;
                }
                break;
            case CanvasNode::Paint::Type::BOX:
                gradient = Gradient::allocBox(paint->_inner, paint->_outer, paint->_start/scale, paint->_extent/scale,
                                              paint->_factor1/scale, paint->_factor2/scale);
                for(auto it = mesh.vertices.begin(); it != mesh.vertices.end(); ++it) {
                    it->gradcoord.set(it->position);
                    it->gradcoord *= paint->_transform;
                    it->gradcoord /= scale;
                }
                for(auto it = border.vertices.begin(); it != border.vertices.end(); ++it) {
                    it->gradcoord.set(it->position);
                    it->gradcoord *= paint->_transform;
                    it->gradcoord /= scale;
                }
                break;
            case CanvasNode::Paint::Type::PATTERN:
                texture = paint->_texture;
                for(auto it = mesh.vertices.begin(); it != mesh.vertices.end(); ++it) {
                    it->texcoord.set(it->position);
                    it->texcoord *= paint->_transform;
                    it->texcoord -= paint->_start;
                    it->texcoord /= paint->_extent;
                    it->texcoord.y = 1-it->texcoord.y;
                }
                for(auto it = border.vertices.begin(); it != border.vertices.end(); ++it) {
                    it->texcoord.set(it->position);
                    it->texcoord *= paint->_transform;
                    it->texcoord -= paint->_start;
                    it->texcoord /= paint->_extent;
                    it->texcoord.y = 1-it->texcoord.y;
                }
                break;
            case CanvasNode::Paint::Type::UNKNOWN:
                break;
                // Nothing
        }
    }

};


#pragma mark -
#pragma mark Contexts
/**
 * A single instance of the canvas state.
 *
 * Like most traditional canvases, all drawing options such as
 * color, transform, or scissor can be saved on stack for use again
 * later. This class represents the elements of that stack.
 */
class CanvasNode::Context {
public:
    CanvasNode* node;
    /** The fringe width for antialiasing */
    float fringe;
    /** The current orientation winding for each path */
    Winding winding;
    /** The current fill rule for rendering solid shapes */
    FillRule fillrule;
    /** The active fill color */
    Color4 fillColor;
    /** The active stroke color */
    Color4 strokeColor;
    /** The stroke width for extruded paths */
    float strokeWidth;
    /** The mitre limit for mitred corners */
    float mitreLimit;
    /** The global alpha value */
    float globalAlpha;
    /** The active blend equation */
    GLenum blendEquation;
    /** The active src blend function for the RGB values */
    GLenum blendSrcRGB;
    /** The active src blend function for the alpha value */
    GLenum blendSrcAlpha;
    /** The active dst blend function for the RGB values */
    GLenum blendDstRGB;
    /** The active dst blend function for the alpha value */
    GLenum blendDstAlpha;
    /** The line end cap for extrusions */
    poly2::EndCap lineCap;
    /** The line joint for extrusions */
    poly2::Joint  lineJoint;
    /** The active matrix transform (may be nullptr) */
    std::shared_ptr<Affine2> transform;
    /** The active scissor (may be nullptr) */
    std::shared_ptr<Scissor> scissor;
    /** The active stroke paint (may be nullptr) */
    std::shared_ptr<Paint> strokePaint;
    /** The active fill paint (may be nullptr) */
    std::shared_ptr<Paint> fillPaint;
    /** The font to use for rendering text */
    std::shared_ptr<Font> fontFace;
    /** The horizontal text alignment */
    HorizontalAlign fontHAlign;
    /** The vertical text alignment */
    VerticalAlign   fontVAlign;
    /** The font size to scale to (0 for default) */
    float fontSize;
    /** The font blur radius */
    float fontBlur;
    /** The font line spacing factor */
    float fontSpacing;

    /**
     * Creates a new drawing context.
     *
     * This only initializes the values with the defaults.  Drawing
     * context are intended to be manipulated directly through their
     * fields (like a struct).
     */
    Context(CanvasNode* node) :
    fringe(0),
    winding(Winding::NONE),
    fillrule(FillRule::NONZERO),
    strokeWidth(2.0f),
    mitreLimit(10.0f),
    globalAlpha(1.0f),
    strokeColor(Color4::WHITE),
    fillColor(Color4::WHITE),
    blendEquation(GL_FUNC_ADD),
    blendSrcRGB(GL_SRC_ALPHA),
    blendSrcAlpha(GL_SRC_ALPHA),
    blendDstRGB(GL_ONE_MINUS_SRC_ALPHA),
    blendDstAlpha(GL_ONE_MINUS_SRC_ALPHA),
    lineCap(poly2::EndCap::BUTT),
    lineJoint(poly2::Joint::MITRE),
    transform(nullptr),
    scissor(nullptr),
    strokePaint(nullptr),
    fillPaint(nullptr),
    fontFace(nullptr),
    fontSize(0),
    fontBlur(0),
    fontSpacing(1),
    fontHAlign(HorizontalAlign::LEFT),
    fontVAlign(VerticalAlign::BASELINE)
    {
        this->node = node;
    }
    
    /**
     * Creates a copy of the drawing context.
     *
     * @param copy  The context to copy
     */
    Context(Context* copy) {
        node = copy->node;
        fringe  = copy->fringe;
        winding = copy->winding;
        fillrule = copy->fillrule;
        strokeWidth = copy->strokeWidth;
        mitreLimit = copy->mitreLimit;
        globalAlpha = copy->globalAlpha;
        strokeColor = copy->strokeColor;
        fillColor = copy->fillColor;
        blendEquation = copy->blendEquation;
        blendSrcRGB = copy->blendSrcRGB;
        blendSrcAlpha = copy->blendSrcAlpha;
        blendDstRGB = copy->blendDstRGB;
        blendDstAlpha = copy->blendDstAlpha;
        lineCap = copy->lineCap;
        lineJoint = copy->lineJoint;
        transform = copy->transform;
        scissor = copy->scissor;
        strokePaint = copy->strokePaint;
        fillPaint = copy->fillPaint;
        fontFace = copy->fontFace;
        fontSize = copy->fontSize;
        fontBlur = copy->fontBlur;
        fontSpacing = copy->fontSpacing;
        fontHAlign = copy->fontHAlign;
        fontVAlign = copy->fontVAlign;
    }
    
    /**
     * Disposes this drawing context.
     */
    ~Context() {
        node = nullptr;
        transform = nullptr;
        scissor = nullptr;
        strokePaint = nullptr;
        fillPaint = nullptr;
        fontFace = nullptr;
    }
    
    /**
     * Resets a drawing context to the default values
     */
    void reset() {
        fringe  = 0;
        winding  = Winding::NONE;
        fillrule = FillRule::NONZERO;
        strokeWidth = 2.0f;
        mitreLimit = 10.0f;
        globalAlpha = 1.0f;
        strokeColor = Color4::WHITE;
        fillColor = Color4::WHITE;
        blendEquation = GL_FUNC_ADD;
        blendSrcRGB = GL_SRC_ALPHA;
        blendSrcAlpha = GL_SRC_ALPHA;
        blendDstRGB = GL_ONE_MINUS_SRC_ALPHA;
        blendDstAlpha = GL_ONE_MINUS_SRC_ALPHA;
        lineCap = poly2::EndCap::BUTT;
        lineJoint = poly2::Joint::MITRE;
        transform = nullptr;
        scissor = nullptr;
        strokePaint = nullptr;
        fillPaint = nullptr;
        fontFace = nullptr;
        fontSize = 0;
        fontBlur = 0;
        fontSpacing = 1;
        fontHAlign = HorizontalAlign::LEFT;
        fontVAlign = VerticalAlign::BASELINE;
    }

};

#pragma mark -
#pragma mark Canvas Pages
/**
 * An enum representing the path orientation.
 *
 * Paths can either be convex or concave, and they can be CCW or CW (a
 * colinear path is degenerate and only supported for strokes). Each
 * of these is treated differently when drawn, so this enum exists to
 * categorize them.
 */
enum PathOrientation {
    /** A path of all colinear points */
    COLINEAR,
    /** A convex path that is oriented counter clockwise */
    CCW_CONVEX,
    /** A concave path that is oriented counter clockwise */
    CCW_CONCAVE,
    /** A convex path that is oriented clockwise */
    CW_CONVEX,
    /** A concave path that is oriented clockwise */
    CW_CONCAVE
};


/**
 * A single drawing canvas page
 *
 * In order to facilitate animation, this class can have multiple
 * active pages. This class represents a single canvas. It stores
 * both the drawing state (as a stack of {@link Context} objects)
 * and the render state (as a sequence of {@link Command} objects).
 */
class CanvasNode::Page {
public:
    /** The canvas node associate with this page */
    CanvasNode* node;
    /** The context stack */
    std::vector<Context*> contexts;
    /** The command stack */
    std::vector<Command*> commands;
    /** The current list of committed paths */
    std::vector<Path2*> paths;
    /** The current path orientations */
    std::vector<PathOrientation> orients;
    /** A tool for flattening splines */
    SplinePather flatner;
    /** A toold for extruding paths */
    SimpleExtruder extruder;
    /* The spline "workspace" for an uncommited path */
    Spline2 spline;
    /** Whether there is an uncommitted path */
    bool active;
    /** The active text layout */
    TextLayout   layout;
    /** The text origin offset */
    Vec2 textorigin;

    /**
     * Creates a new canvas page
     *
     * This initializes a single drawing context for immediate use.
     * No commands (or path objects) are yet created.
     */
    Page(CanvasNode* node) : active(false) {
        this->node = node;
        contexts.push_back(new Context(node));
    }
    
    /**
     * Deletes this canvas page, disposing of all resources.
     */
    ~Page() {
        clearContexts();
        clearCommands();
        clearPaths();
        flatner.clear();
        extruder.clear();
        active = false;
    }
    
    /**
     * Returns the current context for this page
     *
     * @return the current context for this page
     */
    Context* getState() {
        return contexts.back();
    }
    
    /**
     * Saves the current context, pushing it on the stack
     */
    void saveContext() {
        Context* prev = contexts.back();
        Context* next = new Context(prev);
        contexts.push_back(next);
    }

    /**
     * Pops the current context off the stack.
     *
     * The previous context will be restored.  If there is no previous
     * context, this has the same effect as {@link #resetContexts}.
     */
    void restoreContext() {
        Context* curr = contexts.back();
        if (contexts.size() > 1) {
            contexts.pop_back();
            delete curr;
        } else {
            curr->reset();
        }
    }
    
    /**
     * Restores the render state to the default context
     *
     * This option does not affect the context stack. Any contexes that
     * were previously saved are preserved.
     */
    void resetContexts() {
        for(auto it = contexts.begin(); it != contexts.end(); ++it) {
            delete *it;
            *it = nullptr;
        }
        contexts.clear();
        contexts.push_back(new Context(node));
    }

    /**
     * Removes all contexts from the stack
     *
     * This method will completely empty the context stack, making this
     * page unusable. This method should generally be limited to the
     * destructor.
     */
    void clearContexts() {
        for(auto it = contexts.begin(); it != contexts.end(); ++it) {
            delete *it;
            *it = nullptr;
        }
        contexts.clear();
    }

    /**
     * Removes all drawing commands from this page.
     *
     * Drawing this page will now have no effect.
     */
    void clearCommands() {
        for(auto it = commands.begin(); it != commands.end(); ++it) {
            delete *it;
            *it = nullptr;
        }
        commands.clear();
    }
    
    /**
     * Commits the current subpath to the path list.
     *
     * This method will properly orient the path before saving it. It will
     * also clear the spline workspace.
     */
    void savePath() {
        if (spline.size() > 0) {
            flatner.clear();
            flatner.set(&spline);
            flatner.calculate();

            paths.push_back(new Path2());
            Path2* path = paths.back();
            flatner.getPath(path);
            orientLastPath();
        }
        spline.clear();
        active = false;
    }
    
    /**
     * Reorients the most recently committed path
     *
     * The path is oriented so that it agrees with the
     */
    void orientLastPath() {
        Path2* path = paths.back();
        Context* state = contexts.back();

        // Enforce winding
        float area = path->area();
        if (state->winding == Winding::CCW && area < 0.0f) {
            path->reverse();
        } else if (state->winding == Winding::CW && area > 0.0f) {
            path->reverse();
        }
        
        // Get the path orientation
        if (state->winding == Winding::NONE) {
            if (path->isConvex()) {
                orients.push_back(area > 0.0f ? CCW_CONVEX : CW_CONVEX);
            } else {
                orients.push_back(area > 0.0f ? CCW_CONCAVE : CW_CONCAVE);
            }
        } else if (state->winding == Winding::CCW) {
            if (path->isConvex()) {
                orients.push_back(CCW_CONVEX);
            } else {
                orients.push_back(CCW_CONCAVE);
            }
        } else if (path->leftTurns() == 0 && path->closed) {
            orients.push_back(CW_CONVEX);
        } else {
            orients.push_back(CCW_CONCAVE);
        }
    }
    
    /**
     * Removes all cached paths from this page.
     */
    void clearPaths() {
        for(auto it = paths.begin(); it != paths.end(); ++it) {
            delete *it;
            *it = nullptr;
        }
        paths.clear();
        spline.clear();
        active = false;
    }
    
    /**
     * Materializes the current drawing state into a sequence of commands
     *
     * This has the side effect of committing the most recent path, and thus
     * reseting the spline workspace.
     *
     * @param ctype     The type of command to materialize
     */
    void materialize(CommandType ctype) {
        Context* state = getState();
        Command* packet = new Command();
        commands.push_back(packet);
        packet->blendEquation = state->blendEquation;
        packet->blendSrcRGB   = state->blendSrcRGB;
        packet->blendSrcAlpha = state->blendSrcAlpha;
        packet->blendDstRGB   = state->blendDstRGB;
        packet->blendDstAlpha = state->blendDstAlpha;
        packet->scissor  = state->scissor;
        
        Rect bounds = Rect::ZERO;
        packet->border.command = GL_TRIANGLES;
        
        CommandType actual = ctype;
        
        // Text is special
        if (ctype == CommandType::TEXT) {
            switch(state->fillrule) {
                case FillRule::STENCIL:
                    actual = STENCIL_TEXT;
                    break;
                case FillRule::CLIPFILL:
                    actual = CLIP_TEXT;
                    break;
                case FillRule::MASKFILL:
                    actual = MASK_TEXT;
                    break;
                default:
                    actual = NORMAL_TEXT;
                    break;
            }
            Color4 color = state->fillColor;
            color.a *= state->globalAlpha;
            Uint32 rgba = color.getPacked();

            
            // Compute the transform
            Affine2 xform = Affine2::IDENTITY;
            if (state->fontSize > 0 && state->fontSize != state->fontFace->getPointSize()) {
                xform.scale(state->fontSize/state->fontFace->getPointSize());
            }
            xform.translate(textorigin);
            if (state->transform != nullptr) {
                xform *= *(state->transform);
            }

            Size bounds = node->getContentSize();
            std::unordered_map<GLuint,std::shared_ptr<GlyphRun>> runs = layout.getGlyphs();
            for(auto it = runs.begin(); it != runs.end(); ) {
                packet->blurStep = state->fontBlur;
                packet->type = actual;
                packet->mesh = it->second->mesh;
                packet->texture = it->second->texture;
                for(auto jt = packet->mesh.vertices.begin(); jt != packet->mesh.vertices.end(); ++jt) {
                    jt->position *= xform;
                    jt->color = rgba;
                }
                if (state->fillPaint != nullptr && state->fillPaint->isGradient())  {
                    packet->applyPaint(state->fillPaint.get(), bounds);
                }
                
                ++it;
                if (it != runs.end()) {
                    packet = new Command();
                    commands.push_back(packet);
                    packet->blendEquation = state->blendEquation;
                    packet->blendSrcRGB   = state->blendSrcRGB;
                    packet->blendSrcAlpha = state->blendSrcAlpha;
                    packet->blendDstRGB   = state->blendDstRGB;
                    packet->blendDstAlpha = state->blendDstAlpha;
                    packet->scissor = state->scissor;
                }
            }
        } else {
            bool convex = true;
            for(size_t ii = 0; ii < paths.size(); ii++) {
                Path2* path = paths[ii];
                PathOrientation direction = orients[ii];
                if (path->size() > 0) {
                    convex = convex && direction == CCW_CONVEX;
                    bounds.merge(path->getBounds());
                    
                    switch(ctype) {
                        case CommandType::FILL:
                        case CommandType::CONVEX_FILL:
                        case CommandType::CONCAVE_FILL:
                        case CommandType::EVENODD_FILL:
                        case CommandType::STENCIL_FILL:
                        case CommandType::CLIP_FILL:
                        case CommandType::MASK_FILL:
                        {
                            packet->mesh.command = GL_TRIANGLE_FAN;
                            Color4 color = state->fillColor;
                            color.a *= state->globalAlpha;
                            Uint32 rgba = color.getPacked();

                            // Extract the positions
                            Uint32 offset = (Uint32)packet->mesh.vertices.size();
                            packet->mesh.vertices.reserve(offset+path->vertices.size());
                            for(auto jt = path->vertices.begin(); jt != path->vertices.end(); ++jt) {
                                packet->mesh.vertices.push_back(SpriteVertex2());
                                SpriteVertex2* vert = &(packet->mesh.vertices.back());
                                vert->position = *jt;
                                vert->color = rgba;
                            }
                            
                            packet->mesh.indices.push_back((Uint32)packet->mesh.vertices.size());
                            
                            // Process the fringe (if necessary)
                            if (state->fringe > 0) {
                                Color4 clear = state->fillColor;
                                clear.a = 0;
                                
                                SimpleExtruder extruder;
                                extruder.clear();
                                extruder.set(path->vertices,true);
                                extruder.setJoint(poly2::Joint::MITRE);
                                
                                switch (direction) {
                                    case CCW_CONCAVE:
                                    case CW_CONCAVE:
                                        // Need both sides
                                        extruder.calculate(0,state->fringe);
                                        extruder.getMesh(&packet->border,color,clear);
                                        extruder.reset();
                                    case CCW_CONVEX:
                                    case CW_CONVEX:
                                        // Interior is to the left
                                        extruder.calculate(0,state->fringe);
                                        extruder.getMesh(&packet->border,color,clear);
                                        break;
                                    case COLINEAR:
                                        break;
                                }
                            }
                            
                            switch (state->fillrule) {
                                case FillRule::NONZERO:
                                    if (ctype == CommandType::FILL) {
                                        packet->type = convex ? CommandType::CONVEX_FILL : CommandType::CONCAVE_FILL;
                                    } else {
                                        packet->type = ctype;
                                    }
                                    break;
                                case FillRule::EVENODD:
                                    packet->type = CommandType::EVENODD_FILL;
                                    break;
                                case FillRule::STENCIL:
                                    packet->type = CommandType::STENCIL_FILL;
                                    break;
                                case FillRule::CLIPFILL:
                                    packet->type = CommandType::CLIP_FILL;
                                    break;
                                case FillRule::MASKFILL:
                                    packet->type = CommandType::MASK_FILL;
                                    break;
                            }
                        }
                            break;
                        case CommandType::STROKE:
                        case CommandType::NORMAL_STROKE:
                        case CommandType::STENCIL_STROKE:
                        case CommandType::CLIP_STROKE:
                        case CommandType::MASK_STROKE:
                        {
                            packet->mesh.command = GL_TRIANGLES;
                            Color4 color = state->strokeColor;
                            color.a *= state->globalAlpha;
                            
                            // Extrude the basic shape
                            SimpleExtruder extruder;
                            extruder.set(*path);
                            extruder.setMitreLimit(state->mitreLimit);
                            extruder.setEndCap(state->lineCap);
                            extruder.setJoint(state->lineJoint);
                            extruder.calculate(state->strokeWidth-state->fringe/2);
                            packet->mesh.command = GL_TRIANGLES;
                            extruder.getMesh(&packet->mesh,color);
                            
                            if (state->fringe > 0) {
                                Color4 clear = color;
                                clear.a = 0;

                                std::vector<Path2> outlines;
                                extruder.getBorder(outlines);
                                packet->border.command = GL_TRIANGLES;
                                for(auto jt = outlines.begin(); jt != outlines.end(); ++jt) {
                                    extruder.clear();
                                    extruder.set(*jt);
                                    extruder.setJoint(poly2::Joint::MITRE);
                                    extruder.setEndCap(poly2::EndCap::BUTT);
                                    extruder.calculate(0,state->fringe/2);
                                    extruder.getMesh(&packet->border,color,clear);
                                }
                            }
                            
                            
                            switch (state->fillrule) {
                                case FillRule::NONZERO:
                                case FillRule::EVENODD:
                                    if (ctype == CommandType::STROKE) {
                                        packet->type = CommandType::NORMAL_STROKE;
                                    } else {
                                        packet->type = ctype;
                                    }
                                    break;
                                case FillRule::STENCIL:
                                    packet->type = CommandType::STENCIL_STROKE;
                                    break;
                                case FillRule::CLIPFILL:
                                    packet->type = CommandType::CLIP_STROKE;
                                    break;
                                case FillRule::MASKFILL:
                                    packet->type = CommandType::MASK_STROKE;
                                    break;
                            }
                        }
                            break;
                        default:
                            packet->type = ctype;
                            break;
                    }
                }
            }
        
            // Add a final rectangle if not convex
            switch(packet->type) {
                case CommandType::CONCAVE_FILL:
                case CommandType::EVENODD_FILL:
                case CommandType::STENCIL_FILL:
                case CommandType::CLIP_FILL:
                case CommandType::MASK_FILL:
                {
                    SpriteVertex2* vert;
                    packet->mesh.vertices.push_back(SpriteVertex2());
                    vert = &(packet->mesh.vertices.back());
                    vert->position = bounds.origin;
                    vert->color = state->fillColor.getPacked();
                    packet->mesh.vertices.push_back(SpriteVertex2());
                    vert = &(packet->mesh.vertices.back());
                    vert->position = bounds.origin;
                    vert->position.x += bounds.size.width;
                    vert->color = state->fillColor.getPacked();
                    packet->mesh.vertices.push_back(SpriteVertex2());
                    vert = &(packet->mesh.vertices.back());
                    vert->position = bounds.origin+bounds.size;
                    vert->color = state->fillColor.getPacked();
                    packet->mesh.vertices.push_back(SpriteVertex2());
                    vert = &(packet->mesh.vertices.back());
                    vert->position = bounds.origin+bounds.size;
                    vert->position.x -= bounds.size.width;
                    vert->color = state->fillColor.getPacked();
                    packet->mesh.indices.push_back((Uint32)packet->mesh.vertices.size());
                }
                    break;
                default:
                    break;
            }

            // Apply the paints
            switch(packet->type) {
                case CommandType::FILL:
                case CommandType::CONVEX_FILL:
                case CommandType::CONCAVE_FILL:
                case CommandType::EVENODD_FILL:
                case CommandType::CLIP_FILL:
                case CommandType::MASK_FILL:
                    if (state->fillPaint) {
                        packet->applyPaint(state->fillPaint.get(), node->getContentSize());
                    }
                    break;
                case CommandType::STROKE:
                case CommandType::NORMAL_STROKE:
                case CommandType::CLIP_STROKE:
                case CommandType::MASK_STROKE:
                    if (state->strokePaint) {
                        packet->applyPaint(state->strokePaint.get(), node->getContentSize());
                    }
                    break;
                default:
                    break;
            }
        }
    }
};

#pragma mark -
#pragma mark Constructors
/**
 * Disposes all of the resources used by this canvas node.
 *
 * A disposed node can be safely reinitialized. Any children owned by this
 * node will be released.  They will be deleted if no other object owns them.
 *
 * It is unsafe to call this on a Node that is still currently inside of
 * a scene graph.
 */
void CanvasNode::dispose() {
    for(auto it = _canvas.begin(); it != _canvas.end(); ++it) {
        delete *it;
        *it = nullptr;
    }
    _canvas.clear();
    SceneNode::dispose();
}

/**
 * Initializes a canvas node the size of the display.
 *
 * The bounding box of the node is the current screen size. The node
 * is anchored in the center and has position (width/2,height/2) in
 * the parent space. The node origin is the (0,0) at the bottom left
 * corner of the bounding box.
 *
 * The canvas is initialized with only one drawing buffer. You should
 * call {@link #reserve} to add more buffers.
 *
 * @return true if initialization was successful.
 */
bool CanvasNode::init() {
    if (SceneNode::init()) {
        _canvas.push_back(new Page(this));
        return true;
    }
    return false;
}

/**
 * Initializes a canvas node with the given size.
 *
 * The size defines the content size. The bounding box of the node is
 * (0,0,width,height) and is anchored in the bottom left corner (0,0).
 * The node is positioned at the origin in parent space.
 *
 * The canvas is initialized with only one drawing buffer. You should
 * call {@link #reserve} to add more buffers.
 *
 * @param size  The size of the node in parent space
 *
 * @return true if initialization was successful.
 */
bool CanvasNode::initWithBounds(const Size size) {
    if (SceneNode::initWithBounds(size)) {
        _canvas.push_back(new Page(this));
        return true;
    }
    return false;
}

/**
 * Initializes a canvas node with the given bounds.
 *
 * The rectangle origin is the bottom left corner of the node in parent
 * space, and corresponds to the origin of the Node space. The size
 * defines its content width and height in node space. The node anchor
 * is placed in the bottom left corner.
 *
 * The canvas is initialized with only one drawing buffer. You should
 * call {@link #reserve} to add more buffers.
 *
 * @param rect  The bounds of the node in parent space
 *
 * @return true if initialization was successful.
 */
bool CanvasNode::initWithBounds(const Rect rect) {
    if (SceneNode::initWithBounds(rect)) {
        _canvas.push_back(new Page(this));
        return true;
    }
    return false;
}

/**
 * Initializes a canvas node with the given JSON specificaton.
 *
 * This initializer is designed to receive the "data" object from the
 * JSON passed to {@link Scene2Loader}. This JSON format supports the
 * following attribute values:
 *
 *     "pages":     A positive integer indicating the number of pages
 *     "edit":      A positive integer indicating the active edit page
 *     "draw":      A positive integer indicating the active draw page
 *
 * All attributes are optional. There are no required attributes.
 * There are currently no options for drawing to a canvas node in
 * in the JSON (the canvas will start out blank). Serialized drawing
 * commands are a feature for a future release.
 *
 * @param loader    The scene loader passing this JSON file
 * @param data      The JSON object specifying the node
 *
 * @return true if initialization was successful.
 */
bool CanvasNode::initWithData(const Scene2Loader* loader, const std::shared_ptr<JsonValue>& data) {
    if (SceneNode::initWithData(loader, data)) {
        size_t pages = data->getLong("pages",1);
        _edit =  data->getLong("edit",1);
        _draw =  data->getLong("draw",1);
        pages = std::max(std::max(_edit,_draw),pages);
        _canvas.reserve(pages);
        for (size_t ii = 0; ii < pages; ii++ ) {
            _canvas.push_back(new Page(this));
        }
        return true;
    }
    return false;
}


#pragma mark -
#pragma mark Canvas Management
/**
 * Returns the number of pages in this canvas node.
 *
 * Each page is capable of storing its own set of drawing commands.
 * Flipping through pages is an efficient way to animate a canvas
 * node.
 *
 * By default a canvas node has only one page.
 *
 * @return the number of pages in this canvas node.
 */
size_t CanvasNode::pages() const {
    return _canvas.size();
}
    
/**
 * Resizes the canvas node to support the given number of pages
 *
 * Each page is capable of storing its own set of drawing commands.
 * Flipping through pages is an efficient way to animate a canvas
 * node.
 *
 * When repaginating a canvas, all pages with indices less than
 * size are preserved. Any pages with indices exceeding size are
 * discarded.
 *
 * @param size  The number of pages for this canvas node
 */
void CanvasNode::paginate(size_t size) {
    if (size > _canvas.size()) {
        for(auto it = _canvas.begin()+size; it != _canvas.end(); ++it) {
            delete *it;
            *it = nullptr;
        }
        _canvas.resize(size);
    } else if (size < _canvas.size()) {
        _canvas.reserve(_canvas.size()+size);
        for(size_t ii = 0; ii < size; ii++) {
            _canvas.push_back(new Page(this));
        }
    }
}
   
/**
 * Returns the index of the current edit page
 *
 * The edit page is the page that receives drawing commands. It
 * does not need to be the same page as the one currently being
 * drawn.
 *
 * @return the index of the current edit page
 */
size_t CanvasNode::getEditPage() {
    return _edit;
}

/**
 * Sets the index of the current edit page
 *
 * The edit page is the page that receives drawing commands. It
 * does not need to be the same page as the one currently being
 * drawn.
 *
 * If this index is higher than the number of pages, this canvas
 * will {@link #paginate} to support the request.
 *
 * @param page  The index of the current edit page
 */
void CanvasNode::setEditPage(size_t page) {
    if (page >= _canvas.size() ) {
        paginate(page+1);
    }
    _edit = page;
}

/**
 * Returns the index of the current drawing page
 *
 * The drawing page is the page that is shown on the screen. It
 * does not need to be the same page as the one currently
 * receiving drawing commands.
 *
 * @return the index of the current drawing page
 */
size_t CanvasNode::getDrawPage() {
    return _draw;
}
    
/**
 * Sets the index of the current drawing page
 *
 * The drawing page is the page that is shown on the screen. It
 * does not need to be the same page as the one currently
 * receiving drawing commands.
 *
 * If this index is higher than the number of pages, this canvas
 * will {@link #paginate} to support the request.
 *
 * @param page  The index of the current drawing page
 */
void CanvasNode::setDrawPage(size_t page) {
    if (page >= _canvas.size() ) {
        paginate(page+1);
    }
    _draw = page;
}

/**
 * Clears the drawing commands for the active edit page.
 *
 * Any other page is uneffected. This method should be called
 * before drawing to a page, as otherwise the commands are appended
 * to any existing drawing commands.
 */
void CanvasNode::clearPage() {
    Page* page = _canvas[_edit];
    page->clearCommands();
    page->clearPaths();
    page->resetContexts();
}

/**
 * Clears the drawing commands from all pages.
 */
void CanvasNode::clearAll() {
    for(auto it = _canvas.begin(); it != _canvas.end(); ++it) {
        (*it)->clearCommands();
        (*it)->clearPaths();
        (*it)->resetContexts();
    }
}

/**
 * Draws this Node via the given SpriteBatch.
 *
 * This method only worries about drawing the current node.  It does not
 * attempt to render the children.
 *
 * This is the method that you should override to implement your custom
 * drawing code.  You are welcome to use any OpenGL commands that you wish.
 * You can even skip use of the SpriteBatch.  However, if you do so, you
 * must flush the SpriteBatch by calling end() at the start of the method.
 * in addition, you should remember to call begin() at the start of the
 * method.
 *
 * This method provides the correct transformation matrix and tint color.
 * You do not need to worry about whether the node uses relative color.
 * This method is called by render() and these values are guaranteed to be
 * correct.  In addition, this method does not need to check for visibility,
 * as it is guaranteed to only be called when the node is visible.
 *
 * @param batch     The SpriteBatch to draw with.
 * @param transform The global transformation matrix.
 * @param tint      The tint to blend with the Node color.
 */
void CanvasNode::draw(const std::shared_ptr<SpriteBatch>& batch, const Affine2& transform, Color4 tint) {
    Page* page = _canvas[_draw];
    if (page->commands.empty()) {
        return;
    }
    
    std::shared_ptr<Scissor> origsciss = batch->getScissor();
    bool changesciss = false;
    
    Command* comm = page->commands.front();
    float blurStep  = comm->blurStep;
    GLenum blendEq  = comm->blendEquation;
    GLenum srcRGB   = comm->blendSrcRGB;
    GLenum srcAlpha = comm->blendSrcAlpha;
    GLenum dstRGB   = comm->blendDstRGB;
    GLenum dstAlpha = comm->blendDstAlpha;
    std::shared_ptr<Gradient> gradient = comm->gradient;
    std::shared_ptr<Texture>  texture  = comm->texture;
    std::shared_ptr<Scissor>  scissor  = comm->scissor;

    batch->setColor(tint);
    batch->setGradient(gradient);
    batch->setTexture(texture);
    if (scissor) {
        std::shared_ptr<Scissor> local  = Scissor::alloc(scissor);
        local->multiply(transform);
        if (origsciss) {
            local->intersect(origsciss);
        }
        batch->setScissor(local);
        changesciss = true;
    }

    
    batch->setBlendEquation(blendEq);
    batch->setSrcBlendFunc(srcRGB, srcAlpha);
    batch->setDstBlendFunc(dstRGB, dstAlpha);
    batch->setBlur(blurStep);
    for(auto it = page->commands.begin(); it != page->commands.end(); ++it) {
        comm = *it;
        if (comm->blurStep != blurStep) {
            blurStep = comm->blurStep;
            batch->setBlur(blurStep);
        }
        if (comm->blendEquation != blendEq) {
            blendEq = comm->blendEquation;
            batch->setBlendEquation(blendEq);
        }
        if (comm->blendSrcRGB != srcRGB || comm->blendSrcAlpha != srcAlpha) {
            srcRGB   = comm->blendSrcRGB;
            srcAlpha = comm->blendSrcAlpha;
            batch->setSrcBlendFunc(srcRGB, srcAlpha);
        }
        if (comm->blendDstRGB != dstRGB || comm->blendDstAlpha != dstAlpha) {
            dstRGB   = comm->blendDstRGB;
            dstAlpha = comm->blendDstAlpha;
            batch->setDstBlendFunc(dstRGB, dstAlpha);
        }
        if (comm->gradient != gradient) {
            gradient = comm->gradient;
            batch->setGradient(gradient);
        }
        if (comm->texture != texture) {
            texture = comm->texture;
            batch->setTexture(texture);
        }
        if (comm->scissor != scissor) {
            if (scissor) {
                std::shared_ptr<Scissor> local  = Scissor::alloc(scissor);
                local->multiply(transform);
                if (origsciss) {
                    local->intersect(origsciss);
                }
                batch->setScissor(local);
                changesciss = true;
            } else {
                batch->setScissor(origsciss);
                changesciss = false;
            }
        }

        bool fall = false;
        StencilEffect cstamp = StencilEffect::STAMP;
        StencilEffect cmask  = StencilEffect::MASK;
        StencilEffect cfill  = StencilEffect::FILL;

        switch (comm->type) {
            case CommandType::CONVEX_FILL:
            {
                batch->clearStencil();
                batch->setStencilEffect(StencilEffect::NONE);
                size_t start = 0;
                for (auto it = comm->mesh.indices.begin(); it != comm->mesh.indices.end(); ++it) {
                    batch->drawMesh(comm->mesh.vertices.data()+start,*it-start,transform);
                    start = *it;
                }
                if (comm->border.vertices.size() > 0) {
                    batch->drawMesh(comm->border, transform);
                }
            }
                break;
            case CommandType::CONCAVE_FILL:
                cstamp = StencilEffect::STAMP;
                cmask  = StencilEffect::MASK;
                cfill  = StencilEffect::FILL;
                fall = true;
            case CommandType::EVENODD_FILL:
                if (!fall) {
                    cstamp = StencilEffect::STAMP_NONE;
                    cmask  = StencilEffect::MASK_NONE;
                    cfill  = StencilEffect::FILL_NONE;
                }
                batch->clearStencil();
                fall = true;
            case CommandType::CLIP_FILL:
                if (!fall) {
                    cstamp = StencilEffect::STAMP_CLIP;
                    cmask  = StencilEffect::MASK_CLIP;
                    cfill  = StencilEffect::FILL_CLIP;
                }
                fall = true;
            case CommandType::MASK_FILL:
            {
                if (!fall) {
                    cstamp = StencilEffect::STAMP_MASK;
                    cmask  = StencilEffect::MASK_JOIN;
                    cfill  = StencilEffect::FILL_MASK;
                }

                batch->setStencilEffect(cstamp);
                size_t start = 0;
                for (auto it = comm->mesh.indices.begin(); it != comm->mesh.indices.end()-1; ++it) {
                    batch->drawMesh(comm->mesh.vertices.data()+start,*it-start,transform);
                    start = *it;
                }
                if (comm->border.vertices.size() > 0) {
                    batch->setStencilEffect(cmask);
                    batch->drawMesh(comm->border, transform);
                }
                batch->setStencilEffect(cfill);
                batch->drawMesh(comm->mesh.vertices.data()+start,4,transform);
            }
                break;
            case CommandType::STENCIL_FILL:
            {
                batch->setStencilEffect(NONE_STAMP);
                size_t start = 0;
                for (auto it = comm->mesh.indices.begin(); it != comm->mesh.indices.end()-1; ++it) {
                    batch->drawMesh(comm->mesh.vertices.data()+start,*it-start,transform);
                    start = *it;
                }
                if (comm->border.vertices.size() > 0) {
                    batch->drawMesh(comm->border, transform);
                }
            }
                break;
            case CommandType::NORMAL_STROKE:
                batch->clearStencil();
                cstamp = StencilEffect::CLAMP_NONE;
                cmask  = StencilEffect::MASK_NONE;
                fall = true;
            case CommandType::CLIP_STROKE:
                if (!fall) {
                    cstamp = StencilEffect::CLAMP_CLIP;
                    cmask  = StencilEffect::MASK_CLIP;
                }
                fall = true;
            case CommandType::MASK_STROKE:
            {
                if (!fall) {
                    cstamp = StencilEffect::CLAMP_MASK;
                    cmask  = StencilEffect::MASK_JOIN;
                }

                batch->setStencilEffect(cstamp);
                batch->drawMesh(comm->mesh, transform);
                if (comm->border.vertices.size() > 0) {
                    batch->setStencilEffect(cmask);
                    batch->drawMesh(comm->border, transform);
                }
                batch->clearHalfStencil(true);
                batch->setStencilEffect(StencilEffect::NONE);
            }
                break;
            case CommandType::STENCIL_STROKE:
            {
                batch->setStencilEffect(StencilEffect::NONE_CARVE);
                batch->drawMesh(comm->mesh, transform);
                batch->setStencilEffect(StencilEffect::NONE);
            }
                break;
            case CommandType::NORMAL_TEXT:
                batch->clearStencil();
                batch->setStencilEffect(StencilEffect::NONE);
                fall = true;
            case CommandType::STENCIL_TEXT:
                if (!fall) {
                    batch->setStencilEffect(StencilEffect::NONE_STAMP);
                }
                fall = true;
            case CommandType::CLIP_TEXT:
                if (!fall) {
                    batch->setStencilEffect(StencilEffect::NONE_CLIP);
                }
                fall = true;
            case CommandType::MASK_TEXT:
                if (!fall) {
                    batch->setStencilEffect(StencilEffect::NONE_MASK);
                }
                batch->drawMesh(comm->mesh, transform);
                break;
            case CommandType::TEXT:
            case CommandType::FILL:
            case CommandType::STROKE:
            case CommandType::UNDEFINED:
                // This should not happen
                break;
        }
    }
    
    batch->setGradient(nullptr);
    batch->setTexture(nullptr);
    batch->setStencilEffect(StencilEffect::NATIVE);
    if (changesciss) {
        batch->setScissor(origsciss);
    }
}

#pragma mark -
#pragma mark Render State
/**
 * Pushes and saves the current render state on to a state stack.
 *
 * The state which represents how text and paths will be rendered. It
 * contains local transforms, fill and stroke styles, text and font
 * styles, and scissor clipping regions.
 *
 * Note that state is local to a canvas page. Changing the current
 * {@link #getEditPage} will also change the render state to the one
 * for that page.  This happens without any loss to the state of the
 * original page.
 *
 * After the state is saved, a matching call to {@link #restoreState}
 * must be used to restore the state.
 */
void CanvasNode::saveState() {
    Page* page = _canvas[_edit];
    page->saveContext();
}

/**
 * Pops and restores current render state.
 *
 * The state which represents how text and paths will be rendered. It
 * contains local transforms, fill and stroke styles, text and font
 * styles, and scissor clipping regions.
 *
 * Note that state is local to a canvas page. Changing the current
 * {@link #getEditPage} will also change the render state to the one
 * for that page.  This happens without any loss to the state of the
 * original page.
 *
 * If the state was not previously saved with a call to {@link #saveState},
 * then this method will restore all options to their defaults.
 */
void CanvasNode::restoreState() {
    Page* page = _canvas[_edit];
    page->restoreContext();
}

/**
 * Resets current render state to default values.
 *
 * This option does not affect the render state stack. Any states that
 * were previously saved are preserved.
 *
 * Note that state is local to a canvas page. Changing the current
 * {@link #getEditPage} will also change the render state to the one
 * for that page.  This happens without any loss to the state of the
 * original page.
 */
void CanvasNode::resetState() {
    Page* page = _canvas[_edit];
    page->resetContexts();
}

/**
 * Returns the antialiasing fringe for this canvas node
 *
 * If this value is non-zero, any fill or stroke will be surrounded by
 * a stroke the width of the fringe. The stroke will fade to transparent
 * on the outside edge. This is a way of providing antialiasing that is
 * significantly better than multisampling. Furthermore, this works on
 * OpenGLES, which does not support multisampling.
 *
 * A fringe value should be >= 0.5 to have noticeable effects. In
 * practice, values between 1 and 2 work best. Note that this adds to
 * the volume of the fill or stroke. Hence this value should be taken
 * into account when drawing shapes
 *
 * @return the antialiasing fringe for this canvas node
 */
float CanvasNode::getFringe() const {
    return _canvas[_edit]->getState()->fringe;
}

/**
 * Sets the antialiasing fringe for this canvas node
 *
 * If this value is non-zero, any fill or stroke will be surrounded by
 * a stroke the width of the fringe. The stroke will fade to transparent
 * on the outside edge. This is a way of providing antialiasing that is
 * significantly better than multisampling. Furthermore, this works on
 * OpenGLES, which does not support multisampling.
 *
 * A fringe value should be >= 0.5 to have noticeable effects. In
 * practice, values between 1 and 2 work best. Note that this adds to
 * the volume of the fill or stroke. Hence this value should be taken
 * into account when drawing shapes
 *
 * @param fringe    The antialiasing fringe for this canvas node
 */
void CanvasNode::setFringe(float fringe) {
    _canvas[_edit]->getState()->fringe = fringe;
}

/**
 * Returns the transparency to apply to all rendered shapes.
 *
 * The alpha should be a value 0..1. Already transparent paths will get
 * proportionally more transparent as well.
 *
 * @return the transparency to apply to all rendered shapes.
 */
float CanvasNode::getGlobalAlpha() {
    return _canvas[_edit]->getState()->globalAlpha;
}

/**
 * Sets the transparency to apply to all rendered shapes.
 *
 * The alpha should be a value 0..1. Already transparent paths will get
 * proportionally more transparent as well.
 *
 * @param alpha     The transparency to apply to all rendered shapes.
 */
void CanvasNode::setGlobalAlpha(float alpha) {
    _canvas[_edit]->getState()->globalAlpha = alpha;
}

/**
 * Returns the current command transform
 *
 * Transforms are applied to all paths, text, paints, and scissor regions.
 * They are applied at the time that the are passed to the drawing API.
 * So a translation applied after the first point in a path will skip that
 * initial point, but apply to all subsequent points (until the command
 * transform is changed again).
 *
 * When using {@link Paint} objects, it is important to set a transform
 * before applying them. That is because paint objects are specified in
 * the canvas coordinate system, which is affected by the transform. If
 * a paint object is applied to a shape in a different coordinate space
 * then it can have unexpected effects.
 *
 * The current coordinate system can be saved and restored by using the
 * methods {@link #saveState} and {@link #restoreState}.
 *
 * @return the current command transform
 */
const Affine2& CanvasNode::getCommandTransform() {
    return *(_canvas[_edit]->getState()->transform);
}

/**
 * Sets the current command transform
 *
 * Transforms are applied to all paths, text, paints, and scissor regions.
 * They are applied at the time that the are passed to the drawing API.
 * So a translation applied after the first point in a path will skip that
 * initial point, but apply to all subsequent points (until the command
 * transform is changed again).
 *
 * When using {@link Paint} objects, it is important to set a transform
 * before applying them. That is because paint objects are specified in
 * the canvas coordinate system, which is affected by the transform. If
 * a paint object is applied to a shape in a different coordinate space
 * then it can have unexpected effects.
 *
 * The current coordinate system can be saved and restored by using the
 * methods {@link #saveState} and {@link #restoreState}.
 *
 * @param transform     The current command transform
 */
void CanvasNode::setCommandTransform(const Affine2& transform) {
    std::shared_ptr<Affine2> xform = _canvas[_edit]->getState()->transform;
    if (xform == nullptr) {
        xform = std::make_shared<Affine2>(transform);
        _canvas[_edit]->getState()->transform = xform;
    } else {
        xform->set(transform);
    }
}

/**
 * Resets the command transform to an identity matrix.
 *
 * When this method is called all subsequent calls will be applied in
 * the native coordinate space of the canvas node.
 *
 * For more information on how this transform is applied to commands,
 * see {@link #getCommandTransform}.
 */
void CanvasNode::clearCommandTransform() {
    _canvas[_edit]->getState()->transform = nullptr;
}

/**
 * Translates all commands by the given offset.
 *
 * This translation is cumulative with the existing command transform.
 * It is applied after the existing transform operations.
 *
 * For more information on how this transform is applied to commands,
 * see {@link #getCommandTransform}.
 *
 * @param x     The translation x-offset
 * @param y     The translation y-offset
 */
void CanvasNode::translateCommands(float x, float y) {
    std::shared_ptr<Affine2> xform = _canvas[_edit]->getState()->transform;
    std::shared_ptr<Affine2> trans = std::make_shared<Affine2>(Affine2::createTranslation(x, y));
    if (xform == nullptr) {
        _canvas[_edit]->getState()->transform = trans;
    } else {
        // Premultiplication
        trans->multiply(*xform);
        _canvas[_edit]->getState()->transform = trans;
    }
}

/**
 * Scales all commands by the given factor.
 *
 * This resizing operation is cumulative with the existing command
 * transform. It is applied after the existing transform operations.
 *
 * For more information on how this transform is applied to commands,
 * see {@link #getCommandTransform}.
 *
 * @param sx    The x-axis scaling factor
 * @param sy    The y-axis scaling factor
 */
void CanvasNode::scaleCommands(float sx, float sy)  {
    std::shared_ptr<Affine2> xform = _canvas[_edit]->getState()->transform;
    std::shared_ptr<Affine2> trans = std::make_shared<Affine2>(Affine2::createScale(sx,sy));
    if (xform == nullptr) {
        _canvas[_edit]->getState()->transform = trans;
    } else {
        // Premultiplication
        trans->multiply(*xform);
        _canvas[_edit]->getState()->transform = trans;
    }
}

/**
 * Rotates all commands by the given angle.
 *
 * The angle is specified in radians, and specifies a rotation about
 * the origin. This rotation is cumulative with the existing command
 * transform. It is applied after the existing transform operations.
 *
 * For more information on how this transform is applied to commands,
 * see {@link #getCommandTransform}.
 *
 * @param angle     The rotation angle in radians
 */
void CanvasNode::rotateCommands(float angle) {
    std::shared_ptr<Affine2> xform = _canvas[_edit]->getState()->transform;
    std::shared_ptr<Affine2> trans = std::make_shared<Affine2>(Affine2::createRotation(angle));
    if (xform == nullptr) {
        _canvas[_edit]->getState()->transform = trans;
    } else {
        // Premultiplication
        trans->multiply(*xform);
        _canvas[_edit]->getState()->transform = trans;
    }
}

/**
 * Skews all commands along x-axis.
 *
 * A skew is a shear with the given angle is specified in radians.
 * This shear is cumulative with the existing command transform.
 * It is applied after the existing transform operations.
 *
 * For more information on how this transform is applied to commands,
 * see {@link #getCommandTransform}.
 *
 * @param angle     The skew angle in radians
 */
void CanvasNode::skewXCommands(float angle) {
    std::shared_ptr<Affine2> xform = _canvas[_edit]->getState()->transform;
    std::shared_ptr<Affine2> trans = std::make_shared<Affine2>();
    trans->set(1,0,tanf(angle),1,0,0);
    if (xform == nullptr) {
        _canvas[_edit]->getState()->transform = trans;
    } else {
        // Premultiplication
        trans->multiply(*xform);
        _canvas[_edit]->getState()->transform = trans;
    }
}

/**
 * Skews all commands along y-axis.
 *
 * A skew is a shear with the given angle is specified in radians.
 * This shear is cumulative with the existing command transform.
 * It is applied after the existing transform operations.
 *
 * For more information on how this transform is applied to commands,
 * see {@link #getCommandTransform}.
 *
 * @param angle     The skew angle in radians
 */
void CanvasNode::skewYCommands(float angle) {
    std::shared_ptr<Affine2> xform = _canvas[_edit]->getState()->transform;
    std::shared_ptr<Affine2> trans = std::make_shared<Affine2>();
    trans->set(1,tanf(angle),0,1,0,0);
    if (xform == nullptr) {
        _canvas[_edit]->getState()->transform = trans;
    } else {
        // Premultiplication
        trans->multiply(*xform);
        _canvas[_edit]->getState()->transform = trans;
    }
}

/**
 * Returns the current local scissor
 *
 * The local scissor is applied any subsequent drawing commands, but
 * not to any commands issued before the scissor was applied. This
 * is different from {@link SceneNode#getScissor} which is applied
 * globally to the entire scene graph node. The local scissor is
 * transformed by the {@link #getCommandTransform} at the time it
 * is set.
 *
 * If there is both a local and a global scissor, their rectangles
 * will be intersected to produce a single scissor, using the method
 * {@link Scissor#intersect}. The intersection will take place in
 * the coordinate system of this scissor.
 *
 * @return the current local scissor
 */
const std::shared_ptr<Scissor>& CanvasNode::getLocalScissor() const {
    return _canvas[_edit]->getState()->scissor;
}

/**
 * Sets the current local scissor
 *
 * The local scissor is applied any subsequent drawing commands, but
 * not to any commands issued before the scissor was applied. This
 * is different from {@link SceneNode#getScissor} which is applied
 * globally to the entire scene graph node. The local scissor is
 * transformed by the {@link #getCommandTransform} at the time it
 * is set.
 *
 * If there is both a local and a global scissor, their rectangles
 * will be intersected to produce a single scissor, using the method
 * {@link Scissor#intersect}. The intersection will take place in
 * the coordinate system of this scissor.
 *
 * @param scissor   The current local scissor
 */
void CanvasNode::setLocalScissor(const std::shared_ptr<Scissor>& scissor) {
    Context* state = _canvas[_edit]->getState();
    state->scissor = Scissor::alloc(scissor);
    if (state->transform != nullptr) {
        state->scissor->multiply(*(state->transform));
    }
}

/**
 * Applies the given scissor to the stack.
 *
 * If there is no active local scissor, this method is the same as
 * {@link #setLocalScissor}. Otherwise, this method will generate a
 * new local scissor by calling {@link Scissor#intersect} on the
 * previous local scisor. . The intersection will take place in
 * the coordinate system of this scissor.
 *
 * @param scissor   The local scissor to add
 */
void CanvasNode::applyLocalScissor(const std::shared_ptr<Scissor>& scissor) {
    Context* state = _canvas[_edit]->getState();
    auto transciss = Scissor::alloc(scissor);
    if (state->transform != nullptr) {
        transciss->multiply(*(state->transform));
    }
    if (state->scissor) {
        transciss->intersect(state->scissor);
    }
    state->scissor = transciss;
}

/**
 * Resets and disables scissoring for this canvas.
 *
 * Clearing the local scissor will not reveal any commands previously
 * clipped by the local scissor. In addition, this method has no
 * effect on the global scissor {@link SceneNode#getScissor}.
 */
void CanvasNode::clearLocalScissor() {
    _canvas[_edit]->getState()->scissor = nullptr;
}

#pragma mark -
#pragma mark Path Settings
/**
 * Returns the current winding order
 *
 * As a general rule, solid shapes should have a counter clockwise winding,
 * and holes should have a clockwise winding. This property allows you to
 * specify the winding order to use, even if you generate the path in the
 * wrong order. Hence, if this attribute is CCW, your paths will all be
 * counter clockwise even if the drawing commands generate them clockwise.
 *
 * By default this value is None, which means that paths use their native
 * winding order.
 *
 * The winding order is applied to a subpath when it is committed. A subpath
 * is committed at a subsequent call to {@link #moveTo} or a call to either
 * {@link #fillPaths} or {@link #strokePaths}.
 *
 * @return the current winding order
 */
CanvasNode::Winding CanvasNode::getWinding() const {
    return _canvas[_edit]->getState()->winding;
}

/**
 * Sets the current winding order
 *
 * As a general rule, solid shapes should have a counter clockwise winding,
 * and holes should have a clockwise winding. This setting allows you to
 * specify the winding order to use, even if you generate the path in the
 * wrong order. Hence, if this setting is CCW, your paths will all be
 * counter clockwise even if the drawing commands generate them clockwise.
 *
 * By default this value is None, which means that paths use their native
 * winding order.
 *
 * The winding order is applied to a subpath when it is committed. A subpath
 * is committed at a subsequent call to {@link #moveTo} or a call to either
 * {@link #fillPaths} or {@link #strokePaths}.
 *
 * @param winging   The current winding order
 */
void CanvasNode::setWinding(Winding winding) {
    _canvas[_edit]->getState()->winding = winding;
}

/**
 * Returns the current fill rule
 *
 * This setting is applied at a call to {@link #fillPaths}.
 *
 * By default a canvas node uses a nonzero fill rule, as described here:
 *
 *      https://en.wikipedia.org/wiki/Nonzero-rule
 *
 * This rule allows you to put holes inside a filled path simply by reversing
 * the winding order.
 *
 * Alternate fill rules are supported, though they are all the same as the
 * nonzero rule for simple paths. They only differ when either a path has
 * self-intersections, or two subpaths intersect one another.
 *
 * @return the current fill rule
 */
CanvasNode::FillRule CanvasNode::getFillRule() const {
     return _canvas[_edit]->getState()->fillrule;
}

/**
 * Sets the current fill rule
 *
 * This setting is applied at a call to {@link #fillPaths}.
 *
 * By default a canvas node uses a nonzero fill rule, as described here:
 *
 *      https://en.wikipedia.org/wiki/Nonzero-rule
 *
 * This rule allows you to put holes inside a filled path simply by reversing
 * the winding order.
 *
 * Alternate fill rules are supported, though they are all the same as the
 * nonzero rule for simple paths. They only differ when either a path has
 * self-intersections, or two subpaths intersect one another.
 *
 * @param rule  The current fill rule
 */
void CanvasNode::setFillRule(FillRule rule) {
    _canvas[_edit]->getState()->fillrule = rule;
}

/**
 * Returns the color to use for all filled paths
 *
 * This setting is applied at a call to {@link #fillPaths}.
 *
 * It is possible to combine a color together with a paint. It the
 * attribute {@link #getFillPaint} is not nullptr, it will tinted
 * by this color.
 *
 * This color is also the one that will be used to render text.
 * This value is Color4::WHITE by default.
 *
 * @return the color to use for all filled paths
 */
Color4 CanvasNode::getFillColor() const {
    return _canvas[_edit]->getState()->fillColor;
}

/**
 * Sets the color to use for all filled paths
 *
 * This setting is applied at a call to {@link #fillPaths}.
 *
 * It is possible to combine a color together with a paint. It the
 * attribute {@link #getFillPaint} is not nullptr, it will tinted
 * by this color.
 *
 * This color is also the one that will be used to render text.
 * This value is Color4::WHITE by default.
 *
 * @param color     The color to use for all filled paths
 */
void CanvasNode::setFillColor(Color4 color) {
    _canvas[_edit]->getState()->fillColor = color;
}

/**
 * Returns the paint to use for all filled paths.
 *
 * This setting is applied at a call to {@link #fillPaths}.
 *
 * A {@link Paint} object is a user-friend gradient or texture that
 * uses positional coordinates instead of texture coordinates. The
 * paint will be tinted by the value {@link #getStrokeColor} (which
 * is Color4::WHITE by default).
 *
 * If there is non-trivial {@link #getCommandTransform}, it will be
 * applied to the coordinates in this paint object at the time this
 * method is called.
 *
 * A fill paint will also be applied to text, assuming that it is a
 * gradient paint. Pattern paints cannot be applied to text. If this
 * value is nullptr, then all filled paths will have a solid color.
 *
 * @return the paint to use for all filled paths
 */
const std::shared_ptr<CanvasNode::Paint>& CanvasNode::getFillPaint() const {
    return _canvas[_edit]->getState()->fillPaint;
}

/**
 * Sets the paint to use for all filled paths.
 *
 * This setting is applied at a call to {@link #fillPaths}.
 *
 * A {@link Paint} object is a user-friend gradient or texture that
 * uses positional coordinates instead of texture coordinates. The
 * paint will be tinted by the value {@link #getStrokeColor} (which
 * is Color4::WHITE by default).
 *
 * If there is non-trivial {@link #getCommandTransform}, it will be
 * applied to the coordinates in this paint object at the time this
 * method is called.
 *
 * A fill paint will also be applied to text, assuming that it is a
 * gradient paint. Pattern paints cannot be applied to text. If this
 * value is nullptr, then all filled paths will have a solid color.
 *
 * @param paint     The paint to use for all filled paths
 */
void CanvasNode::setFillPaint(const std::shared_ptr<Paint>& paint) {
    Context* state = _canvas[_edit]->getState();
    state->fillPaint = paint;
    if (state->transform != nullptr) {
        state->fillPaint->setTransform(state->transform->getInverse());
    }
}

/**
 * Returns the color to use for all stroked paths
 *
 * This setting is applied at a call to {@link #strokePaths}.
 *
 * It is possible to combine a color together with a paint. It the
 * attribute {@link #getStrokePaint} is not nullptr, it will tinted
 * by this color.
 *
 * This value is Color4::WHITE by default.
 *
 * @return the color to use for all stroked paths
 */
Color4 CanvasNode::getStrokeColor() const {
    return _canvas[_edit]->getState()->strokeColor;
}

/**
 * Sets the color to use for all stroked paths
 *
 * This setting is applied at a call to {@link #strokePaths}.
 *
 * It is possible to combine a color together with a paint. It the
 * attribute {@link #getStrokePaint} is not nullptr, it will tinted
 * by this color.
 *
 * This value is Color4::WHITE by default.
 *
 * @param color     The color to use for all stroked paths
 */
void CanvasNode::setStrokeColor(Color4 color) {
    _canvas[_edit]->getState()->strokeColor = color;
}

/**
 * Returns the paint to use for all stroked paths.
 *
 * This setting is applied at a call to {@link #strokePaths}.
 *
 * A {@link Paint} object is a user-friend gradient or texture that
 * uses positional coordinates instead of texture coordinates. The
 * paint will be tinted by the value {@link #getStrokeColor} (which
 * is Color4::WHITE by default).
 *
 * If there is non-trivial {@link #getCommandTransform}, it will be
 * applied to the coordinates in this paint object at the time this
 * method is called.
 *
 * If this value is nullptr, then all strokes will have a solid color.
 *
 * @return the paint to use for all stroked paths
 */
const std::shared_ptr<CanvasNode::Paint>& CanvasNode::getStrokePaint() const {
    return _canvas[_edit]->getState()->strokePaint;
}

/**
 * Sets the paint to use for all stroked paths.
 *
 * This setting is applied at a call to {@link #strokePaths}.
 *
 * A {@link Paint} object is a user-friend gradient or texture that
 * uses positional coordinates instead of texture coordinates. The
 * paint will be tinted by the value {@link #getStrokeColor} (which
 * is Color4::WHITE by default).
 *
 * If there is non-trivial {@link #getCommandTransform}, it will be
 * applied to the coordinates in this paint object at the time this
 * method is called.
 *
 * If this value is nullptr, then all strokes will have a solid color.
 *
 * @param paint     The paint to use for all stroked paths
 */
void CanvasNode::setStrokePaint(const std::shared_ptr<Paint>& paint) {
    Context* state = _canvas[_edit]->getState();
    state->strokePaint = paint;
    if (state->transform != nullptr) {
        state->strokePaint->setTransform(state->transform->getInverse());
    }
}

/**
 * Returns the width of the stroke style.
 *
 * This setting is applied at a call to {@link #strokePaths}.
 *
 * If the value of {@link #getFringe} is not zero, this will be subtracted
 * from the stroke width when extruding the path. The default stroke width
 * is 2.
 *
 * @return the width of the stroke style.
 */
float CanvasNode::getStrokeWidth() const {
    return _canvas[_edit]->getState()->strokeWidth;
}

/**
 * Sets the width of the stroke style.
 *
 * This setting is applied at a call to {@link #strokePaths}.
 *
 * If the value of {@link #getFringe} is not zero, this will be subtracted
 * from the stroke width when extruding the path. The default stroke width
 * is 2.
 *
 * @param width     The width of the stroke style.
 */
void CanvasNode::setStrokeWidth(float size) {
    _canvas[_edit]->getState()->strokeWidth = size;
}

/**
 * Returns the mitre limit of the extrusion.
 *
 * This setting is applied at a call to {@link #strokePaths}.
 *
 * The mitre limit sets how "pointy" a mitre joint is allowed to be before
 * the algorithm switches it back to a bevel/square joint. Small angles can
 * have very large mitre offsets that go way off-screen.
 *
 * To determine whether to switch a miter to a bevel, the algorithm will take
 * the two vectors at this joint, normalize them, and then average them. It
 * will multiple the magnitude of that vector by the mitre limit. If that
 * value is less than 1.0, it will switch to a bevel. By default this value
 * is 10.0.
 *
 * @return    the mitre limit for joint calculations
 */
float CanvasNode::getMitreLimit() const {
    return _canvas[_edit]->getState()->mitreLimit;
}

/**
 * Sets the mitre limit of the extrusion.
 *
 * This setting is applied at a call to {@link #strokePaths}.
 *
 * The mitre limit sets how "pointy" a mitre joint is allowed to be before
 * the algorithm switches it back to a bevel/square joint. Small angles can
 * have very large mitre offsets that go way off-screen.
 *
 * To determine whether to switch a miter to a bevel, the algorithm will take
 * the two vectors at this joint, normalize them, and then average them. It
 * will multiple the magnitude of that vector by the mitre limit. If that
 * value is less than 1.0, it will switch to a bevel. By default this value
 * is 10.0.
 *
 * @param limit    The mitre limit for joint calculations
 */
void CanvasNode::setMitreLimit(float limit) {
    _canvas[_edit]->getState()->mitreLimit = limit;
}

/**
 * Returns the joint value for the stroke.
 *
 * The joint type determines how the stroke joins the extruded line segments
 * together.  See {@link poly2::Joint} for the description of the types.
 *
 * @return the joint value for the stroke.
 */
poly2::Joint CanvasNode::getLineJoint() {
    return _canvas[_edit]->getState()->lineJoint;
}

/**
 * Sets the joint value for the stroke.
 *
 * The joint type determines how the stroke joins the extruded line segments
 * together.  See {@link poly2::Joint} for the description of the types.
 *
 * @param joint     The joint value for the stroke.
 */
void CanvasNode::setLineJoint(poly2::Joint joint) {
    _canvas[_edit]->getState()->lineJoint = joint;
}

/**
 * Returns the end cap value for the stroke.
 *
 * This setting is applied at a call to {@link #strokePaths}.
 *
 * The end cap type determines how the stroke draws the ends of the line
 * segments at the start and end of the path. See {@link poly2::EndCap}
 * for the description of the types.
 *
 * @return the end cap value for the extrusion.
 */
poly2::EndCap CanvasNode::getLineCap() {
    return _canvas[_edit]->getState()->lineCap;
}

/**
 * Sets the end cap value for the stroke.
 *
 * This setting is applied at a call to {@link #strokePaths}.
 *
 * The end cap type determines how the stroke draws the ends of the line
 * segments at the start and end of the path. See {@link poly2::EndCap}
 * for the description of the types.
 *
 * @param cap   The end cap value for the stroke.
 */
void CanvasNode::setLineCap(poly2::EndCap cap) {
    _canvas[_edit]->getState()->lineCap = cap;
}

/**
 * Returns the blending equation for this canvas node
 *
 * This setting is applied at the call to either {@link #strokePaths}
 * or {@link #fillPaths}.
 *
 * By default this value is GL_FUNC_ADD. For other options, see
 *
 *      https://www.opengl.org/sdk/docs/man/html/glBlendEquation.xhtml
 *
 * @return the blending equation for this canvas node
 */
GLenum CanvasNode::getBlendEquation() const {
    return _canvas[_edit]->getState()->blendEquation;
}

/**
 * Sets the blending equation for this canvas node.
 *
 * This setting is applied at the call to either {@link #strokePaths}
 * or {@link #fillPaths}.
 *
 * The enum must be a standard ones supported by OpenGL. See
 *
 *      https://www.opengl.org/sdk/docs/man/html/glBlendEquation.xhtml
 *
 * However, this setter does not do any error checking to verify that
 * the input is valid. By default, the equation is GL_FUNC_ADD.
 *
 * @param equation  The blending equation for this canvas node
 */
void CanvasNode::setBlendEquation(GLenum equation) {
    _canvas[_edit]->getState()->blendEquation = equation;
}

/**
 * Sets the blending functions for the source color
 *
 * This setting is applied at the call to either {@link #strokePaths}
 * or {@link #fillPaths}.
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
 * By default both values are GL_SRC_ALPHA, as scene graphs do not
 * use premultiplied alpha.
 *
 * @param rgb       The blend function for the source RGB components
 * @param alpha     The blend function for the source alpha component
 */
void CanvasNode::setSrcBlendFunc(GLenum srcRGB, GLenum srcAlpha) {
    _canvas[_edit]->getState()->blendSrcRGB = srcRGB;
    _canvas[_edit]->getState()->blendSrcAlpha = srcAlpha;
}

/**
 * Returns the source blending function for the RGB components
 *
 * This setting is applied at the call to either {@link #strokePaths}
 * or {@link #fillPaths}.
 *
 * By default this value is GL_SRC_ALPHA, as scene graphs do not
 * use premultiplied alpha. For other options, see
 *
 *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
 *
 * @return the source blending function for the RGB components
 */
GLenum CanvasNode::getSrcRGBFunc() const {
    return _canvas[_edit]->getState()->blendSrcRGB;
}

/**
 * Returns the source blending function for the alpha component
 *
 * This setting is applied at the call to either {@link #strokePaths}
 * or {@link #fillPaths}.
 *
 * By default this value is GL_SRC_ALPHA, as scene graphs do not
 * use premultiplied alpha. For other options, see
 *
 *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
 *
 * @return the source blending function for the alpha component
 */
GLenum CanvasNode::getSrcAlphaFunc() const {
    return _canvas[_edit]->getState()->blendSrcAlpha;
}

/**
 * Sets the blending functions for the destination color
 *
 * This setting is applied at the call to either {@link #strokePaths}
 * or {@link #fillPaths}.
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
 * By default both values are GL_ONE_MINUS_SRC_ALPHA, as scene
 * graphs do not use premultiplied alpha.
 *
 * @param rgb       The blend function for the source RGB components
 * @param alpha     The blend function for the source alpha component
 */
void CanvasNode::setDstBlendFunc(GLenum dstRGB, GLenum dstAlpha) {
    _canvas[_edit]->getState()->blendDstRGB = dstRGB;
    _canvas[_edit]->getState()->blendDstAlpha = dstAlpha;
}

/**
 * Returns the destination blending function for the RGB components
 *
 * This setting is applied at the call to either {@link #strokePaths}
 * or {@link #fillPaths}.
 *
 * By default this value is GL_ONE_MINUS_SRC_ALPHA, as sprite batches
 * do not use premultiplied alpha. For other options, see
 *
 *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
 *
 * @return the destination blending function for the RGB components
 */
GLenum CanvasNode::getDstRGBFunc() const {
    return _canvas[_edit]->getState()->blendDstRGB;
}

/**
 * Returns the destination blending function for the alpha component
 *
 * This setting is applied at the call to either {@link #strokePaths}
 * or {@link #fillPaths}.
 *
 * By default this value is GL_ONE_MINUS_SRC_ALPHA, as sprite batches
 * do not use premultiplied alpha. For other options, see
 *
 *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
 *
 * @return the destination blending function for the alpha component
 */
GLenum CanvasNode::getDstAlphaFunc() const {
    return _canvas[_edit]->getState()->blendDstAlpha;
}


#pragma mark -
#pragma mark Path Commands
/**
 * Clears the current path and sub-paths.
 *
 * This method should be called before drawing a new path. Otherwise the
 * commands will simply append to the existing paths.
 *
 * The standard way to draw paths on a canvas is to first call this method
 * and then call {@link #moveTo} to start the path. To create a subpath
 * (for holes or disjoint polygons) simple call {@link #moveTo} again.
 */
void CanvasNode::beginPath() {
    Page* page = _canvas[_edit];
    page->clearPaths();
     
}

/**
 * Starts a new sub-path with specified point as first point.
 *
 * The command transform is applied to this method when called.
 *
 * @param pos   The initial path point
 */
void CanvasNode::moveTo(const Vec2 pos) {
    Page* page = _canvas[_edit];
    std::shared_ptr<Affine2> xform = page->getState()->transform;
    if (page->active) {
        page->savePath();
    }
    page->active = true;
    if (xform != nullptr) {
        page->spline.addAnchor(pos*(*xform));
    } else {
        page->spline.addAnchor(pos);
    }
}

/**
 * Adds a line segment from the previous point to the given one.
 *
 * If there is no current path, this method creates a new subpath starting
 * at the origin. The command transform is applied to this method when called.
 *
 * @param pos   The next path point
 */
void CanvasNode::lineTo(const Vec2 pos) {
    Page* page = _canvas[_edit];
    std::shared_ptr<Affine2> xform = page->getState()->transform;
    if (!page->active) {
        page->active = true;
        if (xform != nullptr) {
            page->spline.addAnchor(Vec2::ZERO*(*xform));
        } else {
            page->spline.addAnchor(Vec2::ZERO);
        }
    }
    if (xform != nullptr) {
        page->spline.addAnchor(pos*(*xform));
    } else {
        page->spline.addAnchor(pos);
    }
}

/**
 * Adds a cubic bezier segment from the previous point.
 *
 * The control points specify the tangents as described in {@link Spline2}.
 *
 * If there is no current path, this method creates a new subpath starting
 * at the origin. The command transform is applied to this method when called.
 *
 * @param c1    The first control point
 * @param c2    The second control point
 * @param p     The end of the bezier segment
 */
void CanvasNode::bezierTo(const Vec2 c1, const Vec2 c2, const Vec2 p) {
    Page* page = _canvas[_edit];
    std::shared_ptr<Affine2> xform = page->getState()->transform;
    if (!page->active) {
        page->active = true;
        if (xform != nullptr) {
            page->spline.addAnchor(Vec2::ZERO*(*xform));
        } else {
            page->spline.addAnchor(Vec2::ZERO);
        }
    }
    if (xform != nullptr) {
        page->spline.addBezier(c1*(*xform), c2*(*xform), p*(*xform));
    } else {
        page->spline.addBezier(c1, c2, p);
    }
}

/**
 * Adds a quadratic bezier segment from the previous point.
 *
 * The control point is as described in {@link Spline2#addQuad}.
 *
 * If there is no current path, this method creates a new subpath starting
 * at the origin. The command transform is applied to this method when called.
 *
 * @param c     The control point
 * @param p     The end of the bezier segment
 */
void CanvasNode::quadTo(const Vec2 c, const Vec2 p) {
    Page* page = _canvas[_edit];
    std::shared_ptr<Affine2> xform = page->getState()->transform;
    if (!page->active) {
        page->active = true;
        if (xform != nullptr) {
            page->spline.addAnchor(Vec2::ZERO*(*xform));
        } else {
            page->spline.addAnchor(Vec2::ZERO);
        }
    }
    if (xform != nullptr) {
        page->spline.addQuad(c*(*xform), p*(*xform));
    } else {
        page->spline.addQuad(c, p);
    }
}

/**
 * Adds an arc segment sweeping from angle a0 to a1.
 *
 * The arc center is at is at c and has radius is r. The method will draw
 * a straight line from the previous point in the path to the point at
 * angle a0.  It will then sweep the arc from angle a0 to a1. The value
 * ccw determines whether the arc sweeps counter clockwise or clockwise,
 * as it is not necessarily possible to tell from the angles themselves.
 *
 * If there is no current path, this method creates a new subpath starting
 * at the point for a0. Note that this differs from other drawing commands
 * that would start a new path at the origin. The command transform is
 * applied to this method when called.
 *
 * @param center    The arc center
 * @param r         The arc radius
 * @param a0        The starting angle of the arc
 * @param a1        The ending angle of the arc
 * @param ccw       Whether draw the arc counter clockwise
 */
void CanvasNode::arcTo(const Vec2 center, float r, float a0, float a1, bool ccw) {
    Page* page = _canvas[_edit];

    // Clamp angles
    float da = a1 - a0;
    if (ccw) {
        if (fabsf(da) >= M_PI*2) {
            da = M_PI*2;
        } else {
            while (da < 0.0f) {
                da += M_PI*2;
            }
        }
    } else {
        if (fabsf(da) >= M_PI*2) {
            da = -M_PI*2;
        } else {
            while (da > 0.0f) {
                da -= M_PI*2;
            }
        }
    }

    // Split arc into max 90 degree segments.
    int ndivs = std::max(1, std::min((int)(fabsf(da) / (M_PI*0.5f) + 0.5f), 5));
    float hda = (da / (float)ndivs) / 2.0f;
    float kappa = fabsf(4.0f / 3.0f * (1.0f - cosf(hda)) / sinf(hda));
    if (!ccw) {
        kappa = -kappa;
    }
    
    Vec2 c, p;
    Vec2 ct, pt;
    for (int ii = 0; ii <= ndivs; ii++) {
        float a = a0 + da * (ii/(float)ndivs);
        ct.set(cosf(a),sinf(a));
        c = center + ct*r;
        ct.perp();
        ct *= r*kappa;

        if (ii == 0) {
            if (!page->active) {
                moveTo(c);
            } else {
                lineTo(c);
            }
        } else {
            bezierTo(p+pt, c-ct, c);
        }
        p = c;
        pt = ct;
    }
}

/**
 * Adds an arc segment whose corner is defined by the previous point.
 *
 * The previous point acts as the center for the arc, which is drawn through
 * the two points with the given radius.
 *
 * If there is no current path, this method creates a new subpath starting
 * at point s. Note that this differs from other drawing commands that would
 * start a new path at the origin. The command transform is applied to this
 * method when called.
 *
 * @param s         The start of the arc
 * @param e         The end of the arc
 * @param radius    The arc radius
 */
void CanvasNode::arcTo(const Vec2 s, const Vec2 e, float radius) {
    Page* page = _canvas[_edit];
    std::shared_ptr<Affine2> xform = page->getState()->transform;
    if (!page->active) {
        page->active = true;
        if (xform != nullptr) {
            page->spline.addAnchor(s*(*xform));
        } else {
            page->spline.addAnchor(s);
        }
    }

    // Get the last point
    Vec2 prev = page->spline.getControlPoints().back();
    if (prev.equals(s, MIN_TOLERANCE) || prev.equals(s, MIN_TOLERANCE) ||
        s.distanceSegmentSquared(prev,e) < MIN_TOLERANCE*MIN_TOLERANCE ||
        radius < MIN_TOLERANCE) {
        lineTo(s);
        return;
    }
    
    // Calculate tangential circle
    Vec2 d0 = prev-s;
    Vec2 d1 = e-s;
    d0.normalize();
    d1.normalize();
    float a = acosf(d0.dot(d1));
    float d = radius/tanf(a/2.0f);
    
    if (d > MAX_TOLERANCE) {
        lineTo(s);
        return;
    }
    
    float a0, a1;
    Vec2 c = s+d*d0;
    bool ccw;
    if (d0.cross(d1) < 0.0f) {
        c.x += d0.y*radius;
        c.y -= d0.x*radius;
        a0 = atan2f(d0.x, -d0.y);
        a1 = atan2f(-d1.x, d1.y);
        ccw = false;
    } else {
        c.x -= d0.y*radius;
        c.y += d0.x*radius;
        a0 = atan2f(-d0.x, d0.y);
        a1 = atan2f(d1.x, -d1.y);
        ccw = true;
    }

    arcTo(c, radius, a0, a1, !ccw);
}

/**
 * Closes the current subpath with a line segment.
 *
 * While this method closes the subpath, it does **not** start a new subpath.
 * You will need to call {@link #moveTo} to do that.
 */
void CanvasNode::closePath() {
    Page* page = _canvas[_edit];
    if (page->active) {
        if (page->spline.size() > 0) {
            page->spline.setClosed(true);
            page->savePath();
        }
        page->active = false;
    }
}
    
/**
 * Creates a new circle arc subpath, sweeping from angle a0 to a1.
 *
 * The arc center is at is at c and has radius is r. The new subpath will
 * start at the point corresponding to angle a0. The value ccw determines
 * whether the arc sweeps counter clockwise or clockwise, as it is not
 * necessarily possible to tell this from the angles themselves.
 *
 * The command transform is applied to this method when called.
 *
 * @param center    The arc center
 * @param r         The arc radius
 * @param a0        The starting angle of the arc
 * @param a1        The ending angle of the arc
 * @param ccw       Whether draw the arc counter clockwise
 */
void CanvasNode::drawArc(const Vec2 center, float r, float a0, float a1, bool ccw) {
    Page* page = _canvas[_edit];
    page->savePath();
    arcTo(center, r, a0, a1, ccw);
}

/**
 * Creates a new rectangle shaped subpath
 *
 * The command transform is applied to this method when called.
 *
 * @param rect  The rectangle bounds
 */
void CanvasNode::drawRect(const Rect rect) {
    Page* page = _canvas[_edit];
    page->savePath();
    std::shared_ptr<Affine2> xform = page->getState()->transform;
    
    page->paths.push_back(new Path2());
    Path2* path = page->paths.back();
    if (xform != nullptr) {
        Vec2 pos = rect.origin;
        path->push(pos*(*xform),true);
        pos.x += rect.size.width;
        path->push(pos*(*xform),true);
        pos.y += rect.size.height;
        path->push(pos*(*xform),true);
        pos.x -= rect.size.width;
        path->push(pos*(*xform),true);
    } else {
        path->set(rect);
    }
    path->closed = true;
    page->orientLastPath();
}

/**
 * Creates a new rounded rectangle shaped subpath.
 *
 * To be well-defined, the corner radius should be no larger than half
 * the width and height (at which point it defines a capsule). Shapes
 * with abnormally large radii are undefined.
 *
 * The command transform is applied to this method when called.
 *
 * @param x     The x-coordinate of the rectangle origin
 * @param y     The y-coordinate of the rectangle origin
 * @param w     The rectangle width
 * @param h     The rectangle height
 * @param r     The corner radius
 */
void CanvasNode::drawRoundedRect(float x, float y, float w, float h, float r) {
    if (r < MIN_TOLERANCE) {
        drawRect(x, y, w, h);
        return;
    }
    
    Uint32 segments = curveSegs(r, (float)M_PI/2.0f, MIN_TOLERANCE);
    const float coef = M_PI/(2.0f*segments);

    Page* page = _canvas[_edit];
    std::shared_ptr<Affine2> xform = page->getState()->transform;
    page->savePath();
    
    page->paths.push_back(new Path2());
    Path2* path = page->paths.back();
    path->reserve(4*segments+4);
    
    // TOP RIGHT
    float cx = x + (w >= 0 ? w : 0) - r;
    float cy = y + (h >= 0 ? h : 0) - r;
    Vec2 vert;
    if (xform != nullptr) {
        for(int ii = 0; ii <= segments; ii++) {
            vert.x = r * cosf(ii*coef) + cx;
            vert.y = r * sinf(ii*coef) + cy;
            path->push(vert*(*xform),false);
        }
    } else {
        for(int ii = 0; ii <= segments; ii++) {
            vert.x = r * cosf(ii*coef) + cx;
            vert.y = r * sinf(ii*coef) + cy;
            path->push(vert,false);
        }
    }
    
    // TOP LEFT
    cx = x + (w >= 0 ? 0 : w) + r;
    cy = y + (h >= 0 ? h : 0) - r;
    if (xform != nullptr) {
        for(int ii = 0; ii <= segments; ii++) {
            vert.x = cx - r * sinf(ii*coef);
            vert.y = r * cosf(ii*coef) + cy;
            path->push(vert*(*xform),false);
        }
    } else {
        for(int ii = 0; ii <= segments; ii++) {
            vert.x = cx - r * sinf(ii*coef);
            vert.y = r * cosf(ii*coef) + cy;
            path->push(vert,false);
        }
    }
    
    // BOTTOM LEFT
    cx = x + (w >= 0 ? 0 : w) + r;
    cy = y + (h >= 0 ? 0 : h) + r;
    if (xform != nullptr) {
        for(int ii = 0; ii <= segments; ii++) {
            vert.x = cx - r * cosf(ii*coef);
            vert.y = cy - r * sinf(ii*coef);
            path->push(vert*(*xform),false);
        }
    } else {
        for(int ii = 0; ii <= segments; ii++) {
            vert.x = cx - r * cosf(ii*coef);
            vert.y = cy - r * sinf(ii*coef);
            path->push(vert,false);
        }
    }

    // BOTTOM RIGHT
    cx = x + (w >= 0 ? w : 0) - r;
    cy = y + (h >= 0 ? 0 : h) + r;
    if (xform != nullptr) {
        for(int ii = 0; ii <= segments; ii++) {
            vert.x = r * sinf(ii*coef) + cx;
            vert.y = cy - r * cosf(ii*coef);
            path->push(vert*(*xform),false);
        }
    } else {
        for(int ii = 0; ii <= segments; ii++) {
            vert.x = r * sinf(ii*coef) + cx;
            vert.y = cy - r * cosf(ii*coef);
            path->push(vert,false);
        }
    }
    
    path->closed = true;
    page->orientLastPath();
}

/**
 * Creates a new rounded rectangle shaped subpath with varying radii.
 *
 * Each corner will have its own radii. But to be well-defined, none of
 * the radii should be larger than half the width and height (at which
 * point it defines a capsule). Shapes with abnormally large radii are
 * undefined.
 *
 * The command transform is applied to this method when called.
 *
 * @param x     The x-coordinate of the rectangle origin
 * @param y     The y-coordinate of the rectangle origin
 * @param w     The rectangle width
 * @param h     The rectangle height
 * @param radBL The corner radius of the bottom left
 * @param radTL The corner radius of the top left
 * @param radTR The corner radius of the top right
 * @param radBR The corner radius of the bottom right
 */
void CanvasNode::drawRoundedRectVarying(float x, float y, float w, float h,
                                        float radTL, float radTR, float radBR, float radBL) {
    if(radTL < MIN_TOLERANCE && radTR < MIN_TOLERANCE && radBR < MIN_TOLERANCE && radBL < MIN_TOLERANCE) {
        drawRect(x, y, w, h);
        return;
    }

    Page* page = _canvas[_edit];
    std::shared_ptr<Affine2> xform = page->getState()->transform;
    page->savePath();
    
    page->paths.push_back(new Path2());
    Path2* path = page->paths.back();

    Uint32 seg;
    float coef;
    Vec2 c, vert;
    
    // TOP RIGHT
    seg = curveSegs(radTR, (float)M_PI/2.0f, MIN_TOLERANCE);
    coef = M_PI/(2.0f*seg);
    c.x = x + (w >= 0 ? w : 0) - radTR;
    c.y = y + (h >= 0 ? h : 0) - radTR;
    path->reserve(seg+1);
    if (xform != nullptr) {
        for(int ii = 0; ii <= seg; ii++) {
            vert.x = radTR * cosf(ii*coef) + c.x;
            vert.y = radTR * sinf(ii*coef) + c.y;
            path->push(vert*(*xform),false);
        }
    } else {
        for(int ii = 0; ii <= seg; ii++) {
            vert.x = radTR * cosf(ii*coef) + c.x;
            vert.y = radTR * sinf(ii*coef) + c.y;
            path->push(vert,false);
        }
    }
    
    // TOP LEFT
    seg = curveSegs(radTL, (float)M_PI/2.0f, MIN_TOLERANCE);
    coef = M_PI/(2.0f*seg);
    c.x = x + (w >= 0 ? 0 : w) - radTL;
    c.y = y + (h >= 0 ? h : 0) - radTL;
    if (xform != nullptr) {
        for(int ii = 0; ii <= seg; ii++) {
            vert.x = c.x - radTL * sinf(ii*coef);
            vert.y = radTL * cosf(ii*coef) + c.y;
            path->push(vert*(*xform),false);
        }
    } else {
        for(int ii = 0; ii <= seg; ii++) {
            vert.x = c.x - radTL * sinf(ii*coef);
            vert.y = radTL * cosf(ii*coef) + c.y;
            path->push(vert,false);
        }
    }
    
    // BOTTOM LEFT
    seg = curveSegs(radBL, (float)M_PI/2.0f, MIN_TOLERANCE);
    coef = M_PI/(2.0f*seg);
    c.x = x + (w >= 0 ? 0 : w) - radBL;
    c.y = y + (h >= 0 ? 0 : h) - radBL;
    if (xform != nullptr) {
        for(int ii = 0; ii <= seg; ii++) {
            vert.x = c.x - radBL * cosf(ii*coef);
            vert.y = c.y - radBL * sinf(ii*coef);
            path->push(vert*(*xform),false);
        }
    } else {
        for(int ii = 0; ii <= seg; ii++) {
            vert.x = c.x - radBL * cosf(ii*coef);
            vert.y = c.y - radBL * sinf(ii*coef);
            path->push(vert,false);
        }
    }

    // BOTTOM RIGHT
    seg = curveSegs(radBR, (float)M_PI/2.0f, MIN_TOLERANCE);
    coef = M_PI/(2.0f*seg);
    c.x = x + (w >= 0 ? w : 0) - radBR;
    c.y = y + (h >= 0 ? 0 : h) - radBR;
    if (xform != nullptr) {
        for(int ii = 0; ii <= seg; ii++) {
            vert.x = radBR * sinf(ii*coef) + c.x;
            vert.y = c.y - radBR * cosf(ii*coef);
            path->push(vert*(*xform),false);
        }
    } else {
        for(int ii = 0; ii <= seg; ii++) {
            vert.x = radBR * sinf(ii*coef) + c.x;
            vert.y = c.y - radBR * cosf(ii*coef);
            path->push(vert,false);
        }
    }
    
    path->closed = true;
    page->orientLastPath();
}

/**
 * Creates a new ellipse shaped subpath.
 *
 * Note that the bounding rectangle defines the bottom left corner of
 * the ellipse and not the center.
 *
 * The command transform is applied to this method when called.
 *
 * @param bounds    The rectangle bounding this ellipse
 */
void CanvasNode::drawEllipse(const Rect bounds) {
    Size radius = bounds.size/2;
    Vec2 center = bounds.origin+radius;
    drawEllipse(center.x, center.y, radius.width, radius.height);
}

/**
 * Creates a new ellipse shaped subpath.
 *
 * The command transform is applied to this method when called.
 *
 * @param cx        The x-coordinate of the ellipse center
 * @param cy        The y-coordinate of the ellipse center
 * @param rx        The radius along the x-axis
 * @param ry        The radius along the y-axis
 */
void CanvasNode::drawEllipse(float cx, float cy, float rx, float ry) {
    Page* page = _canvas[_edit];
    std::shared_ptr<Affine2> xform = page->getState()->transform;
    page->savePath();
    
    page->paths.push_back(new Path2());
    Path2* path = page->paths.back();

    Uint32 segments = curveSegs(std::max(rx/2.0f,ry/2.0f), 2.0f * (float)M_PI, MIN_TOLERANCE);
    const float coef = 2.0f * (float)M_PI/segments;

    Vec2 vert;
    path->reserve(segments);
    if (xform != nullptr) {
        for(unsigned int ii = 0; ii < segments; ii++) {
            float rads = ii*coef;
            vert.x = rx * cosf(rads) + cx;
            vert.y = ry * sinf(rads) + cy;
            path->push(vert*(*xform),false);
        }
    } else {
        for(unsigned int ii = 0; ii < segments; ii++) {
            float rads = ii*coef;
            vert.x = rx * cosf(rads) + cx;
            vert.y = ry * sinf(rads) + cy;
            path->push(vert,false);
        }
    }
    path->closed = true;
    page->orientLastPath();
}

/**
 * Creates a new circle shaped sub-path.
 *
 * @param cx        The x-coordinate of the circle center
 * @param cy        The y-coordinate of the circle center
 * @param r         The circle radius
 */
void CanvasNode::drawCircle(float cx, float cy, float r) {
    drawEllipse(cx, cy, r, r);
}
  
/**
 * Fills the current path (and subpaths) with the current fill style.
 *
 * This method will commit the any outstanding paths, but it will not
 * clear them.  You should call {@link #beginPath} to start a new path
 * sequence.
 */
void CanvasNode::fillPaths() {
    Page* page = _canvas[_edit];
    page->savePath();
    page->materialize(CommandType::FILL);
}

/**
 * Extrudes the current path (and subpaths) with the current stroke style.
 *
 * This method will commit the any outstanding paths, but it will not
 * clear them.  You should call {@link #beginPath} to start a new path
 * sequence.
 */
void CanvasNode::strokePaths() {
    Page* page = _canvas[_edit];
    page->savePath();
    page->materialize(CommandType::STROKE);
}

#pragma mark -
#pragma mark Text Commands
/**
 * Returns the font for the current text style
 *
 * This is the font that will be used as a call to either {@link #drawText}
 * or {@link #drawTextBox}. If there is no active font when one of the those
 * methods are called, they will fail.
 *
 * @return the font for the current text style
 */
const std::shared_ptr<Font>& CanvasNode::getFont() const {
    Page* page = _canvas[_edit];
    return page->getState()->fontFace;
}

/**
 * Sets the font for the current text style
 *
 * This is the font that will be used as a call to either {@link #drawText}
 * or {@link #drawTextBox}. If there is no active font when one of the those
 * methods are called, they will fail.
 *
 * @param font  The font for the current text style
 */
void CanvasNode::setFont(const std::shared_ptr<Font>& font) {
    Page* page = _canvas[_edit];
    page->getState()->fontFace = font;

}
    
/**
 * Returns the font size of the current text style.
 *
 * By default, the text style will use the point size of {@link #getFont}.
 * However, it is possible to scale the font to get a smaller (or larger)
 * text size. With that said, it is generally better to scale down a
 * font than to scale it up.
 *
 * If this value is 0, the canvas will use the point size of the active
 * font. This value is 0 by default.
 *
 * @return the font size of the current text style.
 */
float CanvasNode::getFontSize() const {
    Page* page = _canvas[_edit];
    return page->getState()->fontSize;
}

/**
 * Sets the font size of the current text style.
 *
 * By default, the text style will use the point size of {@link #getFont}.
 * However, it is possible to scale the font to get a smaller (or larger)
 * text size. With that said, it is generally better to scale down a
 * font than to scale it up.
 *
 * If this value is 0, the canvas will use the point size of the active
 * font. This value is 0 by default.
 *
 * @param size  The font size of the current text style.
 */
void CanvasNode::setFontSize(float size) {
    Page* page = _canvas[_edit];
    page->getState()->fontSize = size;
}

/**
 * Returns the blur radius of the current text style.
 *
 * When blurring text, use a font with the same {@link Font#getPadding}
 * as the blur size. This will prevent bleeding across characters in
 * the atlas.
 *
 * @return the blur radius of the current text style.
 */
float CanvasNode::getFontBlur() const {
    Page* page = _canvas[_edit];
    return page->getState()->fontBlur;
}

/**
 * Sets the blur radius of the current text style.
 *
 * When blurring text, use a font with the same {@link Font#getPadding}
 * as the blur size. This will prevent bleeding across characters in
 * the atlas.
 *
 * @param blur  The blur radius of the current text style.
 */
void CanvasNode::setFontBlur(float blur) {
    Page* page = _canvas[_edit];
    page->getState()->fontBlur = blur;
}

/**
 * Returns the line spacing of the current text style.
 *
 * This value is multiplied by the font size to determine the space
 * between lines. So a value of 1 is single-spaced text, while a value
 * of 2 is double spaced. The value should be positive.
 *
 * @return the line spacing of the current text style.
 */
float CanvasNode::getTextSpacing() const {
    Page* page = _canvas[_edit];
    return page->getState()->fontSpacing;
}

/**
 * Sets the line spacing of the current text style.
 *
 * This value is multiplied by the font size to determine the space
 * between lines. So a value of 1 is single-spaced text, while a value
 * of 2 is double spaced. The value should be positive.
 *
 * @param spacing   The line spacing of the current text style.
 */
void CanvasNode::setTextSpacing(float spacing) {
    Page* page = _canvas[_edit];
    page->getState()->fontSpacing = spacing;
    
    if (page->layout.validated()) {
        page->layout.setSpacing(spacing);
    }
}

/**
 * Returns the horizontal alignment of the text.
 *
 * The horizontal alignment has two meanings. First, it is the relationship
 * of the relative alignment of multiple lines. In addition, it defines the
 * x-coordinate origin of the text. The later is relevant even when the text
 * layout is a single line.
 *
 * See {@link HorizontalAlign} for how alignment affects the text origin.
 *
 * @return the horizontal alignment of the text.
 */
HorizontalAlign CanvasNode::getHorizontalTextAlign() const {
    Page* page = _canvas[_edit];
    return page->getState()->fontHAlign;
}

/**
 * Sets the horizontal alignment of the text.
 *
 * The horizontal alignment has two meanings. First, it is the relationship
 * of the relative alignment of multiple lines. In addition, it defines the
 * x-coordinate origin of the text. The later is relevant even when the text
 * layout is a single line.
 *
 * See {@link HorizontalAlign} for how alignment affects the text origin.
 *
 * @param align     The horizontal alignment of the text.
 */
void CanvasNode::setHorizontalTextAlign(HorizontalAlign align) {
    Page* page = _canvas[_edit];
    page->getState()->fontHAlign = align;
    
    if (page->layout.validated()) {
        page->layout.setHorizontalAlignment(align);
    }
}

/**
 * Returns the vertical alignment of the text.
 *
 * The vertical alignment defines the y-coordinate origin of this text layout.
 * In the case of multiple lines, the alignment is (often) with respect to the
 * entire block of text, not just the first line.
 *
 * See {@link VerticalAlign} for how alignment affects the text origin.
 *
 * @return the vertical alignment of the text.
 */
VerticalAlign CanvasNode::getVerticalTextAlign() const {
    Page* page = _canvas[_edit];
    return page->getState()->fontVAlign;
}

/**
 * Sets the vertical alignment of the text.
 *
 * The vertical alignment defines the y-coordinate origin of this text layout.
 * In the case of multiple lines, the alignment is (often) with respect to the
 * entire block of text, not just the first line.
 *
 * See {@link VerticalAlign} for how alignment affects the text origin.
 *
 * @param align     The vertical alignment of the text.
 */
void CanvasNode::setVerticalTextAlign(VerticalAlign align) {
    Page* page = _canvas[_edit];
    page->getState()->fontVAlign = align;
    
    if (page->layout.validated()) {
        page->layout.setVerticalAlignment(align);
    }
}

/**
 * Draws the text string at specified location.
 *
 * Position (x,y) is the location of the text origin, which is defined by both
 * {@link #getHorizontalTextAlign} and {@link #getVerticalTextAlign}. This
 * command is subject to the current command transform.

 * This command will use the current text style, and color the glyphs with
 * the current fill color. If there is a fill {@link Paint} then it will
 * also be applied, provided that it is a gradient (text cannot be textured
 * with images).
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically.
 *
 * @param x     The x-coordinate of the text origin
 * @param y     The y-coordinate of the text origin
 * @param text  The text to display
 */
void CanvasNode::drawText(float x, float y, const char* substr, const char* end) {
    Page* page = _canvas[_edit];
    Context* state = page->getState();
    CUAssertLog(state->fontFace, "Attempting to draw text without a font.");

    page->textorigin.set(x,y);
    page->layout.initWithText(std::string(substr,end), state->fontFace);
    page->layout.setSpacing(state->fontSpacing);
    page->layout.setHorizontalAlignment(state->fontHAlign);
    page->layout.setVerticalAlignment(state->fontVAlign);
    page->layout.layout();
    page->materialize(CommandType::TEXT);
}

/**
 * Draws a multiline text string at specified location.
 *
 * Position (x,y) is the location of the text origin, which is defined by both
 * {@link #getHorizontalTextAlign} and {@link #getVerticalTextAlign}. This
 * command is subject to the current command transform.
 *
 * When breaking up lines, whitespace at the beginning and end of each
 * line will be "swallowed", causing it to be ignored for purposes of
 * alignment. The exception is at the beginning and end of a paragraph.
 * Whitespace there will be preserved. A paragraph is defined as any
 * piece of text separated by a newline. So the first part of the string
 * before a newline is a paragraph, and each substring after a newline
 * is also a paragraph.
 *
 * Words longer than the max width are slit at nearest character. There
 * is no support for hyphenation.
 *
 * This command will use the current text style, and color the glyphs with
 * the current fill color. If there is a fill {@link Paint} then it will
 * also be applied, provided that it is a gradient (text cannot be textured
 * with images).
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically.
 *
 * @param x         The x-coordinate of the text origin
 * @param y         The y-coordinate of the text origin
 * @param width     The text-wrap width
 * @param text      The text to display
 */
void CanvasNode::drawTextBox(float x, float y, float width, const char* substr, const char* end) {
    Page* page = _canvas[_edit];
    Context* state = page->getState();
    CUAssertLog(state->fontFace, "Attempting to draw text without a font.");
    
    // Look at the scale
    float scale = 1.0f;
    if (state->fontSize > 0 && state->fontSize != state->fontFace->getPointSize()) {
        scale = state->fontSize/state->fontFace->getPointSize();
    }
    
    page->textorigin.set(x,y);
    page->layout.initWithTextWidth(std::string(substr,end), state->fontFace, width/scale);
    page->layout.setSpacing(state->fontSpacing);
    page->layout.setHorizontalAlignment(state->fontHAlign);
    page->layout.setVerticalAlignment(state->fontVAlign);
    page->layout.layout();
    page->materialize(CommandType::TEXT);
}

