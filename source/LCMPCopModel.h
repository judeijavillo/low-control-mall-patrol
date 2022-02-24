//
//  LCMPCopModel.h
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#ifndef __LCMP_COP_MODEL_H__
#define __LCMP_COP_MODEL_H__
#include "LCMPPlayerModel.h"
#include <cugl/cugl.h>

//  MARK: - Drawing Constants
/** Identifier to allow us to track the sensor in ContactListener */

//  MARK: - Cop Model
class CopModel : public PlayerModel {
protected:
    /** How long until we can tackle again */
    int  _tackleCooldown;
    /** Whether we are actively tackling */
    bool _isTackling;
    /** The scene graph node for the cop. */
    std::shared_ptr<cugl::scene2::SceneNode> _copNode;

public:
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
    static std::shared_ptr<CopModel> alloc(const cugl::Vec2& pos, const cugl::Size& size) {
        std::shared_ptr<CopModel> result = std::make_shared<CopModel>();
        return (result->init(pos, size) ? result : nullptr);
    }

//  MARK: - Constructors
    
    virtual bool init(const cugl::Vec2 pos, const cugl::Size size);
    
    virtual void update(float delta);
    
    void setCopNode(const std::shared_ptr<cugl::scene2::SceneNode>& node);

    virtual void applyForce();
};

#endif /* __LCMP_COP_MODEL_H__ */
