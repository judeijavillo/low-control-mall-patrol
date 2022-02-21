//
//  CUPolyEnums.h
//  Cornell University Game Library (CUGL)
//
//  This module contains the enumerations used in path extrusion (e.g turning
//  a path into a stroke with width) and path traversal. We have separated out
//  this file so that they can be used by multiple classes. In particular, the
//  extrusion enums are used by both SimpleExtruder (for speed) and
//  ComplexExtruder (for accuracy).
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
#ifndef __CU_POLY_ENUMS_H__
#define __CU_POLY_ENUMS_H__

namespace cugl {

	/**
	 * Enumerations for the polygon factories.
	 *
	 * This namespace is a collection of enumerations for the various polygon tools
	 * like {@link EarclipTriangulator}.  We gather them together in one place so that
	 * they may be used in multiple classes.  For example, we have multiple triangulator
	 * and extruder classes. Therefore, the enumerations should not be tied to any
	 * one class.
	 */ 
    namespace poly2 {
    
/**
 * The types of joints supported in an extrusion.
 *
 * A joint is the rule for how to connect two extruded line segments.
 * If there is not joint, the path will look like a sequence of overlapping
 * rectangles.
 *
 * This enumeration is used by {@link SimpleExtruder}, {@link ComplexExtruder},
 * and {@link scene2::PathNode}.
 */
enum class Joint : int {
    /** Mitre joint; ideal for paths with sharp corners */
    MITRE  = 0,
    /** Bevel joint; ideal for smoother paths (DEFAULT) */
    SQUARE = 1,
    /** Round joint; used to smooth out paths with sharp corners */
    ROUND  = 2
};
        
/**
 * The types of caps supported in an extrusion.
 *
 * A cap is the rule for how to end an extruded line segment that has no
 * neighbor on that end.  If there is no cap, the path terminates at the
 * end vertices.
 *
 * This enumeration is used by {@link SimpleExtruder}, {@link ComplexExtruder},
 * and {@link scene2::PathNode}.
 */
enum class EndCap : int {
    /** No end cap; the path terminates at the end vertices (DEFAULT) */
    BUTT   = 0,
    /** Square cap; like no cap, except the ends are padded by stroke width */
    SQUARE = 1,
    /** Round cap; the ends are half circles whose radius is the stroke width */
    ROUND  = 2
};
    
/**
 * This enum lists the types of path traversal that are supported.
 *
 * This enumeration is used by both {@link PolyFactory} and {@link scene2::WireNode}.
 */
enum class Traversal : int {
    /** No traversal; the index list will be empty. */
    NONE = 0,
    /** Traverse the border, but do not close the ends. */
    OPEN = 1,
    /** Traverse the border, and close the ends. */
    CLOSED = 2,
    /** Traverse the individual triangles in the standard tesselation. */
    INTERIOR = 3
};
    
/**
 * This enum specifies a capsule shape
 *
 * A capsule is a box with semicircular ends along the major axis. They are a
 * popular physics object, particularly for character avatars.  The rounded ends
 * means they are less likely to snag, and they naturally fall off platforms
 * when they go too far.
 *
 * Sometimes we only want half a capsule (so a semicircle at one end, but not
 * both).  This enumeration allows us to specify the exact capsule we want.
 * This enumeration is used by both {@link PolyFactory} and the physics class
 * {@link physics2::CapsuleObstacle}.
 */
enum class Capsule {
    /**
     * A degenerate capsule (e.g. an ellipse)
     *
     * Any capsule with width and height the same is degenerate.
     */
    DEGENERATE = 0,
    /**
     * A full capsule with round ends on the major axis.
     *
     * This type assumes that there is a major axis (e.g. that width
     * and height are not the same).
     */
    FULL = 1,
    /**
     * A half capsule with a rounded end on the default side.
     *
     * The default side is the left if the major axis is x, and the
     * bottom if the major axis is y. This type assumes that there is
     * a major axis (e.g. that width and height are not the same).
     */
    HALF = 2,
    /**
     * A half capsule with a rounded end on the side opposite the default.
     *
     * The opposite side is the right if the major axis is x, and the
     * top if the major axis is y. This type assumes that there is
     * a major axis (e.g. that width and height are not the same).
     */
    HALF_REVERSE = 3
};

    
    }
}

#endif /* __CU_POLY_ENUMS_H__ */
