//
//  CUPoly2.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a class that represents a simple polygon.  The purpose
//  of this class is to separate the geometry (and math) of a polygon from the
//  rendering data of a pipeline. It is one of the most important classes for
//  2D game design in all of CUGL.
//
//  Polygons all have a corresponding geometry. If they are implicit, they
//  cannot be drawn, but can be used for geometric calculation.  Otherwise,
//  the polygon has a mesh defined by a set of vertices. This class is
//  intentionally (based on experience in previous semesters) lightweight.
//  There is no verification that indices are properly defined.  It is up to
//  the user to verify and specify the components. If you need help with
//  triangulation or path extrusion, use one the the related factory classes.
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
//  Version: 1/22/21

#include <algorithm>
#include <vector>
#include <stack>
#include <sstream>
#include <cmath>
#include <iterator>
#include <unordered_set>
#include <unordered_map>
#include <cugl/util/CUDebug.h>
#include <cugl/util/CUStrings.h>
#include <cugl/math/CUPoly2.h>
#include <cugl/math/CUPath2.h>
#include <cugl/math/CURect.h>
#include <cugl/math/CUMat4.h>
#include <cugl/math/CUAffine2.h>
#include <cugl/assets/CUJsonValue.h>
#include <cugl/math/polygon/CUEarclipTriangulator.h>
#include <cugl/math/polygon/CUDelaunayTriangulator.h>

using namespace std;
using namespace cugl;

#pragma mark Triangulation Support
template <typename T>
void triangulate(const std::vector<Vec2>& vertices, std::vector<Uint32>& indices) {
    T triangulator;
    triangulator.set(vertices);
    triangulator.calculate();
    triangulator.getTriangulation(indices);
}

template <typename T>
void triangulate(const std::vector<Vec2>& vertices, const std::vector<Uint32>& holes,
                 std::vector<Uint32>& indices) {
    if (holes.size() == 0) {
        T triangulator;
        triangulator.set(vertices);
        triangulator.calculate();
        triangulator.getTriangulation(indices);
    } else {
        T triangulator;
        triangulator.set(vertices.data(),holes[0]);
        for(size_t ii = 0; ii < holes.size(); ii++) {
            size_t start = holes[ii];
            size_t end = ii == holes.size()-1 ? vertices.size() : holes[ii+1];
            triangulator.addHole(vertices.data()+start,end-start);
        }
        triangulator.calculate();
        triangulator.getTriangulation(indices);
    }
}

#pragma mark -
#pragma mark Detriangulation Support
/**
 * This class is a triangle in a mesh, interpreted as a node in tree decomposition
 *
 * Two triangles are adjacent in this decomposition if they share an edge. This
 * dual graph is not connected, though we do track direction when we are recursively
 * following a path.
 *
 * The elements in a tree node are ordered in ascending order, so that we can
 * uniquely identify a tree node from its contents.
 */
class Poly2TreeNode {
public:
    /** Hash function for a tree node */
    struct TreeHasher {
        /**
         * Returns the hash code for the given Poly2TreeNode
         *
         * @param node  The TreeNode to hash
         *
         * @return the hash code for the given Poly2TreeNode
         */
        std::size_t operator()(Poly2TreeNode* node) const;
    };
    
    /** The elements of this triangle */
    int elements[3];
    /** The adjacent neighbors to this node */
    std::unordered_set<Poly2TreeNode*,TreeHasher> neighbors;
        
    /** The node pointing to this one in a traversal */
    Poly2TreeNode* previous;
        
    /**
     * Creates a Poly2TreeNode from the given three elements.
     *
     * @param a The first element
     * @param b The second element
     * @param c The third element
     */
    Poly2TreeNode(int a, int b, int c);

    /**
     * Delets a Poly2TreeNode, destroying all resources.
     */
    ~Poly2TreeNode();

    /**
     * Returns true if o is a Poly2TreeNode equal to this one
     *
     * Since TreeNode objects sort their contents, o must have its elements in
     * the same order as this on.
     *
     * @return true if o is a Poly2TreeNode equal to this one
     */
    bool operator==(const Poly2TreeNode& o) const;
        
    /**
     * Returns a string representation of this tree node
     *
     * @return a string representation of this tree node
     */
    std::string toString() const;
        
    /**
     * Returns a string representation of a tree node with the given elements
     *
     * This method allows us to get the string of a tree node (from its contents)
     * without actually having to construct the tree node itself.  This is useful
     * for hashtable lookups.
     *
     * @param a The first element
     * @param b The second element
     * @param c The third element
     *
     * @return a string representation of a tree node with the given elements
     */
    static std::string toString(int a, int b, int c);
        
    /**
     * Returns true if x is an element in this node
     *
     * @param x The element to check
     *
     * @return true if x is an element in this node
     */
    bool contains(int x) const;
        
    /**
     * Returns true if node is adjacent to this one.
     *
     * A node is adjacent if it shares exactly one side.
     *
     * @param node  The node to check
     *
     * @return true if node is adjacent to this one.
     */
    bool adjacent(const Poly2TreeNode* node) const;

    /**
     * Returns a boundary index from the node, not in inuse
     *
     * A boundary index is either one that does not appear in any
     * of its neighbors (so this is an ear in a triangulation) or
     * only appears in one neighbor (so this is the either the first
     * or last triangle with this index in a normal traversal).
     *
     * If no boundary index can be found, or they are all already
     * in inuse, this method returns -1.
     *
     * @param inuse The indices to exclude from the search
     *
     * @return a boundary index from the node, not in inuse
     */
    Uint32 pick(const std::unordered_set<Uint32>& inuse) const;
        
    /**
     * Returns the opposite transition point for the given index.
     *
     * A transition point is a node that contains index and for which index is
     * a boundary value (either it has no neighbors with the same index or only
     * one neighbor).  It represents the first and/or last triangle with this
     * index in a normal traversal.
     *
     * If there is only one triangle with this index, this method returns this
     * node.  Otherwise, if this node corresponds to the first triangle, it
     * returns the last, and vice versa.  By following indices, we create a
     * traversal that can find an exterior boundary.
     *
     * @param index The index defining the traversal
     *
     * @return the opposite transition point for the given index.
     */
    Poly2TreeNode* follow(Uint32 index);

    /**
     * Returns the opposite transition point for the given index.
     *
     * This method is the recursive helper for {@link #follow}. It uses
     * the internal previous attribute to track direction.
     *
     * @param index The index defining the traversal
     *
     * @return the opposite transition point for the given index.
     */
    Poly2TreeNode* crawl(int index);
};

/**
 * Returns the hash code for the given Poly2TreeNode
 *
 * @param node  The TreeNode to hash
 *
 * @return the hash code for the given Poly2TreeNode
 */
std::size_t Poly2TreeNode::TreeHasher::operator()(Poly2TreeNode* node) const {
    size_t one = std::hash<int>()((node->elements)[0]) & 0xff;
    size_t two = std::hash<int>()((node->elements)[1]) & 0xff;
    size_t tre = std::hash<int>()((node->elements)[2]) & 0xff;
    return (one | (two << 8) | (tre << 16));
}

/**
 * Creates a Poly2TreeNode from the given three elements.
 *
 * @param a The first element
 * @param b The second element
 * @param c The third element
 */
Poly2TreeNode::Poly2TreeNode(int a, int b, int c) {
    elements[0] = std::min( a, std::min( b, c ) );
    elements[2] = std::max( a, std::max( b, c ) );
    elements[1] = a;
    if (elements[0] == elements[1] || elements[2] == elements[1]) {
        elements[1] = b;
    }
    if (elements[0] == elements[1] || elements[2] == elements[1]) {
        elements[1] = c;
    }
    CUAssertLog(elements[0] < elements[1] && elements[1] < elements[2],
                "The triangle [%d, %d, %d] is degenerate.", a, b, c );
}

/**
 * Delets a TreeNode, destroying all resources.
 */
Poly2TreeNode::~Poly2TreeNode() {
    // TreeNodes own nothing
    neighbors.clear();
    previous = nullptr;
}
            
/**
 * Returns true if o is a TreeNode equal to this one
 *
 * Since TreeNode objects sort their contents, o must have its elements in
 * the same order as this on.
 *
 * @return true if o is a TreeNode equal to this one
 */
bool Poly2TreeNode::operator==(const Poly2TreeNode& o) const {
    return elements[0] == o.elements[0] && elements[1] == o.elements[1] && elements[2] == o.elements[2];
}
            
/**
 * Returns a string representation of this tree node
 *
 * @return a string representation of this tree node
 */
std::string Poly2TreeNode::toString() const {
    char buff[100];
    snprintf(buff, sizeof(buff), "[%d,%d,%d]", elements[0], elements[1], elements[2]);
    std::string result = buff;
    return result;
}

/**
 * Returns a string representation of a tree node with the given elements
 *
 * This method allows us to get the string of a tree node (from its contents)
 * without actually having to construct the tree node itself.  This is useful
 * for hashtable lookups.
 *
 * @param a The first element
 * @param b The second element
 * @param c The third element
 *
 * @return a string representation of a tree node with the given elements
 */
std::string Poly2TreeNode::toString(int a, int b, int c) {
    int indx1 = std::min( a, std::min( b, c ) );
    int indx3 = std::max( a, std::max( b, c ) );
    int indx2 = a;
    if (indx1 == indx2 || indx3 == indx2) {
        indx2 = b;
    }
    if (indx1 == indx2 || indx3 == indx2) {
        indx2 = c;
    }
    char buff[100];
    snprintf(buff, sizeof(buff), "[%d,%d,%d]", indx1, indx2, indx3);
    std::string result = buff;
    return result;
}
            
/**
 * Returns true if x is an element in this node
 *
 * @param x The element to check
 *
 * @return true if x is an element in this node
 */
bool Poly2TreeNode::contains(int x) const {
    return x >= 0 && (elements[0] == x || elements[1] == x || elements[2] == x);
}
    
/**
 * Returns true if node is adjacent to this one.
 *
 * A node is adjacent if it shares exactly one side.
 *
 * @param node  The node to check
 *
 * @return true if node is adjacent to this one.
 */
bool Poly2TreeNode::adjacent(const Poly2TreeNode* node) const {
    int count = 0;
    for (int ii = 0; ii < 3; ii++) {
        count += contains( node->elements[ii] ) ? 1 : 0;
    }
    return count == 2;
}
    
/**
 * Returns a boundary index from the node, not in inuse
 *
 * A boundary index is either one that does not appear in any
 * of its neighbors (so this is an ear in a triangulation) or
 * only appears in one neighbor (so this is the either the first
 * or last triangle with this index in a normal traversal).
 *
 * If no boundary index can be found, or they are all already
 * in inuse, this method returns -1.
 *
 * @param inuse The indices to exclude from the search
 *
 * @return a boundary index from the node, not in inuse
 */
Uint32 Poly2TreeNode::pick(const std::unordered_set<Uint32>& inuse) const {
    int count[3];
    count[0] = 0;
    count[1] = 0;
    count[2] = 0;
    for (auto it = neighbors.begin(); it != neighbors.end(); ++it) {
        Poly2TreeNode* node = *it;
        for (int ii = 0; ii < 3; ii++) {
            if (node->contains( elements[ii] )) {
                    count[ii]++;
            }
        }
    }
    
    int ptr = -1;
    for (int ii = 0; ii < 3; ii++) {
        if ((count[ii] == 0 || count[ii] == 1)) {
            if (inuse.find(elements[ii]) == inuse.end()) {
                if (ptr == -1) {
                    ptr = ii;
                } else if (count[ii] < count[ptr]) {
                    ptr = ii;
                }
            }
        }
    }
    
    return ptr != -1 ? elements[ptr] : -1;
}
            
/**
 * Returns the opposite transition point for the given index.
 *
 * A transition point is a node that contains index and for which index is
 * a boundary value (either it has no neighbors with the same index or only
 * one neighbor).  It represents the first and/or last triangle with this
 * index in a normal traversal.
 *
 * If there is only one triangle with this index, this method returns this
 * node.  Otherwise, if this node corresponds to the first triangle, it
 * returns the last, and vice versa.  By following indices, we create a
 * traversal that can find an exterior boundary.
 *
 * @param index The index defining the traversal
 *
 * @return the opposite transition point for the given index.
 */
Poly2TreeNode* Poly2TreeNode::follow(Uint32 index) {
    previous = nullptr;
    return crawl( index );
}

/**
 * Returns the opposite transition point for the given index.
 *
 * This method is the recursive helper for {@link #follow}. It uses
 * the internal previous attribute to track direction.
 *
 * @param index The index defining the traversal
 *
 * @return the opposite transition point for the given index.
 */
Poly2TreeNode* Poly2TreeNode::crawl(int index) {
    if (!contains(index)) {
        return nullptr;
    }

    Poly2TreeNode* next = nullptr;
    for (auto it = neighbors.begin(); it != neighbors.end(); ++it) {
        Poly2TreeNode* node = *it;
        if (node != previous && node->contains( index )) {
            next = node;
        }
    }

    if (next == nullptr) {
        return this;
    } else if (next->previous == this) {
        return nullptr;
    }
    next->previous = this;
    return next->crawl( index );
}

/**
 * Returns true if the given points are colinear (within margin of error)
 *
 * @param v     The first point
 * @param w     The second point
 * @param p     The third point
 * @param err   The margin of error
 */
static bool colinear(const Vec2& v, const Vec2& w, const Vec2& p, float err) {
    const float l2 = (w-v).lengthSquared();
    double distance = 0.0f;
    if (l2 == 0.0) {
        distance = p.distance(v);
    } else {
        const float t = std::max(0.0f, std::min(1.0f, (p - v).dot(w - v) / l2));
        Vec2 projection = v + t * (w - v);
        distance = p.distance(projection);
    }

    return (distance <= err);
}

#pragma mark -
#pragma mark Setters
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
Poly2& Poly2::set(const vector<Vec2>& vertices) {
    this->vertices = vertices;
    indices.clear();
    return *this;
}

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
Poly2& Poly2::set(const Vec2* vertices, size_t vertsize) {
    this->vertices.assign(vertices,vertices+vertsize);
    indices.clear();
    return *this;
}

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
Poly2& Poly2::set(const Poly2& poly) {
    vertices = poly.vertices;
    indices  = poly.indices;
    return *this;
}

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
Poly2& Poly2::set(const Rect rect) {
    vertices.clear();
    indices.clear();
    vertices.reserve(4);
    vertices.push_back(rect.origin);
    vertices.push_back(Vec2(rect.origin.x+rect.size.width, rect.origin.y));
    vertices.push_back(rect.origin+rect.size);
    vertices.push_back(Vec2(rect.origin.x, rect.origin.y+rect.size.height));
    
    indices.reserve(6);
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(3);
    return *this;
}

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
Poly2& Poly2::set(const std::shared_ptr<JsonValue>& data) {
    vertices.clear();
    indices.clear();
    if (data->isArray()) {
        JsonValue* poly = data.get();
        CUAssertLog(poly->size() % 2 == 0, "polygon data should be an even list of numbers");
        vertices.reserve(poly->size()/2);
        for(Uint32 ii = 0; ii < poly->size(); ii += 2) {
            Vec2 vert;
            vert.x = poly->get(ii  )->asFloat(0.0f);
            vert.y = poly->get(ii+1)->asFloat(0.0f);
            vertices.push_back(vert);
        }
        triangulate<EarclipTriangulator>(vertices, indices);
    } else if (data->has("vertices")) {
        std::vector<Uint32> holes;
        JsonValue* poly = data->get("vertices").get();
        CUAssertLog(poly->size() % 2 == 0, "'vertices' should be an even list of numbers");
        vertices.reserve(poly->size()/2);
        for(int ii = 0; ii < poly->size(); ii += 2) {
            Vec2 vert;
            vert.x = poly->get(ii  )->asFloat(0.0f);
            vert.y = poly->get(ii+1)->asFloat(0.0f);
            vertices.push_back(vert);
        }
        if (data->has("holes")) {
            JsonValue* holelist = data->get("holes").get();
            for(int ii = 0; ii < holelist->size(); ii++) {
                JsonValue* hole = holelist->get(ii).get();
                CUAssertLog(hole->size() % 2 == 0, "a hole should be an even list of numbers");
                holes.push_back((Uint32)vertices.size());
                vertices.reserve(vertices.size()+hole->size()/2);
                for(int jj = 0; jj < hole->size(); jj += 2) {
                    Vec2 vert;
                    vert.x = hole->get(jj  )->asFloat(0.0f);
                    vert.y = hole->get(jj+1)->asFloat(0.0f);
                    vertices.push_back(vert);
                }
            }

        }
        if (data->has("indices")) {
            JsonValue* tris = data->get("indices").get();
            indices.reserve(tris->size());
            for(Uint32 ii = 0; ii < tris->size(); ii++) {
                indices.push_back(tris->get(ii  )->asInt(0));
            }
        } else {
            std::string choice = data->getString("triangulator","earclip");
            if (choice == "monotone") {
                triangulate<EarclipTriangulator>(vertices, holes, indices);
            } else if (choice == "delaunay") {
                triangulate<DelaunayTriangulator>(vertices, holes, indices);
            } else {
                triangulate<EarclipTriangulator>(vertices, holes, indices);
            }
        }
    }
    return *this;
}


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
Poly2& Poly2::setIndices(const vector<Uint32>& indices) {
    this->indices = indices;
    return *this;
}

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
Poly2& Poly2::setIndices(const Uint32* indices, size_t indxsize) {
    this->indices.assign(indices, indices+indxsize);
    return *this;
}

/**
 * Clears the contents of this polygon (both vertices and indices)
 *
 * @return This polygon, returned for chaining
 */
Poly2& Poly2::clear() {
    vertices.clear();
    indices.clear();
    return *this;
}

#pragma mark -
#pragma mark Polygon Operations
/**
 * Uniformly scales all of the vertices of this polygon.
 *
 * The vertices are scaled from the origin of the coordinate space.  This
 * means that if the origin is not in the interior of this polygon, the
 * polygon will be effectively translated by the scaling.
 *
 * @param scale The uniform scaling factor
 *
 * @return This polygon, scaled uniformly.
 */
Poly2& Poly2::operator*=(float scale) {
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        *it *= scale;
    }
    return *this;
}

/**
 * Nonuniformly scales all of the vertices of this polygon.
 *
 * The vertices are scaled from the origin of the coordinate space.  This
 * means that if the origin is not in the interior of this polygon, the
 * polygon will be effectively translated by the scaling.
 *
 * @param scale The non-uniform scaling factor
 *
 * @return This polygon, scaled non-uniformly.
 */
Poly2& Poly2::operator*=(const Vec2 scale) {
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        it->x *= scale.x;
        it->y *= scale.y;
    }
    return *this;
}

/**
 * Transforms all of the vertices of this polygon.
 *
 * @param transform The affine transform
 *
 * @return This polygon with the vertices transformed
 */
Poly2& Poly2::operator*=(const Affine2& transform) {
    Vec2 tmp;
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        Affine2::transform(transform, *it, &tmp);
        *it = tmp;
    }
    return *this;
}

/**
 * Transforms all of the vertices of this polygon.
 *
 * The vertices are transformed as points. The z-value is 0.
 *
 * @param transform The transform matrix
 *
 * @return This polygon with the vertices transformed
 */
Poly2& Poly2::operator*=(const Mat4& transform) {
    Vec2 tmp;
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        Mat4::transform(transform, *it, &tmp);
        *it = tmp;
    }
    return *this;
}

/**
 * Uniformly scales all of the vertices of this polygon.
 *
 * The vertices are scaled from the origin of the coordinate space.  This
 * means that if the origin is not in the interior of this polygon, the
 * polygon will be effectively translated by the scaling.
 *
 * @param scale The inverse of the uniform scaling factor
 *
 * @return This polygon, scaled uniformly.
 */
Poly2& Poly2::operator/=(float scale) {
    CUAssertLog(scale != 0, "Division by 0");
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        it->x /= scale;
        it->y /= scale;
    }
    return *this;
}

/**
 * Nonuniformly scales all of the vertices of this polygon.
 *
 * The vertices are scaled from the origin of the coordinate space.  This
 * means that if the origin is not in the interior of this polygon, the
 * polygon will be effectively translated by the scaling.
 *
 * @param scale The inverse of the non-uniform scaling factor
 *
 * @return This polygon, scaled non-uniformly.
 */
Poly2& Poly2::operator/=(const Vec2 scale) {
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        it->x /= scale.x;
        it->y /= scale.y;
    }
    return *this;
}

/**
 * Uniformly translates all of the vertices of this polygon.
 *
 * @param offset The uniform translation amount
 *
 * @return This polygon, translated uniformly.
 */
Poly2& Poly2::operator+=(float offset) {
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        it->x += offset;
        it->y += offset;
    }
    return *this;
}

/**
 * Non-uniformly translates all of the vertices of this polygon.
 *
 * @param offset The non-uniform translation amount
 *
 * @return This polygon, translated non-uniformly.
 */
Poly2& Poly2::operator+=(const Vec2 offset) {
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        *it += offset;
    }
    return *this;
}

/**
 * Uniformly translates all of the vertices of this polygon.
 *
 * @param offset The inverse of the uniform translation amount
 *
 * @return This polygon, translated uniformly.
 */
Poly2& Poly2::operator-=(float offset) {
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        it->x -= offset;
        it->y -= offset;
    }
    return *this;
}

/**
 * Non-uniformly translates all of the vertices of this polygon.
 *
 * @param offset The inverse of the non-uniform translation amount
 *
 * @return This polygon, translated non-uniformly.
 */
Poly2& Poly2::operator-=(const Vec2 offset) {
    for(auto it = vertices.begin(); it != vertices.end(); ++it) {
        *it -= offset;
    }
    return *this;
}

#pragma mark -
#pragma mark Geometry Methods
/**
 * Returns the bounding box for the polygon
 *
 * The bounding box is the minimal rectangle that contains all of the vertices in
 * this polygon.  It is recomputed whenever the vertices are set.
 *
 * @return the bounding box for the polygon
 */
const Rect Poly2::getBounds() const {
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
std::vector<Uint32> Poly2::convexHull() const {
    return Path2::convexHull(vertices);
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
 * @param  point    The point to test
 *
 * @return true if this polygon contains the given point.
 */
bool Poly2::contains(float x, float y) const {
    bool inside = false;
    for (int ii = 0; !inside && 3 * ii < indices.size(); ii++) {
        Vec2 temp2(x,y);
        Vec3 temp3 = getBarycentric( temp2, ii );
        inside = (0 <= temp3.x && temp3.x <= 1 &&
                  0 <= temp3.y && temp3.y <= 1 &&
                  0 <= temp3.z && temp3.z <= 1);
    }
    return inside;
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
bool Poly2::incident(float x, float y, float err) const {
    Vec2 p(x,y);
    std::vector<std::vector<Uint32>> bounds = boundaries();
    for(auto it = bounds.begin(); it != bounds.end(); ++it) {
        for (size_t ii = 0; ii < it->size(); ii += 2) {
            Vec2 v = vertices[it->at(ii)];
            Vec2 w = vertices[it->at(ii+1)];
            if (colinear(v,w,p,err)) {
                return true;
            }
        }
    }
    return false;
}

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
std::unordered_set<Uint32> Poly2::exterior() const {
    std::unordered_set<Uint32> result;
    exterior(result);
    return result;
}

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
 * @param indices   A buffer to store the indices on the boundary
 *
 * @return the number of elements added to the buffer
 */
size_t Poly2::exterior(std::unordered_set<Uint32>& buffer) const {
    std::unordered_map<Uint32,std::unordered_set<Uint32>*> neighbors;
    std::unordered_map<Uint32,Uint32> count;
    for(int ii = 0; ii < indices.size(); ii += 3) {
        for(int jj = 0; jj < 3; jj++) {
            Uint32 indx = indices[ii+jj];
            auto search = neighbors.find(indx);
            std::unordered_set<Uint32>* slot = nullptr;
            if (search == neighbors.end()) {
                slot = new std::unordered_set<Uint32>();
                neighbors[indx] = slot;
                count[indx] = 0;
            } else {
                slot = search->second;
            }

            slot->emplace( indices[ii+((jj+1) % 3)] );
            slot->emplace( indices[ii+((jj+2) % 3)] );
            count[indx] = count[indx]+1;
        }
    }

    size_t csize = buffer.size();
    buffer.reserve(csize+vertices.size());
    for(Uint32 ii = 0; ii < vertices.size(); ii++) {
        if (neighbors[ii]->size() > count[ii]) {
            buffer.emplace(ii);
        }
    }
    
    // Clean up
    for(auto it = neighbors.begin(); it != neighbors.end(); ++it) {
        delete it->second;
        it->second = nullptr;
    }
    return buffer.size()-csize;
}

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
std::vector<std::vector<Uint32>> Poly2::boundaries() const {
    std::vector<std::vector<Uint32>> result;
    boundaries(result);
    return result;
}

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
 * @param indices   A buffer to connected boundary components
 *
 * @return the number of elements added to the buffer
 */
size_t Poly2::boundaries(std::vector<std::vector<Uint32>>& buffer) const {
    // Create the decomposition
    std::unordered_map<std::string,Poly2TreeNode*> decomp;
    for(int ii = 0; ii < indices.size(); ii += 3) {
        std::string key = Poly2TreeNode::toString(indices[ii],indices[ii+1],indices[ii+2]);
        auto search = decomp.find(key);
        if (search == decomp.end()) {
            Poly2TreeNode* current = new Poly2TreeNode(indices[ii],indices[ii+1],indices[ii+2]);
            for(auto it = decomp.begin(); it != decomp.end(); ++it) {
                Poly2TreeNode* node = it->second;
                if (node->adjacent( current )) {
                    node->neighbors.emplace( current );
                    current->neighbors.emplace(node);
                }
            }
            decomp[key] = current;
        }
    }
     
    // Create arrays for the result and to track our progress
    std::unordered_set<Uint32> inuse;
    std::unordered_set<Uint32> total;
    for(auto it = indices.begin(); it != indices.end(); ++it) {
        total.emplace(*it);
    }
     
    // Keep going until all boundaries found
    size_t csize = buffer.size();
    buffer.push_back(std::vector<Uint32>());
    std::vector<Uint32>* array = &buffer.back();
    bool abort = false;
    while (inuse.size() != total.size() && !abort) {
        // Pick a valid (exterior) starting point at the correct position
        Poly2TreeNode* current = nullptr;
        int start = -1;
        for(auto it = decomp.begin(); start == -1 && it != decomp.end(); ++it) {
            Poly2TreeNode* node = it->second;
            start = node->pick(inuse);
            if (start != -1) {
                current = node;
            }
        }
         
        // Self-crossings may allow a point to be reused, so we
        // need a local "visited" set for each path.
        std::unordered_set<Uint32> visited;
        if (start != -1) {
            // Follow the path until no more indices to pick
            int index = start;
            current = current->follow( index );
            while (current != nullptr) {
                visited.emplace( index );
                array->push_back( index );
                index = current->pick( visited );
                current = current->follow( index );
            }
            // Add this to the global results
            for(auto it = visited.begin(); it != visited.end(); ++it) {
                inuse.emplace(*it);
            }
            buffer.push_back(std::vector<Uint32>());
            array = &buffer.back();

            // Reset the tree node internal state for next pass
            if (inuse.size() != total.size()) {
                for(auto it = decomp.begin(); it != decomp.end(); ++it) {
                    it->second->previous = nullptr;
                }
            }
        } else {
            // All the indices found were internal
            abort = true;
        }
    }
     
    // Clean up the decomposition
    for(auto it = decomp.begin(); it != decomp.end(); ++it) {
        delete it->second;
        it->second = nullptr;
    }

    array = &buffer.back();
    if (array->empty()) {
         buffer.pop_back();
    }
    
    // Algorithm produces borders with REVERSE orientation (outside is CW)
    // We need to reverse all of them to guarantee CCW rule
    for(size_t ii = csize; ii < buffer.size(); ii++) {
        std::vector<Uint32>* border = &(buffer[ii]);
        size_t bsize = border->size();
        for(size_t jj = 0; jj < bsize/2; jj++) {
            Uint32 temp = border->at(jj);
            border->at(jj) = border->at(bsize-jj-1);
            border->at(bsize-jj-1) = temp;
        }
    }

    return buffer.size()-csize;
}

#pragma mark -
#pragma mark Conversion Methods
/**
 * Returns a string representation of this rectangle for debugging purposes.
 *
 * If verbose is true, the string will include class information.  This
 * allows us to unambiguously identify the class.
 *
 * @param verbose Whether to include class information
 *
 * @return a string representation of this rectangle for debuggging purposes.
 */
std::string Poly2::toString(bool verbose) const {
    std::stringstream ss;
    ss << (verbose ? "cugl::Poly2[" : "[");
    for(size_t ii = 0; ii < indices.size(); ii += 3) {
        if (ii > 0) {
    		ss << (ii == 0 ? "; " : ", ");
        }
        ss << "{ ";
        ss << vertices[indices[ii+0]].toString();
        ss << ", ";
        ss << vertices[indices[ii+1]].toString();
        ss << ", ";
        ss << vertices[indices[ii+2]].toString();
        ss << " }";
    }
    ss << "]";
    return ss.str();
}

/** Cast from Poly2 to a Rect. */
Poly2::operator Rect() const {
	return getBounds();
}

#pragma mark -
#pragma mark Internal Helpers
/**
 * Returns the barycentric coordinates for a point relative to a triangle.
 *
 * The triangle is identified by the given index.  For index ii, it is the
 * triangle defined by indices 3*ii, 3*ii+1, and 3*ii+2.
 *
 * This method is not defined if the polygon is not SOLID.
 */
Vec3 Poly2::getBarycentric(Vec2 point, Uint32 index) const {
    Vec2 a = vertices[indices[3*index  ]];
    Vec2 b = vertices[indices[3*index+1]];
    Vec2 c = vertices[indices[3*index+2]];
    
    float det = (b.y-c.y)*(a.x-c.x)+(c.x-b.x)*(a.y-c.y);
    Vec3 result;
    result.x = (b.y-c.y)*(point.x-c.x)+(c.x-b.x)*(point.y-c.y);
    result.y = (c.y-a.y)*(point.x-c.x)+(a.x-c.x)*(point.y-c.y);
    result.x /= det;
    result.y /= det;
    result.z = 1 - result.x - result.y;
    return result;
}
