//
//  CUPolyFactory.h
//
//  This module provides a convenient way to generate simple (solid) polygons,
//  like circles and rounded rectangles. It is lighter weight than the other
//  factory classes because it does not separate the calculation step from the
//  materialization step. That is because all of its calculations are very short
//  and do not need to be factored into a separate thread.
//
//  Because math objects are intended to be on the stack, we do not provide
//  any shared pointer support in this class.
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
//  Version: 7/12/21
//
#ifndef __CU_POLY_FACTORY_H__
#define __CU_POLY_FACTORY_H__

#include <cugl/math/CUPoly2.h>
#include <cugl/math/CURect.h>
#include <cugl/math/polygon/CUPolyEnums.h>

namespace cugl {

/**
 * This class is a factory for generating common {@link Poly2} objects.
 *
 * Most of the time that we create a solid polygon, we are using it to approximate a common
 * shape, like a circle, or a rounded rectangle.  Instead of embedding all of this
 * functionality into {@link Poly2} (which already has enough to do on its own), we
 * have factored this out into a a separate factory class. This factory can generate
 * new polygons or reset existing ones (conserving memory).
 *
 * This factory is much lighter weight than the triangulation or extrusion factories.
 * In this factory, the calculation step and the materialization step are one in the
 * same. That is because the calculations are short and do not need to be refactored
 * for multithread calculation. Indeed, the only reason this factory is not a
 * collection of simple functions is because that we have have some settings (like
 * precision and geometry) that we want to set separately.
 */
class PolyFactory {
private:
    /** The curve tolerance for rounded shapes */
    float _tolerance;

#pragma mark Initialization
public:
    /**
     * Creates a PolyFactory for generating solid polygons.
     *
     * This factory will use the default tolerance.
     */
    PolyFactory();

    /**
     * Creates a factory for generating solid polygons with the given tolerance.
     *
     * @param tol   The curve tolerance for rounded shapes
     */
    PolyFactory(float tol);

    /**
     * Returns the curve tolerance for rounded shapes.
     *
     * The tolerance guarantees that curved shapes have enough segments so that
     * any points on the true shape are always within tolerance of the segmented
     * approximation.
     *
     * Rounded shapes include {@link #makeEllipse}, {@link #makeCircle},
     * {@link #makeArc}, and {@link #makeRoundedRect}.
     *
     * @return the curve tolerance for rounded shapes.
     */
    float getTolerance() const {
        return _tolerance;
    }

    /**
     * Sets the curve tolerance for rounded shapes.
     *
     * The tolerance guarantees that curved shapes have enough segments so that
     * any points on the true shape are always within tolerance of the segmented
     * approximation.
     *
     * Rounded shapes include {@link #makeEllipse}, {@link #makeCircle},
     * {@link #makeArc}, and {@link #makeRoundedRect}.
     *
     * @param tol   The curve tolerance for rounded shapes.
     */
    void setTolerance(float tol) {
        _tolerance = tol;
    }

#pragma mark Unrounded Shapes
    /**
     * Returns a solid polygon that represents a simple triangle.
     *
     * @param  a    The first vertex.
     * @param  b    The second vertex.
     * @param  c    The third vertex.
     *
     * @return a solid polygon that represents a simple triangle.
     */
    Poly2 makeTriangle(const Vec2 a, const Vec2 b, const Vec2 c) const {
        return makeTriangle(a.x,a.y,b.x,b.y,c.x,c.y);
    }

    /**
     * Returns a solid polygon that represents a simple triangle.
     *
     * @param  ax   The x-coordinate of the first vertex.
     * @param  ay   The y-coordinate of the first vertex.
     * @param  bx   The x-coordinate of the second vertex.
     * @param  by   The y-coordinate of the second vertex.
     * @param  cx   The x-coordinate of the third vertex.
     * @param  cy   The y-coordinate of the third vertex.
     *
     * @return a solid polygon that represents a simple triangle.
     */
    Poly2 makeTriangle(float ax, float ay, float bx, float by, float cx, float cy) const;

    /**
     * Stores a simple triangle in the provided buffer.
     *
     * The triangle will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly  The polygon to store the result
     * @param a     The first vertex
     * @param b     The second vertex
     * @param c     The third vertex
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeTriangle(Poly2* poly, const Vec2 a, const Vec2 b, const Vec2 c) const {
        return makeTriangle( poly, a.x,a.y,b.x,b.y,c.x,c.y );
    }

    /**
     * Stores a simple triangle in the provided buffer.
     *
     * The triangle will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly  The polygon to store the result
     * @param ax    The x-coordinate of the first vertex
     * @param ay    The y-coordinate of the first vertex
     * @param bx    The x-coordinate of the second vertex
     * @param by    The y-coordinate of the second vertex
     * @param cx    The x-coordinate of the third vertex
     * @param cy    The y-coordinate of the third vertex
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeTriangle(Poly2* poly, float ax, float ay, float bx, float by, float cx, float cy) const;
    
    /**
     * Returns a solid polygon that represents a rectangle
     *
     * @param origin    The rectangle origin
     * @param size      The rectangle size
     *
     * @return a solid polygon that represents a rectangle
     */
    Poly2 makeRect(const Vec2 origin, const Vec2 size) const {
        return makeRect( origin.x, origin.y, size.x, size.y );
    }
    
    /**
     * Returns a solid polygon that represents a rectangle
     *
     * @param rect      The rectangle
     *
     * @return a solid polygon that represents a rectangle
     */
    Poly2 makeRect(const Rect rect) const {
        return makeRect( rect.origin.x, rect.origin.y, rect.size.width, rect.size.height );
    }

    /**
     * Returns a solid polygon that represents a rectangle
     *
     * @param x     The x-coordinate of the bottom left corner
     * @param y     The y-coordinate of the bottom left corner
     * @param w     The rectangle width
     * @param h     The rectangle height
     *
     * @return a solid polygon that represents a rectangle
     */
    Poly2 makeRect(float x, float y, float w, float h) const;

    /**
     * Stores a rectangle in the provided buffer.
     *
     * The rectangle will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly      The polygon to store the result
     * @param origin    The rectangle origin
     * @param size      The rectangle size
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeRect(Poly2* poly, const Vec2 origin, const Vec2 size) const {
        return makeRect( poly, origin.x, origin.y, size.x, size.y );
    }
    
    /**
     * Stores a rectangle in the provided buffer.
     *
     * The rectangle will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly      The polygon to store the result
     * @param rect      The rectangle to create
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeRect(Poly2* poly, const Rect rect) const {
        return makeRect( poly, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height );
    }

    /**
     * Stores a rectangle in the provided buffer.
     *
     * The rectangle will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly  The polygon to store the result
     * @param x     The x-coordinate of the bottom left corner
     * @param y     The y-coordinate of the bottom left corner
     * @param w     The rectangle width
     * @param h     The rectangle height
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeRect(Poly2* poly, float x, float y, float w, float h) const;
    
    /**
     * Returns a solid polygon that represents a regular, many-sided polygon.
     *
     * The polygon will be centered at the given origin with the given radius.
     * A regular polygon is essentially a circle where the number of segments
     * is explicit (instead of implicit from the curve tolerance).
     *
     * @param center    The polygon center point
     * @param radius    The polygon radius
     * @param sides     The number of sides
     *
     * @return a solid polygon that represents a regular, many-sided polygon.
     */
    Poly2 makeNgon(const Vec2 center, float radius, Uint32 sides) const {
        return makeNgon( center.x, center.y, radius, sides );
    }

    /**
     * Returns a solid polygon that represents a regular, many-sided polygon.
     *
     * The polygon will be centered at the given origin with the given radius.
     * A regular polygon is essentially a circle where the number of segments
     * is explicit (instead of implicit from the curve tolerance).
     *
     * @param cx        The x-coordinate of the center point
     * @param cy        The y-coordinate of the center point
     * @param radius    The polygon radius
     * @param sides     The number of sides
     *
     * @return a solid polygon that represents a regular, many-sided polygon.
     */
    Poly2 makeNgon(float cx, float cy, float radius, Uint32 sides) const;

    /**
     * Stores a regular, many-sided polygon in the provided buffer.
     *
     * The polygon will be centered at the given origin with the given radius.
     * A regular polygon is essentially a circle where the number of segments
     * is explicit (instead of implicit from the curve tolerance).
     *
     * The polygon will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly      The polygon to store the result
     * @param center    The polygon center point
     * @param radius    The polygon radius
     * @param sides     The number of sides
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeNgon(Poly2* poly, const Vec2 center, float radius, Uint32 sides) const {
        return makeNgon( poly, center.x, center.y, radius, sides );
    }

    /**
     * Stores a regular, many-sided polygon in the provided buffer.
     *
     * The polygon will be centered at the given origin with the given radius.
     * A regular polygon is essentially a circle where the number of segments
     * is explicit (instead of implicit from the curve tolerance).
     *
     * The polygon will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly      The polygon to store the result
     * @param cx        The x-coordinate of the center point
     * @param cy        The y-coordinate of the center point
     * @param radius    The polygon radius
     * @param sides     The number of sides
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeNgon(Poly2* poly, float cx, float cy, float radius, Uint32 sides) const;
    
#pragma mark Rounded Shapes
    /**
     * Returns a solid polygon that represents an ellipse of the given dimensions.
     *
     * @param center    The ellipse center point
     * @param size      The size of the ellipse
     *
     * @return a solid polygon that represents an ellipse of the given dimensions.
     */
    Poly2 makeEllipse(const Vec2 center, const Vec2 size) const {
        return makeEllipse( center.x, center.y, size.x, size.y );
    }

    /**
     * Returns a solid polygon that represents an ellipse of the given dimensions.
     *
     * @param cx    The x-coordinate of the center point
     * @param cy    The y-coordinate of the center point
     * @param sx    The size (diameter) along the x-axis
     * @param sy    The size (diameter) along the y-axis
     *
     * @return a solid polygon that represents an ellipse of the given dimensions.
     */
    Poly2 makeEllipse(float cx, float cy, float sx, float sy) const;

    /**
     * Stores an ellipse in the provided buffer.
     *
     * The ellipse will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly      The polygon to store the result
     * @param center    The ellipse center point
     * @param size      The size of the ellipse
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeEllipse(Poly2* poly, const Vec2 center, const Vec2 size) const {
        return makeEllipse( poly, center.x, center.y, size.x, size.y );
    }

    /**
     * Stores an ellipse in the provided buffer.
     *
     * The ellipse will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly  The polygon to store the result
     * @param cx    The x-coordinate of the center point
     * @param cy    The y-coordinate of the center point
     * @param sx    The size (diameter) along the x-axis
     * @param sy    The size (diameter) along the y-axis
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeEllipse(Poly2* poly, float cx, float cy, float sx, float sy) const;
    
    /**
     * Returns a solid polygon that represents a circle of the given dimensions.
     *
     * @param center    The circle center point
     * @param radius    The circle radius
     *
     * @return a solid polygon that represents an circle of the given dimensions.
     */
    Poly2 makeCircle(const Vec2 center, float radius) const {
        return makeCircle( center.x, center.y, radius );
    }

    /**
     * Returns a solid polygon that represents a circle of the given dimensions.
     *
     * @param cx        The x-coordinate of the center point
     * @param cy        The y-coordinate of the center point
     * @param radius    The circle radius
     *
     * @return a solid polygon that represents an circle of the given dimensions.
     */
    Poly2 makeCircle(float cx, float cy, float radius) const;

    /**
     * Stores a circle in the provided buffer.
     *
     * The circle will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly      The polygon to store the result
     * @param center    The circle center point
     * @param radius    The circle radius
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeCircle(Poly2* poly, const Vec2 center, float radius) const {
        return makeCircle( poly, center.x, center.y, radius );
    }

    /**
     * Stores a circle in the provided buffer.
     *
     * The circle will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly      The polygon to store the result
     * @param cx        The x-coordinate of the center point
     * @param cy        The y-coordinate of the center point
     * @param radius    The circle radius
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeCircle(Poly2* poly, float cx, float cy, float radius) const;

    /**
     * Returns a solid polygon that represents an arc of the given dimensions.
     *
     * All arc measurements are in degrees, not radians.
     *
     * @param center    The arc center point (of the defining circle
     * @param radius    The radius from the center point
     * @param start     The starting angle in degrees
     * @param degrees   The number of degrees to generate
     *
     * @return a solid polygon that represents an arc of the given dimensions.
     */
    Poly2 makeArc(const Vec2 center, float radius, float start, float degrees) const {
        return makeArc(center.x,center.y,radius,start,degrees);
    }

    /**
     * Returns a solid polygon that represents an arc of the given dimensions.
     *
     * All arc measurements are in degrees, not radians.
     *
     * @param cx        The x-coordinate of the center point
     * @param cy        The y-coordinate of the center point
     * @param radius    The radius from the center point
     * @param start     The starting angle in degrees
     * @param degrees   The number of degrees to generate
     *
     * @return a solid polygon that represents an arc of the given dimensions.
     */
    Poly2 makeArc(float cx, float cy, float radius, float start, float degrees) const;

    /**
     * Stores an arc in the provided buffer.
     *
     * All arc measurements are in degrees, not radians.
     *
     * The arc will be appended to the buffer.  You should clear the buffer first
     * if you do not want to preserve the original data.
     *
     * @param poly      The polygon to store the result
     * @param center    The arc center point (of the defining circle
     * @param radius    The radius from the center point
     * @param start     The starting angle in degrees
     * @param degrees   The number of degrees to generate
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeArc(Poly2* poly, const Vec2 center, float radius, float start, float degrees) const {
        return makeArc(poly,center.x,center.y,radius,start,degrees);
    }

    /**
     * Stores an arc in the provided buffer.
     *
     * All arc measurements are in degrees, not radians.
     *
     * The arc will be appended to the buffer.  You should clear the buffer first
     * if you do not want to preserve the original data.
     *
     * @param poly      The polygon to store the result
     * @param cx        The x-coordinate of the center point
     * @param cy        The y-coordinate of the center point
     * @param radius    The radius from the center point
     * @param start     The starting angle in degrees
     * @param degrees   The number of degrees to generate
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeArc(Poly2* poly, float cx, float cy, float radius, float start, float degrees) const;

    /**
     * Returns a solid polygon that represents a rounded rectangle of the given dimensions.
     *
     * The radius should not exceed either half the width or half the height.
     *
     * @param origin    The enclosing rectangle origin
     * @param size      The enclosing rectangle size
     * @param radius    The radius of each corner
     *
     * @return a solid polygon that represents a rounded rectangle of the given dimensions.
     */
    Poly2 makeRoundedRect(const Vec2 origin, const Size size, float radius) const {
        return makeRoundedRect(origin.x, origin.y, size.width, size.height, radius);
    }
    
    /**
     * Returns a solid polygon that represents a rounded rectangle of the given dimensions.
     *
     * The radius should not exceed either half the width or half the height.
     *
     * @param rect      The enclosing rectangle
     * @param radius    The radius of each corner
     *
     * @return a solid polygon that represents a rounded rectangle of the given dimensions.
     */
    Poly2 makeRoundedRect(const Rect rect, float radius) const {
        return makeRoundedRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height, radius);
    }

    /**
     * Returns a solid polygon that represents a rounded rectangle of the given dimensions.
     *
     * The radius should not exceed either half the width or half the height.
     *
     * @param x     The x-coordinate of the bottom left corner of the bounding box
     * @param y     The y-coordinate of the bottom left corner of the bounding box
     * @param w     The rectangle width
     * @param h     The rectangle height
     * @param r     The radius of each corner
     *
     * @return a solid polygon that represents a rounded rectangle of the given dimensions.
     */
    Poly2 makeRoundedRect(float x, float y, float w, float h, float r) const;

    /**
     * Stores a rounded rectangle in the provided buffer.
     *
     * The radius should not exceed either half the width or half the height.
     *
     * The rounded rectangle will be appended to the buffer.  You should clear the
     * buffer first if you do not want to preserve the original data.
     *
     * @param poly      The polygon to store the result
     * @param origin    The enclosing rectangle origin
     * @param size      The enclosing rectangle size
     * @param radius    The radius of each corner
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeRoundedRect(Poly2* poly, const Vec2 origin, const Size size, float radius) const {
        return makeRoundedRect(poly, origin.x, origin.y, size.width, size.height, radius);
    }

    /**
     * Stores a rounded rectangle in the provided buffer.
     *
     * The radius should not exceed either half the width or half the height.
     *
     * The rounded rectangle will be appended to the buffer.  You should clear the
     * buffer first if you do not want to preserve the original data.
     *
     * @param poly      The polygon to store the result
     * @param rect      The enclosing rectangle
     * @param radius    The radius of each corner
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeRoundedRect(Poly2* poly, const Rect rect, float radius) const {
        return makeRoundedRect(poly, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height, radius);
    }

    /**
     * Stores a rounded rectangle in the provided buffer.
     *
     * The radius should not exceed either half the width or half the height.
     *
     * The rounded rectangle will be appended to the buffer.  You should clear the
     * buffer first if you do not want to preserve the original data.
     *
     * @param poly  The polygon to store the result
     * @param x     The x-coordinate of the bottom left corner of the bounding box
     * @param y     The y-coordinate of the bottom left corner of the bounding box
     * @param w     The rectangle width
     * @param h     The rectangle height
     * @param r     The radius of each corner
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeRoundedRect(Poly2* poly, float x, float y, float w, float h, float r) const;

#pragma mark Capsules
    /**
     * Returns a solid polygon that represents a (full) capsule of the given dimensions.
     *
     * A capsule is a pill-like shape that fits inside of given rectangle.  If
     * width < height, the capsule will be oriented vertically with the rounded
     * portions at the top and bottom. Otherwise it will be oriented horizontally.
     *
     * @param origin    The enclosing rectangle origin
     * @param size      The enclosing rectangle size
     *
     * @return a solid polygon that represents a (full) capsule of the given dimensions.
     */
    Poly2 makeCapsule(const Vec2 origin, const Size size) const {
        return makeCapsule(origin.x, origin.y, size.width, size.height);
    }
    
    /**
     * Returns a solid polygon that represents a (full) capsule of the given dimensions.
     *
     * A capsule is a pill-like shape that fits inside of given rectangle.  If
     * width < height, the capsule will be oriented vertically with the rounded
     * portions at the top and bottom. Otherwise it will be oriented horizontally.
     *
     * @param rect      The enclosing rectangle
     *
     * @return a solid polygon that represents a (full) capsule of the given dimensions.
     */
    Poly2 makeCapsule(const Rect rect) const {
        return makeCapsule(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
    }

    /**
     * Returns a solid polygon that represents a (full) capsule of the given dimensions.
     *
     * A capsule is a pill-like shape that fits inside of given rectangle.  If
     * width < height, the capsule will be oriented vertically with the rounded
     * portions at the top and bottom. Otherwise it will be oriented horizontally.
     *
     * @param x     The x-coordinate of the bottom left corner of the bounding box
     * @param y     The y-coordinate of the bottom left corner of the bounding box
     * @param w     The capsule width
     * @param h     The capsule height
     *
     * @return a solid polygon that represents a (full) capsule of the given dimensions.
     */
    Poly2 makeCapsule(float x, float y, float w, float h) const;

    /**
     * Stores a (full) capsule in the provided buffer.
     *
     * A capsule is a pill-like shape that fits inside of given rectangle.  If
     * width < height, the capsule will be oriented vertically with the rounded
     * portions at the top and bottom. Otherwise it will be oriented horizontally.
     *
     * The capsule will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly      The polygon to store the result
     * @param origin    The enclosing rectangle origin
     * @param size      The enclosing rectangle size
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeCapsule(Poly2* poly, const Vec2 origin, const Size size) const {
        return makeCapsule( poly, origin.x, origin.y, size.width, size.height);
    }
    
    /**
     * Stores a (full) capsule in the provided buffer.
     *
     * A capsule is a pill-like shape that fits inside of given rectangle.  If
     * width < height, the capsule will be oriented vertically with the rounded
     * portions at the top and bottom. Otherwise it will be oriented horizontally.
     *
     * The capsule will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly  The polygon to store the result
     * @param rect  The enclosing rectangle
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeCapsule(Poly2* poly, const Rect rect) const {
        return makeCapsule( poly, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
    }

    /**
     * Stores a (full) capsule in the provided buffer.
     *
     * A capsule is a pill-like shape that fits inside of given rectangle.  If
     * width < height, the capsule will be oriented vertically with the rounded
     * portions at the top and bottom.Otherwise it will be oriented horizontally.
     *
     * The capsule will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly  The polygon to store the result
     * @param x     The x-coordinate of the bottom left corner of the bounding box
     * @param y     The y-coordinate of the bottom left corner of the bounding box
     * @param w     The capsule width
     * @param h     The capsule height
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeCapsule(Poly2* poly, float x, float y, float w, float h) const;

    /**
     * Returns a solid polygon that represents a capsule of the given dimensions.
     *
     * A capsule typically is a pill-like shape that fits inside of given rectangle.
     * If width < height, the capsule will be oriented vertically with the rounded
     * portions at the top and bottom. Otherwise it will be oriented horizontally.
     *
     * This method allows for the creation of half-capsules, simply by using the
     * enumeration {@link poly2::Capsule}. The enumeration specifies which side
     * should be rounded in case of a half-capsule. Half-capsules are sized so that
     * the corresponding full capsule would fit in the bounding box.
     *
     * @param shape     The capsule shape
     * @param origin    The enclosing rectangle origin
     * @param size      The enclosing rectangle size
     *
     * @return a solid polygon that represents a capsule of the given dimensions.
     */
    Poly2 makeCapsule(poly2::Capsule shape, const Vec2 origin, const Size size) const {
        return makeCapsule(shape, origin.x, origin.y, size.width, size.height);
    }
    
    /**
     * Returns a solid polygon that represents a capsule of the given dimensions.
     *
     * A capsule typically is a pill-like shape that fits inside of given rectangle.
     * If width < height, the capsule will be oriented vertically with the rounded
     * portions at the top and bottom. Otherwise it will be oriented horizontally.
     *
     * This method allows for the creation of half-capsules, simply by using the
     * enumeration {@link poly2::Capsule}. The enumeration specifies which side
     * should be rounded in case of a half-capsule. Half-capsules are sized so that
     * the corresponding full capsule would fit in the bounding box.
     *
     * @param shape     The capsule shape
     * @param rect      The enclosing rectangle
     *
     * @return a solid polygon that represents a capsule of the given dimensions.
     */
    Poly2 makeCapsule(poly2::Capsule shape, const Rect rect) const {
        return makeCapsule(shape, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
    }

    /**
     * Returns a solid polygon that represents a (full) capsule of the given dimensions.
     *
     * A capsule typically is a pill-like shape that fits inside of given rectangle.
     * If width < height, the capsule will be oriented vertically with the rounded
     * portions at the top and bottom. Otherwise it will be oriented horizontally.
     *
     * This method allows for the creation of half-capsules, simply by using the
     * enumeration {@link poly2::Capsule}. The enumeration specifies which side
     * should be rounded in case of a half-capsule. Half-capsules are sized so that
     * the corresponding full capsule would fit in the bounding box.
     *
     * @param shape     The capsule shape
     * @param x         The x-coordinate of the bottom left corner of the bounding box
     * @param y         The y-coordinate of the bottom left corner of the bounding box
     * @param w         The capsule width
     * @param h         The capsule height
     *
     * @return a solid polygon that represents a capsule of the given dimensions.
     */
    Poly2 makeCapsule(poly2::Capsule shape, float x, float y, float w, float h) const;

    /**
     * Stores a capsule in the provided buffer.
     *
     * A capsule typically is a pill-like shape that fits inside of given rectangle.
     * If width < height, the capsule will be oriented vertically with the rounded
     * portions at the top and bottom. Otherwise it will be oriented horizontally.
     *
     * This method allows for the creation of half-capsules, simply by using the
     * enumeration {@link poly2::Capsule}. The enumeration specifies which side
     * should be rounded in case of a half-capsule. Half-capsules are sized so that
     * the corresponding full capsule would fit in the bounding box.
     *
     * The capsule will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly      The polygon to store the result
     * @param shape     The capsule shape
     * @param origin    The enclosing rectangle origin
     * @param size      The enclosing rectangle size
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeCapsule(Poly2* poly, poly2::Capsule shape, const Vec2 origin, const Size size) const {
        return makeCapsule( poly, shape, origin.x, origin.y, size.width, size.height);
    }
    
    /**
     * Stores a capsule in the provided buffer.
     *
     * A capsule typically is a pill-like shape that fits inside of given rectangle.
     * If width < height, the capsule will be oriented vertically with the rounded
     * portions at the top and bottom. Otherwise it will be oriented horizontally.
     *
     * This method allows for the creation of half-capsules, simply by using the
     * enumeration {@link poly2::Capsule}. The enumeration specifies which side
     * should be rounded in case of a half-capsule. Half-capsules are sized so that
     * the corresponding full capsule would fit in the bounding box.
     *
     * The capsule will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly      The polygon to store the result
     * @param shape     The capsule shape
     * @param rect      The enclosing rectangle
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeCapsule(Poly2* poly, poly2::Capsule shape, const Rect rect) const {
        return makeCapsule( poly, shape, rect.origin.x, rect.origin.y,
                            rect.size.width, rect.size.height);
    }

    /**
     * Stores a capsule in the provided buffer.
     *
     * A capsule typically is a pill-like shape that fits inside of given rectangle.
     * If width < height, the capsule will be oriented vertically with the rounded
     * portions at the top and bottom. Otherwise it will be oriented horizontally.
     *
     * This method allows for the creation of half-capsules, simply by using the
     * enumeration {@link poly2::Capsule}. The enumeration specifies which side
     * should be rounded in case of a half-capsule. Half-capsules are sized so that
     * the corresponding full capsule would fit in the bounding box.
     *
     * The capsule will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly      The polygon to store the result
     * @param shape     The capsule shape
     * @param x         The x-coordinate of the bottom left corner of the bounding box
     * @param y         The y-coordinate of the bottom left corner of the bounding box
     * @param w         The capsule width
     * @param h         The capsule height
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* makeCapsule(Poly2* poly, poly2::Capsule shape, float x, float y, float w, float h) const;

};

}

#endif /* __CU_POLY_FACTORY_H__ */
