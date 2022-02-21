//
//  CUDelaunyTriangulator.cpp
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
#include <cugl/math/polygon/CUDelaunayTriangulator.h>
#include <cugl/math/CUPoly2.h>
#include <cugl/math/CUPath2.h>
#include <cugl/util/CUDebug.h>
#include <deque>

using namespace cugl;

/**
 * Returns the point of intersection of a ray with the bounding box.
 *
 * This function extends the ray with direction dir and anchor start, until
 * it intersects with one of the edges of the bounding box.  If there is
 * no intersection, it will return the origin (0,0).
 *
 * @param start the start anchor of the ray
 * @param dir   the direction vector of the ray
 * @param rect  the bounding box to intersect
 *
 * @return the point of intersection of a ray with the bounding box
 */
static Vec2 get_intersection(const Vec2& start, const Vec2& dir, const Rect& box) {
    float s = -1;
    float t = -1;
    
    Vec2::doesLineIntersect(start,start+dir,box.origin,box.origin+Vec2(box.size.width,0),&s,&t);
    if (0 <= t && t <= 1 && s >= 0) {
        return box.origin+Vec2(box.size.width*t,0);
    }
    
    Vec2::doesLineIntersect(start,start+dir,box.origin,box.origin+Vec2(0,box.size.height),&s,&t);
    if (0 <= t && t <= 1 && s >= 0) {
        return box.origin+Vec2(0,box.size.height*t);
    }
    
    Vec2::doesLineIntersect(start,start+dir,box.origin+Vec2(box.size.width,0),box.origin+box.size,&s,&t);
    if (0 <= t && t <= 1 && s >= 0) {
        return box.origin+Vec2(box.size.width,box.size.height*t);
    }
    
    Vec2::doesLineIntersect(start,start+dir,box.origin+Vec2(0,box.size.height),box.origin+box.size,&s,&t);
    if (0 <= t && t <= 1 && s >= 0) {
        return box.origin+Vec2(box.size.width*t,box.size.height);
    }
    
    return Vec2(0,0);
}

/**
 * Returns the corner vertex between two boundary points.
 *
 * This function assumes the boundary points start and end are on two different
 * but adjacent edges of the bounding box.
 *
 * @param start the first boundary point
 * @param end   the second boundary point
 * @param rect  the bounding box to check
 *
 * @return he corner vertex between two boundary points.
 */
static Vec2 get_interior(const Vec2& start, const Vec2& end, const Rect& box) {
    Vec2 result;
    if (start.x == box.origin.x || end.x == box.origin.x) {
        result.x = box.origin.x;
    } else {
        result.x = box.origin.x+box.size.width;
    }
    if (start.y == box.origin.y || end.y == box.origin.y) {
        result.y = box.origin.y;
    } else {
        result.y = box.origin.y+box.size.height;
    }
    return result;
}

#pragma mark -
#pragma mark Constructors
/**
 * Creates a triangulator with no vertex data.
 */
DelaunayTriangulator::DelaunayTriangulator() :
_triangulator(nullptr),
_calculated(false) {
}

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
DelaunayTriangulator::DelaunayTriangulator(const std::vector<Vec2>& points) :
_triangulator(nullptr),
_calculated(false) {
    set(points);
}


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
DelaunayTriangulator::DelaunayTriangulator(const Path2& path) :
_triangulator(nullptr),
_calculated(false) {
    set(path);
}

/**
 * Deletes this triangulator, releasing all resources.
 */
DelaunayTriangulator::~DelaunayTriangulator() {
    if (_triangulator != nullptr) {
        delete _triangulator;
        _triangulator = nullptr;
    }
}


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
 * as well as any previously added holes or Steiner points. You
 * will need to re-add any lost data and reperform the calucation.
 *
 * @param points    The vertices to triangulate
 */
void DelaunayTriangulator::set(const std::vector<Vec2>& points) {
    CUAssertLog(Path2::orientation(points) == -1, "Path orientiation is not CCW");
    reset();
    _hull.reserve(points.size());
    for(auto it = points.begin(); it != points.end(); ++it) {
        _hull.push_back(p2t::Point(it->x, it->y));
    }
}

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
void DelaunayTriangulator::set(const Vec2* points, size_t size) {
    CUAssertLog(Path2::orientation(points,size) == -1, "Path orientiation is not CCW");
    reset();
    _hull.reserve(size);
    for(size_t ii = 0; ii < size; ii++) {
        _hull.push_back(p2t::Point(points[ii].x, points[ii].y));
    }
}


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
void DelaunayTriangulator::set(const Path2& path) {
    CUAssertLog(path.orientation() == -1, "Path orientiation is not CCW");
    reset();
    _hull.reserve(path.size());
    for(auto it = path.vertices.begin(); it != path.vertices.end(); ++it) {
        _hull.push_back(p2t::Point(it->x, it->y));
    }
}

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
void DelaunayTriangulator::addHole(const std::vector<Vec2>& points) {
    CUAssertLog(Path2::orientation(points) == 1, "Hole orientiation is not CW");
    std::vector<p2t::Point> hole;
    hole.reserve(points.size());
    for(auto it = points.begin(); it != points.end(); ++it) {
        hole.push_back(p2t::Point(it->x, it->y));
    }
    _holes.push_back(hole);
}

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
void DelaunayTriangulator::addHole(const Vec2* points, size_t size) {
    CUAssertLog(Path2::orientation(points, size) == 1, "Hole orientiation is not CW");
    std::vector<p2t::Point> hole;
    hole.reserve(size);
    for(size_t ii = 0; ii < size; ii++) {
        hole.push_back(p2t::Point(points[ii].x, points[ii].y));
    }
    _holes.push_back(hole);
}

/**
 * Adds the given hole to the triangulation.
 *
 * The hole is assumed to be a closed path with no self-crossings.
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
void DelaunayTriangulator::addHole(const Path2& path) {
    CUAssertLog(path.orientation() == 1, "Hole orientiation is not CW");
    std::vector<p2t::Point> hole;
    hole.reserve(path.size());
    for(auto it = path.vertices.begin(); it != path.vertices.end(); ++it) {
        hole.push_back(p2t::Point(it->x, it->y));
    }
    _holes.push_back(hole);
}

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
void DelaunayTriangulator::addSteiner(Vec2 point) {
    _stein.push_back(p2t::Point(point.x, point.y));
}

#pragma mark -
#pragma mark Calculation
/**
 * Clears all internal data, but still maintains the initial vertex data.
 *
 * This method also retains any holes or Steiner points. It only clears
 * the triangulation results.
 */
void DelaunayTriangulator::reset() {
    if (_triangulator != nullptr) {
        delete _triangulator;
        _triangulator = nullptr;
    }
    _vertices.clear();
    _idxmap.clear();
    _indices.clear();
    _voronoi.clear();
    _calculated = false;
    _dualated = false;
}

/**
 * Clears all internal data, including the initial vertex data.
 *
 * When this method is called, you will need to set a new vertices before
 * calling calculate. In addition, any holes or Steiner points will be
 * lost as well.
 */
void DelaunayTriangulator::clear() {
    reset();
    _vertices.clear();
    _hull.clear();
    _stein.clear();
    _holes.clear();
}

/**
 * Performs a triangulation of the current vertex data.
 *
 * This only calculates the triangulation.  It does not compute the
 * Voronoi dual.
 */
void DelaunayTriangulator::calculate() {
    reset();

    // Set up the triangulator
    _vertices.reserve(_hull.size());
    for(size_t ii = 0; ii < _hull.size(); ii++) {
        p2t::Point* p = &(_hull[ii]);
        _idxmap.emplace(p,_vertices.size());
        _vertices.push_back(p);
    }
    _triangulator = new p2t::CDT(_vertices);
    
    std::vector<p2t::Point*> hole;
    for(auto it = _holes.begin(); it != _holes.end(); ++it) {
        hole.clear();
        size_t next = _vertices.size()+it->size();
        hole.reserve(it->size());
        _vertices.reserve(next);
        for(size_t ii = 0; ii < it->size(); ii++) {
            p2t::Point* p = &(it->at(ii));
            hole.push_back(p);
            _idxmap.emplace(p,_vertices.size());
            _vertices.push_back(p);
        }
        _triangulator->AddHole(hole);
    }
    
    size_t next = _vertices.size()+_stein.size();
    _vertices.reserve(next);
    for(size_t ii = 0; ii < _stein.size(); ii++) {
        p2t::Point* p = &(_stein[ii]);
        _idxmap.emplace(p,_vertices.size());
        _vertices.push_back(p);
        _triangulator->AddPoint(p);
    }
    
    _triangulator->Triangulate();
    
    // Extract the information
    std::vector<p2t::Triangle*> tris = _triangulator->GetTriangles();
    _indices.reserve(3*tris.size());
    for(auto it = tris.begin(); it != tris.end(); ++it) {
        // poly2tri points are automatically CCW
        for(int ii = 0; ii < 3; ii++) {
            p2t::Point* p = (*it)->GetPoint(ii);
            auto search = _idxmap.find(p);
            CUAssertLog(search != _idxmap.end(), "Triangulation introduced an unknown vertex");
            _indices.push_back(search->second);
        }
    }

    std::list<p2t::Triangle*> map = _triangulator->GetMap();
    _extended.reserve(3*map.size());
    for(auto it = tris.begin(); it != tris.end(); ++it) {
        // poly2tri points are automatically CCW
        for(int ii = 0; ii < 3; ii++) {
            p2t::Point* p = (*it)->GetPoint(ii);
            auto search = _idxmap.find(p);
            CUAssertLog(search != _idxmap.end(), "Extended triangulation introduced an unknown vertex");
            _extended.push_back(search->second);
        }
    }

    _calculated = true;
}

/**
 * Calculates the Voronoi diagram.
 *
 * This will force a triangulation if one has not been computed
 * already. In cases where triangles are missing to fully define the
 * Voronoi diagram (such as on the boundary of the diagram), the
 * missing triangles are interpolated.
 */
void DelaunayTriangulator::calculateDual() {
    if (!_calculated) {
        calculate();
    }
    _voronoi.clear();
    
    std::list<p2t::Triangle*> map = _triangulator->GetMap();
    for(auto it = map.begin(); it != map.end(); ++it) {
        for(int ii = 0; ii < 3; ii++) {
            p2t::Point* p = (*it)->GetPoint(ii);
            auto index  = _idxmap.find(p);
            if (index != _idxmap.end()) {
                auto search = _voronoi.find(index->second);
                if (search == _voronoi.end()) {
                    std::deque<Vec2> deque = calculateCell(p, *it);
                    _voronoi.emplace(index->second,Poly2());
                    Poly2* poly = &_voronoi[index->second];
                    
                    // Add vertices
                    for(auto it = deque.begin(); it != deque.end(); ++it) {
                        poly->vertices.push_back(*it);
                    }
                    size_t size = poly->vertices.size();
                    poly->vertices.push_back(Vec2(p->x,p->y));
                    
                    // Add indices
                    if (size > 2) {
                        for(int ii = 0; ii < size-1; ii++) {
                            // This is CCW
                            poly->indices.push_back(ii);
                            poly->indices.push_back(ii+1);
                            poly->indices.push_back((Uint32)size);
                        }
                    }
                    poly->indices.push_back((Uint32)size-1);
                    poly->indices.push_back(0);
                    poly->indices.push_back((Uint32)size);
                }
            }
        }
    }
    _dualated = true;
}

#pragma mark -
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
std::vector<Uint32> DelaunayTriangulator::getTriangulation() const {
    return _indices;
}

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
size_t DelaunayTriangulator::getTriangulation(std::vector<Uint32>& buffer) const {
    if (_calculated) {
        buffer.insert(buffer.end(), _indices.begin(), _indices.end());
        return _indices.size();
    }
    return 0;
}

/**
 * Returns a polygon representing the triangulation.
 *
 * This polygon is the proper triangulation, constrained to the interior
 * of the polygon hull. t contains the vertices of the exterior polygon,
 * as well as any holes. It may or many not include any Steiner points that
 * were specified.
 *
 * The triangulator does not maintain references to this polygon and it
 * is safe to modify it. If the calculation is not yet performed, this
 * method will return the empty polygon.
 *
 * @return a polygon representing the triangulation.
 */
Poly2 DelaunayTriangulator::getPolygon() const {
    Poly2 poly;
    if (_calculated) {
        for(auto it = _vertices.begin(); it != _vertices.end(); ++it) {
            poly.vertices.push_back(Vec2((*it)->x,(*it)->y));
        }
        poly.indices  = _indices;
    }
    return poly;
}

/**
 * Stores the triangulation in the given buffer.
 *
 * The polygon produced is the proper triangulation, constrained to the
 * interior of the polygon hull. t contains the vertices of the exterior
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
Poly2* DelaunayTriangulator::getPolygon(Poly2* buffer) const {
    CUAssertLog(buffer, "Destination buffer is null");
    if (_calculated) {
        Uint32 offset = (int)buffer->vertices.size();
        buffer->vertices.reserve(offset+_vertices.size());
        for(auto it = _vertices.begin(); it != _vertices.end(); ++it) {
            buffer->vertices.push_back(Vec2((*it)->x,(*it)->y));
        }
        buffer->indices.reserve(buffer->indices.size()+_indices.size());
        for(auto it = _indices.begin(); it != _indices.end(); ++it) {
            buffer->indices.push_back(offset+*it);
        }
    }
    return buffer;
}

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
std::vector<Uint32> DelaunayTriangulator::getMap() const {
    return _extended;
}

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
size_t DelaunayTriangulator::getMap(std::vector<Uint32>& buffer) const {
    if (_calculated) {
        buffer.reserve(buffer.size()+_extended.size());
        for(auto it = _extended.begin(); it != _extended.end(); ++it) {
            buffer.push_back(*it);
        }
        return _indices.size();
    }
    return 0;
}


/**
 * Returns a polygon representing the triangulation map.
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
Poly2 DelaunayTriangulator::getMapPolygon() const {
    Poly2 poly;
    if (_calculated) {
        for(auto it = _vertices.begin(); it != _vertices.end(); ++it) {
            poly.vertices.push_back(Vec2((*it)->x,(*it)->y));
        }
        poly.indices  = _extended;
    }
    return poly;
}

/**
 * Stores the triangulation in the given buffer.
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
Poly2* DelaunayTriangulator::getMapPolygon(Poly2* buffer) const {
    CUAssertLog(buffer, "Destination buffer is null");
    if (_calculated) {
        Uint32 offset = (int)buffer->vertices.size();
        buffer->vertices.reserve(offset+_vertices.size());
        for(auto it = _vertices.begin(); it != _vertices.end(); ++it) {
            buffer->vertices.push_back(Vec2((*it)->x,(*it)->y));
        }
        buffer->indices.reserve(buffer->indices.size()+_extended.size());
        for(auto it = _extended.begin(); it != _extended.end(); ++it) {
            buffer->indices.push_back(offset+*it);
        }
    }
    return buffer;
}

#pragma mark -
#pragma mark Voronoi
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
std::vector<Poly2> DelaunayTriangulator::getVoronoi() const {
    std::vector<Poly2> result;
    if (_dualated) {
        result.reserve(_vertices.size());
        for(Uint32 ii = 0; ii < _vertices.size(); ii++) {
            auto search = _voronoi.find(ii);
            if (search != _voronoi.end()) {
                result.push_back(search->second);
            } else {
                result.push_back(Poly2());
            }
        }
    }
    return result;
}

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
Poly2 DelaunayTriangulator::getVoronoiCell(size_t index) const {
    auto search = _voronoi.find((Uint32)index);
    if (search == _voronoi.end()) {
        return Poly2();
    }
    return search->second;
}

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
Poly2* DelaunayTriangulator::getVoronoiCell(size_t index, Poly2* buffer) const {
    CUAssertLog(buffer, "Destination buffer is null");
    if (_dualated) {
        auto search = _voronoi.find((Uint32)index);
        if (search != _voronoi.end()) {
            const Poly2* p = &(search->second);
            Uint32 offset = (Uint32)buffer->vertices.size();
            buffer->vertices.reserve(offset+p->vertices.size());
            for(auto it = p->vertices.begin(); it != p->vertices.end(); ++it) {
                buffer->vertices.push_back(*it);
            }
            Uint32 icapacity = (Uint32)buffer->indices.size();
            buffer->indices.reserve(icapacity+p->indices.size());
            for(auto it = p->indices.begin(); it != p->indices.end(); ++it) {
                buffer->indices.push_back(*it+offset);
            }
        }
    }
    return buffer;
}

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
const std::deque<Vec2> DelaunayTriangulator::calculateCell(p2t::Point* p, p2t::Triangle* tri) {
    std::deque<Vec2> result;

    Vec2 origin(p->x,p->y);
    result.push_back(getCircumcenter(tri));
    
    // Work CCW.
    p2t::Triangle* prev = tri;
    p2t::Triangle* next = tri->NeighborCCW(*p);
    bool loop = next == tri;
    while (next != nullptr && !loop) {
        result.push_back(getCircumcenter(next));
        prev = next;
        next = next->NeighborCCW(*p);
        loop = next == tri;
    }

    if (!loop) {
        // We did not make a circle
        p2t::Point* ccw = prev->PointCCW(*p);
        Vec2 lanchor(ccw->x,ccw->y);
        
        // Work CW
        prev = tri;
        next = tri->NeighborCW(*p);
        loop = next == tri;
        while (next != nullptr && !loop) {
            result.push_front(getCircumcenter(next));
            prev = next;
            next = next->NeighborCW(*p);
            loop = next == tri;
        }
        
        CUAssertLog(!loop, "Delaunay map had degenerate loop");

        p2t::Point* cw = prev->PointCW(*p);
        Vec2 ranchor(cw->x,cw->y);
        
        // Computation time
        Vec2 r = lanchor-ranchor;
        Vec2 q = origin -ranchor;
        Vec2::project(q, r, &q);
        q += ranchor;
        q = (origin-q)+origin;
        p2t::Point anchor(q.x,q.y);
        
        result.push_back(getCircumcenter(p,ccw,&anchor));
        result.push_back(getCircumcenter(p,cw, &anchor));
    }
    return result;
}

/**
 * Returns the circumcenter for the given triangle tri
 *
 * @param tri   The triangle to compute
 *
 * @return the centriod for the given triangle tri
 */
Vec2 DelaunayTriangulator::getCircumcenter(p2t::Triangle* tri) const {
    p2t::Point* v1 = tri->GetPoint(0);
    p2t::Point* v2 = tri->GetPoint(1);
    p2t::Point* v3 = tri->GetPoint(2);
    return getCircumcenter(v1,v2,v3);
}

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
Vec2 DelaunayTriangulator::getCircumcenter(p2t::Point* p1, p2t::Point* p2, p2t::Point* p3) const {
    double ab = (p1->x * p1->x) + (p1->y * p1->y);
    double cd = (p2->x * p2->x) + (p2->y * p2->y);
    double ef = (p3->x * p3->x) + (p3->y * p3->y);

    double x = (ab * (p3->y - p2->y) + cd * (p1->y - p3->y) + ef * (p2->y - p1->y));
    x /= (p1->x * (p3->y - p2->y) + p2->x * (p1->y - p3->y) + p3->x * (p2->y - p1->y));
    x /= 2.0f;

    double y = (ab * (p3->x - p2->x) + cd * (p1->x - p3->x) + ef * (p2->x - p1->x));
    y /= (p1->y * (p3->x - p2->x) + p2->y * (p1->x - p3->x) + p3->y * (p2->x - p1->x));
    y /= 2.0f;

    return Vec2(x,y);
}
