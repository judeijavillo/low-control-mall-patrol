//
//  CUComplexExtruder.cpp
//  Cornell University Game Library (CUGL)
//
//  This module is a factory for extruding a path polgyon into a stroke with
//  width. It has support for joints and end caps.
//
//  This version of the extruder is built on top of the famous Clipper library:
//
//      http://www.angusj.com/delphi/clipper.php
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
//  Version: 1/22/21
//
#include <cugl/math/polygon/CUComplexExtruder.h>
#include <cugl/math/polygon/CUDelaunayTriangulator.h>
#include <cugl/util/CUDebug.h>
#include <cugl/math/CUPoly2.h>
#include <cugl/math/CUPath2.h>
#include <cugl/util/CUTimestamp.h>
#include <iterator>

using namespace cugl;

/** The default Clipper resolution */
#define RESOLUTION 8
#define MITRELIMIT 2

#pragma mark -
#pragma mark Constructors
/**
 * Creates an extruder with no vertex data.
 */
ComplexExtruder::ComplexExtruder() :
_calculated(false),
_resolution(RESOLUTION),
_mitrelimit(MITRELIMIT) {
}

/**
 * Creates an extruder with the given vertex data.
 *
 * The vertex data is copied.  The extruder does not retain any references
 * to the original data.
 *
 * @param points    The vertices to extrude
 * @param closed    Whether the path is closed
 */
ComplexExtruder::ComplexExtruder(const std::vector<Vec2>& points, bool closed) :
_calculated(false),
_resolution(RESOLUTION),
_mitrelimit(MITRELIMIT) {
    set(points,closed);
}
  
/**
 * Creates an extruder with the given vertex data.
 *
 * The path data is copied. The extruder does not retain any references
 * to the original data.
 *
 * @param path    The vertices to extrude
 */
ComplexExtruder::ComplexExtruder(const Path2& path) :
_calculated(false),
_resolution(RESOLUTION),
_mitrelimit(MITRELIMIT) {
    set(path);
}

#pragma mark -
#pragma mark Initialization
/**
 * Sets the vertex data for this extruder.
 *
 * The vertex data is copied.  The extruder does not retain any references
 * to the original data.
 *
 * This method resets all interal data.  You will need to reperform the
 * calculation before accessing data.
 *
 * @param points    The vertices to extruder
 * @param closed    Whether the path is closed
 */
void ComplexExtruder::set(const std::vector<Vec2>& points, bool closed) {
    reset();
    _input.vertices = points;
    _input.closed = closed;
}

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
void ComplexExtruder::set(const Vec2* points, size_t size, bool closed) {
    reset();
    _input.vertices.insert(_input.vertices.begin(), points, points+size);
    _input.closed = closed;
}

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
void ComplexExtruder::set(const Path2& path) {
    reset();
    _input = path;
}

#pragma mark -
#pragma mark Clipper Attributes
/**
 * Sets the joint value for the extrusion.
 *
 * The joint type determines how the extrusion joins the extruded
 * line segments together.  See {@link poly2::Joint} for the
 * description of the types.
 *
 * @param joint     The extrusion joint type
 */
void ComplexExtruder::setJoint(poly2::Joint joint) {
    switch (joint) {
        case poly2::Joint::SQUARE:
            _joint = ClipperLib::jtSquare;
        case poly2::Joint::MITRE:
            _joint = ClipperLib::jtMiter;
        case poly2::Joint::ROUND:
            _joint = ClipperLib::jtRound;
        default:
            _joint = ClipperLib::jtSquare;
    }
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
poly2::Joint ComplexExtruder::getJoint() const {
    switch (_joint) {
        case ClipperLib::jtSquare:
            return poly2::Joint::SQUARE;
        case ClipperLib::jtMiter:
            return poly2::Joint::MITRE;
        case ClipperLib::jtRound:
            return poly2::Joint::ROUND;
    }
    return poly2::Joint::SQUARE;
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
void ComplexExtruder::setEndCap(poly2::EndCap endcap) {
    switch (endcap) {
        case poly2::EndCap::BUTT:
            _endcap = ClipperLib::etOpenButt;
        case poly2::EndCap::SQUARE:
            _endcap = ClipperLib::etOpenSquare;
        case poly2::EndCap::ROUND:
            _endcap = ClipperLib::etOpenRound;
    }
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
poly2::EndCap ComplexExtruder::getEndCap() const {
    switch (_endcap) {
        case ClipperLib::etOpenButt:
            return poly2::EndCap::BUTT;
        case ClipperLib::etOpenSquare:
            return poly2::EndCap::SQUARE;
        case ClipperLib::etOpenRound:
            return poly2::EndCap::ROUND;
        default:
            return poly2::EndCap::BUTT;
    }
    return poly2::EndCap::BUTT;
}

#pragma mark -
#pragma mark Calculation
/**
 * Clears all computed data, but still maintains the settings.
 *
 * This method preserves all initial vertex data, as well as the joint,
 * cap, and precision settings.
 */
void ComplexExtruder::reset() {
    _bounds.clear();
    _output.clear();
    _calculated = false;
}

/**
 * Clears all internal data, including initial vertex data.
 *
 * When this method is called, you will need to set a new vertices before
 * calling {@link #calculate}.  However, the joint, cap, and precision
 * settings are preserved.
 */
void ComplexExtruder::clear() {
    reset();
    _input.clear();
}

/**
 * Performs a extrusion of the current vertex data.
 *
 * An extrusion of a polygon is a second polygon that follows the path of
 * the first one, but gives it width.  Hence it takes a path and turns it
 * into a solid shape. This is more complicated than simply triangulating
 * the original polygon.  The new polygon has more vertices, depending on
 * the choice of joint (shape at the corners) and cap (shape at the end).
 *
 * This method uses the Clipper library to perform the extrusion. While
 * accurate and preferred for static shapes, it is not ideal to call
 * this method at framerate.
 *
 * @param stroke    The stroke width of the extrusion
 */
void ComplexExtruder::calculate(float stroke) {
    if (_calculated) {
        return;
    } else if (_input.size() == 0) {
        _calculated = true;
        return;
    }
    
    ClipperLib::Path inverts;
    ClipperLib::PolyTree solution;
    for(auto it = _input.vertices.begin(); it != _input.vertices.end(); ++it) {
        inverts << ClipperLib::IntPoint((ClipperLib::cInt)(it->x*_resolution),
                                        (ClipperLib::cInt)(it->y*_resolution));
    }
    
    ClipperLib::ClipperOffset worker;
    worker.MiterLimit = _mitrelimit;
    worker.AddPath( inverts, _joint, _input.closed ?  ClipperLib::etClosedLine : _endcap );
    worker.Execute(solution, stroke*_resolution);
    for(auto it = solution.Childs.begin(); it != solution.Childs.end(); it++) {
        processNode(*it);
    }

    _calculated = true;
}

/**
 * Processes a single node of a Clipper PolyTree
 *
 * This method is used to extract the data from the Clipper solution
 * and convert it to a cugl Poly2 object. This is a recursive method
 * and assumes that the PolyNode is a outer polygon and not a hole.
 *
 * @param node  The PolyNode to accumulate
 */
void ComplexExtruder::processNode(const ClipperLib::PolyNode* node) {
    Path2 path;
    for(auto it = node->Contour.begin(); it != node->Contour.end(); ++it) {
        path.vertices.push_back(Vec2((float)(it->X/(double)_resolution),(float)(it->Y/(double)_resolution)));
    }
    path.closed = true;
    _bounds.push_back(path);

    DelaunayTriangulator triang(path);
    for(auto it = node->Childs.begin(); it != node->Childs.end(); ++it) {
        Path2 hole;
        for(auto jt = (*it)->Contour.begin(); jt != (*it)->Contour.end(); ++jt) {
            hole.vertices.push_back(Vec2((float)(jt->X/(double)_resolution),(float)(jt->Y/(double)_resolution)));
        }
        hole.closed = true;
        _bounds.push_back(hole);
        triang.addHole(hole);
    }

    triang.calculate();
    triang.getPolygon(&_output);

    // Just in case.  But should never be called.
    for(auto it = node->Childs.begin(); it != node->Childs.end(); ++it) {
        for(auto jt = (*it)->Childs.begin(); jt != (*it)->Childs.end(); ++jt) {
            ClipperLib::PolyNode* next = (*jt);
            processNode(next);
        }
    }
}

#pragma mark -
#pragma mark Materialization
/**
 * Returns a polygon representing the path extrusion.
 *
 * The polygon contains the original vertices together with the new
 * indices defining the wireframe path.  The extruder does not maintain
 * references to this polygon and it is safe to modify it.
 *
 * If the calculation is not yet performed, this method will return the
 * empty polygon.
 *
 * @return a polygon representing the path extrusion.
 */
Poly2 ComplexExtruder::getPolygon() const {
    return _output;
}

/**
 * Stores the path extrusion in the given buffer.
 *
 * This method will add both the original vertices, and the corresponding
 * indices to the new buffer.  If the buffer is not empty, the indices
 * will be adjusted accordingly. You should clear the buffer first if
 * you do not want to preserve the original data.
 *
 * If the calculation is not yet performed, this method will do nothing.
 *
 * @param buffer    The buffer to store the extruded polygon
 *
 * @return a reference to the buffer for chaining.
 */
Poly2* ComplexExtruder::getPolygon(Poly2* buffer) const {
    CUAssertLog(buffer, "Destination buffer is null");
    if (_calculated) {
        if (buffer->vertices.size() == 0) {
            buffer->vertices = _output.vertices;
            buffer->indices  = _output.indices;
        } else {
            int offset = (int)buffer->vertices.size();
            buffer->vertices.reserve(offset+_output.vertices.size());
            std::copy(_output.vertices.begin(),_output.vertices.end(),std::back_inserter(buffer->vertices));
            
            buffer->indices.reserve(buffer->indices.size()+_output.indices.size());
            for(auto it = _output.indices.begin(); it != _output.indices.end(); ++it) {
                buffer->indices.push_back(offset+*it);
            }
        }
    }
    return buffer;
}

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
std::vector<Path2> ComplexExtruder::getBorder() const {
    return _bounds;
}

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
size_t ComplexExtruder::getBorder(std::vector<Path2>& buffer) const {
    buffer.insert(buffer.end(), _bounds.begin(), _bounds.end());
    return _bounds.size();
}
