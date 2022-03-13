//
//  LCMPTrapMode.h
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#ifndef __LCMP_TRAP_MODEL_H__ 
#define __LCMP_TRAP_MODEL_H__
#include <cugl/cugl.h>
#include "LCMPPlayerModel.h"

class TrapModel {
protected:
//  MARK: - Properties
    
    /** Unique id of this trap*/
    int _trapID;
    /** Area affected by trap */
    std::shared_ptr<cugl::physics2::SimpleObstacle> effectArea;
    /** Area within which thief can activate trap*/
    std::shared_ptr<cugl::physics2::SimpleObstacle> triggerArea;
	/** Position of the unactivated trap texture */
	std::shared_ptr<cugl::Vec2> triggerPos;
	/** Whether cops should collide with this trap*/
	bool copCollide;
	/** Whether thief should collide with this trap */
	bool thiefCollide;
	/** Number of times this trap can be used on a player. -1 to indicate infinite uses*/
	int usesRemaining;
	/** Amount of time the trap effects the player after they leave the effect bounds */
	float lingerDuration;
	/** Affine modification for thief velocity */
	std::shared_ptr<cugl::Affine2> thiefVelocityModifier;
	/** Affine modification for cop velocity */
	std::shared_ptr<cugl::Affine2> copVelocityModifier;

    /** Defining the filter bits for the effect trap model*/
    b2Filter effectFilter;
    /** Defining the filter bits for the trigger trap model*/
    b2Filter triggerFilter;
    
    /** The texture of the trigger prior to being activated */
    std::shared_ptr<cugl::Texture> _triggerTexture;
    /** The texture of the trigger after being activated */
    std::shared_ptr<cugl::Texture> _triggerActivatedTexture;
    /** The texture of the effect area*/
    std::shared_ptr<cugl::Texture> _effectAreaTexture;
    
    /** World node of the scene graph */
    std::shared_ptr<cugl::scene2::SceneNode> _node;
    /** Reference to the debug node of the scene graph */
    std::shared_ptr<cugl::scene2::SceneNode> _debugnode;
    /** Reference to the node showing the texture of the trap untriggered */
    std::shared_ptr<cugl::scene2::PolygonNode> _triggerNode;
    /** Reference to the node showing the texture of the trap triggered */
    std::shared_ptr<cugl::scene2::PolygonNode> _triggerActivatedNode;
    /** Reference to the node showing the texture of the effect area */
    std::shared_ptr<cugl::scene2::PolygonNode> _effectAreaNode;

public:
    /** Whether the trap is activated */
    bool activated;
    
public:
//  MARK: - Enumerations

    /** The different types of traps */
    enum TrapType {
        MopBucket,
        Stairs,
        Piston,
        CopWall
    };

//	MARK: - Constructors

	/**
	 * Constructs a Trap Model
	 */
	TrapModel() {};

	/**
	 * Destructs a Trap Model
	 */
	~TrapModel() { dispose(); };

    /**
     * Disposes of all resources in use by this instance of Trap Model
     */
	void dispose() {};

    /**
     * Initializes a Trap Model that doesn't start triggered
     */
    bool init(int trapID,
              const std::shared_ptr<cugl::physics2::SimpleObstacle> area,
              const std::shared_ptr<cugl::physics2::SimpleObstacle> triggerArea,
              const std::shared_ptr<cugl::Vec2> triggerPosition,
              bool copSolid, bool thiefSolid,
              int numUses,
              float lingerDur,
              const std::shared_ptr<cugl::Affine2> thiefVelMod,
              const std::shared_ptr<cugl::Affine2> copVelMod) {
        return init(trapID,
                    area,
                    triggerArea,
                    triggerPosition,
                    copSolid, thiefSolid,
                    numUses,
                    lingerDur,
                    thiefVelMod, copVelMod,
                    false);
    };
    
    /**
     * Initializes a Trap Model with components of the thief and cop effects
     */
    bool init(int trapID,
              const std::shared_ptr<cugl::physics2::SimpleObstacle> area,
              const std::shared_ptr<cugl::physics2::SimpleObstacle> triggerArea,
              const std::shared_ptr<cugl::Vec2> triggerPosition,
              bool copSolid, bool thiefSolid,
              int numUses,
              float lingerDur,
              float thiefXScale, float thiefXTrans,
              float thiefYScale, float thiefYTrans,
              float copXScale, float copXTrans,
              float copYScale, float copYTrans) {

        std::shared_ptr<cugl::Affine2> thiefy = std::make_shared<cugl::Affine2>(thiefXScale, 0, 0, thiefYScale, thiefXTrans, thiefYTrans);
        std::shared_ptr<cugl::Affine2> coppy = std::make_shared<cugl::Affine2>(copXScale, 0, 0, copYScale, copXTrans, copYTrans);

        return init(trapID,
                    area,
                    triggerArea,
                    triggerPosition,
                    copSolid, thiefSolid,
                    numUses,
                    lingerDur,
                    thiefy,
                    coppy);
    };
    
    /**
     * Initializes a Trap Model
     */
    bool init(int trapID,
              const std::shared_ptr<cugl::physics2::SimpleObstacle> area,
              const std::shared_ptr<cugl::physics2::SimpleObstacle> triggerArea,
              const std::shared_ptr<cugl::Vec2> triggerPosition,
              bool copSolid, bool thiefSolid,
              int numUses,
              float lingerDur,
              const std::shared_ptr<cugl::Affine2> thiefVelMod,
              const std::shared_ptr<cugl::Affine2> copVelMod,
              bool startsTriggered);


//	MARK: - Methods
    
    /**
     * Returns the appropriate key for the texture of the trigger for the trap
     */
    std::string getTriggerKey(TrapType t){
        switch(t){
        case MopBucket:
            return "bucket";
        default:
            return "asdf";
        }
    };
    
    /**
     * Returns the ID for this trap
     */
    int getTrapID() { return _trapID; }
    
    /**
     * Returns true if the thief can collide with this obstacle (this obstacle is solid for thief).
     */
    bool getThiefCollide() { return thiefCollide; }

    /**
     * Returns true if the cops can collide with this obstacle (this obstacle is solid for cop).
     */
    bool getCopCollide() { return copCollide; }

    /**
     * Returns the duration the trap effect lingers after leaving the trap.
     */
    float getLingerDuration() { return lingerDuration; }

    /**
     * Returns the Trigger Area
     */
    std::shared_ptr<cugl::physics2::SimpleObstacle> getTriggerArea() { return triggerArea; };

    /**
     * Returns the Effect Area
     */
    std::shared_ptr<cugl::physics2::SimpleObstacle> getEffectArea() { return effectArea; };
    
    /**
     * Sets all of the assets for this trap
     */
    void setAssets(float scale,
                    const std::shared_ptr<cugl::scene2::SceneNode>& node,
                    const std::shared_ptr<cugl::AssetManager>& assets,
                     TrapType type);
    
    /**
     * Sets the debug scene to all of the children nodes
     */
    void setDebugScene(const std::shared_ptr<cugl::scene2::SceneNode>& node);

	/**
	 * Changes the current thief velocity in place given the trap's affine matrix
	 * 
	 * @param velocity	current thief velocity
	 * @return a referece to the modified Vec2 for chaining
	 */
    cugl::Vec2& changeThiefVelocity(cugl::Vec2& velocity) { return velocity.set(thiefVelocityModifier->transform(velocity)); };

	/**
	 * Changes the current cop velocity in place given the trap's affine matrix
	 *
	 * @param velocity	current cop velocity
	 * @return a referece to the modified Vec2 for chaining
	*/
	cugl::Vec2& changeCopVelocity(cugl::Vec2& velocity) { return velocity.set(copVelocityModifier->transform(velocity)); };
    
    /**
     * Returns true and decrements remaining uses if trap can be used. Returns false and has no effect otherwise.
     */
    bool use();
    
    /**
     * Activates this trap.
     */
    void activate();

};

#endif /* __LCMP_TRAP_MODEL_H__ */
