//
//  CUGlyphRun.h
//  Cornell University Game Library (CUGL)
//
//  A glyph run is a mesh for a specific font atlas. In order to scalably
//  support unicode, fonts often have mutliple atlases, each composed of a
//  disjoint set of glyphs. When we render a string of text, we break it
//  up into multiple glyph runs, one for each relevant atlas. This allows
//  for efficient rendering of the text with minimal texture switching.
//
//  This class is essentially a struct with some shared pointer support.
//  We make all of the fields publicly accessible. We have a static constuctor
//  with a shared pointer, to be consistent with other CUGL classes.
//  However, we do not have an init method, since the default values of
//  the fields are acceptable.
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
//  Version: 2/12/21
#ifndef __CU_GLYPH_RUN_H__
#define __CU_GLYPH_RUN_H__

#include <cugl/render/CUTexture.h>
#include <cugl/render/CUMesh.h>
#include <cugl/render/CUSpriteVertex.h>
#include <utf8/utf8.h>

namespace cugl {

/**
 * This class represents a single glyph run for a rendered text string
 *
 * A glyph run is a mesh for a specific font atlas. In order to scalably
 * support unicode, fonts often have mutliple atlases, each composed of a
 * disjoint set of glyphs. When we render a string of text, we break it
 * up into multiple glyph runs, one for each relevant atlas. This allows
 * for efficient rendering of the text with minimal texture switching.
 */
class GlyphRun {
public:
    /** The glyphs represented in this glyph run */
    std::unordered_set<Uint32> contents;
    /** The mesh for the individual glyphs */
    Mesh<SpriteVertex2> mesh;
    /** The font texture necessary to render the mesh */
    std::shared_ptr<Texture> texture;
    
    /**
     * Creates an empty glyp run.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
     * the heap, use one of the static constructors instead.
     */
    GlyphRun() {
        mesh.command = GL_TRIANGLES;
        texture = nullptr;
    }
    
    /**
     * Deletes this glyph run, disposing of all resources.
     */
    ~GlyphRun() { dispose(); }
    
    /**
     * Deletes the glyph run resources.
     */
    void dispose() {
        contents.clear();
        mesh.clear();
        texture = nullptr;
    }
    
    /**
     * Returns a newly allocated glyph run
     *
     * The glyphy run is empty. Access its fields to initialize it.
     *
     * @return a newly allocated glyph run
     */
    static std::shared_ptr<GlyphRun> alloc() {
        return std::make_shared<GlyphRun>();
    }
    
    /**
     * Returns true if this glyph run contains the given glyph.
     *
     * This method depends on the value of the attribute contents.  It is
     * the reponsibility of the user to ensure that contents matches the
     * data in mesh.
     *
     * @param glyph The glyph unicode value
     *
     * @return true if this glyph run contains the given glyph.
     */
    bool contains(Uint32 glyph) const {
        return contents.find(glyph) != contents.end();
    }

    /**
     * Returns true if this glyph run contains all of the given glyphs.
     *
     * If just one glyph in the given vector is missing, this returns false.
     * This method depends on the value of the attribute contents.  It is
     * the reponsibility of the user to ensure that contents matches the
     * data in mesh.
     *
     * @param glyphs The glyph unicode values
     *
     * @return true if this glyph run contains all of the given glyphs.
     */
    bool contains(const std::vector<Uint32>& glyphs) const {
        bool success = true;
        for(auto it = glyphs.begin(); success && it != glyphs.end(); ++it) {
            success =  contents.find(*it) != contents.end();
        }
        return success;
    }
    
    /**
     * Returns true if this glyph run contains all of the given glyphs.
     *
     * The string is converted into a glyph sequence using UTF8 decoding.
     * If just one glyph in the given vector is missing, this returns false.
     * This method depends on the value of the attribute contents.  It is
     * the reponsibility of the user to ensure that contents matches the
     * data in mesh.
     *
     * @param glyphs The glyphs as a UTF8 string
     *
     * @return true if this glyph run contains all of the given glyphs.
     */
    bool contains(const std::string glyphs) {
        std::string charset = glyphs;
        std::vector<Uint32> utf32;
        utf8::utf8to16(charset.begin(), charset.end(), back_inserter(utf32));
        return contains(utf32);
    }
};

}

#endif /* __CU_GLYPH_RUN_H__ */
