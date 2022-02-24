//
//  LCMPThiefModel.h
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#ifndef __LCMP_THIEF_MODEL_H__
#define __LCMP_THIEF_MODEL_H__
#include "LCMPPlayerModel.h"
#include <cugl/cugl.h>

/** The texture for the character avatar */
#define THIEF_TEXTURE    "thief"

class ThiefModel : public PlayerModel {
protected:
	/** The scene graph node for the cop. */
	std::shared_ptr<cugl::scene2::SceneNode> _thiefNode;

public:
	
    /**
    * Returns the scene graph node representing this thief.
    *
    * By storing a reference to the scene graph node, the model can update
    * the node to be in sync with the physics info. It does this via the
    * {@link Obstacle#update(float)} method.
    *
    * @return the scene graph node representing this thief.
    */
    const std::shared_ptr<cugl::scene2::SceneNode>& getThiefNode() const { return _thiefNode; }

    /**
     * Sets the scene graph node representing this thief.
     *
     * By storing a reference to the scene graph node, the model can update
     * the node to be in sync with the physics info. It does this via the
     * {@link Obstacle#update(float)} method.
     *
     * If the animation nodes are not null, this method will remove them from
     * the previous scene and add them to the new one.
     *
     * @param node  The scene graph node representing this thief.
     */
    void setThiefNode(const std::shared_ptr<cugl::scene2::SceneNode>& node);

    //  MARK: - Static Constructors
    /**
     * Returns a newly allocated thief with the given position and size
     *
     * The player size is specified in world coordinates.
     *
     * The scene graph is completely decoupled from the physics system.
     * The node does not have to be the same size as the physics body. We
     * only guarantee that the scene graph node is positioned correctly
     * according to the drawing scale.
     *
     * @param pos   Initial position in world coordinates
     * @param size  The dimensions of the box.
     *
     * @return a newly allocated thief with the given position
     */
    static std::shared_ptr<ThiefModel> alloc(const cugl::Vec2& pos, const cugl::Size& size) {
        std::shared_ptr<ThiefModel> result = std::make_shared<ThiefModel>();
        return (result->init(pos, size) ? result : nullptr);
    }

	void update(float delta) override;

	void applyForce() override;

	void setMovement(cugl::Vec2 value) override;
};

#endif /* __LCMP_THIEF_MODEL_H__ */
