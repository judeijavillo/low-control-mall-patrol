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

/** The thrust factor to convert player input into thrust */
#define DEFAULT_THRUST 100000.0f

//  MARK: - Drawing Constants
/** Identifier to allow us to track the sensor in ContactListener */

//  MARK: - Cop Model
class CopModel : public PlayerModel {
protected:
    /** The force to apply to this rocket */
    cugl::Vec2 _force;
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
        
    virtual void update(float delta);
    
    void setCopNode(const std::shared_ptr<cugl::scene2::SceneNode>& node);
    
//  MARK: - Accessors
    
    /**
     * Returns the x-component of the force applied to the cop.
     *
     * Remember to modify the input values by the thrust amount before assigning
     * the value to force.
     *
     * @return the x-component of the force applied to the cop.
     */
    float getFX() const { return _force.x; }
    
    /**
     * Sets the x-component of the force applied to the cop.
     *
     * Remember to modify the input values by the thrust amount before assigning
     * the value to force.
     *
     * @param value the x-component of the force applied to the cop.
     */
    void setFX(float value) { _force.x = value; }
    
    /**
     * Returns the y-component of the force applied to the cop.
     *
     * Remember to modify the input values by the thrust amount before assigning
     * the value to force.
     *
     * @return the y-component of the force applied to the cop.
     */
    float getFY() const { return _force.y; }
    
    /**
     * Sets the x-component of the force applied to the cop.
     *
     * Remember to modify the input values by the thrust amount before assigning
     * the value to force.
     *
     * @param value the x-component of the force applied to the cop.
     */
    void setFY(float value) { _force.y = value; }
    
    /**
     * Returns the amount of thrust that the cop has.
     *
     * Multiply this value times the horizontal and vertical values in the
     * input controller to get the force.
     *
     * @return the amount of thrust that this cop has.
     */
    float getThrust() const { return DEFAULT_THRUST; }

    virtual void applyForce();
};

#endif /* __LCMP_COP_MODEL_H__ */
