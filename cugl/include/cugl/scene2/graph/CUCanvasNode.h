//
//  CUCanvasNode.h
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
#ifndef __CU_CANVAS_NODE_H__
#define __CU_CANVAS_NODE_H__
#include <cugl/scene2/graph/CUSceneNode.h>
#include <cugl/render/CUTextAlignment.h>

namespace cugl {
/** Forward reference for fonts */
class Font;

    namespace scene2 {
/**
 * This is a scene graph node to support scalable vector graphics
 *
 * **WARNING**: This is a highly experimental class. This class is the
 * foundation for SVG support in the future roadmap, but it still needs
 * significant testing. Use this class at your own risk.
 *
 * A canvas node is a drawing slate, not unlike the classic drawing
 * Turtle found in most programming languages. The programmer issues
 * a sequence of drawing commands, and these commands produce an image
 * on the screen. The commands are stored so that the image is shown
 * every animation frame. However, the programmer can change or erase
 * the drawing commands at any time, thus creating arbitrary animation
 * effects.
 *
 * There are actually two ways that a programmer can use a canvas node
 * to animate the image. One way is to erase and reissue the drawing
 * commands every animation frame. But the other way to make use of
 * pages. A canvas can have any number of pages. At any given time,
 * {@link #getEditPage} is the page that receives the drawing commands
 * while {@link #getDrawPage} is the page whose drawing commands are
 * shown on the screen. This allows the user to save multiple drawings
 * and then switch between them, just as {@link SpriteNode} does for
 * sprite sheets.
 *
 * When drawing to a canvas node, it is often useful to think of units
 * as pixels. Indeed, if the canvas node is the size of the display and
 * anchored in the bottom left corner, this is indeed the case. However,
 * a canvas node can itself be transformed just like any scene graph
 * node, including scaling and rotating.
 *
 * It is important to stress that this node is designed for complex
 * **noninteractive** graphics. This node does not remember the format
 * or geometry of any shape or object drawn. This is particularly true
 * for text, which is immediately rendered to a mesh, with all glyph
 * information lost. If the user needs to interact with part of the
 * image, you should use a dedicated-purpose scene graph node for that
 * element, such as {@link PolygonNode} or {@link Label}.
 *
 * The API for this node is designed to be similar to the SVG API,
 * allowing this class to render some subset of SVG files. However,
 * there are some important differences. Most notably, the origin of
 * this scene graph node is, as is the case for all scene graph nodes,
 * in the bottom left corner. This is different from an SVG file where
 * the origin is in the top left and the y-axis increases downwards. A
 * conversion between these two formats is currently in development
 * and will be released at a later date.
 *
 * Much of the code for canvas nodes is heavily inspired by the nanovg
 * framework, developed by Mikko Mononen (memon@inside.org). However,
 * there are numerous optimizations and changes to make it compatible
 * with the CUGL scene graph architecture.
 */
class CanvasNode : public SceneNode {
protected:
    /**
     * A drawing command to send to the {@link SpriteBatch}
     *
     * Each command encapsulates a single mesh gradient/texture pairing
     * to send to the {@link SpriteBatch}. A command represents a drawing
     * that has been completed and saved.
     */
    class Command;
    
    /**
     * A single instance of the canvas state.
     *
     * Like most traditional canvases, all drawing options such as
     * color, transform, or scissor can be saved on stack for use again
     * later. This class represents the elements of that stack.
     */
    class Context;

    /**
     * A single drawing canvas page
     *
     * In order to facilitate animation, this class can have multiple
     * active pages. This class represents a single canvas. It stores
     * both the drawing state (as a stack of {@link Context} objects)
     * and the render state (as a sequence of {@link Command} objects).
     */
    class Page;
    
#pragma mark -
#pragma mark Paints
public:
    /**
     * A combination gradient/texture for painting on the canvas.
     *
     * Traditional {@link Gradient} and {@link Texture} objects use
     * classic texture coordinates, making them somewhat difficult to
     * use. A paint is a more user-friendly approach that allows you
     * to define these elements using positional coordinates. This
     * makes it easier to align gradients or images with the shapes
     * that are drawn.
     *
     * For example, suppose a paint is a linear gradient that starts
     * at the origin and ends at position (100,100). If applied to a
     * rectangle at the origin with size 100x100, this gradient will
     * span the entire rectangle, running along the diagonal. However,
     * if it is applied to a rectangle of size 200x200, it will stop
     * in the center.
     *
     * An important limitation of the canvas node architecture is that
     * any shape can either colored by a gradient or a texture, but not
     * both. You should use a special purpose scene graph node such as
     * {@link PolygonNode} if you need both at the same time.
     */
    class Paint {
    private:
        /**
         * An enum representing the paint type
         *
         * The purpose of this enum is to convert a paint into a proper
         * {@link Gradient} or {@link Texture} object when generating
         * a drawing command.
         */
        enum Type {
            /** The paint type is unknown or undefined */
            UNKNOWN,
            /** The paint type is a linear gradient */
            LINEAR,
            /** The paint type is a radial gradient */
            RADIAL,
            /** The paint type is a box gradient */
            BOX,
            /** The paint type is a texture pattern */
            PATTERN
        };

        
        /** The paint type (gradient or texture) */
        Type    _type;
        /** The inner color of this gradient (gradient paints only) */
        Color4  _inner;
        /** The outer color of this gradient (gradient paints only) */
        Color4  _outer;
        /** The gradient start position (gradient paints only) */
        Vec2    _start;
        /** The gradient extent (gradient paints only) */
        Vec2    _extent;
        /** A type-specific factor (gradient paints only) */
        float   _factor1;
        /** A type-specific factor (gradient paints only) */
        float   _factor2;
        /** A transform to apply to this paint */
        Affine2 _transform;
        /** The paint texture (texture paints only) */
        std::shared_ptr<Texture> _texture;

    public:
        /**
         * Creates an uninitialized paint.
         *
         * You must initialize this paint before use. Otherwise it will not do
         * anything do anything when applied.
         *
         * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate a Node on
         * the heap, use one of the static constructors instead.
         */
        Paint();
        
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
        bool initLinearGradient(Color4 inner, Color4 outer, const Vec2 start, const Vec2 end);
        
        /**
         * Initializes a simple radial gradient of the two colors.
         *
         * In a simple radial gradient, the inner color starts at the center
         * and transitions smoothly to the outer color at the given radius.
         *
         * This initializer is very similar to {@link Gradient#initRadial},
         * except that the positions are given in the coordinate system of the
         * canvas node, and not using texture coordinates.
         *
         * @param inner     The inner gradient color
         * @param outer     The outer gradient color
         * @param center    The center of the radial gradient
         * @param radius    The radius for the outer color
         *
         * @return true if initialization was successful.
         */
        bool initRadialGradient(Color4 inner, Color4 outer, const Vec2 center, float radius) {
            return initRadialGradient(inner,outer,center,radius,radius);
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
        bool initRadialGradient(Color4 inner, Color4 outer, const Vec2 center, float iradius, float oradius);
        
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
         * to a corner, consider two segments.  The first starts at the corner
         * and moves towards the center of the rectangle half-feather in
         * distance. The end of this segment is the end of the inner color
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
         * @param bounds    The bounds of the rounded rectangle.
         * @param radius    The corner radius of the rounded rectangle.
         * @param feather   The feather value for color interpolation
         *
         * @return true if initialization was successful.
         */
        bool initBoxGradient(Color4 inner, Color4 outer, const Rect bounds, float radius, float feather) {
            return initBoxGradient(inner,outer,bounds.origin,bounds.size,radius,feather);
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
         * @param origin    The origin of the rounded rectangle.
         * @param size      The size of the rounded rectangle.
         * @param radius    The corner radius of the rounded rectangle.
         * @param feather   The feather value for color interpolation
         *
         * @return true if initialization was successful.
         */
        bool initBoxGradient(Color4 inner, Color4 outer, const Vec2 origin, const Size size, float radius, float feather);

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
         * @param bounds    The bounding box to place the texture
         *
         * @return true if initialization was successful.
         */
        bool initPattern(std::shared_ptr<Texture> texture, const Rect bounds) {
            return initPattern(texture,bounds.origin,bounds.size);
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
        bool initPattern(std::shared_ptr<Texture> texture, const Vec2 origin, const Size size);

        /**
         * Returns a new linear gradient with the given start and end positions.
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
         * @return a new linear gradient with the given start and end positions.
         */
        static std::shared_ptr<Paint> allocLinearGradient(Color4 inner, Color4 outer, const Vec2 start, const Vec2 end) {
            std::shared_ptr<Paint> result = std::make_shared<Paint>();
            return (result->initLinearGradient(inner,outer,start,end) ? result : nullptr);
        }

        /**
         * Returns a new simple radial gradient of the two colors.
         *
         * In a simple radial gradient, the inner color starts at the center
         * and transitions smoothly to the outer color at the given radius.
         *
         * This initializer is very similar to {@link Gradient#initRadial},
         * except that the positions are given in the coordinate system of the
         * canvas node, and not using texture coordinates.
         *
         * @param inner     The inner gradient color
         * @param outer     The outer gradient color
         * @param center    The center of the radial gradient
         * @param radius    The radius for the outer color
         *
         * @return a new simple radial gradient of the two colors.
         */
        static std::shared_ptr<Paint> allocRadialGradient(Color4 inner, Color4 outer, const Vec2 center,
                                                          float radius) {
            std::shared_ptr<Paint> result = std::make_shared<Paint>();
            return (result->initRadialGradient(inner,outer,center,radius) ? result : nullptr);
        }
        
        /**
         * Returns a new general radial gradient of the two colors.
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
         * @return a new general radial gradient of the two colors.
         */
        static std::shared_ptr<Paint> allocRadialGradient(Color4 inner, Color4 outer, const Vec2 center,
                                                          float iradius, float oradius) {
            std::shared_ptr<Paint> result = std::make_shared<Paint>();
            return (result->initRadialGradient(inner,outer,center,iradius,oradius) ? result : nullptr);
        }

        /**
         * Returns a new box gradient of the two colors.
         *
         * Box gradients paint the inner color in a rounded rectangle, and
         * then use a feather setting to transition to the outer color. To be
         * well-defined, the corner radius should be no larger than half the
         * width and height (at which point it defines a capsule). Shapes
         * with abnormally large radii are undefined.
         *
         * The feather value acts like the inner and outer radius of a radial
         * gradient. If a line is drawn from the center of the round rectangle
         * to a corner, consider two segments.  The first starts at the corner
         * and moves towards the center of the rectangle half-feather in
         * distance. The end of this segment is the end of the inner color
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
         * @param bounds    The bounds of the rounded rectangle.
         * @param radius    The corner radius of the rounded rectangle.
         * @param feather   The feather value for color interpolation
         *
         * @return a new box gradient of the two colors.
         */
        static std::shared_ptr<Paint> allocBoxGradient(Color4 inner, Color4 outer, const Rect bounds,
                                                       float radius, float feather) {
            std::shared_ptr<Paint> result = std::make_shared<Paint>();
            return (result->initBoxGradient(inner,outer,bounds.origin,bounds.size,radius,feather) ? result : nullptr);
        }

        /**
         * Returns a new box gradient of the two colors.
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
         * @param origin    The origin of the rounded rectangle.
         * @param size      The size of the rounded rectangle.
         * @param radius    The corner radius of the rounded rectangle.
         * @param feather   The feather value for color interpolation
         *
         * @return a new box gradient of the two colors.
         */
        static std::shared_ptr<Paint> allocBoxGradient(Color4 inner, Color4 outer, const Vec2 origin,
                                                       const Size size, float radius, float feather) {
            std::shared_ptr<Paint> result = std::make_shared<Paint>();
            return (result->initBoxGradient(inner,outer,origin,size,radius,feather) ? result : nullptr);
        }
        
        /**
         * Returns a new texture pattern with the given bounds.
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
         * @param bounds    The bounding box to place the texture
         *
         * @return a new texture pattern with the given bounds.
         */
        static std::shared_ptr<Paint> allocPattern(std::shared_ptr<Texture> texture, const Rect bounds) {
            std::shared_ptr<Paint> result = std::make_shared<Paint>();
            return (result->initPattern(texture,bounds.origin,bounds.size) ? result : nullptr);
        }

        /**
         * Returns a new texture pattern with the given bounds.
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
         * @return a new texture pattern with the given bounds.
         */
        static std::shared_ptr<Paint> allocPattern(std::shared_ptr<Texture> texture, const Vec2 origin, const Size size) {
            std::shared_ptr<Paint> result = std::make_shared<Paint>();
            return (result->initPattern(texture,origin,size) ? result : nullptr);
        }

        /**
         * Returns true if the paint is a gradient.
         *
         * Note that only gradient paints may be applied to text. Images
         * patterns applied to text will be ignored.
         *
         * @return true if the paint is a gradient.
         */
        bool isGradient() const;

        /**
         * Returns the local transform for this paint
         *
         * Local transforms are applied to all drawing commands, including paints.
         * This is necessary because the paints are specified in the coordinate
         * system of the canvas node.
         *
         * @return the local transform for this paint
         */
        const Affine2& getTransform() const { return _transform; }

        /**
         * Sets the local transform for this paint
         *
         * Local transforms are applied to all drawing commands, including paints.
         * This is necessary because the paints are specified in the coordinate
         * system of the canvas node.
         *
         * @param transform     The local transform for this paint
         */
        void setTransform(const Affine2& transform) { _transform = transform; }
        
        /** Allow a drawing command to access the attributes of this paint */
        friend class Command;

    };
    
#pragma mark -
#pragma mark Enums
    /**
     * An enum to allow the user to specify an explicit winding
     *
     * This enum allows you to specify the winding order when creating
     * a new path. A rule other than NONE will guarantee your path has
     * a certain orientation even if you generate a path in the wrong
     * order.
     */
    enum class Winding : int {
        /** Use the default, given orientation for each path */
        NONE = 0,
        /** Use a counter-clockwise orientation for each path */
        CCW  = 1,
        /** Use a clockwise orientation for each path */
        CW   = 2
    };
    
    /**
     * An enum to specify the winding rule for filled polygons
     *
     * This rule specifies how to fill a polygon in the case of multiple
     * subpaths and self-intersections.  See
     *
     *      https://en.wikipedia.org/wiki/Nonzero-rule
     *
     * for a discussion of how the default rule works.
     *
     * In addition, this category includes rules for clipping and masking.
     * That is because, due to the way canvas nodes are designed, any
     * non-trivial use of the stencil buffer mandates an even-odd fill
     * rule.
     */
    enum class FillRule : int {
        /**
         * Uses the non-zero winding rule (DEFAULT)
         *
         * If a stencil was previously created by the {@link FillRule#STENCIL} rule,
         * this fill rule with erase that stencil.
         */
        NONZERO = 0,
        /**
         * Uses the even-odd rule
         *
         * If a stencil was previously created by the {@link FillRule#STENCIL} rule,
         * this fill rule with erase that stencil.
         */
        EVENODD = 1,
        /**
         * Creates a stencil buffer with this shape via an even-odd rule.
         *
         * This fill rule writes to the stencil buffer but it does not
         * draw to the screen. It is used in combination with {@link FillRule#CLIPFILL}
         * and {@link FillRule#MASKFILL} to provide visual effects.
         */
        STENCIL = 2,
        /**
         * Uses the even-odd rule to draw a shape clipped to the stencil buffer.
         *
         * This rule must be preceded by a drawing sequence using the fill rule
         * {@link FillRule#STENCIL} to have any effect.
         */
        CLIPFILL = 3,
        /**
         * Uses the even-odd rule to draw a shape masked by the stencil buffer.
         *
         * This rule must be preceded by a drawing sequence using the fill rule
         * {@link FillRule#STENCIL} to have any effect.
         */
        MASKFILL = 4
    };

    
protected:
    /** The individual canvases of this node */
    std::vector<Page*> _canvas;
    /** The active page for drawing */
    size_t _draw;
    /** The active page for editing */
    size_t _edit;
    
public:
#pragma mark -
#pragma mark Constructors
    /**
     * Creates an uninitialized canvas node.
     *
     * You must initialize this canvas node before use.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate a Node on the
     * heap, use one of the static constructors instead.
     */
    CanvasNode() : _draw(0), _edit(0) {}
    
    /**
     * Deletes this canvas node, disposing all resources
     */
    ~CanvasNode() { dispose(); }
    
    /**
     * Disposes all of the resources used by this canvas node.
     *
     * A disposed node can be safely reinitialized. Any children owned by this
     * node will be released.  They will be deleted if no other object owns them.
     *
     * It is unsafe to call this on a Node that is still currently inside of
     * a scene graph.
     */
    virtual void dispose() override;

    /**
     * Initializes a canvas node the size of the display.
     *
     * The bounding box of the node is the current screen size. The node
     * is anchored in the center and has position (width/2,height/2) in
     * the parent space. The node origin is the (0,0) at the bottom left
     * corner of the bounding box.
     *
     * The canvas is initialized with only one drawing buffer. You should
     * call {@link #paginate} to add more buffers.
     *
     * @return true if initialization was successful.
     */
    virtual bool init() override;
    
    /**
     * Initializes a canvas node with the given size.
     *
     * The size defines the content size. The bounding box of the node is
     * (0,0,width,height) and is anchored in the bottom left corner (0,0).
     * The node is positioned at the origin in parent space.
     *
     * The canvas is initialized with only one drawing buffer. You should
     * call {@link #paginate} to add more buffers.
     *
     * @param size  The size of the node in parent space
     *
     * @return true if initialization was successful.
     */
    virtual bool initWithBounds(const Size size) override;

    /**
     * Initializes a canvas node with the given size.
     *
     * The size defines the content size. The bounding box of the node is
     * (0,0,width,height) and is anchored in the bottom left corner (0,0).
     * The node is positioned at the origin in parent space.
     *
     * The canvas is initialized with only one drawing buffer. You should
     * call {@link #paginate} to add more buffers.
     *
     * @param width     The width of the node in parent space
     * @param height    The height of the node in parent space
     *
     * @return true if initialization was successful.
     */
    virtual bool initWithBounds(float width, float height) override {
        return initWithBounds(Size(width,height));
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
     * call {@link #paginate} to add more buffers.
     *
     * @param rect  The bounds of the node in parent space
     *
     * @return true if initialization was successful.
     */
    virtual bool initWithBounds(const Rect rect) override;
    
    /**
     * Initializes a canvas node with the given bounds.
     *
     * The rectangle origin is the bottom left corner of the node in parent
     * space, and corresponds to the origin of the Node space. The size
     * defines its content width and height in node space. The node anchor
     * is placed in the bottom left corner.
     *
     * The canvas is initialized with only one drawing buffer. You should
     * call {@link #paginate} to add more buffers.
     *
     * @param x         The x-coordinate of the node origin in parent space
     * @param y         The y-coordinate of the node origin in parent space
     * @param width     The width of the node in parent space
     * @param height    The height of the node in parent space
     *
     * @return true if initialization was successful.
     */
    virtual bool initWithBounds(float x, float y, float width, float height) override {
        return initWithBounds(Rect(x,y,width,height));
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
    virtual bool initWithData(const Scene2Loader* loader, const std::shared_ptr<JsonValue>& data) override;

#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a newly allocated a canvas node the size of the display.
     *
     * The bounding box of the node is the current screen size. The node
     * is anchored in the center and has position (width/2,height/2) in
     * the parent space. The node origin is the (0,0) at the bottom left
     * corner of the bounding box.
     *
     * The canvas is initialized with only one drawing buffer.  You should
     * call {@link #paginate} to add more buffers.
     *
     * @return a newly allocated a canvas node the size of the display.
     */
    static std::shared_ptr<CanvasNode> alloc() {
        std::shared_ptr<CanvasNode> result = std::make_shared<CanvasNode>();
        return (result->init() ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated a canvas node with the given size.
     *
     * The size defines the content size. The bounding box of the node is
     * (0,0,width,height) and is anchored in the bottom left corner (0,0).
     * The node is positioned at the origin in parent space.
     *
     * The canvas is initialized with only one drawing buffer.  You should
     * call {@link #paginate} to add more buffers.
     *
     * @param size  The size of the node in parent space
     *
     * @return a newly allocated a canvas node with the given size.
     */
    static std::shared_ptr<CanvasNode> allocWithBounds(const Size size) {
        std::shared_ptr<CanvasNode> result = std::make_shared<CanvasNode>();
        return (result->initWithBounds(size) ? result : nullptr);
    }

    /**
     * Returns a newly allocated a canvas node with the given size.
     *
     * The size defines the content size. The bounding box of the node is
     * (0,0,width,height) and is anchored in the bottom left corner (0,0).
     * The node is positioned at the origin in parent space.
     *
     * The canvas is initialized with only one drawing buffer.  You should
     * call {@link #paginate} to add more buffers.
     *
     * @param width     The width of the node in parent space
     * @param height    The height of the node in parent space
     *
     * @return a newly allocated a canvas node with the given size.
     */
    static std::shared_ptr<CanvasNode> allocWithBounds(float width, float height) {
        std::shared_ptr<CanvasNode> result = std::make_shared<CanvasNode>();
        return (result->initWithBounds(width, height) ? result : nullptr);
    }

    /**
     * Returns a newly allocated a canvas node with the given bounds.
     *
     * The rectangle origin is the bottom left corner of the node in parent
     * space, and corresponds to the origin of the Node space. The size
     * defines its content width and height in node space. The node anchor
     * is placed in the bottom left corner.
     *
     * The canvas is initialized with only one drawing buffer.  You should
     * call {@link #paginate} to add more buffers.
     *
     * @param rect  The bounds of the node in parent space
     *
     * @return a newly allocated a canvas node with the given bounds.
     */
    static std::shared_ptr<CanvasNode> allocWithBounds(const Rect rect) {
        std::shared_ptr<CanvasNode> result = std::make_shared<CanvasNode>();
        return (result->initWithBounds(rect) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated a canvas node with the given bounds.
     *
     * The rectangle origin is the bottom left corner of the node in parent
     * space, and corresponds to the origin of the Node space. The size
     * defines its content width and height in node space. The node anchor
     * is placed in the bottom left corner.
     *
     * The canvas is initialized with only one drawing buffer.  You should
     * call {@link #paginate} to add more buffers.
     *
     * @param x         The x-coordinate of the node origin in parent space
     * @param y         The y-coordinate of the node origin in parent space
     * @param width     The width of the node in parent space
     * @param height    The height of the node in parent space
     *
     * @return a newly allocated a canvas node with the given bounds.
     */
    static std::shared_ptr<CanvasNode> allocWithBounds(float x, float y, float width, float height) {
        std::shared_ptr<CanvasNode> result = std::make_shared<CanvasNode>();
        return (result->initWithBounds(x,y,width,height) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated a canvas node with the given JSON specificaton.
     *
     * This initializer is designed to receive the "data" object from the
     * JSON passed to {@link Scene2Loader}.  This JSON format supports the
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
     * @return a newly allocated a canvas node with the given JSON specificaton.
     */
    static std::shared_ptr<CanvasNode> allocWithData(const Scene2Loader* loader, const std::shared_ptr<JsonValue>& data) {
        std::shared_ptr<CanvasNode> result = std::make_shared<CanvasNode>();
        return (result->initWithData(loader,data) ? result : nullptr);
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
    size_t pages() const;
    
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
    void paginate(size_t size);
    
    /**
     * Returns the index of the current edit page
     *
     * The edit page is the page that receives drawing commands. It
     * does not need to be the same page as the one currently being
     * drawn.
     *
     * @return the index of the current edit page
     */
    size_t getEditPage();
    
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
    void setEditPage(size_t page);

    /**
     * Returns the index of the current drawing page
     *
     * The drawing page is the page that is shown on the screen. It
     * does not need to be the same page as the one currently
     * receiving drawing commands.
     *
     * @return the index of the current drawing page
     */
    size_t getDrawPage();
    
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
    void setDrawPage(size_t page);
    
    /**
     * Clears the drawing commands for the active edit page.
     *
     * Any other page is uneffected. This method should be called
     * before drawing to a page, as otherwise the commands are appended
     * to any existing drawing commands.
     */
    void clearPage();

    /**
     * Clears the drawing commands from all pages.
     */
    void clearAll();
    
    /**
     * Draws the drawing page via the given SpriteBatch.
     *
     * This method only worries about drawing the current node.  It does not
     * attempt to render the children.
     *
     * @param batch     The SpriteBatch to draw with.
     * @param transform The global transformation matrix.
     * @param tint      The tint to blend with the Node color.
     */
    virtual void draw(const std::shared_ptr<SpriteBatch>& batch, const Affine2& transform, Color4 tint) override;
    
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
    void saveState();

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
    void restoreState();

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
    void resetState();
    
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
    float getFringe() const;
    
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
    void setFringe(float fringe);
    
    /**
     * Returns the transparency to apply to all rendered shapes.
     *
     * The alpha should be a value 0..1. Already transparent paths will get
     * proportionally more transparent as well.
     *
     * @return the transparency to apply to all rendered shapes.
     */
    float getGlobalAlpha();

    /**
     * Sets the transparency to apply to all rendered shapes.
     *
     * The alpha should be a value 0..1. Already transparent paths will get
     * proportionally more transparent as well.
     *
     * @param alpha     The transparency to apply to all rendered shapes.
     */
    void setGlobalAlpha(float alpha);

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
    const Affine2& getCommandTransform();

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
    void setCommandTransform(const Affine2& transform);

    /**
     * Resets the command transform to an identity matrix.
     *
     * When this method is called all subsequent calls will be applied in
     * the native coordinate space of the canvas node.
     *
     * For more information on how this transform is applied to commands,
     * see {@link #getCommandTransform}.
     */
    void clearCommandTransform();

    /**
     * Translates all commands by the given offset.
     *
     * This translation is cumulative with the existing command transform.
     * It is applied after the existing transform operations.
     *
     * For more information on how this transform is applied to commands,
     * see {@link #getCommandTransform}.
     *
     * @param p     The translation offset
     */
    void translateCommands(const Vec2 p) {
        translateCommands(p.x,p.y);
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
    void translateCommands(float x, float y);

    /**
     * Scales all commands by the given factor.
     *
     * This resizing operation is cumulative with the existing command
     * transform. It is applied after the existing transform operations.
     *
     * For more information on how this transform is applied to commands,
     * see {@link #getCommandTransform}.
     *
     * @param s     The scaling factor
     */
    void scaleCommands(const Vec2 s) {
        scaleCommands(s.x,s.y);
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
    void scaleCommands(float sx, float sy);

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
    void rotateCommands(float angle);

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
    void skewXCommands(float angle);

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
    void skewYCommands(float angle);
    
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
    const std::shared_ptr<Scissor>& getLocalScissor() const;
    
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
    void setLocalScissor(const std::shared_ptr<Scissor>& scissor);
    
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
    void applyLocalScissor(const std::shared_ptr<Scissor>& scissor);

    /**
     * Resets and disables scissoring for this canvas.
     *
     * Clearing the local scissor will not reveal any commands previously
     * clipped by the local scissor. In addition, this method has no
     * effect on the global scissor {@link SceneNode#getScissor}.
     */
    void clearLocalScissor();
    
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
    Winding getWinding() const;
    
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
     * @param winding   The current winding order
     */
    void setWinding(Winding winding);
    
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
    FillRule getFillRule() const;

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
    void setFillRule(FillRule rule);

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
    Color4 getFillColor() const;

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
    void setFillColor(Color4 color);

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
    const std::shared_ptr<Paint>& getFillPaint() const;

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
    void setFillPaint(const std::shared_ptr<Paint>& paint);
    
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
    Color4 getStrokeColor() const;

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
    void setStrokeColor(Color4 color);
    
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
    const std::shared_ptr<Paint>& getStrokePaint() const;

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
    void setStrokePaint(const std::shared_ptr<Paint>& paint);

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
    float getStrokeWidth() const;

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
    void setStrokeWidth(float width);
    
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
    float getMitreLimit() const;

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
    void setMitreLimit(float limit);
    
    /**
     * Returns the joint value for the stroke.
     *
     * The joint type determines how the stroke joins the extruded line segments
     * together.  See {@link poly2::Joint} for the description of the types.
     *
     * @return the joint value for the stroke.
     */
    poly2::Joint getLineJoint();

    /**
     * Sets the joint value for the stroke.
     *
     * The joint type determines how the stroke joins the extruded line segments
     * together.  See {@link poly2::Joint} for the description of the types.
     *
     * @param joint     The joint value for the stroke.
     */
    void setLineJoint(poly2::Joint joint);

    
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
    poly2::EndCap getLineCap();

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
    void setLineCap(poly2::EndCap cap);

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
    GLenum getBlendEquation() const;

    
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
    void setBlendEquation(GLenum equation);
    
    /**
     * Sets the blending function for the source color
     *
     * This setting is applied at the call to either {@link #strokePaths}
     * or {@link #fillPaths}.
     *
     * The enums are the standard ones supported by OpenGL. See
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
     *
     * However, this setter does not do any error checking to verify that
     * the enums are valid.
     *
     * By default this value is GL_SRC_ALPHA, as scene graphs do not
     * use premultiplied alpha.
     *
     * @param func  Specifies how the source color is blended
     */
    void setSrcBlendFunc(GLenum func) {
        setSrcBlendFunc(func,func);
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
     * @param srcRGB       The blend function for the source RGB components
     * @param srcAlpha     The blend function for the source alpha component
     */
    void setSrcBlendFunc(GLenum srcRGB, GLenum srcAlpha);
    
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
    GLenum getSrcRGBFunc() const;
    
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
    GLenum getSrcAlphaFunc() const;

    /**
     * Sets the blending function for the destination color
     *
     * This setting is applied at the call to either {@link #strokePaths}
     * or {@link #fillPaths}.
     *
     * The enums are the standard ones supported by OpenGL.  See
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
     *
     * However, this setter does not do any error checking to verify that
     * the enums are valid.
     *
     * By default this value is GL_ONE_MINUS_SRC_ALPHA, as scene graphs
     * do not use premultiplied alpha.
     *
     * @param func  Specifies how the source color is blended
     */
    void setDstBlendFunc(GLenum func) {
        setDstBlendFunc(func, func);
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
     * @param dstRGB       The blend function for the destination RGB components
     * @param dstAlpha     The blend function for the destination alpha component
     */
    void setDstBlendFunc(GLenum dstRGB, GLenum dstAlpha);

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
    GLenum getDstRGBFunc() const;
    
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
    GLenum getDstAlphaFunc() const;


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
    void beginPath();
    
    /**
     * Starts a new sub-path with specified point as first point.
     *
     * The command transform is applied to this method when called.
     *
     * @param pos   The initial path point
     */
    void moveTo(const Vec2 pos);

    /**
     * Starts a new sub-path with specified point as first point.
     *
     * The command transform is applied to this method when called.
     *
     * @param x     The x-coordinate of the initial path point
     * @param y     The y-coordinate of the initial path point
     */
    void moveTo(float x, float y) {
        moveTo(Vec2(x,y));
    }
    
    /**
     * Adds a line segment from the previous point to the given one.
     *
     * If there is no current path, this method creates a new subpath starting
     * at the origin. The command transform is applied to this method when called.
     *
     * @param pos   The next path point
     */
    void lineTo(const Vec2 pos);

    /**
     * Adds a line segment from the previous point to the given one.
     *
     * If there is no current path, this method creates a new subpath starting
     * at the origin. The command transform is applied to this method when called.
     *
     * @param x     The x-coordinate of the next path point
     * @param y     The y-coordinate of the next path point
     */
    void lineTo(float x, float y)  {
        lineTo(Vec2(x,y));
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
    void bezierTo(const Vec2 c1, const Vec2 c2, const Vec2 p);

    /**
     * Adds a cubic bezier segment from the previous point.
     *
     * The control points specify the tangents as described in {@link Spline2}.
     *
     * If there is no current path, this method creates a new subpath starting
     * at the origin. The command transform is applied to this method when called.
     *
     * @param c1x   The x-coordinate of the first control point
     * @param c1y   The y-coordinate of the first control point
     * @param c2x   The x-coordinate of the second control point
     * @param c2y   The y-coordinate of the second control point
     * @param px    The x-coordinate of the bezier end
     * @param py    The y-coordinate of the bezier end
     */
    void bezierTo(float c1x, float c1y, float c2x, float c2y, float px, float py) {
        bezierTo(Vec2(c1x,c1y), Vec2(c2x,c2y), Vec2(px,py));
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
    void quadTo(const Vec2 c, const Vec2 p);
    
    /**
     * Adds a quadratic bezier segment from the previous point.
     *
     * The control point is as described in {@link Spline2#addQuad}.
     *
     * If there is no current path, this method creates a new subpath starting
     * at the origin. The command transform is applied to this method when called.
     *
     * @param cx    The x-coordinate of the control point
     * @param cy    The y-coordinate of the control point
     * @param px    The x-coordinate of the bezier end
     * @param py    The y-coordinate of the bezier end
     */
    void quadTo(float cx, float cy, float px, float py) {
        quadTo(Vec2(cx,cy),Vec2(px,py));
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
    void arcTo(const Vec2 center, float r, float a0, float a1, bool ccw=true);

    /**
     * Adds an arc segment sweeping from angle a0 to a1.
     *
     * The arc center is at is at (cx,cy) and has radius is r. The method will
     * draw a straight line from the previous point in the path to the point
     * at angle a0.  It will then sweep the arc from angle a0 to a1. The value
     * ccw determines whether the arc sweeps counter clockwise or clockwise,
     * as it is not necessarily possible to tell from the angles themselves.
     *
     * If there is no current path, this method creates a new subpath starting
     * at the point for a0. Note that this differs from other drawing commands
     * that would start a new path at the origin. The command transform is
     * applied to this method when called.
     *
     * @param cx        The x-coordinate of the arc center
     * @param cy        The y-coordinate of the arc center
     * @param r         The arc radius
     * @param a0        The starting angle of the arc
     * @param a1        The ending angle of the arc
     * @param ccw       Whether draw the arc counter clockwise
     */
    void arcTo(float cx, float cy, float r, float a0, float a1, bool ccw=true) {
        arcTo(Vec2(cx,cy),r,a0,a1,ccw);
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
    void arcTo(const Vec2 s, const Vec2 e, float radius);
    
    /**
     * Adds an arc segment whose corner is defined by the previous point.
     *
     * The previous point acts as the center for the arc, which is drawn through
     * the two points with the given radius.
     *
     * If there is no current path, this method creates a new subpath starting
     * at point (sx,sy). Note that this differs from other drawing commands that
     * would start a new path at the origin. The command transform is applied to
     * this method when called.
     *
     * @param sx        The x-coordinate of the start of the arc
     * @param sy        The y-coordinate of the start of the arc
     * @param ex        The x-coordinate of the end of the arc
     * @param ey        The y-coordinate of the end of the arc
     * @param radius    The arc radius
     */
    void arcTo(float sx, float sy, float ex, float ey, float radius) {
        arcTo(Vec2(sx,sy),Vec2(ex,ey),radius);
    }

    /**
     * Closes the current subpath with a line segment.
     *
     * While this method closes the subpath, it does **not** start a new subpath.
     * You will need to call {@link #moveTo} to do that.
     */
    void closePath();
    
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
    void drawArc(const Vec2 center, float r, float a0, float a1, bool ccw=true);

    /**
     * Creates a new circle arc subpath, sweeping from angle a0 to a1.
     *
     * The arc center is at is at (cx,cy) and has radius is r. The new subpath
     * will start at the point corresponding to angle a0. The value ccw
     * determines whether the arc sweeps counter clockwise or clockwise, as it
     * is not necessarily possible to tell this from the angles themselves.
     *
     * The command transform is applied to this method when called.
     *
     * @param cx        The x-coordinate of the arc center
     * @param cy        The y-coordinate of the arc center
     * @param r         The arc radius
     * @param a0        The starting angle of the arc
     * @param a1        The ending angle of the arc
     * @param ccw       Whether draw the arc counter clockwise
     */
    void drawArc(float cx, float cy, float r, float a0, float a1, bool ccw=true) {
        drawArc(Vec2(cx,cy),r,a0,a1,ccw);
    }

    /**
     * Creates a new rectangle shaped subpath
     *
     * The command transform is applied to this method when called.
     *
     * @param rect  The rectangle bounds
     */
    void drawRect(const Rect rect);
    
    /**
     * Creates a new rectangle shaped subpath
     *
     * The command transform is applied to this method when called.
     *
     * @param x     The x-coordinate of the rectangle origin
     * @param y     The y-coordinate of the rectangle origin
     * @param w     The rectangle width
     * @param h     The rectangle height
     */
    void drawRect(float x, float y, float w, float h)  {
        drawRect(Rect(x,y,w,h));
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
     * @param rect  The rectangle bounds
     * @param r     The corner radius
     */
    void drawRoundedRect(const Rect rect, float r) {
        drawRoundedRect(rect.origin.x,rect.origin.y,rect.size.width,rect.size.height,r);
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
    void drawRoundedRect(float x, float y, float w, float h, float r);

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
     * @param rect  The rectangle bounds
     * @param radBL The corner radius of the bottom left
     * @param radTL The corner radius of the top left
     * @param radTR The corner radius of the top right
     * @param radBR The corner radius of the bottom right
     */
    void drawRoundedRectVarying(const Rect rect, float radBL, float radTL, float radTR, float radBR) {
        drawRoundedRectVarying(rect.origin.x,rect.origin.y,rect.size.width,rect.size.height,
                               radTL,radTR,radBR,radBL);
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
    void drawRoundedRectVarying(float x, float y, float w, float h,
                                float radBL, float radTL, float radTR, float radBR);

    /**
     * Creates a new ellipse shaped subpath.
     *
     * Note that the ellipse size defines the two diameters of the ellipse,
     * and not the radii.
     *
     * The command transform is applied to this method when called.
     *
     * @param center    The ellipse center
     * @param size      The ellipse size
     */
    void drawEllipse(const Vec2 center, const Size size) {
        drawEllipse(center.x, center.y, size.width/2, size.height/2);
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
    void drawEllipse(const Rect bounds);

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
    void drawEllipse(float cx, float cy, float rx, float ry);

    /**
     * Creates a new circle shaped sub-path.
     *
     * @param center    The circle center
     * @param r         The circle radius
     */
    void drawCircle(const Vec2 center, float r) {
        drawEllipse(center.x, center.y, r, r);
    }

    /**
     * Creates a new circle shaped sub-path.
     *
     * @param cx        The x-coordinate of the circle center
     * @param cy        The y-coordinate of the circle center
     * @param r         The circle radius
     */
    void drawCircle(float cx, float cy, float r);
  
    /**
     * Fills the current path (and subpaths) with the current fill style.
     *
     * This method will commit the any outstanding paths, but it will not
     * clear them.  You should call {@link #beginPath} to start a new path
     * sequence.
     */
    void fillPaths();
    
    /**
     * Extrudes the current path (and subpaths) with the current stroke style.
     *
     * This method will commit the any outstanding paths, but it will not
     * clear them.  You should call {@link #beginPath} to start a new path
     * sequence.
     */
    void strokePaths();

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
    const std::shared_ptr<Font>& getFont() const;
    
    /**
     * Sets the font for the current text style
     *
     * This is the font that will be used as a call to either {@link #drawText}
     * or {@link #drawTextBox}. If there is no active font when one of the those
     * methods are called, they will fail.
     *
     * @param font  The font for the current text style
     */
    void setFont(const std::shared_ptr<Font>& font);
    
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
    float getFontSize() const;

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
    void setFontSize(float size);

    /**
     * Returns the blur radius of the current text style.
     *
     * When blurring text, use a font with the same {@link Font#getPadding}
     * as the blur size. This will prevent bleeding across characters in
     * the atlas.
     *
     * @return the blur radius of the current text style.
     */
    float getFontBlur() const;

    /**
     * Sets the blur radius of the current text style.
     *
     * When blurring text, use a font with the same {@link Font#getPadding}
     * as the blur size. This will prevent bleeding across characters in
     * the atlas.
     *
     * @param blur  The blur radius of the current text style.
     */
    void setFontBlur(float blur);

    /**
     * Returns the line spacing of the current text style.
     *
     * This value is multiplied by the font size to determine the space
     * between lines. So a value of 1 is single-spaced text, while a value
     * of 2 is double spaced. The value should be positive.
     *
     * @return the line spacing of the current text style.
     */
    float getTextSpacing() const;

    /**
     * Sets the line spacing of the current text style.
     *
     * This value is multiplied by the font size to determine the space
     * between lines. So a value of 1 is single-spaced text, while a value
     * of 2 is double spaced. The value should be positive.
     *
     * @param spacing   The line spacing of the current text style.
     */
    void setTextSpacing(float spacing);

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
    HorizontalAlign getHorizontalTextAlign() const;
    
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
    void setHorizontalTextAlign(HorizontalAlign align);

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
    VerticalAlign getVerticalTextAlign() const;
    
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
    void setVerticalTextAlign(VerticalAlign align);

    /**
     * Draws the text string at specified location.
     *
     * Position p is the location of the text origin, which is defined by both
     * {@link #getHorizontalTextAlign} and {@link #getVerticalTextAlign}. This
     * command is subject to the current command transform.
     *
     * This command will use the current text style, and color the glyphs with
     * the current fill color. If there is a fill {@link Paint} then it will
     * also be applied, provided that it is a gradient (text cannot be textured
     * with images).
     *
     * The string may either be in UTF8 or ASCII; the method will handle
     * conversion automatically.
     *
     * @param p     The position of the text origin
     * @param text  The text to display
     */
    void drawText(const Vec2 p, const std::string text) {
        drawText(p.x,p.y,text.c_str(),text.c_str()+text.size());
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
    void drawText(float x, float y, const std::string text) {
        drawText(x,y,text.c_str(),text.c_str()+text.size());
    }

    
    /**
     * Draws the text string at specified location.
     *
     * Position p is the location of the text origin, which is defined by both
     * {@link #getHorizontalTextAlign} and {@link #getVerticalTextAlign}. This
     * command is subject to the current command transform.
     *
     * This command will use the current text style, and color the glyphs with
     * the current fill color. If there is a fill {@link Paint} then it will
     * also be applied, provided that it is a gradient (text cannot be textured
     * with images).
     *
     * The C-style string substr need not be null-terminated. Instead,
     * the termination is indicated by the parameter end. This provides
     * efficient substring processing. The string may either be in UTF8
     * or ASCII; the method will handle conversion automatically.
     *
     * @param p         The position of the text origin
     * @param substr    The start of the string
     * @param end       The end of the string
     */
    void drawText(const Vec2 p, const char* substr, const char* end) {
        drawText(p.x,p.y,substr,end);
    }
    
    /**
     * Draws the text string at specified location.
     *
     * Position (x,y) is the location of the text origin, which is defined by both
     * {@link #getHorizontalTextAlign} and {@link #getVerticalTextAlign}. This
     * command is subject to the current command transform.
     *
     * This command will use the current text style, and color the glyphs with
     * the current fill color. If there is a fill {@link Paint} then it will
     * also be applied, provided that it is a gradient (text cannot be textured
     * with images).
     *
     * The C-style string substr need not be null-terminated. Instead,
     * the termination is indicated by the parameter end. This provides
     * efficient substring processing. The string may either be in UTF8
     * or ASCII; the method will handle conversion automatically.
     *
     * @param x         The x-coordinate of the text origin
     * @param y         The y-coordinate of the text origin
     * @param substr    The start of the string
     * @param end       The end of the string
     */
    void drawText(float x, float y, const char* substr, const char* end);

    /**
     * Draws a multiline text string at specified location.
     *
     * Position p is the location of the text origin, which is defined by both
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
     * @param p         The position of the text origin
     * @param width     The text-wrap width
     * @param text      The text to display
     */
    void drawTextBox(const Vec2 p, float width, const std::string text) {
        drawTextBox(p.x,p.y,width,text.c_str(),text.c_str()+text.size());
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
    void drawTextBox(float x, float y, float width, const std::string text) {
        drawTextBox(x,y,width,text.c_str(),text.c_str()+text.size());
    }


    /**
     * Draws a multiline text string at specified location.
     *
     * Position p is the location of the text origin, which is defined by both
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
     * The C-style string substr need not be null-terminated. Instead,
     * the termination is indicated by the parameter end. This provides
     * efficient substring processing. The string may either be in UTF8
     * or ASCII; the method will handle conversion automatically.
     *
     * @param p         The position of the text origin
     * @param width     The text-wrap width
     * @param substr    The start of the string
     * @param end       The end of the string
     */
    void drawTextBox(const Vec2 p, float width, const char* substr, const char* end) {
        drawTextBox(p.x,p.y,width,substr,end);
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
     * The C-style string substr need not be null-terminated. Instead,
     * the termination is indicated by the parameter end. This provides
     * efficient substring processing. The string may either be in UTF8
     * or ASCII; the method will handle conversion automatically.
     *
     * @param x         The x-coordinate of the text origin
     * @param y         The y-coordinate of the text origin
     * @param width     The text-wrap width
     * @param substr    The start of the string
     * @param end       The end of the string
     */
    void drawTextBox(float x, float y, float width, const char* substr, const char* end);

private:
    /** This macro disables the copy constructor (not allowed on scene graphs) */
    CU_DISALLOW_COPY_AND_ASSIGN(CanvasNode);

    
};
    }
}

#endif /* __CU_CANVAS_NODE_H__ */
