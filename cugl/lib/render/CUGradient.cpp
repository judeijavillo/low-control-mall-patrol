//
//  CUGradient.cpp
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
//  Version: 3/15/20
#include <sstream>
#include <cugl/render/CUGradient.h>
#include <cugl/util/CUDebug.h>
#include <cugl/util/CUStrings.h>
#include <cugl/math/CUMat4.h>
#include <cugl/assets/CUJsonValue.h>

using namespace cugl;

#pragma mark Constructors
/**
 * Creates a degenerate, white-colored gradient.
 */
Gradient::Gradient() : 
_radius(0),
_feather(0) {
    _inverse.setIdentity();
    _inner.set(1.0f,1.0f,1.0f,1.0f);
    _outer.set(1.0f,1.0f,1.0f,1.0f);
    _extent.setZero();
}

/**
 * Creates a copy with the resource of the given gradient.
 *
 * The original gradient is no longer safe to use after
 * calling this constructor.
 *
 * @param grad  The gradient to take from
 */
Gradient::Gradient(Gradient&& grad) {
    set(grad);
}

/**
 * Deletes the gradient and resets all attributes.
 *
 * You must reinitialize the gradient to use it.
 */
void Gradient::dispose() {
    _radius  = 0;
    _feather = 0;
    _inverse.setIdentity();
    _inner.set(1.0f,1.0f,1.0f,1.0f);
    _outer.set(1.0f,1.0f,1.0f,1.0f);
    _extent.setZero();
}

/**
 * Initializes a degenerate gradient of the given color.
 *
 * @param color The gradient color
 *
 * @return true if initialization was successful.
 */
bool Gradient::init(const Color4f color) {
    set(color);
    return true;
}

/**
 * Initializes a linear gradient of the two colors.
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
 * @return true if initialization was successful.
 */
bool Gradient::initLinear(const Color4 inner, const Color4 outer,
                          const Vec2 start, const Vec2 end) {
    set(inner,outer,start,end);
    return true;
}

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
bool Gradient::initRadial(const Color4 inner, const Color4 outer,
                          const Vec2 center, float radius) {
    set(inner,outer,center,radius);
    return true;
}

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
bool Gradient::initRadial(const Color4 inner, const Color4 outer,
                          const Vec2 center, float iradius, float oradius) {
    set(inner,outer,center,iradius,oradius);
    return true;
}

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
bool Gradient::initBox(const Color4 inner, const Color4 outer,
                       const Rect box, float radius, float feather) {
    set(inner,outer,box,radius,feather);
    return true;
}

bool Gradient::initBox(const Color4 inner, const Color4 outer,
                       const Vec2 origin, const Size size, float radius, float feather) {
    set(inner,outer,origin,size,radius,feather);
    return true;
}

/**
 * Initializes this gradient to be a copy of the given gradient.
 *
 * @param grad  The gradient to copy
 *
 * @return true if initialization was successful.
 */
bool Gradient::initCopy(const std::shared_ptr<Gradient>& grad) {
    set(grad);
    return true;
}

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
bool Gradient::initWithData(const std::shared_ptr<JsonValue>& data) {
    if (!data->isObject()) {
        CUAssertLog(false, "JSON data must be an object");
        return false;
    }
    
    Color4 inner;
    if (data->has("inner")) {
        JsonValue* col = data->get("inner").get();
        if (col->isString()) {
            inner.set(col->asString("#ffffff"));
        } else {
            CUAssertLog(col->size() >= 4, "'color' must be a four element number array");
            inner.r = col->get(0)->asInt(0);
            inner.g = col->get(1)->asInt(0);
            inner.b = col->get(2)->asInt(0);
            inner.a = col->get(3)->asInt(0);
        }
    }
    Color4 outer;
    if (data->has("outer")) {
        JsonValue* col = data->get("outer").get();
        if (col->isString()) {
            outer.set(col->asString("#ffffff"));
        } else {
            CUAssertLog(col->size() >= 4, "'color' must be a four element number array");
            outer.r = col->get(0)->asInt(0);
            outer.g = col->get(1)->asInt(0);
            outer.b = col->get(2)->asInt(0);
            outer.a = col->get(3)->asInt(0);
        }
    } else {
        outer = inner;
    }
    if (!data->has("type") || !data->has("center") || !data->has("extent")) {
        return init(inner);
    } else {
        std::string type = data->getString("type");
        JsonValue* pt = data->get("center").get();
        CUAssertLog(pt->size() >= 2, "'center' must have at least two numbers");
        Vec2 center;
        center.x = pt->get(0)->asFloat(0.0f);
        center.y = pt->get(1)->asFloat(0.0f);
        pt = data->get("extent").get();
        CUAssertLog(pt->size() >= 2, "'extent' must have at least two numbers");
        Vec2 extent;
        extent.x = pt->get(0)->asFloat(0.0f);
        extent.y = pt->get(1)->asFloat(0.0f);

        if (type == "linear") {
            return initLinear(inner, outer, center, extent);
        } else if (type == "radial") {
            float oradius = extent.distance(center);
            if (data->has("radius")) {
                return initRadial(inner, outer, center, data->getFloat("radius",0.0f), oradius);
            } else {
                return initRadial(inner, outer, center, oradius);
            }
        } else if (type == "box") {
            float radius  = data->getFloat("radius",0.0f);
            float feather = data->getFloat("feather",0.0f);
            Rect bounds;
            bounds.size = 2*(extent-center);
            bounds.origin = center-extent;
            return initBox(inner, outer, bounds, radius, feather);
        }
    }
    return init(inner);
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
Gradient& Gradient::set(const Gradient& grad) {
    _inverse = grad._inverse;
    _inner  = grad._inner;
    _outer  = grad._outer;
    _extent = grad._extent;
    _radius = grad._radius;
    _feather = grad._feather;
    return *this;
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
Gradient& Gradient::set(const Color4 color) {
    _inverse.setIdentity();
    _inner = color;
    _outer = color;
    return *this;
}

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
Gradient& Gradient::set(const Color4 inner, const Color4 outer, const Vec2 start, const Vec2 end) {
    float dx, dy, d;
    const float large = 1e5;
    _inverse.setZero();

    // Calculate transform aligned to the line
    dx = end.x - start.x;
    dy = end.y - start.y;
    d = sqrtf(dx*dx + dy*dy);
    if (d > 0.0001f) {
        dx /= d;
        dy /= d;
    } else {
        dx = 0;
        dy = 1;
    }
    _inverse.m[0] = dy; _inverse.m[1] = -dx;
    _inverse.m[2] = dx; _inverse.m[3] = dy;
    _inverse.m[4] = start.x - dx*large;
    _inverse.m[5] = start.y - dy*large;
    _inverse.invert();
    
    _inner = inner;
    _outer = outer;
    _extent.set(large, large +d*0.5f);
    _radius = 0.0f;
    _feather = d;
    return *this;
}

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
Gradient& Gradient::set(const Color4 inner, const Color4 outer, const Vec2 center, float radius) {
    _inverse.setIdentity();
    _inverse.m[4] = -center.x;
    _inverse.m[5] = -center.y;
    
    _inner = inner;
    _outer = outer;
    _extent.set(radius,radius);
    _radius  = radius;
    _feather = 0.0f;
    return *this;
}

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
Gradient& Gradient::set(const Color4 inner, const Color4 outer, const Vec2 center, float iradius, float oradius) {
    float r = (iradius+oradius)*0.5f;
    _inverse.setIdentity();
    _inverse.m[4] = -center.x;
    _inverse.m[5] = -center.y;
    
    _inner = inner;
    _outer = outer;
    _extent.set(r,r);
    _radius  = r;
    _feather = oradius-iradius;
    return *this;
}

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
Gradient& Gradient::set(const Color4 inner, const Color4 outer, const Rect box, float radius, float feather) {
    _inverse.setIdentity();
    _inverse.m[4] = box.origin.x+box.size.width*0.5f;
    _inverse.m[5] = box.origin.y+box.size.height*0.5f;
    _inverse.invert();

    _inner = inner;
    _outer = outer;
    _extent  = box.size*0.5f;
    _radius  = radius;
    _feather = feather;
    return *this;
}

Gradient& Gradient::set(const Color4 inner, const Color4 outer, const Vec2 origin, const Size size, float radius, float feather) {
    _inverse.setIdentity();
    _inverse.m[4] = origin.x+size.width*0.5f;
    _inverse.m[5] = origin.y+size.height*0.5f;
    _inverse.invert();

    _inner = inner;
    _outer = outer;
    _extent  = size*0.5f;
    _radius  = radius;
    _feather = feather;
    return *this;

}


#pragma mark -
#pragma mark Transforms
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
 * @return the transform component of this gradient
 */
Affine2 Gradient::getTransform() const {
    return _inverse.getInverse();
}

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
 * @param transform The transform component of this gradient
 */
void Gradient::setTransform(const Affine2& transform) {
    _inverse = transform;
    _inverse.invert();
}

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
 * @param transform The transform component of this gradient
 */
void Gradient::setTransform(const Mat4& transform) {
    _inverse = transform;
    _inverse.invert();
}

/**
 * Applies a rotation to this gradient.
 *
 * The rotation is in radians, counter-clockwise about the given axis.
 *
 * @param angle The angle (in radians).
 *
 * @return This gradient, after rotation.
 */
Gradient& Gradient::rotate(float angle) {
    Affine2 temp;
    temp.rotate(-angle);
    Affine2::multiply(temp,_inverse,&_inverse);
    return *this;
}

/**
 * Applies a uniform scale to this gradient.
 *
 * @param value The scalar to multiply by.
 *
 * @return This gradient, after scaling.
 */
Gradient& Gradient::scale(float value) {
    if (value == 0) {
        _inverse.setZero();
    }
    Affine2 temp;
    temp.scale(1/value);
    Affine2::multiply(temp,_inverse,&_inverse);
    return *this;
}

/**
 * Applies a non-uniform scale to this gradient.
 *
 * @param s        The vector storing the individual scaling factors
 *
 * @return This gradient, after scaling.
 */
Gradient& Gradient::scale(const Vec2 s) {
    if (s.x == 0 || s.y == 0) {
        _inverse.setZero();
    }
    Affine2 temp;
    temp.scale(1/s.x,1/s.y);
    Affine2::multiply(temp,_inverse,&_inverse);
    return *this;
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
Gradient& Gradient::translate(const Vec2 t) {
    Affine2 temp;
    temp.translate(-t.x,-t.y);
    Affine2::multiply(temp,_inverse,&_inverse);
    return *this;
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
Gradient& Gradient::operator*=(const Mat4& mat) {
    Affine2::multiply((Affine2)mat.getInverse(),_inverse,&_inverse);
    return *this;        
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
Gradient& Gradient::operator*=(const Affine2& aff) {
    Affine2::multiply(aff.getInverse(),_inverse,&_inverse);
    return *this;        
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
float* Gradient::getData(float* array) const {
    _inverse.get3x4(array);
    _inner.get(array+12);
    _outer.get(array+16);
    array[20] = _extent.x;
    array[21] = _extent.y;
    array[22] = _radius;
    array[23] = _feather;
    return array;
}

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
float* Gradient::getComponents(float* array) const {
    _inverse.get3x3(array);
    _inner.get(array+9);
    _outer.get(array+13);
    array[17] = _extent.x;
    array[18] = _extent.y;
    array[19] = _radius;
    array[20] = _feather;
    return array;
}

/**
 * Returns a string representation of this gradient for debuggging purposes.
 *
 * If verbose is true, the string will include class information.  This
 * allows us to unambiguously identify the class.
 *
 * @param verbose Whether to include class information
 *
 * @return a string representation of this vector for debuggging purposes.
 */
std::string Gradient::toString(bool verbose) const {
    std::stringstream ss;
    ss << (verbose ? "cugl::Gradient[" : "[");
    ss << _inverse.toString();
    ss << "; ";
    ss << _extent.toString();
    ss << "; ";
    ss << _radius;
    ss << "; ";
    ss << _feather;
    ss << "]";
    return ss.str();
}
