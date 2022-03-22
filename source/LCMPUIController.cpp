//
//  LCMPUIController.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 3/20/22
//

#include "LCMPUIController.h"
#include "LCMPConstants.h"

using namespace std;
using namespace cugl;

//  MARK: - Constants

/** How far from the thief the directional indicators appear. */
#define DIREC_INDICATOR_DIST_SCALAR     250.0f
/** How long one side of the directional indicator is. */
#define DIREC_INDICATOR_SIZE            60.0f

/** The radius of the outer accelerometer visualization. */
#define OUTER_ACCEL_VIS_RADIUS    75.0f
/** The radius of the inner accelerometer visualization. */
#define INNER_ACCEL_VIS_RADIUS    7.0f

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
    
}

/**
 * Initializes a UI Controller
 */
bool UIController::init(const shared_ptr<scene2::SceneNode> worldnode,
                        const shared_ptr<scene2::SceneNode> uinode,
                        const shared_ptr<GameModel> game,
                        const shared_ptr<Font> font,
                        Size screenSize,
                        Vec2 offset) {
    
    // Save properties
    _worldnode = worldnode;
    _uinode = uinode;
    _game = game;
    _screenSize = screenSize;
    _offset = offset;
    _font = font;
    
    // Create subnodes
    _direcIndicatorsNode = scene2::SceneNode::alloc();
    _thiefIndicatorNode = scene2::SceneNode::alloc();
    _joystickNode = scene2::SceneNode::alloc();
    _accelVisNode = scene2::SceneNode::alloc();
    
    // Add nodes to the top-level nodes
    _uinode->addChild(_direcIndicatorsNode);
    _uinode->addChild(_thiefIndicatorNode);
    _uinode->addChild(_joystickNode);
    _uinode->addChild(_accelVisNode);
    
    // Hide them all to start
    _direcIndicatorsNode->setVisible(false);
    _thiefIndicatorNode->setVisible(false);
    _joystickNode->setVisible(false);
    _accelVisNode->setVisible(false);
    
    // Call helpers to populate sub-level nodes
    initJoystick();
    initAccelVis();
    initDirecIndicators();
    initThiefIndicator();
    initMessage();
    
    return true;
}

//  MARK: - Methods

/**
 * Updates the UI Controller
 */
void UIController::update(float timestep, bool isThief, Vec2 movement,
                          bool didPress, Vec2 origin, Vec2 position, int copID) {
    // Show these nodes if the player is a thief, hide them if they're a cop
    _joystickNode->setVisible(isThief);
    _direcIndicatorsNode->setVisible(isThief);
    
    // Show these nodes if the player is a cop, hide them if they're a thief
    _accelVisNode->setVisible(!isThief);
    _thiefIndicatorNode->setVisible(!isThief);
    
    // If the player is the thief, update directional indicators and joystick
    if (isThief) {
        updateJoystick(didPress, origin, position);
        updateDirecIndicators();
    }
    
    // If the player is a cop, update accelerometer visualization and distance
    else {
        updateAccelVis(movement);
        updateThiefIndicator(copID);
    }
    
    // Show the appropriate message
    updateMessage();
}

//  MARK: - Helpers

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
    // Calculate distances of cops from thief
    for (int i = 0; i < _game->numberOfCops(); i++) {
        // Create and display directional indicators
        Poly2 triangle = _pf.makeTriangle(Vec2::ZERO,
                                          Vec2(DIREC_INDICATOR_SIZE,
                                               0.0f),
                                          Vec2(DIREC_INDICATOR_SIZE /2,
                                               DIREC_INDICATOR_SIZE * 1.5));
        _direcIndicators[i] = scene2::PolygonNode::allocWithPoly(triangle);
        _direcIndicators[i]->setAnchor(cugl::Vec2::ANCHOR_CENTER);
        _direcIndicators[i]->setScale(1.0f);
        _direcIndicators[i]->setColor(Color4::RED);
        _direcIndicatorsNode->addChild(_direcIndicators[i]);
    }
}

/**
 * Creates a thief indicator for the cops that shows how far they are from the thief and adds it to the
 * thief indicator node.
 */
void UIController::initThiefIndicator() {
    // Create and show distance on screen
    _thiefIndicator = scene2::Label::allocWithText("Thief Distance: 0", _font);
    _thiefIndicator->setAnchor(Vec2::ANCHOR_CENTER);
    _thiefIndicator->setPosition(Vec2(SCENE_WIDTH/2,SCENE_HEIGHT-SCENE_HEIGHT_ADJUST) + _offset);
    _thiefIndicatorNode->addChild(_thiefIndicator);
}

/**
 * Creates the message label and adds it the UI node
 */
void UIController::initMessage() {
    _message = scene2::Label::allocWithText("Cops Win!", _font);
    _message->setAnchor(Vec2::ANCHOR_CENTER);
    _message->setPosition(Vec2(SCENE_WIDTH/2,SCENE_HEIGHT/2) + _offset);
    _message->setVisible(false);
    _uinode->addChild(_message);
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
void UIController::updateDirecIndicators() {
    // Constants and variables used throughout the code
    const float size_scalar_min = 0.25;
    const float size_scalar_max_dist = 90;
    const int color_opacity = 200;
    
    // Get the position of the thief in node coordinates w/ respect to worldnode
    Vec2 thiefPos = _worldnode->nodeToScreenCoords(_game->getThief()->getNode()->getPosition());

    // Run calculations for each directonal indicator
    for (int i = 0; i < _game->numberOfCops(); i++) {
        // By default, show this indicator
        _direcIndicators[i]->setVisible(true);
        
        // Get the position of cop in node coordinates w/ respect to worldnode
        Vec2 copPos = _worldnode->nodeToScreenCoords(_game->getCop(i)->getNode()->getPosition());
        
        // Hide the directional indicator if the cop is on screen
        if (copPos.over(Vec2::ZERO) && copPos.under((Vec2) _screenSize)) {
            _direcIndicators[i]->setVisible(false);
        }
        
        // Calculate distance and angle from thief to cop
        Vec2 distance = _game->getCop(i)->getPosition() - _game->getThief()->getPosition();
        float angle = distance.getAngle() + M_PI_2 + M_PI;
        float displayDistance = distance.length();

        // Set scale directional indicators based on distance
        float scale = (size_scalar_max_dist - displayDistance) / size_scalar_max_dist;
        scale = max(scale, size_scalar_min);
        
        // Set the color based on the distance
        Color4 color((int)(255 * scale), (int)(255 * (1 - scale)), 0, color_opacity);

        // Make the vector's origin at the thief
        distance = distance.getNormalization() * (copPos - thiefPos).length();
        distance.x += thiefPos.x;
        distance.y += _screenSize.height - thiefPos.y;

        // Clamp the position of the indicator within the screen
        float minDim = DIREC_INDICATOR_SIZE * scale;
        Vec2 minVec(minDim, minDim);
        Vec2 maxVec(_screenSize.width - minDim, _screenSize.height - minDim);
        distance.clamp(minVec, maxVec);
        
        // Scale the distance to match the fixed height
        distance *= SCENE_HEIGHT / _screenSize.height;

        // Set up directional indicators
        _direcIndicators[i]->setPosition(distance);
        _direcIndicators[i]->setAngle(angle);
        _direcIndicators[i]->setScale(scale);
        _direcIndicators[i]->setColor(color);
    }
}

/**
 * Updates the thief indicator
 */
void UIController::updateThiefIndicator(int copID) {
    float distance = _game->getThief()->getPosition().distance(_game->getCop(copID)->getPosition());
    _thiefIndicator->setText("Thief Distance: " + to_string((int) distance), true);
}

/**
 * Updates the message label
 */
void UIController::updateMessage() {
    _message->setVisible(_game->isGameOver());
}

