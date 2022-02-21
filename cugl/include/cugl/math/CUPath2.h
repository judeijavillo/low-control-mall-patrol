//
//  CUPath2.h
//  CUGL
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
#ifndef __CU_PATH2_H__
#define __CU_PATH2_H__

#include <vector>
#include <unordered_set>
#include <cugl/math/CUVec2.h>
#include <cugl/math/CURect.h>


namespace cugl {

// Forward references
class Mat4;
class Affine2;
class JsonValue;
    
/**
 * Class to represent a flattened polyline.
 *
 * This class is intended to represent any continuous polyline.  While it may be
 * either open or closed, it should not have any gaps between vertices.  If you
 * need a path with gaps, that should be represented by multiple Path2 objects.
 *
 * It is possible to draw a path object directly to a {@link SpriteBatch}. However,
 * in most applications you will want to convert a path object to a {@link Poly2}
 * for width and texturing.  In particular, you will often want to either extrude
 * (give stroke width) or triangulate (fill) a path.
 *
 * We have provided several factories for converting a path to a {@link Poly2}.
 * These factories allow for delegating index computation to a separate thread,
 * if it takes too long. These factories are as follows:
 *
 * {@link EarclipTriangulator}: This is a simple earclipping-triangulator for
 * tesselating paths into polygons. It supports holes, but does not support
 * self-intersections. While it produces better (e.g. less thin) triangles
 * than MonotoneTriangulator, this comes at a cost. This triangulator has
 * worst case O(n^2).  With that said, it has low overhead and so is very
 * efficient on small polygons.
 *
 * {@link DelaunayTriangulator}: This is a Delaunay Triangular that gives a
 * more uniform triangulation in accordance to the Vornoi diagram. This
 * triangulator uses an advancing-front algorithm that is the fastest in
 * practice (though worst case O(n log n) is not guaranteed).  However, it
 * has a lot of overhead that is unnecessary for small polygons. As with
 * EarclipTriangulator, it supports holes, but does not support
 * self-intersections.
 *
 * {@link PathFactory}: This is a tool is used to generate several basic
 * path shapes, such as rounded rectangles or arcs. It also allows you
 * construct wireframe traversals of polygon meshes.
 *
 * {@link SimpleExtruder}: This is a tool can take a path and convert it
 * into a solid polygon. This solid polygon is the same as the path, except
 * that the path now has a width and a mitre at the joints.  This algorithm
 * is quite fast, but the resulting polygon may overlap itself. This is ideal
 * for strokes that only need to be drawn and do not need accurate geometric
 * information.
 *
 * {@link ComplexExtruder}: Like {@link SimpleExtruder}, this is a tool can
 * take a path polygon and convert it into a solid polygon. However it is
 * much more powerful and guarantees that the resulting polygon has no
 * overlaps. Unforunately, it is extremely slow (in the 10s of milliseconds)
 * and is unsuitable for calcuations at framerate.
 */
class Path2 {
#pragma mark Values
public:
    /** The vector of vertices in this path */
    std::vector<Vec2> vertices;
    /** The corner points of this path (used for extrusion). */
    std::unordered_set<size_t> corners;
    /** Whether or not this path is closed */
    bool closed;

    /**
     * Creates an empty path.
     *
     * The created path has no vertices.  The bounding box is trivial.
     */
    Path2() : closed(false) { }
    
    /**
     * Creates a path with the given vertices
     *
     * No vertices are marked are as corner vertices. The path will be open.
     *
     * @param vertices  The vector of vertices (as Vec2) in this path
     */
    Path2(const std::vector<Vec2>& vertices) : closed(false) { set(vertices); }
    
    /**
     * Creates a path with the given vertices
     *
     * No vertices are marked are as corner vertices.  The path will be open.
     *
     * @param vertices  The array of vertices in this path
     * @param vertsize  The number of elements to use from vertices
     */
    Path2(const Vec2* vertices,  size_t vertsize) : closed(false) {
        set(vertices, vertsize);
    }

    /**
     * Creates a copy of the given path.
     *
     * Both the vertices and the annotations are copied. No references to the
     * original path are kept.
     *
     * @param path  The path to copy
     */
    Path2(const Path2& path) { set(path); }

    /**
     * Creates a copy with the resource of the given path.
     *
     * As a move constructor, this will invalidate the original path
     *
     * @param path  The path to take from
     */
    Path2(Path2&& path) :
    vertices(std::move(path.vertices)),
    corners(std::move(path.corners)) {
        closed = path.closed;
    }

    /**
     * Creates a path for the given rectangle.
     *
     * The path will have four vertices, one for each corner of the rectangle.
     * It will be closed.
     *
     * @param rect  The rectangle to copy
     */
    Path2(const Rect rect)  : closed(true) { set(rect); }
    
    /**
     * Creates a path from the given JsonValue
     *
     * The JsonValue should either be an array of floats or an JSON object.
     * If it is an array of floats, then it interprets those floats as the
     * vertices. All points are corners and the path is closed.
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
     * If "closed" is missing, then the path is closed by default.
     *
     * @param data      The JSON object specifying the path
     */
    Path2(const std::shared_ptr<JsonValue>& data) { set(data); }
    
    /**
     * Deletes the given path, freeing all resources.
     */
    ~Path2() { }

#pragma mark -
#pragma mark Setters
    /**
     * Sets this path to be a copy of the given one.
     *
     * All of the contents are copied, so that this path does not hold any
     * references to elements of the other path.
     *
     * This method returns a reference to this path for chaining.
     *
     * @param other  The path to copy
     *
     * @return This path, returned for chaining
     */
    Path2& operator= (const Path2& other) { return set(other); }

    /**
     * Sets this path to be have the resources of the given one.
     *
     * This method returns a reference to this path for chaining.
     *
     * @param other  The path to take from
     *
     * @return This path, returned for chaining
     */
    Path2& operator=(Path2&& other) {
        vertices = std::move(other.vertices);
        corners  = std::move(other.corners);
        closed = other.closed;
        return *this;
    }
    
    /**
     * Sets this path to be a copy of the given rectangle.
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
    Path2& operator=(const Rect rect) { return set(rect); }

    /**
     * Sets this path from the data in the given JsonValue
     *
     * The JsonValue should either be an array of floats or an JSON object.
     * If it is an array of floats, then it interprets those floats as the
     * vertices. All points are corners and the path is closed.
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
     * If "closed" is missing, then the path is closed by default.
     *
     * @param data      The JSON object specifying the path
     *
     * @return This path, returned for chaining
     */
    Path2& operator=(const std::shared_ptr<JsonValue>& data) { return set(data); }
    
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
    Path2& set(const std::vector<Vec2>& vertices);

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
    Path2& set(const Vec2* vertices, size_t vertsize);
    
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
    Path2& set(const Path2& path);
    
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
    Path2& set(const Rect rect);
    
    /**
     * Sets this path from the data in the given JsonValue
     *
     * The JsonValue should either be an array of floats or an JSON object.
     * If it is an array of floats, then it interprets those floats as the
     * vertices. All points are corners and the path is closed.
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
     * If "closed" is missing, then the path is closed by default.
     *
     * @param data      The JSON object specifying the path
     *
     * @return This path, returned for chaining
     */
    Path2& set(const std::shared_ptr<JsonValue>& data);
    
    /**
     * Clears the contents of this path
     *
     * @return This path, returned for chaining
     */
    Path2& clear();

#pragma mark -
#pragma mark Path Attributes
    /**
     * Returns true if the path is empty.
     *
     * @return true if the path is empty.
     */
    bool empty() const { return vertices.empty(); }

    /**
     * Returns the number of vertices in a path.
     *
     * @return the number of vertices in a path.
     */
    size_t size() const { return vertices.size(); }

    /**
     * Returns whether the path is closed.
     *
     * @return whether the path is closed.
     */
    bool isClosed() const { return closed; }

    /**
     * Returns a reference to the point at the given index.
     *
     * This accessor will allow you to change the (singular) point. It is
     * intended to allow minor distortions to the path without changing
     * the underlying geometry.
     *
     * @param index  The path index
     *
     * @return a reference to the point at the given index.
     */
    Vec2& at(size_t index) { return vertices.at(index); }
    
    /**
     * Returns a reference to the point at the given index.
     *
     * This accessor will allow you to change the (singular) point. It is
     * intended to allow minor distortions to the path without changing
     * the underlying geometry.
     *
     * @param index  The path index
     *
     * @return a reference to the point at the given index.
     */
    const Vec2& at(size_t index) const { return vertices.at(index); }

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
    bool isCorner(size_t index) const;

    /**
     * Returns the list of vertices
     *
     * This accessor will not permit any changes to the vertex array.  To change
     * the array, you must change the path via a set() method.
     *
     * @return a reference to the vertex array
     */
    const std::vector<Vec2>& getVertices() const { return vertices; }
        
    /**
     * Returns the bounding box for the path
     *
     * The bounding box is the minimal rectangle that contains all of the vertices in
     * this path.  This method will recompute the bounds and is hence O(n).
     *
     * @return the bounding box for the path
     */
    const Rect getBounds() const;
    
    /**
     * Returns a list of vertex indices representing this path.
     *
     * The indices are intended to be used in a drawing mesh to
     * display this path. The number of indices will be a multiple
     * of two.
     *
     * @return a list of vertex indices for using in a path mesh.
     */
    std::vector<Uint32> getIndices() const;

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
    size_t getIndices(std::vector<Uint32>& buffer) const;

#pragma mark -
#pragma mark Path Modification
    /**
     * Returns the former end point in the path, after removing it
     *
     * If this path is empty, this will return the zero vector.
     *
     * @return the former end point in the path
     */
    Vec2 pop();
    
    /**
     * Adds a point to the end of this path
     *
     * @param point     The point to add
     * @param corner    Whether this point is a corner
     */
    void push(Vec2 point, bool corner=false);

    /**
     * Adds a point to the end of this path
     *
     * @param x         The x-coordinate to add
     * @param y         The y-coordinate to add
     * @param corner    Whether this point is a corner
     */
    void push(float x, float y, bool corner=false);

    /**
     * Returns the former point at the given index, after removing it
     *
     * If this path is empty, this will return the zero vector.
     *
     * @return the former point at the given index
     */
    Vec2 remove(size_t index);
    
    /**
     * Adds a point at the given index
     *
     * @param index     The index to add the point
     * @param point     The point to add
     * @param corner    Whether this point is a corner
     */
    void add(size_t index, Vec2 point, bool corner=false);

    /**
     * Adds a point at the given index
     *
     * @param index     The index to add the point
     * @param x         The x-coordinate to add
     * @param y         The y-coordinate to add
     * @param corner    Whether this point is a corner
     */
    void add(size_t index, float x, float y, bool corner=false);
    
    /**
     * Allocates space in this path for the given number of points.
     *
     * This method can help performance when a path is being constructed
     * piecemeal.
     *
     * @param size  The number of spots allocated for future points.
     */
    void reserve(size_t size);

#pragma mark -
#pragma mark Geometry Methods
    /**
     * Returns the set of points forming the convex hull of this path.
     *
     * The returned set of points is guaranteed to be a counter-clockwise traversal
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
     * @return the set of points forming the convex hull of this polygon.
     */
    std::vector<Uint32> convexHull() const;
    
    /**
     * Returns the set of points forming the convex hull of the given points.
     *
     * The returned set of points is guaranteed to be a counter-clockwise traversal
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
     * @return the set of points forming the convex hull of the given points.
     */
    static std::vector<Uint32> convexHull(const std::vector<Vec2>& vertices);
    
    /**
     * Returns true if the interior of this path contains the given point.
     *
     * This mehtod returns false if the path is open.  Otherwise, it uses an even-odd 
     * crossing rule to determine containment. Containment is not strict. Points on the 
     * boundary are contained within this polygon.
     *
     * @param  point    The point to test
     *
     * @return true if this path contains the given point.
     */
    bool contains(Vec2 point) const {
        return contains(point.x,point.y);
    }
    
    /**
     * Returns true if the interior of this path contains the given point.
     *
     * This mehtod returns false if the path is open.  Otherwise, it uses an even-odd 
     * crossing rule to determine containment. Containment is not strict. Points on the 
     * boundary are contained within this polygon.
     *
     * @param x         The x-coordinate to test
     * @param y         The y-coordinate to test
     *
     * @return true if this path contains the given point.
     */
    bool contains(float x, float y) const;
    
    /**
     * Returns true if the given point is on the path.
     *
     * This method returns true if the point is within margin of error of a
     * line segment.
     *
     * @param point The point to check
     * @param err   The distance tolerance
     *
     * @return true if the given point is on the path.
     */
    bool incident(Vec2 point, float err=CU_MATH_EPSILON) const {
        return incident(point.x,point.y,err);
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
    bool incident(float x, float y, float err=CU_MATH_EPSILON) const;

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
    Uint32 leftTurns() const;
    
	/**
	 * Returns true if this path defines a convex shape.
	 *
	 * This method returns false if the path is open.
	 *
	 * @return true if this path defines a convex shape.
	 */
	bool isConvex() const;

    
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
    float area() const;
    
    /**
     * Returns -1, 0, or 1 indicating the path orientation.
     *
     * If the method returns -1, this is a counter-clockwise path. If 1, it
     * is a clockwise path.  If 0, that means it is undefined.  The
     * orientation can be undefined if all the points are colinear.
     *
     * @return -1, 0, or 1 indicating the path orientation.
     */
    int orientation() const;

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
    static int orientation(const Vec2& a, const Vec2& b, const Vec2& c);
    
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
    static int orientation(const std::vector<Vec2>& path);

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
    static int orientation(const Vec2* path, size_t size);

    /**
     * Reverses the orientation of this path in place
     *
     * The path will have all of its vertices in the reverse order from the
     * original. This path will not be affected.
     *
     * @return This path, returned for chaining
     */
    Path2& reverse();

    /**
     * Returns a path with the reverse orientation of this one.
     *
     * The path will have all of its vertices in the reverse order from the
     * original. This path will not be affected.
     *
     * @return a path with the reverse orientation of this one.
     */
    Path2 reversed() const;
    
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
    Path2& operator*=(float scale);
    
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
    Path2& operator*=(const Vec2 scale);

    /**
     * Transforms all of the vertices of this path.
     *
     * @param transform The affine transform
     *
     * @return This path with the vertices transformed
     */
    Path2& operator*=(const Affine2& transform);
    
    /**
     * Transforms all of the vertices of this path.
     *
     * The vertices are transformed as 3d points. The z-value is 0.
     *
     * @param transform The transform matrix
     *
     * @return This path with the vertices transformed
     */
    Path2& operator*=(const Mat4& transform);
    
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
    Path2& operator/=(float scale);
    
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
    Path2& operator/=(const Vec2 scale);
    
    /**
     * Translates all of the vertices of this path.
     *
     * @param offset The translation amount
     *
     * @return This path, translated.
     */
    Path2& operator+=(const Vec2 offset);
    
    /**
     * Translates all of the vertices of this path.
     *
     * @param offset The inverse of the translation amount
     *
     * @return This path, translated
     */
    Path2& operator-=(const Vec2 offset);
    
    /**
     * Returns a new path by scaling the vertices uniformly.
     *
     * The vertices are scaled from the origin of the coordinate space. This
     * means that if the origin is not path of this path, then the path will
     * be effectively translated by the scaling.
     *
     * Note: This method does not modify the path.
     *
     * @param scale The uniform scaling factor
     *
     * @return The scaled path
     */
    Path2 operator*(float scale) const { return Path2(*this) *= scale; }
    
    /**
     * Returns a new path by scaling the vertices non-uniformly.
     *
     * The vertices are scaled from the origin of the coordinate space. This
     * means that if the origin is not path of this path, then the path will
     * be effectively translated by the scaling.
     *
     * Note: This method does not modify the path.
     *
     * @param scale The non-uniform scaling factor
     *
     * @return The scaled path
     */
    Path2 operator*(const Vec2 scale) const { return Path2(*this) *= scale; }
    
    /**
     * Returns a new path by transforming all of the vertices of this path.
     *
     * Note: This method does not modify the path.
     *
     * @param transform The affine transform
     *
     * @return The transformed path
     */
    Path2 operator*(const Affine2& transform) const { return Path2(*this) *= transform; }

    /**
     * Returns a new path by transforming all of the vertices of this path.
     *
     * The vertices are transformed as 3d points. The z-value is 0.
     *
     * Note: This method does not modify the path.
     *
     * @param transform The transform matrix
     *
     * @return The transformed path
     */
    Path2 operator*(const Mat4& transform) const { return Path2(*this) *= transform; }
    
    /**
     * Returns a new path by scaling the vertices uniformly.
     *
     * The vertices are scaled from the origin of the coordinate space. This
     * means that if the origin is not path of this path, then the path will
     * be effectively translated by the scaling.
     *
     * Note: This method does not modify the path.
     *
     * @param scale The inverse of the uniform scaling factor
     *
     * @return The scaled path
     */
    Path2 operator/(float scale) const { return Path2(*this) /= scale; }
    
    /**
     * Returns a new path by scaling the vertices non-uniformly.
     *
     * The vertices are scaled from the origin of the coordinate space. This
     * means that if the origin is not path of this path, then the path will
     * be effectively translated by the scaling.
     *
     * Note: This method does not modify the path.
     *
     * @param scale The inverse of the non-uniform scaling factor
     *
     * @return The scaled path
     */
    Path2 operator/(const Vec2 scale) const { return Path2(*this) /= scale; }
    
    /**
     * Returns a new path by translating the vertices
     *
     * Note: This method does not modify the path.
     *
     * @param offset The translation amount
     *
     * @return The translated path
     */
    Path2 operator+(const Vec2 offset) const { return Path2(*this) += offset; }
    
    /**
     * Returns a new path by translating the vertices
     *
     * Note: This method does not modify the path.
     *
     * @param offset The inverse of the translation amount
     *
     * @return The translated path
     */
    Path2 operator-(const Vec2 offset) { return Path2(*this) -= offset; }
    
    /**
     * Returns a new path by scaling the vertices uniformly.
     *
     * The vertices are scaled from the origin of the coordinate space. This
     * means that if the origin is not path of this path, then the path will
     * be effectively translated by the scaling.
     *
     * @param scale     The uniform scaling factor
     * @param path      The path to scale
     *
     * @return The scaled path
     */
    friend Path2 operator*(float scale, const Path2& path) { return path*scale; }
    
    /**
     * Returns a new path by scaling the vertices non-uniformly.
     *
     * The vertices are scaled from the origin of the coordinate space. This
     * means that if the origin is not path of this path, then the path will
     * be effectively translated by the scaling.
     *
     * @param scale     The non-uniform scaling factor
     * @param path      The path to scale
     *
     * @return The scaled path
     */
    friend Path2 operator*(const Vec2 scale, const Path2& path) { return path*scale; }

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
    Path2& operator+=(const Path2& extra);
    
    /**
     * Returns a new path by appending extra to the end of this path
     *
     * The vertices are appended in order to the end of the path.  If
     * the original path was closed, the copy is open (regardless of
     * whether or not extra is closed)
     *
     * Note: This method does not modify the original path.
     *
     * @param extra The path to add
     *
     * @return The extended path
     */
    Path2 operator+(const Path2& extra) const { return Path2(*this) += extra; }
    
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
    Path2 slice(size_t start, size_t end) const;

    /**
     * Returns the slice of this mesh from the start index to the end.
     *
     * The sliced mesh will use the indices from start to the end. It will
     * include the vertices referenced by those indices, and only those
     * vertices. The resulting path is open.
     *
     * @param start The start index
     *
     * @return the slice of this mesh between start and end.
     */
    Path2 sliceFrom(size_t start) const  {
        return slice(start,vertices.size());
    }

    /**
     * Returns the slice of this mesh from the begining to end.
     *
     * The sliced mesh will use the indices up to  (but not including) end.
     * It will include the vertices referenced by those indices, and only
     * those vertices. The resulting path is open.
     *
     * @param end   The end index
     *
     * @return the slice of this mesh between start and end.
     */
    Path2 sliceTo(size_t end) const  {
        return slice(0,end);
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
     * @return a string representation of this path for debugging purposes.
     */
    std::string toString(bool verbose = false) const;
    
    /** Cast from Path2 to a string. */
    operator std::string() const { return toString(); }
    
    /** Cast from Path2 to a Rect. */
    operator Rect() const;
    
#pragma mark -
#pragma mark Internal Helper Methods
private:
    /**
     * Returns an index of a point on the convex hull
     *
     * The expact point returned is not guaranteed, but it is typically
     * with the least x and y values (whenever that is possible).
     *
     * @return an index of a point on the convex hull
     */
    size_t hullPoint() const;
    
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
    static size_t hullPoint(const std::vector<Vec2>& path);

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
    static size_t hullPoint(const Vec2* path, size_t size);
    
    /** Friend classes */
    friend class PathSmoother;
    friend class PathFactory;
    friend class SplinePather;
    friend class SimpleExtruder;
    friend class ComplexExtruder;
    friend class EarclipTriangulator;
    friend class DelaunayTriangulator;
};

}
#endif /* __CU_PATH2_H__ */
