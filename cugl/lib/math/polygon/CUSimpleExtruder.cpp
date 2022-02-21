//
//  CUSimpleExtruder.cpp
//  Cornell University Game Library (CUGL)
//
//  This module is a factory for extruding a path polgyon into a stroke with
//  width. It has support for joints and end caps.
//
//  The code in this factory is inspired from the extrusion code in NanoVG
//  Mikko Mononen (memon@inside.org). Unlike the extrusion code in previous
//  iterations of CUGL, this extrusion guarantees that the triangle mesh is
//  "in order" (instead of back-filling joints after the segments) while
//  still guaranteeing fast, linear performance.
//
//  This code has been heavily profiled and optimized to guarantee sub-millisecond
//  performance for most applications.  However, extruded paths that are the
//  result of drawing should always be passed through a {@link PathSmoother}
//  first for best performance.
//
//  Since math objects are intended to be on the stack, we do not provide
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
//  Version: 6/22/21
//
#include <cugl/math/polygon/CUSimpleExtruder.h>
#include <cugl/math/CUVec2.h>
#include <cugl/math/CUPoly2.h>
#include <cugl/math/CUPath2.h>
#include <cugl/render/CUMesh.h>
#include <cugl/util/CUDebug.h>
#include <cugl/render/CUSpriteVertex.h>
#include <cugl/util/CUTimestamp.h>
#include <iterator>
#include <cmath>

/** Default rounding tolerance */
#define TOLERANCE   0.25f
/** Default mitre limit */
#define MITER_LIMIT 10.0f
/** Epsilon value for small angles/segments */
#define EPSILON     0.000001f
/** Algorithm-specific scaling factor */
#define SCALE_LIMIT 600.0f

/** The mark for a left-side vertex */
#define LEFT_MK    -1.0f
/** The mark for a right-side vertex */
#define RGHT_MK     1.0f
/** The mark for a vertex at the path head */
#define HEAD_MK    -1.0f
/** The mark for a vertex at the path tail */
#define TAIL_MK     1.0f

/** Mark a point as a corner (so it can take a mitre or rounded joint) */
#define FLAG_CORNER 0x01
/** Mark a point as left turning */
#define FLAG_LEFT   0x02
/** Mark a point as requiring a bevel/square joint */
#define FLAG_BEVEL  0x04
/** Mark a point as requiring a special interior join */
#define FLAG_INNER  0x08

namespace cugl {

#pragma mark -
#pragma mark Internal Tracking
/**
 * An annotated point in the path
 *
 * This struct class keeps track of information about the direction to
 * and from this point. It substantially cuts down on repeated calculation
 * in our extrusion algorithm.
 */
class SimpleExtruder::Point {
public:
    /** The point x-coordinate */
    float x;
    /** The point y-coordinate */
    float y;
    /** The (normalized) x-direction to the next point in the path */
    float dx;
    /** The (normalized) y-direction to the next point in the path */
    float dy;
    /** The distance to the next point in the path */
    float len;
    /** The x-coordinate of the vector average (incoming, outgoing) at this point */
    float dmx;
    /** The y-coordinate of the vector average (incoming, outgoing) at this point */
    float dmy;
    /** The flag annations (corner, left-turning) of this point */
    Uint32 flags;
};

}

using namespace cugl;

#pragma mark -
#pragma mark Constructors
/**
 * Creates an extruder with no path data.
 */
SimpleExtruder::SimpleExtruder() :
_joint(poly2::Joint::SQUARE),
_endcap(poly2::EndCap::BUTT),
_tolerance(TOLERANCE),
_mitrelimit(MITER_LIMIT),
_calculated(false),
_closed(false),
_convex(true),
_points(nullptr),
_verts(nullptr),
_lefts(nullptr),
_rghts(nullptr),
_sides(nullptr),
_indxs(nullptr),
_plimit(0),
_psize(0),
_vlimit(0),
_vsize(0),
_lsize(0),
_rsize(0),
_ilimit(0),
_isize(0) {
}

/**
 * Creates an extruder with the given path.
 *
 * The path data is copied. The extruder does not retain any references
 * to the original data.
 *
 * @param points    The path to extrude
 * @param closed    Whether the path is closed
 */
SimpleExtruder::SimpleExtruder(const std::vector<Vec2>& points, bool closed) :
_joint(poly2::Joint::SQUARE),
_endcap(poly2::EndCap::BUTT),
_tolerance(TOLERANCE),
_mitrelimit(MITER_LIMIT),
_calculated(false),
_closed(false),
_convex(true),
_points(nullptr),
_verts(nullptr),
_sides(nullptr),
_indxs(nullptr),
_plimit(0),
_psize(0),
_vlimit(0),
_vsize(0),
_ilimit(0),
_isize(0) {
    set(points,closed);
}

/**
 * Creates an extruder with the given path.
 *
 * The path data is copied. The extruder does not retain any references
 * to the original data.
 *
 * @param path        The path to extrude
 */
SimpleExtruder::SimpleExtruder(const Path2& path) :
_joint(poly2::Joint::SQUARE),
_endcap(poly2::EndCap::BUTT),
_tolerance(TOLERANCE),
_mitrelimit(MITER_LIMIT),
_calculated(false),
_closed(false),
_convex(true),
_points(nullptr),
_verts(nullptr),
_lefts(nullptr),
_rghts(nullptr),
_sides(nullptr),
_indxs(nullptr),
_plimit(0),
_psize(0),
_vlimit(0),
_vsize(0),
_lsize(0),
_rsize(0),
_ilimit(0),
_isize(0) {
    set(path);
}

/**
 * Deletes this extruder, releasing all resources.
 */
SimpleExtruder::~SimpleExtruder() {
    if (_points != nullptr) {
        free(_points);
        _points = nullptr;
    }
    if (_verts != nullptr) {
        free(_verts);
        _verts = nullptr;
    }
    if (_lefts != nullptr) {
        free(_lefts);
        _lefts = nullptr;
    }
    if (_rghts != nullptr) {
        free(_rghts);
        _rghts = nullptr;
    }
    if (_sides != nullptr) {
        free(_sides);
        _sides = nullptr;
    }
    if (_indxs != nullptr) {
        free(_indxs);
        _indxs = nullptr;
    }
}


#pragma mark -
#pragma mark Initialization

/**
 * Sets the path for this extruder.
 *
 * The path data is copied. The extruder does not retain any references
 * to the original data.  All points will be consider to be corner
 * points.
 *
 * This method resets all interal data. You will need to reperform the
 * calculation before accessing data.
 *
 * @param points    The path to extruder
 * @param closed    Whether the path is closed
 */
void SimpleExtruder::set(const std::vector<Vec2>& points, bool closed) {
    clear();
    _closed = closed;
    
    _psize = points.size();
    if (_plimit == 0 || _plimit < _psize) {
        if (_points != nullptr) {
            free(_points);
        }
        _plimit = _psize;
        _points = (Point*)malloc(sizeof(Point)*_plimit);
    }
        
    Point* v = _points;
    for(size_t ii = 0; ii < _psize-1; ii++) {
        const Vec2* p1 = &points[ii];
        const Vec2* p2 = &points[ii+1];

        v->x = p1->x;
        v->y = p1->y;
        v->flags = FLAG_CORNER;
        v->dx = p2->x-p1->x;
        v->dy = p2->y-p1->y;
        v->len = sqrtf(v->dx*v->dx+v->dy*v->dy);
        if (v->len > 1e-6) {
            v->dx /= v->len;
            v->dy /= v->len;
        }
        v++;
    }
    v->x = points.back().x;
    v->y = points.back().y;
    v->flags = FLAG_CORNER;
    v->dx = _points->x-v->x;
    v->dy = _points->y-v->y;
    v->len = sqrtf(v->dx*v->dx+v->dy*v->dy);
    if (v->len > 1e-6) {
        v->dx /= v->len;
        v->dy /= v->len;
    }
}

/**
 * Sets the path for this extruder.
 *
 * The path data is copied. The extruder does not retain any references
 * to the original data. All points will be considered to be corner
 * points.
 *
 * This method resets all interal data. You will need to reperform the
 * calculation before accessing data.
 *
 * @param points    The path to extrude
 * @param size      The number of points
 * @param closed    Whether the path is closed
 */
void SimpleExtruder::set(const Vec2* points, size_t size, bool closed) {
    clear();
    _closed = closed;
    
    _psize = size;
    if (_plimit == 0 || _plimit < _psize) {
        if (_points != nullptr) {
            free(_points);
        }
        _plimit = _psize;
        _points = (Point*)malloc(sizeof(Point)*_plimit);
    }
        
    Point* v = _points;
    for(size_t ii = 0; ii < _psize-1; ii++) {
        const Vec2* p1 = &points[ii];
        const Vec2* p2 = &points[ii+1];

        v->x = p1->x;
        v->y = p1->y;
        v->flags = FLAG_CORNER;
        v->dx = p2->x-p1->x;
        v->dy = p2->y-p1->y;
        v->len = sqrtf(v->dx*v->dx+v->dy*v->dy);
        if (v->len > 1e-6) {
            v->dx /= v->len;
            v->dy /= v->len;
        }
        v++;
    }
    v->x = points[size-1].x;
    v->y = points[size-1].y;
    v->flags = FLAG_CORNER;
    v->dx = _points->x-v->x;
    v->dy = _points->y-v->y;
    v->len = sqrtf(v->dx*v->dx+v->dy*v->dy);
    if (v->len > 1e-6) {
        v->dx /= v->len;
        v->dy /= v->len;
    }
}

/**
 * Sets the path for this extruder.
 *
 * The path data is copied. The extruder does not retain any references
 * to the original data.
 *
 * This method resets all interal data. You will need to reperform the
 * calculation before accessing data.
 *
 * @param path        The path to extrude
 */
void SimpleExtruder::set(const Path2& path) {
    clear();
    _closed = path.isClosed();
    if (path.empty()) {
        return;
    }
    
    _psize = path.size();
    if (_plimit == 0 || _plimit < _psize) {
        if (_points != nullptr) {
            free(_points);
        }
        _plimit = _psize;
        _points = (Point*)malloc(sizeof(Point)*_plimit);
    }
        
    Point* v = _points;
    for(size_t ii = 0; ii < _psize-1; ii++) {
        const Vec2* p1 = &(path.at(ii));
        const Vec2* p2 = &(path.at(ii+1));

        v->x = p1->x;
        v->y = p1->y;
        v->flags = path.isCorner(ii) ? FLAG_CORNER : 0;
        v->dx = p2->x-p1->x;
        v->dy = p2->y-p1->y;
        v->len = sqrtf(v->dx*v->dx+v->dy*v->dy);
        if (v->len > 1e-6) {
            v->dx /= v->len;
            v->dy /= v->len;
        }
        v++;
    }
    const Vec2* p = &(path.at(_psize-1));
    v->x = p->x;
    v->y = p->y;
    v->flags = path.isCorner(_psize-1);
    v->dx = _points->x-v->x;
    v->dy = _points->y-v->y;
    v->len = sqrtf(v->dx*v->dx+v->dy*v->dy);
    if (v->len > 1e-6) {
        v->dx /= v->len;
        v->dy /= v->len;
    }
}


#pragma mark -
#pragma mark Calculation
/**
 * Clears all computed data, but still maintains the settings.
 *
 * This method preserves all initial vertex data, as well as the joint,
 * cap, and precision settings.
 */
void SimpleExtruder::reset() {
    _vsize = 0;
    _lsize = 0;
    _rsize = 0;
    _isize = 0;
    _iback1 = 0;
    _iback2 = 0;
    _calculated = false;
}

/**
 * Clears all internal data, including initial vertex data.
 *
 * When this method is called, you will need to set a new vertices before
 * calling {@link #calculate}.  However, the joint, cap, and precision
 * settings are preserved.
 */
void SimpleExtruder::clear() {
    reset();
    _psize = 0;
    _closed = false;
    _convex = true;
}

/**
 * Performs an asymmetric extrusion of the current path data.
 *
 * An extrusion of a path is a polygon that follows the path but gives it
 * width. Hence it takes a path and turns it into a solid shape. This is
 * more complicated than simply triangulating the original path. The new
 * polygon has more vertices, depending on the choice of joint (shape at
 * the corners) and cap (shape at the end).
 *
 * This version of the method allows you to specify the left and right side
 * widths independently. In particular, this allows us to define an "half
 * extrusion" that starts from the center line.
 *
 * @param lwidth    The width of the left side of the extrusion
 * @param rwidth    The width of the right side of the extrusion
 */
void SimpleExtruder::calculate(float lwidth, float rwidth) {
    if (_calculated) {
        return;
    }
    
    Uint32 ind;    
    float leftmark = lwidth > 0 ? LEFT_MK : 0;
    float rghtmark = rwidth > 0 ? RGHT_MK : 0;
    
    float width = lwidth+rwidth;
    Uint32 ncap = curveSegs(width, M_PI, _tolerance);
    Uint32 nbevel = analyze(width);
    Uint32 cverts = 0;
    if (_joint == poly2::Joint::ROUND) {
        cverts += (_psize + nbevel*(ncap+2) + 1) * 2;  // plus one for loop
    } else {
        cverts += (_psize + nbevel*5 + 1) * 2;         // plus one for loop
    }

    if (!_closed) {
        // space for caps
        if (_endcap == poly2::EndCap::ROUND) {
            cverts += (ncap*2 + 2)*2;
        } else {
            cverts += (3+3)*2;
        }
    }
    
    if (!cverts || !_psize) return;
    prealloc(cverts);

    Point* p0;
    Point* p1;
    Uint32 s, e;

    if (_closed) {
        // Looping
        p0 = _points+_psize-1;
        p1 = _points;
        s = 0;
        e = (Uint32)_psize;
        //printf("Closed\n");
    } else {
        // Add cap
        p0 = _points;
        p1 = _points+1;
        s = 1;
        e = (Uint32)_psize-1;

        float dx = p1->x - p0->x;
        float dy = p1->y - p0->y;
        float mag = sqrtf(dx*dx+dy*dy);
        if (mag > EPSILON) {
            dx /= mag; dy /= mag;
        }
        
        switch(_endcap) {
        case poly2::EndCap::BUTT:
            startButt(p0, dx, dy, lwidth, rwidth);
            break;
        case poly2::EndCap::SQUARE:
            startSquare(p0, dx, dy, lwidth, rwidth, width);
            break;
        case poly2::EndCap::ROUND:
            startRound(p0, dx, dy, lwidth, rwidth, ncap);
            break;
        }
    }
    for(Uint32 jj = s; jj < e; jj++) {
        if ((p1->flags & (FLAG_BEVEL | FLAG_INNER)) != 0) {
            if (_joint == poly2::Joint::ROUND) {
                joinRound(p0, p1, lwidth, rwidth, ncap, _closed && jj == s);
            } else {
                joinBevel(p0, p1, lwidth, rwidth, _closed && jj == s);
            }
        } else if (_closed && jj == s) {
            _iback2 = addPoint(p1->x - (p1->dmx * lwidth), p1->y - (p1->dmy * lwidth), leftmark, 0);
            _iback1 = addPoint(p1->x + (p1->dmx * rwidth), p1->y + (p1->dmy * rwidth), rghtmark, 0);
            addLeft(_iback2);
            addRight(_iback1);
        } else {
            ind = addPoint(p1->x - (p1->dmx * lwidth), p1->y - (p1->dmy * lwidth), leftmark, 0);
            addLeft(ind);
            triLeft(ind);
            ind = addPoint(p1->x + (p1->dmx * rwidth), p1->y + (p1->dmy * rwidth), rghtmark, 0);
            addRight(ind);
            triRight(ind);
        }
        p0 = p1++;
    }
    
    if (_closed) {
        addLeft(0);
        triLeft(0);
        addRight(1);
        triRight(1);
    } else {
        // Add cap
        p1 = _points+e;
        float dx = p1->x - p0->x;
        float dy = p1->y - p0->y;
        float mag = sqrtf(dx*dx+dy*dy);
        if (mag > EPSILON) {
            dx /= mag; dy /= mag;
        }
        
        switch(_endcap) {
        case poly2::EndCap::BUTT:
            endButt(p1, dx, dy, lwidth, rwidth);
            break;
        case poly2::EndCap::SQUARE:
            endSquare(p1, dx, dy, lwidth, rwidth, width);
            break;
        case poly2::EndCap::ROUND:
            endRound(p1, dx, dy, lwidth, rwidth, ncap);
            break;
        }
    }
    _calculated = true;
}

/**
 * Returns the estimated number of vertices in the extrusion
 *
 * This method is important for preallocating the number of vertices and
 * indices for the extrusion. In addition, this method will annotate the
 * path data to ensure that the proper joints are used as each turn.
 *
 * @param width     The stroke width of the extrusion
 *
 * @return the estimated number of vertices in the extrusion
 */
Uint32 SimpleExtruder::analyze(float width) {
    float iwidth = width > 0.0f ? 1.0f/width : 0.0f;
    Uint32 nleft  = 0;
    Uint32 nbevel = 0;
    
    Point* v0 = _points+_psize-1;
    Point* v1 = _points;
    for(Uint32 ii = 0; ii < _psize; ii++) {
        float dlx0 = v0->dy;
        float dly0 = -v0->dx;
        float dlx1 = v1->dy;
        float dly1 = -v1->dx;
        
        // Calculate extrusions
        v1->dmx = (dlx0 + dlx1) * 0.5f;
        v1->dmy = (dly0 + dly1) * 0.5f;

        float dmr2 = v1->dmx*v1->dmx + v1->dmy*v1->dmy;
        if (dmr2 > EPSILON) {
            float scale = 1.0f / dmr2;
            if (scale > SCALE_LIMIT) {
                scale = SCALE_LIMIT;
            }
            v1->dmx *= scale;
            v1->dmy *= scale;
        }
        
        // Clear flags, but keep the corner.
        v1->flags = (v1->flags & FLAG_CORNER) ? FLAG_CORNER : 0;

        // Keep track of left turns.
        float cross = v1->dx * v0->dy - v0->dx * v1->dy;
        if (cross < 0.0) {
            nleft += 1;
            v1->flags |= FLAG_LEFT;
        }

        // Calculate if we should use bevel or miter for inner join.
        float limit = std::max(1.01f, std::min(v0->len, v1->len) * iwidth);
        
        if ((dmr2 * limit*limit) < 1.0f) {
            v1->flags |= FLAG_INNER;
        }

        // Check to see if the corner needs to be beveled.
        if (v1->flags & FLAG_CORNER) {
            if ((dmr2 * _mitrelimit*_mitrelimit) < 1.0 ||
                 _joint == poly2::Joint::SQUARE ||
                 _joint == poly2::Joint::ROUND) {
                v1->flags |= FLAG_BEVEL;
            }
        }
        
        if ((v1->flags & (FLAG_BEVEL | FLAG_INNER)) != 0) {
            nbevel += 1;
        }
        v0 = v1++;
    }

    _convex = (nleft == _psize);
    return nbevel;
}

/**
 * Allocates space for the extrusion vertices and indices
 *
 * This method guarantees that the output buffers will have enough capacity
 * for the algorithm.
 *
 * @param size      The estimated number of vertices in the extrusion
 */
void SimpleExtruder::prealloc(Uint32 size) {
    if (size > _vlimit) {
        if (_verts != nullptr) {
            free(_verts);
        }
        if (_sides != nullptr) {
            free(_sides);
        }
        if (_lefts != nullptr) {
            free(_lefts);
        }
        if (_rghts != nullptr) {
            free(_rghts);
        }
        _verts = (float*)malloc(sizeof(float)*2*size);
        _lefts = (float*)malloc(sizeof(float)*2*size);
        _rghts = (float*)malloc(sizeof(float)*2*size);
        _sides = (float*)malloc(sizeof(float)*2*size);
        _vlimit = size;
    }
    if (3*(size-2) > _ilimit) {
        if (_indxs != nullptr) {
            free(_indxs);
        }
        _ilimit = 3*(size-2);
        _indxs = (Uint32*)malloc(sizeof(Uint32)*_ilimit);
    }
}

/**
 * Computes the bevel vertices at the given joint
 *
 * The pair of vertices is assigned to (x0,y0) and (x1,y1).
 *
 * @param inner     Whether to use an inner bevel
 * @param p0        The point leading to the joint
 * @param p1        The point at the joint
 * @param w         The stroke width of the extrusion
 * @param x0        The x-coordinate of the first vertex
 * @param y0        The y-coordinate of the first vertex
 * @param x1        The x-coordinate of the second vertex
 * @param y1        The y-coordinate of the second vertex
 */
void SimpleExtruder::chooseBevel(bool inner, Point* p0, Point* p1, float w,
                                   float& x0, float& y0, float& x1, float& y1) {
    if (inner) {
        x0 = p1->x + p0->dy * w;
        y0 = p1->y - p0->dx * w;
        x1 = p1->x + p1->dy * w;
        y1 = p1->y - p1->dx * w;
    } else {
        x0 = p1->x + p1->dmx * w;
        y0 = p1->y + p1->dmy * w;
        x1 = p1->x + p1->dmx * w;
        y1 = p1->y + p1->dmy * w;
    }
}

/**
 * Produces a round joint at the point p1
 *
 * @param p0        The point leading to the joint
 * @param p1        The point at the joint
 * @param lw        The width of the left side of the extrusion
 * @param rw        The width of the right side of the extrusion
 * @param ncap      The number of segments in the rounded joint
 * @param start     Whether this is the first joint in an the extrusion
 */
void SimpleExtruder::joinRound(Point* p0, Point* p1, float lw, float rw, Uint32 ncap, bool start) {
    float dlx0 = p0->dy;
    float dly0 = -p0->dx;
    float dlx1 = p1->dy;
    float dly1 = -p1->dx;
    Uint32 ind = 0;

    float leftmark = lw > 0 ? LEFT_MK : 0;
    float rghtmark = rw > 0 ? RGHT_MK : 0;
    
    if (p1->flags & FLAG_LEFT) {
        float lx0,ly0,lx1,ly1;
        chooseBevel(p1->flags & FLAG_INNER, p0, p1, -lw, lx0, ly0, lx1, ly1);
        float a0 = atan2f(dly0, dlx0);
        float a1 = atan2f(dly1, dlx1);
        if (a1 < a0) {
             a1 += M_PI*2;
        }

        
        if (start) {
            _iback2 = addPoint(lx0, ly0, leftmark, 0);
            addLeft(_iback2);
            _iback1 = addPoint(p1->x + dlx0*rw, p1->y + dly0*rw, rghtmark, 0);
            addRight(_iback1);
        } else {
            ind = addPoint(lx0, ly0, leftmark, 0);
            addLeft(ind);
            triLeft(ind);
            ind = addPoint(p1->x + dlx0*rw, p1->y + dly0*rw, rghtmark, 0);
            addRight(ind);
            triRight(ind);
        }
        
        Uint32 n = clampi((int)ceil(((a1 - a0) / M_PI) * ncap), 2, ncap);
        Uint32 center = addPoint(p1->x,p1->y,0,0);
        triLeft(center);
        for(Uint32 ii = 0; ii < n; ii++) {
            float u = ii/((float)(n-1));
            float a = a0 + u*(a1-a0);
            float rx = p1->x + cosf(a) * rw;
            float ry = p1->y + sinf(a) * rw;

            ind = addPoint(rx, ry, rghtmark, 0);
            addRight(ind);
            triRight(ind);
            _iback2 = ind;
            _iback1 = center;
        }

        _iback1 = _iback2;
        _iback2 = center;
        ind = addPoint(lx1, ly1, leftmark, 0);
        addLeft(ind);
        triLeft(ind);
        ind = addPoint(p1->x + dlx1*rw, p1->y + dly1*rw, rghtmark, 0);
        addRight(ind);
        triRight(ind);
    } else {
        float rx0,ry0,rx1,ry1;
        chooseBevel(p1->flags & FLAG_INNER, p0, p1, rw, rx0, ry0, rx1, ry1);
        float a0 = atan2f(-dly0, -dlx0);
        float a1 = atan2f(-dly1, -dlx1);
        if (a1 > a0) {
            a1 -= M_PI*2;
        }

        if (start) {
            _iback1 = addPoint(p1->x - dlx0*lw, p1->y - dly0*lw, leftmark, 0);
            _iback2 = addPoint(rx0, ry0, rghtmark, 0);
        } else {
            ind = addPoint(p1->x - dlx0*lw, p1->y - dly0*lw, leftmark, 0);
            addLeft(ind);
            triLeft(ind);
            ind = addPoint(rx0, ry0, rghtmark, 0);
            addRight(ind);
            triRight(ind);
        }
        
        Uint32 n = clampi((int)ceil(((a0 - a1) / M_PI) * ncap), 2, ncap);
        Uint32 center = addPoint(p1->x,p1->y,0,0);

        float u = 0;
        float a = a0;
        float lx = p1->x + cosf(a) * lw;
        float ly = p1->y + sinf(a) * lw;
        ind = addPoint(lx, ly, leftmark, 0);
        addLeft(ind);
        triLeft(ind);
        triRight(center);

        for(Uint32 ii = 1; ii < n; ii++) {
            u = ii/((float)(n-1));
            a = a0 + u*(a1-a0);
            lx = p1->x + cosf(a) * lw;
            ly = p1->y + sinf(a) * lw;
            
            ind = addPoint(lx, ly, leftmark, 0);
            _iback1 = center;
            addLeft(ind);
            triLeft(ind);
            _iback2 = ind;
        }

        _iback1 = center;
        ind = addPoint(p1->x - dlx1*lw, p1->y - dly1*lw, leftmark, 0);
        addLeft(ind);
        triLeft(ind);
        ind = addPoint(rx1, ry1, rghtmark, 0);
        addRight(ind);
        triRight(ind);
    }
}

/**
 * Produces a bevel/square joint at the point p1
 *
 * @param p0        The point leading to the joint
 * @param p1        The point at the joint
 * @param lw        The width of the left side of the extrusion
 * @param rw        The width of the right side of the extrusion
 * @param start     Whether this is the first joint in an the extrusion
 */
void SimpleExtruder::joinBevel(Point* p0, Point* p1, float lw, float rw, bool start) {
    float dlx0 = p0->dy;
    float dly0 = -p0->dx;
    float dlx1 = p1->dy;
    float dly1 = -p1->dx;

    float leftmark = lw > 0 ? LEFT_MK : 0;
    float rghtmark = rw > 0 ? RGHT_MK : 0;

    Uint32 ind;
    if (p1->flags & FLAG_LEFT) {
        float lx0,ly0,lx1,ly1;
        chooseBevel(p1->flags & FLAG_INNER, p0, p1, -lw, lx0, ly0, lx1, ly1);
        
        if (start) {
            _iback2 = addPoint(lx0, ly0, leftmark, 0);
            _iback1 = addPoint(p1->x + dlx0*rw, p1->y + dly0*rw, rghtmark, 0);
        } else {
            ind = addPoint(lx0, ly0, leftmark, 0);
            addLeft(ind);
            triLeft(ind);
            ind = addPoint(p1->x + dlx0*rw, p1->y + dly0*rw, rghtmark, 0);
            addRight(ind);
            triRight(ind);
        }

        if (p1->flags & FLAG_BEVEL) {
            ind = addPoint(lx1, ly1, leftmark, 0);
            triLeft(ind);
            ind = addPoint(p1->x + dlx1*rw, p1->y + dly1*rw, rghtmark, 0);
            triRight(ind);
       } else {
            float rx0 = p1->x + p1->dmx * rw;
            float ry0 = p1->y + p1->dmy * rw;
            
            ind = addPoint(p1->x, p1->y, 0,0);
            triLeft(ind);
            ind = addPoint(p1->x + dlx0*rw, p1->y + dly0*rw, rghtmark, 0);
            addRight(ind);
            triRight(ind);
            
            ind = addPoint(rx0, ry0, rghtmark, 0);
            addRight(ind);
            triLeft(ind);

            _iback2 = ind;
            _iback1 = addPoint(p1->x, p1->y, 0, 0);
            ind = addPoint(p1->x + dlx1*rw, p1->y + dly1*rw, rghtmark, 0);
            addRight(ind);
            triRight(ind);
        }
        
        ind = addPoint(lx1, ly1, leftmark, 0);
        addLeft(ind);
        triLeft(ind);
        ind = addPoint(p1->x + dlx1*rw, p1->y + dly1*rw, rghtmark, 0);
        addRight(ind);
        triRight(ind);
    } else {
        float rx0,ry0,rx1,ry1;
        chooseBevel(p1->flags & FLAG_INNER, p0, p1, rw, rx0, ry0, rx1, ry1);

        if (start) {
            _iback2 = addPoint(p1->x - dlx0*lw, p1->y - dly0*lw, leftmark, 0);
            _iback1 = addPoint(rx0, ry0, rghtmark, 0);
        } else {
            ind = addPoint(p1->x - dlx0*lw, p1->y - dly0*lw, leftmark, 0);
            addLeft(ind);
            triLeft(ind);
            ind = addPoint(rx0, ry0, rghtmark, 0);
            addRight(ind);
            triRight(ind);
        }
        
        if (p1->flags & FLAG_BEVEL) {
            ind = addPoint(p1->x - dlx1*lw, p1->y - dly1*lw, leftmark, 0);
            addLeft(ind);
            triLeft(ind);
            ind = addPoint(rx1, ry1, rghtmark, 0);
            addRight(ind);
            triRight(ind);
        } else {
            float lx0 = p1->x - p1->dmx * lw;
            float ly0 = p1->y - p1->dmy * lw;
            
            ind = addPoint(p1->x - dlx0*lw, p1->y - dly0*lw, leftmark, 0);
            addLeft(ind);
            triLeft(ind);
            ind = addPoint(p1->x, p1->y, 0, 0);
            triRight(ind);
            
            ind = addPoint(lx0, ly0, leftmark,0);
            addLeft(ind);
            triLeft(ind);
            
            _iback2 = ind;
            _iback1 = addPoint(p1->x - dlx1*lw, p1->y - dly1*lw, leftmark, 0);
            ind = addPoint(p1->x, p1->y, 0, 0);
            addLeft(_iback1);
            triRight(ind);
        }
        
        ind = addPoint(p1->x - dlx1*lw, p1->y - dly1*lw, leftmark, 0);
        addLeft(ind);
        triLeft(ind);
        ind = addPoint(rx1, ry1, rghtmark, 0);
        addRight(ind);
        triRight(ind);
    }
}

/**
 * Produces a butt (degenerate) cap at the head of the extrusion.
 *
 * @param p     The head of the path
 * @param dx    The x-direction from the head to the next path point
 * @param dy    The y-direction from the head to the next path point
 * @param lw    The width of the left side of the extrusion
 * @param rw    The width of the right side of the extrusion
 */
void SimpleExtruder::startButt(Point* p, float dx, float dy, float lw, float rw) {
    float dlx = dy;
    float dly = -dx;
    _iback2 = addPoint(p->x - dlx*lw, p->y - dly*lw, lw > 0 ? LEFT_MK : 0, 0);
    addLeft(_iback2);
    _iback1 = addPoint(p->x + dlx*rw, p->y + dly*rw, rw > 0 ? RGHT_MK : 0, 0);
    addRight(_iback1);
}

/**
 * Produces a butt (degenerate) cap at the tail of the extrusion.
 *
 * @param p     The tail of the path
 * @param dx    The x-direction from the penultimate path point to the tail
 * @param dy    The y-direction from the penultimate path point to the tail
 * @param lw    The width of the left side of the extrusion
 * @param rw    The width of the right side of the extrusion
 */
void SimpleExtruder::endButt(Point* p, float dx, float dy, float lw, float rw) {
    float dlx = dy;
    float dly = -dx;
    Uint32 ind;
    ind = addPoint(p->x - dlx*lw, p->y - dly*lw, lw > 0 ? LEFT_MK : 0, 0);
    addLeft(ind);
    triLeft(ind);
    ind = addPoint(p->x + dlx*rw, p->y + dly*rw, rw > 0 ? RGHT_MK : 0, 0);
    addRight(ind);
    triRight(ind);
}

/**
 * Produces a square cap at the head of the extrusion.
 *
 * @param p     The head of the path
 * @param dx    The x-direction from the head to the next path point
 * @param dy    The y-direction from the head to the next path point
 * @param lw    The width of the left side of the extrusion
 * @param rw    The width of the right side of the extrusion
 * @param d     The length of the cap
 */
void SimpleExtruder::startSquare(Point* p, float dx, float dy, float lw, float rw, float d) {
    float px = p->x - dx*d;
    float py = p->y - dy*d;
    float dlx = dy;
    float dly = -dx;

    float leftmark = lw > 0 ? LEFT_MK : 0;
    float rghtmark = rw > 0 ? RGHT_MK : 0;


    Uint32 ind;
    _iback2 = addPoint(px - dlx*lw, py - dly*lw, leftmark, HEAD_MK);
    addLeft(_iback2);
    _iback1 = addPoint(px + dlx*rw, py + dly*rw, rghtmark, HEAD_MK);
    addRight(_iback1);

    px = p->x;
    py = p->y;
    ind = addPoint(px - dlx*lw, py - dly*lw, leftmark, 0);
    addLeft(ind);
    triLeft(ind);
    ind = addPoint(px + dlx*rw, py + dly*rw, rghtmark, 0);
    addRight(ind);
    triRight(ind);
}

/**
 * Produces a square cap at the tail of the extrusion.
 *
 * @param p     The tail of the path
 * @param dx    The x-direction from the penultimate path point to the tail
 * @param dy    The y-direction from the penultimate path point to the tail
 * @param lw    The width of the left side of the extrusion
 * @param rw    The width of the right side of the extrusion
 * @param d     The length of the cap
 */
void SimpleExtruder::endSquare(Point* p, float dx, float dy, float lw, float rw, float d) {
    float px = p->x;
    float py = p->y;
    float dlx = dy;
    float dly = -dx;

    float leftmark = lw > 0 ? LEFT_MK : 0;
    float rghtmark = rw > 0 ? RGHT_MK : 0;

    Uint32 ind;
    ind = addPoint(px - dlx*lw, py - dly*lw, leftmark, 0);
    triLeft(ind);
    ind = addPoint(px + dlx*rw, py + dly*rw, rghtmark, 0);
    triRight(ind);

    px = p->x + dx*d;
    py = p->y + dy*d;
    ind = addPoint(px - dlx*lw, py - dly*lw, leftmark, TAIL_MK);
    addLeft(ind);
    triLeft(ind);
    ind = addPoint(px + dlx*rw, py + dly*rw, rghtmark, TAIL_MK);
    addRight(ind);
    triRight(ind);
}

/**
 * Produces a rounded cap at the head of the extrusion.
 *
 * @param p        The head of the path
 * @param dx    The x-direction from the head to the next path point
 * @param dy    The y-direction from the head to the next path point
 * @param lw    The width of the left side of the extrusion
 * @param rw    The width of the right side of the extrusion
 * @param ncap    The number of segments in the rounded cap
 */
void SimpleExtruder::startRound(Point* p, float dx, float dy, float lw, float rw, Uint32 ncap) {
    float dlx = dy;
    float dly = -dx;
    float w = (lw+rw)/2.0f;

    float px = p->x + (dlx*rw - dlx*lw)/2.0f;
    float py = p->y + (dly*rw - dly*lw)/2.0f;

    float leftmark = lw > 0 ? LEFT_MK : 0;
    float rghtmark = rw > 0 ? RGHT_MK : 0;

    Uint32 center = addPoint(px, py, 0, 0);
    Uint32 first  = addPoint(px - dlx*w, py - dly*w, leftmark, 0);
    _iback1 = center;
    _iback2 = first;
    addLeft(first);

    Uint32 ind = first;
    for(Uint32 ii = 1; ii < ncap; ii++) {
        float a = (ii*M_PI)/(ncap-1);
        float cx = cosf(a);
        float ax = cx*w;
        float ay = sinf(a)*w;
        
        ind = addPoint(px - dlx*ax - dx*ay, py - dly*ax - dy*ay,
                       leftmark*(1+cx)/2+rghtmark*(1-cx)/2,HEAD_MK*ay/w);
        addRight(ind);
        triRight(ind);
        _iback2 = _iback1;
        _iback1 = center;
    }
    
    _iback1 = ind;
    _iback2 = first;
}

/**
 * Produces a rounded cap at the tail of the extrusion.
 *
 * @param p        The tail of the path
 * @param dx    The x-direction from the penultimate path point to the tail
 * @param dy    The y-direction from the penultimate path point to the tail
 * @param lw    The width of the left side of the extrusion
 * @param rw    The width of the right side of the extrusion
 * @param ncap    The number of segments in the rounded cap
 */
void SimpleExtruder::endRound(Point* p, float dx, float dy, float lw, float rw, Uint32 ncap) {
    float dlx = dy;
    float dly = -dx;
    float w = (lw+rw)/2.0f;

    float px = p->x + (dlx*rw - dlx*lw)/2.0f;
    float py = p->y + (dly*rw - dly*lw)/2.0f;

    float leftmark = lw > 0 ? LEFT_MK : 0;
    float rghtmark = rw > 0 ? RGHT_MK : 0;

    Uint32 first = addPoint(px - dlx*w, py - dly*w, leftmark, 0);
    Uint32 last = addPoint(px + dlx*w, py + dly*w, rghtmark, 0);
    addLeft(first);
    triLeft(first);
    addRight(last);
    triRight(last);
    
    Uint32 center = addPoint(px, py, 0, 0);
    _iback1 = center;
    
    Uint32 ind;
    for(Uint32 ii = 1; ii < ncap-1; ii++) {
        float a = (ii*M_PI)/(ncap-1);
        float cx = cosf(a);
        float ax = cx*w;
        float ay = sinf(a) * w;
        
        ind = addPoint(px - dlx*ax + dx*ay, py - dly*ax + dy*ay,
                       leftmark*(1+cx)/2+rghtmark*(1-cx)/2,TAIL_MK*ay/w);
        addLeft(ind);
        triLeft(ind);
        _iback2 = _iback1;
        _iback1 = center;
    }
    
    triLeft(last);
    _iback1 = center;
}


#pragma mark -
#pragma mark Materialization
/**
 * Returns a polygon representing the path extrusion.
 *
 * The polygon contains the original vertices together with the new
 * indices defining the wireframe path.  The extruder does not maintain
 * references to this polygon and it is safe to modify it.
 *
 * If the calculation is not yet performed, this method will return the
 * empty polygon.
 *
 * @return a polygon representing the path extrusion.
 */
Poly2 SimpleExtruder::getPolygon() const {
    Poly2 poly;
    if (_calculated) {
        Vec2* vts = reinterpret_cast<Vec2*>(_verts);
        poly.vertices.insert(poly.vertices.begin(), vts, vts+_vsize);
        poly.indices.insert(poly.indices.begin(), _indxs, _indxs+_isize);
    }
    return poly;
}

/**
 * Stores the path extrusion in the given buffer.
 *
 * This method will add both the original vertices, and the corresponding
 * indices to the new buffer.  If the buffer is not empty, the indices
 * will be adjusted accordingly. You should clear the buffer first if
 * you do not want to preserve the original data.
 *
 * If the calculation is not yet performed, this method will do nothing.
 *
 * @param buffer    The buffer to store the extruded polygon
 *
 * @return a reference to the buffer for chaining.
 */
Poly2* SimpleExtruder::getPolygon(Poly2* buffer) const {
    CUAssertLog(buffer, "Destination buffer is null");
    if (_calculated) {
        if (buffer->vertices.size() == 0) {
            Vec2* vts = reinterpret_cast<Vec2*>(_verts);
            buffer->vertices.insert(buffer->vertices.begin(), vts, vts+_vsize);
            buffer->indices.insert(buffer->indices.begin(), _indxs, _indxs+_isize);
        } else {
            int offset = (int)buffer->vertices.size();
            buffer->vertices.reserve(offset+_vsize);
            
            Vec2* vts = reinterpret_cast<Vec2*>(_verts);
            std::copy(vts,vts+_vsize,std::back_inserter(buffer->vertices));
            
            buffer->indices.reserve(buffer->indices.size()+_isize);
            for(size_t ii = 0; ii < _isize; ii++) {
                buffer->indices.push_back(offset+_indxs[ii]);
            }
        }
    }
    return buffer;
}

/**
 * Returns a (closed) path representing the extrusion border(s)
 *
 * So long as the calculation is complete, the vector is guaranteed to
 * contain at least one path. Counter-clockwise paths correspond to
 * the exterior boundary of the stroke.  Clockwise paths are potential
 * holes in the extrusion. There is no guarantee on the order of the
 * returned paths.
 *
 * If the calculation is not yet performed, this method will return the
 * empty path.
 *
 * @return a (closed) path representing the extrusion border
 */
std::vector<Path2> SimpleExtruder::getBorder() const {
    std::vector<Path2> result;
    getBorder(result);
    return result;
}

/**
 * Stores a (closed) path representing the extrusion border in the buffer
 *
 * So long as the calculation is complete, the vector is guaranteed to
 * contain at least one path. Counter-clockwise paths correspond to
 * the exterior boundary of the stroke. Clockwise paths are potential
 * holes in the extrusion. There is no guarantee on the order of the
 * returned paths.
 *
 * This method will append append its results to the provided buffer. It
 * will not erase any existing data. You should clear the buffer first if
 * you do not want to preserve the original data.
 *
 * If the calculation is not yet performed, this method will do nothing.
 *
 * @param buffer    The buffer to store the path around the extrusion
 *
 * @return the number of elements added to the buffer
 */
size_t SimpleExtruder::getBorder(std::vector<Path2>& buffer) const {
    size_t size = buffer.size();
    if (_calculated) {
        if (_closed) {
            Path2* path;
            buffer.push_back(Path2());
            path = &(buffer.back());
            Vec2* vts = reinterpret_cast<Vec2*>(_rghts);
            path->vertices.insert(path->vertices.begin(), vts, vts+_rsize-1);
            path->closed = true;
            buffer.push_back(Path2());
            path = &(buffer.back());
            vts = reinterpret_cast<Vec2*>(_lefts);
            std::reverse_copy(vts, vts+_lsize-1, std::back_inserter(path->vertices));
            path->closed = true;
        } else {
            buffer.push_back(Path2());
            Path2* path = &(buffer.back());
            Vec2* vts = reinterpret_cast<Vec2*>(_rghts);
            path->vertices.insert(path->vertices.begin(), vts, vts+_rsize);
            vts = reinterpret_cast<Vec2*>(_lefts);
            std::reverse_copy(vts, vts+_lsize, std::back_inserter(path->vertices));
            path->closed = true;
        }
    }
    return buffer.size()-size;
}

/**
 * Returns a mesh representing the path extrusion.
 *
 * This method creates a triangular mesh with the vertices of the extrusion,
 * coloring each vertex white. However, if fringe is set to true, then each
 * vertex will instead be colored clear (transparent), unless that vertex is
 * on a zero-width side. This effect can be used to produce border "fringes"
 * around a polygon for anti-aliasing.
 *
 * If the calculation is not yet performed, this method will return the
 * empty mesh.
 *
 * @return a mesh representing the path extrusion.
 */
Mesh<SpriteVertex2> SimpleExtruder::getMesh(Color4 color) const {
    Mesh<SpriteVertex2> mesh;
    getMesh(&mesh,color);
    return mesh;
}

/**
 * Stores a mesh representing the path extrusion in the given buffer
 *
 * This method will add both the new vertices, and the corresponding indices
 * to the buffer. If the buffer is not empty, the indices will be adjusted
 * accordingly. You should clear the buffer first if you do not want to
 * preserve the original data.
 *
 * The vertices in this mesh will be colored white by default. However, if
 * fringe is set to true, then each vertex will instead be colored clear
 * (transparent), unless that vertex is on a zero-width side. This effect
 * can be used to produce border "fringes" around a polygon for anti-aliasing.
 *
 * If the calculation is not yet performed, this method will do nothing.
 *
 * @param buffer    The buffer to store the extruded path mesh
 *
 * @return a reference to the buffer for chaining.
 */
Mesh<SpriteVertex2>* SimpleExtruder::getMesh(Mesh<SpriteVertex2>* mesh, Color4 color) const {
    CUAssertLog(mesh, "Destination buffer is null");
    CUAssertLog(mesh->command == GL_TRIANGLES,
               "Buffer geometry is incompatible with this result.");
    if (_calculated) {
        Vec2* vts = reinterpret_cast<Vec2*>(_verts);
        Uint32 offset = (Uint32)mesh->vertices.size();
        mesh->vertices.reserve(_vsize+offset);
        GLuint clr = color.getPacked();
        for(size_t ii = 0; ii < _vsize; ii++) {
            mesh->vertices.push_back(SpriteVertex2());
            SpriteVertex2* vertex = &(mesh->vertices.back());
            vertex->position = vts[ii];
            vertex->color = clr;
        }
        mesh->indices.reserve( _isize+mesh->indices.size());
        for(size_t ii = 0; ii < _isize; ii++) {
            mesh->indices.push_back(_indxs[ii]+offset);
        }
    }
    return mesh;
}

/**
 * Returns a mesh representing the path extrusion.
 *
 * This method creates a triangular mesh with the vertices of the extrusion,
 * coloring each vertex white. However, if fringe is set to true, then each
 * vertex will instead be colored clear (transparent), unless that vertex is
 * on a zero-width side. This effect can be used to produce border "fringes"
 * around a polygon for anti-aliasing.
 *
 * If the calculation is not yet performed, this method will return the
 * empty mesh.
 *
 * @return a mesh representing the path extrusion.
 */
cugl::Mesh<SpriteVertex2> SimpleExtruder::getMesh(Color4 inner, Color4 outer) const {
    Mesh<SpriteVertex2> mesh;
    getMesh(&mesh,inner,outer);
    return mesh;
}

/**
 * Stores a mesh representing the path extrusion in the given buffer
 *
 * This method will add both the new vertices, and the corresponding indices
 * to the buffer. If the buffer is not empty, the indices will be adjusted
 * accordingly. You should clear the buffer first if you do not want to
 * preserve the original data.
 *
 * The vertices in this mesh will be colored white by default. However, if
 * fringe is set to true, then each vertex will instead be colored clear
 * (transparent), unless that vertex is on a zero-width side. This effect
 * can be used to produce border "fringes" around a polygon for anti-aliasing.
 *
 * If the calculation is not yet performed, this method will do nothing.
 *
 * @param buffer    The buffer to store the extruded path mesh
 *
 * @return a reference to the buffer for chaining.
 */
cugl::Mesh<SpriteVertex2>* SimpleExtruder::getMesh(cugl::Mesh<SpriteVertex2>* mesh, Color4 inner, Color4 outer) const {
    CUAssertLog(mesh, "Destination buffer is null");
    CUAssertLog(mesh->command == GL_TRIANGLES,
               "Buffer geometry is incompatible with this result.");
    if (_calculated) {
        Vec2* vts = reinterpret_cast<Vec2*>(_verts);
        Uint32 offset = (Uint32)mesh->vertices.size();
        mesh->vertices.reserve(_vsize+offset);

        GLuint icolor = inner.getPacked();
        GLuint ocolor = outer.getPacked();
        for(size_t ii = 0; ii < _vsize; ii++) {
            mesh->vertices.push_back(SpriteVertex2());
            SpriteVertex2* vertex = &(mesh->vertices.back());
            vertex->position = vts[ii];
            if (!_sides[2*ii]) {
                vertex->color = icolor;
            } else {
                vertex->color = ocolor;
            }
        }
        mesh->indices.reserve( _isize+mesh->indices.size());
        for(size_t ii = 0; ii < _isize; ii++) {
            mesh->indices.push_back(_indxs[ii]+offset);
        }
    }
    return mesh;
}

/**
 * Returns the side information for the vertex at the given index
 *
 * The side information is a two dimensional vector.  The x-coordinate indicates
 * left vs. right side.  A value of -1 is on the left while 1 is on the right.
 * A value of 0 means an interior node sitting on the path itself.
 *
 * On the other hand the y-coordinate indicates cap positioning for an open curve.
 * A value of -1 is on the start cap.  A value of 1 is on the end cap. 0 values
 * lie along the body of the main curve.
 *
 * It is possible to have intermediate cap values for both left-right and start-end
 * in the case of rounded caps.  In this case, the intermediate value tracks the
 * traversal from one side to another.
 *
 * @param index The vertex index
 *
 * @return the side information for the vertex at the given index
 */
Vec2 SimpleExtruder::getSide(Uint32 index) const {
    Vec2* result = (Vec2*)(_sides+2*index);
    return *result;
}
