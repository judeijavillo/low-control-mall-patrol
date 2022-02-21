//
//  CUScissor.h
//  Cornell University Game Library (CUGL)
//
//  This module provides support for a scissor mask that supports rotation and
//  other transforms.  A scissor mask is a rectangular region that whose size
//  is defined by the extent attribute.  The associated transform transforms
//  this rectangle about its center.
//
//  This module is based on the NVGpaint datatype from nanovg by Mikko Mononen
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
#ifndef __CU_SCISSOR_H__
#define __CU_SCISSOR_H__
#include <cugl/math/CUAffine2.h>
#include <cugl/math/CURect.h>

namespace cugl {

/**
 * This class defines a general purpose scissor mask.
 *
 * A scissor mask is used to prevent portions of a 2d shape from showing. The
 * mask is a transformed rectangle, and any pixel outside of this region is
 * dropped. Unlike {@link Gradient}, a scissor is applied to a region of the
 * framebuffer and is not a texture that can be applied to a surface. Therefore,
 * the scissor mask region must be defined in terms of pixels (or at least in
 * the same coordinate system as the vertices it is masking).
 *
 * A scissor mask is defined by three values (in terms of largest to smallest data):
 * 
 * <ul>
 *    <li>An affine transform (for offset and rotation)</li>
 *    <li>A size vector for the extent</li>
 *    <li>A "fringe" value for edge aliasing</li>
 * </ul>
 *
 * Unpacking this data into std140 format is a 16 element array of floats (the 
 * fringe is expanded into a per-axis value for the shader). And this is the format 
 * that this data is represented in the {@link getData} method so that it can be 
 * passed to a {@link UniformBuffer} for improved performance. It is also possible 
 * to get access to the individual components of the scissor mask, to pass them 
 * to a shader directly (though the transform must be inverted first if it is passed
 * directly).
 *
 * Scissor masks can be intersected.  However, a scissor mask must always be a 
 * transformed rectangle, and not all quadrilateral intersections are guaranteed
 * to be transformed rectangles.  Therefore, these intersections are always an
 * an approximation, with the intersecting scissor mask converted into an 
 * axis-aligned rectangle in the coordinate space of the current scissor mask.
 * The affect is the same as the {@link Rect#intersect} operation in in {@link Rect}.
 */
class Scissor {
#pragma mark Values
private:
    /* The primary scissor transform (for OpenGL) */
    Affine2 _scissor;
    /* The inverse scissor transform (for OpenGL)  */
    Affine2 _inverse;
    /** The coordinate space transform (for intersections) */
    Affine2 _transform;
    /** The scissor bounds */
    Rect    _bounds;
    /** The anti-aliasing fringe */
    float   _fringe;

public:
#pragma mark -
#pragma mark Constructors
    /**
     * Creates a degenerate scissor of size 0.  
     *
     * All pixels will be dropped by this mask.
     */
    Scissor();

    /**
     * Creates a copy of the given scissor mask.
     *
     * @param mask    The scissor mask to copy
     */
    Scissor(const Scissor& mask) { set(mask); }

    /**
     * Creates a copy with the resources of the given scissor mask.
     *
     * The original scissor mask is no longer safe to use after calling this 
     * constructor.
     *
     * @param mask    The scissor mask to take from
      */
    Scissor(Scissor&& mask);

    /**
     * Deletes this scissor mask, releasing all resources.
     */
    ~Scissor() {}

    /**
     * Deletes the scissor mask and resets all attributes.
     *
     * You must reinitialize the scissor mask to use it.
     */
    void dispose();

    /**
     * Initializes a scissor with the given size and fringe.
     *
     * The bounding box will have origin (0,0). The fringe is the size of the scissor
     * border in pixels. A value less than 0 gives a sharp transition, where larger
     * values have more gradual transitions.
     *
     * @param size      The scissor mask size
     * @param fringe    The size of the scissor border in pixels
     *
     * @return true if initialization was successful.
     */
    bool init(const Size size, float fringe=0.5) {
        return init(Rect(Vec2::ZERO,size),fringe);
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
    bool init(const Rect rect, float fringe=0.5);

    /**
     * Initializes a scissor with the given transformed bounds and fringe.
     *
     * The bounding box will have origin (0,0). The fringe is the size of the scissor
     * border in pixels. A value less than 0 gives a sharp transition, where larger
     * values have more gradual transitions.
     *
     * @param size      The scissor mask size
     * @param aff       The scissor mask transform
     * @param fringe    The size of the scissor border in pixels
     *
     * @return true if initialization was successful.
     */
    bool init(const Size size, const Affine2& aff, float fringe=0.5) {
        return init(Rect(Vec2::ZERO,size),aff,fringe);
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
    bool init(const Rect rect, const Affine2& aff, float fringe=0.5);

    /**
     * Initializes a scissor with the given transformed bounds and fringe.
     *
     * The bounding box will have origin (0,0). The fringe is the size of the scissor
     * border in pixels. A value less than 0 gives a sharp transition, where larger
     * values have more gradual transitions.
     *
     * All z-components from the given matrix transform are lost.
     *
     * @param size      The scissor mask size
     * @param mat       The scissor mask transform
     * @param fringe    The size of the scissor border in pixels
     *
     * @return true if initialization was successful.
     */
    bool init(const Size size, const Mat4& mat, float fringe=0.5) {
        return init(Rect(Vec2::ZERO,size),mat,fringe);
    }

    /**
     * Initializes a scissor with the given transformed bounds and fringe.
     *
     * The fringe is the size of the scissor border in pixels.  A value less than
     * 0 gives a sharp transition, where larger values have more gradual transitions.
     *
     * All z-components from the given matrix transform are lost.
     *
     * @param rect      The scissor mask bounds
     * @param mat       The scissor mask transform
     * @param fringe    The size of the scissor border in pixels
     *
     * @return true if initialization was successful.
     */
    bool init(const Rect rect, const Mat4& mat, float fringe=0.5);

    /**
     * Initializes this scissor mask to be a copy of the other.
     *
     * @param mask  The scissor mask to copy
     *
     * @return true if initialization was successful.
     */
    bool init(const std::shared_ptr<Scissor>& mask);


#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a new scissor with the given bounds and fringe.
     *
     * The bounding box will have origin (0,0). The fringe is the size of the scissor
     * border in pixels. A value less than 0 gives a sharp transition, where larger
     * values have more gradual transitions.
     *
     * @param size      The scissor mask size
     * @param fringe    The size of the scissor border in pixels
     *
     * @return a new scissor with the given bounds and fringe.
     */
    static std::shared_ptr<Scissor> alloc(const Size size, float fringe=0.5) {
        std::shared_ptr<Scissor> result = std::make_shared<Scissor>();
        return (result->init(size,fringe) ? result : nullptr);
    }

    /**
     * Returns a new scissor with the given bounds and fringe.
     *
     * The fringe is the size of the scissor border in pixels.  A value less than
     * 0 gives a sharp transition, where larger values have more gradual transitions.
     *
     * @param rect      The scissor mask bounds
     * @param fringe    The size of the scissor border in pixels
     *
     * @return a new scissor with the given bounds and fringe.
     */
    static std::shared_ptr<Scissor> alloc(const Rect rect, float fringe=0.5) {
        std::shared_ptr<Scissor> result = std::make_shared<Scissor>();
        return (result->init(rect,fringe) ? result : nullptr);
    }

    /**
     * Returns a new scissor with the given transformed bounds and fringe.
     *
     * The bounding box will have origin (0,0). The fringe is the size of the scissor
     * border in pixels. A value less than 0 gives a sharp transition, where larger
     * values have more gradual transitions.
     *
     * @param size      The scissor mask size
     * @param aff       The scissor mask transform
     * @param fringe    The size of the scissor border in pixels
     *
     * @return a new scissor with the given transformed bounds and fringe.
     */
    static std::shared_ptr<Scissor> alloc(const Size size, const Affine2& aff, float fringe=0.5) {
        std::shared_ptr<Scissor> result = std::make_shared<Scissor>();
        return (result->init(size,aff,fringe) ? result : nullptr);
    }

    /**
     * Returns a new scissor with the given transformed bounds and fringe.
     *
     * The fringe is the size of the scissor border in pixels.  A value less than
     * 0 gives a sharp transition, where larger values have more gradual transitions.
     *
     * @param rect      The scissor mask bounds
     * @param aff       The scissor mask transform
     * @param fringe    The size of the scissor border in pixels
     *
     * @return a new scissor with the given transformed bounds and fringe.
     */
    static std::shared_ptr<Scissor> alloc(const Rect rect, const Affine2& aff, float fringe=0.5) {
        std::shared_ptr<Scissor> result = std::make_shared<Scissor>();
        return (result->init(rect,aff,fringe) ? result : nullptr);
    }

    /**
     * Returns a new scissor with the given transformed bounds and fringe.
     *
     * The bounding box will have origin (0,0). The fringe is the size of the scissor
     * border in pixels. A value less than 0 gives a sharp transition, where larger
     * values have more gradual transitions.
     *
     * All z-components from the given matrix transform are lost.
     *
     * @param size      The scissor mask size
     * @param mat       The scissor mask transform
     * @param fringe    The size of the scissor border in pixels
     *
     * @return a new scissor with the given transformed bounds and fringe.
     */
    static std::shared_ptr<Scissor> alloc(const Size size, const Mat4& mat, float fringe=0.5) {
        std::shared_ptr<Scissor> result = std::make_shared<Scissor>();
        return (result->init(size,mat,fringe) ? result : nullptr);
    }

    /**
     * Returns a new scissor with the given transformed bounds and fringe.
     *
     * The fringe is the size of the scissor border in pixels.  A value less than
     * 0 gives a sharp transition, where larger values have more gradual transitions.
     *
     * All z-components from the given matrix transform are lost.
     *
     * @param rect      The scissor mask bounds
     * @param mat       The scissor mask transform
     * @param fringe    The size of the scissor border in pixels
     *
     * @return a new scissor with the given transformed bounds and fringe.
     */
    static std::shared_ptr<Scissor> alloc(const Rect rect, const Mat4& mat, float fringe=0.5) {
        std::shared_ptr<Scissor> result = std::make_shared<Scissor>();
        return (result->init(rect,mat,fringe) ? result : nullptr);
    }

    /**
     * Returns a new scissor mask that is a copy of the other.
     *
     * @param mask  The scissor mask to copy
     *
     * @return a new scissor mask that is a copy of the other.
     */
    static std::shared_ptr<Scissor> alloc(const std::shared_ptr<Scissor>& mask) {
        std::shared_ptr<Scissor> result = std::make_shared<Scissor>();
        return (result->init(mask) ? result : nullptr);
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
    Scissor& operator= (const Scissor& mask) { return set(mask); }

    /**
     * Sets this scissor mask to be have the resources of the given one.
     *
     * The original scissor mask is no longer safe to use after calling 
     * this operator.
     *
     * @param mask    The scissor mask to take from
     *
     * @return this scissor mask, returned for chaining
     */
    Scissor& operator=(Scissor&& mask)  {
        _inverse   = mask._inverse;
        _transform = mask._transform;
        _bounds  = mask._bounds;
        _fringe  = mask._fringe;
        return *this;
    }
    
    /**
     * Sets this to be a scissor mask with the given bouding rectangle.
     *
     * Any previous transforms are dropped when this operator is called.
     *
     * @param rect    The scissor mask bounds
     *
     * @return this scissor mask, returned for chaining
     */
    Scissor& operator=(const Rect rect) { return set(rect); }

    /**
     * Sets this scissor mask to be a copy of the given one.
     *
     * @param mask    The scissor mask to copy
     *
     * @return this scissor mask, returned for chaining
     */
    Scissor& set(const Scissor& mask);
    
    /**
     * Sets this scissor mask to be a copy of the given one.
     *
     * @param mask    The scissor mask to copy
     *
     * @return this scissor mask, returned for chaining
     */
    Scissor& set(const std::shared_ptr<Scissor>& mask) {
        return set(*mask);
    }
    
    /**
     * Sets the scissor mask to have the given bounds and fringe.
     *
     * Any previous transforms are dropped when this method is called.
     *
     * The fringe is the size of the scissor border in pixels.  A value less than
     * 0 gives a sharp transition, where larger values have more gradual transitions.
     *
     * @param rect      The scissor mask bounds
     * @param fringe    The size of the scissor border in pixels
     *
     * @return this scissor mask, returned for chaining
     */
    Scissor& set(const Rect rect, float fringe=0.5);

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
    Scissor& set(const Rect rect, const Affine2& aff, float fringe=0.5);

    /**
     * Sets the scissor mask to have the given transformed bounds and fringe.
     *
     * Any previous transforms are dropped when this method is called.
     *
     * The fringe is the size of the scissor border in pixels.  A value less than
     * 0 gives a sharp transition, where larger values have more gradual transitions.
     *
     * All z-components from the given matrix transform are lost.
     *
     * @param rect      The scissor mask bounds
     * @param mat       The scissor mask transform
     * @param fringe    The size of the scissor border in pixels
     *
     * @return this scissor mask, returned for chaining
     */
    Scissor& set(const Rect rect, const Mat4& mat, float fringe=0.5);
    
    /**
     * Sets this to be a degenerate scissor of size 0.  
     *
     * All pixels will be dropped by this mask.
     *
     * @return this scissor mask, returned for chaining
     */
    Scissor& setZero();
    

#pragma mark -
#pragma mark Attributes
    /**
     * Returns the transform component of this scissor mask
     *
     * If the scissor mask is not rotated or otherwise transformed, this
     * value will be the identity.
     *
     * This value only contains the transform on the scissor mask bounding
     * box.  It is not the same as the scissor matrix in a scissor shader.  Do
     * not pass this information directly to the shader.  Use either the method
     * {@link getData} or {@link getComponents} depending on whether or not you
     * need std140 representation.
     *
     * @return the transform component of this scissor mask
     */
    Affine2 getTransform() const { return _transform; }

    /**
     * Sets the transform component of this scissor mask
     *
     * If the scissor mask is not rotated or otherwise transformed, this
     * value should be the identity.
     *
     * This value only contains the transform on the scissor mask bounding
     * box. It is not the same as the scissor matrix in a scissor shader.  Do
     * not pass this information directly to the shader.  Use either the method
     * {@link getData} or {@link getComponents} depending on whether or not you
     * need std140 representation.
     *
     * @param transform The transform component of this scissor mask
     */
    void setTransform(const Affine2& transform);

    /**
     * Sets the transform component of this scissor mask
     *
     * If the scissor mask is not rotated or otherwise transformed, this
     * value should be the identity.
     *
     * This value only contains the transform on the scissor mask bounding
     * box. It is not the same as the scissor matrix in a scissor shader.  Do
     * not pass this information directly to the shader.  Use either the method
     * {@link getData} or {@link getComponents} depending on whether or not you
     * need std140 representation.
     *
     * @param transform The transform component of this scissor mask
     */
    void setTransform(const Mat4& transform);

    /**
     * Returns the bounding box of this scissor mask
     *
     * The bounding box is axis-aligned.  It ignored the transform component
     * of the scissor mask.
     *
     * @return the bounding box of this scissor mask
     */
    Rect getBounds() const { return _bounds; }

    /**
     * Sets the bounding box of this scissor mask
     *
     * The bounding box is axis-aligned. It ignores the transform component
     * of the scissor mask.
     *
     * @param bounds    The bounding box of this scissor mask
     */
    void setBounds(const Rect bounds);
    
    /**
     * Returns the edge fringe of this scissor mask
     *
     * The fringe is the size of the scissor border in pixels.  A value less than
     * 0 gives a sharp transition, where larger values have more gradual transitions.
     *
     * @return the edge fringe of this scissor mask
     */
    float getFringe() const { return _fringe; }

    /**
     * Sets the edge fringe of this scissor mask
     *
     * The fringe is the size of the scissor border in pixels.  A value less than
     * 0 gives a sharp transition, where larger values have more gradual transitions.
     *
     * @param fringe    The edge fringe of this scissor mask
     */
    void setFringe(float fringe) { _fringe = fringe; }


#pragma mark -
#pragma mark Transforms
    /**
     * Applies a rotation to this scissor mask.
     *
     * The rotation is in radians, counter-clockwise about the given axis.
     *
     * @param angle The angle (in radians).
     *
     * @return This scissor mask, after rotation.
     */
    Scissor& rotate(float angle);
    
    /**
     * Applies a uniform scale to this scissor mask.
     *
     * @param value The scalar to multiply by.
     *
     * @return This scissor mask, after scaling.
     */
    Scissor& scale(float value);
    
    /**
     * Applies a non-uniform scale to this scissor mask.
     *
     * @param s        The vector storing the individual scaling factors
     *
     * @return This scissor mask, after scaling.
     */
    Scissor& scale(const Vec2 s);
    
    /**
     * Applies a non-uniform scale to this scissor mask.
     *
     * @param sx    The amount to scale along the x-axis.
     * @param sy    The amount to scale along the y-axis.
     *
     * @return This scissor mask, after scaling.
     */
    Scissor& scale(float sx, float sy) {
        return scale(Vec2(sx,sy));
    }
    
    /**
     * Applies a translation to this gradient.
     *
     * @param t     The vector storing the individual translation offsets
     *
     * @return This scissor mask, after translation.
     */
    Scissor& translate(const Vec2 t);
    
    /**
     * Applies a translation to this gradient.
     *
     * @param tx    The translation offset for the x-axis.
     * @param ty    The translation offset for the y-axis.
     *
     * @return This scissor mask, after translation.
     */
    Scissor& translate(float tx, float ty) {
        return translate(Vec2(tx,ty));
    }

    /**
     * Applies the given transform to this scissor mask.
     *
     * This transform is applied after the existing gradient transform
     * (which is natural, since the transform defines the gradient shape).
     * To pre-multiply a transform, set the transform directly.
     *
     * @param mat The matrix to multiply by.
     *
     * @return A reference to this (modified) scissor mask for chaining.
     */
    Scissor& multiply(const Mat4& mat) {
        return *this *= mat;
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
    Scissor& multiply(const Affine2& aff) {
        return *this *= aff;
    }
        
    /**
     * Applies the given transform to this scissor mask.
     *
     * This transform is applied after the existing gradient transform
     * (which is natural, since the transform defines the gradient shape).
     * To pre-multiply a transform, set the transform directly.
     *
     * @param mat The matrix to multiply by.
     *
     * @return A reference to this (modified) scissor mask for chaining.
     */
    Scissor& operator*=(const Mat4& mat);

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
    Scissor& operator*=(const Affine2& aff);

    /**
     * Returns a copy of the scissor mask transformed by the given matrix.
     *
     * The matrix transform is applied after the existing scissor transform
     * (which is natural, since the transform defines the initial box bounds).
     * To pre-multiply a transform, set the transform directly.
     *
     * Note: This does not modify the scissor mask.
     *
     * @param mat   The transform to multiply by.
     *
     * @return a copy of the scissor mask transformed by the given matrix.
     */
    Scissor operator*(const Mat4& mat) const {
        Scissor result = *this;
        return result *= mat;
    }
    
    /**
     * Returns a copy of the scissor mask transformed by the given matrix.
     *
     * The matrix transform is applied after the existing scissor transform
     * (which is natural, since the transform defines the initial box bounds).
     * To pre-multiply a transform, set the transform directly.
     *
     * Note: This does not modify the scissor mask.
     *
     * @param aff   The transform to multiply by.
     *
     * @return a copy of the scissor mask transformed by the given matrix.
     */
    Scissor operator*(const Affine2& aff) const {
        Scissor result = *this;
        return result *= aff;
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
     * @param mask  The scissor mask to intersect with this one
     *
     * @return a reference to the scissor for chaining
     */
    Scissor& intersect(const Scissor& mask);
    
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
     * @param mask  The scissor mask to intersect with this one
     *
     * @return the intersection of the given scissor mask with this one.
     */
    Scissor getIntersection(const Scissor& mask) const;
    
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
     * @param mask  The scissor mask to intersect with this one
     *
     * @return a reference to the scissor for chaining
     */
    Scissor& intersect(const std::shared_ptr<Scissor>& mask) {
        return intersect(*mask);
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
     * @param mask  The scissor mask to intersect with this one
     *
     * @return the intersection of the given scissor mask with this one.
     */
    std::shared_ptr<Scissor> getIntersection(const std::shared_ptr<Scissor>& mask) const;

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
    float* getData(float* array) const;

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
    float* getComponents(float* array) const;
    
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
    std::string toString(bool verbose = false) const;
        
    /** Cast from Scissor to a string. */
    operator std::string() const { return toString(); }

private:
    /**
     * Recomputes the internal transform for OpenGL.
     */
    void recompute();
};

}
#endif /* __CU_SCISSOR_H__ */
