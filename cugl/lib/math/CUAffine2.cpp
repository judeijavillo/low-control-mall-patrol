//
//  CUAffine2.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides support for a 2d affine transform.  It has some of the
//  functionality of Mat4, with a lot less memory footprint.  Profiling suggests
//  that this class is 20% faster than Mat4 when only 2d functionality is needed.
//
//  Because math objects are intended to be on the stack, we do not provide
//  any shared pointer support in this class.
//
//  This module is based on an original file from GamePlay3D: http://gameplay3d.org.
//  It has been modified to support the CUGL framework.
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
//  Version: 3/12/20
#include <sstream>
#include <algorithm>
#include <SDL/SDL.h>
#include <cugl/util/CUDebug.h>
#include <cugl/util/CUStrings.h>
#include <cugl/math/CUAffine2.h>
#include <cugl/math/CUMat4.h>
#include <cugl/math/CURect.h>

using namespace cugl;

#define MATRIX_SIZE ( sizeof(float) *  6)

#pragma mark -
#pragma mark Constructors
/**
 * Creates the identity transform.
 *
 *     1  0  0
 *     0  1  0
 */
Affine2::Affine2() {
    std::memset(m, 0, MATRIX_SIZE);
    m[0] = m[3] = 1.0f;
}

/**
 * Constructs a matrix initialized to the specified values.
 *
 * @param m11   The first element of the first row.
 * @param m12   The second element of the first row.
 * @param m21   The first element of the second row.
 * @param m22   The second element of the second row.
 * @param tx    The translation offset for the x-coordinate.
 * @param ty    The translation offset for the y-coordinate.
 */
Affine2::Affine2(float m11, float m12, float m21, float m22, float tx, float ty) {
    set(m11,m12,m21,m22,tx,ty);
}


/**
 * Creates a matrix initialized to the specified column-major array.
 *
 * The passed-in array is six elements in column-major order, with
 * the last two elements being the translation offset.  Hence the
 * memory layout of the array is as follows:
 *
 *     0   2   4
 *     1   3   5
 *
 * @param mat An array containing 6 elements in column-major order.
 */
Affine2::Affine2(const float* mat) {
    CUAssertLog(mat, "Source array is null");
    std::memcpy(m, mat, MATRIX_SIZE);
}

/**
 * Constructs a new transform that is the copy of the specified one.
 *
 * @param copy The transform to copy.
 */
Affine2::Affine2(const Affine2& copy) {
    std::memcpy(this->m, copy.m, MATRIX_SIZE);
}

/**
 * Constructs a new transform that contains the resources of the specified one.
 *
 * @param copy The transform contributing resources.
 */
Affine2::Affine2(Affine2&& copy) {
    std::memcpy(this->m, copy.m, MATRIX_SIZE);
}

#pragma mark -
#pragma mark Static Constructors
/**
 * Creates a uniform scale transform.
 *
 * @param scale The amount to scale.
 * @param dst   A transform to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::createScale(float scale, Affine2* dst) {
    CUAssertLog(dst,"Assignment transform is null");
    std::memset(dst->m, 0, MATRIX_SIZE);
    dst->m[0] = dst->m[3] = scale;
    return dst;
}

/**
 * Creates a nonuniform scale transform.
 *
 * @param sx    The amount to scale along the x-axis.
 * @param sy    The amount to scale along the y-axis.
 * @param dst   A transform to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::createScale(float sx, float sy, Affine2* dst) {
    CUAssertLog(dst,"Assignment transform is null");
    std::memset(dst->m, 0, MATRIX_SIZE);
    dst->m[0] = sx;
    dst->m[3] = sy;
    return dst;
}

/**
 * Creates a nonuniform scale transform from the given vector.
 *
 * @param scale The nonuniform scale value.
 * @param dst   A transform to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::createScale(const Vec2 scale, Affine2* dst) {
    CUAssertLog(dst,"Assignment transform is null");
    std::memset(dst->m, 0, MATRIX_SIZE);
    dst->m[0] = scale.x;
    dst->m[3] = scale.y;
    return dst;
}

/**
 * Creates a rotation transform for the given angle
 *
 * The angle measurement is in radians.  The rotation is counter
 * clockwise about the z-axis.
 *
 * @param angle The angle (in radians).
 * @param dst   A transform to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::createRotation(float angle, Affine2* dst) {
    CUAssertLog(dst,"Assignment transform is null");
    
    float c = cos(angle);
    float s = sin(angle);

    std::memset(dst->m, 0, MATRIX_SIZE);
    dst->m[0] = c;
    dst->m[1] = s;
    dst->m[2] = -s;
    dst->m[3] = c;
    return dst;
}

/**
 * Creates a translation transform from the given offset.
 *
 * @param trans The translation offset.
 * @param dst   A transform to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::createTranslation(const Vec2 trans, Affine2* dst) {
    CUAssertLog(dst,"Assignment transform is null");
    std::memset(dst->m, 0, MATRIX_SIZE);
    dst->m[0] = dst->m[3] = 1.0f;
    dst->m[4] = trans.x;
    dst->m[5] = trans.y;
    return dst;
}

/**
 * Creates a translation transform from the given parameters.
 *
 * @param tx    The translation on the x-axis.
 * @param ty    The translation on the y-axis.
 * @param dst   A transform to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::createTranslation(float tx, float ty, Affine2* dst) {
    CUAssertLog(dst,"Assignment transform is null");
    std::memset(dst->m, 0, MATRIX_SIZE);
    dst->m[0] = dst->m[3] = 1.0f;
    dst->m[4] = tx;
    dst->m[5] = ty;
    return dst;
}

#pragma mark -
#pragma mark Setters
/**
 * Sets the individal values of this transform.
 *
 * @param m11   The first element of the first row.
 * @param m12   The second element of the first row.
 * @param m21   The first element of the second row.
 * @param m22   The second element of the second row.
 * @param tx    The translation offset for the x-coordinate.
 * @param ty    The translation offset for the y-coordinate.
 *
 * @return A reference to this (modified) Affine2 for chaining.
 */
Affine2& Affine2::set(float m11, float m12, float m21, float m22, float tx, float ty) {
    std::memset(m, 0, MATRIX_SIZE);
    m[0] = m11;
    m[1] = m21;
    m[2] = m12;
    m[3] = m22;
    m[4] = tx;
    m[5] = ty;
    return *this;
}

/**
 * Sets the values of this transform to those in the specified column-major array.
 *
 * The passed-in array is six elements in column-major order, with
 * the last two elements being the translation offset.  Hence the
 * memory layout of the array is as follows:
 *
 *     0   2   4
 *     1   3   5
 *
 * @param mat An array containing 6 elements in column-major order.
 *
 * @return A reference to this (modified) Affine2 for chaining.
 */
Affine2& Affine2::set(const float* mat) {
    CUAssertLog(mat, "Source array is null");
    std::memcpy(m, mat, MATRIX_SIZE);
    return *this;
}

/**
 * Sets the values of this transform to those in the specified column-major array.
 *
 *
 * The passed-in array is six elements grouped in pairs, with each pair
 * separated by a stride.  For example, if stride is 4, then mat is a
 * 12-element array with the first column at 0,1, the second column at
 * 4,5 and the translation component at 8,9.
 *
 * @param mat       An array containing elements in column-major order.
 * @param stride    The stride offset of each column pair
 *
 * @return A reference to this (modified) Affine2 for chaining.
 */
Affine2& Affine2::set(const float* mat, size_t stride) {
    CUAssertLog(mat, "Source array is null");
    std::memset(m, 0, MATRIX_SIZE);
    m[0] = mat[0];
    m[1] = mat[1];
    m[2] = mat[0+stride];
    m[3] = mat[1+stride];
    m[4] = mat[0+2*stride];
    m[5] = mat[0+2*stride];
    return *this;
}

/**
 * Sets the elements of this transform to those in the specified transform.
 *
 * @param mat The transform to copy.
 *
 * @return A reference to this (modified) Affine2 for chaining.
 */
Affine2& Affine2::set(const Affine2& mat) {
    std::memcpy(this->m, mat.m, MATRIX_SIZE);
    return *this;
}

/**
 * Sets this transform to the identity transform.
 *
 * @return A reference to this (modified) Affine2 for chaining.
 */
Affine2& Affine2::setIdentity() {
    std::memset(m, 0, MATRIX_SIZE);
    m[0] = m[3] = 1.0f;
    return *this;
}

/**
 * Sets all elements of the current transform to zero.
 *
 * @return A reference to this (modified) Affine2 for chaining.
 */
Affine2& Affine2::setZero() {
    std::memset(m, 0, MATRIX_SIZE);
    return *this;
}

#pragma mark -
#pragma mark Static Arithmetic
/**
 * Adds the specified offset to the given and stores the result in dst.
 *
 * Addition is applied to the offset only; the core matrix is unchanged.
 *
 * @param m     The initial transform.
 * @param v     The offset to add.
 * @param dst   A transform to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::add(const Affine2& m, const Vec2 v, Affine2* dst) {
    dst->set(m);
    dst->m[4] += v.x;
    dst->m[5] += v.y;
    return dst;
}

/**
 * Adds the specified offset to the given and stores the result in dst.
 *
 * Addition is applied to the offset only; the core matrix is unchanged.
 * Both of the float arrays should have at least 6-elements where each of the
 * three pairs have the given stride.
 *
 * @param m         The initial transform in column major order
 * @param v         The offset to add.
 * @param dst       A transform to store the result in column major order
 * @param stride    The pairwise data stride
 *
 * @return A reference to dst for chaining
 */
float* Affine2::add(const float* m, const Vec2 v, float* dst, size_t stride) {
    const size_t p0 = 0;
    const size_t p1 = 1;
    const size_t p2 = 0+stride;
    const size_t p3 = 1+stride;
    const size_t p4 = 0+2*stride;
    const size_t p5 = 1+2*stride;
    dst[p0] = m[p0];
    dst[p1] = m[p1];
    dst[p2] = m[p2];
    dst[p3] = m[p3];
    dst[p4] = m[p4]+v.x;
    dst[p5] = m[p5]+v.y;
    return dst;
}

/**
 * Subtracts the offset v from m and stores the result in dst.
 *
 * Subtraction is applied to the offset only; the core matrix is unchanged.
 *
 * @param m     The initial transform.
 * @param v     The offset to subtract.
 * @param dst   A transform to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::subtract(const Affine2& m, const Vec2 v, Affine2* dst) {
    dst->set(m);
    dst->m[4] -= v.x;
    dst->m[5] -= v.y;
    return dst;
}

/**
 * Subtracts the offset v from m and stores the result in dst.
 *
 * Subtraction is applied to the offset only; the core matrix is unchanged.
 * Both of the float arrays should have at least 6-elements where each of the
 * three pairs have the given stride.
 *
 * @param m1        The initial transform in column major order
 * @param v         The offset to subtract.
 * @param dst       A transform to store the result in column major order
 * @param stride    The pairwise data stride
 *
 * @return A reference to dst for chaining
 */
float* Affine2::subtract(const float* m, const Vec2 v, float* dst, size_t stride) {
    const size_t p0 = 0;
    const size_t p1 = 1;
    const size_t p2 = 0+stride;
    const size_t p3 = 1+stride;
    const size_t p4 = 0+2*stride;
    const size_t p5 = 1+2*stride;
    dst[p0] = m[p0];
    dst[p1] = m[p1];
    dst[p2] = m[p2];
    dst[p3] = m[p3];
    dst[p4] = m[p4]-v.x;
    dst[p5] = m[p5]-v.y;
    return dst;
}

/**
 * Multiplies the specified transform by a scalar and stores the result in dst.
 *
 * The scalar is applied to BOTH the core matrix and the offset.
 *
 * @param mat       The transform.
 * @param scalar    The scalar value.
 * @param dst       A transform to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::multiply(const Affine2& mat, float scalar, Affine2* dst) {
    dst->m[0] = mat.m[0]*scalar;
    dst->m[1] = mat.m[1]*scalar;
    dst->m[2] = mat.m[2]*scalar;
    dst->m[3] = mat.m[3]*scalar;
    dst->m[4] = mat.m[4]*scalar;
    dst->m[5] = mat.m[5]*scalar;
    return dst;
}

/**
 * Multiplies the specified transform by a scalar and stores the result in dst.
 *
 * The scalar is applied to BOTH the core matrix and the offset. Both of the float
 * arrays should have at least 6-elements where each of the three pairs have
 * the given stride.
 *
 * @param mat       The transform in column major order
 * @param scalar    The scalar value.
 * @param dst       A transform to store the result in column major order
 * @param stride    The pairwise data stride
 *
 * @return A reference to dst for chaining
 */
float* Affine2::multiply(const float* mat, float scalar, float* dst, size_t stride) {
    const size_t p0 = 0;
    const size_t p1 = 1;
    const size_t p2 = 0+stride;
    const size_t p3 = 1+stride;
    const size_t p4 = 0+2*stride;
    const size_t p5 = 1+2*stride;
    dst[p0] = mat[p0]*scalar;
    dst[p1] = mat[p1]*scalar;
    dst[p2] = mat[p2]*scalar;
    dst[p3] = mat[p3]*scalar;
    dst[p4] = mat[p4]*scalar;
    dst[p5] = mat[p5]*scalar;
    return dst;
}

/**
 * Multiplies m1 by the transform m2 and stores the result in dst.
 *
 * Transform multiplication is defined as standard function composition.
 * The transform m2 is on the right.  This means that it corresponds to
 * an subsequent transform; transforms are applied left-to-right.
 *
 * @param m1    The first transform to multiply.
 * @param m2    The second transform to multiply.
 * @param dst   A matrix to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::multiply(const Affine2& m1, const Affine2& m2, Affine2* dst) {
    // Need to be prepared for them to be the same
    float a  = m2.m[0] * m1.m[0] + m2.m[2] * m1.m[1];
    float b  = m2.m[0] * m1.m[2] + m2.m[2] * m1.m[3];
    float c  = m2.m[1] * m1.m[0] + m2.m[3] * m1.m[1];
    float d  = m2.m[1] * m1.m[2] + m2.m[3] * m1.m[3];
    float tx = m2.m[0] * m1.m[4] + m2.m[2] * m1.m[5] + m2.m[4];
    float ty = m2.m[1] * m1.m[4] + m2.m[3] * m1.m[5] + m2.m[5];
    dst->m[0] = a;
    dst->m[1] = c;
    dst->m[2] = b;
    dst->m[3] = d;
    dst->m[4] = tx;
    dst->m[5] = ty;
    return dst;
}

/**
 * Multiplies m1 by the matrix m2 and stores the result in dst.
 *
 * The matrix m2 is on the right.  This means that it corresponds to
 * a subsequent transform, when looking at the order of transforms.
 * The z component of m2 is ignored.
 *
 * @param m1    The first transform to multiply.
 * @param m2    The second transform to multiply.
 * @param dst   A matrix to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::multiply(const Affine2& m1, const Mat4& m2, Affine2* dst) {
    // Need to be prepared for them to be the same
    float a  = m2.m[0] * m1.m[0] + m2.m[4] * m1.m[1];
    float b  = m2.m[0] * m1.m[2] + m2.m[4] * m1.m[3];
    float c  = m2.m[1] * m1.m[0] + m2.m[5] * m1.m[1];
    float d  = m2.m[1] * m1.m[2] + m2.m[5] * m1.m[3];
    float tx = m2.m[0] * m1.m[4] + m2.m[4] * m1.m[5] + m2.m[12];
    float ty = m2.m[1] * m1.m[4] + m2.m[5] * m1.m[5] + m2.m[13];
    dst->m[0] = a;
    dst->m[1] = c;
    dst->m[2] = b;
    dst->m[3] = d;
    dst->m[4] = tx;
    dst->m[5] = ty;
    return dst;
}

/**
 * Multiplies m1 by the matrix m2 and stores the result in dst.
 *
 * The matrix m2 is on the right.  This means that it corresponds to
 * a subsequent transform, when looking at the order of transforms.
 * The z component of m2 is ignored.
 *
 * @param m1    The first transform to multiply.
 * @param m2    The second transform to multiply.
 * @param dst   A matrix to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::multiply(const Mat4& m1, const Affine2& m2, Affine2* dst) {
    // Need to be prepared for them to be the same
    float a  = m2.m[0] * m1.m[0] + m2.m[2] * m1.m[1];
    float b  = m2.m[0] * m1.m[4] + m2.m[2] * m1.m[5];
    float c  = m2.m[1] * m1.m[0] + m2.m[3] * m1.m[1];
    float d  = m2.m[1] * m1.m[4] + m2.m[3] * m1.m[5];
    float tx = m2.m[0] * m1.m[12] + m2.m[2] * m1.m[13] + m2.m[4];
    float ty = m2.m[1] * m1.m[12] + m2.m[3] * m1.m[13] + m2.m[5];
    dst->m[0] = a;
    dst->m[1] = c;
    dst->m[2] = b;
    dst->m[3] = d;
    dst->m[4] = tx;
    dst->m[5] = ty;
    return dst;
}

/**
 * Multiplies m1 by the transform m2 and stores the result in dst.
 *
 * Transform multiplication is defined as standard function composition.
 * The transform m2 is on the right.  This means that it corresponds to
 * an subsequent transform; transforms are applied left-to-right.
 *
 * Both of the float arrays should have at least 6-elements where each of the
 * three pairs have the given stride.
 *
 * @param m1        The first transform to multiply in column major order
 * @param m2        The second transform to multiply in column major order
 * @param dst       A matrix to store the result in column major order
 * @param stride    The pairwise data stride
 *
 * @return A reference to dst for chaining
 */
float* Affine2::multiply(const float* m1, const float* m2, float* dst, size_t stride) {
    const size_t p0 = 0;
    const size_t p1 = 1;
    const size_t p2 = 0+stride;
    const size_t p3 = 1+stride;
    const size_t p4 = 0+2*stride;
    const size_t p5 = 1+2*stride;

    // Need to be prepared for them to be the same
    float a  = m2[p0] * m1[p0] + m2[p2] * m1[p1];
    float b  = m2[p0] * m1[p2] + m2[p2] * m1[p3];
    float c  = m2[p1] * m1[p0] + m2[p3] * m1[p1];
    float d  = m2[p1] * m1[p2] + m2[p3] * m1[p3];
    float tx = m2[p0] * m1[p4] + m2[p2] * m1[p5] + m2[p4];
    float ty = m2[p1] * m1[p4] + m2[p3] * m1[p5] + m2[p5];
    dst[p0] = a;
    dst[p1] = c;
    dst[p2] = b;
    dst[p3] = d;
    dst[p4] = tx;
    dst[p5] = ty;
    return dst;
}

/**
 * Inverts m1 and stores the result in dst.
 *
 * If the transform cannot be inverted, this method stores the zero transform
 * in dst.
 *
 * @param m1    The transform to negate.
 * @param dst   A transform to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::invert(const Affine2& m1, Affine2* dst) {
    float det = m1.getDeterminant();
    if (det == 0.0f) {
        dst->setZero();
        return dst;
    }
    // Need to be prepared for transforms to be the same.
    det = 1.0f/det;
    float m11 =  m1.m[3]*det;
    float m12 = -m1.m[2]*det;
    float m21 = -m1.m[1]*det;
    float m22 =  m1.m[0]*det;
    float mtx = -(m11 * m1.m[4] + m12 * m1.m[5]);
    float mty = -(m21 * m1.m[4] + m22 * m1.m[5]);
    dst->m[0] = m11;
    dst->m[1] = m21;
    dst->m[2] = m12;
    dst->m[3] = m22;
    dst->m[4] = mtx;
    dst->m[5] = mty;
    return dst;
}

/**
 * Inverts m1 and stores the result in dst.
 *
 * If the transform cannot be inverted, this method stores the zero
 * transform in dst. Both of the float arrays should have at least
 * 6-elements where each of the three pairs have the given stride.
 * When converting a matrix to the zero transform, positions outside
 * of the 6 core elements are ignored.
 *
 * @param m1        The transform to negate in column major order
 * @param dst       A transform to store the result in column major order
 * @param stride    The pairwise data stride
 *
 * @return A reference to dst for chaining
 */
float* Affine2::invert(const float* m1, float* dst, size_t stride) {
    const size_t p0 = 0;
    const size_t p1 = 1;
    const size_t p2 = 0+stride;
    const size_t p3 = 1+stride;
    const size_t p4 = 0+2*stride;
    const size_t p5 = 1+2*stride;
    float det = m1[p0]*m1[p3]-m1[p2]*m1[p1];

    if (det == 0.0f) {
        dst[p0] = dst[p1] = dst[p2] = 0;
        dst[p3] = dst[p4] = dst[p5] = 0;
        return dst;
    }
    // Need to be prepared for transforms to be the same.
    det = 1.0f/det;
    float m11 =  m1[p3]*det;
    float m12 = -m1[p2]*det;
    float m21 = -m1[p1]*det;
    float m22 =  m1[p0]*det;
    float mtx = -(m11 * m1[p4] + m12 * m1[p5]);
    float mty = -(m21 * m1[p4] + m22 * m1[p5]);
    dst[p0] = m11;
    dst[p1] = m21;
    dst[p2] = m12;
    dst[p3] = m22;
    dst[p4] = mtx;
    dst[p5] = mty;
    return dst;
}


#pragma mark -
#pragma mark Comparisons
/**
 * Returns true if the transforms are exactly equal to each other.
 *
 * This method may be unreliable given that the elements are floats.
 * It should only be used to compared transform that have not undergone
 * a lot of manipulation.
 *
 * @param aff   The transform to compare against.
 *
 * @return true if the transforms are exactly equal to each other.
 */
bool Affine2::isExactly(const Affine2& aff) const {
    return std::memcmp(m,aff.m,MATRIX_SIZE) == 0;
}

/**
 * Returns true if the transforms are within tolerance of each other.
 *
 * The tolerance is applied to each element of the transform individually.
 *
 * @param aff       The transform to compare against.
 * @param variance  The comparison tolerance.
 *
 * @return true if the transforms are within tolerance of each other.
 */
bool Affine2::equals(const Affine2& aff, float variance) const {
    bool similar = true;
    for(int ii = 0; similar && ii < 6; ii++) {
        similar = (fabsf(m[ii]-aff.m[ii]) <= variance);
    }
    return similar;
}


#pragma mark -
#pragma mark Affine Attributes
/**
 * Returns true if this transform is equal to the identity transform.
 *
 * The optional comparison tolerance takes into accout that elements
 * are floats and this may not be exact.  The tolerance is applied to
 * each element individually.  By default, the match must be exact.
 *
 * @param variance The comparison tolerance
 *
 * @return true if this transform is equal to the identity transform.
 */
bool Affine2::isIdentity(float variance) const {
    return equals(IDENTITY,variance);
}

/**
 * Reads the affine transform as a 3x3 matrix into the given array.
 *
 * The array should contain at least 9 elements.  The transform
 * is read in column major order as a 3x3 matrix in homogenous
 * coordinates.  That is, the z values are all 0, except for the
 * translation component which is 1.
 *
 * @param array The array to store the values
 *
 * @return a reference to the array for chaining
 */
float* Affine2::get3x3(float* array) const {
    array[0] = m[0];
    array[1] = m[1];
    array[2] = 0.0f;
    array[3] = m[2];
    array[4] = m[3];
    array[5] = 0.0f;
    array[6] = m[4];
    array[7] = m[5];
    array[8] = 1.0f;
    return array;
}

/**
 * Reads the affine transform as a 3x4 matrix into the given array.
 *
 * The array should contain at least 12 elements.  The transform
 * is read in column major order as a 3x4 matrix in homogenous
 * coordinates.  That is, the z and w values are all 0, except
 * for the translation z component which is 1.
 *
 * @param array The array to store the values
 *
 * @return a reference to the array for chaining
 */
float* Affine2::get3x4(float* array) const {
    array[0] = m[0];
    array[1] = m[1];
    array[2] = 0.0f;
    array[3] = 0.0f;
    array[4] = m[2];
    array[5] = m[3];
    array[6] = 0.0f;
    array[7] = 0.0f;
    array[8] = m[4];
    array[9] = m[5];
    array[10] = 1.0f;
    array[11] = 0.0f;
    return array;
}

/**
 * Reads the affine transform as a 4x4 matrix into the given array.
 *
 * The array should contain at least 16 elements.  The transform
 * is read in column major order as a 4x4 matrix in homogenous
 * coordinates.  The z and w values are all 0, except for the
 * translation w component which is 1.
 *
 * @param array The array to store the values
 *
 * @return a reference to the array for chaining
 */
float* Affine2::get4x4(float* array) const {
    std::memset(array, 0, 16*sizeof(float));
    array[0] = m[0];
    array[1] = m[1];
    array[4] = m[2];
    array[5] = m[3];
    array[12] = m[4];
    array[13] = m[5];
    array[15] = 1.0f;
    return array;
}


/**
 * Reads the affine transform as an array with the given stride
 *
 * Both of the float arrays should have at least 6-elements where each
 * of the three pairs have the given stride.  Postions outside of
 * the 6 element core are left untouched.
 *
 * @param array     The array to store the values
 * @param stride    The pairwise data stride
 *
 * @return a reference to the array for chaining
 */
float* Affine2::get(float* array, size_t stride) const {
     const size_t p0 = 0;
     const size_t p1 = 1;
     const size_t p2 = 0+stride;
     const size_t p3 = 1+stride;
     const size_t p4 = 0+2*stride;
     const size_t p5 = 1+2*stride;

    array[p0] = m[p0];
    array[p1] = m[p1];
    array[p2] = m[p2];
    array[p3] = m[p3];
    array[p4] = m[p4];
    array[p5] = m[p5];
    return array;
}
/**
 * Decomposes the scale, rotation and translation components of the given matrix.
 *
 * To work properly, the matrix must have been constructed in the following
 * order: scale, then rotate, then translation.  While the rotation matrix
 * will always be correct, the scale and translation are not guaranteed
 * to be correct.
 *
 * If any pointer is null, the method simply does not assign that result.
 * However, it will still continue to compute the component with non-null
 * vectors to store the result.
 *
 * If the scale component is too small, then it may be impossible to
 * extract the rotation. In that case, if the rotation pointer is not
 * null, this method will return false.
 *
 * @param mat   The transform to decompose.
 * @param scale The scale component.
 * @param rot   The rotation component.
 * @param trans The translation component.
 *
 * @return true if all requested components were properly extracted
 */
bool Affine2::decompose(const Affine2& mat, Vec2* scale, float* rot, Vec2* trans) {
    if (trans != nullptr) {
        // Extract the translation.
        trans->set(mat.m[4],mat.m[5]);
    }
    
    // Nothing left to do.
    if (scale == nullptr && rot == nullptr) {
        return true;
    }
    
    // Extract the scale.
    // This is simply the length of each axis (row/column) in the matrix.
    Vec2 xaxis(mat.m[0], mat.m[1]);
    float scaleX = xaxis.length();
    
    Vec2 yaxis(mat.m[2], mat.m[3]);
    float scaleY = yaxis.length();
    
    // Determine if we have a negative scale (true if determinant is less than zero).
    // In this case, we simply negate a single axis of the scale.
    float det = mat.getDeterminant();
    if (det < 0) {
        scaleY = -scaleY;
    }
    
    if (scale != nullptr) {
        scale->set(scaleX,scaleY);
    }
    
    // Nothing left to do.
    if (rot == nullptr) {
        return true;
    }
    
    // Scale too close to zero, can't decompose rotation.
    if (scaleX < CU_MATH_EPSILON || fabsf(scaleY) < CU_MATH_EPSILON) {
        return false;
    }
    
    float rn;
    
    // Factor the scale out of the matrix axes.
    rn = 1.0f / scaleX;
    xaxis.x *= rn;
    xaxis.y *= rn;

    rn = 1.0f / scaleY;
    yaxis.x *= rn;
    yaxis.y *= rn;

    *rot = atan2f(xaxis.y,yaxis.y);
    return true;
}


#pragma mark -
#pragma mark Vector Operations
/**
 * Transforms the point and stores the result in dst.
 *
 * @param aff   The affine transform.
 * @param point The point to transform.
 * @param dst   A vector to store the transformed point in.
 *
 * @return A reference to dst for chaining
 */
Vec2* Affine2::transform(const Affine2& aff, const Vec2 point, Vec2* dst) {
    float x = aff.m[0]*point.x+aff.m[2]*point.y+aff.m[4];
    float y = aff.m[1]*point.x+aff.m[3]*point.y+aff.m[5];
    dst->set(x,y);
    return dst;
}

/**
 * Transforms the vector array, and stores the result in dst.
 *
 * The vector is array is treated as a list of 2 element vectors (@see Vec2).
 * The transform is applied in order and written to the output array.
 *
 * @param mat       The transform matrix.
 * @param input     The array of vectors to transform.
 * @param output    The array to store the transformed vectors.
 * @param size      The size of the two arrays.
 *
 * @return A reference to dst for chaining
 */
float* Affine2::transform(const Affine2& aff, float const* input, float* output, size_t size) {
    for(size_t ii = 0; ii < size; ii++) {
        float x = aff.m[0]*input[2*ii]+aff.m[2]*input[2*ii+1]+aff.m[4];
        float y = aff.m[1]*input[2*ii]+aff.m[3]*input[2*ii+1]+aff.m[5];
        output[2*ii  ] = x;
        output[2*ii+1] = y;
    }
    return output;
}

/**
 * Transforms the rectangle and stores the result in dst.
 *
 * This method transforms the four defining points of the rectangle.  It
 * then computes the minimal bounding box storing these four points
 *
 * @param aff   The affine transform.
 * @param rect  The rect to transform.
 * @param dst   A rect to store the transformed rectangle in.
 *
 * @return A reference to dst for chaining
 */
cugl::Rect* Affine2::transform(const Affine2& aff, const Rect rect, Rect* dst) {
    Vec2 point1(rect.getMinX(),rect.getMinY());
    Vec2 point2(rect.getMinX(),rect.getMaxY());
    Vec2 point3(rect.getMaxX(),rect.getMinY());
    Vec2 point4(rect.getMaxX(),rect.getMaxY());
    Affine2::transform(aff,point1,&point1);
    Affine2::transform(aff,point2,&point2);
    Affine2::transform(aff,point3,&point3);
    Affine2::transform(aff,point4,&point4);
    float minx = std::min(point1.x,std::min(point2.x,std::min(point3.x,point4.x)));
    float maxx = std::max(point1.x,std::max(point2.x,std::max(point3.x,point4.x)));
    float miny = std::min(point1.y,std::min(point2.y,std::min(point3.y,point4.y)));
    float maxy = std::max(point1.y,std::max(point2.y,std::max(point3.y,point4.y)));
    dst->origin.set(minx,miny);
    dst->size.set(maxx-minx,maxy-miny);
    return dst;
}

/**
 * Returns a copy of the given rectangle transformed.
 *
 * This method transforms the four defining points of the rectangle.  It
 * then computes the minimal bounding box storing these four points
 *
 * Note: This does not modify the original rectangle. To transform a
 * point in place, use the static method.
 *
 * @param rect  The rect to transform.
 *
 * @return A reference to dst for chaining
 */
cugl::Rect Affine2::transform(const Rect rect) const {
    Rect result;
    return *(transform(*this,rect,&result));
}

#pragma mark -
#pragma mark Static Transform Manipulation
/**
 * Applies a rotation to the given transform and stores the result in dst.
 *
 * The rotation is in radians, counter-clockwise about the z-axis.
 *
 * The rotation is applied on the right.  Given our convention, that means
 * that it takes place AFTER any previously applied transforms.
 *
 * @param aff   The transform to rotate.
 * @param angle The angle (in radians).
 * @param dst   A transform to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::rotate(const Affine2& aff, float angle, Affine2* dst) {
    Affine2 result;
    createRotation(angle, &result);
    multiply(aff, result, dst);
    return dst;
}

/**
 * Applies a rotation to the given transform and stores the result in dst.
 *
 * The rotation is in radians, counter-clockwise about the z-axis.
 * Both of the float arrays should have at least 6-elements where each
 * of the three pairs have the given stride.
 *
 * The rotation is applied on the right.  Given our convention, that
 * means that it takes place AFTER any previously applied transforms.
 *
 * @param aff       The transform to rotate in column major order
 * @param angle     The angle (in radians).
 * @param dst       A transform to store the result in column major order
 * @param stride    The pairwise data stride
 *
 * @return A reference to dst for chaining
 */
float* Affine2::rotate(const float* aff, float angle, float* dst, size_t stride) {
    const size_t p0 = 0;
    const size_t p1 = 1;
    const size_t p2 = 0+stride;
    const size_t p3 = 1+stride;
    const size_t p4 = 0+2*stride;
    const size_t p5 = 1+2*stride;

    // Need to be prepared for them to be the same
    Affine2 rot;
    createRotation(angle, &rot);
    float a  = rot.m[0] * aff[p0] + rot.m[2] * aff[p1];
    float b  = rot.m[0] * aff[p2] + rot.m[2] * aff[p3];
    float c  = rot.m[1] * aff[p0] + rot.m[3] * aff[p1];
    float d  = rot.m[1] * aff[p2] + rot.m[3] * aff[p3];
    float tx = rot.m[0] * aff[p4] + rot.m[2] * aff[p5];
    float ty = rot.m[1] * aff[p4] + rot.m[3] * aff[p5];
    dst[p0] = a;
    dst[p1] = c;
    dst[p2] = b;
    dst[p3] = d;
    dst[p4] = tx;
    dst[p5] = ty;
    return dst;
}

/**
 * Applies a uniform scale to the given transform and stores the result in dst.
 *
 * The scaling operation is applied on the right.  Given our convention,
 * that means that it takes place AFTER any previously applied transforms.
 *
 * @param aff   The transform to scale.
 * @param value The scalar to multiply by.
 * @param dst   A transform to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::scale(const Affine2& aff, float value, Affine2* dst) {
    if (dst->m != aff.m) std::memcpy(dst->m, aff.m, MATRIX_SIZE);
    dst->m[0] *= value;
    dst->m[1] *= value;
    dst->m[2] *= value;
    dst->m[3] *= value;
    dst->m[4] *= value;
    dst->m[5] *= value;
    return dst;
}

/**
 * Applies a uniform scale to the given transform and stores the result in dst.
 *
 * Both of the float arrays should have at least 6-elements where each
 * of the three pairs have the given stride.
 *
 * The scaling operation is applied on the right. Given our convention,
 * that means that it takes place AFTER any previously applied transforms.
 *
 * @param aff       The transform to scale in column major order
 * @param value     The scalar to multiply by.
 * @param dst       A transform to store the result in column major order
 * @param stride    The pairwise data stride
 *
 * @return A reference to dst for chaining
 */
float* Affine2::scale(const float* aff, float value, float* dst, size_t stride) {
    CUAssertLog(dst, "Destination transform is null");
    const size_t p0 = 0;
    const size_t p1 = 1;
    const size_t p2 = 0+stride;
    const size_t p3 = 1+stride;
    const size_t p4 = 0+2*stride;
    const size_t p5 = 1+2*stride;

    dst[p0] = aff[p0]*value;
    dst[p1] = aff[p1]*value;
    dst[p2] = aff[p2]*value;
    dst[p3] = aff[p3]*value;
    dst[p4] = aff[p4]*value;
    dst[p5] = aff[p5]*value;
    return dst;
}

/**
 * Applies a non-uniform scale to the given transform and stores the result in dst.
 *
 * The scaling operation is applied on the right.  Given our convention,
 * that means that it takes place AFTER any previously applied transforms.
 *
 * @param aff   The transform to scale.
 * @param s     The vector storing the individual scaling factors
 * @param dst   A transform to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::scale(const Affine2& aff, const Vec2 s, Affine2* dst) {
    CUAssertLog(dst, "Destination transform is null");
    if (dst->m != aff.m) std::memcpy(dst->m, aff.m, MATRIX_SIZE);
    dst->m[0] *= s.x;
    dst->m[1] *= s.y;
    dst->m[2] *= s.x;
    dst->m[3] *= s.y;
    dst->m[4] *= s.x;
    dst->m[5] *= s.y;
    return dst;
}

/**
 * Applies a non-uniform scale to the given transform and stores the result in dst.
 *
 * Both of the float arrays should have at least 6-elements where each
 * of the three pairs have the given stride.
 *
 * The scaling operation is applied on the right.  Given our convention,
 * that means that it takes place AFTER any previously applied transforms.
 *
 * @param aff       The transform to scale in column major order
 * @param s         The vector storing the individual scaling factors
 * @param dst       A transform to store the result in column major order
 * @param stride    The pairwise data stride
 *
 * @return A reference to dst for chaining
 */
float* Affine2::scale(const float* aff, const Vec2 s, float* dst, size_t stride) {
    CUAssertLog(dst, "Destination transform is null");
    const size_t p0 = 0;
    const size_t p1 = 1;
    const size_t p2 = 0+stride;
    const size_t p3 = 1+stride;
    const size_t p4 = 0+2*stride;
    const size_t p5 = 1+2*stride;

    dst[p0] = aff[p0]*s.x;
    dst[p1] = aff[p1]*s.y;
    dst[p2] = aff[p2]*s.x;
    dst[p3] = aff[p3]*s.y;
    dst[p4] = aff[p4]*s.x;
    dst[p5] = aff[p5]*s.y;
    return dst;
}

/**
 * Applies a non-uniform scale to the given transform and stores the result in dst.
 *
 * The scaling operation is applied on the right.  Given our convention,
 * that means that it takes place AFTER any previously applied transforms.
 *
 * @param aff   The transform to scale.
 * @param sx    The amount to scale along the x-axis.
 * @param sy    The amount to scale along the y-axis.
 * @param dst   A transform to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::scale(const Affine2& aff, float sx, float sy, Affine2* dst) {
    CUAssertLog(dst, "Destination transform is null");
    if (dst->m != aff.m) std::memcpy(dst->m, aff.m, MATRIX_SIZE);
    dst->m[0] *= sx;
    dst->m[1] *= sy;
    dst->m[2] *= sx;
    dst->m[3] *= sy;
    dst->m[4] *= sx;
    dst->m[5] *= sy;
    return dst;
}

/**
 * Applies a non-uniform scale to the given transform and stores the result in dst.
 *
 * Both of the float arrays should have at least 6-elements where each
 * of the three pairs have the given stride.
 *
 * The scaling operation is applied on the right.  Given our convention,
 * that means that it takes place AFTER any previously applied transforms.
 *
 * @param aff       The transform to scale in column major order
 * @param sx        The amount to scale along the x-axis.
 * @param sy        The amount to scale along the y-axis.
 * @param dst       A transform to store the result in column major order
 * @param stride    The pairwise data stride
 *
 * @return A reference to dst for chaining
 */
float* Affine2::scale(const float* aff, float sx, float sy, float* dst, size_t stride) {
    CUAssertLog(dst, "Destination transform is null");
    const size_t p0 = 0;
    const size_t p1 = 1;
    const size_t p2 = 0+stride;
    const size_t p3 = 1+stride;
    const size_t p4 = 0+2*stride;
    const size_t p5 = 1+2*stride;

    dst[p0] = aff[p0]*sx;
    dst[p1] = aff[p1]*sy;
    dst[p2] = aff[p2]*sx;
    dst[p3] = aff[p3]*sy;
    dst[p4] = aff[p4]*sx;
    dst[p5] = aff[p5]*sy;
    return dst;
}

/**
 * Applies a translation to the given transform and stores the result in dst.
 *
 * The translation operation is applied on the right.  Given our convention,
 * that means that it takes place AFTER any previously applied transforms.
 *
 * @param aff   The transform to translate.
 * @param t     The vector storing the individual translation offsets
 * @param dst   A transform to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::translate(const Affine2& aff, const Vec2 t, Affine2* dst) {
    CUAssertLog(dst, "Destination transform is null");
    if (dst->m != aff.m) std::memcpy(dst->m, aff.m, MATRIX_SIZE);
    dst->m[4] += t.x;
    dst->m[5] += t.y;
    return dst;
}

/**
 * Applies a translation to the given transform and stores the result in dst.
 *
 * Both of the float arrays should have at least 6-elements where each
 * of the three pairs have the given stride.
 *
 * The translation operation is applied on the right.  Given our convention,
 * that means that it takes place AFTER any previously applied transforms.
 *
 * @param aff       The transform to translate in column major order
 * @param t         The vector storing the individual translation offsets
 * @param dst       A transform to store the result in column major order
 * @param stride    The pairwise data stride
 *
 * @return A reference to dst for chaining
 */
float* Affine2::translate(const float* aff, const Vec2 t, float* dst, size_t stride) {
    CUAssertLog(dst, "Destination transform is null");
    const size_t p0 = 0;
    const size_t p1 = 1;
    const size_t p2 = 0+stride;
    const size_t p3 = 1+stride;
    const size_t p4 = 0+2*stride;
    const size_t p5 = 1+2*stride;

    dst[p0] = aff[p0];
    dst[p1] = aff[p1];
    dst[p2] = aff[p2];
    dst[p3] = aff[p3];
    dst[p4] = aff[p4]+t.x;
    dst[p5] = aff[p5]+t.y;
    return dst;
}

/**
 * Applies a translation to the given transform and stores the result in dst.
 *
 * The translation operation is applied on the right.  Given our convention,
 * that means that it takes place AFTER any previously applied transforms.
 *
 * @param aff   The transform to translate.
 * @param tx    The translation offset for the x-axis.
 * @param ty    The translation offset for the y-axis.
 * @param dst   A transform to store the result in.
 *
 * @return A reference to dst for chaining
 */
Affine2* Affine2::translate(const Affine2& aff, float tx, float ty, Affine2* dst) {
    CUAssertLog(dst, "Destination transform is null");
    if (dst->m != aff.m) std::memcpy(dst->m, aff.m, MATRIX_SIZE);
    dst->m[4] += tx;
    dst->m[5] += ty;
    return dst;
}

/**
 * Applies a translation to the given transform and stores the result in dst.
 *
 * Both of the float arrays should have at least 6-elements where each
 * of the three pairs have the given stride.
 *
 * The translation operation is applied on the right.  Given our convention,
 * that means that it takes place AFTER any previously applied transforms.
 *
 * @param aff       The transform to translate in column major order
 * @param tx        The translation offset for the x-axis.
 * @param ty        The translation offset for the y-axis.
 * @param dst       A transform to store the result in column major order
 * @param stride    The pairwise data stride
 *
 * @return A reference to dst for chaining
 */
float* Affine2::translate(const float* aff, float tx, float ty, float* dst, size_t stride) {
    CUAssertLog(dst, "Destination transform is null");
     const size_t p0 = 0;
     const size_t p1 = 1;
     const size_t p2 = 0+stride;
     const size_t p3 = 1+stride;
     const size_t p4 = 0+2*stride;
     const size_t p5 = 1+2*stride;

     dst[p0] = aff[p0];
     dst[p1] = aff[p1];
     dst[p2] = aff[p2];
     dst[p3] = aff[p3];
     dst[p4] = aff[p4]+tx;
     dst[p5] = aff[p5]+ty;
     return dst;
}


#pragma mark -
#pragma mark Conversion Methods

/**
 * Returns a string representation of this transform for debugging purposes.
 *
 * If verbose is true, the string will include class information.  This
 * allows us to unambiguously identify the class.
 *
 * @param verbose Whether to include class information
 *
 * @return a string representation of this transform for debugging purposes.
 */
std::string Affine2::toString(bool verbose) const {
    std::stringstream ss;
    if (verbose) {
        ss << "cugl::Affine2";
    }
    const int PRECISION = 8;
    ss << "\n";
    ss << "|  ";
    ss << strtool::to_string(m[0]).substr(0,PRECISION);
    ss << ", ";
    ss << strtool::to_string(m[2]).substr(0,PRECISION);
    ss << ", ";
    ss << strtool::to_string(m[4]).substr(0,PRECISION);
    ss << "  |";
    ss << "\n";
    ss << "|  ";
    ss << strtool::to_string(m[1]).substr(0,PRECISION);
    ss << ", ";
    ss << strtool::to_string(m[3]).substr(0,PRECISION);
    ss << ", ";
    ss << strtool::to_string(m[5]).substr(0,PRECISION);
    ss << "  |";
    return ss.str();
}

/** Cast from Affine2 to a Mat4. */
Affine2::operator Mat4() const {
    Mat4 result(m[0],m[2],0,m[4], m[1],m[3],0,m[5], 0,0,1,0, 0,0,0,1);
    return result;
}

/**
 * Creates an affine transform from the given matrix.
 *
 * The z values are all uniformly ignored.  However, it the final element
 * of the matrix is not 1 (e.g. the translation has a w value of 1), then
 * it divides the entire matrix before creating the affine transform
 *
 * @param mat The matrix to convert
 */
Affine2::Affine2(const Mat4& mat) {
    float v = 1.0f;
    if (mat.m[15] != 1.0f && fabsf(mat.m[15]) > CU_MATH_EPSILON) {
        v = 1.0f/mat.m[15];
    }
    m[0] = mat.m[0]*v;
    m[1] = mat.m[1]*v;
    m[2] = mat.m[4]*v;
    m[3] = mat.m[5]*v;
    m[4] = mat.m[12]*v;
    m[5] = mat.m[13]*v;
}

/**
 * Sets the elements of this transform to those of the given matrix.
 *
 * The z values are all uniformly ignored.  However, it the final element
 * of the matrix is not 1 (e.g. the translation has a w value of 1), then
 * it divides the entire matrix before creating the affine transform
 *
 * @param mat The matrix to convert
 *
 * @return A reference to this (modified) Affine2 for chaining.
 */
Affine2& Affine2::operator= (const Mat4& mat) {
    return set(mat);
}

/**
 * Sets the elements of this transform to those of the given matrix.
 *
 * The z values are all uniformly ignored.  However, it the final element
 * of the matrix is not 1 (e.g. the translation has a w value of 1), then
 * it divides the entire matrix before creating the affine transform
 *
 * @param mat The matrix to convert
 *
 * @return A reference to this (modified) Affine2 for chaining.
 */
Affine2& Affine2::set(const Mat4& mat) {
    float v = 1.0f;
    if (mat.m[15] != 1.0f && fabsf(mat.m[15]) > CU_MATH_EPSILON) {
        v = 1.0f/mat.m[15];
    }
    m[0] = mat.m[0]*v;
    m[1] = mat.m[1]*v;
    m[2] = mat.m[4]*v;
    m[3] = mat.m[5]*v;
    m[4] = mat.m[12]*v;
    m[5] = mat.m[13]*v;
    return *this;
}

#pragma mark -
#pragma mark Constants

/** The identity transform (ones on the diagonal) */
const Affine2 Affine2::IDENTITY(1.0f,0.0f,0.0f,1.0f,0.0f,0.0f);

/** The transform with all zeroes */
const Affine2 Affine2::ZERO(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

/** The transform with all ones */
const Affine2 Affine2::ONE(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
