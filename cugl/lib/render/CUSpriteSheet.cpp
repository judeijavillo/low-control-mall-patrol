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
#include <cugl/render/CUSpriteSheet.h>
#include <cugl/render/CUSpriteBatch.h>
#include <cugl/render/CUTexture.h>

using namespace cugl;

#pragma mark Constructors

/**
 * Creates a degenerate sprite sheet with no frames.
 *
 * You must initialize the sheet before using it.
 */
SpriteSheet::SpriteSheet() :
_size(0),
_cols(0),
_frame(0) {
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
bool SpriteSheet::init(const std::shared_ptr<Texture>& texture, int rows, int cols, int size) {
    _texture = texture;
    _cols = cols;
    _size = size;
    _bounds.size = texture->getSize();
    _bounds.size.width /= cols;
    _bounds.size.height /= rows;
    _region.set(_bounds);
    return true;
}

/**
 * Deletes the sprite sheet and resets all attributes.
 *
 * You must reinitialize the sprite batch to use it.
 */
void SpriteSheet::dispose() { }

#pragma mark Drawing Commands

/**
 * Sets the active frame as the given index.
 *
 * If the frame index is invalid, an error is raised.
 *
 * @param frame the index to make the active frame
 */
void SpriteSheet::setFrame(int frame) {
    _frame = frame;
    float x = (frame % _cols)*_bounds.size.width;
    float y = _texture->getSize().height - (1+frame/_cols)*_bounds.size.height;
    Vec2 offset(x,y);
    offset -= _bounds.origin;
    _region += offset;
    _bounds.origin.set(x,y);

}


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
void SpriteSheet::draw(const std::shared_ptr<cugl::SpriteBatch>& batch,
                       const cugl::Affine2& transform) const {
    batch->draw(_texture,_region,_bounds.origin+_origin,transform);
}

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
void SpriteSheet::draw(const std::shared_ptr<cugl::SpriteBatch>& batch,
                       cugl::Color4 color, const cugl::Affine2& transform) const {
    batch->draw(_texture,color,_region,_bounds.origin+_origin,transform);
}


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
void SpriteSheet::draw(const std::shared_ptr<cugl::SpriteBatch>& batch,
                       Vec2 origin, const cugl::Affine2& transform) const {
    batch->draw(_texture,_region,_bounds.origin+origin,transform);
}

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
void SpriteSheet::draw(const std::shared_ptr<cugl::SpriteBatch>& batch,
                       cugl::Color4 color, Vec2 origin, const cugl::Affine2& transform) const {
    batch->draw(_texture,color,_region,_bounds.origin+_origin,transform);
}

