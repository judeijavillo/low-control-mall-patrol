//
//  CUEarclipTriangulator.cpp
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
#include <cugl/math/polygon/CUEarclipTriangulator.h>
#include <cugl/math/CUPoly2.h>
#include <cugl/math/CUPath2.h>
#include <cugl/util/CUDebug.h>

using namespace cugl;

#pragma mark Support Class
/**
 * An internal class that manages vertex data
 *
 * The primary reason for this class is to implement a doubly linked list that we
 * use to cut holes out of our polygon.  But it also allows us to quickly categorize
 * ear status.
 */
class EarclipTriangulator::Vertex {
public:
    /** The index position of this vertex in the input set */
    Uint32 index;
    /** The vertex coordinate */
    Vec2 coord;
    /** The (current) interior angle of this vertex */
    float angle;
    /** Whether or not this vertex is (currently) an ear */
    bool eartip;
    /** Whether or not this vertex is (currently) active */
    bool active;
    /** The next vertex along this path */
    Vertex* next;
    /** The previous vertex along this path */
    Vertex* prev;
    
    /**
     * Initializes a default vertex
     */
    Vertex() :
    index(0),
    angle(0),
    eartip(false),
    active(true),
    next(nullptr),
    prev(nullptr) {
    }
    
    /**
     * Initializes a vertex from the given input set
     *
     * @param input The input set
     * @param pos   The vertex position
     */
    Vertex(const std::vector<Vec2>& input, Uint32 pos) :
    angle(0),
    eartip(false),
    active(true),
    next(nullptr),
    prev(nullptr) {
        index = pos;
        coord = input[pos];
    }
    
    /**
     * Sets this vertex to be the value at the given
     * input set.
     *
     * @param input The input set
     * @param pos   The vertex position
     */
    void set(const std::vector<Vec2>& input, Uint32 pos) {
        index = pos;
        coord = input[pos];
        angle = 0;
        eartip = false;
        active = true;
        next = nullptr;
        prev = nullptr;
    }
    
    /**
     * Copies this vertex into the specified location
     *
     * All values, including the next and prev pointers are copied.
     * This method is necessary for hole slicing.
     */
    void copy(Vertex* dst) {
        dst->index  = index;
        dst->coord  = coord;
        dst->angle  = angle;
        dst->eartip = eartip;
        dst->active = active;
        dst->next = next;
        dst->prev = prev;
    }
    
    /**
     * Returns true if this vertex has a convex interior angle.
     *
     * @return true if this vertex has a convex interior angle.
     */
    bool convex() {
        return convex(prev->coord,coord,next->coord);
    }

    /**
     * Returns true if the angle defined by the three points is convex.
     *
     * The defined angle is centered at p2, with p1 going into p2 and p2
     * going out to p3.
     *
     * @param p1    The start of the angle
     * @param p1    The center of the angle
     * @param p1    The end of the angle
     *
     * @return true if the angle defined by the three points is convex.
     */
    static bool convex(const Vec2& p1, const Vec2& p2, const Vec2& p3) {
        float tmp = (p3.y - p1.y) * (p2.x - p1.x) - (p3.x - p1.x) * (p2.y - p1.y);
        return tmp > 0;
    }
    
    /**
     * Returns true if p is inside this vertex's ear region
     *
     * The ear region for a vertex is the triangle defined by it and its
     * two neighbors.
     *
     * @param p The point to check
     *
     * @return true if p is inside this vertex's ear region
     */
    bool inside(const Vec2& p) {
        if (convex(prev->coord, p, coord)) { return false; }
        if (convex(coord, p, next->coord)) { return false; }
        if (convex(next->coord, p, prev->coord)) { return false; }
        return true;
    }

    /**
     * Returns true if p is inside of the cone defined by this vertex
     *
     * The vertex cone is centered at this vertex and defined by the
     * lines (not line segments) to the two neighbors.
     *
     * @param p The point to check
     *
     * @return true if p is inside of the cone defined by this vertex
     */
    bool incone(const Vec2& p) {
        if (convex()) {
            return (convex(prev->coord, coord, p) && convex(coord, next->coord, p));
        }
        return (convex(prev->coord, coord, p) || convex(coord, next->coord, p));
    }

    /**
     * Updates this vertex to determine ear status
     *
     * This method should be called whenever an ear is clipped from the
     * vertex set.
     */
    void update() {
        Vec2 vec1 = prev->coord-coord;
        Vec2 vec3 = next->coord-coord;
        vec1.normalize();
        vec3.normalize();
        
        angle = vec1.x * vec3.x + vec1.y * vec3.y;
        if (convex()) {
            eartip = true;
            Vertex* curr = next->next;
            while (curr != prev) {
                bool test = curr->index != index && curr->index != prev->index && curr->index != next->index;
                if (test && inside(curr->coord)) {
                    eartip = false;
                }
                curr = curr->next;
            }
        } else {
            eartip = false;
        }
    }
};

/**
 * Returns true if the lines defined by the two pairs of vectors intersect
 *
 * @param p11   First vector of first line
 * @param p12   Second vector of first line
 * @param p21   First vector of second line
 * @param p22   Second vector of second line
 *
 * @return true if the lines defined by the two pairs of vectors intersect
 */
static bool intersects(const Vec2& p11, const Vec2& p12, const Vec2& p21, const Vec2& p22) {
    if (p11 == p21 || p11 == p22 || p12 == p21 || p12 == p22) {
        return false;
    }
    
    Vec2 v1ort(p12.y-p11.y,p11.x-p12.x);
    Vec2 v2ort(p22.y-p21.y,p21.x-p22.x);
    float dot21 = v1ort.dot(p21-p11);
    float dot22 = v1ort.dot(p22-p11);
    float dot11 = v2ort.dot(p11-p21);
    float dot12 = v2ort.dot(p12-p21);
    
    return !(dot11 * dot12 > 0 || dot21 * dot22 > 0);
}

#pragma mark -
#pragma mark Constructors
/**
 * Creates a triangulator with no vertex data.
 */
EarclipTriangulator::EarclipTriangulator() :
_vertices(nullptr),
_exterior(0),
_vertsize(0),
_vertlimit(0),
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
EarclipTriangulator::EarclipTriangulator(const std::vector<Vec2>& points) :
_vertices(nullptr),
_exterior(0),
_vertsize(0),
_vertlimit(0),
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
EarclipTriangulator::EarclipTriangulator(const Path2& path) :
_vertices(nullptr),
_exterior(0),
_vertsize(0),
_vertlimit(0),
_calculated(false) {
    set(path);
}

/**
 * Deletes this triangulator, releasing all resources.
 */
EarclipTriangulator::~EarclipTriangulator() {
    if (_vertices != nullptr) {
        free(_vertices);
        _vertices = nullptr;
        _vertsize = 0;
        _vertlimit = 0;
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
 * as well as any previously added holes. You will need to re-add
 * any lost data and reperform the calculation.
 *
 * @param points    The vertices to triangulate
 */
void EarclipTriangulator::set(const std::vector<Vec2>& points) {
    CUAssertLog(Path2::orientation(points) == -1, "Path orientiation is not CCW");
    reset();
    _exterior = points.size();
    _input.reserve(_exterior);
    _input.insert(_input.end(), points.begin(), points.end());
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
 * as well as any previously added holes. You will need to re-add
 * any lost data and reperform the calculation.
 *
 * @param points    The vertices to triangulate
 * @param size      The number of vertices
 */
void EarclipTriangulator::set(const Vec2* points, size_t size) {
    CUAssertLog(Path2::orientation(points,size) == -1, "Path orientiation is not CCW");
    reset();
    _exterior = size;
    _input.reserve(_exterior);
    _input.insert(_input.end(), points, points+size);
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
 * as well as any previously added holes. You will need to re-add
 * any lost data and reperform the calculation.
 *
 * @param path    The vertices to triangulate
 */
void EarclipTriangulator::set(const Path2& path) {
    CUAssertLog(path.orientation() == -1, "Path orientiation is not CCW");
    reset();
    _exterior = path.size();
    _input.reserve(_exterior);
    _input.insert(_input.end(), path.vertices.begin(), path.vertices.end());
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
void EarclipTriangulator::addHole(const std::vector<Vec2>& points) {
    CUAssertLog(Path2::orientation(points) == 1, "Hole orientiation is not CW");
    size_t size = _input.size();
    _holes.push_back(size);
    _holes.push_back(points.size());
    _input.reserve(size+points.size());
    _input.insert(_input.end(), points.begin(), points.end());
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
void EarclipTriangulator::addHole(const Vec2* points, size_t size) {
    CUAssertLog(Path2::orientation(points,size) == 1, "Hole orientiation is not CW");
    size_t isize = _input.size();
    _holes.push_back(isize);
    _holes.push_back(size);
    _input.reserve(isize+size);
    _input.insert(_input.end(), points, points+size);
}

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
void EarclipTriangulator::addHole(const Path2& path) {
    CUAssertLog(path.orientation() == 1, "Hole orientiation is not CW");
    size_t size = _input.size();
    _holes.push_back(size);
    _holes.push_back(path.size());
    _input.reserve(size+path.size());
    _input.insert(_input.end(), path.vertices.begin(), path.vertices.end());
}

#pragma mark -
#pragma mark Calculation
/**
 * Clears all internal data, but still maintains the initial vertex data.
 *
 * This method also retains any holes. It only clears the triangulation results.
 */
void EarclipTriangulator::reset() {
    _vertsize = 0;
    _output.clear();
    _calculated = false;
}

/**
 * Clears all internal data, including the initial vertex data.
 *
 * When this method is called, you will need to set a new vertices before
 * calling calculate. In addition, any holes will be lost as well.
 */
void EarclipTriangulator::clear() {
    reset();
    _input.clear();
    _holes.clear();
}

/**
 * Performs a triangulation of the current vertex data.
 */
void EarclipTriangulator::calculate() {
    reset();
    if (_exterior > 0) {
        allocateVertices();
        removeHoles();
        computeTriangles();
    }
    _calculated = true;
}


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
std::vector<Uint32> EarclipTriangulator::getTriangulation() const {
    return _output;
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
size_t EarclipTriangulator::getTriangulation(std::vector<Uint32>& buffer) const {
    if (_calculated) {
        buffer.insert(buffer.end(), _output.begin(), _output.end());
        return _output.size();
    }
    return 0;
}

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
Poly2 EarclipTriangulator::getPolygon() const {
    Poly2 poly;
    if (_calculated) {
        poly.vertices = _input;
        poly.indices  = _output;
    }
    return poly;
}

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
Poly2* EarclipTriangulator::getPolygon(Poly2* buffer) const {
    CUAssertLog(buffer, "Destination buffer is null");
    if (_calculated) {
        Uint32 offset = (int)buffer->vertices.size();
        if (offset > 0) {
            buffer->vertices.insert(buffer->vertices.end(), _input.begin(), _input.end());
            buffer->indices.reserve(buffer->indices.size()+_output.size());
            for(auto it = _output.begin(); it != _output.end(); ++it) {
                buffer->indices.push_back(offset+*it);
            }
        } else {
            buffer->vertices = _input;
            buffer->indices  = _output;
        }
    }
    return buffer;
}


#pragma mark -
#pragma mark Internal Computation
/**
 * Allocates the doubly-linked list(s) to manage the vertices
 */
void EarclipTriangulator::allocateVertices() {
    Uint32 holepos = -1;
    size_t needed = _input.size()+2*_holes.size();
    if (needed > _vertlimit) {
        if (_vertices != nullptr) {
            free(_vertices);
            _vertices = nullptr;
        }
        _vertlimit = needed;
        _vertices = (Vertex*)malloc(_vertlimit*sizeof(Vertex));
    }
    _vertsize = 0;
    for(size_t pos = 0; pos < _input.size(); pos++) {
        _vertices[pos].set(_input,(Uint32)pos);
        size_t start = holepos == -1 ? 0 : _holes[2*holepos];
        size_t end   = holepos == -1 ? _exterior : _holes[2*holepos]+_holes[2*holepos+1];
        if (pos == start) {
            _vertices[pos].prev = _vertices+(end-1);
        } else {
            _vertices[pos].prev = _vertices+(pos-1);
        }
        if (pos == end-1) {
            holepos++;
            _vertices[pos].next = _vertices+(start);
        } else {
            _vertices[pos].next = _vertices+(pos+1);
        }
    }
    _vertsize = _input.size();
}

/**
 * Slices out holes, merging vertices into one doubly-linked list
 */
void EarclipTriangulator::removeHoles() {
    if (_holes.size() == 0) {
        return;
    }
    
    size_t* holesleft;
    size_t  holessize = _holes.size()/2;
    holesleft = (size_t*)malloc(2*holessize*sizeof(size_t));
    std::memcpy(holesleft, _holes.data(), 2*holessize*sizeof(size_t));
    
    while (holessize > 0) {
        // Find the hole point with the largest x.
        bool hasholes = false;
        size_t holepart = 0;
        size_t holeindx = 0;
        for(size_t ii = 0; ii < holessize; ii++) {
            if (!hasholes) {
                hasholes = true;
                holepart = ii;
                holeindx = holesleft[2*ii];
            }
            for(size_t jj = 0; jj < holesleft[2*ii+1]; jj++) {
                size_t index = holesleft[2*ii]+jj;
                if (_vertices[index].coord.x > _vertices[holeindx].coord.x) {
                    holepart = ii;
                    holeindx = holesleft[2*ii]+jj;
                }
            }
        }
        
        
        Vertex* holepoint = _vertices+holeindx;
        Vertex* bestpoint = nullptr;

        // Find the point on the exterior to connect it to
        bool repeat = true;
        Vertex* curr = _vertices;
        while (repeat || curr != _vertices) {
            if (curr->coord.x > holepoint->coord.x && curr->incone(holepoint->coord)) {
                bool checkit = true;
                if (bestpoint) {
                    Vec2 v1 = curr->coord-holepoint->coord;
                    Vec2 v2 = bestpoint->coord-holepoint->coord;
                    v1.normalize();
                    v2.normalize();
                    if (v2.x > v1.x) {
                        checkit = false;
                    }
                }
                if (checkit) {
                    bool pointvisible = true;
                    for(size_t ii = 0; pointvisible && ii < holessize; ii++) {
                        for(size_t jj = 0; pointvisible && jj < holesleft[2*ii+1]; jj++) {
                            Vertex* checkpt = _vertices+holesleft[2*ii]+jj;
                            Vertex* nextpt  = checkpt->next;
                            if (intersects(holepoint->coord, curr->coord, checkpt->coord, nextpt->coord)) {
                                pointvisible = false;
                            }
                        }
                    }
                    if (pointvisible) {
                        bestpoint = curr;
                    }
                }
            }
            repeat = false;
            curr = curr->next;
        }
        
        if (bestpoint == nullptr) {
            free(holesleft);
            return;
        }
        
        // Copy the split points
        Vertex* holecopy = _vertices+_vertsize;
        Vertex* bestcopy = _vertices+_vertsize+1;
        _vertsize += 2;
        
        holepoint->copy(holecopy);
        bestpoint->copy(bestcopy);
        
        // Split at the divider
        bestpoint->prev->next = bestcopy;
        holepoint->prev->next = holecopy;
        
        bestpoint->prev = holecopy;
        holecopy->next = bestpoint;
        bestcopy->next = holepoint;
        holepoint->prev = bestcopy;
        
        // Remove this hole
        holesleft[2*holepart  ] = holesleft[2*holessize-2];
        holesleft[2*holepart+1] = holesleft[2*holessize-1];
        holessize--;
    }
    free(holesleft);
}

/**
 * Computes the triangle indices for the active vertices.
 */
void EarclipTriangulator::computeTriangles() {
    // Degenerate case
    if (_vertsize == 3) {
        _output.push_back(0);
        _output.push_back(1);
        _output.push_back(2);
        return;
    }
    
    // Find some initial ears
    for(size_t ii = 0; ii < _vertsize; ii++) {
        _vertices[ii].update();
    }
    
    _output.reserve(3*(_vertsize-3));
    for(size_t ii = 0; ii < _vertsize-3; ii++) {
        Vertex* bestear = nullptr;
        // Find the most extruded ear.
        for(size_t jj = 0; jj < _vertsize; jj++) {
            Vertex* v = _vertices+jj;
            if (v->active && v->eartip) {
                if (bestear == nullptr) {
                    bestear = v;
                } else if (v->angle > bestear->angle) {
                    bestear = v;
                }
            }
        }
        
        if (bestear == nullptr) {
            CUAssertLog(bestear != nullptr, "Could not find a suitable ear");
            return;
        }
        
        _output.push_back(bestear->prev->index);
        _output.push_back(bestear->index);
        _output.push_back(bestear->next->index);
        
        // Cut off the ear
        bestear->active = false;
        bestear->prev->next = bestear->next;
        bestear->next->prev = bestear->prev;
        
        if (ii != _vertsize - 4) {
            bestear->prev->update();
            bestear->next->update();
        }
    }

    // Scan for first missing ear
    bool open = true;
    for(size_t ii = 0; open && ii < _vertsize; ii++) {
        if (_vertices[ii].active) {
            _output.push_back(_vertices[ii].prev->index);
            _output.push_back(_vertices[ii].index);
            _output.push_back(_vertices[ii].next->index);
            open = false;
        }
    }
}
