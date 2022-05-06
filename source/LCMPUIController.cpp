//
//  LCMPUIController.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 3/20/22
//

#include "LCMPUIController.h"
#include "LCMPPauseController.h"
#include "LCMPConstants.h"
#include "LCMPAudioController.h"

using namespace std;
using namespace cugl;

//  MARK: - Constants

/** How far from the thief the directional indicators appear. */
#define DIREC_INDICATOR_DIST_SCALAR     250.0f
/** How long one side of the directional indicator is. */
#define DIREC_INDICATOR_SIZE            120.0f

/** The radius of the outer accelerometer visualization. */
#define OUTER_ACCEL_VIS_RADIUS    75.0f
/** The radius of the inner accelerometer visualization. */
#define INNER_ACCEL_VIS_RADIUS    7.0f

/** The key for the settings button texture */
#define SETTINGS_BUTTON_TEXTURE    "settings_button"
/** The key for the directional indicator arrow texture */
#define DIR_IND_ARROW_TEXTURE      "ui_arrow"

/** Define the time settings for animation */
#define ACT_KEY  "settings animation"

/** The resting position of the joystick */
float JOYSTICK_HOME[]   {200, 200};

/**
 *  The position on the screen of the accelerometer visualization.
 *  The floats represent what fraction of the screen the x and y coordinates are at.
*/
float OUTER_ACCEL_VIS_POS[2]{ 0.1f, 0.1f };

//  MARK: - Constructors

/**
 * Disposes of all resources in this instance of UI Controller
 */
void UIController::dispose() {
    _settings.dispose();
    _uinode->removeAllChildren();
}

/**
 * Initializes a UI Controller
 */
bool UIController::init(const shared_ptr<scene2::SceneNode> worldnode,
                        const shared_ptr<scene2::SceneNode> uinode,
                        const shared_ptr<GameModel> game,
                        const shared_ptr<Font> font,
                        Size screenSize,
                        Vec2 offset,
                        const std::shared_ptr<cugl::AssetManager>& assets,
                        const std::shared_ptr<cugl::scene2::ActionManager>& actions,
                        const std::shared_ptr<AudioController> audio) {
    
    // Save properties
    _worldnode = worldnode;
    _uinode = uinode;
    _game = game;
    _assets = assets;
    _screenSize = screenSize;
    _offset = offset;
    _font = font;
    _actions = actions;
    _audio = audio;
    
    // Initialize booleans
    _didQuit = false;
    _didPause = false;

    // Initialize transparency
    _transparent = Color4(255, 255, 255, (int)(255 * 0.75));

    // Set up dimen
    _dimen = _screenSize;
    _dimen *= SCENE_HEIGHT / _dimen.height;


    // Create subnodes
    _direcIndicatorsNode = scene2::SceneNode::alloc();
    _thiefIndicatorNode = scene2::SceneNode::alloc();
    _joystickNode = scene2::SceneNode::alloc();
    _accelVisNode = scene2::SceneNode::alloc();
//    _victoryNode = scene2::SceneNode::alloc();

    initTimer();
    initElementsNode();

    initThiefIndicator();

    // Add nodes to the top-level nodes
    _uinode->addChild(_direcIndicatorsNode);
//    _uinode->addChild(_victoryNode);
    _uinode->addChild(_joystickNode);
    _uinode->addChild(_accelVisNode);
    
    // Hide them all to start
    _direcIndicatorsNode->setVisible(false);
    _thiefIndicatorNode->setVisible(false);
    _joystickNode->setVisible(false);
    _accelVisNode->setVisible(false);
//    _victoryNode->setVisible(false);

    // Call helpers to populate sub-level nodes
    initJoystick();
    initAccelVis();
    initDirecIndicators();
//    initThiefIndicator();
//    initMessage();
//    initSettings();
    _settings.init(_uinode, _screenSize, _offset, _assets, _actions, _audio);

    initSettingsButton();
    
    return true;
}

//  MARK: - Methods

/**
 * Updates the UI Controller
 */
void UIController::update(float timestep, bool isThief, Vec2 movement,
                          bool didPress, Vec2 origin, Vec2 position, int copID,
                          float gameTime, bool isThiefWin) {
    
    // Show these nodes regardless
    _direcIndicatorsNode->setVisible(true);

    // Show these nodes if the player is a thief, hide them if they're a cop
    _joystickNode->setVisible(isThief);
    
    // Show these nodes if the player is a cop, hide them if they're a thief
    _accelVisNode->setVisible(!isThief);
    _thiefIndicatorNode->setVisible(!isThief);
    
    // If the player is the thief, update directional indicators and joystick
    if (isThief) {
        updateJoystick(didPress, origin, position);
    }
    
    // If the player is a cop, update accelerometer visualization and distance
    else {
        updateAccelVis(movement);
        updateThiefIndicator(copID, isThief);
    }
   
    updateDirecIndicators(isThief, copID);

    // Update setttings
    updateSettingsButton(timestep);

//    updateMessage(isThief, isThiefWin);
    updateTimer(gameTime);
}

//  MARK: - Helpers

/**
 * Initializes the node from JSON that is the parent of various UI elements. 
 */
void UIController::initElementsNode() {

    //     Initialize settings button from assets manager
        _elementsNode = _assets->get<scene2::SceneNode>("game");
        _elementsNode->setContentSize(_screenSize);
        Vec2 elementsPos = _elementsNode->getContentSize();
        elementsPos *= SCENE_HEIGHT / _screenSize.height;
        _elementsNode->setContentSize(elementsPos);
        _elementsNode->doLayout(); // Repositions the HUD

        _uinode->addChild(_elementsNode);
}

/**
 * Initializes the settings button
 */
void UIController::initSettingsButton() {
    _settingsButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("game_gameUIsettings"));

//     Initialize settings button from assets manager

    // Make settings button transparent
    _settingsButton->setColor(_transparent);

    _settingsButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            _settings.setDidPause(true);
        }
        });

    _settingsButton->activate();
}


/**
 * Creates the necessary nodes for showing the joystick and adds them to the joystick node.
 */
void UIController::initJoystick() {
    // Create outer part of joystick
    _outerJoystick = scene2::PolygonNode::allocWithPoly(_pf.makeCircle(Vec2(0,0), JOYSTICK_RADIUS));
    _outerJoystick->setAnchor(cugl::Vec2::ANCHOR_CENTER);
    _outerJoystick->setScale(1.0f);
    _outerJoystick->setColor(Color4(Vec4(0, 0, 0, 0.25)));
    _joystickNode->addChild(_outerJoystick);
    
    // Create inner part of joystick view
    _innerJoystick = scene2::PolygonNode::allocWithPoly(_pf.makeCircle(Vec2(0,0), JOYSTICK_RADIUS / 2));
    _innerJoystick->setAnchor(cugl::Vec2::ANCHOR_CENTER);
    _innerJoystick->setScale(1.0f);
    _innerJoystick->setColor(Color4(Vec4(0, 0, 0, 0.25)));
    _joystickNode->addChild(_innerJoystick);
    
    // Reposition the joystick components
    _outerJoystick->setPosition(JOYSTICK_HOME);
    _innerJoystick->setPosition(JOYSTICK_HOME);
}

/**
 * Creates the necessary nodes for the accelerometer visualization and adds them to the accelerometer
 * visualization node.
 */
void UIController::initAccelVis() {
    // Create outer part of accelerometer visualization
    Poly2 outerCircle = _pf.makeCircle(Vec2(OUTER_ACCEL_VIS_POS) * Vec2(SCENE_WIDTH, SCENE_HEIGHT), OUTER_ACCEL_VIS_RADIUS);
    _outerAccelVis = scene2::PolygonNode::allocWithPoly(outerCircle);
    _outerAccelVis->setAnchor(cugl::Vec2::ANCHOR_CENTER);
    _outerAccelVis->setScale(1.0f);
    _outerAccelVis->setColor(Color4(cugl::Vec4(0, 0, 0, 0.25)));
    _accelVisNode->addChild(_outerAccelVis);

    // Create inner part of accelerometer visualization
    Poly2 innerCircle = _pf.makeCircle(Vec2(OUTER_ACCEL_VIS_POS), INNER_ACCEL_VIS_RADIUS);
    _innerAccelVis = scene2::PolygonNode::allocWithPoly(innerCircle);
    _innerAccelVis->setAnchor(cugl::Vec2::ANCHOR_CENTER);
    _innerAccelVis->setScale(1.0f);
    _innerAccelVis->setColor(Color4::RED);
    _accelVisNode->addChild(_innerAccelVis);
}

/**
 * Creates directional indicators for the thief that point towards the cops and adds them to the directional
 * indicators node.
 */
void UIController::initDirecIndicators() {
    // Set texture for directional indicators
    _direcIndTexture = _assets->get<Texture>(DIR_IND_ARROW_TEXTURE);

    for (int i = 0; i < _game->numberOfCops(); i++) {
        // Create and display directional indicators
        _direcIndicators[i] = scene2::PolygonNode::allocWithTexture(_direcIndTexture);

        _direcIndicators[i]->setAnchor(cugl::Vec2::ANCHOR_CENTER);
        float scale = DIREC_INDICATOR_SIZE / _direcIndicators[i]->getTexture()->getWidth();
        _direcIndicators[i]->setScale(scale);
        _direcIndicators[i]->setColor(Color4::RED);
        _direcIndicatorsNode->addChild(_direcIndicators[i]);
    }
}

/**
 * Sets the references for the thief indicator from the JSON.
 */
void UIController::initThiefIndicator() {
    
    _thiefIndicatorNode = std::dynamic_pointer_cast<scene2::SceneNode>(_assets->get<scene2::SceneNode>("game_thiefIndicator"));
    _thiefIndicator = std::dynamic_pointer_cast<scene2::Label>(_assets->get<scene2::SceneNode>("game_thiefIndicator_text"));
    _thiefIndicatorNode->setColor(_transparent);

}

/**
 * Creates the message label and adds it the UI node
 */
//void UIController::initMessage() {
//    _victoryText = scene2::Label::allocWithText("should be replaced", _font);
//    _victoryText->setAnchor(Vec2::ANCHOR_CENTER);
//    _victoryText->setPosition(Vec2(SCENE_WIDTH/2,SCENE_HEIGHT/2) + _offset);
//    _victoryNode->addChild(_victoryText);
//}

/**
 * Creates the timer texture and adds it the UI node
 */
void UIController::initTimer() {
    // Create clock
    _timer = scene2::PolygonNode::allocWithTexture(_assets->get<Texture>("clock"));
    _timer->setAnchor(Vec2::ANCHOR_CENTER);
    _timer->setPosition(Vec2(SCENE_WIDTH_ADJUST,SCENE_HEIGHT-SCENE_HEIGHT_ADJUST));
    _timer->setScale(0.3f);
    
    // Create hour and minute hands
    _hourHand = scene2::PolygonNode::allocWithTexture(_assets->get<Texture>("hour_hand"));
    _minuteHand = scene2::PolygonNode::allocWithTexture(_assets->get<Texture>("minute_hand"));
    _hourHand->setScale(_timer->getScale());
    _minuteHand->setScale(_timer->getScale());
    _hourHand->setAnchor(Vec2(0.5,0));
    _minuteHand->setAnchor(Vec2(0.5,0));
    _hourHand->setPosition(_timer->getPosition() - Vec2(0,10));
    _minuteHand->setPosition(_timer->getPosition() - Vec2(0,10));
    
    //Make timer transparent
    _timer->setColor(_transparent);
    _hourHand->setColor(_transparent);
    _minuteHand->setColor(_transparent);

    _uinode->addChild(_timer);
    _uinode->addChild(_hourHand);
    _uinode->addChild(_minuteHand);
}

/**
 * Updates the minute and hour hand nodes
 */
void UIController::updateTimer(float gameTime) {
    float ang = 0;
    float deg = (360 * gameTime / GAME_LENGTH);
    ang = (- deg) * 3.14 / 180;
    
    _minuteHand->setAngle(ang);
    _hourHand->setAngle(ang/60);
}

/**
 * Updates the joystick
 */
void UIController::updateJoystick(bool didPress, Vec2 origin, Vec2 position) {
    Vec2 difference = position - origin;
    if (didPress) {
        if (difference.length() > JOYSTICK_RADIUS)
            position = origin + difference.getNormalization() * JOYSTICK_RADIUS;
        _innerJoystick->setPosition(didPress ? position : JOYSTICK_HOME);
        _outerJoystick->setPosition(didPress ? origin : JOYSTICK_HOME);
        _outerJoystick->setVisible(true);
        _innerJoystick->setVisible(true);
    }
    else {
        _outerJoystick->setVisible(false);
        _innerJoystick->setVisible(false);
    }
}

/**
 * Updates the accelerometer visualization
 */
void UIController::updateAccelVis(Vec2 movement) {
    _innerAccelVis->setPosition(_outerAccelVis->getPosition() + (movement * OUTER_ACCEL_VIS_RADIUS));
}

/**
 * Updates directional indicators
 */
void UIController::updateDirecIndicators(bool isThief, int copID) {
    // Get the position of the thief in node coordinates w/ respect to worldnode
    Vec2 thiefScreenPos = _worldnode->nodeToScreenCoords(_game->getThief()->getNode()->getPosition());
    // Get the position of cop with respect to Box2D world
    Vec2 thiefPos = _game->getThief()->getPosition();
    if (isThief) {
        // Run calculations for each directonal indicator
        for (int i = 0; i < _game->numberOfCops(); i++) {
            // Get the position of cop in node coordinates w/ respect to worldnode
            Vec2 copScreenPos = _worldnode->nodeToScreenCoords(_game->getCop(i)->getNode()->getPosition());
            // Get the position of cop with respect to Box2D world
            Vec2 copPos = _game->getCop(i)->getPosition();

            // Call helper
            updateDirecIndicatorHelper(thiefPos, copPos, thiefScreenPos, copScreenPos, isThief, i);
        }
    }
    else {
        // Get the position of cop in node coordinates w/ respect to worldnode
        Vec2 copScreenPos = _worldnode->nodeToScreenCoords(_game->getCop(copID)->getNode()->getPosition());
        // Get the position of cop with respect to Box2D world
        Vec2 copPos = _game->getCop(copID)->getPosition();
        for (int i = 0; i < _game->numberOfCops(); i++) {
            _direcIndicators[i]->setVisible(false);
        }
        // Call helper
        updateDirecIndicatorHelper(copPos, thiefPos, copScreenPos, thiefScreenPos, isThief, copID);
    }
}

/**
 * Updates a single directional indicator.
 * Takes in the player's position, the position in the direction we want to move to, the screen coordinates
 * of these positions, and the index of the directional indicator within the map.
*/
void UIController::updateDirecIndicatorHelper(cugl::Vec2 pos1, cugl::Vec2 pos2, 
    cugl::Vec2 screenPos1, cugl::Vec2 screenPos2, bool isThief, int index) {
    // Constants and variables used throughout the code
    const float size_scalar_min = 0.3;
    const float size_scalar_max_dist = 100;
    const int color_opacity = 220;
    const float cop_min_thief_visible_distance = 25.0;
    const float distance_from_edge = 20;
    const float defaultScale = DIREC_INDICATOR_SIZE / _direcIndicators[index]->getTexture()->getWidth();;
    
    // By default, show this indicator
    _direcIndicators[index]->setVisible(true);

    // Hide the directional indicator if the cop is on screen
    if (screenPos2.over(Vec2::ZERO) && screenPos2.under((Vec2)_screenSize)) {
        _direcIndicators[index]->setVisible(false);
    }

    // Calculate distance and angle from thief to cop
    Vec2 distance = pos2 - pos1;
    float angle = distance.getAngle() + M_PI_2 + M_PI;
    float displayDistance = distance.length();

    // Set scale directional indicators based on distance

    float scale = (size_scalar_max_dist - displayDistance) / size_scalar_max_dist;
    scale = max(scale, size_scalar_min);

    // We use the direct value of scale for color changing, but we also have to reduce
    // the size to accomodate the texture. This is why we have both scale and outputScale.
    // outputScale takes the texture size into account.
    float outputScale = scale * defaultScale;

    // Set the color based on the distance
    Color4 closeColor = Color4(255, 0, 0, color_opacity);
    Color4 farColor = Color4(182, 227, 255, color_opacity);
    float normalScale = (scale - size_scalar_min) / (1 - size_scalar_min);

    Color4 color((int)(((closeColor.r - farColor.r) * normalScale) + farColor.r), 
        (int)(((closeColor.g - farColor.g) * normalScale) + farColor.g),
        (int)(((closeColor.b - farColor.b) * normalScale) + farColor.b),
        color_opacity);
   
    // Make the vector's origin at the thief
    distance = distance.getNormalization() * (screenPos2 - screenPos1).length();
    distance.x += screenPos1.x;
    distance.y += _screenSize.height - screenPos1.y;

    // Clamp the position of the indicator within the screen
    float minDim = DIREC_INDICATOR_SIZE * scale;
    Vec2 minVec(minDim + distance_from_edge, minDim + distance_from_edge);
    Vec2 maxVec(_screenSize.width - minDim - distance_from_edge, _screenSize.height - minDim - distance_from_edge);
    distance.clamp(minVec, maxVec);

    // Scale the distance to match the fixed height
    distance *= SCENE_HEIGHT / _screenSize.height;

    // Set up directional indicators
    _direcIndicators[index]->setPosition(distance);
    _direcIndicators[index]->setAngle(angle);
    _direcIndicators[index]->setScale(outputScale);
    _direcIndicators[index]->setColor(color);

    if (!isThief && displayDistance > cop_min_thief_visible_distance) {
        _direcIndicators[index]->setVisible(false);
    } 

}

/**
 * Updates the thief indicator
 */
void UIController::updateThiefIndicator(int copID, bool isThief) {
    if (isThief) {
        _thiefIndicatorNode->setVisible(false);
        return;
    }
    float distance = _game->getThief()->getPosition().distance(_game->getCop(copID)->getPosition());
    //_thiefIndicatorBorder->setText("Thief Distance: " + to_string((int) distance), true);
    //_thiefIndicator->setText("Thief Distance: " + to_string((int)distance), true);
    _thiefIndicator->setText(to_string((int)distance) + "m", true);

}

/**
 * Updates the settings button
 */
void UIController::updateSettingsButton(float timestep) {
    _settings.update(timestep);
    _didPause = _settings.didPause();
    _isPaused = _settings.isPaused();
    _didQuit = _settings.didQuit();
    if (_isPaused || _actions->isActive(SETTINGS_ACT_KEY)) {
        _settingsButton->deactivate();
        _settingsButton->setDown(false);
    }
    else {
        _settingsButton->activate();
    }
}

/**
 * Updates the message label
 */
//void UIController::updateMessage(bool isThief, bool isThiefWin) {
//    if (! _game->isGameOver()) return;
//
//    if (isThief) {
//        if (isThiefWin) {
//            _victoryText->setText("Thief Wins!", true);
//        }
//        else {
//            _victoryText->setText("Thief Loses!", true);
//        }
//    }
//    else {
//        if (isThiefWin) {
//            _victoryText->setText("Cops Lose!", true);
//        }
//        else {
//            _victoryText->setText("Cops Win!", true);
//        }
//    }
//    _victoryNode->setVisible(_game->isGameOver());
//}
