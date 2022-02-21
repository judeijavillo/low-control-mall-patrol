//
//  CUPathFactory.h
//
//  This module provides a convenient way to generate simple paths, like lines
//  or circles. It is lighter weight than the other factory classes because it
//  does not separate the calculation step from the materialization step. That
//  is because all of its calculations are very short and do not need to be
//  factored into a separate thread.
//
//  This class differs from PolyFactory in that it only produces paths, and
//  not pre-triangulated polygons.
//
//  Because math objects are intended to be on the stack, we do not provide
//  any shared pointer support in this class.
//
//  This implementation is largely inspired by the LibGDX implementation from
//  Nicolas Gramlich, Eric Spits, Thomas Cate, and Nathan Sweet.
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
#ifndef __CU_PATH_FACTORY_H__
#define __CU_PATH_FACTORY_H__

#include <cugl/math/CUPath2.h>
#include <cugl/math/CURect.h>
#include <cugl/math/polygon/CUPolyEnums.h>

namespace cugl {

// Forward declaration
class Poly2;


/**
 * This class is a factory for generating common {@link Path2} objects.
 *
 * Most of the time that we create a path, we are using it to approximate a common
 * shape, like a circle, or a rounded rectangle.  Instead of embedding all of this
 * functionality into {@link Path2} (which already has enough to do on its own), we
 * have factored this out into a a separate factory class. This factory can generate
 * new path or reset existing ones (conserving memory).
 *
 * Note that this factory only generates path, and does not create {@link Path2}
 * objects. It is intended for shapes that will be drawn as lines or extruded
 * later. If you want a solid (e.g. triangulated) shape, use {@link PolyFactory}
 * instead.
 */
class PathFactory {
private:
    /** The curve tolerance for rounded shapes */
    float _tolerance;

#pragma mark Initialization
public:
    /**
     * Creates a factory for generating common paths.
     *
     * This factory will use the default tolerance.
     */
    PathFactory();

    /**
     * Creates a factory for generating common paths with the given tolerance.
     *
     * @param tol   The curve tolerance for rounded shapes
     */
    PathFactory(float tol);

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
     * Returns a path that represents a line segment from origin to dest.
     *
     * @param origin    The line origin
     * @param dest      The line destination
     *
     * @return a path that represents a line segment from origin to dest.
     */
    Path2 makeLine(const Vec2 origin, const Vec2 dest) const {
        return makeLine(origin.x,origin.y,dest.x,dest.y);
    }

    /**
     * Returns a path that represents a line segment from origin to dest.
     *
     * @param ox    The x-coordinate of the origin
     * @param oy    The y-coordinate of the origin
     * @param dx    The x-coordinate of the destination
     * @param dy    The y-coordinate of the destination
     *
     * @return a path that represents a line segment from origin to dest.
     */
    Path2 makeLine(float ox, float oy, float dx, float dy) const;

    /**
     * Stores a line segment from origin to dest in the provided buffer.
     *
     * The line will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param path      The path to store the result
     * @param origin    The line origin
     * @param dest      The line destination
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeLine(Path2* path, const Vec2 origin, const Vec2 dest) const {
        return makeLine(path,origin.x,origin.y,dest.x,dest.y);
    }

    /**
     * Stores a line segment from origin to dest in the provided buffer.
     *
     * The line will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param path  The path to store the result
     * @param ox    The x-coordinate of the origin
     * @param oy    The y-coordinate of the origin
     * @param dx    The x-coordinate of the destination
     * @param dy    The y-coordinate of the destination
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeLine(Path2* path, float ox, float oy, float dx, float dy) const;
    
  
    /**
     * Returns a path that represents a simple triangle.
     *
     * @param  a    The first vertex.
     * @param  b    The second vertex.
     * @param  c    The third vertex.
     *
     * @return a path that represents a simple triangle.
     */
    Path2 makeTriangle(const Vec2 a, const Vec2 b, const Vec2 c) const {
        return makeTriangle(a.x,a.y,b.x,b.y,c.x,c.y);
    }

    /**
     * Returns a path that represents a simple triangle.
     *
     * @param  ax   The x-coordinate of the first vertex.
     * @param  ay   The y-coordinate of the first vertex.
     * @param  bx   The x-coordinate of the second vertex.
     * @param  by   The y-coordinate of the second vertex.
     * @param  cx   The x-coordinate of the third vertex.
     * @param  cy   The y-coordinate of the third vertex.
     *
     * @return a path that represents a simple triangle.
     */
    Path2 makeTriangle(float ax, float ay, float bx, float by, float cx, float cy) const;

    /**
     * Stores a simple triangle in the provided buffer.
     *
     * The triangle will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param path  The path to store the result
     * @param a     The first vertex
     * @param b     The second vertex
     * @param c     The third vertex
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeTriangle(Path2* path, const Vec2 a, const Vec2 b, const Vec2 c) const {
        return makeTriangle( path, a.x,a.y,b.x,b.y,c.x,c.y );
    }

    /**
     * Stores a simple triangle in the provided buffer.
     *
     * The triangle will be appended to the buffer. You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param path  The path to store the result
     * @param ax    The x-coordinate of the first vertex
     * @param ay    The y-coordinate of the first vertex
     * @param bx    The x-coordinate of the second vertex
     * @param by    The y-coordinate of the second vertex
     * @param cx    The x-coordinate of the third vertex
     * @param cy    The y-coordinate of the third vertex
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeTriangle(Path2* path, float ax, float ay, float bx, float by, float cx, float cy) const;
    
    /**
     * Returns a path that represents a rectangle
     *
     * @param origin    The rectangle origin
     * @param size      The rectangle size
     *
     * @return a path that represents a rectangle
     */
    Path2 makeRect(const Vec2 origin, const Vec2 size) const {
        return makeRect( origin.x, origin.y, size.x, size.y );
    }
    
    /**
     * Returns a path that represents a rectangle
     *
     * @param rect      The rectangle
     *
     * @return a path that represents a rectangle
     */
    Path2 makeRect(const Rect rect) const {
        return makeRect( rect.origin.x, rect.origin.y, rect.size.width, rect.size.height );
    }

    /**
     * Returns a path that represents a rectangle
     *
     * @param x     The x-coordinate of the bottom left corner
     * @param y     The y-coordinate of the bottom left corner
     * @param w     The rectangle width
     * @param h     The rectangle height
     *
     * @return a path that represents a rectangle
     */
    Path2 makeRect(float x, float y, float w, float h) const;

    /**
     * Stores a rectangle in the provided buffer.
     *
     * The rectangle will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param path      The path to store the result
     * @param origin    The rectangle origin
     * @param size      The rectangle size
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeRect(Path2* path, const Vec2 origin, const Vec2 size) const {
        return makeRect( path, origin.x, origin.y, size.x, size.y );
    }
    
    /**
     * Stores a rectangle in the provided buffer.
     *
     * The rectangle will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param path      The path to store the result
     * @param rect      The rectangle to create
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeRect(Path2* path, const Rect rect) const {
        return makeRect( path, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height );
    }

    /**
     * Stores a rectangle in the provided buffer.
     *
     * The rectangle will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param path  The path to store the result
     * @param x     The x-coordinate of the bottom left corner
     * @param y     The y-coordinate of the bottom left corner
     * @param w     The rectangle width
     * @param h     The rectangle height
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeRect(Path2* path, float x, float y, float w, float h) const;
    
    /**
     * Returns a path that represents a regular, many-sided polygon.
     *
     * The polygon will be centered at the given origin with the given radius.
     * A regular polygon is essentially a circle where the number of segments
     * is explicit (instead of implicit from the curve tolerance).
     *
     * @param center    The polygon center point
     * @param radius    The polygon radius
     * @param sides     The number of sides
     *
     * @return a path that represents a regular, many-sided polygon.
     */
    Path2 makeNgon(const Vec2 center, float radius, Uint32 sides) const {
        return makeNgon( center.x, center.y, radius, sides );
    }

    /**
     * Returns a path that represents a regular, many-sided polygon.
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
     * @return a path that represents a regular, many-sided polygon.
     */
    Path2 makeNgon(float cx, float cy, float radius, Uint32 sides) const;

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
     * @param path      The path to store the result
     * @param center    The polygon center point
     * @param radius    The polygon radius
     * @param sides     The number of sides
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeNgon(Path2* path, const Vec2 center, float radius, float sides) const {
        return makeNgon( path, center.x, center.y, radius, sides );
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
     * @param path      The path to store the result
     * @param cx        The x-coordinate of the center point
     * @param cy        The y-coordinate of the center point
     * @param radius    The polygon radius
     * @param sides     The number of sides
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeNgon(Path2* path, float cx, float cy, float radius, Uint32 sides) const;
    
#pragma mark Rounded Shapes
    /**
     * Returns a path that represents an ellipse of the given dimensions.
     *
     * @param center    The ellipse center point
     * @param size      The size of the ellipse
     *
     * @return a path that represents an ellipse of the given dimensions.
     */
    Path2 makeEllipse(const Vec2 center, const Vec2 size) const {
        return makeEllipse( center.x, center.y, size.x, size.y );
    }

    /**
     * Returns a path that represents an ellipse of the given dimensions.
     *
     * @param cx    The x-coordinate of the center point
     * @param cy    The y-coordinate of the center point
     * @param sx    The size (diameter) along the x-axis
     * @param sy    The size (diameter) along the y-axis
     *
     * @return a path that represents an ellipse of the given dimensions.
     */
    Path2 makeEllipse(float cx, float cy, float sx, float sy) const;

    /**
     * Stores an ellipse in the provided buffer.
     *
     * The ellipse will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param path      The path to store the result
     * @param center    The ellipse center point
     * @param size      The size of the ellipse
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeEllipse(Path2* path, const Vec2 center, const Vec2 size) const {
        return makeEllipse( path, center.x, center.y, size.x, size.y );
    }

    /**
     * Stores an ellipse in the provided buffer.
     *
     * The ellipse will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param path  The path to store the result
     * @param cx    The x-coordinate of the center point
     * @param cy    The y-coordinate of the center point
     * @param sx    The size (diameter) along the x-axis
     * @param sy    The size (diameter) along the y-axis
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeEllipse(Path2* path, float cx, float cy, float sx, float sy) const;
    
    /**
     * Returns a path that represents a circle of the given dimensions.
     *
     * @param center    The circle center point
     * @param radius    The circle radius
     *
     * @return a path that represents an circle of the given dimensions.
     */
    Path2 makeCircle(const Vec2 center, float radius) const {
        return makeCircle( center.x, center.y, radius );
    }

    /**
     * Returns a path that represents a circle of the given dimensions.
     *
     * @param cx        The x-coordinate of the center point
     * @param cy        The y-coordinate of the center point
     * @param radius    The circle radius
     *
     * @return a path that represents an circle of the given dimensions.
     */
    Path2 makeCircle(float cx, float cy, float radius) const;

    /**
     * Stores a circle in the provided buffer.
     *
     * The circle will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param path      The path to store the result
     * @param center    The circle center point
     * @param radius    The circle radius
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeCircle(Path2* path, const Vec2 center, float radius) const {
        return makeCircle( path, center.x, center.y, radius );
    }

    /**
     * Stores a circle in the provided buffer.
     *
     * The circle will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param path      The path to store the result
     * @param cx        The x-coordinate of the center point
     * @param cy        The y-coordinate of the center point
     * @param radius    The circle radius
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeCircle(Path2* path, float cx, float cy, float radius) const;

    /**
     * Returns a path that represents an arc of the given dimensions.
     *
     * All arc measurements are in degrees, not radians.
     *
     * @param center    The arc center point (of the defining circle
     * @param radius    The radius from the center point
     * @param start     The starting angle in degrees
     * @param degrees   The number of degrees to generate
     *
     * @return a path that represents an arc of the given dimensions.
     */
    Path2 makeArc(const Vec2 center, float radius, float start, float degrees) const {
        return makeArc(center.x,center.y,radius,start,degrees);
    }

    /**
     * Returns a path that represents an arc of the given dimensions.
     *
     * All arc measurements are in degrees, not radians.
     *
     * @param cx        The x-coordinate of the center point
     * @param cy        The y-coordinate of the center point
     * @param radius    The radius from the center point
     * @param start     The starting angle in degrees
     * @param degrees   The number of degrees to generate
     *
     * @return a path that represents an arc of the given dimensions.
     */
    Path2 makeArc(float cx, float cy, float radius, float start, float degrees) const;

    /**
     * Stores an arc in the provided buffer.
     *
     * All arc measurements are in degrees, not radians.
     *
     * The arc will be appended to the buffer.  You should clear the buffer first
     * if you do not want to preserve the original data.
     *
     * @param path      The path to store the result
     * @param center    The arc center point (of the defining circle
     * @param radius    The radius from the center point
     * @param start     The starting angle in degrees
     * @param degrees   The number of degrees to generate
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeArc(Path2* path, const Vec2 center, float radius, float start, float degrees) const {
        return makeArc(path,center.x,center.y,radius,start,degrees);
    }

    /**
     * Stores an arc in the provided buffer.
     *
     * All arc measurements are in degrees, not radians.
     *
     * The arc will be appended to the buffer.  You should clear the buffer first
     * if you do not want to preserve the original data.
     *
     * @param path      The path to store the result
     * @param cx        The x-coordinate of the center point
     * @param cy        The y-coordinate of the center point
     * @param radius    The radius from the center point
     * @param start     The starting angle in degrees
     * @param degrees   The number of degrees to generate
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeArc(Path2* path, float cx, float cy, float radius, float start, float degrees) const;

    /**
     * Returns a path that represents a rounded rectangle of the given dimensions.
     *
     * The radius should not exceed either half the width or half the height.
     *
     * @param origin    The enclosing rectangle origin
     * @param size      The enclosing rectangle size
     * @param radius    The radius of each corner
     *
     * @return a path that represents a rounded rectangle of the given dimensions.
     */
    Path2 makeRoundedRect(const Vec2 origin, const Size size, float radius) const {
        return makeRoundedRect(origin.x, origin.y, size.width, size.height, radius);
    }
    
    /**
     * Returns a path that represents a rounded rectangle of the given dimensions.
     *
     * The radius should not exceed either half the width or half the height.
     *
     * @param rect      The enclosing rectangle
     * @param radius    The radius of each corner
     *
     * @return a path that represents a rounded rectangle of the given dimensions.
     */
    Path2 makeRoundedRect(const Rect rect, float radius) const {
        return makeRoundedRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height, radius);
    }

    /**
     * Returns a path that represents a rounded rectangle of the given dimensions.
     *
     * The radius should not exceed either half the width or half the height.
     *
     * @param x     The x-coordinate of the bottom left corner of the bounding box
     * @param y     The y-coordinate of the bottom left corner of the bounding box
     * @param w     The rectangle width
     * @param h     The rectangle height
     * @param r     The radius of each corner
     *
     * @return a path that represents a rounded rectangle of the given dimensions.
     */
    Path2 makeRoundedRect(float x, float y, float w, float h, float r) const;

    /**
     * Stores a rounded rectangle in the provided buffer.
     *
     * The radius should not exceed either half the width or half the height.
     *
     * The rounded rectangle will be appended to the buffer.  You should clear the
     * buffer first if you do not want to preserve the original data.
     *
     * @param path      The path to store the result
     * @param origin    The enclosing rectangle origin
     * @param size      The enclosing rectangle size
     * @param radius    The radius of each corner
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeRoundedRect(Path2* path, const Vec2 origin, const Size size, float radius) const {
        return makeRoundedRect(path, origin.x, origin.y, size.width, size.height, radius);
    }

    /**
     * Stores a rounded rectangle in the provided buffer.
     *
     * The radius should not exceed either half the width or half the height.
     *
     * The rounded rectangle will be appended to the buffer.  You should clear the
     * buffer first if you do not want to preserve the original data.
     *
     * @param path      The path to store the result
     * @param rect      The enclosing rectangle
     * @param radius    The radius of each corner
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeRoundedRect(Path2* path, const Rect rect, float radius) const {
        return makeRoundedRect(path, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height, radius);
    }

    /**
     * Stores a rounded rectangle in the provided buffer.
     *
     * The radius should not exceed either half the width or half the height.
     *
     * The rounded rectangle will be appended to the buffer.  You should clear the
     * buffer first if you do not want to preserve the original data.
     *
     * @param path  The path to store the result
     * @param x     The x-coordinate of the bottom left corner of the bounding box
     * @param y     The y-coordinate of the bottom left corner of the bounding box
     * @param w     The rectangle width
     * @param h     The rectangle height
     * @param r     The radius of each corner
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeRoundedRect(Path2* path, float x, float y, float w, float h, float r) const;

    
#pragma mark Capsules
    /**
     * Returns a path that represents a (full) capsule of the given dimensions.
     *
     * A capsule is a pill-like shape that fits inside of given rectangle.  If
     * width < height, the capsule will be oriented vertically with the rounded
     * portions at the top and bottom. Otherwise it will be oriented horizontally.
     *
     * @param origin    The enclosing rectangle origin
     * @param size      The enclosing rectangle size
     *
     * @return a path that represents a (full) capsule of the given dimensions.
     */
    Path2 makeCapsule(const Vec2 origin, const Size size) const {
        return makeCapsule(origin.x, origin.y, size.width, size.height);
    }
    
    /**
     * Returns a path that represents a (full) capsule of the given dimensions.
     *
     * A capsule is a pill-like shape that fits inside of given rectangle.  If
     * width < height, the capsule will be oriented vertically with the rounded
     * portions at the top and bottom. Otherwise it will be oriented horizontally.
     *
     * @param rect      The enclosing rectangle
     *
     * @return a path that represents a (full) capsule of the given dimensions.
     */
    Path2 makeCapsule(const Rect rect) const {
        return makeCapsule(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
    }

    /**
     * Returns a path that represents a (full) capsule of the given dimensions.
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
     * @return a path that represents a (full) capsule of the given dimensions.
     */
    Path2 makeCapsule(float x, float y, float w, float h) const;

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
     * @param path      The path to store the result
     * @param origin    The enclosing rectangle origin
     * @param size      The enclosing rectangle size
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeCapsule(Path2* path, const Vec2 origin, const Size size) const {
        return makeCapsule( path, origin.x, origin.y, size.width, size.height);
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
     * @param path  The path to store the result
     * @param rect  The enclosing rectangle
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeCapsule(Path2* path, const Rect rect) const {
        return makeCapsule( path, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
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
     * @param path  The path to store the result
     * @param x     The x-coordinate of the bottom left corner of the bounding box
     * @param y     The y-coordinate of the bottom left corner of the bounding box
     * @param w     The capsule width
     * @param h     The capsule height
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeCapsule(Path2* path, float x, float y, float w, float h) const;

    /**
     * Returns a path that represents a capsule of the given dimensions.
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
     * @return a path that represents a capsule of the given dimensions.
     */
    Path2 makeCapsule(poly2::Capsule shape, const Vec2 origin, const Size size) const {
        return makeCapsule(shape, origin.x, origin.y, size.width, size.height);
    }
    
    /**
     * Returns a path that represents a capsule of the given dimensions.
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
     * @return a path that represents a capsule of the given dimensions.
     */
    Path2 makeCapsule(poly2::Capsule shape, const Rect rect) const {
        return makeCapsule(shape, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
    }

    /**
     * Returns a path that represents a (full) capsule of the given dimensions.
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
     * @return a path that represents a capsule of the given dimensions.
     */
    Path2 makeCapsule(poly2::Capsule shape, float x, float y, float w, float h) const;

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
     * @param path      The path to store the result
     * @param shape     The capsule shape
     * @param origin    The enclosing rectangle origin
     * @param size      The enclosing rectangle size
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeCapsule(Path2* path, poly2::Capsule shape, const Vec2 origin, const Size size) const {
        return makeCapsule( path, shape, origin.x, origin.y, size.width, size.height);
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
     * @param path      The path to store the result
     * @param shape     The capsule shape
     * @param rect      The enclosing rectangle
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeCapsule(Path2* path, poly2::Capsule shape, const Rect rect) const {
        return makeCapsule( path, shape, rect.origin.x, rect.origin.y,
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
     * @param path      The path to store the result
     * @param shape     The capsule shape
     * @param x         The x-coordinate of the bottom left corner of the bounding box
     * @param y         The y-coordinate of the bottom left corner of the bounding box
     * @param w         The capsule width
     * @param h         The capsule height
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* makeCapsule(Path2* path, poly2::Capsule shape, float x, float y, float w, float h) const;

#pragma mark Traversals
    /**
     * Returns a path set representing a wire frame of an existing polygon.
     *
     * Traversals generate not just one path, but a sequence of paths (which
     * may all be either open or closed). This method provides four types of
     * traversals: `NONE`, `OPEN`, `CLOSED` and `INTERIOR`. The open and closed
     * traversals apply to the boundary of the polygon. If there is more than
     * one boundary, then each boundary is traversed separately.
     *
     * The interior traversal creates a wire frame a polygon triangulation.
     * That means it will generate a separate path for each triangle.
     *
     * @param src   The source polygon to traverse
     * @param type  The traversal type
     *
     * @return a path set representing a wire frame of an existing polygon.
     */
    std::vector<Path2> makeTraversal(const Poly2& src, poly2::Traversal type) const;

    /**
     * Stores a wire frame of an existing polygon in the provided buffer.
     *
     * Traversals generate not just one path, but a sequence of paths (which
     * may all be either open or closed). This method provides four types of
     * traversals: `NONE`, `OPEN`, `CLOSED` and `INTERIOR`. The open and closed
     * traversals apply to the boundary of the polygon. If there is more than
     * one boundary, then each boundary is traversed separately.
     *
     * The interior traversal creates a wire frame a polygon triangulation.
     * That means it will generate a separate path for each triangle.
     *
     * The traversal will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param paths The container to store the result
     * @param src   The source polygon to traverse
     * @param type  The traversal type
     *
     * @return a reference to the buffer for chaining.
     */
    std::vector<Path2>* makeTraversal(std::vector<Path2>* paths, const Poly2& src, poly2::Traversal type) const;
    
private:
    /**
     * Stores a wire frame of an existing polygon in the provided buffer.
     *
     * This method is dedicated to either an `OPEN` or `CLOSED` traversal.
     * It creates a path for each boundary. These paths are either open or
     * closed as specified.
     *
     * The traversal will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param paths     The container to store the result
     * @param src       The source polygon to traverse
     * @param closed    Whether to close the paths
     *
     * @return a reference to the buffer for chaining.
     */
    std::vector<Path2>* makeBoundaryTraversal(std::vector<Path2>* paths, const Poly2& src, bool closed) const;

    /**
     * Stores a wire frame of an existing polygon in the provided buffer.
     *
     * This method is dedicated to an `INTERIOR` traversal.  See the description
     * of {@link #makeTraversal} for more information.  This method simply
     * exists to make the code more readable.
     *
     * The traversal will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param paths The container to store the result
     * @param src   The source polygon to traverse
     *
     * @return a reference to the buffer for chaining.
     */
    std::vector<Path2>* makeInteriorTraversal(std::vector<Path2>* paths, const Poly2& src) const;

};

}

#endif /* __CU_POLY_FACTORY_H__ */
