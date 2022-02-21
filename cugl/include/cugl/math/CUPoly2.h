//
//  CUPoly2.h
//  Cornell University Game Library (CUGL)
//
//  This module provides a class that represents a simple polygon.  The purpose
//  of this class is to separate the geometry (and math) of a polygon mesh from
//  the rendering data of a pipeline. It is one of the most important classes for
//  2D game design in all of CUGL.
//
//  In previous iterations of CUGL, this class contained many features that are
//  now found in the Path2 class. However, as we added more computational geometry
//  features to the engine, this became untenable.
//
//  This class is intentionally (based on experience in previous semesters)
//  lightweight. There is no verification that indices are properly defined.
//  It is up to the user to verify and specify the components. If you need help
//  with triangulation or path extrusion, use one the the related factory classes.
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
//  Version: 7/22/21
//
#ifndef __CU_POLY2_H__
#define __CU_POLY2_H__

#include <vector>
#include <unordered_set>
#include <cugl/math/CUVec2.h>
#include <cugl/math/CURect.h>

namespace cugl {

// Forward references
class Path2;
class Mat4;
class Affine2;
class JsonValue;

/**
 * Class to represent a simple polygon.
 *
 * This class is intended to represent any polygon (including non-convex polygons).
 * that does not have self-interections (as these can cause serious problems with
 * the mathematics). Most polygons are simple, meaning that they have no holes.
 * However, this class does support complex polygons with holes, provided that
 * the polygon is not implicit and has an corresponding mesh.
 *
 * To define a mesh, the user should provide a set of indices which will be used
 * in rendering. These indices should represent a triangulation of the polygon.
 * However, this class performs no verification. It will not check that a mesh
 * is in proper form, nor will it search for holes or self-intersections. These
 * are the responsibility of the programmer.
 *
 * Generating indices for a Poly2 can be nontrivial.  While this class has
 * standard constructors for custom meshes, most Poly2 objects are created
 * through alternate means. In particular, there are several Poly2 factories
 * available. These factories allow for delegating index computation to a
 * separate thread, if it takes too long. These factories are as follows:
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
 * {@link PolyFactory}: This is a tool is used to generate several basic 
 * path shapes, such as rounded rectangles or arcs.  It also allows you
 * construct wireframe traversals of existing polygons.
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
class Poly2 {
#pragma mark Values
public:
    /** The vector of vertices in this polygon */
    std::vector<Vec2> vertices;
    /** The vector of indices in the triangulation */
    std::vector<Uint32> indices;
    
#pragma mark -
#pragma mark Constructors
    /**
     * Creates an empty polygon.
     *
     * The created polygon has no vertices and no triangulation. The bounding
     * box is trivial.
     */
    Poly2() { }
    
    /**
     * Creates a polygon with the given vertices
     *
     * The new polygon has no indices triangulating the vertices.
     *
     * @param vertices  The vector of vertices (as Vec2) in this polygon
     */
    Poly2(const std::vector<Vec2>& vertices) { set(vertices); }

    /**
     * Creates a polygon with the given vertices
     *
     * The new polygon has no indices triangulating the vertices.
     *
     * @param vertices  The vector of vertices (as Vec2) in this polygon
     * @param vertsize  The number of elements to use from vertices
     */
    Poly2(const Vec2* vertices, size_t vertsize) { set(vertices,vertsize); }
    
    /**
     * Creates a polygon with the given vertices and indices.
     *
     * A valid list of indices must only refer to vertices in the vertex array.
     * That is, the indices should all be non-negative, and each value should be
     * less than the number of vertices. In addition, the number of indices
     * should be a multiple of three, each group representing a counterclockwise
     * triangle of vertices.
     *
     * @param vertices  The vector of vertices (as Vec2) in this polygon
     * @param indices   The vector of indices for the rendering
     */
    Poly2(const std::vector<Vec2>& vertices, const std::vector<Uint32>& indices) {
        this->vertices = vertices;
        this->indices = indices;
    }

    /**
     * Creates a copy of the given polygon.
     *
     * Both the vertices and the indices are of the source are copied. No
     * references to the original polygon are kept.
     *
     * @param poly  The polygon to copy
     */
    Poly2(const Poly2& poly) { set(poly); }

    /**
     * Creates a copy with the resource of the given polygon.
     *
     * @param poly  The polygon to take from
     */
    Poly2(Poly2&& poly) : vertices(std::move(poly.vertices)), indices(std::move(poly.indices)) {}
    
    /**
     * Creates a polygon for the given rectangle.
     *
     * The polygon will have four vertices, one for each corner of the rectangle.
     * The indices will define two triangles on these vertices. This method is
     * faster than using one of the more heavy-weight triangulators.
     *
     * @param rect  The rectangle to copy
     */
    Poly2(const Rect rect) { set(rect); }
    
    /**
     * Creates a polygon from the given JsonValue
     *
     * The JsonValue should either be an array of floats or an JSON object.
     * If it is an array of floats, then it interprets those floats as the
     * vertices. The polygon indices will be generated using an
     * {@link EarclipTriangulator}.
     *
     * On the other hand, if it is a JSON object, it supports the following
     * attributes:
     *
     *     "vertices":      An (even) list of floats, representing the vertices
     *     "indices":       An intenger list of triangle indices (in multiples of 3)
     *     "triangulator":  One of 'monotone', 'earclip' or 'delaunay'
     *
     * All attributes are optional. If "vertices" are missing, the polygon will
     * be empty.  If both "indices" and "triangulator" are missing, the polygon
     * will have no indices. The "triangulator" choice will only be applied if
     * the "indices" are missing.
     *
     * @param data      The JSON object specifying the polygon
     */
    Poly2(const std::shared_ptr<JsonValue>& data) { set(data); }
    
    /**
     * Deletes the given polygon, freeing all resources.
     */
    ~Poly2() { }

    
#pragma mark -
#pragma mark Setters
    /**
     * Sets this polygon to be a copy of the given one.
     *
     * All of the contents are copied, so that this polygon does not hold any
     * references to elements of the other polygon. This method returns
     * a reference to this polygon for chaining.
     *
     * @param other  The polygon to copy
     *
     * @return This polygon, returned for chaining
     */
    Poly2& operator=(const Poly2& other) { return set(other); }

    /**
     * Sets this polygon to be have the resources of the given one.
     *
     * @param other  The polygon to take from
     *
     * @return This polygon, returned for chaining
     */
    Poly2& operator=(Poly2&& other) {
        vertices = std::move(other.vertices);
        indices  = std::move(other.indices);
        return *this;
    }
    
    /**
     * Sets this polygon to be a copy of the given rectangle.
     *
     * The polygon will have four vertices, one for each corner of the rectangle.
     * The indices will define two triangles on these vertices. This method is
     * faster than using one of the more heavy-weight triangulators.
     *
     * @param rect  The rectangle to copy
     *
     * @return This polygon, returned for chaining
     */
    Poly2& operator=(const Rect rect) { return set(rect); }

    /**
     * Sets this polygon from the data in the given JsonValue
     *
     * The JsonValue should either be an array of floats or an JSON object.
     * If it is an array of floats, then it interprets those floats as the
     * vertices. The polygon indices will be generated using an
     * {@link EarclipTriangulator}.
     *
     * On the other hand, if it is a JSON object, it supports the following
     * attributes:
     *
     *     "vertices":      An (even) list of floats, representing the vertices
     *     "indices":       An intenger list of triangle indices (in multiples of 3)
     *     "triangulator":  One of 'monotone', 'earclip' or 'delaunay'
     *
     * All attributes are optional. If "vertices" are missing, the polygon will
     * be empty.  If both "indices" and "triangulator" are missing, the polygon
     * will have no indices. The "triangulator" choice will only be applied if
     * the "indices" are missing.
     *
     * @param data      The JSON object specifying the polygon
     *
     * @return This polygon, returned for chaining
     */
    Poly2& operator=(const std::shared_ptr<JsonValue>& data) { return set(data); }
    
    /**
     * Sets the polygon to have the given vertices
     *
     * The resulting polygon has no indices triangulating the vertices.
     *
     * This method returns a reference to this polygon for chaining.
     *
     * @param vertices  The vector of vertices (as Vec2) in this polygon
     *
     * @return This polygon, returned for chaining
     */
    Poly2& set(const std::vector<Vec2>& vertices);
    
    /**
     * Sets the polygon to have the given vertices.
     *
     * The resulting polygon has no indices triangulating the vertices.
     *
     * This method returns a reference to this polygon for chaining.
     *
     * @param vertices  The array of vertices (as Vec2) in this polygon
     * @param vertsize  The number of elements to use from vertices
     *
     * @return This polygon, returned for chaining
     */
    Poly2& set(const Vec2* vertices, size_t vertsize);
    
    /**
     * Sets this polygon to be a copy of the given one.
     *
     * All of the contents are copied, so that this polygon does not hold any
     * references to elements of the other polygon.
     *
     * This method returns a reference to this polygon for chaining.
     *
     * @param poly  The polygon to copy
     *
     * @return This polygon, returned for chaining
     */
    Poly2& set(const Poly2& poly);
    
    /**
     * Sets the polygon to represent the given rectangle.
     *
     * The polygon will have four vertices, one for each corner of the rectangle.
     * The indices will define two triangles on these vertices. This method is
     * faster than using one of the more heavy-weight triangulators.
     *
     * @param rect  The rectangle to copy
     *
     * @return This polygon, returned for chaining
     */
    Poly2& set(const Rect rect);

    /**
     * Sets this polygon from the data in the given JsonValue
     *
     * The JsonValue should either be an array of floats or an JSON object.
     * If it is an array of floats, then it interprets those floats as the
     * vertices. The polygon indices will be generated using an
     * {@link EarclipTriangulator}.
     *
     * On the other hand, if it is a JSON object, it supports the following
     * attributes:
     *
     *     "vertices":      An (even) list of floats, representing the vertices
     *     "indices":       An intenger list of triangle indices (in multiples of 3)
     *     "triangulator":  One of 'monotone', 'earclip' or 'delaunay'
     *
     * All attributes are optional. If "vertices" are missing, the polygon will
     * be empty.  If both "indices" and "triangulator" are missing, the polygon
     * will have no indices. The "triangulator" choice will only be applied if
     * the "indices" are missing.
     *
     * @param data      The JSON object specifying the polygon
     *
     * @return This polygon, returned for chaining
     */
    Poly2& set(const std::shared_ptr<JsonValue>& data);
    
    /**
     * Sets the indices for this polygon to the ones given.
     *
     * A valid list of indices must only refer to vertices in the vertex array.
     * That is, the indices should all be non-negative, and each value should be
     * less than the number of vertices. In addition, the number of indices
     * should be a multiple of three, each group representing a counterclockwise
     * triangle of vertices.
     *
     * The provided indices are copied. The polygon does not retain a reference.
     *
     * @param indices   The vector of indices for the shape
     *
     * @return This polygon, returned for chaining
     */
    Poly2& setIndices(const std::vector<Uint32>& indices);
    
    /**
     * Sets the indices for this polygon to the ones given.
     *
     * A valid list of indices must only refer to vertices in the vertex array.
     * That is, the indices should all be non-negative, and each value should be
     * less than the number of vertices. In addition, the number of indices
     * should be a multiple of three, each group representing a counterclockwise
     * triangle of vertices.
     *
     * The provided indices are copied. The polygon does not retain a reference.
     *
     * @param indices   The array of indices for the rendering
     * @param indxsize  The number of elements to use for the indices
     *
     * @return This polygon, returned for chaining
     */
    Poly2& setIndices(const Uint32* indices, size_t indxsize);
    
    /**
     * Clears the contents of this polygon (both vertices and indices)
     *
     * @return This polygon, returned for chaining
     */
    Poly2& clear();

    
#pragma mark -
#pragma mark Polygon Attributes
    /**
     * Returns the number of vertices in the polygon.
     *
     * @return the number of vertices in the polygon.
     */
    size_t size() const { return vertices.size(); }

    /**
     * Returns the number of indices in the polygon.
     *
     * @return the number of indices in the polygon.
     */
    size_t indexSize() const { return indices.size(); }

    /**
     * Returns a reference to the attribute at the given index.
     *
     * This accessor will allow you to change the (singular) vertex. It is
     * intended to allow minor distortions to the polygon without changing
     * the underlying mesh.
     *
     * @param index  The attribute index
     *
     * @return a reference to the attribute at the given index.
     */
    Vec2& at(int index) { return vertices.at(index); }
    
    /**
     * Returns a reference to the attribute at the given index.
     *
     * This accessor will allow you to change the (singular) vertex. It is
     * intended to allow minor distortions to the polygon without changing
     * the underlying mesh.
     *
     * @param index  The attribute index
     *
     * @return a reference to the attribute at the given index.
     */
    const Vec2& at(int index) const { return vertices.at(index); }

    /**
     * Returns the list of vertices
     *
     * This accessor will not permit any changes to the vertex array.  To change
     * the array, you must change the polygon via a set() method.
     *
     * @return a reference to the vertex array
     */
    const std::vector<Vec2>& getVertices() const { return vertices; }

    /**
     * Returns a reference to list of indices.
     *
     * This accessor will not permit any changes to the index array.  To change
     * the array, you must change the polygon via a set() method.
     *
     * @return a reference to the vertex array
     */
    const std::vector<Uint32>& getIndices() const  { return indices; }

    /**
     * Returns the bounding box for the polygon
     *
     * The bounding box is the minimal rectangle that contains all of the vertices in
     * this polygon.  It is recomputed whenever the vertices are set.
     *
     * @return the bounding box for the polygon
     */
    const Rect getBounds() const;
    
    
#pragma mark -
#pragma mark Operators
    /**
     * Uniformly scales all of the vertices of this polygon.
     *
     * The vertices are scaled from the origin of the coordinate space. This
     * means that if the origin is not in the interior of this polygon, the 
     * polygon will be effectively translated by the scaling.
     *
     * @param scale The uniform scaling factor
     *
     * @return This polygon, scaled uniformly.
     */
    Poly2& operator*=(float scale);
    
    /**
     * Nonuniformly scales all of the vertices of this polygon.
     *
     * The vertices are scaled from the origin of the coordinate space. This
     * means that if the origin is not in the interior of this polygon, the
     * polygon will be effectively translated by the scaling.
     *
     * @param scale The non-uniform scaling factor
     *
     * @return This polygon, scaled non-uniformly.
     */
    Poly2& operator*=(const Vec2 scale);

    /**
     * Transforms all of the vertices of this polygon.
     *
     * @param transform The affine transform
     *
     * @return This polygon with the vertices transformed
     */
    Poly2& operator*=(const Affine2& transform);
    
    /**
     * Transforms all of the vertices of this polygon.
     *
     * The vertices are transformed as points. The z-value is 0.
     *
     * @param transform The transform matrix
     *
     * @return This polygon with the vertices transformed
     */
    Poly2& operator*=(const Mat4& transform);
    
    /**
     * Uniformly scales all of the vertices of this polygon.
     *
     * The vertices are scaled from the origin of the coordinate space. This
     * means that if the origin is not in the interior of this polygon, the
     * polygon will be effectively translated by the scaling.
     *
     * @param scale The inverse of the uniform scaling factor
     *
     * @return This polygon, scaled uniformly.
     */
    Poly2& operator/=(float scale);
    
    /**
     * Nonuniformly scales all of the vertices of this polygon.
     *
     * The vertices are scaled from the origin of the coordinate space. This
     * means that if the origin is not in the interior of this polygon, the
     * polygon will be effectively translated by the scaling.
     *
     * @param scale The inverse of the non-uniform scaling factor
     *
     * @return This polygon, scaled non-uniformly.
     */
    Poly2& operator/=(const Vec2 scale);
    
    /**
     * Uniformly translates all of the vertices of this polygon.
     *
     * @param offset The uniform translation amount
     *
     * @return This polygon, translated uniformly.
     */
    Poly2& operator+=(float offset);
    
    /**
     * Non-uniformly translates all of the vertices of this polygon.
     *
     * @param offset The non-uniform translation amount
     *
     * @return This polygon, translated non-uniformly.
     */
    Poly2& operator+=(const Vec2 offset);
    
    /**
     * Uniformly translates all of the vertices of this polygon.
     *
     * @param offset The inverse of the uniform translation amount
     *
     * @return This polygon, translated uniformly.
     */
    Poly2& operator-=(float offset);
    
    /**
     * Non-uniformly translates all of the vertices of this polygon.
     *
     * @param offset The inverse of the non-uniform translation amount
     *
     * @return This polygon, translated non-uniformly.
     */
    Poly2& operator-=(const Vec2 offset);
    
    /**
     * Returns a new polygon by scaling the vertices uniformly.
     *
     * The vertices are scaled from the origin of the coordinate space.  This
     * means that if the origin is not in the interior of this polygon, the
     * polygon will be effectively translated by the scaling.
     *
     * Note: This method does not modify the polygon.
     *
     * @param scale The uniform scaling factor
     *
     * @return The scaled polygon
     */
    Poly2 operator*(float scale) const { return Poly2(*this) *= scale; }
    
    /**
     * Returns a new polygon by scaling the vertices non-uniformly.
     *
     * The vertices are scaled from the origin of the coordinate space.  This
     * means that if the origin is not in the interior of this polygon, the
     * polygon will be effectively translated by the scaling.
     *
     * Note: This method does not modify the polygon.
     *
     * @param scale The non-uniform scaling factor
     *
     * @return The scaled polygon
     */
    Poly2 operator*(const Vec2 scale) const { return Poly2(*this) *= scale; }

    /**
     * Returns a new polygon by transforming all of the vertices of this polygon.
     *
     * Note: This method does not modify the polygon.
     *
     * @param transform The affine transform
     *
     * @return The transformed polygon
     */
    Poly2 operator*(const Affine2& transform) const { return Poly2(*this) *= transform; }

    /**
     * Returns a new polygon by transforming all of the vertices of this polygon.
     *
     * The vertices are transformed as points. The z-value is 0.
     *
     * Note: This method does not modify the polygon.
     *
     * @param transform The transform matrix
     *
     * @return The transformed polygon
     */
    Poly2 operator*(const Mat4& transform) const { return Poly2(*this) *= transform; }
    
    /**
     * Returns a new polygon by scaling the vertices uniformly.
     *
     * The vertices are scaled from the origin of the coordinate space.  This
     * means that if the origin is not in the interior of this polygon, the
     * polygon will be effectively translated by the scaling.
     *
     * Note: This method does not modify the polygon.
     *
     * @param scale The inverse of the uniform scaling factor
     *
     * @return The scaled polygon
     */
    Poly2 operator/(float scale) const { return Poly2(*this) /= scale; }
    
    /**
     * Returns a new polygon by scaling the vertices non-uniformly.
     *
     * The vertices are scaled from the origin of the coordinate space.  This
     * means that if the origin is not in the interior of this polygon, the
     * polygon will be effectively translated by the scaling.
     *
     * Note: This method does not modify the polygon.
     *
     * @param scale The inverse of the non-uniform scaling factor
     *
     * @return The scaled polygon
     */
    Poly2 operator/(const Vec2 scale) const { return Poly2(*this) /= scale; }
    
    /**
     * Returns a new polygon by translating the vertices uniformly.
     *
     * Note: This method does not modify the polygon.
     *
     * @param offset The uniform translation amount
     *
     * @return The translated polygon
     */
    Poly2 operator+(float offset) const { return Poly2(*this) += offset; }
    
    /**
     * Returns a new polygon by translating the vertices non-uniformly.
     *
     * Note: This method does not modify the polygon.
     *
     * @param offset The non-uniform translation amount
     *
     * @return The translated polygon
     */
    Poly2 operator+(const Vec2 offset) const { return Poly2(*this) += offset; }
    
    /**
     * Returns a new polygon by translating the vertices uniformly.
     *
     * Note: This method does not modify the polygon.
     *
     * @param offset The inverse of the uniform translation amount
     *
     * @return The translated polygon
     */
    Poly2 operator-(float offset) { return Poly2(*this) -= offset; }
    
    /**
     * Returns a new polygon by translating the vertices non-uniformly.
     *
     * Note: This method does not modify the polygon.
     *
     * @param offset The inverse of the non-uniform translation amount
     *
     * @return The translated polygon
     */
    Poly2 operator-(const Vec2 offset) { return Poly2(*this) -= offset; }
    
    /**
     * Returns a new polygon by scaling the vertices uniformly.
     *
     * The vertices are scaled from the origin of the coordinate space.  This
     * means that if the origin is not in the interior of this polygon, the
     * polygon will be effectively translated by the scaling.
     *
     * @param scale The uniform scaling factor
     * @param poly 	The polygon to scale
     *
     * @return The scaled polygon
     */
    friend Poly2 operator*(float scale, const Poly2& poly) { return poly*scale; }
    
    /**
     * Returns a new polygon by scaling the vertices non-uniformly.
     *
     * The vertices are scaled from the origin of the coordinate space.  This
     * means that if the origin is not in the interior of this polygon, the
     * polygon will be effectively translated by the scaling.
     *
     * @param scale The non-uniform scaling factor
     * @param poly 	The polygon to scale
     *
     * @return The scaled polygon
     */
    friend Poly2 operator*(const Vec2 scale, const Poly2& poly) { return poly*scale; }

    
#pragma mark -
#pragma mark Geometry Methods
    /**
     * Returns the vertex indices forming the convex hull of this polygon.
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
    std::vector<Uint32> convexHull() const;
    
    /**
     * Returns true if this polygon contains the given point.
     *
     * Unlike {@link Path2}, this method does not use an even-odd rule. Instead,
     * it checks for containment within the associated triangle mesh.
     *
     * Containment is not strict. Points on the boundary are contained within
     * this polygon.
     *
     * @param  point    The point to test
     *
     * @return true if this polygon contains the given point.
     */
    bool contains(Vec2 point) const {
        return contains(point.x,point.y);
    }
    
    /**
     * Returns true if this polygon contains the given point.
     *
     * Unlike {@link Path2}, this method does not use an even-odd rule. Instead,
     * it checks for containment within the associated triangle mesh.
     *
     * Containment is not strict. Points on the boundary are contained within
     * this polygon.
     *
     * @param x         The x-coordinate to test
     * @param y         The y-coordinate to test
     *
     * @return true if this polygon contains the given point.
     */
    bool contains(float x, float y) const;
    
    /**
     * Returns true if the given point is on the boundary of this polygon.
     *
     * This method generates uses {@link #boundaries} to determine the boundaries.
     * It returns true if the point is within margin of error of a line segment
     * on one of the boundaries.
     *
     * @param point The point to check
     * @param err   The distance tolerance
     *
     * @return true if the given point is on the boundary of this polygon.
     */
    bool incident(Vec2 point, float err=CU_MATH_EPSILON) const {
        return incident(point.x,point.y,err);
    }

    /**
     * Returns true if the given point is on the boundary of this polygon.
     *
     * This method generates uses {@link #boundaries} to determine the boundaries.
     * It returns true if the point is within margin of error of a line segment
     * on one of the boundaries.
     *
     * @param x     The x-coordinate to test
     * @param y     The y-coordinate to test
     * @param err   The distance tolerance
     *
     * @return true if the given point is on the boundary of this polygon.
     */
    bool incident(float x, float y, float err=CU_MATH_EPSILON) const;

    /**
     * Returns the set of indices that are on a boundary of this polygon
     *
     * This method can identify the outer hull using the graph properties of the
     * triangle mesh. An internal node if the number of neighbors is the same as
     * the number of attached triangles. An index that is not internal is external.
     *
     * Unlike {@link #boundaries}, this method does not order the boundary indices
     * or decompose them into connected components.
     *
     * @return the set of indices that are on a boundary of this polygon
     */
    std::unordered_set<Uint32> exterior() const;

    /**
     * Stores the set of indices that are on a boundary of this polygon
     *
     * This method can identify the outer hull using the graph properties of the
     * triangle mesh. An internal node if the number of neighbors is the same as
     * the number of attached triangles. An index that is not internal is external.
     *
     * Unlike {@link #boundaries}, this method does not order the boundary indices
     * or decompose them into connected components.
     *
     * @param buffer    A buffer to store the indices on the boundary
     *
     * @return the number of elements added to the buffer
     */
    size_t exterior(std::unordered_set<Uint32>& buffer) const;

    /**
     * Returns the connected boundary components for this polygon.
     *
     * This method allows us to reconstruct the exterior boundary of a solid
     * shape, or to compose a pathwise connected curve into components.
     *
     * This method detriangulates the polygon mesh, returning the outer hull,
     * discarding any interior points. This hull need not be convex. If the
     * mesh represents a simple polygon, only one boundary will be returned.
     * If the mesh is not continuous, the outer array will contain the boundary
     * of each disjoint polygon. If the mesh has holes, each hole will be returned
     * as a separate boundary. There is no guarantee on the order of boundaries
     * returned.
     *
     * @return the connected boundary components for this polygon.
     */
    std::vector<std::vector<Uint32>> boundaries() const;
    
    /**
     * Stores the connected boundary components for this polygon.
     *
     * This method allows us to reconstruct the exterior boundary of a solid
     * shape, or to compose a pathwise connected curve into components.
     *
     * This method detriangulates the polygon mesh, returning the outer hull,
     * discarding any interior points. This hull need not be convex. If the
     * mesh represents a simple polygon, only one boundary will be returned.
     * If the mesh is not continuous, the outer array will contain the boundary
     * of each disjoint polygon. If the mesh has holes, each hole will be returned
     * as a separate boundary. There is no guarantee on the order of boundaries
     * returned.
     *
     * @param buffer    A buffer to connected boundary components
     *
     * @return the number of elements added to the buffer
     */
    size_t boundaries(std::vector<std::vector<Uint32>>& buffer) const;
    
#pragma mark -
#pragma mark Conversion Methods
    /**
     * Returns a string representation of this polygon for debugging purposes.
     *
     * If verbose is true, the string will include class information.  This
     * allows us to unambiguously identify the class.
     *
     * @param verbose Whether to include class information
     *
     * @return a string representation of this polygon for debuggging purposes.
     */
    std::string toString(bool verbose = false) const;
    
    /** Cast from Poly to a string. */
    operator std::string() const { return toString(); }
    
	/** Cast from Poly2 to a Rect. */
    operator Rect() const;

    
#pragma mark -
#pragma mark Internal Helper Methods
private:
    /**
     * Returns the barycentric coordinates for a point relative to a triangle.
     *
     * The triangle is identified by the given index.  For index ii, it is the
     * triangle defined by indices 3*ii, 3*ii+1, and 3*ii+2.
     *
     * @param point The point to convert
     * @param index The triangle index in this polygon
     *
     * @return the barycentric coordinates for a point relative to a triangle.
     */
    Vec3 getBarycentric(Vec2 point, Uint32 index) const;

};

}
#endif /* __CU_POLY2_H__ */
