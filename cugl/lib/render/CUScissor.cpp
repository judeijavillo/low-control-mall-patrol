//
//  CUScissor.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides support for a scissor mask that supports rotation and
//  other transforms.  A scissor mask is a rectangular region that whose size
//  is defined by the extent attribute.  The associated transform transforms
//  this rectangle about its center.
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
//  Version: 3/25/20
#include <sstream>
#include <cugl/render/CUScissor.h>
#include <cugl/util/CUDebug.h>
#include <cugl/util/CUStrings.h>
#include <cugl/math/CUMat4.h>

using namespace cugl;

#pragma mark -
#pragma mark Constructors
/**
 * Creates a degenerate scissor of size 0.
 */
Scissor::Scissor() {
    setZero();
}

/**
 * Creates a copy with the resources of the given scissor mask.
 *
 * The original scissor mask is no longer safe to use after calling this
 * constructor.
 *
 * @param mask    The scissor mask to take from
  */
Scissor::Scissor(Scissor&& mask) {
    _scissor   = mask._scissor;
    _inverse   = mask._inverse;
    _transform = mask._transform;
    _bounds  = mask._bounds;
    _fringe  = mask._fringe;
}

/**
 * Deletes the scissor mask and resets all attributes.
 *
 * You must reinitialize the scissor mask to use it.
 */
void Scissor::dispose() {
	setZero();
}

/**
 * Initializes a scissor with the given bounds and fringe.
 *
 * The fringe is the size of the scissor border in pixels.  A value less than
 * 0 gives a sharp transition, where larger values have more gradual transitions.
 *
 * @param rect      The scissor mask bounds
 * @param fringe    The size of the scissor border in pixels
 *
 * @return true if initialization was successful.
 */
bool Scissor::init(const Rect rect, float fringe) {
    set(rect,fringe);
    return true;
}

/**
 * Initializes a scissor with the given transformed bounds and fringe.
 *
 * The fringe is the size of the scissor border in pixels.  A value less than
 * 0 gives a sharp transition, where larger values have more gradual transitions.
 *
 * @param rect      The scissor mask bounds
 * @param aff       The scissor mask transform
 * @param fringe    The size of the scissor border in pixels
 *
 * @return true if initialization was successful.
 */
bool Scissor::init(const Rect rect, const Affine2& aff, float fringe) {
    set(rect,aff,fringe);
    return true;
}

/**
 * Initializes a scissor with the given transformed bounds and fringe.
 *
 * The fringe is the size of the scissor border in pixels.  A value less than
 * 0 gives a sharp transition, where larger values have more gradual transitions.
 *
 * @param rect      The scissor mask bounds
 * @param mat       The scissor mask transform
 * @param fringe    The size of the scissor border in pixels
 *
 * @return true if initialization was successful.
 */
bool Scissor::init(const Rect rect, const Mat4& mat, float fringe) {
    set(rect,mat,fringe);
    return true;
}

/**
 * Initializes this scissor mask to be a copy of the other.
 *
 * @param mask  The scissor mask to copy
 *
 * @return true if initialization was successful.
 */
bool Scissor::init(const std::shared_ptr<Scissor>& mask) {
    set(mask);
    return true;
}




#pragma mark -
#pragma mark Setters
/**
 * Sets this scissor mask to be a copy of the given one.
 *
 * @param mask    The scissor mask to copy
 *
 * @return this scissor mask, returned for chaining
 */
Scissor& Scissor::set(const Scissor& mask) {
    _scissor   = mask._scissor;
    _inverse   = mask._inverse;
    _transform = mask._transform;
    _bounds  = mask._bounds;
    _fringe  = mask._fringe;
    return *this;
}

/**
 * Sets the scissor mask to have the given bounds and fringe.
 *
 * Any previous transforms are dropped when this method is called.
 *
 * The fringe is the size of the scissor border in pixels.  A value less than
 * 0 gives a sharp transition, where larger values have more gradual transitions.
 *
 * @param rect         The scissor mask bounds
 * @param fringe    The size of the scissor border in pixels
 *
 * @return this scissor mask, returned for chaining
 */
Scissor& Scissor::set(const Rect rect, float fringe) {
    _transform.setIdentity();
    _bounds = rect;
    _fringe = fringe;
    recompute();
    return *this;
}

/**
 * Sets the scissor mask to have the given transformed bounds and fringe.
 *
 * Any previous transforms are dropped when this method is called.
 *
 * The fringe is the size of the scissor border in pixels.  A value less than
 * 0 gives a sharp transition, where larger values have more gradual transitions.
 *
 * @param rect      The scissor mask bounds
 * @param aff       The scissor mask transform
 * @param fringe    The size of the scissor border in pixels
 *
 * @return this scissor mask, returned for chaining
 */
Scissor& Scissor::set(const Rect rect, const Affine2& aff, float fringe) {
    _transform = aff;
    _bounds = rect;
    _fringe = fringe;
    recompute();
    return *this;
}

/**
 * Sets the scissor mask to have the given transformed bounds and fringe.
 *
 * Any previous transforms are dropped when this method is called.
 *
 * The fringe is the size of the scissor border in pixels.  A value less than
 * 0 gives a sharp transition, where larger values have more gradual transitions.
 *
 * @param rect      The scissor mask bounds
 * @param mat       The scissor mask transform
 * @param fringe    The size of the scissor border in pixels
 *
 * @return this scissor mask, returned for chaining
 */
Scissor& Scissor::set(const Rect rect, const Mat4& mat, float fringe) {
    _transform = mat;
    _bounds = rect;
    _fringe = fringe;
    recompute();
    return *this;
}

/**
 * Sets the bounding box of this scissor mask
 *
 * The bounding box is axis-aligned. It ignores the transform component
 * of the scissor mask.
 *
 * @param bounds    The bounding box of this scissor mask
 */
void Scissor::setBounds(const Rect bounds) {
    _bounds = bounds;
    recompute();
}

/**
 * Sets this to be a degenerate scissor of size 0.
 *
 * All pixels will be dropped by this mask.
 *
 * @return this scissor mask, returned for chaining
 */
Scissor& Scissor::setZero() {
    _inverse.setIdentity();
    _transform.setIdentity();
    _bounds.set(0,0,0,0);
    _fringe = 1;
    return *this;
}


#pragma mark -
#pragma mark Transforms
/**
 * Sets the transform component of this scissor mask
 *
 * If the scissor mask is not rotated or otherwise transformed, this
 * value should be the identity.
 *
 * This value only contains the transform on the scissor mask bounding
 * box.  It is not the same as the scissor matrix in a scissor shader.  Do
 * not pass this information directly to the shader.  Use either the method
 * {@link getData} or {@link getComponents} depending on whether or not you
 * need std140 representation.
 *
 * @param transform The transform component of this scissor mask
 */
void Scissor::setTransform(const Affine2& transform) {
    _transform = transform;
    recompute();
}

/**
 * Sets the transform component of this scissor mask
 *
 * If the scissor mask is not rotated or otherwise transformed, this
 * value should be the identity.
 *
 * This value only contains the transform on the scissor mask bounding
 * box.  It is not the same as the scissor matrix in a scissor shader.  Do
 * not pass this information directly to the shader.  Use either the method
 * {@link getData} or {@link getComponents} depending on whether or not you
 * need std140 representation.
 *
 * @param transform The transform component of this scissor mask
 */
void Scissor::setTransform(const Mat4& transform) {
    _transform = transform;
    recompute();
}

/**
 * Applies a rotation to this scissor mask.
 *
 * The rotation is in radians, counter-clockwise about the given axis.
 *
 * @param angle The angle (in radians).
 *
 * @return This scissor mask, after rotation.
 */
Scissor& Scissor::rotate(float angle)  {
    _transform.rotate(angle);
    recompute();
    return *this;
}

/**
 * Applies a uniform scale to this scissor mask.
 *
 * @param value The scalar to multiply by.
 *
 * @return This scissor mask, after scaling.
 */
Scissor& Scissor::scale(float value) {
    if (value == 0) {
        setZero();
    } else {
        _transform.scale(value);
    }
    recompute();
    return *this;
}


/**
 * Applies a non-uniform scale to this scissor mask.
 *
 * @param s        The vector storing the individual scaling factors
 *
 * @return This scissor mask, after scaling.
 */
Scissor& Scissor::scale(const Vec2 s) {
    if (s.x == 0 || s.y == 0) {
        setZero();
    } else {
        _transform.scale(s);
    }
    recompute();
    return *this;
}

/**
 * Applies a translation to this gradient.
 *
 * @param t     The vector storing the individual translation offsets
 *
 * @return This scissor mask, after translation.
 */
Scissor& Scissor::translate(const Vec2 t) {
    _transform.translate(t);
    recompute();
    return *this;
}

/**
 * Applies the given transform to this scissor mask.
 *
 * This transform is applied after the existing gradient transform
 * (which is natural, since the transform defines the gradient shape).
 * To pre-multiply a transform, set the transform directly.
 *
 * @param aff The matrix to multiply by.
 *
 * @return A reference to this (modified) scissor mask for chaining.
 */
Scissor& Scissor::operator*=(const Mat4& mat) {
    Affine2::multiply(_transform,mat,&_transform);
    recompute();
    return *this;
}

/**
 * Applies the given transform to this scissor mask.
 *
 * The matrix transform is applied after the existing scissor transform
 * (which is natural, since the transform defines the initial box bounds).
 * To pre-multiply a transform, set the transform directly.
 *
 * @param aff The matrix to multiply by.
 *
 * @return A reference to this (modified) scissor mask for chaining.
 */
Scissor& Scissor::operator*=(const Affine2& aff) {
    Affine2::multiply(_transform,aff,&_transform);
    recompute();
    return *this;
}


#pragma mark -
#pragma mark Scissor Intersection
/**
 * Intersects the given scissor mask with this one.
 *
 * The intersection will take place in the coordinate system of this scissor
 * mask. The other mask will be transformed to be in this coordinate space.
 * This transformation will compute the bounding box of the transformed scissor
 * and interesect it with the bounding box of this scissor.
 *
 * As long as the scissors have the same rotational angle, this will have the
 * expected effect of intersecting two scissors. However, if their rotational
 * angles differ, the transformed scissor will be the axis-aligned bounding
 * box (in the coordinate system of this scissor mask) of the original. This
 * my result in revealing areas once hidden.
 *
 * @param mask	The scissor mask to intersect with this one
 *
 * @return a reference to the scissor for chaining
 */
Scissor& Scissor::intersect(const Scissor& mask) {
    Affine2 transform = _transform;
    transform.invert();
    Affine2::multiply(mask._transform,transform,&transform);
    _bounds.intersect(mask._bounds * transform);
    recompute();
    return *this;
}

/**
 * Returns the intersection of the given scissor mask with this one.
 *
 * The intersection will take place in the coordinate system of this scissor
 * mask. The other mask will be transformed to be in this coordinate space.
 * This transformation will compute the bounding box of the transformed scissor
 * and interesect it with the bounding box of this scissor.
 *
 * As long as the scissors have the same rotational angle, this will have the
 * expected effect of intersecting two scissors. However, if their rotational
 * angles differ, the transformed scissor will be the axis-aligned bounding
 * box (in the coordinate system of this scissor mask) of the original. This
 * my result in revealing areas once hidden.
 *
 * This scissor mask will not be affected by this method.
 *
 * @param mask	The scissor mask to intersect with this one
 *
 * @return the intersection of the given scissor mask with this one.
 */
Scissor Scissor::getIntersection(const Scissor& mask) const {
    Affine2 transform = _transform;
    transform.invert();
    Affine2::multiply(mask._transform,transform,&transform);

    Scissor result(*this);
    result._bounds.intersect(mask._bounds * transform);
    result.recompute();
    return result;
}

/**
 * Returns the intersection of the given scissor mask with this one.
 *
 * The intersection will take place in the coordinate system of this scissor
 * mask. The other mask will be transformed to be in this coordinate space.
 * This transformation will compute the bounding box of the transformed scissor
 * and interesect it with the bounding box of this scissor.
 *
 * As long as the scissors have the same rotational angle, this will have the
 * expected effect of intersecting two scissors. However, if their rotational
 * angles differ, the transformed scissor will be the axis-aligned bounding
 * box (in the coordinate system of this scissor mask) of the original. This
 * my result in revealing areas once hidden.
 *
 * This scissor mask will not be affected by this method.
 *
 * @param mask	The scissor mask to intersect with this one
 *
 * @return the intersection of the given scissor mask with this one.
 */
std::shared_ptr<Scissor> Scissor::getIntersection(const std::shared_ptr<Scissor>& mask) const {
    Affine2 transform = _transform;
    transform.invert();
    Affine2::multiply(mask->_transform,transform,&transform);

    std::shared_ptr<Scissor> result = Scissor::alloc(_bounds,_transform,_fringe);
    result->_bounds.intersect(mask->_bounds * transform);
    result->recompute();
    return result;
}


#pragma mark -
#pragma mark Conversion
/**
 * Reads the scissor mask into the provided array
 *
 * The scissor mask is written to the given array in std140 format.
 * That is (1) 12 floats for the affine transform (as a 3x3 homogenous
 * matrix), (2) 2 floats for the extent, and (3) 2 floats for the
 * fringe (one for each axis). Values are written in this order.
 *
 * @param array     The array to store the values
 *
 * @return a reference to the array for chaining
 */
float* Scissor::getData(float* array) const {
    _inverse.get3x4(array);
    array[12] = _bounds.size.width/2;
    array[13] = _bounds.size.height/2;
    array[14] = sqrtf(_scissor.m[0]*_scissor.m[0] + _scissor.m[2]*_scissor.m[2]) / _fringe;
    array[15] = sqrtf(_scissor.m[1]*_scissor.m[1] + _scissor.m[3]*_scissor.m[3]) / _fringe;
    return array;
}

/**
 * Reads the scissor mask into the provided array
 *
 * The scissor mask is written to the array so that it can be passed
 * the the shader one component at a time (e.g. NOT in std140 format).
 * It differs from getData in that it only uses 9 floats for the
 * affine transform (as a 3x3 homogenous matrix).
 *
 * @param array     The array to store the values
 *
 * @return a reference to the array for chaining
 */
float* Scissor::getComponents(float* array) const {
    _inverse.get3x3(array);
    array[9 ] = _bounds.size.width/2;
    array[10] = _bounds.size.height/2;
    array[11] = sqrtf(_scissor.m[0]*_scissor.m[0] + _scissor.m[2]*_scissor.m[2]) / _fringe;
    array[12] = sqrtf(_scissor.m[1]*_scissor.m[1] + _scissor.m[3]*_scissor.m[3]) / _fringe;
    return array;
}

/**
 * Returns a string representation of this scissor for debuggging purposes.
 *
 * If verbose is true, the string will include class information.  This
 * allows us to unambiguously identify the class.
 *
 * @param verbose Whether to include class information
 *
 * @return a string representation of this scissor for debuggging purposes.
 */
std::string Scissor::toString(bool verbose) const {
    std::stringstream ss;
    if (verbose) {
        ss << "cugl::Scissor";
    }
    const int PRECISION = 8;
    ss << "\n";
    ss << "|  ";
    ss << strtool::to_string(_scissor.m[0]).substr(0,PRECISION);
    ss << ", ";
    ss << strtool::to_string(_scissor.m[2]).substr(0,PRECISION);
    ss << ", ";
    ss << strtool::to_string(_scissor.m[4]).substr(0,PRECISION);
    ss << "  |   |  ";
    ss << strtool::to_string(_bounds.size.width).substr(0,PRECISION);
    ss << "  |";
    ss << "\n";
    ss << "|  ";
    ss << strtool::to_string(_scissor.m[1]).substr(0,PRECISION);
    ss << ", ";
    ss << strtool::to_string(_scissor.m[3]).substr(0,PRECISION);
    ss << ", ";
    ss << strtool::to_string(_scissor.m[5]).substr(0,PRECISION);
    ss << "  |;  |  ";
    ss << strtool::to_string(_bounds.size.height).substr(0,PRECISION);
    ss << "  |; ";
    ss << strtool::to_string(_fringe).substr(0,PRECISION);
    ss << "\n";
    return ss.str();
}

/**
 * Recomputes the internal transform for OpenGL.
 */
void Scissor::recompute() {
    _scissor.setIdentity();
    _scissor.translate(_bounds.origin+_bounds.size/2);
    _scissor *= _transform;
    _inverse = _scissor;
    _inverse.invert();
}

    
