//
//  CUGradient.h
//  Cornell University Game Library (CUGL)
//
//  This module provides support for a two-color gradient.  It uses a box
//  gradient definition to support both linear gradients and radial gradients
//  in addition to box gradients.
//
//  This module is based on he NVGpaint datatype from nanovg by Mikko Mononen
//  (memon@inside.org). It has been modified to support the CUGL framework.
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
//  With that said, this class looks very similar to the classes in the math 
//  package in that has many methods that assume the object is on the stack and
//  not in a pointer.  That is because we often want to transform these objects
//  with math classes, but we still want shared pointer support for SpriteBatch
//  management.  The result is a class with a bit of a hybrid feel.
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
//  Version: 3/2/21
#ifndef __CU_GRADIENT_H__
#define __CU_GRADIENT_H__
#include <cugl/math/CUAffine2.h>
#include <cugl/math/CUColor4.h>
#include <cugl/math/CUVec2.h>
#include <cugl/math/CURect.h>

namespace cugl {

// Forward declaration
class JsonValue;

/**
 * This class defines a two color gradient.
 *
 * All gradients, including linear and radial gradients, are variations
 * of (rounded) box gradients. A box gradient is defined by (in terms of
 * largest to smallest data):
 *
 * <ul>
 *    <li>An affine transform (for offset and rotation)</li>
 *    <li>An inner color</li>
 *    <li>An outer color</li>
 *    <li>A size vector of the gradient</li>
 *    <li>A corner radius for the rounded rectangle</li>
 *    <li>A feather factor for the transition speed.</li>
 * </ul>
 *
 * Assuming this data is in std140 format, this is a 24 element array of floats.
 * And this is the format that this data is represented in the {@link getData}
 * method so that it can be passed to a {@link UniformBuffer} for improved 
 * performance. It is also possible to get access to the individual components 
 * of the paint gradient, to pass them to a shader directly (though the transform 
 * must be inverted first if it is passed directly).
 *
 * Paint gradients are applied to surfaces in the same way textures are. The
 * gradient is defined on a unit square from (0,0) to (1,1). To be consistent
 * with textures, the origin is at the top right corner. To apply the gradient,
 * the shader should use the texture coordinates of each vertex (or an attribute
 * similar to texture coordinates) combined with the uniforms for this gradient.
 * For a tutorial on how to do this, see the SpriteShader shaders in the
 * the CUGL render package.
 *
 * For simplicity we only permit two colors in a gradient.  For multicolored
 * gradients, the shape should be tesellated with multiple gradient values.
 */
class Gradient {
#pragma mark Values
private:
    Affine2 _inverse;
    Color4f _inner;
    Color4f _outer;
    Vec2    _extent;
    float   _radius;
    float   _feather;
    
public:
#pragma mark -
#pragma mark Constructors
    /**
     * Creates a degenerate, white-colored gradient.
     */
    Gradient();

    /**
     * Creates a copy of the given gradient.
     *
     * @param grad  The gradient to copy
     */
    Gradient(const Gradient& grad) { set(grad); }

    /**
     * Creates a copy with the resources of the given gradient.
     *
     * The original gradient is no longer safe to use after calling this 
     * constructor.
     *
     * @param grad  The gradient to take from
     */
    Gradient(Gradient&& grad);

    /**
     * Deletes this gradient, releasing all resources.
     */
    ~Gradient() {}

    /**
     * Deletes the gradient and resets all attributes.
     *
     * You must reinitialize the gradient to use it.
     */
    void dispose();
    
    /**
     * Initializes a degenerate gradient of the given color.
     *
     * @param color The gradient color
     *
     * @return true if initialization was successful.
     */
    bool init(const Color4f color);

    /**
     * Initializes a linear gradient of the two colors.
     *
     * In a linear gradient, the inner starts at position start, and
     * transitions to the outer color at position end. The transition
     * is along the vector end-start.
     *
     * The values start and end are specified in texture coordinates.
     * That is, (0,0) is the top left corner of the gradient bounding
     * box and (1,1) is the bottom right corner.  It is permissible
     * to have coordinates out of range (so negative or greater than
     * 1). Such values will be interpretted accordingly.
     *
     * @param inner The inner gradient color
     * @param outer The outer gradient color
     * @param start The start position of the inner color
     * @param end   The start position of the outer color
     *
     * @return true if initialization was successful.
     */
    bool initLinear(const Color4 inner, const Color4 outer, const Vec2 start, const Vec2 end);

    /**
     * Initializes a simple radial gradient of the two colors.
     *
     * In a simple radial gradient, the inner color starts at the center
     * and transitions smoothly to the outer color at the given radius.
     *
     * The center and radius are specified in texture coordinates.
     * That is, (0,0) is the top left corner of the gradient bounding
     * box and (1,1) is the bottom right corner.  It is permissible
     * to a center value out of range (so coordinates negative or
     * greater than 1). Such values will be interpretted accordingly.
     *
     * @param inner     The inner gradient color
     * @param outer     The outer gradient color
     * @param center    The center of the radial gradient
     * @param radius    The radius for the outer color
     *
     * @return true if initialization was successful.
     */
    bool initRadial(const Color4 inner, const Color4 outer, const Vec2 center, float radius);

    /**
     * Initializes a general radial gradient of the two colors.
     *
     * In a general radial gradient, the inner color starts at the center
     * and continues to the inner radius.  It then transitions smoothly
     * to the outer color at the outer radius.
     *
     * The center and radii are all specified in texture coordinates.
     * That is, (0,0) is the top left corner of the gradient bounding
     * box and (1,1) is the bottom right corner.  It is permissible
     * to a center value out of range (so coordinates negative or
     * greater than 1). Such value will be interpretted accordingly.
     *
     * @param inner     The inner gradient color
     * @param outer     The outer gradient color
     * @param center    The center of the radial gradient
     * @param iradius   The radius for the inner color
     * @param oradius   The radius for the outer color
     *
     * @return true if initialization was successful.
     */
    bool initRadial(const Color4 inner, const Color4 outer, const Vec2 center,
                    float iradius, float oradius);

    /**
     * Initializes a box gradient of the two colors.
     *
     * Box gradients paint the inner color in a rounded rectangle, and
     * then use a feather setting to transition to the outer color. The
     * box position and corner radius are given in texture coordinates.
     * That is, (0,0) is the top left corner of the gradient bounding
     * box and (1,1) is the bottom right corner. It is permissible for
     * these coordinates to be out of range (so negative values or greater
     * than 1). Such values will be interpretted accordingly.
     *
     * To be well-defined, the corner radius should be no larger than
     * half the width and height (at which point it defines an ellipse).
     * Shapes with abnormally large radii are undefined.
     *
     * The feather value acts like the inner and outer radius of a radial
     * gradient. If a line is drawn from the center of the round rectangle
     * to a corner, consider two segments.  The first starts at the corner
     * and moves towards the center of the rectangle half-feather in
     * distance.  The end of this segment is the end of the inner color
     * The second second starts at the corner and moves in the opposite
     * direction the same amount.  The end of this segement is the other
     * color.  In between, the colors are smoothly interpolated.
     *
     * So, if feather is 0, there is no gradient and the shift from
     * inner color to outer color is immediate.  On the other hand,
     * if feather is larger than the width and hight of the rectangle,
     * the inner color immediately transitions to the outer color.
     *
     * @param inner     The inner gradient color
     * @param outer     The outer gradient color
     * @param box       The bounds of the rounded rectangle.
     * @param radius    The corner radius of the rounded rectangle.
     * @param feather   The feather value for color interpolation
     *
     * @return true if initialization was successful.
     */
    bool initBox(const Color4 inner, const Color4 outer, const Rect box, float radius, float feather);

    /**
     * Initializes a box gradient of the two colors.
     *
     * Box gradients paint the inner color in a rounded rectangle, and
     * then use a feather setting to transition to the outer color. The
     * box position and corner radius are given in texture coordinates.
     * That is, (0,0) is the top left corner of the gradient bounding
     * box and (1,1) is the bottom right corner. It is permissible for
     * these coordinates to be out of range (so negative values or greater
     * than 1). Such values will be interpretted accordingly.
     *
     * To be well-defined, the corner radius should be no larger than
     * half the width and height (at which point it defines an ellipse).
     * Shapes with abnormally large radii are undefined.
     *
     * The feather value acts like the inner and outer radius of a radial
     * gradient. If a line is drawn from the center of the round rectangle
     * to a corner, consider two segments.  The first starts at the corner
     * and moves towards the center of the rectangle half-feather in
     * distance.  The end of this segment is the end of the inner color
     * The second second starts at the corner and moves in the opposite
     * direction the same amount.  The end of this segement is the other
     * color.  In between, the colors are smoothly interpolated.
     *
     * So, if feather is 0, there is no gradient and the shift from
     * inner color to outer color is immediate.  On the other hand,
     * if feather is larger than the width and hight of the rectangle,
     * the inner color immediately transitions to the outer color.
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
    bool initBox(const Color4 inner, const Color4 outer, const Vec2 origin, const Size size, float radius, float feather);
    
    /**
     * Initializes this gradient to be a copy of the given gradient.
     *
     * @param grad  The gradient to copy
     *
     * @return true if initialization was successful.
     */
    bool initCopy(const std::shared_ptr<Gradient>& grad);

    /**
     * Initializes a gradient from the given JsonValue
     *
     * The JsonValue should a JSON object. It supports the following attributes:
     *
     *     "type":      One of 'linear', 'radial', or 'box'
     *     "inner":     The inner gradient color (string or 4-element array 0..255)
     *     "outer":     The outer gradient color (string or 4-element array 0..255)
     *     "center":    A two-element array representing the gradient center
     *     "extent":    A two-element array representing the gradient extent
     *     "radius":    A number representing the radius of the inner color
     *                  (radial and box gradients only)
     *     "feather":   A number representing the feather value (box gradients only)
     *
     * All values are optional. Note, however, that specifying no values results in
     * a solid white color, and not specifying the "center" and/or "extent" will
     * produce a solid color of the inner color.
     *
     * For a linear gradient, the "center" is the start and the "extent" is the end.
     * All other values are ignored. For a radial gradient, the "extent" defines the
     * outer radius, while the "radius" is the "radius" of the inner color. If radius
     * is not specified, then the inner and outer radius are the same.
     *
     * Finally for box gradients, the "center" is the center, while the "extent" defines
     * the width and height.  All other values are interpretted normally.
     *
     * @param data      The JSON object specifying the gradient
     *
     * @return true if initialization was successful.
     */
    bool initWithData(const std::shared_ptr<JsonValue>& data);


#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a new degenerate gradient of the given color.
     *
     * @param color The gradient color
     *
     * @return a new degenerate gradient of the given color.
     */
    static std::shared_ptr<Gradient> alloc(const Color4 color) {
        std::shared_ptr<Gradient> result = std::make_shared<Gradient>();
        return (result->init(color) ? result : nullptr);
    }
    
    /**
     * Returns a new linear gradient of the two colors.
     *
     * In a linear gradient, the inner starts at position start, and
     * transitions to the outer color at position end. The transition
     * is along the vector end-start.
     *
     * The values start and end are specified in texture coordinates.
     * That is, (0,0) is the top left corner of the gradient bounding
     * box and (1,1) is the bottom right corner.  It is permissible
     * to have coordinates out of range (so negative or greater than
     * 1). Such values will be interpretted accordingly.
     *
     * @param inner The inner gradient color
     * @param outer The outer gradient color
     * @param start The start position of the inner color
     * @param end   The start position of the outer color
     *
     * @return a new linear gradient of the two colors.
     */
    static std::shared_ptr<Gradient> allocLinear(const Color4 inner, const Color4 outer,
                                                 const Vec2 start, const Vec2 end) {
        std::shared_ptr<Gradient> result = std::make_shared<Gradient>();
        return (result->initLinear(inner,outer,start,end) ? result : nullptr);
    }

    /**
     * Returns a new simple radial gradient of the two colors.
     *
     * In a simple radial gradient, the inner color starts at the center
     * and transitions smoothly to the outer color at the given radius.
     *
     * The center and radius are specified in texture coordinates.
     * That is, (0,0) is the top left corner of the gradient bounding
     * box and (1,1) is the bottom right corner.  It is permissible
     * to a center value out of range (so coordinates negative or
     * greater than 1). Such values will be interpretted accordingly.
     *
     * @param inner     The inner gradient color
     * @param outer     The outer gradient color
     * @param center    The center of the radial gradient
     * @param radius    The radius for the outer color
     *
     * @return  a new simple radial gradient of the two colors.
     */
    static std::shared_ptr<Gradient> allocRadial(const Color4 inner, const Color4 outer,
                                                 const Vec2 center, float radius) {
        std::shared_ptr<Gradient> result = std::make_shared<Gradient>();
        return (result->initRadial(inner,outer,center,radius) ? result : nullptr);
    }
    
    /**
     * Returns a new general radial gradient of the two colors.
     *
     * In a general radial gradient, the inner color starts at the center
     * and continues to the inner radius.  It then transitions smoothly
     * to the outer color at the outer radius.
     *
     * The center and radii are all specified in texture coordinates.
     * That is, (0,0) is the top left corner of the gradient bounding
     * box and (1,1) is the bottom right corner.  It is permissible
     * to a center value out of range (so coordinates negative or
     * greater than 1). Such value will be interpretted accordingly.
     *
     * @param inner     The inner gradient color
     * @param outer     The outer gradient color
     * @param center    The center of the radial gradient
     * @param iradius   The radius for the inner color
     * @param oradius   The radius for the outer color
     *
     * @return a new general radial gradient of the two colors.
     */
    static std::shared_ptr<Gradient> allocRadial(const Color4 inner, const Color4 outer,
                                                 const Vec2 center, float iradius, float oradius) {
        std::shared_ptr<Gradient> result = std::make_shared<Gradient>();
        return (result->initRadial(inner,outer,center,iradius,oradius) ? result : nullptr);
    }

    /**
     * Returns a new box gradient of the two colors.
     *
     * Box gradients paint the inner color in a rounded rectangle, and
     * then use a feather setting to transition to the outer color. The
     * box position and corner radius are given in texture coordinates.
     * That is, (0,0) is the top left corner of the gradient bounding
     * box and (1,1) is the bottom right corner. It is permissible for
     * these coordinates to be out of range (so negative values or greater
     * than 1). Such values will be interpretted accordingly.
     *
     * To be well-defined, the corner radius should be no larger than
     * half the width and height (at which point it defines an ellipse).
     * Shapes with abnormally large radii are undefined.
     *
     * The feather value acts like the inner and outer radius of a radial
     * gradient. If a line is drawn from the center of the round rectangle
     * to a corner, consider two segments.  The first starts at the corner
     * and moves towards the center of the rectangle half-feather in
     * distance.  The end of this segment is the end of the inner color
     * The second second starts at the corner and moves in the opposite
     * direction the same amount.  The end of this segement is the other
     * color.  In between, the colors are smoothly interpolated.
     *
     * So, if feather is 0, there is no gradient and the shift from
     * inner color to outer color is immediate.  On the other hand,
     * if feather is larger than the width and hight of the rectangle,
     * the inner color immediately transitions to the outer color.
     *
     * @param inner     The inner gradient color
     * @param outer     The outer gradient color
     * @param box       The bounds of the rounded rectangle.
     * @param radius    The corner radius of the rounded rectangle.
     * @param feather   The feather value for color interpolation
     *
     * @return a new box gradient of the two colors.
     */
    static std::shared_ptr<Gradient> allocBox(const Color4 inner, const Color4 outer,
                                              const Rect box, float radius, float feather) {
        std::shared_ptr<Gradient> result = std::make_shared<Gradient>();
        return (result->initBox(inner,outer,box,radius,feather) ? result : nullptr);
    }

    /**
     * Returns a new box gradient of the two colors.
     *
     * Box gradients paint the inner color in a rounded rectangle, and
     * then use a feather setting to transition to the outer color. The
     * box position and corner radius are given in texture coordinates.
     * That is, (0,0) is the top left corner of the gradient bounding
     * box and (1,1) is the bottom right corner. It is permissible for
     * these coordinates to be out of range (so negative values or greater
     * than 1). Such values will be interpretted accordingly.
     *
     * To be well-defined, the corner radius should be no larger than
     * half the width and height (at which point it defines an ellipse).
     * Shapes with abnormally large radii are undefined.
     *
     * The feather value acts like the inner and outer radius of a radial
     * gradient. If a line is drawn from the center of the round rectangle
     * to a corner, consider two segments.  The first starts at the corner
     * and moves towards the center of the rectangle half-feather in
     * distance.  The end of this segment is the end of the inner color
     * The second second starts at the corner and moves in the opposite
     * direction the same amount.  The end of this segement is the other
     * color.  In between, the colors are smoothly interpolated.
     *
     * So, if feather is 0, there is no gradient and the shift from
     * inner color to outer color is immediate.  On the other hand,
     * if feather is larger than the width and hight of the rectangle,
     * the inner color immediately transitions to the outer color.
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
    static std::shared_ptr<Gradient> allocBox(const Color4 inner, const Color4 outer,
                                              const Vec2 origin, const Size size,
                                              float radius, float feather) {
        std::shared_ptr<Gradient> result = std::make_shared<Gradient>();
        return (result->initBox(inner,outer,origin,size,radius,feather) ? result : nullptr);
    }
    
    /**
     * Returns a new copy of the given gradient.
     *
     * @param grad  The gradient to copy
     *
     * @return a new copy of the given gradient.
     */
    static std::shared_ptr<Gradient> allocCopy(const std::shared_ptr<Gradient>& grad) {
        std::shared_ptr<Gradient> result = std::make_shared<Gradient>();
        return (result->initCopy(grad) ? result : nullptr);
    }

    /**
     * Returns a new gradient from the given JsonValue
     *
     * The JsonValue should a JSON object. It supports the following attributes:
     *
     *     "type":      One of 'linear', 'radial', or 'box'
     *     "inner":     The inner gradient color (string or 4-element array 0..255)
     *     "outer":     The outer gradient color (string or 4-element array 0..255)
     *     "center":    A two-element array representing the gradient center
     *     "extent":    A two-element array representing the gradient extent
     *     "radius":    A number representing the radius of the inner color
     *                  (radial and box gradients only)
     *     "feather":   A number representing the feather value (box gradients only)
     *
     * All values are optional. Note, however, that specifying no values results in
     * a solid white color, and not specifying the "center" and/or "extent" will
     * produce a solid color of the inner color.
     *
     * For a linear gradient, the "center" is the start and the "extent" is the end.
     * All other values are ignored. For a radial gradient, the "extent" defines the
     * outer radius, while the "radius" is the "radius" of the inner color. If radius
     * is not specified, then the inner and outer radius are the same.
     *
     * Finally for box gradients, the "center" is the center, while the "extent" defines
     * the width and height.  All other values are interpretted normally.
     *
     * @param data      The JSON object specifying the gradient
     *
     * @return a new gradient from the given JsonValue
     */
    static std::shared_ptr<Gradient> allocWithData(const std::shared_ptr<JsonValue>& data) {
        std::shared_ptr<Gradient> result = std::make_shared<Gradient>();
        return (result->initWithData(data) ? result : nullptr);
    }

#pragma mark -
#pragma mark Setters
    /**
     * Sets this gradient to be a copy of the given one.
     *
     * @param other  The gradient to copy
     *
     * @return this gradient, returned for chaining
     */
    Gradient& operator=(const Gradient& other) { return set(other); }

    /**
     * Sets this gradient to be have the resources of the given one.
     *
     * The original gradient is no longer safe to use after
     * calling this operator.
     *
     * @param other  The gradient to take from
     *
     * @return this gradient, returned for chaining
     */
    Gradient& operator=(Gradient&& other) { return set(other); }
    
    /**
     * Sets this to be a degenerate gradient with the given color.
     *
     * The inner color and outer color will be the same, so there
     * will be no transition.
     *
     * @param color  The gradient color
     *
     * @return this gradient, returned for chaining
     */
    Gradient& operator=(const Color4 color) { return set(color); }

    /**
     * Sets this gradient to be a copy of the given one.
     *
     * @param grad  The gradient to copy
     *
     * @return this gradient, returned for chaining
     */
    Gradient& set(const Gradient& grad);
    
    /**
     * Sets this gradient to be a copy of the given one.
     *
     * @param grad  The gradient to copy
     *
     * @return this gradient, returned for chaining
     */
    Gradient& set(const std::shared_ptr<Gradient>& grad) {
        return set(*grad);
    }
    
    /**
     * Sets this to be a degenerate gradient with the given color.
     *
     * The inner color and outer color will be the same, so there
     * will be no transition.
     *
     * @param color  The gradient color
     *
     * @return this gradient, returned for chaining
     */
    Gradient& set(const Color4 color);

    /**
     * Sets this to be a linear gradient of the two colors.
     *
     * In a linear color, the inner starts at position start, and
     * transitions to the outer color at position end. The transition
     * is along the vector end-start.
     *
     * The values start and end are specified in texture coordinates.
     * That is, (0,0) is the top left corner of the gradient bounding
     * box and (1,1) is the bottom right corner.  It is permissible
     * to have coordinates out of range (so negative or greater than
     * 1). Such values will be interpretted accordingly.
     *
     * @param inner The inner gradient color
     * @param outer The outer gradient color
     * @param start The start position of the inner color
     * @param end   The start position of the outer color
     *
     * @return this gradient, returned for chaining
     */
    Gradient& set(const Color4 inner, const Color4 outer, const Vec2 start, const Vec2 end);

    /**
     * Sets this to be a simple radial gradient of the two colors.
     *
     * In a simple radial gradient, the inner color starts at the center
     * and transitions smoothly to the outer color at the given radius.
     *
     * The center and radius are specified in texture coordinates.
     * That is, (0,0) is the top left corner of the gradient bounding
     * box and (1,1) is the bottom right corner.  It is permissible
     * to a center value out of range (so coordinates negative or
     * greater than 1). Such values will be interpretted accordingly.
     *
     * @param inner     The inner gradient color
     * @param outer     The outer gradient color
     * @param center    The center of the radial gradient
     * @param radius    The radius for the outer color
     *
     * @return this gradient, returned for chaining
     */
    Gradient& set(const Color4 inner, const Color4 outer, const Vec2 center, float radius);
    
    /**
     * Sets this to be a general radial gradient of the two colors.
     *
     * In a general radial gradient, the inner color starts at the center
     * and continues to the inner radius.  It then transitions smoothly
     * to the outer color at the outer radius.
     *
     * The center and radii are all specified in texture coordinates.
     * That is, (0,0) is the top left corner of the gradient bounding
     * box and (1,1) is the bottom right corner.  It is permissible
     * to a center value out of range (so coordinates negative or
     * greater than 1). Such value will be interpretted accordingly.
     *
     * @param inner     The inner gradient color
     * @param outer     The outer gradient color
     * @param center    The center of the radial gradient
     * @param iradius   The radius for the inner color
     * @param oradius   The radius for the outer color
     *
     * @return this gradient, returned for chaining
     */
    Gradient& set(const Color4 inner, const Color4 outer, const Vec2 center, float iradius, float oradius);

    /**
     * Sets this to be a box gradient of the two colors.
     *
     * Box gradients paint the inner color in a rounded rectangle, and
     * then use a feather setting to transition to the outer color. The
     * box position and corner radius are given in texture coordinates.
     * That is, (0,0) is the top left corner of the gradient bounding
     * box and (1,1) is the bottom right corner. It is permissible for
     * these coordinates to be out of range (so negative values or greater
     * than 1). Such values will be interpretted accordingly.
     *
     * To be well-defined, the corner radius should be no larger than
     * half the width and height (at which point it defines an ellipse).
     * Shapes with abnormally large radii are undefined.
     *
     * The feather value acts like the inner and outer radius of a radial
     * gradient. If a line is drawn from the center of the round rectangle
     * to a corner, consider two segments.  The first starts at the corner
     * and moves towards the center of the rectangle half-feather in
     * distance.  The end of this segment is the end of the inner color
     * The second second starts at the corner and moves in the opposite
     * direction the same amount.  The end of this segement is the other
     * color.  In between, the colors are smoothly interpolated.
     *
     * So, if feather is 0, there is no gradient and the shift from
     * inner color to outer color is immediate.  On the other hand,
     * if feather is larger than the width and hight of the rectangle,
     * the inner color immediately transitions to the outer color.
     *
     * @param inner     The inner gradient color
     * @param outer     The outer gradient color
     * @param box       The bounds of the rounded rectangle.
     * @param radius    The corner radius of the rounded rectangle.
     * @param feather   The feather value for color interpolation
     *
     * @return this gradient, returned for chaining
     */
    Gradient& set(const Color4 inner, const Color4 outer, const Rect box, float radius, float feather);

    /**
     * Sets this to be a box gradient of the two colors.
     *
     * Box gradients paint the inner color in a rounded rectangle, and
     * then use a feather setting to transition to the outer color. The
     * box position and corner radius are given in texture coordinates.
     * That is, (0,0) is the top left corner of the gradient bounding
     * box and (1,1) is the bottom right corner. It is permissible for
     * these coordinates to be out of range (so negative values or greater
     * than 1). Such values will be interpretted accordingly.
     *
     * To be well-defined, the corner radius should be no larger than
     * half the width and height (at which point it defines an ellipse).
     * Shapes with abnormally large radii are undefined.
     *
     * The feather value acts like the inner and outer radius of a radial
     * gradient. If a line is drawn from the center of the round rectangle
     * to a corner, consider two segments.  The first starts at the corner
     * and moves towards the center of the rectangle half-feather in
     * distance.  The end of this segment is the end of the inner color
     * The second second starts at the corner and moves in the opposite
     * direction the same amount.  The end of this segement is the other
     * color.  In between, the colors are smoothly interpolated.
     *
     * So, if feather is 0, there is no gradient and the shift from
     * inner color to outer color is immediate.  On the other hand,
     * if feather is larger than the width and hight of the rectangle,
     * the inner color immediately transitions to the outer color.
     *
     * @param inner     The inner gradient color
     * @param outer     The outer gradient color
     * @param origin    The origin of the rounded rectangle.
     * @param size      The size of the rounded rectangle.
     * @param radius    The corner radius of the rounded rectangle.
     * @param feather   The feather value for color interpolation
     *
     * @return this gradient, returned for chaining
     */
    Gradient& set(const Color4 inner, const Color4 outer, const Vec2 origin, const Size size, float radius, float feather);

#pragma mark -
#pragma mark Attributes
    /**
     * Returns the transform component of this gradient
     *
     * The transform maps the origin of the current coordinate system to
     * the center and rotation of the rounded rectangular box with the 
     * inner color. Applying further transforms will adjust the gradient
     * in texture space.
     *
     * The transform is primarily for representing rotation.  It typically
     * only has a scale component when the gradient is linear.
     *
     * If this transform is passed directly to a gradient shader, it should
     * be inverted first.  If you really need to pass individual components
     * to a shader, you should use {@link getComponents} instead.
     *
     * @return the transform component of this gradient
     */
    Affine2 getTransform() const;

    /**
     * Sets the transform component of this gradient
     *
     * The transform maps the origin of the current coordinate system to
     * the center and rotation of the rounded rectangular box with the 
     * inner color. Applying further transforms will adjust the gradient
     * in texture space.
     *
     * The transform is primarily for representing rotation.  It typically
     * only has a scale component when the gradient is linear.
     *
     * If this transform is passed directly to a gradient shader, it should
     * be inverted first.  If you really need to pass individual components
     * to a shader, you should use {@link getComponents} instead.
     *
     * @param transform The transform component of this gradient
     */
    void setTransform(const Affine2& transform);

    /**
     * Sets the transform component of this gradient
     *
     * The transform maps the origin of the current coordinate system to
     * the center and rotation of the rounded rectangular box with the 
     * inner color. Applying further transforms will adjust the gradient
     * in texture space.
     *
     * The transform is primarily for representing rotation.  It typically
     * only has a scale component when the gradient is linear.
     *
     * If this transform is passed directly to a gradient shader, it should
     * be inverted first.  If you really need to pass individual components
     * to a shader, you should use {@link getComponents} instead.
     *
     * @param transform The transform component of this gradient
     */
    void setTransform(const Mat4& transform);

    /**
     * Returns the inner color of this gradient
     *
     * The inner color is the color inside of the rounded rectangle
     * defining the gradient.
     *
     * @return the inner color of this gradient
     */
    Color4 getInnerColor() const { return (Color4)_inner; }
    
    /**
     * Sets the inner color of this gradient
     *
     * The inner color is the color inside of the rounded rectangle
     * defining the gradient.
     *
     * @param color The inner color of this gradient
     */
    void setInnerColor(const Color4 color) { _inner = color; }

    /**
     * Returns the outer color of this gradient
     *
     * The outer color is the color outside of the rounded rectangle
     * defining the gradient.
     *
     * @return the outer color of this gradient
     */
    Color4 getOuterColor() const { return (Color4)_outer; }
    
    /**
     * Sets the outer color of this gradient
     *
     * The outer color is the color outside of the rounded rectangle
     * defining the gradient.
     *
     * @param color The outer color of this gradientt
     */
    void setOuterColor(const Color4 color) { _outer = color; }

    /**
     * Returns the extent of this gradient
     *
     * The extent is the vector from the center of the rounded
     * rectangle to one of its corners.  It defines the size of
     * the rounded rectangle.
     *
     * @return the extent of this gradient
     */
    Vec2 getExtent() const { return _extent; }

    /**
     * Sets the extent of this gradient
     *
     * The extent is the vector from the center of the rounded
     * rectangle to one of its corners.  It defines the size of
     * the rounded rectangle.
     *
     * @param extent    The extent of this gradient
     */
    void setExtent(const Vec2 extent) { _extent = extent; }

    /**
     * Returns the corner radius of the gradient rectangle
     *
     * The corner radius is the radius of the circle inscribed
     * in (each) corner of the rounded rectangle.
     *
     * To be well-defined, it should be no more than half the width
     * and height. When it is equal to both half the width and
     * half the height, the rectangle becomes a circle. For large
     * values this inner rectangle will collapse and disappear
     * completely.
     *
     * @return the corner radius of the gradient rectangle
     */
    float getRadius() const { return _radius; }

    /**
     * Sets the corner radius of the gradient rectangle
     *
     * The corner radius is the radius of the circle inscribed
     * in (each) corner of the rounded rectangle.
     *
     * To be well-defined, it should be no more than half the width
     * and height. When it is equal to both half the width and
     * half the height, the rectangle becomes a circle. For large
     * values this inner rectangle will collapse and disappear
     * completely.
     *
     * @param radius    The corner radius of the gradient rectangle
     */
    void setRadius(float radius) { _radius = radius; }

    /**
     * Returns the feather value for this gradient.
     *
     * The feature value is perhaps the trickiest value to understand.
     * This value acts like the inner and outer radius of a radial
     * gradient. If a line is drawn from the center of the round rectangle
     * to a corner, consider two segments.  The first starts at the corner
     * and moves towards the center of the rectangle half-feather in
     * distance.  The end of this segment is the end of the inner color
     * The second second starts at the corner and moves in the opposite
     * direction the same amount.  The end of this segement is the other
     * color.  In between, the colors are smoothly interpolated.
     *
     * So, if feather is 0, there is no gradient and the shift from
     * inner color to outer color is immediate.  On the other hand,
     * if feather is larger than the width and hight of the rectangle,
     * the inner color immediately transitions to the outer color.
     *
     * @return the feather value for this gradient.
     */
    float getFeather() const { return _feather; }

    /**
     * Sets the feather value for this gradient.
     *
     * The feature value is perhaps the trickiest value to understand.
     * This value acts like the inner and outer radius of a radial
     * gradient. If a line is drawn from the center of the round rectangle
     * to a corner, consider two segments.  The first starts at the corner
     * and moves towards the center of the rectangle half-feather in
     * distance.  The end of this segment is the end of the inner color
     * The second second starts at the corner and moves in the opposite
     * direction the same amount.  The end of this segement is the other
     * color.  In between, the colors are smoothly interpolated.
     *
     * So, if feather is 0, there is no gradient and the shift from
     * inner color to outer color is immediate.  On the other hand,
     * if feather is larger than the width and hight of the rectangle,
     * the inner color immediately transitions to the outer color.
     *
     * @param feather   The feather value for this gradient.
     */
    void setFeather(float feather) { _feather = feather; }


#pragma mark -
#pragma mark Transforms
    /**
     * Applies a rotation to this gradient.
     *
     * The rotation is in radians, counter-clockwise about the given axis.
     *
     * @param angle The angle (in radians).
     *
     * @return This gradient, after rotation.
     */
    Gradient& rotate(float angle);
    
    /**
     * Applies a uniform scale to this gradient.
     *
     * @param value The scalar to multiply by.
     *
     * @return This gradient, after scaling.
     */
    Gradient& scale(float value);
    
    /**
     * Applies a non-uniform scale to this gradient.
     *
     * @param s        The vector storing the individual scaling factors
     *
     * @return This gradient, after scaling.
     */
    Gradient& scale(const Vec2 s);
    
    /**
     * Applies a non-uniform scale to this gradient.
     *
     * @param sx    The amount to scale along the x-axis.
     * @param sy    The amount to scale along the y-axis.
     *
     * @return This gradient, after scaling.
     */
    Gradient& scale(float sx, float sy) {
        return scale(Vec2(sx,sy));
    }
    
    /**
     * Applies a translation to this gradient.
     *
     * The translation should be in texture coordinates, which (generally)
     * have values 0 to 1.
     *
     * @param t     The vector storing the individual translation offsets
     *
     * @return This gradient, after translation.
     */
    Gradient& translate(const Vec2 t);
    
    /**
     * Applies a translation to this gradient.
     *
     * The translation should be in texture coordinates, which (generally)
     * have values 0 to 1.
     *
     * @param tx    The translation offset for the x-axis.
     * @param ty    The translation offset for the y-axis.
     *
     * @return This gradient, after translation.
     */
    Gradient& translate(float tx, float ty) {
        return translate(Vec2(tx,ty));
    }

    /**
     * Applies the given transform to this gradient.
     *
     * This transform is applied after the existing gradient transform
     * (which is natural, since the transform defines the gradient shape).
     * To pre-multiply a transform, set the transform directly.
     *
     * @param mat The matrix to multiply by.
     *
     * @return A reference to this (modified) Gradient for chaining.
     */
    Gradient& multiply(const Mat4& mat) {
        return *this *= mat;
    }

    /**
     * Applies the given transform to this gradient.
     *
     * This transform is applied after the existing gradient transform
     * (which is natural, since the transform defines the gradient shape).
     * To pre-multiply a transform, set the transform directly.
     *
     * @param aff The matrix to multiply by.
     *
     * @return A reference to this (modified) Gradient for chaining.
     */
    Gradient& multiply(const Affine2& aff) {
        return *this *= aff;
    }
    
    /**
     * Applies the given transform to this gradient.
     *
     * This transform is applied after the existing gradient transform
     * (which is natural, since the transform defines the gradient shape).
     * To pre-multiply a transform, set the transform directly.
     *
     * @param mat The matrix to multiply by.
     *
     * @return A reference to this (modified) Gradient for chaining.
     */
    Gradient& operator*=(const Mat4& mat);

    /**
     * Applies the given transform to this gradient.
     *
     * This transform is applied after the existing gradient transform
     * (which is natural, since the transform defines the gradient shape).
     * To pre-multiply a transform, set the transform directly.
     *
     * @param aff The matrix to multiply by.
     *
     * @return A reference to this (modified) Gradient for chaining.
     */
    Gradient& operator*=(const Affine2& aff);

    /**
     * Returns a copy of the gradient transformed by the given matrix.
     *
     * The matrix transform is applied after the existing gradient transform
     * (which is natural, since the transform defines the gradient shape).
     * To pre-multiply a transform, set the transform directly.
     *
     * Note: This does not modify the gradient.
     *
     * @param mat   The transform to multiply by.
     *
     * @return a copy of the gradient transformed by the given matrix.
     */
    Gradient operator*(const Mat4& mat) const {
        Gradient result = *this;
        return result *= mat;
    }
    
    /**
     * Returns a copy of the gradient transformed by the given matrix.
     *
     * The matrix transform is applied after the existing gradient transform
     * (which is natural, since the transform defines the gradient shape).
     * To pre-multiply a transform, set the transform directly.
     *
     * Note: This does not modify the gradient.
     *
     * @param aff   The transform to multiply by.
     *
     * @return a copy of the gradient transformed by the given matrix.
     */
    Gradient operator*(const Affine2& aff) const {
        Gradient result = *this;
        return result *= aff;
    }

#pragma mark -
#pragma mark Conversion
    /**
     * Reads the gradient into the provided array
     *
     * The gradient is written to the given array in std140 format.
     * That is (1) 12 floats for the affine transform (as a 3x3
     * homogenous matrix), (2) 4 floats for the inner color, (3)
     * 4 floats for the outer color, (4) 2 floats for the extent,
     * (5) 1 float for the corner radius, and (6) 1 float for the
     * feather value.  Values are written in this order.
     *
     * @param array     The array to store the values
     *
     * @return a reference to the array for chaining
     */
    float* getData(float* array) const;

    /**
     * Reads the gradient into the provided array
     *
     * The gradient is written to the array so that it can be passed
     * the the shader one component at a time (e.g. NOT in std140 format).
     * It differs from getData in that it only uses 9 floats for the
     * affine transform (as a 3x3 homogenous matrix).
     *
     * @param array     The array to store the values
     *
     * @return a reference to the array for chaining
     */
    float* getComponents(float* array) const;
    
    /**
     * Returns a string representation of this gradient for debuggging purposes.
     *
     * If verbose is true, the string will include class information.  This
     * allows us to unambiguously identify the class.
     *
     * @param verbose Whether to include class information
     *
     * @return a string representation of this gradient for debuggging purposes.
     */
    std::string toString(bool verbose = false) const;
        
    /** Cast from Gradient to a string. */
    operator std::string() const { return toString(); }
};

}
#endif /* __CU_GRADIENT_H__ */
