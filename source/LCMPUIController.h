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
    /** Reference to the node containing the settings button */
    std::shared_ptr<cugl::scene2::Button> _settingsButton;

    // References to the settings menu
    /** Reference to the node containing the settings menu */
    std::shared_ptr<cugl::scene2::SceneNode> _settingsMenu;
    /** Reference to the node containing the sounds button */
    std::shared_ptr<cugl::scene2::Button> _soundsButton;
    /** Reference to the node containing the stats button */
    std::shared_ptr<cugl::scene2::Button> _statsButton;
    /** Reference to the node containing the quit button */
    std::shared_ptr<cugl::scene2::Button> _quitButton;
    /** Reference to the node containing the close button */
    std::shared_ptr<cugl::scene2::Button> _closeButton;

        
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
    std::shared_ptr<cugl::scene2::SceneNode> _victoryNode;
    std::shared_ptr<cugl::scene2::Label> _victoryText;
    
    /** A reference to the node for displaying time remaining */
    std::shared_ptr<cugl::scene2::PolygonNode> _timer;
    std::shared_ptr<cugl::scene2::PolygonNode> _hourHand;
    std::shared_ptr<cugl::scene2::PolygonNode> _minuteHand;
    std::shared_ptr<cugl::Texture> _hourTexture;
    std::shared_ptr<cugl::Texture> _minuteTexture;
    
    /** A model to represent all models within the game */
    std::shared_ptr<GameModel> _game;
    /** The asset manager for this game mode */
    std::shared_ptr<cugl::AssetManager> _assets;
    /** A reference to the font style for the UI node */
    std::shared_ptr<cugl::Font> _font;
    /** The actual size of the display. */
    cugl::Size _screenSize;
    /** The amount to move the world node by to center it in the scene */
    cugl::Vec2 _offset;
    /** The PolyFactory instance */
    cugl::PolyFactory _pf;
    
    // Information to be sent to GameScene
    /** Whether or not the game has been quit. */
    bool _didQuit;
    /** Whether or not the game is being sent to the pause menu. */
    bool _didPause;


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
              cugl::Vec2 offset,
              const std::shared_ptr<cugl::AssetManager>& assets);
        
//  MARK: - Methods
    
    /**
     * Updates the UI Controller
     */
    void update(float timestep, bool isThief, cugl::Vec2 movement,
                bool didPress, cugl::Vec2 origin, cugl::Vec2 position, int copID,
                float gameTime, bool isThiefWin);

//  MARK: - Getters

    /**
     * Gets whether or not the game is being quit. 
     */
    bool didQuit() { return _didQuit; }
    /**
     * Gets whether or not the game is being paused.
     */
    bool didPause() { return _didPause; }

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
     * Creates the timer nodes and adds them the UI node
     */
    void initTimer();
    
    /**
     * Updates the minute and hour hand nodes
     */
    void updateTimer(float time);

    /**
     * Creates the settings button.
     */
    void initSettingsButton();

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
    void updateDirecIndicators(bool isThief, int copID);

    /**
    * Update single directional indicator (this is a helper)
    * Vec2 pos1 = the origin of the directional vector
    * This is equal to the player's position.
    * Vec2 pos2 = the end of the directional vector
    * This is equal to the position of the character we want the direction to.
    * Vec2 screenPos = the screen coordinates of the end of the directional vector.
    * int index = the index of the directional indicator within the map.
    */
    void updateDirecIndicatorHelper(cugl::Vec2 pos1, cugl::Vec2 pos2, 
        cugl::Vec2 screenPos1, cugl::Vec2 screenPos2, bool isThief, int index);


    /**
     * Updates the thief indicator
     */
    void updateThiefIndicator(int copID);

    /**
     * Updates the victory label
     */
    void updateMessage(bool isThief, bool isThiefWin);
        
};

#endif /* __LCMP_UI_CONTROLLER_H__ */
