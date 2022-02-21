//
//  CUSimpleExtruder.h
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
#ifndef __CU_SIMPLE_EXTRUDER_H__
#define __CU_SIMPLE_EXTRUDER_H__

#include <cugl/math/polygon/CUPolyEnums.h>
#include <cugl/math/CUMathBase.h>
#include <cugl/math/CUVec2.h>
#include <vector>

namespace cugl {

// Forward references
class Poly2;
class Path2;
class Color4;
class SpriteVertex2;

// It is generally a bad idea to include a template in our header
template<typename T>
class Mesh;

/**
 * This class is a factory for extruding paths into a solid polygon.
 *
 * An extrusion of a path is a polygon that follows the path but gives it
 * width. Hence it takes a path and turns it into a solid shape. This is
 * more complicated than simply triangulating the original path. The new
 * polygon has more vertices, depending on the choice of joint (shape at
 * the corners) and cap (shape at the end).
 *
 * This class is significantly faster than {@link ComplexExtruder}.  It
 * guarantees sub-millisecond performance for most applications (even
 * thousands of vertices), and so it can be used at framerate. However,
 * this speed comes at significant cost percision. In particular, the
 * {@link Poly2} object created has overlapping triangles, as the algorithm
 * makes no effort to detecting crossing or overlaps. While this is fine
 * for drawing the extrusion with a {@link scene2::PathNode}, it is not accurate
 * for high precision geometry computations. In particular, it will not
 * provide an accurate picture of the boundary. For a geometrically accurate
 * extrusion, you should use {@link ComplexExtruder} instead.
 *
 * One advantage of this tool has over {@link ComplexExtruder} is that it
 * keeps track of the left and right sides of the extrusion at all times
 * (which is only possible since overlaps are preserved). This information
 * can be abstracted and used to properly fade or texture the stroke.
 *
 * As with all factories, the methods are broken up into three phases:
 * initialization, calculation, and materialization. To use the factory,
 * you first set the data (in this case a set of vertices or {@link Path2}
 * object) with the initialization methods. You then call the calculation
 * method. Finally, you use the materialization methods to access the data
 * in several different ways.
 *
 * This division allows us to support multithreaded calculation if the data
 * generation takes too long.  However, note that this factory is not thread
 * safe in that you cannot access data while it is still in mid-calculation.
 *
 * CREDITS: This code heavily inspired by NanoVG by Mikko Mononen
 * (memon@inside.org), and the bulk of the algorithm is taken from that code.
 * However it is has been altered and optimized according to extensive profiling.
 */
class SimpleExtruder {
#pragma mark Values
private:
    /** Internal class for processing the path */
    class Point;

    /** The extrusion joint settings */
    poly2::Joint  _joint;
    /** The extrusion end cap settings */
    poly2::EndCap _endcap;

    /** The rounded joint/cap tolerance */
    float  _tolerance;
    /** The mitre limit (bevel joint if the mitre is too pointy) */
    float  _mitrelimit;
    
    /** Whether the path is closed */
    bool _closed;
    /** Whether the path is convex */
    bool _convex;
       
    /** Whether or not the calculation has been run */
    bool _calculated;

    /** The set of points in the path to extrude */
    Point* _points;
    /** The capacity of the point buffer */
    size_t _plimit;
       /** The number of elements currently in the point buffer */
    size_t _psize;
    
    /** The set of vertices in the active extrusion */
    float* _verts;
    /** The edge markings of each of the extruded vertices */
    float* _sides;
    /** The left side of the extrusion */
    float* _lefts;
    /** The right side of the extrusion */
    float* _rghts;
   /** The capacity of the vertex buffer */
    size_t _vlimit;
   /** The number of elements currently in the vertex buffer */
    size_t _vsize;
    /** The number of elements currently on the left side */
    size_t _lsize;
    /** The number of elements currently on the right side */
    size_t _rsize;

    /** The set of indices indicating the vertex triangulation */
    Uint32* _indxs;
   /** The capacity of the index buffer */
    size_t _ilimit;
   /** The number of elements currently in the index buffer */
    size_t _isize;
    /** The first vertex for the next triangle to produce */
    Uint32 _iback2;
    /** The seconnd vertex for the next triangle to produce */
    Uint32 _iback1;
    
#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates an extruder with no vertex data.
     */
    SimpleExtruder();
    
    /**
     * Creates an extruder with the given path.
     *
     * The path data is copied. The extruder does not retain any references
     * to the original data.
     *
     * @param points    The path to extrude
     * @param closed    Whether the path is closed
     */
    SimpleExtruder(const std::vector<Vec2>& points, bool closed);

    /**
     * Creates an extruder with the given path.
     *
     * The path data is copied. The extruder does not retain any references
     * to the original data.
     *
     * @param path        The path to extrude
     */
    SimpleExtruder(const Path2& path);
    
    /**
     * Deletes this extruder, releasing all resources.
     */
    ~SimpleExtruder();

#pragma mark -
#pragma mark Attributes
    /**
     * Sets the joint value for the extrusion.
     *
     * The joint type determines how the extrusion joins the extruded
     * line segments together.  See {@link poly2::Joint} for the
     * description of the types.
     *
     * @param joint     The extrusion joint type
     */
    void setJoint(poly2::Joint joint) {
        _joint = joint;
    }
    
    /**
     * Returns the joint value for the extrusion.
     *
     * The joint type determines how the extrusion joins the extruded
     * line segments together.  See {@link poly2::Joint} for the
     * description of the types.
     *
     * @return the joint value for the extrusion.
     */
    poly2::Joint getJoint() const {
        return _joint;
    }

    /**
     * Sets the end cap value for the extrusion.
     *
     * The end cap type determines how the extrusion draws the ends of
     * the line segments at the start and end of the path. See
     * {@link poly2::EndCap} for the description of the types.
     *
     * @param endcap    The extrusion end cap type
     */
    void setEndCap(poly2::EndCap endcap) {
        _endcap = endcap;
    }
    
    /**
     * Returns the end cap value for the extrusion.
     *
     * The end cap type determines how the extrusion draws the ends of
     * the line segments at the start and end of the path. See
     * {@link poly2::EndCap} for the description of the types.
     *
     * @return the end cap value for the extrusion.
     */
    poly2::EndCap getEndCap() const {
        return _endcap;
    }
    
    /**
     * Sets the error tolerance of the extrusion.
     *
     * This value is mostly used to determine the number of segments needed
     * for a rounded joint or endcap. The default value is 0.25f.
     *
     * @param tolerance    The error tolerance of the extrusion.
     */
    void setTolerance(float tolerance) {
         _tolerance = tolerance;
    }
    
    /**
     * Returns the error tolerance of the extrusion.
     *
     * This value is mostly used to determine the number of segments needed
     * for a rounded joint or endcap. The default value is 0.25f.
     *
     * @return the error tolerance of the extrusion.
     */
    float getTolerance() const {
         return _tolerance;
    }

    /**
     * Sets the mitre limit of the extrusion.
     *
     * The mitre limit sets how "pointy" a mitre joint is allowed to be before
     * the algorithm switches it back to a bevel/square joint. Small angles can
     * have very large mitre offsets that go way off-screen.
     *
     * To determine whether to switch a miter to a bevel, the algorithm will take
     * the two vectors at this joint, normalize them, and then average them. It
     * will multiple the magnitude of that vector by the mitre limit. If that
     * value is less than 1.0, it will switch to a bevel.  By default this value
     * is 10.0.
     *
     * @param limit    The mitre limit for joint calculations
     */
    void setMitreLimit(float limit) {
         _mitrelimit = limit;
    }
    
    /**
     * Returns the mitre limit of the extrusion.
     *
     * The mitre limit sets how "pointy" a mitre joint is allowed to be before
     * the algorithm switches it back to a bevel/square joint. Small angles can
     * have very large mitre offsets that go way off-screen.
     *
     * To determine whether to switch a miter to a bevel, the algorithm will take
     * the two vectors at this joint, normalize them, and then average them. It
     * will multiple the magnitude of that vector by the mitre limit. If that
     * value is less than 1.0, it will switch to a bevel.  By default this value
     * is 10.0.
     *
     * @return    the mitre limit for joint calculations
     */
    float getMitreLimit() const {
         return _mitrelimit;
    }

#pragma mark -
#pragma mark Initialization
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
     * @param closed    Whether the path is closed
     */
    void set(const std::vector<Vec2>& points, bool closed);

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
    void set(const Vec2* points, size_t size, bool closed);

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
    void set(const Path2& path);
    
#pragma mark -
#pragma mark Calculation
    /**
     * Clears all computed data, but still maintains the settings.
     *
     * This method preserves all initial path data, as well as the joint,
     * cap, and tolerance settings.
     */
    void reset();
    
    /**
     * Clears all internal data, including initial path data.
     *
     * When this method is called, you will need to set a new path before
     * calling {@link #calculate}.  However, the joint, cap, and tolerance
     * settings are preserved.
     */
    void clear();
    
    /**
     * Performs a extrusion of the current path data.
     *
     * An extrusion of a path is a polygon that follows the path but gives it
     * width. Hence it takes a path and turns it into a solid shape. This is
     * more complicated than simply triangulating the original path. The new
     * polygon has more vertices, depending on the choice of joint (shape at
     * the corners) and cap (shape at the end).
     *
     * The stroke width is measured from the left side of the extrusion to the
     * right side of the extrusion. So a stroke of width 20 is actually 10 pixels
     * from the center on each side.
     *
     * @param width        The stroke width of the extrusion
     */
    void calculate(float width) {
        return calculate(width/2.0f,width/2.0f);
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
    void calculate(float lwidth, float rwidth);
    
#pragma mark -
#pragma mark Materialization
    /**
     * Returns a polygon representing the path extrusion.
     *
     * The polygon contains the new set of vertices together with the indices
     * defining the extrusion path.  The extruder does not maintain references
     * to this polygon and it is safe to modify it.
     *
     * If the calculation is not yet performed, this method will return the
     * empty polygon.
     *
     * @return a polygon representing the path extrusion.
     */
    Poly2 getPolygon() const;
    
    /**
     * Stores the path extrusion in the given buffer.
     *
     * This method will add both the new vertices, and the corresponding
     * indices to the buffer. If the buffer is not empty, the indices
     * will be adjusted accordingly. You should clear the buffer first if
     * you do not want to preserve the original data.
     *
     * If the calculation is not yet performed, this method will do nothing.
     *
     * @param buffer    The buffer to store the extruded path
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* getPolygon(Poly2* buffer) const;

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
    std::vector<Path2> getBorder() const;
    
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
    size_t getBorder(std::vector<Path2>& buffer) const;
    
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
    cugl::Mesh<SpriteVertex2> getMesh(Color4 color) const;

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
     * @param mesh      The buffer to store the extruded path mesh
     * @param color     The default mesh color
     *
     * @return a reference to the buffer for chaining.
     */
    cugl::Mesh<SpriteVertex2>* getMesh(cugl::Mesh<SpriteVertex2>* mesh, Color4 color) const;
    
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
    cugl::Mesh<SpriteVertex2> getMesh(Color4 inner, Color4 outer) const;

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
     * @param mesh      The buffer to store the extruded path mesh
     * @param inner     The interior mesh color
     * @param outer     The exterior mesh color
     *
     * @return a reference to the buffer for chaining.
     */
    cugl::Mesh<SpriteVertex2>* getMesh(cugl::Mesh<SpriteVertex2>* mesh, Color4 inner, Color4 outer) const;

    
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
    Vec2 getSide(Uint32 index) const;
    
#pragma mark -
#pragma mark Internal Data Generation
private:
    /**
     * Returns the index of the annotated point after adding it to the vertex buffer
     *
     * This method assumes that _vsize < _vlimit, but it does not check it (for
     * performance reasons).
     *
     * @param x     The vertex x-coordinate
     * @param y     The vertex y-coordinate
     * @param u     The vertex left-right annotation
     * @param y     The vertex head-tail annotation
     *
     * @return the index of the annotated point
     */
    Uint32 inline addPoint(float x, float y, float u, float v) {
        Uint32 index = (Uint32)_vsize;
        _verts[2*_vsize  ] = x;
        _verts[2*_vsize+1] = y;
        _sides[2*_vsize  ] = u;
        _sides[2*_vsize+1] = v;
        _vsize++;
        return index;
    }
    
    /**
     * Returns true if the given vertices are a non-degenerate triangle
     *
     * This method will return false if the vertices are colinear.
     *
     * @param p        The first vertex in the triangle
     * @param q        The second vertex in the triangle
     * @param r        The third vertex in the triangle
     *
     * @return true if the given vertices are a non-degenerate triangle
     */
    static inline bool valid_tri(const cugl::Vec2* p, const cugl::Vec2* q, const cugl::Vec2* r) {
        return (p->x * (q->y - r->y) + q->x * (r->y - p->y) + r->x * (p->y - q->y));
    }
    
    /**
     * Adds a vertex to the left side of the extrusion
     *
     * This method is used to build the extrusion boder
     *
     * @param index     The index of the left vertex
     */
    void inline addLeft(Uint32 index) {
        _lefts[2*_lsize  ] = _verts[2*index  ];
        _lefts[2*_lsize+1] = _verts[2*index+1];
        _lsize++;
    }
    
    /**
     * Creates a triangle on the left side of the extrusion.
     *
     * At any given time, this algorithm has a two buffered indices.  Calling
     * this method creates an counter-clockwise triangle from thes indices
     * (in order) extended to the given vertex.
     *
     * @param index     The index to complete the triangle
     */
    void inline triLeft(Uint32 index) {
        if (valid_tri((Vec2*)(_verts)+_iback1,(Vec2*)(_verts)+_iback2,(Vec2*)(_verts)+index)) {
            _indxs[_isize++] = _iback2;
            _indxs[_isize++] = _iback1;
            _indxs[_isize++] = index;
        }
        _iback2 = _iback1;
        _iback1 = index;
    }

    /**
     * Adds a vertex to the right side of the extrusion
     *
     * This method is used to build the extrusion boder
     *
     * @param index     The index of the right vertex
     */
    void inline addRight(Uint32 index) {
        _rghts[2*_rsize  ] = _verts[2*index  ];
        _rghts[2*_rsize+1] = _verts[2*index+1];
        _rsize++;
    }

    /**
     * Creates a triangle on the right side of the extrusion.
     *
     * At any given time, this algorithm has a two buffered indices.  Calling
     * this method creates an counter-clockwise triangle from these indices
     * (in reverse order) extended to the given vertex.
     *
     * @param index     The index to complete the triangle
     */
    void inline triRight(Uint32 index) {
        if (valid_tri((Vec2*)(_verts)+_iback1,(Vec2*)(_verts)+_iback2,(Vec2*)(_verts)+index)) {
            _indxs[_isize++] = _iback1;
            _indxs[_isize++] = _iback2;
            _indxs[_isize++] = index;
        }
        _iback2 = _iback1;
        _iback1 = index;
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
    Uint32 analyze(float width);
    
    /**
     * Allocates space for the extrusion vertices and indices
     *
     * This method guarantees that the output buffers will have enough capacity
     * for the algorithm.
     *
     * @param size      The estimated number of vertices in the extrusion
     */
    void prealloc(Uint32 size);
    
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
    void chooseBevel(bool inner, Point* p0, Point* p1, float w,
                     float& x0, float& y0, float& x1, float& y1);

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
    void joinRound(Point* p0, Point* p1, float lw, float rw, Uint32 ncap, bool start);

    /**
     * Produces a bevel/square joint at the point p1
     *
     * @param p0        The point leading to the joint
     * @param p1        The point at the joint
     * @param lw        The width of the left side of the extrusion
     * @param rw        The width of the right side of the extrusion
     * @param start     Whether this is the first joint in an the extrusion
     */
    void joinBevel(Point* p0, Point* p1, float lw, float rw, bool start);

    /**
     * Produces a butt (degenerate) cap at the head of the extrusion.
     *
     * @param p     The head of the path
     * @param dx    The x-direction from the head to the next path point
     * @param dy    The y-direction from the head to the next path point
     * @param lw    The width of the left side of the extrusion
     * @param rw    The width of the right side of the extrusion
     */
    void startButt(Point* p, float dx, float dy, float lw, float rw);

    /**
     * Produces a butt (degenerate) cap at the tail of the extrusion.
     *
     * @param p     The tail of the path
     * @param dx    The x-direction from the penultimate path point to the tail
     * @param dy    The y-direction from the penultimate path point to the tail
     * @param lw    The width of the left side of the extrusion
     * @param rw    The width of the right side of the extrusion
     */
    void endButt(Point* p, float dx, float dy, float lw, float rw);
      
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
    void startSquare(Point* p, float dx, float dy, float lw, float rw, float d);

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
    void endSquare(Point* p, float dx, float dy, float lw, float rw, float d);

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
    void startRound(Point* p, float dx, float dy, float lw, float rw, Uint32 ncap);

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
    void endRound(Point* p, float dx, float dy, float lw, float rw, Uint32 ncap);
};

}

#endif /* __CU_SIMPLE_EXTRUDER_H__ */
