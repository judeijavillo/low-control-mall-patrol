//
//  CUSpriteVertex.h
//  Cornell University Game Library (CUGL)
//
//  This module provides the basic structs for the sprite batch pipeline.
//  These structs are meant to be passed by value, so we have no methods for
//  shared pointers.
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
//  Version: 2/12/20

#ifndef __CU_SPRITE_VERTEX_H__
#define __CU_SPRITE_VERTEX_H__

#include <cugl/math/CUVec2.h>
#include <cugl/math/CUVec3.h>
#include <cugl/math/CUVec4.h>

namespace cugl {

/**
 * This class/struct is rendering information for a 2d sprite batch vertex.
 *
 * The class is intended to be used as a struct.  This struct has the basic
 * rendering information required by a {@link SpriteBatch} for rendering.
 *
 * Note that the exact meaning of these attributes can vary depending upon
 * the current drawing mode in the sprite batch.  For example, if the color
 * is a gradient, rather than a pure color, then the color attribute holds
 * the texture coordinates (plus additional modulation factors) for that
 * gradient, and not a real color value.
 */
class SpriteVertex2 {
public:
    /** The vertex position */
    cugl::Vec2    position;
    /** The vertex color */
    GLuint        color;
    /** The vertex texture coordinate */
    cugl::Vec2    texcoord;
    /** The vertex gradient coordinate */
    cugl::Vec2    gradcoord;

    /** The memory offset of the vertex position */
    static const GLvoid* positionOffset()   { return (GLvoid*)offsetof(SpriteVertex2, position);  }
    /** The memory offset of the vertex color */
    static const GLvoid* colorOffset()      { return (GLvoid*)offsetof(SpriteVertex2, color);     }
    /** The memory offset of the vertex texture coordinate */
    static const GLvoid* texcoordOffset()   { return (GLvoid*)offsetof(SpriteVertex2, texcoord);  }
    /** The memory offset of the vertex texture coordinate */
    static const GLvoid* gradcoordOffset()   { return (GLvoid*)offsetof(SpriteVertex2,gradcoord);  }
};

/**
 * This class/struct is rendering information for a 2d sprite batch vertex.
 *
 * The class is intended to be used as a struct.  This struct has the basic
 * rendering information required by a {@link SpriteBatch} for rendering.
 * Even though sprite batches are a 2d rendering pipeline, they can use
 * the z value for depth testing, enabling out of order drawing.
 *
 * Note that the exact meaning of these attributes can vary depending upon
 * the current drawing mode in the sprite batch.  For example, if the color
 * is a gradient, rather than a pure color, then the color attribute holds
 * the texture coordinates (plus additional modulation factors) for that
 * gradient, and not a real color value.
 */
class SpriteVertex3 {
public:
    /** The vertex position */
    cugl::Vec3 position;
    /** The vertex color (or texture gradient texture coordinates)  */
    GLuint     color;
    /** The vertex texture coordinate */
    cugl::Vec2 texcoord;
    /** The vertex gradient coordinate */
    cugl::Vec2 gradcoord;

    /** The memory offset of the vertex position */
    static const GLvoid* positionOffset()   { return (GLvoid*)offsetof(SpriteVertex3, position);  }
    /** The memory offset of the vertex color */
    static const GLvoid* colorOffset()      { return (GLvoid*)offsetof(SpriteVertex3, color);     }
    /** The memory offset of the vertex texture coordinate */
    static const GLvoid* texcoordOffset()   { return (GLvoid*)offsetof(SpriteVertex3, texcoord);  }
    /** The memory offset of the vertex texture coordinate */
    static const GLvoid* gradcoordOffset()  { return (GLvoid*)offsetof(SpriteVertex3, gradcoord);  }
};

}

#endif /* __CU_SPRITE_VERTEX_H__ */
