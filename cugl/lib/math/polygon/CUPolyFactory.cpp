//
//  CUPolyFactory.cpp
//
//  This module provides a convenient way to generate simple polygons, like lines
//  or circles. It is lighter weight than the other factory classes because it
//  does not separate the calculation step from the materialization step. That
//  is because all of its calculations are very short and do not need to be
//  factored into a separate thread.
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
#include <cugl/math/polygon/CUPolyFactory.h>
#include <cugl/math/CUPath2.h>
#include <cugl/util/CUDebug.h>

using namespace cugl;

/** The default tolerance for rounded shapes */
#define DEFAULT_TOLERANCE 0.5

#pragma mark Initialization
/**
 * Creates a PolyFactory for generating solid polygons.
 *
 * This factory will use the default tolerance.
 */
PolyFactory::PolyFactory() :
_tolerance(DEFAULT_TOLERANCE) {
}

/**
 * Creates a factory for generating solid polygons with the given tolerance.
 *
 * @param tol   The curve tolerance for rounded shapes
 */
PolyFactory::PolyFactory(float tol) {
    _tolerance = tol;
}

#pragma mark -
#pragma mark Unrounded Shapes
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
Poly2 PolyFactory::makeTriangle(float ax, float ay, float bx, float by, float cx, float cy) const {
    Poly2 result;
    makeTriangle(&result,ax,ay,bx,by,cx,cy);
    return result;
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
Poly2* PolyFactory::makeTriangle(Poly2* poly, float ax, float ay, float bx, float by, float cx, float cy) const {
    CUAssertLog(poly != nullptr, "Polygon buffer is null");
    Uint32 offset = (Uint32)poly->vertices.size();
    poly->vertices.reserve( offset+3 );
    poly->vertices.push_back(Vec2(ax,ay));
    poly->vertices.push_back(Vec2(bx,by));
    poly->vertices.push_back(Vec2(cx,cy));

    poly->indices.reserve(poly->indices.size()+3);
    if (Path2::orientation(Vec2(ax,ay),Vec2(bx,by),Vec2(cx,cy)) >= 0) {
        poly->indices.push_back(offset+2);
        poly->indices.push_back(offset+1);
        poly->indices.push_back(offset);
    } else {
        poly->indices.push_back(offset);
        poly->indices.push_back(offset+1);
        poly->indices.push_back(offset+2);
    }
    return poly;
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
Poly2 PolyFactory::makeRect(float x, float y, float w, float h) const {
    Poly2 result;
    makeRect(&result,x,y,w,h);
    return result;
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
Poly2* PolyFactory::makeRect(Poly2* poly, float x, float y, float w, float h) const {
    CUAssertLog(poly != nullptr, "Polygon buffer is null");
    Uint32 offset = (Uint32)poly->vertices.size();
    poly->vertices.reserve( offset+4 );
    poly->vertices.push_back(Vec2(x  ,y  ));
    poly->vertices.push_back(Vec2(x+w,y  ));
    poly->vertices.push_back(Vec2(x+w,y+h));
    poly->vertices.push_back(Vec2(x  ,y+h));

    poly->indices.reserve(poly->indices.size()+6);
    poly->indices.push_back(offset);
    poly->indices.push_back(offset+1);
    poly->indices.push_back(offset+2);
    poly->indices.push_back(offset+2);
    poly->indices.push_back(offset+3);
    poly->indices.push_back(offset);

    return poly;
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
Poly2 PolyFactory::makeNgon(float cx, float cy, float radius, Uint32 sides) const {
    Poly2 result;
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
 * @param poly      The polygon to store the result
 * @param cx        The x-coordinate of the center point
 * @param cy        The y-coordinate of the center point
 * @param radius    The polygon radius
 * @param sides     The number of sides
 *
 * @return a reference to the buffer for chaining.
 */
Poly2* PolyFactory::makeNgon(Poly2* poly, float cx, float cy, float radius, Uint32 sides) const {
    CUAssertLog(poly != nullptr, "Polygon buffer is null");
    const float coef = 2.0f * (float)M_PI/sides;
    Uint32 offset = (Uint32)poly->vertices.size();

    Vec2 vert;
    poly->vertices.reserve(offset+sides+1);
    for(unsigned int ii = 0; ii < sides; ii++) {
        float rads = ii*coef;
        vert.x = radius * cosf(rads) + cx;
        vert.y = radius * sinf(rads) + cy;
        poly->vertices.push_back(vert);
    }
    
    poly->vertices.push_back(Vec2(cx,cy));
    poly->indices.reserve(poly->indices.size()+3*sides);
    for(Uint32 ii = 0; ii < sides-1; ii++) {
        poly->indices.push_back(ii+offset);
        poly->indices.push_back(ii+offset+1);
        poly->indices.push_back(sides+offset);
    }
    poly->indices.push_back(sides+offset-1);
    poly->indices.push_back(offset);
    poly->indices.push_back(sides+offset);

    return poly;
}

#pragma mark -
#pragma mark Rounded Shapes
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
Poly2 PolyFactory::makeEllipse(float cx, float cy, float sx, float sy) const {
    Poly2 result;
    makeEllipse(&result,cx,cy,sx,sy);
    return result;
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
Poly2* PolyFactory::makeEllipse(Poly2* poly, float cx, float cy, float sx, float sy) const {
    CUAssertLog(poly != nullptr, "Polygon buffer is null");
    Uint32 segments = curveSegs(std::max(sx/2.0f,sy/2.0f), 2.0f * (float)M_PI, _tolerance);
    const float coef = 2.0f * (float)M_PI/segments;
    Uint32 offset = (Uint32)poly->vertices.size();

    Vec2 vert;
    poly->vertices.reserve(offset+segments);
    for(unsigned int ii = 0; ii < segments; ii++) {
        float rads = ii*coef;
        vert.x = 0.5f * sx * cosf(rads) + cx;
        vert.y = 0.5f * sy * sinf(rads) + cy;
        poly->vertices.push_back(vert);
    }
    
    poly->vertices.push_back(Vec2(cx,cy));
    poly->indices.reserve(poly->indices.size()+3*segments);
    for(Uint32 ii = 0; ii < segments-1; ii++) {
        poly->indices.push_back(ii+offset);
        poly->indices.push_back(ii+offset+1);
        poly->indices.push_back(segments+offset);
    }
    poly->indices.push_back(segments+offset-1);
    poly->indices.push_back(offset);
    poly->indices.push_back(segments+offset);

    return poly;
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
Poly2 PolyFactory::makeCircle(float cx, float cy, float radius) const {
    Poly2 result;
    makeCircle(&result,cx,cy,radius);
    return result;
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
Poly2* PolyFactory::makeCircle(Poly2* poly, float cx, float cy, float radius) const {
    return makeEllipse(poly, cx, cy, 2*radius, 2*radius);
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
Poly2 PolyFactory::makeArc(float cx, float cy, float radius, float start, float degrees) const {
    Poly2 result;
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
 * @param poly      The polygon to store the result
 * @param cx        The x-coordinate of the center point
 * @param cy        The y-coordinate of the center point
 * @param radius    The radius from the center point
 * @param start     The starting angle in degrees
 * @param degrees   The number of degrees to generate
 *
 * @return a reference to the buffer for chaining.
 */
Poly2* PolyFactory::makeArc(Poly2* poly, float cx, float cy, float radius, float start, float degrees) const {
    CUAssertLog(poly != nullptr, "Polygon buffer is null");
    Uint32 offset = (Uint32)poly->vertices.size();
    Uint32 segments = curveSegs(radius, degrees*(float)M_PI/180.0f, _tolerance);
    segments = (degrees < segments ? (int)degrees : segments);
    float srad = ((float)M_PI/180.0f)*start;
    float arad = ((float)M_PI/180.0f)*degrees;
    float coef = arad/segments;

    poly->vertices.reserve(offset+segments+1);
    Vec2 vert;
    for(int ii = 0; ii < segments+1; ii++) {
        float rads = srad+ii*coef;
        vert.x = 0.5f * radius  * cosf(rads) + cx;
        vert.y = 0.5f * radius  * sinf(rads) + cy;
        poly->vertices.push_back(vert);
    }
    
    poly->vertices.push_back(Vec2(cx,cy));
    poly->indices.reserve(poly->indices.size()+3*segments);
    for(Uint32 ii = 0; ii < segments+1; ii++) {
        poly->indices.push_back(ii+offset);
        poly->indices.push_back(ii+offset+1);
        poly->indices.push_back(segments+offset+1);
    }

    return poly;
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
Poly2 PolyFactory::makeRoundedRect(float x, float y, float w, float h, float r) const {
    Poly2 result;
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
 * @param poly  The polygon to store the result
 * @param x     The x-coordinate of the bottom left corner of the bounding box
 * @param y     The y-coordinate of the bottom left corner of the bounding box
 * @param w     The rectangle width
 * @param h     The rectangle height
 * @param r     The radius of each corner
 *
 * @return a reference to the buffer for chaining.
 */
Poly2* PolyFactory::makeRoundedRect(Poly2* poly, float x, float y, float w, float h, float r) const {
    CUAssertLog(poly != nullptr, "Polygon buffer is null");
    CUAssertLog(r <= w/2.0f, "Radius %.3f exceeds width %.3f", r, w);
    CUAssertLog(r <= h/2.0f, "Radius %.3f exceeds height %.3f", r, h);
    Uint32 offset = (Uint32)poly->vertices.size();
    Uint32 segments = curveSegs(r, 2.0f * (float)M_PI, _tolerance);
    const float coef = M_PI/(2.0f*segments);

    float c1x = w >= 0 ? w : 0;
    float c1y = h >= 0 ? h : 0;
    float c2x = w >= 0 ? 0 : w;
    float c2y = h >= 0 ? h : 0;
    float c3x = w >= 0 ? 0 : w;
    float c3y = h >= 0 ? 0 : h;
    float c4x = w >= 0 ? w : 0;
    float c4y = h >= 0 ? 0 : h;
    
    poly->vertices.reserve(offset+4*segments+4);
    
    // TOP RIGHT
    float cx = x + c1x - r;
    float cy = y + c1y - r;
    Vec2 vert;
    for(int ii = 0; ii <= segments; ii++) {
        vert.x = r * cosf(ii*coef) + cx;
        vert.y = r * sinf(ii*coef) + cy;
        poly->vertices.push_back(vert);
    }
    
    // TOP LEFT
    cx = x + c2x + r;
    cy = y + c2y - r;
    for(int ii = 0; ii <= segments; ii++) {
        vert.x = cx - r * sinf(ii*coef);
        vert.y = r * cosf(ii*coef) + cy;
        poly->vertices.push_back(vert);
    }
    
    cx = x + c3x + r;
    cy = y + c3y + r;
    for(int ii = 0; ii <= segments; ii++) {
        vert.x = cx - r * cosf(ii*coef);
        vert.y = cy - r * sinf(ii*coef);
        poly->vertices.push_back(vert);
    }

    cx = x + c4x - r;
    cy = y + c4y + r;
    for(int ii = 0; ii <= segments; ii++) {
        vert.x = r * sinf(ii*coef) + cx;
        vert.y = cy - r * cosf(ii*coef);
        poly->vertices.push_back(vert);
    }
    
    cx = x + w/2.0f;
    cy = y + h/2.0f;


    poly->vertices.push_back(Vec2(cx,cy));
    Uint32 capacity = 4*segments+4;
    poly->indices.reserve( poly->indices.size()+3*capacity );
    for(int ii = 0; ii < capacity-1; ii++) {
        poly->indices.push_back(offset+ii);
        poly->indices.push_back(offset+ii+1);
        poly->indices.push_back(offset+capacity);
    }
    poly->indices.push_back(offset+capacity-1);
    poly->indices.push_back(offset);
    poly->indices.push_back(offset+capacity);
    
    return poly;
}

#pragma mark -
#pragma mark Capsules
/**
 * Returns a solid polygon that represents a capsule of the given dimensions.
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
 * @return a solid polygon that represents a capsule of the given dimensions.
 */
Poly2 PolyFactory::makeCapsule(float x, float y, float w, float h) const {
    Poly2 result;
    makeCapsule(&result,poly2::Capsule::FULL,x,y,w,h);
    return result;
}

/**
 * Stores a capsule in the provided buffer.
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
Poly2* PolyFactory::makeCapsule(Poly2* poly, float x, float y, float w, float h) const {
    return makeCapsule(poly, poly2::Capsule::FULL, x, y, w, h);
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
Poly2 PolyFactory::makeCapsule(poly2::Capsule shape, float x, float y, float w, float h) const {
    Poly2 result;
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
Poly2* PolyFactory::makeCapsule(Poly2* poly, poly2::Capsule shape, float x, float y, float w, float h) const {
    CUAssertLog(poly != nullptr, "Polygon buffer is null");
    if (shape == poly2::Capsule::DEGENERATE) {
        return makeEllipse(poly, x+w/2, y+h/2, w, h);
    } else if (w == h) {
        return makeCircle(poly, x+w/2, y+h/2, w);
    }
    
    Uint32 segments = curveSegs(std::min(w/2,h/2), (float)M_PI, _tolerance);
    Uint32 offset = (Uint32)poly->vertices.size();
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
            poly->vertices.push_back(vert);
            vert.x = cx + radius;
            poly->vertices.push_back(vert);
            vcount += 2;
        } else {
            poly->vertices.reserve( offset+segments+1 );
            for (Uint32 ii = 0; ii <= segments; ii++) {
                // Try to handle round off gracefully
                float rads = (ii == segments ? M_PI : ii * coef);
                vert.x = cx - radius * cosf( rads );
                vert.y = iy - radius * sinf( rads );
                poly->vertices.push_back(vert);
            }
            vcount += segments+1;
        }
        
        // Now around the top
        if (shape == poly2::Capsule::HALF) {
            vert.x = cx + radius;
            vert.y = iy + ih;
            poly->vertices.push_back(vert);
            vert.x = cx - radius;
            poly->vertices.push_back(vert);
            vcount += 2;
        } else {
            poly->vertices.reserve( offset+vcount+segments+1 );
            for (Uint32 ii = 0; ii <= segments; ii++) {
                // Try to handle round off gracefully
                float rads = (ii == segments ? M_PI : ii * coef);
                vert.x = cx + radius * cosf( rads );
                vert.y = iy + ih + radius * sinf( rads );
                poly->vertices.push_back(vert);
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
            poly->vertices.push_back(vert);
            vert.x = cx - radius;
            poly->vertices.push_back(vert);
            vcount += 2;
        } else {
            poly->vertices.reserve( offset+segments+1 );
            for (int ii = 0; ii <= segments; ii++) {
                // Try to handle round off gracefully
                float rads = (ii == segments ? M_PI : ii * coef);
                vert.x = ix - radius * cosf( rads );
                vert.y = cy + radius * sinf( rads );
                poly->vertices.push_back(vert);
            }
            vcount += segments+1;
        }
        
        // Now around the right side
        if (shape == poly2::Capsule::HALF_REVERSE) {
            vert.x = ix + iw;
            vert.y = cy - radius;
            poly->vertices.push_back(vert);
            vert.x = cx + radius;
            poly->vertices.push_back(vert);
            vcount += 2;
        } else {
            poly->vertices.reserve( offset+vcount+segments+1 );
            for (int ii = 0; ii <= segments; ii++) {
                // Try to handle round off gracefully
                float rads = (ii == segments ? M_PI : ii * coef);
                vert.x = ix + iw + radius * cosf( rads );
                vert.y = cy - radius * sinf( rads );
                poly->vertices.push_back(vert);
            }
            vcount += segments+1;
        }
    }
    
    poly->vertices.push_back(Vec2(cx,cy));
    poly->indices.reserve( poly->indices.size()+3*vcount );
    for(int ii = 0; ii < vcount-1; ii++) {
        poly->indices.push_back(ii);
        poly->indices.push_back(ii+1);
        poly->indices.push_back(vcount);
    }
    poly->indices.push_back(vcount-1);
    poly->indices.push_back(0);
    poly->indices.push_back(vcount);

    return poly;
}
