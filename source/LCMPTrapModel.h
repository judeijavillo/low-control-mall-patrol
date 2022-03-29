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
#include "LCMPConstants.h"

class TrapModel {
public:
    //  MARK: - Enumerations

    /** The different types of traps */
    enum TrapType {
        VelMod,
        Directional_VelMod,
        Moving_Platform,
        Teleport
    };

    //  MARK: - Effect Class
    class Effect {
    public:
        /** The type of effect that will be acted on the player */
        TrapType traptype;

        /** Currently all effects are representable using a single Vec2 */
        std::shared_ptr <cugl::Vec2> effectVec;

        //	MARK: - Constructors

        /**
         * Constructs a Trap Model
         */
        Effect(){};

        /**
         * Initializes an Effect
         */
        bool init(TrapType type, std::shared_ptr<cugl::Vec2> effect);

        /**
         * Destructs a Trap Model
         */
        ~Effect() { dispose(); };

        /**
        * Disposes of all resources in use by this instance of Effect
        */
        void dispose() {};



    };



protected:
//  MARK: - Properties
    
    /** Unique id of this trap*/
    int _trapID;
    /** Area affected by trap */
    std::shared_ptr<cugl::physics2::SimpleObstacle> thiefEffectArea;
    std::shared_ptr<cugl::physics2::SimpleObstacle> copEffectArea;
    /** Area within which thief can activate trap*/
    std::shared_ptr<cugl::physics2::SimpleObstacle> triggerArea;
    /** Area within which cop can deactivate trap*/
    std::shared_ptr<cugl::physics2::SimpleObstacle> deactivationArea;
	/** Position of the unactivated trap texture */
	std::shared_ptr<cugl::Vec2> triggerPos;
	/** Whether cops should collide with this trap*/
	bool copCollide;
	/** Whether thief should collide with this trap */
	bool thiefCollide;
	/** Number of times this trap can be used on a player. -1 to indicate infinite uses*/
	int usesRemaining;
	/** Thief Effect */
	std::shared_ptr<Effect> thiefEffect;
    /** Cop Effect */
    std::shared_ptr<Effect> copEffect;
    /** Thief Linger Effect */
    std::shared_ptr<Effect> thiefLingerEffect;
    /** Cop Linger Effect */
    std::shared_ptr<Effect> copLingerEffect;

    /** Defining the filter bits for the thief effect trap model*/
    b2Filter thiefEffectFilter;
    /** Defining the filter bits for the cop effect trap model*/
    b2Filter copEffectFilter;
    /** Defining the filter bits for the trigger trap model*/
    b2Filter triggerFilter;
    /** Defining the filter bits for the deactivation area model*/
    b2Filter deactivationFilter;
    
    /** The texture of the trigger prior to being activated */
    std::shared_ptr<cugl::Texture> _triggerTexture;
    /** The texture of the trigger after being activated */
    std::shared_ptr<cugl::Texture> _triggerActivatedTexture;
    /** The texture of the effect area*/
    std::shared_ptr<cugl::Texture> _effectAreaTexture;
    
    /** the lingering duration for the cop linger effect*/
    float copLingerDuration;
    /** the lingering duration for the thief linger effect*/
    float thiefLingerDuration; 

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
    


        

//	MARK: - Constructors

	/**
	 * Constructs a Trap Model
	 */
	TrapModel(){};

	/**
	 * Destructs a Trap Model
	 */
	~TrapModel() { dispose(); }; 

    /**
     * Disposes of all resources in use by this instance of Trap Model
     */
	void dispose() {};
    
    /**
     * Initializes a Trap Model
     */
    bool init(int trapID,
              const std::shared_ptr<cugl::physics2::SimpleObstacle> thiefEffectArea, const std::shared_ptr<cugl::physics2::SimpleObstacle> copEffectArea,
              const std::shared_ptr<cugl::physics2::SimpleObstacle> triggerArea, const std::shared_ptr<cugl::physics2::SimpleObstacle> deactivationArea,
              const std::shared_ptr<cugl::Vec2> triggerPosition,
              bool copSolid, bool thiefSolid,
              int numUses,
              float copLingerDuration,
              float thiefLingerDuration,
              std::shared_ptr<Effect> copEffect,
              std::shared_ptr<Effect> ThiefEffect,
              std::shared_ptr<Effect> copLingerEffect, 
              std::shared_ptr<Effect> thiefLingerEffect );


//	MARK: - Methods
    
    /**
     * Returns the appropriate key for the texture of the trigger for the trap
     */
    std::string getTriggerKey(TrapType t){

        //TODO: Instead if using a key bound to the traptype, have it linked to some predefined map of int keys to textures
        return "bucket";
        /**
        
        
        switch (t) {
        case MopBucket:
            return "bucket";
        default:
            return "asdf";
        } */
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
     * Returns the Trigger Area
     */
    std::shared_ptr<cugl::physics2::SimpleObstacle> getTriggerArea() { return triggerArea; };

    /**
     * Returns the deactivation Area
     */
    std::shared_ptr<cugl::physics2::SimpleObstacle> getDeactivationArea() { return deactivationArea; };

    /**
     * Returns the Thief Effect Area
     */
    std::shared_ptr<cugl::physics2::SimpleObstacle> getThiefEffectArea() { return thiefEffectArea; };

    /**
    * Returns the Cop Effect Area
    */
    std::shared_ptr<cugl::physics2::SimpleObstacle> getCopEffectArea() { return copEffectArea; };
    
    /**
    * Returns the thief effect 
    */
    std::shared_ptr<Effect> getThiefEffect() { return thiefEffect; };

    /**
    * Returns the cop effect
    */
    std::shared_ptr<Effect> getCopEffect() { return copEffect; };
    
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

    // Legacy Code

	/**
	 * Changes the current thief velocity in place given the trap's affine matrix
	 * 
	 * @param velocity	current thief velocity
	 * @return a referece to the modified Vec2 for chaining
	 */
    //cugl::Vec2& changeThiefVelocity(cugl::Vec2& velocity) { return velocity.set(thiefVelocityModifier->transform(velocity)); };

	/**
	 * Changes the current cop velocity in place given the trap's affine matrix
	 *
	 * @param velocity	current cop velocity
	 * @return a referece to the modified Vec2 for chaining
	*/
	//cugl::Vec2& changeCopVelocity(cugl::Vec2& velocity) { return velocity.set(copVelocityModifier->transform(velocity)); };
    
    /**
     * Returns true and decrements remaining uses if trap can be used. Returns false and has no effect otherwise.
     */
    bool use();
    
    /**
     * Activates this trap.
     */
    void activate();


    /**
    * Deactivates this trap.
    */
    void deactivate();
};

#endif /* __LCMP_TRAP_MODEL_H__ */ 
