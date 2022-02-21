//
//  CUPathSmoother.h
//  Cornell University Game Library (CUGL)
//
//  This module is a factory taking a continuous path of points and smoothing it
//  into a path with less points. We realized that this class was necessary
//  when students kept writing code to track a finger gesture and stored all the
//  points, no matter how close they were together (and then tried to draw that
//  path, overwhelming the poor early versions of the SpriteBatch).
//
//  This class uses the Douglas-Peuker algorithm, as described here:
//
//       https://en.wikipedia.org/wiki/Ramer–Douglas–Peucker_algorithm
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
//  Version: 7/12/21
//
#include <cugl/math/polygon/CUPathSmoother.h>
#include <cugl/util/CUDebug.h>

using namespace cugl;

/* This makes sense as default for touch coordinates */
#define DEFAULT_EPSILON 1

#pragma mark Constructors
/**
 * Creates a path smoother with no vertex data.
 */
PathSmoother::PathSmoother() :
_calculated(false),
_epsilon(DEFAULT_EPSILON) {
}

/**
 * Creates a path smoother with the given vertex data.
 *
 * The vertex data is copied.  The smother does not retain any references
 * to the original data.
 *
 * @param points    The vertices to triangulate
 */
PathSmoother::PathSmoother(const std::vector<Vec2>& points) :
_calculated(false),
_epsilon(DEFAULT_EPSILON) {
    set(points);
}

#pragma mark -
#pragma mark Calculation
/**
 * Clears all internal data, but still maintains the initial vertex data.
 */
void PathSmoother::reset() {
    _output.clear();
    _calculated = false;
}

/**
 * Clears all internal data, the initial vertex data.
 *
 * When this method is called, you will need to set a new vertices before
 * calling calculate.
 */
void PathSmoother::clear() {
    reset();
    _input.clear();
}

/**
 * Performs a triangulation of the current vertex data.
 */
void PathSmoother::calculate() {
    douglasPeucker(0,_input.size()-1);
}

/**
 * Recursively performs Douglas-Peuker on the given input segment
 *
 * The results will be pulled from _input and placed in _output.
 *
 * @param start The first position in _input to process
 * @param end   The last position in _input to process
 *
 * @return the number of points preserved in smoothing
 */
size_t PathSmoother::douglasPeucker(size_t start, size_t end) {
    const size_t OVER = (size_t)-1;
    Vec2 sp = _input[start];
    Vec2 ep = (end == OVER ? _input[0] : _input[end]);
    if (end - start <= 1 || (end == OVER && start == _input.size()-1)) {
        _output.push_back(sp);
        _output.push_back(ep);
        return 2;
    } else if (sp == ep) {
        _output.push_back(sp);
        size_t index = OVER;
        for(size_t ii = start+1; index == OVER && ii < end; ii++) {
            Vec2 v = _input[ii];
            if (v != sp) {
                index = ii;
            }
        }
        if (index != OVER) {
            return douglasPeucker( index, end)+1;
        } else {
            _output.push_back(ep);
            return 2;
        }
    }

    float dMax = 0;
    size_t index = 0;
    for(size_t ii = start+1; ii < end; ii++) {
        Vec2 v = _input[ii];
        Vec2 u = ep-sp;
        float dist = fabsf((u.y*v.x-u.x*v.y+ep.x*sp.y-ep.y*sp.x)/u.length());
        if (dist > dMax) {
            index = ii;
            dMax = dist;
        }
    }

    if (dMax > _epsilon) {
        size_t result = 0;
        result += douglasPeucker(start, index);
        _output.pop_back();
        result += douglasPeucker(index, end);
        return result;
    } else {
        _output.push_back(sp);
        _output.push_back(ep);
        return 2;
    }
    
    return 0;
}


#pragma mark -
#pragma mark Materialization
/**
 * Returns a list of points representing the smoothed path.
 *
 * The result is guaranteed to be a subset of the original vertex path,
 * order preserved. The smoother does not retain a reference to the returned
 * list; it is safe to modify it.
 *
 * If the calculation is not yet performed, this method will return the
 * empty list.
 *
 * @return a list of indices representing the triangulation.
 */
std::vector<Vec2> PathSmoother::getPoints() const {
    if (_calculated) {
        return _output;
    }
    return std::vector<Vec2>();
}

/**
 * Stores the smoothed path in the given buffer.
 *
 * The result is guaranteed to be a subset of the original vertex path,
 * order preserved. The points will be appended to the provided vector.
 * You should clear the vector first if you do not want to preserve the
 * original data.
 *
 * If the calculation is not yet performed, this method will do nothing.
 *
 * @return the number of elements added to the buffer
 */
size_t PathSmoother::getPoints(std::vector<Vec2>& buffer) const {
    if (_calculated) {
        buffer.insert(buffer.end(),_output.begin(),_output.end());
    }
    return _output.size();
}

/**
 * Returns a path object representing the smoothed result.
 *
 * The resulting path is open.
 *
 * If the calculation is not yet performed, this method will return the
 * empty path.
 *
 * @return a polygon representing the triangulation.
 */
Path2 PathSmoother::getPath() const {
    Path2 path;
    path.vertices = _output;
    return path;
}

/**
 * Stores the smoothed path in the given buffer.
 *
 * If the buffer is not empty, the vertices will be appended to
 * the end of the buffer.
 *
 * If the calculation is not yet performed, this method will do nothing.
 *
 * @param buffer    The buffer to store the smoothed path
 *
 * @return a reference to the buffer for chaining.
 */
Path2* PathSmoother::getPath(Path2* buffer) const {
    CUAssertLog(buffer, "Destination buffer is null");
    buffer->vertices.insert(buffer->vertices.end(),_output.begin(), _output.end());
    return buffer;

}
