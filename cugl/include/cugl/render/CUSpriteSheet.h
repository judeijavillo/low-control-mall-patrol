//
//  CUSpriteSheet.h
//  Cornell University Game Library (CUGL)
//
//  This module provides a class that support sprite sheet animation, when
//  combined with a sprite batch. This is to allow the user to create simple
//  animations without the use of a scene graph.
//
//  While it is possible to swap out the shader for this class, the shader is
//  very peculiar in how it uses uniforms.  You should study SpriteShader.frag
//  and SpriteShader.vert before making any shader changes to this class.
//
//  This class uses our standard shared-pointer architecture.
//
//  1. The constructor does not perform any initialization; it just sets all
//     attributes to their defaults.
//
//  2. All initialization takes place via init methods, which can fail if an
//     object is initialized more than once.
//
//  3. All allocation takes place via static constructors which return a shared
//     pointer.
//
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
//  Version: 1/20/22
//

#ifndef __CU_SPRITE_SHEET_H__
#define __CU_SPRITE_SHEET_H__
#include <cugl/math/CUVec2.h>
#include <cugl/math/CURect.h>
#include <cugl/math/CUPoly2.h>
#include <cugl/math/CUColor4.h>


namespace cugl {

// Forward class references
class Texture;
class SpriteBatch;

/**
 * This class breaks a sprite sheet into frames for animation.
 *
 * This class is an alternative for {@link scene2::SpriteNode} for developers that
 * do not want to use scene graphs in their implementation. Like that class, it
 * allows the user to manage the current frame of the sprite sheet.  All drawing
 * details are handled by the class, greatly simplifying the animation.
 *
 * Note that this is a stateful class with a mutable attribute ({@link #setFrame}).
 * That means this class is **not** an asset, and should not be loaded as such.
 * The underlying texture is an asset. Multiple objects could all share the same
 * sprite sheet texture.  But as all of these objects may be at different animation
 * frames, they each need their own sprite sheet.
 *
 * You cannot change the texture or size of a sprite sheet.  If you need to change
 * the animation source, you should make a new sprite sheet object.
 */
class SpriteSheet {
protected:
    std::shared_ptr<cugl::Texture> _texture;
    /** The number of columns in this sprite sheet */
    int _cols;
    /** The number of frames in this sprite sheet */
    int _size;
    /** The active animation frame */
    int _frame;
    /** The default transform origin of this sprite sheet */
    Vec2 _origin;
    /** The bounds of the current frame in the sprite sheet */
    Rect _bounds;
    /** The display region for animation */
    Poly2 _region;

public:
    /**
     * Creates a degenerate sprite sheet with no frames.
     *
     * You must initialize the sheet before using it.
     */
    SpriteSheet();
    
    /**
     * Deletes the sprite sheet, disposing all resources
     */
    ~SpriteSheet() { dispose(); }

    /**
     * Deletes the sprite sheet and resets all attributes.
     *
     * You must reinitialize the sprite batch to use it.
     */
    void dispose();
    
    /**
     * Initializes the sprite sheet with the given texture.
     *
     * This initializer assumes that the sprite sheet is rectangular, and that
     * there are no unused frames.
     *
     * @param texture   The texture image to use
     * @param rows      The number of rows in the sprite sheet
     * @param cols      The number of columns in the sprite sheet
     *
     * @return  true if the sprite sheet is initialized properly, false otherwise.
     */
    bool init(const std::shared_ptr<Texture>& texture, int rows, int cols) {
        return init(texture,rows,cols,rows*cols);
    }
    
    /**
     * Initializes the sprite sheet with the given texture.
     *
     * The parameter size is to indicate that there are unused frames in the
     * filmstrip.  The value size must be less than or equal to rows*cols, or
     * this constructor will raise an error.
     *
     * @param texture   The texture image to use
     * @param rows      The number of rows in the sprite sheet
     * @param cols      The number of columns in the sprite sheet
     * @param size      The number of frames in the sprite sheet
     *
     * @return  true if the sprite sheet is initialized properly, false otherwise.
     */
    bool init(const std::shared_ptr<Texture>& texture, int rows, int cols, int size);
    
#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a newly allocated sprite sheet from the given texture.
     *
     * This constructor assumes that the prite sheet is rectangular, and that
     * there are no unused frames.
     *
     * @param texture   The texture image to use
     * @param rows      The number of rows in the sprite sheet
     * @param cols      The number of columns in the sprite sheet
     *
     * @return a newly allocated sprite sheete from the given texture.
     */
    static std::shared_ptr<SpriteSheet> alloc(const std::shared_ptr<Texture>& texture,
                                              int rows, int cols) {
        std::shared_ptr<SpriteSheet> sheet = std::make_shared<SpriteSheet>();
        return (sheet->init(texture,rows,cols) ? sheet : nullptr);
    }
        
    /**
     * Returns a newly allocated filmstrip node from the given texture.
     *
     * The parameter size is to indicate that there are unused frames in the
     * filmstrip.  The value size must be less than or equal to rows*cols, or
     * this constructor will raise an error.
     *
     * The size of the node is equal to the size of a single frame in the
     * filmstrip. To resize the node, scale it up or down.  Do NOT change the
     * polygon, as that will interfere with the animation.
     *
     * @param texture   The texture image to use
     * @param rows      The number of rows in the filmstrip
     * @param cols      The number of columns in the filmstrip
     * @param size      The number of frames in the filmstrip
     *
     * @return a newly allocated filmstrip node from the given texture.
     */
    static std::shared_ptr<SpriteSheet> alloc(const std::shared_ptr<Texture>& texture,
                                              int rows, int cols, int size) {
        std::shared_ptr<SpriteSheet> sheet = std::make_shared<SpriteSheet>();
        return (sheet->init(texture,rows,cols,size) ? sheet : nullptr);
    }
    
#pragma mark -
#pragma mark Attribute Accessors
    /**
     * Returns the number of frames in this sprite sheet.
     *
     * @return the number of frames in this sprite sheet.
     */
    int getSize() const { return _size; }
    
    /**
     * Returns the current active frame.
     *
     * @return the current active frame.
     */
    unsigned int getFrame() const { return _frame; }
    
    /**
     * Sets the active frame as the given index.
     *
     * If the frame index is invalid, an error is raised.
     *
     * @param frame the index to make the active frame
     */
    void setFrame(int frame);
    
    /**
     * Returns the texture associated with this sprite sheet.
     *
     * @return the texture associated with this sprite sheet.
     */
    std::shared_ptr<Texture> getTexture() const { return _texture; }

    /**
     * Returns the size of a single animation frame.
     *
     * @return the size of a single animation frame.
     */
    Size getFrameSize() const { return _bounds.size; }

    /**
     * Returns the default origin of this sprite sheet
     *
     * The origin is the offset (in pixels) from the bottom left
     * corner of the current frame. The origin is used when
     * drawing the sprite sheet; the transform applies rotations
     * and scale operations relative to this origin. By default
     * this value is (0,0).
     *
     * @return the default origin of this sprite sheet
     */
    Vec2 getOrigin() const { return _origin; }

    /**
     * Sets the default origin of this sprite sheet
     *
     * The origin is the offset (in pixels) from the bottom left
     * corner of the current frame. The origin is used when
     * drawing the sprite sheet; the transform applies rotations
     * and scale operations relative to this origin. By default
     * this value is (0,0).
     *
     * @param origin    The default origin of this sprite sheet
     */
    void setOrigin(Vec2 origin) { _origin = origin; }

#pragma mark -
#pragma mark Drawing Commands
    /**
     * Draws this sprite sheet to the given sprite batch.
     *
     * Only the active frame will be drawn. The transform will be
     * applied to the active frame at the default origin. The
     * sprite will not be tinted.
     *
     * @param batch     The sprite batch for the drawing
     * @param transform The drawing transform
     */
    void draw(const std::shared_ptr<SpriteBatch>& batch,
              const Affine2& transform) const;

    /**
     * Draws this sprite sheet to the given sprite batch.
     *
     * Only the active frame will be drawn. The transform will be
     * applied to the active frame at the default origin. The
     * sprite will be tinted by the given color.
     *
     * @param batch     The sprite batch for the drawing
     * @param color     The tint color
     * @param transform The drawing transform
     */
    void draw(const std::shared_ptr<SpriteBatch>& batch,
              Color4 color, const Affine2& transform) const;

    
    /**
     * Draws this sprite sheet to the given sprite batch.
     *
     * Only the active frame will be drawn. The transform will be
     * applied to the active frame at the specified origin. The
     * sprite will not be tinted.
     *
     * @param batch     The sprite batch for the drawing
     * @param origin    The transform origin offset
     * @param transform The drawing transform
     */
    void draw(const std::shared_ptr<SpriteBatch>& batch,
              Vec2 origin, const Affine2& transform) const;

    /**
     * Draws this sprite sheet to the given sprite batch.
     *
     * Only the active frame will be drawn. The transform will be
     * applied to the active frame at the specified origin. The
     * sprite will be tinted by the given color.
     *
     * @param batch     The sprite batch for the drawing
     * @param color     The tint color
     * @param origin    The transform origin offset
     * @param transform The drawing transform
     */
    void draw(const std::shared_ptr<SpriteBatch>& batch,
              Color4 color, Vec2 origin, const Affine2& transform) const;

};

}

#endif /* __CU_SPRITE_SHEET_H__ */
