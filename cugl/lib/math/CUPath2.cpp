//
//  CUPath2.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a class that represents a flattened polyline (e.g. a
//  1-dimensional, piecewise linear path). In previous iterations of CUGL, this
//  functionality was included as part of the Poly2 class. However, as we added
//  more computational geometry features to the engine, this became untenable.
//  So we elected to separate the two.
//
//  Paths can be converted into Poly2 objects by using either a triangulator or
//  an extruder.  In the case of triangulation, the interior of a Path is always
//  determined by the left (counter-clockwise) sides.  Hence the boundary of
//  of a shape should be a counter-clockwise path, while any hole should be a
//  clockwise path.
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
//  Version: 6/20/21
#include <cmath>
#include <sstream>
#include <cugl/util/CUDebug.h>
#include <cugl/math/CUPath2.h>
#include <cugl/math/CUMat4.h>
#include <cugl/math/CUAffine2.h>
#include <cugl/assets/CUJsonValue.h>

using namespace cugl;

#pragma mark Assigment
/**
 * Sets the path to have the given vertices
 *
 * No vertices are marked are as corner vertices. The path will be open.
 *
 * This method returns a reference to this path for chaining.
 *
 * @param vertices  The vector of vertices (as Vec2) in this path
 *
 * @return This path, returned for chaining
 */
Path2& Path2::set(const std::vector<Vec2>& vertices) {
    this->vertices = vertices;
    corners.clear();
    closed = false;
    return *this;
}

/**
 * Sets the path to have the given vertices.
 *
 * No vertices are marked are as corner vertices. The path will be open.
 *
 * This method returns a reference to this path for chaining.
 *
 * @param vertices  The array of vertices in this path
 * @param vertsize  The number of elements to use from vertices
 *
 * @return This path, returned for chaining
 */
Path2& Path2::set(const Vec2* vertices, size_t vertsize) {
    this->vertices.assign(vertices,vertices+vertsize);
    corners.clear();
    closed = false;
    return *this;
}

/**
 * Sets this path to be a copy of the given one.
 *
 * All of the contents are copied, so that this path does not hold any
 * references to elements of the other path.
 *
 * This method returns a reference to this path for chaining.
 *
 * @param path  The path to copy
 *
 * @return This path, returned for chaining
 */
Path2& Path2::set(const Path2& path) {
    vertices = path.vertices;
    corners  = path.corners;
    closed = path.closed;
    return *this;
}

/**
 * Sets the path to represent the given rectangle.
 *
 * The path will have four vertices, one for each corner of the rectangle.
 * The path will be closed.
 *
 * This method returns a reference to this path for chaining.
 *
 * @param rect  The rectangle to copy
 *
 * @return This path, returned for chaining
 */
Path2& Path2::set(const Rect rect) {
    vertices.clear();
    corners.clear();
    if (rect.size != Size::ZERO) {
        vertices.reserve(4);
        corners.reserve(4);
        Vec2 temp;
        temp.set(rect.origin);
        vertices.push_back(temp);
        corners.emplace(0);
        temp.y += rect.size.height;
        vertices.push_back(temp);
        corners.emplace(1);
        temp.x += rect.size.width;
        vertices.push_back(temp);
        corners.emplace(2);
        temp.y -= rect.size.height;
        vertices.push_back(temp);
        corners.emplace(3);
        closed = true;
    }
    return *this;
}

/**
 * Sets this path from the data in the given JsonValue
 *
 * The JsonValue should either be an array of floats or an JSON object.
 * If it is an array of floats, then it interprets those floats as the
 * vertices. All points are corners and the path is open.
 *
 * On the other hand, if it is a JSON object, it supports the following
 * attributes:
 *
 *     "vertices":  An (even) list of floats, representing the vertices
 *     "corners":   A list of integers representing corner positions
 *     "closed":    A boolean value, representing if the path is closed
 *
 * All attributes are optional. If "vertices" are missing, then the path
 * will be empty. If "corners" is missing, then all vertices are corners.
 * If "closed" is missing, then the path is open by default.
 *
 * @param data      The JSON object specifying the path
 *
 * @return This path, returned for chaining
 */
Path2& Path2::set(const std::shared_ptr<JsonValue>& data) {
    vertices.clear();
    corners.clear();
    if (data->isArray()) {
        JsonValue* path = data.get();
        CUAssertLog(path->size() % 2 == 0, "path data should be an even list of numbers");
        vertices.reserve(path->size()/2);
        corners.reserve(path->size()/2);
        for(Uint32 ii = 0; ii < path->size(); ii += 2) {
            Vec2 vert;
            vert.x = path->get(ii  )->asFloat(0.0f);
            vert.y = path->get(ii+1)->asFloat(0.0f);
            vertices.push_back(vert);
            corners.emplace(ii/2);
        }
        closed = false;
    } else if (data->has("vertices")) {
        JsonValue* path = data->get("vertices").get();
        CUAssertLog(path->size() % 2 == 0, "'vertices' should be an even list of numbers");
        vertices.reserve(path->size()/2);
        for(int ii = 0; ii < path->size(); ii += 2) {
            Vec2 vert;
            vert.x = path->get(ii  )->asFloat(0.0f);
            vert.y = path->get(ii+1)->asFloat(0.0f);
            vertices.push_back(vert);
        }
        if (data->has("corners")) {
            JsonValue* hinge = data->get("corners").get();
            corners.reserve(hinge->size());
            for(Uint32 ii = 0; ii < hinge->size(); ii++) {
                corners.emplace(hinge->get(ii  )->asLong(0L));
            }
        } else {
            for(Uint32 ii = 0; ii < vertices.size(); ii++) {
                corners.emplace(ii);
            }
        }
        closed = data->getBool("closed",false);
    }
    return *this;
}

/**
 * Clears the contents of this path
 *
 * @return This path, returned for chaining
 */
Path2& Path2::clear() {
    vertices.clear();
    corners.clear();
    closed = false;
    return *this;
}


#pragma mark -
#pragma mark Path Attributes
/**
 * Returns true if the point at the given index is a corner
 *
 * Corner points will be assigned a joint style when extruded. Points
 * that are not corners will be extruded smoothly (typically because
 * they are the result of a bezier expansion).
 *
 * @param index  The attribute index
 *
 * @return true if the point at the given index is a corner
 */
bool Path2::isCorner(size_t index) const {
    return corners.find(index) != corners.end();
}

/**
 * Returns a list of vertex indices representing this path.
 *
 * The indices are intended to be used in a drawing mesh to
 * display this path. The number of indices will be a multiple
 * of two.
 *
 * @return a list of vertex indices for using in a path mesh.
 */
std::vector<Uint32> Path2::getIndices() const {
    std::vector<Uint32> result;
    getIndices(result);
    return result;
}

/**
 * Stores a list of vertex indices in the given buffer.
 *
 * The indices are intended to be used in a drawing mesh to
 * display this path. The number of indices will be a multiple
 * of two.
 *
 * The indices will be appended to the provided vector. You should clear
 * the vector first if you do not want to preserve the original data.
 * If the calculation is not yet performed, this method will do nothing.
 *
 * @param buffer    a buffer to store the list of indices.
 *
 * @return the number of elements added to the buffer
 */
size_t Path2::getIndices(std::vector<Uint32>& buffer) const {
    size_t isize = buffer.size();
    buffer.reserve(isize+2*vertices.size());
    for(Uint32 ii = 0; ii < vertices.size()-1; ii++) {
        buffer.push_back(ii  );
        buffer.push_back(ii+1);
    }
    if (closed) {
        buffer.push_back((Uint32)vertices.size()-1);
        buffer.push_back(0);
    }
    return buffer.size()-isize;
}

#pragma mark -
#pragma mark Path Modification
/**
 * Returns the former end point in the path, after removing it
 *
 * If this path is empty, this will return the zero vector.
 *
 * @return the former end point in the path
 */
Vec2 Path2::pop() {
    CUAssertLog(vertices.size(), "Path is currently empty");
    if (vertices.empty()) {
        return Vec2::ZERO;
    }
    
    Vec2 result = vertices.back();
    vertices.pop_back();
    Uint32 pos = (Uint32)vertices.size();
    corners.erase(pos);
    return result;
}

/**
 * Adds a point to the end of this path
 *
 * @param point     The point to add
 * @param corner    Whether this point is a corner
 */
void Path2::push(Vec2 point, bool corner) {
    Uint32 pos = (Uint32)vertices.size();
    vertices.push_back(point);
    if (corner) {
        corners.emplace(pos);
    }
}

/**
 * Adds a point to the end of this path
 *
 * @param x         The x-coordinate to add
 * @param y         The y-coordinate to add
 * @param corner    Whether this point is a corner
 */
void Path2::push(float x, float y, bool corner) {
    Uint32 pos = (Uint32)vertices.size();
    vertices.push_back(Vec2(x,y));
    if (corner) {
        corners.emplace(pos);
    }
}

/**
 * Returns the former point at the given index, after removing it
 *
 * If this path is empty, this will return the zero vector.
 *
 * @return the former point at the given index
 */
Vec2 Path2::remove(size_t index) {
    CUAssertLog(index < vertices.size(), "Index %zu is out of bounds",index);
    if (vertices.empty()) {
        return Vec2::ZERO;
    }
    Vec2 result = vertices[index];
    vertices.erase(vertices.begin()+index);
    if (corners.size() > 0) {
        bool end = index == vertices.size();
        if (end) {
            corners.erase(index);
        } else {
            std::unordered_set<size_t> alts;
            for(auto it = corners.begin(); it != corners.end(); ++it) {
                if (*it < index) {
                    alts.emplace(*it);
                } else if (*it > index) {
                    alts.emplace(*it-1);
                }
            }
            corners = alts;
        }
    }
    return result;
}

/**
 * Adds a point at the given index
 *
 * @param index     The index to add the point
 * @param point     The point to add
 * @param corner    Whether this point is a corner
 */
void Path2::add(size_t index, Vec2 point, bool corner) {
    CUAssertLog(index <= vertices.size(), "Index %zu is out of bounds",index);
    bool end = index == vertices.size();
    vertices.insert(vertices.begin()+index, point);
    if (corners.size() > 0 && !end) {
        std::unordered_set<size_t> alts;
        for(auto it = corners.begin(); it != corners.end(); ++it) {
            if (*it < index) {
                alts.emplace(*it);
            } else if (*it >= index) {
                alts.emplace(*it+1);
            }
        }
        corners = alts;
    }
    if (corner) {
        corners.emplace(index);
    }
}

/**
 * Adds a point at the given index
 *
 * @param index     The index to add the point
 * @param x         The x-coordinate to add
 * @param y         The y-coordinate to add
 * @param corner    Whether this point is a corner
 */
void Path2::add(size_t index, float x, float y, bool corner) {
    CUAssertLog(index <= vertices.size(), "Index %zu is out of bounds",index);
    bool end = index == vertices.size();
    vertices.insert(vertices.begin()+index, Vec2(x,y));
    if (corners.size() > 0 && !end) {
        std::unordered_set<size_t> alts;
        for(auto it = corners.begin(); it != corners.end(); ++it) {
            if (*it < index) {
                alts.emplace(*it);
            } else if (*it >= index) {
                alts.emplace(*it+1);
            }
        }
        corners = alts;
    }
    if (corner) {
        corners.emplace(index);
    }
}

/**
 * Allocates space in this path for the given number of points.
 *
 * This method can help performance when a path is being constructed
 * piecemeal.
 *
 * @param size  The number of spots allocated for future points.
 */
void Path2::reserve(size_t size) {
    vertices.reserve(size);
    corners.reserve(size);
}

#pragma mark -
#pragma mark Geometry Methods
/**
 * Returns true if the given point is incident to the given line segment.
 *
 * The variance specifies the tolerance that we allow for begin off the line
 * segment.
 *
 * @param point     The point to check
 * @param a         The start of the line segment
 * @param b         The end of the line segment
 * @param variance  The distance tolerance
 *
 * @return true if the given point is incident to the given line segment.
 */
static bool onsegment(const Vec2& point, const Vec2& a, const Vec2& b, float variance) {
    float d1 = point.distance(a);
    float d2 = point.distance(b);
    float d3 = a.distance(b);
    return fabsf(d3-d2-d1) <= variance;
}

/**
 * This class implements a pivot for the Graham Scan convex hull algorithm.
 *
 * This pivot allows us to have a relative comparison function to an
 * anchor point.
 */
class Path2Pivot {
public:
    /** The pivot anchor */
    const Vec2* anchor;
    /** The vertex set */
    const std::vector<Vec2>* vertices;
    
    /** Constructs an empty pivot */
    Path2Pivot() : anchor(nullptr) {}
    
    /**
     * Returns true if a < b in the polar order with respect to the pivot.
     *
     * The polar order is computed relative to the pivot anchor.
     *
     * @param a     The first point
     * @param b     The second point
     *
     * @return true if a < b in the polar order with respect to the pivot.
     */
    bool compare(const Vec2 a, const Vec2 b) {
        int order = Path2::orientation(*anchor, a, b);
        if (order == 0) {
            float d1 = anchor->distanceSquared(a);
            float d2 = anchor->distanceSquared(b);
            return (d1 < d2 || (d1 == d2 && a < b));
        }
        return (order == -1);
    }

    /**
     * Returns true if vertex[a] < vertex[b] in the polar order with respect to the pivot.
     *
     * The polar order is computed relative to the pivot anchor. This comparator
     * allows us to pivot on indices rather than points.
     *
     * @param a     The index of the first point
     * @param b     The index of the second point
     *
     * @return true if vertex[a] < vertex[b] in the polar order with respect to the pivot.
     */
    bool compare(Uint32 a, Uint32 b) {
        int order = Path2::orientation(*anchor, vertices->at(a), vertices->at(b));
        if (order == 0) {
            float d1 = anchor->distanceSquared(vertices->at(a));
            float d2 = anchor->distanceSquared(vertices->at(b));
            return (d1 < d2 || (d1 == d2 && vertices->at(a) < vertices->at(a)));
        }
        return (order == -1);
    }
};

/**
 * Returns the vertex indices forming the convex hull of this path.
 *
 * The returned set of indices is guaranteed to be a counter-clockwise traversal
 * of the hull.
 *
 * The points on the convex hull define the "border" of the shape.  In addition
 * to minimizing the number of vertices, this is useful for determining whether
 * or not a point lies on the boundary.
 *
 * This implementation is adapted from the example at
 *
 *   http://www.geeksforgeeks.org/convex-hull-set-2-graham-scan/
 *
 * @return the vertex indices forming the convex hull of this polygon.
 */
std::vector<Uint32> Path2::convexHull() const {
    return convexHull(vertices);
}

/**
 * Returns the vertex indices forming the convex hull of the given points.
 *
 * The returned set of indices is guaranteed to be a counter-clockwise traversal
 * of the hull.
 *
 * The points on the convex hull define the "border" of the shape.  In addition
 * to minimizing the number of vertices, this is useful for determining whether
 * or not a point lies on the boundary.
 *
 * This implementation is adapted from the example at
 *
 *   http://www.geeksforgeeks.org/convex-hull-set-2-graham-scan/
 *
 * @param vertices  The points for the computation
 *
 * @return the vertex indices forming the convex hull of the given points.
 */
std::vector<Uint32> Path2::convexHull(const std::vector<Vec2>& vertices) {
    std::vector<Uint32> indices;
    indices.resize(vertices.size());
    for(Uint32 ii = 0; ii < vertices.size(); ii++) {
        indices.push_back(ii);
    }
    std::vector<Uint32> hull;

    // Find the bottommost point (or chose the left most point in case of tie)
    Uint32 n = (Uint32)indices.size();
    Uint32 ymin = 0;
    for (Uint32 ii = 1; ii < n; ii++) {
        float y1 = vertices[ii].y;
        float y2 = vertices[ymin].y;
        if (y1 < y2 || (y1 == y2 && vertices[ii].x < vertices[ymin].x)) {
            ymin = ii;
        }
    }
    
    // Place the bottom-most point at first position
    Uint32 temp = indices[0];
    indices[0] = indices[ymin];
    indices[ymin] = temp;

    // Set the pivot at this first point
    Path2Pivot pivot;
    pivot.anchor = &(vertices[indices[0]]);
    pivot.vertices = &vertices;
    
    // Sort the remaining points by polar angle.
    // This creates a counter-clockwise traversal of the points.
    std::sort(indices.begin()+1, indices.end(),
              [&](Uint32 a, Uint32 b) { return pivot.compare(a,b); });
    
    
    // Remove the colinear points.
    int m = 1;
    for (int ii = 1; ii < n; ii++) {
        // Keep removing i while angle of i and i+1 is same with respect to pivot
        while (ii < n-1 && orientation(*(pivot.anchor),
                                       vertices[indices[ii]],
                                       vertices[indices[ii+1]]) == 0) {
            ii++;
        }
        
        indices[m] = indices[ii];
        m++;   // Update size of modified array
    }
    indices.resize(m);

    // If modified array of points has less than 3 points, convex hull is not possible
    if (m < 3) {
        return hull;
    }
    
    // Push first three points to the vector (as a stack).
    hull.push_back(indices[0]);
    hull.push_back(indices[1]);
    hull.push_back(indices[2]);
    
    // Process remaining n-3 points
    for (int ii = 3; ii < m; ii++) {
        // Keep removing back whenever we make a non-left turn
        const Vec2* atback   = &(vertices[hull[hull.size()-1]]);
        const Vec2* nextback = &(vertices[hull[hull.size()-2]]);
        
        while (hull.size() >= 2 && orientation(*nextback, *atback, vertices[indices[ii]]) != -1) {
            hull.pop_back();
            atback = nextback;
            nextback =  &(vertices[hull[hull.size()-2]]);
        }
        hull.push_back(indices[ii]);
    }
    return hull;
}

/**
 * Returns true if the interior of this path contains the given point.
 *
 * This method returns false if the path is open.  Otherwise, it uses an even-odd 
 * crossing rule to determine containment. Containment is not strict. Points on the 
 * boundary are contained within this polygon.
 *
 * @param x         The x-coordinate to test
 * @param y         The y-coordinate to test
 *
 * @return true if this path contains the given point.
 */
bool Path2::contains(float x, float y) const {
    // Use a winding rule otherwise
    int intersects = 0;
    
	for (size_t ii = 0; ii < vertices.size()-1; ii++) {
		const Vec2* v1 = &(vertices[ii]);
        const Vec2* v2 = &(vertices[ii+1]);
		if (((v1->y <= y && y < v2->y) || (v2->y <= y && y < v1->y)) && x < ((v2->x - v1->x) / (v2->y - v1->y) * (y - v1->y) + v1->x)) {
			intersects++;
		}
	}
    const Vec2* v1 = &(vertices[vertices.size() - 1]);
    const Vec2* v2 = &(vertices[0]);
    if (((v1->y <= y && y < v2->y) || (v2->y <= y && y < v1->y)) && x < ((v2->x - v1->x) / (v2->y - v1->y) * (y - v1->y) + v1->x)) {
        intersects++;
    }
    return (intersects & 1) == 1;
}



/**
 * Returns true if the given point is on the path.
 *
 * This method returns true if the point is within margin of error of a
 * line segment.
 *
 * @param x     The x-coordinate to test
 * @param y     The y-coordinate to test
 * @param err   The distance tolerance
 *
 * @return true if the given point is on the path.
 */
bool Path2::incident(float x, float y, float err) const {
    Vec2 p(x,y);
	for (int ii = 0; ii < vertices.size()-1; ii++) {
		if (onsegment(p,vertices[ii],vertices[ii+1],err)) {
			return true;
		}
	}
	if (closed) {
		return onsegment(p,vertices[vertices.size()-1],vertices[0],err);
	}
	return false;
}

/**
 * Returns the number of left turns in this path.
 *
 * Left turns are determined by looking at the interior angle generated at
 * each point (assuming that the path is intended to be counterclockwise).
 * In the case of an open path, the first and last vertexes are not counted.
 *
 * This method is a generalization of {@link isConvex} that can be used to
 * analyze the convexity of a path.
 *
 * @return true if this path defines a convex shape.
 */
Uint32 Path2::leftTurns() const {
    if (vertices.size() <= 2) {
        return 0;
    }

    Uint32 nleft  = 0;
    if (closed) {
        const Vec2* p0 = &(vertices.back());
        const Vec2* p1 = &(vertices.front());
        for(auto it = vertices.begin()+1; it != vertices.end(); ++it) {
            const Vec2* p2 = &(*it);
            float cross = (p2->x-p1->x) * (p1->y-p0->y) - (p1->x-p0->x) * (p2->y-p1->y);
            if (cross < 0.0) { nleft += 1; }
            p0 = p1;
            p1 = p2;
        }

        const Vec2* p2 = &(vertices.front());
        float cross = (p2->x-p1->x) * (p1->y-p0->y) - (p1->x-p0->x) * (p2->y-p1->y);
        if (cross < 0.0) { nleft += 1; }
    } else {
        const Vec2* p0 = &(vertices[0]);
        const Vec2* p1 = &(vertices[1]);
        for(auto it = vertices.begin()+2; it != vertices.end(); ++it) {
            const Vec2* p2 = &(*it);
            float cross = (p2->x-p1->x) * (p1->y-p0->y) - (p1->x-p0->x) * (p2->y-p1->y);
            if (cross < 0.0) { nleft += 1; }
            p0 = p1;
            p1 = p2;
        }
    }
    return nleft;
}

/**
 * Returns true if this path defines a convex shape.
 *
 * This method returns false if the path is open.
 *
 * @return true if this path defines a convex shape.
 */
bool Path2::isConvex() const {
    if (vertices.size() <= 2 || !closed) {
        return false;
    }

    Uint32 nleft  = 0;
    const Vec2* p0 = &(vertices.back());
    const Vec2* p1 = &(vertices.front());
    for(auto it = vertices.begin()+1; it != vertices.end(); ++it) {
        const Vec2* p2 = &(*it);
        float cross = (p2->x-p1->x) * (p1->y-p0->y) - (p1->x-p0->x) * (p2->y-p1->y);
        if (cross < 0.0) { nleft += 1; }
        p0 = p1;
        p1 = p2;
    }

    const Vec2* p2 = &(vertices.front());
    float cross = (p2->x-p1->x) * (p1->y-p0->y) - (p1->x-p0->x) * (p2->y-p1->y);
    if (cross < 0.0) { nleft += 1; }
    return nleft == vertices.size();
}



#pragma mark -
#pragma mark Orientation Methods
/**
 * Returns the area enclosed by this path.
 *
 * The area is defined as the sum of oriented triangles in a triangle
 * fan from a point on the convex hull. Counter-clockwise triangles
 * have positive area, while clockwise triangles have negative area.
 * The result agrees with the traditional concept of area for counter
 * clockwise paths.
 *
 * The area can be used to determine the orientation.  It the area is
 * negative, that means this path essentially represents a hole (e.g.
 * is clockwise instead of counter-clockwise).
 *
 * @return the area enclosed by this path.
 */
float Path2::area() const {
    Vec2 ab, ac;
    Vec2 a = vertices[0];
    float area = 0.0f;
    for (size_t ii = 2; ii < vertices.size() ; ii++) {
        ab = vertices[ii-1];
        ac = vertices[ii];
        ab -= a;
        ac -= a;
        area += ab.x*ac.y - ac.x*ab.y;
    }
    return area * 0.5f;
}


/**
 * Returns -1, 0, or 1 indicating the path orientation.
 *
 * If the method returns -1, this is a counter-clockwise path. If 1, it
 * is a clockwise path.  If 0, that means it is undefined.  The
 * orientation can be undefined if all the points are colinear.
 *
 * @return -1, 0, or 1 indicating the path orientation.
 */
int Path2::orientation() const {
    size_t idx = hullPoint();
    size_t bx = idx == 0 ? vertices.size()-1 : idx-1;
    size_t ax = idx == vertices.size()-1 ? 0 : idx+1;
    return orientation(vertices[bx],vertices[idx],vertices[ax]);
}

/**
 * Returns -1, 0, or 1 indicating the orientation of a -> b -> c
 *
 * If the function returns -1, this is a counter-clockwise turn.  If 1, it
 * is a clockwise turn.  If 0, it is colinear.
 *
 * @param a     The first point
 * @param b     The second point
 * @param c     The third point
 *
 * @return -1, 0, or 1 indicating the orientation of a -> b -> c
 */
int Path2::orientation(const Vec2& a, const Vec2& b, const Vec2& c) {
    float val = (b.y - a.y) * (c.x - a.x) - (b.x - a.x) * (c.y - a.y);
    if (-CU_MATH_EPSILON < val && val < CU_MATH_EPSILON) return 0;  // colinear
    return (val > 0) ? 1: -1; // clock or counterclock wise
}

/**
 * Returns -1, 0, or 1 indicating the path orientation.
 *
 * If the method returns -1, this is a counter-clockwise path. If 1, it
 * is a clockwise path.  If 0, that means it is undefined.  The
 * orientation can be undefined if all the points are colinear.
 *
 * @param path  The path to check
 *
 * @return -1, 0, or 1 indicating the path orientation.
 */
int Path2::orientation(const std::vector<Vec2>& path) {
    size_t idx = hullPoint(path);
    size_t bx = idx == 0 ? path.size()-1 : idx-1;
    size_t ax = idx == path.size()-1 ? 0 : idx+1;
    return orientation(path[bx],path[idx],path[ax]);
}

/**
 * Returns -1, 0, or 1 indicating the path orientation.
 *
 * If the method returns -1, this is a counter-clockwise path. If 1, it
 * is a clockwise path.  If 0, that means it is undefined.  The
 * orientation can be undefined if all the points are colinear.
 *
 * @param path  The path to check
 * @param size  The path size
 *
 * @return -1, 0, or 1 indicating the path orientation.
 */
int Path2::orientation(const Vec2* path, size_t size) {
    size_t idx = hullPoint(path,size);
    size_t bx = idx == 0 ? size-1 : idx-1;
    size_t ax = idx == size-1 ? 0 : idx+1;
    return orientation(path[bx],path[idx],path[ax]);
}

    
/**
 * Reverses the orientation of this path in place
 *
 * The path will have all of its vertices in the reverse order from the
 * original. This path will not be affected.
 *
 * @return This path, returned for chaining
 */
Path2& Path2::reverse() {
    size_t end = vertices.size()-1;
    for(size_t ii = 0; ii < vertices.size()/2; ii++) {
        Vec2 temp = vertices[end-ii];
        vertices[end-ii] = vertices[ii];
        vertices[ii] = temp;
    }
    if (corners.size() > 0) {
        std::unordered_set<size_t> alts;
        alts.reserve(corners.size());
        for(auto it = corners.begin(); it != corners.end(); ++it) {
            alts.emplace(end-*it);
        }
        corners = alts;
    }
    return *this;
}

/**
 * Returns a path with the reverse orientation of this one.
 *
 * The path will have all of its vertices in the reverse order from the
 * original. This path will not be affected.
 *
 * @return a path with the reverse orientation of this one.
 */
Path2 Path2::reversed() const {
    Path2 copy = *this;
    copy.reverse();
    return copy;
}

/**
 * Returns an index of a point on the convex hull
 *
 * The expact point returned is not guaranteed, but it is typically
 * with the least x and y values (whenever that is possible).
 * @return an
 */
size_t Path2::hullPoint() const {
    return hullPoint(vertices);
}

/**
 * Returns an index of a point on the convex hull
 *
 * The expact point returned is not guaranteed, but it is typically
 * with the least x and y values (whenever that is possible).
 *
 * @param path  The path to check
 *
 * @return an index of a point on the convex hull
 */
size_t Path2::hullPoint(const std::vector<Vec2>& path) {
    CUAssertLog(!path.empty(), "The path is empty");
    
    // Find the min point
    double mx = path[0].x;
    double my = path[0].y;
    size_t pos = 0;
    for(size_t ii = 1; ii < path.size(); ii++) {
        if (path[ii].x < mx) {
            mx = path[ii].x;
            my = path[ii].y;
            pos = ii;
        } else if (path[ii].x == mx && path[ii].y < my) {
            my = path[ii].y;
            pos = ii;
        }
    }
    return pos;
}

/**
 * Returns an index of a point on the convex hull
 *
 * The expact point returned is not guaranteed, but it is typically
 * with the least x and y values (whenever that is possible).
 *
 * @param path  The path to check
 * @param size  The path size
 *
 * @return an index of a point on the convex hull
 */
size_t Path2::hullPoint(const Vec2* path, size_t size) {
    CUAssertLog(path != nullptr && size != 0, "The path is empty");
    
    // Find the min point
    double mx = path[0].x;
    double my = path[0].y;
    size_t pos = 0;
    for(size_t ii = 1; ii < size; ii++) {
        if (path[ii].x < mx) {
            mx = path[ii].x;
            my = path[ii].y;
            pos = ii;
        } else if (path[ii].x == mx && path[ii].y < my) {
            my = path[ii].y;
            pos = ii;
        }
    }
    return pos;
}


#pragma mark -
#pragma mark Operators
/**
 * Uniformly scales all of the vertices of this path.
 *
 * The vertices are scaled from the origin of the coordinate space. This
 * means that if the origin is not path of this path, then the path will
 * be effectively translated by the scaling.
 *
 * @param scale The uniform scaling factor
 *
 * @return This path, scaled uniformly.
 */
Path2& Path2::operator*=(float scale) {
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        *it *= scale;
    }
    return *this;
}
    
/**
 * Nonuniformly scales all of the vertices of this path.
 *
 * The vertices are scaled from the origin of the coordinate space. This
 * means that if the origin is not path of this path, then the path will
 * be effectively translated by the scaling.
 *
 * @param scale The non-uniform scaling factor
 *
 * @return This path, scaled non-uniformly.
 */
Path2& Path2::operator*=(const Vec2 scale) {
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        *it *= scale;
    }
    return *this;
}

/**
 * Transforms all of the vertices of this path.
 *
 * @param transform The affine transform
 *
 * @return This path with the vertices transformed
 */
Path2& Path2::operator*=(const Affine2& transform) {
    Vec2 tmp;
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        Affine2::transform(transform,*it,&tmp);
        *it = tmp;
    }
    return *this;
}

/**
 * Transforms all of the vertices of this path.
 *
 * The vertices are transformed as 3d points. The z-value is 0.
 *
 * @param transform The transform matrix
 *
 * @return This path with the vertices transformed
 */
Path2& Path2::operator*=(const Mat4& transform) {
    Vec2 tmp;
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        Mat4::transform(transform,*it,&tmp);
        *it = tmp;
    }
    return *this;
}

/**
 * Uniformly scales all of the vertices of this path.
 *
 * The vertices are scaled from the origin of the coordinate space. This
 * means that if the origin is not path of this path, then the path will
 * be effectively translated by the scaling.
 *
 * @param scale The inverse of the uniform scaling factor
 *
 * @return This path, scaled uniformly.
 */
Path2& Path2::operator/=(float scale) {
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        *it /= scale;
    }
    return *this;
}

/**
 * Nonuniformly scales all of the vertices of this path.
 *
 * The vertices are scaled from the origin of the coordinate space. This
 * means that if the origin is not path of this path, then the path will
 * be effectively translated by the scaling.
 *
 * @param scale The inverse of the non-uniform scaling factor
 *
 * @return This path, scaled non-uniformly.
 */
Path2& Path2::operator/=(const Vec2 scale) {
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        *it /= scale;
    }
    return *this;
}

/**
 * Translates all of the vertices of this path.
 *
 * @param offset The translation amount
 *
 * @return This path, translated
 */
Path2& Path2::operator+=(const Vec2 offset) {
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        *it += offset;
    }
    return *this;
}

/**
 * Translates all of the vertices of this path.
 *
 * @param offset The inverse of the translation amount
 *
 * @return This path, translated
 */
Path2& Path2::operator-=(const Vec2 offset) {
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        *it -= offset;
    }
    return *this;
}

#pragma mark -
#pragma mark Slicing Methods
/**
 * Appends the given path to the end of this one
 *
 * The vertices are appended in order to the end of the path.  If
 * the original path was closed, it is now open (regardless of
 * whether or not extra is closed)
 *
 * @param extra The path to append
 *
 * @return This path, extended.
 */
Path2& Path2::operator+=(const Path2& extra) {
    size_t size = vertices.size();
    vertices.insert(vertices.end(), extra.vertices.begin(), extra.vertices.end());
    corners.reserve(corners.size()+extra.corners.size());
    for(auto it = extra.corners.begin(); it != extra.corners.end(); ++it) {
        corners.emplace(*it+size);
    }
    closed = false;
    return *this;
}

/**
 * Returns the slice of this path between start and end.
 *
 * The sliced path will use the indices from start to end (not including
 * end). It will include the vertices referenced by those indices, and
 * only those vertices. The resulting path is open.
 *
 * @param start The start index
 * @param end   The end index
 *
 * @return the slice of this mesh between start and end.
 */
Path2 Path2::slice(size_t start, size_t end) const {
    CUAssertLog(start <= end, "The indices %zu, %zu are invalid", start, end);
    Path2 copy;
    if (start < end) {
        copy.vertices.assign(vertices.begin()+start, vertices.begin()+(end-1));
        for(auto it = corners.begin(); it != corners.end(); ++it) {
            if (*it >= start && *it < end) {
                copy.corners.emplace(*it-start);
            }
        }
        copy.closed = false;
    }
    return copy;
}
    
#pragma mark -
#pragma mark Conversion Methods
/**
 * Returns a string representation of this path for debugging purposes.
 *
 * If verbose is true, the string will include class information.  This
 * allows us to unambiguously identify the class.
 *
 * @param verbose Whether to include class information
 *
 * @return a string representation of this path for debuggging purposes.
 */
std::string Path2::toString(bool verbose) const {
    std::stringstream ss;
    ss << (verbose ? "cugl::Path2[" : "[");
    ss << (closed ? "CLOSED"  : "OPEN");
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        ss << (it == vertices.begin() ? "; " : ", ");
        ss << it->toString();
    }
    ss << "]";
    return ss.str();
}

/**
 * Returns the bounding box for the path
 *
 * The bounding box is the minimal rectangle that contains all of the vertices in
 * this path.  This method will recompute the bounds and is hence O(n).
 *
 * @return the bounding box for the path
 */
const Rect Path2::getBounds() const {
    if (vertices.empty()) {
        return Rect::ZERO;
    }
    float minx, maxx;
    float miny, maxy;
    
    minx = vertices[0].x;
    maxx = vertices[0].x;
    miny = vertices[0].y;
    maxy = vertices[0].y;
    for(auto it = vertices.begin()+1; it != vertices.end(); ++it) {
        if (it->x < minx) {
            minx = it->x;
        } else if (it->x > maxx) {
            maxx = it->x;
        }
        if (it->y < miny) {
            miny = it->y;
        } else if (it->y > maxy) {
            maxy = it->y;
        }
    }
    
    return Rect(minx,miny,maxx-minx,maxy-miny);
}

/** Cast from Path2 to a Rect. */
Path2::operator Rect() const  { return getBounds(); }



