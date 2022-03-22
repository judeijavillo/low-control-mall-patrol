//
//  LCMPUIController.h
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 3/20/22
//

#ifndef __LCMP_UI_CONTROLLER_H__
#define __LCMP_UI_CONTROLLER_H__
#include <cugl/cugl.h>
#include "LCMPGameModel.h"

class UIController {
//  MARK: - Properties
    
    // Top-level nodes
    /** Reference to the physics node of the scene graph */
    std::shared_ptr<cugl::scene2::SceneNode> _worldnode;
    /** Reference to the debug node of the scene graph */
    std::shared_ptr<cugl::scene2::SceneNode> _uinode;
    
    // Sub-level nodes
    /** Reference to the node containing the joystick */
    std::shared_ptr<cugl::scene2::SceneNode> _joystickNode;
    /** Reference to the node containing the accelerometer visualization */
    std::shared_ptr<cugl::scene2::SceneNode> _accelVisNode;
    /** Reference to the node containing the direcional indicators */
    std::shared_ptr<cugl::scene2::SceneNode> _direcIndicatorsNode;
    /** Reference to the node containing the thief indicator */
    std::shared_ptr<cugl::scene2::SceneNode> _thiefIndicatorNode;
    
    // Joystick
    /** A view of the part of the joystick indicating what the player is controlling */
    std::shared_ptr<cugl::scene2::PolygonNode> _innerJoystick;
    /** A view of the part of the joystick indicating how far the josyick can reach */
    std::shared_ptr<cugl::scene2::PolygonNode> _outerJoystick;

    // Accelerometer Visualization
    /** A view of the part of the accelerometer visualization indicating how far the player can tilt the phone */
    std::shared_ptr<cugl::scene2::PolygonNode> _outerAccelVis;
    /** A view of the part of the accelerometer visualization indicating the current tilt of the phone */
    std::shared_ptr<cugl::scene2::PolygonNode> _innerAccelVis;

    // Directional Indicators
    /** The directional indicators for the thief that point to the cops */
    std::unordered_map<int, std::shared_ptr<cugl::scene2::PolygonNode>> _direcIndicators;
    
    // Thief Indicators
    /** A reference to the label for displaying thief distance for cop */
    std::shared_ptr<cugl::scene2::Label> _thiefIndicator;
    
    // Victory/Defeat Message
    /** A reference to the label for displaying the vicory/defeat meesage */
    std::shared_ptr<cugl::scene2::SceneNode> _message;
    
    /** A model to represent all models within the game */
    std::shared_ptr<GameModel> _game;
    /** A reference to the font style for the UI node */
    std::shared_ptr<cugl::Font> _font;
    /** The actual size of the display. */
    cugl::Size _screenSize;
    /** The amount to move the world node by to center it in the scene */
    cugl::Vec2 _offset;
    /** The PolyFactory instance */
    cugl::PolyFactory _pf;

public:
//  MARK: - Constructors
    
    /**
     * Constructs a UI Controller
     */
    UIController() {};
    
    /**
     * Destructs a UI Controller
     */
    ~UIController() { dispose(); }
    
    /**
     * Disposes of all resources in this instance of UI Controller
     */
    void dispose();
    
    /**
     * Initializes a UI Controller
     */
    bool init(const std::shared_ptr<cugl::scene2::SceneNode> worldnode,
              const std::shared_ptr<cugl::scene2::SceneNode> uinode,
              const std::shared_ptr<GameModel> game,
              const std::shared_ptr<cugl::Font> font,
              cugl::Size screenSize,
              cugl::Vec2 offset);
        
//  MARK: - Methods
    
    /**
     * Updates the UI Controller
     */
    void update(float timestep, bool isThief, cugl::Vec2 movement,
                bool didPress, cugl::Vec2 origin, cugl::Vec2 position, int copID);

    
private:
//  MARK: - Helpers
    
    /**
     * Creates the necessary nodes for showing the joystick and adds them to the joystick node.
     */
    void initJoystick();

    /**
     * Creates the necessary nodes for the accelerometer visualization and adds them to the accelerometer
     * visualization node.
     */
    void initAccelVis();

    /**
     * Creates directional indicators for the thief that point towards the cops and adds them to the directional
     * indicators node.
     */
    void initDirecIndicators();

    /**
     * Creates a thief indicator for the cops that shows how far they are from the thief and adds it to the
     * thief indicator node.
     */
    void initThiefIndicator();
    
    /**
     * Creates the message label and adds it the UI node
     */
    void initMessage();
    
    /**
     * Updates the joystick
     */
    void updateJoystick(bool didPress, cugl::Vec2 origin, cugl::Vec2 position);
    
    /**
     * Updates the accelerometer visualization
     */
    void updateAccelVis(cugl::Vec2 movement);

    /**
     * Updates directional indicators
     */
    void updateDirecIndicators();

    /**
     * Updates the thief indicator
     */
    void updateThiefIndicator(int copID);

    /**
     * Updates the message label
     */
    void updateMessage();
        
};

#endif /* __LCMP_UI_CONTROLLER_H__ */
