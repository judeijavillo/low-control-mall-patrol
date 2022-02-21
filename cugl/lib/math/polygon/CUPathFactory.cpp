//
//  CUPathFactory.cpp
//
//  This module provides a convenient way to generate simple paths, like lines
//  or circles. It is lighter weight than the other factory classes because it
//  does not separate the calculation step from the materialization step. That
//  is because all of its calculations are very short and do not need to be
//  factored into a separate thread.
//
//  This class differs from PathFactory in that it only produces paths, and
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
#include <cugl/math/polygon/CUPathFactory.h>
#include <cugl/math/CUPoly2.h>
#include <cugl/util/CUDebug.h>

using namespace cugl;

/** The default tolerance for rounded shapes */
#define DEFAULT_TOLERANCE 0.5


#pragma mark Initialization
/**
 * Creates a factory for generating common paths.
 *
 * This class will use the default tolerance.
 */
PathFactory::PathFactory() :
_tolerance(DEFAULT_TOLERANCE) {
}

/**
 * Creates a factory for generating common paths with the given tolerance.
 *
 * @param tol   The curve tolerance for rounded shapes
 */
PathFactory::PathFactory(float tol) {
    _tolerance = tol;
}

#pragma mark -
#pragma mark Unrounded Shapes
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
Path2 PathFactory::makeLine(float ox, float oy, float dx, float dy) const {
    Path2 result;
    makeLine(&result,ox,oy,dx,dy);
    return result;
}

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
Path2* PathFactory::makeLine(Path2* path, float ox, float oy, float dx, float dy) const {
    CUAssertLog(path != nullptr, "Path buffer is null");
    path->push(Vec2(ox,oy),true);
    path->push(Vec2(dx,dy),true);
    path->closed = true;
    return path;
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
Path2 PathFactory::makeTriangle(float ax, float ay, float bx, float by, float cx, float cy) const {
    Path2 result;
    makeTriangle(&result,ax,ay,bx,by,cx,cy);
    return result;
}

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
Path2* PathFactory::makeTriangle(Path2* path, float ax, float ay, float bx, float by, float cx, float cy) const {
    CUAssertLog(path != nullptr, "Path buffer is null");
    Uint32 offset = (Uint32)path->vertices.size();
    path->reserve( offset+3 );
    path->push(Vec2(ax,ay),true);
    path->push(Vec2(bx,by),true);
    path->push(Vec2(cx,cy),true);
    path->closed = true;
    return path;
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
Path2 PathFactory::makeRect(float x, float y, float w, float h) const {
    Path2 result;
    makeRect(&result,x,y,w,h);
    return result;
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
Path2* PathFactory::makeRect(Path2* path, float x, float y, float w, float h) const {
    CUAssertLog(path != nullptr, "Path buffer is null");
    Uint32 offset = (Uint32)path->vertices.size();
    path->reserve( offset+4 );
    path->push(Vec2(x  ,y  ), true);
    path->push(Vec2(x+w,y  ), true);
    path->push(Vec2(x+w,y+h), true);
    path->push(Vec2(x  ,y+h), true);
    path->closed = true;
    return path;
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
Path2 PathFactory::makeNgon(float cx, float cy, float radius, Uint32 sides) const  {
    Path2 result;
    makeNgon(&result,cx,cy,radius,sides);
    return result;
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
Path2* PathFactory::makeNgon(Path2* path, float cx, float cy, float radius, Uint32 sides) const {
    CUAssertLog(path != nullptr, "Path buffer is null");
    const float coef = 2.0f * (float)M_PI/sides;
    Uint32 offset = (Uint32)path->vertices.size();

    Vec2 vert;
    path->reserve(offset+sides);
    for(unsigned int ii = 0; ii < sides; ii++) {
        float rads = ii*coef;
        vert.x = radius * cosf(rads) + cx;
        vert.y = radius * sinf(rads) + cy;
        path->push(vert, false);
    }

    path->closed = true;
    return path;
}

#pragma mark -
#pragma mark Rounded Shapes
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
Path2 PathFactory::makeEllipse(float cx, float cy, float sx, float sy) const {
    Path2 result;
    makeEllipse(&result,cx,cy,sx,sy);
    return result;
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
Path2* PathFactory::makeEllipse(Path2* path, float cx, float cy, float sx, float sy) const {
    CUAssertLog(path != nullptr, "Path buffer is null");
    Uint32 segments = curveSegs(std::max(sx/2.0f,sy/2.0f), 2.0f * (float)M_PI, _tolerance);
    const float coef = 2.0f * (float)M_PI/segments;
    Uint32 offset = (Uint32)path->vertices.size();

    Vec2 vert;
    path->reserve(offset+segments);
    for(unsigned int ii = 0; ii < segments; ii++) {
        float rads = ii*coef;
        vert.x = 0.5f * sx * cosf(rads) + cx;
        vert.y = 0.5f * sy * sinf(rads) + cy;
        path->push(vert,false);
    }

    path->closed = true;
    return path;
}

/**
 * Returns a path that represents a circle of the given dimensions.
 *
 * @param center    The circle center point
 * @param radius    The circle radius
 *
 * @return a path that represents an circle of the given dimensions.
 */
Path2 PathFactory::makeCircle(float cx, float cy, float radius) const {
    Path2 result;
    makeCircle(&result,cx,cy,radius);
    return result;
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
Path2* PathFactory::makeCircle(Path2* poly, float cx, float cy, float radius) const {
    return makeEllipse(poly, cx, cy, 2*radius, 2*radius);
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
Path2 PathFactory::makeArc(float cx, float cy, float radius, float start, float degrees) const {
    Path2 result;
    makeArc(&result,cx,cy,radius,start,degrees);
    return result;
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
Path2* PathFactory::makeArc(Path2* path, float cx, float cy, float radius, float start, float degrees) const {
    CUAssertLog(path != nullptr, "Path buffer is null");
    Uint32 offset = (Uint32)path->vertices.size();
    Uint32 segments = curveSegs(radius, degrees*(float)M_PI/180.0f, _tolerance);
    segments = (degrees < segments ? (int)degrees : segments);
    float srad = ((float)M_PI/180.0f)*start;
    float arad = ((float)M_PI/180.0f)*degrees;
    float coef = arad/segments;

    path->reserve(offset+segments+1);
    Vec2 vert;
    for(int ii = 0; ii < segments+1; ii++) {
        float rads = srad+ii*coef;
        vert.x = 0.5f * radius  * cosf(rads) + cx;
        vert.y = 0.5f * radius  * sinf(rads) + cy;
        path->push(vert,false);
    }
    
    path->closed = false;
    return path;
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
Path2 PathFactory::makeRoundedRect(float x, float y, float w, float h, float r) const {
    Path2 result;
    makeRoundedRect(&result,x,y,w,h,r);
    return result;
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
Path2* PathFactory::makeRoundedRect(Path2* path, float x, float y, float w, float h, float r) const {
    CUAssertLog(path != nullptr, "Path buffer is null");
    CUAssertLog(r <= w/2.0f, "Radius %.3f exceeds width %.3f", r, w);
    CUAssertLog(r <= h/2.0f, "Radius %.3f exceeds height %.3f", r, h);
    Uint32 segments = curveSegs(r, 2.0f * (float)M_PI, _tolerance);
    Uint32 offset = (Uint32)path->vertices.size();
    const float coef = M_PI/(2.0f*segments);

    float c1x = w >= 0 ? w : 0;
    float c1y = h >= 0 ? h : 0;
    float c2x = w >= 0 ? 0 : w;
    float c2y = h >= 0 ? h : 0;
    float c3x = w >= 0 ? 0 : w;
    float c3y = h >= 0 ? 0 : h;
    float c4x = w >= 0 ? w : 0;
    float c4y = h >= 0 ? 0 : h;
    
    path->reserve(offset+4*segments+4);
    
    // TOP RIGHT
    float cx = x + c1x - r;
    float cy = y + c1y - r;
    Vec2 vert;
    for(int ii = 0; ii <= segments; ii++) {
        vert.x = r * cosf(ii*coef) + cx;
        vert.y = r * sinf(ii*coef) + cy;
        path->push(vert,false);
    }
    
    // TOP LEFT
    cx = x + c2x + r;
    cy = y + c2y - r;
    for(int ii = 0; ii <= segments; ii++) {
        vert.x = cx - r * sinf(ii*coef);
        vert.y = r * cosf(ii*coef) + cy;
        path->push(vert,false);
    }
    
    cx = x + c3x + r;
    cy = y + c3y + r;
    for(int ii = 0; ii <= segments; ii++) {
        vert.x = cx - r * cosf(ii*coef);
        vert.y = cy - r * sinf(ii*coef);
        path->push(vert,false);
    }

    cx = x + c4x - r;
    cy = y + c4y + r;
    for(int ii = 0; ii <= segments; ii++) {
        vert.x = r * sinf(ii*coef) + cx;
        vert.y = cy - r * cosf(ii*coef);
        path->push(vert,false);
    }
    
    path->closed = true;
    return path;
}

#pragma mark -
#pragma mark Capsules
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
Path2 PathFactory::makeCapsule(float x, float y, float w, float h) const {
    Path2 result;
    makeCapsule(&result,poly2::Capsule::FULL,x,y,w,h);
    return result;
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
Path2* PathFactory::makeCapsule(Path2* poly, float x, float y, float w, float h) const {
    return makeCapsule(poly, poly2::Capsule::FULL, x, y, w, h);
}

/**
 * Returns a path that represents a capsule of the given dimensions.
 *
 * A capsule typically is a pill-like shape that fits inside of given rectangle.
 * If width < height, the capsule will be oriented vertically with the rounded
 * portions at the top and bottom. Otherwise it will be oriented horizontally.
 *
 * This method allows for the creation of half-capsules, simply by using the
 * enumeration {@link Path2::Capsule}. The enumeration specifies which side
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
Path2 PathFactory::makeCapsule(poly2::Capsule shape, float x, float y, float w, float h) const {
    Path2 result;
    makeCapsule(&result,shape,x,y,w,h);
    return result;
}

/**
 * Stores a capsule in the provided buffer.
 *
 * A capsule typically is a pill-like shape that fits inside of given rectangle.
 * If width < height, the capsule will be oriented vertically with the rounded
 * portions at the top and bottom. Otherwise it will be oriented horizontally.
 *
 * This method allows for the creation of half-capsules, simply by using the
 * enumeration {@link Path2::Capsule}. The enumeration specifies which side
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
Path2* PathFactory::makeCapsule(Path2* path, poly2::Capsule shape, float x, float y, float w, float h) const {
    CUAssertLog(path != nullptr, "Path buffer is null");
    if (shape == poly2::Capsule::DEGENERATE) {
        return makeEllipse(path, x+w/2, y+h/2, w, h);
    } else if (w == h) {
        return makeCircle(path, x+w/2, y+h/2, w);
    }
    
    Uint32 segments = curveSegs(std::min(w/2,h/2), 2.0f * (float)M_PI, _tolerance);
    Uint32 offset = (Uint32)path->vertices.size();
    const float coef = M_PI/segments;
        
    const float cx = x + w/2.0f;
    const float cy = y + h/2.0f;
    Uint32 vcount = 0;
    if (w <= h) {
        float radius = w / 2.0f;
        float iy = y+radius;
        float ih = h-w;
        Vec2 vert;
        
        // Start at bottom left of interior rectangle
        if (shape == poly2::Capsule::HALF_REVERSE) {
            vert.x = cx - radius;
            vert.y = iy;
            path->push(vert,true);
            vert.x = cx + radius;
            path->push(vert,true);
            vcount += 2;
        } else {
            path->reserve( offset+segments+1 );
            for (Uint32 ii = 0; ii <= segments; ii++) {
                // Try to handle round off gracefully
                float rads = (ii == segments ? M_PI : ii * coef);
                vert.x = cx - radius * cosf( rads );
                vert.y = iy - radius * sinf( rads );
                path->push(vert,false);
            }
            vcount += segments+1;
        }
        
        // Now around the top
        if (shape == poly2::Capsule::HALF) {
            vert.x = cx + radius;
            vert.y = iy + ih;
            path->push(vert,true);
            vert.x = cx - radius;
            path->push(vert,true);
            vcount += 2;
        } else {
            path->reserve( offset+vcount+segments+1 );
            for (Uint32 ii = 0; ii <= segments; ii++) {
                // Try to handle round off gracefully
                float rads = (ii == segments ? M_PI : ii * coef);
                vert.x = cx + radius * cosf( rads );
                vert.y = iy + ih + radius * sinf( rads );
                path->push(vert,false);
            }
            vcount += segments+1;
        }
    } else {
        float radius = h / 2.0f;
        float ix = x+radius;
        float iw = w-h;
        Vec2 vert;
        
        // Start at the top left of the interior rectangle
        if (shape == poly2::Capsule::HALF_REVERSE) {
            vert.x = ix;
            vert.y = cy + radius;
            path->push(vert,true);
            vert.x = cx - radius;
            path->push(vert,true);
            vcount += 2;
        } else {
            path->reserve( offset+segments+1 );
            for (int ii = 0; ii <= segments; ii++) {
                // Try to handle round off gracefully
                float rads = (ii == segments ? M_PI : ii * coef);
                vert.x = ix - radius * cosf( rads );
                vert.y = cy + radius * sinf( rads );
                path->push(vert,false);
            }
            vcount += segments+1;
        }
        
        // Now around the right side
        if (shape == poly2::Capsule::HALF_REVERSE) {
            vert.x = ix + iw;
            vert.y = cy - radius;
            path->push(vert,true);
            vert.x = cx + radius;
            path->push(vert,true);
            vcount += 2;
        } else {
            path->reserve( offset+vcount+segments+1 );
            for (int ii = 0; ii <= segments; ii++) {
                // Try to handle round off gracefully
                float rads = (ii == segments ? M_PI : ii * coef);
                vert.x = ix + iw + radius * cosf( rads );
                vert.y = cy - radius * sinf( rads );
                path->push(vert,false);
            }
            vcount += segments+1;
        }
    }
    path->closed = true;
    return path;
}


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
std::vector<Path2> PathFactory::makeTraversal(const Poly2& src, poly2::Traversal type) const {
    std::vector<Path2> result;
    makeTraversal(&result,src,type);
    return result;
}

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
std::vector<Path2>* PathFactory::makeTraversal(std::vector<Path2>* paths, const Poly2& src, poly2::Traversal type) const {
    CUAssertLog(paths != nullptr, "Path buffer is null");
    switch (type) {
        case poly2::Traversal::NONE:
            // Do nothing
            break;
        case poly2::Traversal::OPEN:
            makeBoundaryTraversal(paths, src, false);
            break;
        case poly2::Traversal::CLOSED:
            makeBoundaryTraversal(paths, src, true);
            break;
        case poly2::Traversal::INTERIOR:
            makeInteriorTraversal(paths, src);
            break;
    }

    return paths;
}
    
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
std::vector<Path2>* PathFactory::makeBoundaryTraversal(std::vector<Path2>* paths, const Poly2& src, bool closed) const {
    std::vector<std::vector<Uint32>> bounds;
    src.boundaries(bounds);

    for(auto it = bounds.begin(); it != bounds.end(); ++it) {
        paths->push_back(Path2());
        Path2* path = &(paths->back());
        path->reserve(it->size());
        for(Uint32 pos = 0; pos < it->size(); pos++) {
            path->push(src.vertices[it->at(pos)],true);
        }
        path->closed = closed;
    }
    
    return paths;
}

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
std::vector<Path2>* PathFactory::makeInteriorTraversal(std::vector<Path2>* paths, const Poly2& src) const {
    const std::vector<Vec2>*   verts = &(src.vertices);
    const std::vector<Uint32>* indxs = &(src.indices);
    for(int ii = 0; ii < indxs->size(); ii+=3) {
        paths->push_back(Path2());
        Path2* path = &(paths->back());
        path->reserve(3);
        path->push(verts->at(indxs->at(ii  )),true);
        path->push(verts->at(indxs->at(ii+1)),true);
        path->push(verts->at(indxs->at(ii+2)),true);
        path->closed = true;
    }

    return paths;
}
