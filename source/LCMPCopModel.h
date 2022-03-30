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
#include "LCMPConstants.h"

/** Defining the filter bits for the cop model*/
#define COP_FILTER_BITS 0b01001




class CopModel : public PlayerModel {
protected:
    std::shared_ptr<cugl::scene2::PolygonNode> _character;
    std::shared_ptr<cugl::Texture> _tackleDownTexture;
    std::shared_ptr<cugl::Texture> _tackleUpTexture;
    std::shared_ptr<cugl::Texture> _tackleRightTexture;
    std::shared_ptr<cugl::Texture> _tackleLeftTexture;
    std::shared_ptr<cugl::Texture> _landDownTexture;
    std::shared_ptr<cugl::Texture> _landUpTexture;
    std::shared_ptr<cugl::Texture> _landRightTexture;
    std::shared_ptr<cugl::Texture> _landLeftTexture;
    
    cugl::Vec2 _tackleDirection;
    cugl::Vec2 _tacklePosition;
    float _tackleTime;
    bool _tackling;
    bool _caughtThief;
    bool _tackleSuccessful;

public:
    bool didTackle;
    bool didLand;
//  MARK: - Constructors
    
    /** Flag for whether cop is in range to deactivate a trap. -1 indicates out of range, otherwise it indicates the id of the trap the cop will deactivate */
    int trapDeactivationFlag = -1;

    /**
     * Constructs a Cop Model
     */
    CopModel() {};
    
    /**
     * Destructs a Cop Model
     */
    ~CopModel() { dispose(); }
    
    /**
     * Disposes of all resources in this instance of Cop Model
     */
    void dispose();
    
    /**
     * Initializes a Cop Model
     */
    bool init(int copID, float scale,
              const std::shared_ptr<cugl::scene2::SceneNode>& node,
              const std::shared_ptr<cugl::AssetManager>& assets,
              std::shared_ptr<cugl::scene2::ActionManager>& actions);
    
//  MARK: - Methods
    
    /**
     * Returns the damping constant
     */
    cugl::Vec2 getDamping() override { return cugl::Vec2( COP_DAMPING_DEFAULT * _dampingMultiplier.x, COP_DAMPING_DEFAULT * _dampingMultiplier.y); }
    
    /**
     * Returns the max speed of a cop
     */
    float getMaxSpeed() override { return COP_MAX_SPEED_DEFAULT * _maxSpeedMultiplier; }
    
    /**
     * Returns the acceleration of a cop
     */
    cugl::Vec2 getAcceleration() override { return cugl::Vec2(COP_ACCELERATION_DEFAULT * _accelerationMultiplier.x, COP_ACCELERATION_DEFAULT * _accelerationMultiplier.y); }
    
    /**
     * Returns the direction of the tackle
     */
    cugl::Vec2 getTackleDirection() { return _tackleDirection; }
    
    /**
     * Returns the initial position of the tackle
     */
    cugl::Vec2 getTacklePosition() { return _tacklePosition; }
    
    /**
     * Returns the amount of time spent tackling so far
     */
    float getTackleTime() { return _tackleTime; }
    
    /**
     * Returns whether this cop is currently tackling
     */
    bool getTackling() { return _tackling; }
    
    /**
     * Returns whether this cop caught the thief
     */
    bool getCaughtThief() { return _caughtThief; }
    
    /**
     * Returns whether the tackle was successful
     */
    bool getTackleSuccessful() { return _tackleSuccessful; }
    
    /**
     * Sets whether this cop caught the thief
     */
    void setCaughtThief(bool value) { _caughtThief = value; }

    /**
     * Attempts to tackle the thief and sets the appropriate properties depending on success/failure
     */
    void attemptTackle(cugl::Vec2 thiefPosition, cugl::Vec2 tackle);
    
    /**
     * Applies physics to cop when tackling
     */
    void applyTackle(float timestep, cugl::Vec2 thiefPosition);
    
    /**
     * Updates the position and velocity of the cop, applies forces, and updates tackle properties
     */
    void applyNetwork(cugl::Vec2 position, cugl::Vec2 velocity,
                      cugl::Vec2 force, cugl::Vec2 tackleDirection,
                      cugl::Vec2 tacklePosition,
                      float tackleTime,
                      bool tackling,
                      bool caughtThief,
                      bool tackleSuccessful);
    
    /**
     * Performs a film strip action
     */
    void playAnimation();
    
private:
//  MARK: - Helpers
    
    /**
     * Applies physics during a failed tackle
     */
    void applyTackleFailure();
    
    /**
     * Applies physics during a successful tackle
     */
    void applyTackleSuccess(cugl::Vec2 thiefPosition);
    
    /**
     * Updates nodes to show tackle animation
     */
    void playTackle();

};

#endif /* __LCMP_COP_MODEL_H__ */
