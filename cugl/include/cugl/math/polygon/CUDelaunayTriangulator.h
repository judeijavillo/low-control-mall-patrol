//
//  CUDelaunayTriangulator.h
//  Cornell University Game Library (CUGL)
//
//  This module is a factory for a Constrained Delaunay triangulator. This
//  is distinct from the normal Delaunay triangulator in that it can handle
//  complex polygons (polygons with holes, but not self-crossings). It is
//  also much more heavy-weight than the other triangulators because it is
//  built on top of the popular poly2tri library
//
//      https://github.com/jhasse/poly2tri
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
//  Version: 1/21/21
//
#ifndef __CU_DELAUNAY_TRIANGULATOR_H__
#define __CU_DELAUNAY_TRIANGULATOR_H__

#include <poly2tri/poly2tri.h>
#include <cugl/math/CUVec2.h>
#include <unordered_map>
#include <deque>
#include <vector>

namespace cugl {

// Forward declarations
class Path2;
class Poly2;

/**
 * This class is a factory for producing solid Poly2 objects from a set of vertices.
 *
 * For all but the simplist of shapes, it is important to have a triangulator
 * that can divide up the polygon into triangles for drawing. This triangulator
 * uses the poly2tri library to perform a Constrained Delaunay triangulation.
 * This library supports complex polygons, namely those with interior holes
 * (but not self-crossings). All triangles produced are guaranteed to be
 * counter-clockwise.
 *
 * Note that poly2tri uses a sweep-line algorithm described in
 * 
 *   https://www.tandfonline.com/doi/abs/10.1080/13658810701492241
 *
 * While most sweep-line algorithms have O(n^2) time, this algorithm experimentally
 * outperforms an O(n log n) algorithm. However, no worst-case analysis on this
 * algorithm is available.
 *
 * Because the Voronoi diagram is the dual of the Delaunay triangulation, this
 * factory can be used to extract this diagram. The Voronoi diagram can be
 * extracted as either a single polygon (for each point) or a collection of
 * polygons.  Each polygon is a solid Poly2 representing a region.

 * As with all factories, the methods are broken up into three phases:
 * initialization, calculation, and materialization.  To use the factory, you
 * first set the data (in this case a set of vertices or another {@link Poly2})
 * with the initialization methods. You then call the calculation method.
 * Finally, you use the materialization methods to access the data. Unlike
 * the other triangulators, you cannot simply get this triangulation as a
 * set of indices. That is because the triangulation may have added additional
 * vertices.
 *
 * This division allows us to support multithreaded calculation if the data
 * generation takes too long.  However, note that this factory is not thread
 * safe in that you cannot access data while it is still in mid-calculation.
 */
class DelaunayTriangulator {
private:
    /** The poly2tri triangulator */
    p2t::CDT* _triangulator;

    /** The points to use for the outer hull */
    std::vector<p2t::Point> _hull;
    /** The set of Steiner points to use in the calculation */
    std::vector<p2t::Point> _stein;
    /** The set of holes to use in the calculation */
    std::vector<std::vector<p2t::Point>> _holes;

    /** The set of vertices to use in the calculation (hull, holes, and Steiner) */
    std::vector<p2t::Point*> _vertices;
    /** A map to reverse look-up indices from poly2tri triangles */
    std::unordered_map<p2t::Point*, Uint32> _idxmap;
    
    /** The output results of the triangulation */
    std::vector<Uint32> _indices;
    /** The output results of the extends triangulation */
    std::vector<Uint32> _extended;
    /** Whether or not the triangulation has been computed */
    bool _calculated;
    
    /** Whether or not the voronoi diagram has been computed */
    bool _dualated;
    /** The Vornoi diagram as a collection of solid Polys */
    std::unordered_map<Uint32, Poly2> _voronoi;

public:
    /**
     * Creates a triangulator with no vertex data.
     */
    DelaunayTriangulator();
    
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
    DelaunayTriangulator(const std::vector<Vec2>& points);
    
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
    DelaunayTriangulator(const Path2& path);
    
    /**
     * Deletes this triangulator, releasing all resources.
     */
    ~DelaunayTriangulator();
    
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
     * as well as any previously added holes or Steiner points. You
     * will need to re-add any lost data and reperform the calucation.
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
     * as well as any previously added holes or Steiner points. You
     * will need to re-add any lost data and reperform the calucation.
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
     * as well as any previously added holes or Steiner points. You
     * will need to re-add any lost data and reperform the calucation.
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

    /**
     * Adds the given Steiner point to the triangulation.
     *
     * A Steiner point may be included in the triangulation results,
     * but it does not have to be. Any Steiner points added to the
     * triangulator will be lost if the exterior polygon is changed
     * via the {@link #set} method.
     *
     * The vertex data is copied. The triangulator does not retain any
     * references to the original data. Steiner points are added last.
     * That is, when the triangulation is computed, the highest indices
     * all refer to these points, in the order that they were provided.
     *
     * @param point     The Steiner point
     */
    void addSteiner(Vec2 point);
    
#pragma mark Calculation
    /**
     * Clears all internal data, but still maintains the initial vertex data.
     *
     * This method also retains any holes or Steiner points. It only clears
     * the triangulation results.
     */
    void reset();
    
    /**
     * Clears all internal data, including the initial vertex data.
     *
     * When this method is called, you will need to set a new vertices before
     * calling calculate. In addition, any holes or Steiner points will be
     * lost as well.
     */
    void clear();
    
    /**
     * Performs a triangulation of the current vertex data.
     *
     * This only calculates the triangulation.  It does not compute the
     * Voronoi dual.
     */
    void calculate();
    
    /**
     * Calculates the Voronoi diagram.
     *
     * This will force a triangulation if one has not been computed
     * already. In cases where triangles are missing to fully define the
     * Voronoi diagram (such as on the boundary of the diagram), the
     * missing triangles are interpolated.
     */
    void calculateDual();
    
#pragma mark Materialization
    /**
     * Returns a list of indices representing the triangulation.
     *
     * The indices represent positions in the original vertex list, which
     * included both holes and Steiner points. Positions are ordered as
     * follows: first the exterior hull, then all holes in order, and
     * finally the Steiner points.
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
     * as well as any holes. It may or many not include any Steiner points that
     * were specified.
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
     * polygon, as well as any holes. It may or many not include any Steiner
     * points that were specified.
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
   
    /**
     * Returns a list of indices representing the extended triangulation map.
     *
     * The indices represent positions in the original vertex list, which
     * included both holes and Steiner points. Positions are ordered as
     * follows: first the exterior hull, then all holes in order, and
     * finally the Steiner points. As these indices represent the extended
     * triangulation, they may include triangles outside of the exterior hull.
     *
     * The triangulator does not retain a reference to the returned list; it
     * is safe to modify it. If the calculation is not yet performed, this method
     * will return the empty list.
     *
     * @return a list of indices representing the triangulation.
     */
    std::vector<Uint32> getMap() const;
    
    /**
     * Stores the extended triangulation map indices in the given buffer.
     *
     * The indices represent positions in the original vertex list, which
     * included both holes and Steiner points. Positions are ordered as
     * follows: first the exterior hull, then all holes in order, and
     * finally the Steiner points. As these indices represent the extended
     * triangulation, they may include triangles outside of the exterior hull.
     *
     * The indices will be appended to the provided vector. You should clear
     * the vector first if you do not want to preserve the original data.
     * If the calculation is not yet performed, this method will do nothing.
     *
     * @param buffer    The buffer to store the extended triangulation map
     *
     * @return the number of elements added to the buffer
     */
    size_t getMap(std::vector<Uint32>& buffer) const;
    
    /**
     * Returns a polygon representing the extended triangulation map.
     *
     * This polygon is the extended triangulation, which may include triangles
     * outside of the polygon hull. It may or many not include any Steiner points
     * that were specified.
     *
     * The triangulator does not maintain references to this polygon and it
     * is safe to modify it. If the calculation is not yet performed, this
     * method will return the empty polygon.
     *
     * @return a polygon representing the triangulation.
     */
    Poly2 getMapPolygon() const;
    
    /**
     * Stores the extended triangulation map in the given buffer.
     *
     * This polygon is the extended triangulation, which may include triangles
     * outside of the polygon hull. It may or many not include any Steiner points
     * that were specified.
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
    Poly2* getMapPolygon(Poly2* buffer) const;
    
    
#pragma mark Voronoization
    /**
     * Returns the Voronoi diagram as a list of polygons
     *
     * Each polygon represents a single Voronoi cell. A Voronoi cell is a
     * polygon whose vertices are the boundary of the cell. Each Voronoi
     * cell corresponds to a vertex in the original triangulation.
     *
     * The cells are returned in the same order as the vertices. For each
     * cell, the Poly2 object is triangulated as a fan with the associated
     * vertex point at its center.
     *
     * If the Voronoi diagram is not calculated, this method will do nothing.
     *
     * @return the Voronoi diagram as a list of polygons
     */
    std::vector<Poly2> getVoronoi() const;

    /**
     * Returns the Voronoi cell for the given index
     *
     * A Voronoi cell is a polygon whose vertices are the boundary of the cell.
     * The index corresponds to the vertex in the original triangulation.
     * The returns Poly2 object is triangulated as a fan with the associated
     * vertex point at its center.
     *
     * If the Voronoi diagram is not calculated, this method will return an
     * empty polygon
     *
     * @param index The index of the vertex generating the cell
     *
     * @return he Voronoi cell for the given index
     */
    Poly2 getVoronoiCell(size_t index) const;
    
    /**
     * Stores the Voronoi cell in the given buffer.
     *
     * A Voronoi cell is a polygon whose vertices are the boundary of the cell.
     * The index corresponds to the vertex in the original triangulation.
     * The returns Poly2 object is triangulated as a fan with the associated
     * vertex point at its center.
     *
     * If the Voronoi diagram is not calculated, this method will do nothing.
     *
     * @param index     The index of the vertex generating the cell
     * @param buffer    The buffer to store the Voronoi cell
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* getVoronoiCell(size_t index, Poly2* buffer) const;

#pragma mark Internal Helpers
    
private:
    /**
     * Returns the boundary points for the given Voronoi region.
     *
     * This returns the boundary vertices for the Voronoi region containing
     * the point p, which can then later be triangulated to produce the Poly2
     * object. In cases where triangles are missing to fully define the
     * Voronoi diagram (such as on the boundary of the diagram), the missing
     * triangles are interpolated.
     *
     * This method uses the fact that the poly2tri triangulation keeps track
     * of all neighbors for each point.  Hence, we need a triangle from the
     * triangulation to start.
     *
     * @param p     The point defining the Voronoi region
     * @param tri   A poly2tri triangle containing p as a vertex.
     *
     * @return the boundary points for the given Voronoi region.
     */
    const std::deque<Vec2> calculateCell(p2t::Point* p, p2t::Triangle* tri);

    /**
     * Returns the circumcenter for the given triangle tri
     *
     * @param tri   The triangle to compute
     *
     * @return the centriod for the given triangle tri
     */
    Vec2 getCircumcenter(p2t::Triangle* tri) const;
    
    /**
     * Returns the circumcenter for the given 3 points
     *
     * The circumcenter is of the triangle implicitly defined by the
     * three points.
     *
     * @param p1   The first triangle vertex
     * @param p2   The second triangle vertex
     * @param p3   The third triangle vertex
     *
     * @return the centriod for the given triangle tri
     */
    Vec2 getCircumcenter(p2t::Point* p1, p2t::Point* p2, p2t::Point* p3) const;
};
  
}


#endif /* __CU_DELAUNAY_TRIANGULATOR_H__ */
