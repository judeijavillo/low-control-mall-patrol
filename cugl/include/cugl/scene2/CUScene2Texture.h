//
//  CUScene2Texture.h
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
#ifndef __CU_SCENE_2_TEXTURE_H__
#define __CU_SCENE_2_TEXTURE_H__
#include <cugl/scene2/CUScene2.h>
#include <cugl/render/CUTexture.h>
#include <cugl/render/CURenderTarget.h>
#include <memory>

namespace cugl {

/**
 * This class provides the root node of an offscreen scene graph.
 *
 * This subclass of {@link Scene2} supports offscreen rendering to a texture.
 * It has its own {@link RenderTarget}, which is what it uses to render to.
 * You can then access the result of the render with {@link #getTexture()}.
 * The rendering process ensures that the origin of the scene is rendered
 * to the bottom left corner of the texture (and not the top right, as is
 * the default in OpenGL), making it consist with sprite-based images used
 * by the scene graph.
 *
 * As a result, this class provides support for simple multi-pass rendering.
 * Simply render to a scene to a texture in one pass, and then use that
 * texture in future passes.
 */
class Scene2Texture : public Scene2 {
protected:
    /** The texture created by this scene */
    std::shared_ptr<Texture> _texture;
    
    /** The offscreen buffer for rendering the texture. */
    std::shared_ptr<RenderTarget> _target;

#pragma mark Constructors
public:
    /**
     * Creates a new degenerate Scene2Texture on the stack.
     *
     * The scene has no camera and must be initialized.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
     * the heap, use one of the static constructors instead.
     */
    Scene2Texture();
        
    /**
     * Deletes this scene, disposing all resources
     */
    ~Scene2Texture() { dispose(); }
  
    /**
     * Disposes all of the resources used by this scene.
     *
     * A disposed Scene2Texture can be safely reinitialized. Any children owned
     * by this scene will be released. They will be deleted if no other object
     * owns them.
     */
    virtual void dispose() override;
    
    /**
     * Initializes a Scene2Texture with the given viewport.
     *
     * The viewport origin is assumed to be (0,0).
     *
     * @param size      The viewport size
     *
     * @return true if initialization was successful.
     */
    bool init(const Size size) {
        return init(0,0,size.width,size.height);
    }
    
    /**
     * Initializes a Scene2Texture with the given viewport.
     *
     * The viewport origin is assumed to be (0,0).
     *
     * @param width     The viewport width
     * @param height    The viewport height
     *
     * @return true if initialization was successful.
     */
    bool init(float width, float height) {
        return init(0,0,width,height);
    }
    
    /**
     * Initializes a Scene2Texture with the given viewport.
     *
     * Offseting the viewport origin has little affect on the scene in general.
     * It only affects the coordinate conversion methods {@link Camera#project()}
     * and {@link Camera#unproject()}. It is supposed to represent the offset
     * of the viewport in a larger canvas.
     *
     * @param rect      The viewport bounding box
     *
     * @return true if initialization was successful.
     */
    bool init(const Rect rect) {
        return init(rect.origin.x,rect.origin.y,
                    rect.size.width, rect.size.height);
    }
    
    /**
     * Initializes a Scene2Texture with the given viewport.
     *
     * Offseting the viewport origin has little affect on the scene in general.
     * It only affects the coordinate conversion methods {@link Camera#project()}
     * and {@link Camera#unproject()}. It is supposed to represent the offset
     * of the viewport in a larger canvas.
     *
     * @param origin    The viewport offset
     * @param size      The viewport size
     *
     * @return true if initialization was successful.
     */
    bool init(const Vec2 origin, const Size size) {
        return init(origin.x,origin.y,size.width, size.height);
    }
    
    /**
     * Initializes a Scene2Texture with the given viewport.
     *
     * Offseting the viewport origin has little affect on the scene in general.
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
    virtual bool init(float x, float y, float width, float height) override;

#pragma mark Static Constructors
    /**
     * Returns a newly allocated Scene2Texture for the given viewport.
     *
     * The viewport origin is assumed to be (0,0).
     *
     * @param size      The viewport size
     *
     * @return a newly allocated Scene for the given viewport.
     */
    static std::shared_ptr<Scene2Texture> alloc(const Size size) {
        std::shared_ptr<Scene2Texture> result = std::make_shared<Scene2Texture>();
        return (result->init(size) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated Scene2Texture for the given viewport.
     *
     * The viewport origin is assumed to be (0,0).
     *
     * @param width     The viewport width
     * @param height    The viewport height
     *
     * @return a newly allocated Scene for the given viewport.
     */
    static std::shared_ptr<Scene2Texture> alloc(float width, float height) {
        std::shared_ptr<Scene2Texture> result = std::make_shared<Scene2Texture>();
        return (result->init(width,height) ? result : nullptr);
    }

    /**
     * Returns a newly allocated Scene2Texture for the given viewport.
     *
     * Offseting the viewport origin has little affect on the Scene in general.
     * It only affects the coordinate conversion methods {@link Camera#project()}
     * and {@link Camera#unproject()}. It is supposed to represent the offset
     *
     * @param rect      The viewport bounding box
     *
     * @return a newly allocated Scene for the given viewport.
     */
    static std::shared_ptr<Scene2Texture> alloc(const Rect rect) {
        std::shared_ptr<Scene2Texture> result = std::make_shared<Scene2Texture>();
        return (result->init(rect) ? result : nullptr);
    }

    /**
     * Returns a newly allocated Scene2Texture for the given viewport.
     *
     * Offseting the viewport origin has little affect on the Scene in general.
     * It only affects the coordinate conversion methods {@link Camera#project()}
     * and {@link Camera#unproject()}. It is supposed to represent the offset
     *
     * @param origin    The viewport offset
     * @param size      The viewport size
     *
     * @return a newly allocated Scene for the given viewport.
     */
    static std::shared_ptr<Scene2Texture> alloc(const Vec2 origin, const Size size) {
        std::shared_ptr<Scene2Texture> result = std::make_shared<Scene2Texture>();
        return (result->init(origin,size) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated Scene2Texture for the given viewport.
     *
     * Offseting the viewport origin has little affect on the Scene in general.
     * It only affects the coordinate conversion methods {@link Camera#project()}
     * and {@link Camera#unproject()}. It is supposed to represent the offset
     *
     * @param x         The viewport x offset
     * @param y         The viewport y offset
     * @param width     The viewport width
     * @param height    The viewport height
     *
     * @return a newly allocated Scene for the given viewport.
     */
    static std::shared_ptr<Scene2Texture> alloc(float x, float y, float width, float height) {
        std::shared_ptr<Scene2Texture> result = std::make_shared<Scene2Texture>();
        return (result->init(x,y,width,height) ? result : nullptr);
    }
    
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
     * To override this draw order, you should place an {@link scene2::OrderedNode}
     * in the scene graph to specify an alternative order.
     *
     * This will render to the offscreen texture associated with this scene.
     * That texture can then be used in subsequent render passes.
     *
     * @param batch     The SpriteBatch to draw with.
     */
    virtual void render(const std::shared_ptr<SpriteBatch>& batch) override;

    /**
     * Returns the texture associated with this scene graph.
     *
     * Rendering this scene graph will draw to the offscreen texture. This method
     * returns that texture so that it can be used in subsequent passes.
     *
     * @return the texture associated with this scene graph.
     */
    std::shared_ptr<Texture> getTexture() const {
        return _texture;
    }
};

}

#endif /* __CU_SCENE_2_TEXTURE_H__ */
