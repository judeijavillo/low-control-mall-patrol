//
//  CUScene2Texture.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides support for writing the results of a scene graph to
//  a texture. This is very helpful for doing simple multi-pass rendering.
//  You can render to a texture, and then post-process that texture
//  a second pass.
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
//  Version: 1/25/21
//

#include <cugl/scene2/CUScene2Texture.h>

using namespace cugl;

/**
 * Creates a new degenerate Scene2Texture on the stack.
 *
 * The scene has no camera and must be initialized.
 *
 * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
 * the heap, use one of the static constructors instead.
 */
Scene2Texture::Scene2Texture() : Scene2() {
}
  
/**
 * Disposes all of the resources used by this scene.
 *
 * A disposed Scene2Texture can be safely reinitialized. Any children owned
 * by this scene will be released. They will be deleted if no other object
 * owns them.
 */
void Scene2Texture::dispose() {
    Scene2::dispose();
    _target = nullptr;
    _texture = nullptr;
}
    
/**
 * Initializes a Scene2Texture with the given viewport.
 *
 * Offseting the viewport origin has little affect on the Scene in general.
 * It only affects the coordinate conversion methods {@link Camera#project()}
 * and {@link Camera#unproject()}. It is supposed to represent the offset
 * of the viewport in a larger canvas.
 *
 * @param x         The viewport x offset
 * @param y         The viewport y offset
 * @param width     The viewport width
 * @param height    The viewport height
 *
 * @return true if initialization was successful.
 */
bool Scene2Texture::init(float x, float y, float width, float height) {
    if (Scene2::init(x,y,width,height)) {
        _target = RenderTarget::alloc(width, height);
        _texture = (_target == nullptr ? nullptr : _target->getTexture());
        return _texture != nullptr;
    }
    return false;
}
  
#pragma mark -
#pragma mark Scene Logic
/**
 * Draws all of the children in this scene with the given SpriteBatch.
 *
 * This method assumes that the sprite batch is not actively drawing.
 * It will call both begin() and end().
 *
 * Rendering happens by traversing the the scene graph using an "Pre-Order"
 * tree traversal algorithm ( https://en.wikipedia.org/wiki/Tree_traversal#Pre-order ).
 * That means that parents are always draw before (and behind children).
 * To override this draw order, you should place an {@link OrderedNode}
 * in the scene graph to specify an alternative order.
 *
 * This will render to the offscreen texture associated with this scene.
 * That texture can then be used in subsequent render passes.
 *
 * @param batch     The SpriteBatch to draw with.
 */
void Scene2Texture::render(const std::shared_ptr<SpriteBatch>& batch) {
    Affine2 matrix = _camera->getCombined();
    matrix.scale(1, -1); // Flip the y axis for texture write
    
    _target->begin();
    batch->begin(matrix);
    batch->setSrcBlendFunc(_srcFactor);
    batch->setDstBlendFunc(_dstFactor);
    batch->setBlendEquation(_blendEquation);

    for(auto it = _children.begin(); it != _children.end(); ++it) {
        (*it)->render(batch, Affine2::IDENTITY, _color);
    }

    batch->end();
    _target->end();
}
