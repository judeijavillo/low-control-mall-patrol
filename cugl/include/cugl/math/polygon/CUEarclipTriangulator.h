//
//  CUEarclipTriangulator.h
//  Cornell University Game Library (CUGL)
//
//  This module is a factory for a lightweight earclipping triangulator. While
//  we do have access to the very powerful Pol2Tri, that API has a lot of overhead
//  with it. While earclipping is an O(n^2) algorithm, the lower overhead of this
//  class makes it more performant in smaller applications.
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
//  Version: 7/15/21
//
#ifndef __CU_EARCLIP_TRIANGULATOR_H__
#define __CU_EARCLIP_TRIANGULATOR_H__

#include <cugl/math/CUVec2.h>
#include <vector>

namespace cugl {

// Forward declarations
class Path2;
class Poly2;

/**
 * This class is a factory for producing solid Poly2 objects from a set of vertices.
 *
 * For all but the simplist of shapes, it is important to have a triangulator
 * that can divide up the polygon into triangles for drawing. This class is an
 * implementation of the the ear clipping algorithm to triangulate polygons.
 * This algorithm supports complex polygons, namely those with interior holes
 * (but not self-crossings). All triangles produced are guaranteed to be
 * counter-clockwise.
 *
 * The running time of this algorithm is O(n^2), making it one of the slower
 * triangulation methods available. However, it has very low overhead, making
 * it better than {@link DelaunayTriangulator} in some cases.  In addition, it
 * is guaranteed to make better (e.g. not thin) triangles than
 * {@link MonotoneTriangulator}
 *
 * As with all factories, the methods are broken up into three phases:
 * initialization, calculation, and materialization.  To use the factory, you
 * first set the data (in this case a set of vertices or another Poly2) with the
 * initialization methods.  You then call the calculation method.  Finally,
 * you use the materialization methods to access the data in several different
 * ways.
 *
 * This division allows us to support multithreaded calculation if the data
 * generation takes too long.  However, note that this factory is not thread
 * safe in that you cannot access data while it is still in mid-calculation.
 */
class EarclipTriangulator {
#pragma mark Values
private:
    /** An intermediate class for processing vertices */
    class Vertex;
    
    /** The vertices to process */
    Vertex* _vertices;
    /** The number of vertices to process */
    size_t  _vertsize;
    /** The vertex capacity */
    size_t  _vertlimit;
    
    /** The number of points on the exterior */
    size_t _exterior;
    /** The (raw) set of vertices to use in the calculation */
    std::vector<Vec2> _input;
    /** The offset and size of the hole positions in the input */
    std::vector<size_t> _holes;
    /** The output results of the triangulation */
    std::vector<Uint32> _output;
    
    /** Whether or not the calculation has been run */
    bool _calculated;

#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates a triangulator with no vertex data.
     */
    EarclipTriangulator();

    /**
     * Creates a triangulator with the given vertex data.
     *
     * The vertices are assumed to be the outer hull, and do not
     * include any holes (which may be specified later). The vertex
     * data is copied. The triangulator does not retain any references
     * to the original data.
     *
     * @param points    The vertices to triangulate
     */
    EarclipTriangulator(const std::vector<Vec2>& points);

    /**
     * Creates a triangulator with the given vertex data.
     *
     * The path is assumed to be the outer hull, and does not include any
     * holes (which may be specified later). The vertex data is copied.
     * The triangulator does not retain any references to the original
     * data.
     *
     * @param path      The vertices to triangulate
     */
    EarclipTriangulator(const Path2& path);

    /**
     * Deletes this triangulator, releasing all resources.
     */
    ~EarclipTriangulator();

#pragma mark -
#pragma mark Initialization
    /**
     * Sets the exterior vertex data for this triangulator.
     *
     * The vertices are assumed to be the outer hull, and do not
     * include any holes (which may be specified later). The vertices
     * should define the hull in a counter-clockwise traversal.
     *
     * The vertex data is copied. The triangulator does not retain any
     * references to the original data. Hull points are added first.
     * That is, when the triangulation is computed, the lowest indices
     * all refer to these points, in the order that they were provided.
     *
     * This method resets all interal data. The triangulation is lost,
     * as well as any previously added holes. You will need to re-add
     * any lost data and reperform the calculation.
     *
     * @param points    The vertices to triangulate
     */
    void set(const std::vector<Vec2>& points);
 
    /**
     * Sets the exterior vertex data for this triangulator.
     *
     * The vertices are assumed to be the outer hull, and do not
     * include any holes (which may be specified later). The vertices
     * should define the hull in a counter-clockwise traversal.
     *
     * The vertex data is copied. The triangulator does not retain any
     * references to the original data. Hull points are added first.
     * That is, when the triangulation is computed, the lowest indices
     * all refer to these points, in the order that they were provided.
     *
     * This method resets all interal data. The triangulation is lost,
     * as well as any previously added holes. You will need to re-add
     * any lost data and reperform the calculation.
     *
     * @param points    The vertices to triangulate
     * @param size      The number of vertices
     */
    void set(const Vec2* points, size_t size);
    
    /**
     * Sets the exterior vertex data for this triangulator.
     *
     * The path is assumed to be the outer hull, and does not include
     * any holes (which may be specified later). The path should define
     * the hull in a counter-clockwise traversal.
     *
     * The vertex data is copied. The triangulator does not retain any
     * references to the original data. Hull points are added first.
     * That is, when the triangulation is computed, the lowest indices
     * all refer to these points, in the order that they were provided.
     *
     * This method resets all interal data. The triangulation is lost,
     * as well as any previously added holes. You will need to re-add
     * any lost data and reperform the calculation.
     *
     * @param path    The vertices to triangulate
     */
    void set(const Path2& path);

    /**
     * Adds the given hole to the triangulation.
     *
     * The hole is assumed to be a closed path with no self-crossings.
     * In addition, it is assumed to be inside the polygon outer hull, with
     * vertices ordered in clockwise traversal. If any of these is not true,
     * the results are undefined.
     *
     * The vertex data is copied. The triangulator does not retain any
     * references to the original data. Hole points are added after
     * the hull points, in order. That is, when the triangulation is
     * computed, if the hull is size n, then the hull points are
     * indices 0..n-1, while n is the index of a hole point.
     *
     * Any holes added to the triangulator will be lost if the exterior
     * polygon is changed via the {@link #set} method.
     *
     * @param points    The hole vertices
     */
    void addHole(const std::vector<Vec2>& points);
    
    /**
     * Adds the given hole to the triangulation.
     *
     * The hole is assumed to be a closed path with no self-crossings.
     * In addition, it is assumed to be inside the polygon outer hull, with
     * vertices ordered in clockwise traversal. If any of these is not true,
     * the results are undefined.
     *
     * The vertex data is copied. The triangulator does not retain any
     * references to the original data. Hole points are added after
     * the hull points, in order. That is, when the triangulation is
     * computed, if the hull is size n, then the hull points are
     * indices 0..n-1, while n is the index of a hole point.
     *
     * Any holes added to the triangulator will be lost if the exterior
     * polygon is changed via the {@link #set} method.
     *
     * @param points    The hole vertices
     * @param size      The number of vertices
     */
    void addHole(const Vec2* points, size_t size);

    /**
     * Adds the given hole to the triangulation.
     *
     * The hole path should be a closed path with no self-crossings.
     * In addition, it is assumed to be inside the polygon outer hull,
     * with vertices ordered in clockwise traversal. If any of these is
     * not true, the results are undefined.
     *
     * The vertex data is copied. The triangulator does not retain any
     * references to the original data. Hole points are added after
     * the hull points, in order. That is, when the triangulation is
     * computed, if the hull is size n, then the hull points are
     * indices 0..n-1, while n is the index of a hole point.
     *
     * Any holes added to the triangulator will be lost if the exterior
     * polygon is changed via the {@link #set} method.
     *
     * @param path      The hole path
     */
    void addHole(const Path2& path);

#pragma mark -
#pragma mark Calculation
    /**
     * Clears all internal data, but still maintains the initial vertex data.
     *
     * This method also retains any holes. It only clears the triangulation results.
     */
    void reset();
    
    /**
     * Clears all internal data, including the initial vertex data.
     *
     * When this method is called, you will need to set a new vertices before
     * calling calculate. In addition, any holes will be lost as well.
     */
    void clear();
    
    /**
     * Performs a triangulation of the current vertex data.
     */
    void calculate();
    
#pragma mark -
#pragma mark Materialization
    /**
     * Returns a list of indices representing the triangulation.
     *
     * The indices represent positions in the original vertex list, which
     * included holes as well. Positions are ordered as follows: first the
     * exterior hull, and then all holes in order.
     *
     * The triangulator does not retain a reference to the returned list;
     * it is safe to modify it. If the calculation is not yet performed,
     * this method will return the empty list.
     *
     * @return a list of indices representing the triangulation.
     */
    std::vector<Uint32> getTriangulation() const;

    /**
     * Stores the triangulation indices in the given buffer.
     *
     * The indices represent positions in the original vertex list, which
     * included both holes and Steiner points. Positions are ordered as
     * follows: first the exterior hull, then all holes in order, and
     * finally the Steiner points.
     *
     * The indices will be appended to the provided vector. You should clear
     * the vector first if you do not want to preserve the original data.
     * If the calculation is not yet performed, this method will do nothing.
     *
     * @param buffer    The buffer to store the triangulation indices
     *
     * @return the number of elements added to the buffer
     */
    size_t getTriangulation(std::vector<Uint32>& buffer) const;

    /**
     * Returns a polygon representing the triangulation.
     *
     * This polygon is the proper triangulation, constrained to the interior
     * of the polygon hull. It contains the vertices of the exterior polygon,
     * as well as any holes.
     *
     * The triangulator does not maintain references to this polygon and it
     * is safe to modify it. If the calculation is not yet performed, this
     * method will return the empty polygon.
     *
     * @return a polygon representing the triangulation.
     */
    Poly2 getPolygon() const;
    
    /**
     * Stores the triangulation in the given buffer.
     *
     * The polygon produced is the proper triangulation, constrained to the
     * interior of the polygon hull. It contains the vertices of the exterior
     * polygon, as well as any holes.
     *
     * This method will append the vertices to the given polygon. If the buffer
     * is not empty, the indices will be adjusted accordingly. You should clear
     * the buffer first if you do not want to preserve the original data.
     *
     * If the calculation is not yet performed, this method will do nothing.
     *
     * @param buffer    The buffer to store the triangulated polygon
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* getPolygon(Poly2* buffer) const;

#pragma mark -
#pragma mark Internal Computation
private:
    /**
     * Allocates the doubly-linked list(s) to manage the vertices
     */
    void allocateVertices();

    /**
     * Slices out holes, merging vertices into one doubly-linked list
     */
    void removeHoles();

    /**
     * Computes the triangle indices for the active vertices.
     */
    void computeTriangles();

};

}

#endif /* __CU_EARCLIP_TRIANGULATOR_H__ */

