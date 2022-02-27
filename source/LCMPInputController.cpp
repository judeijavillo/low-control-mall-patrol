//
//  LCMPInputController.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#include "LCMPInputController.h"

using namespace cugl;

//  MARK: - Input Factors

/** Whether to active the accelerometer (this is TRICKY!) */
#define USE_ACCELEROMETER           true

/** How much the keyboard adds to the input. */
#define KEYBOARD_FORCE_INCREMENT  1.0f

/** This defines the joystick "deadzone" (how far we must move) */
#define JSTICK_DEADZONE  15
/** This defines the joystick radial size (for reseting the anchor) */
#define JSTICK_RADIUS    50
/** How far to display the virtual joystick above the finger */
#define JSTICK_OFFSET    80

/** How far we must swipe left or right for a gesture */
#define EVENT_SWIPE_LENGTH  100
/** How fast we must swipe left or right for a gesture */
#define EVENT_SWIPE_TIME   1000
/** How far we must turn the tablet for the accelerometer to register */
#define EVENT_ACCEL_THRESH  M_PI/10.0f
/** The key for the event handlers */
#define LISTENER_KEY        1

// MARK: -Input Controller

/**
 * Creates a new input controller.
 *
 * This constructor does NOT do any initialzation.  It simply allocates the
 * object. This makes it safe to use this class without a pointer.
 */
InputController::InputController() :
	_active(false),
	_forceLeft(0.0f),
	_forceRight(0.0f),
	_forceUp(0.0f),
	_forceDown(0.0f),
	_joystick(false),
    _horizontal(0.0f),
    _vertical(0.0f)
{
}

/**
 * Deactivates this input controller, releasing all listeners.
 *
 * This method will not dispose of the input controller. It can be reused
 * once it is reinitialized.
 */
void InputController::dispose() {
    if (_active) {
#ifndef CU_TOUCH_SCREEN
        Input::deactivate<Keyboard>();
#else
        if (USE_ACCELEROMETER) {
            Input::deactivate<Accelerometer>();
        }
        Touchscreen* touch = Input::get<Touchscreen>();
        touch->removeBeginListener(LISTENER_KEY);
        touch->removeEndListener(LISTENER_KEY);
        _active = false;
#endif
    }
}

/**
 * Initializes the input control for the given drawing scale.
 *
 * This method works like a proper constructor, initializing the input
 * controller and allocating memory.  However, it still does not activate
 * the listeners.  You must call start() do that.
 *
 * @return true if the controller was initialized successfully
 */
bool InputController::init(const Rect bounds) {
    bool success = true;
    _sbounds = bounds;
    _tbounds = Application::get()->getDisplayBounds();
    clearTouchInstance(_mtouch);
    
    // Only process keyboard on desktop
#ifndef CU_TOUCH_SCREEN
    success = Input::activate<Keyboard>();
#else
    if (USE_ACCELEROMETER) {
        success = Input::activate<Accelerometer>();
    }
    Touchscreen* touch = Input::get<Touchscreen>();

    touch->addBeginListener(LISTENER_KEY, [=](const cugl::TouchEvent& event, bool focus) {
        this->touchBeganCB(event, focus);
        });
    touch->addEndListener(LISTENER_KEY, [=](const cugl::TouchEvent& event, bool focus) {
        this->touchEndedCB(event, focus);
        });
    touch->addMotionListener(LISTENER_KEY, [=](const TouchEvent& event, const Vec2& previous, bool focus) {
        this->touchesMovedCB(event, previous, focus);
        });
#endif
    _active = success;
    return success;
}

/**
 * Populates the initial values of the input TouchInstance
 */
void InputController::clearTouchInstance(TouchInstance& touchInstance) {
    touchInstance.touchids.clear();
    touchInstance.position = Vec2::ZERO;
}

/**
 * Processes the currently cached inputs.
 *
 * This method is used to to poll the current input state.  This will poll the
 * keyboad and accelerometer.
 *
 * This method also gathers the delta difference in the touches. Depending on
 * the OS, we may see multiple updates of the same touch in a single animation
 * frame, so we need to accumulate all of the data together.
 */
void InputController::update(float dt) {
    int left = false;
    int rght = false;
    int up   = false;
    int down = false;

    // Only process keyboard on desktop
#ifndef CU_TOUCH_SCREEN
    Keyboard* keys = Input::get<Keyboard>();

    // Forces increase the longer you hold a key.
    if (keys->keyDown(KeyCode::ARROW_LEFT)) {
        _forceLeft = KEYBOARD_FORCE_INCREMENT;
    }
    else {
        _forceLeft = 0.0f;
    }
    if (keys->keyDown(KeyCode::ARROW_RIGHT)) {
        _forceRight = KEYBOARD_FORCE_INCREMENT;
    }
    else {
        _forceRight = 0.0f;
    }
    if (keys->keyDown(KeyCode::ARROW_DOWN)) {
        _forceDown = KEYBOARD_FORCE_INCREMENT;
    }
    else {
        _forceDown = 0.0f;
    }
    if (keys->keyDown(KeyCode::ARROW_UP)) {
        _forceUp = KEYBOARD_FORCE_INCREMENT;
    }
    else {
        _forceUp = 0.0f;
    }

    _keybdThrust = Vec2::ZERO;

    // Update the keyboard thrust.  Result is cumulative.
    _keybdThrust.x += _forceRight;
    _keybdThrust.x -= _forceLeft;
    _keybdThrust.y += _forceUp;
    _keybdThrust.y -= _forceDown;

    // Transfer to main thrust. This keeps us from "adding" to accelerometer or touch.
    _inputMovement.x = _keybdThrust.x;
    _inputMovement.y = _keybdThrust.y;
    _inputMovement.normalize();

#else
    // MOBILE CONTROLS
    if (USE_ACCELEROMETER) {
        Vec3 acc = Input::get<Accelerometer>()->getAcceleration();
        
        // Measure the "steering wheel" tilt of the device
        float pitch = atan2(-acc.x, sqrt(acc.y*acc.y + acc.z*acc.z));
        float vert = atan2(-acc.y, sqrt(acc.x*acc.x + acc.z*acc.z));
                
        // Check if we turned left or right
        left |= (pitch > EVENT_ACCEL_THRESH);
        rght |= (pitch < -EVENT_ACCEL_THRESH);
        up   |= (vert > EVENT_ACCEL_THRESH/2);
        down |= (vert < -EVENT_ACCEL_THRESH/2);

    // Directional controls
    _horizontal = 0.0f;
    if (rght) {
        _horizontal += 1.0f;
    }
    if (left) {
        _horizontal -= 1.0f;
    }

    _vertical = 0.0f;
    if (up) {
        _vertical += 1.0f;
    }
    if (down) {
        _vertical -= 1.0f;
    }
            
    }
#endif

// If it does not support keyboard, we must reset "virtual" keyboard
//#ifdef CU_TOUCH_SCREEN
//    _keyExit = false;
//#endif
}

/**
 * Clears any buffered inputs so that we may start fresh.
 */
void InputController::clear() {
    _inputMovement = Vec2::ZERO;
    _keybdThrust = Vec2::ZERO;

    _forceLeft = 0.0f;
    _forceRight = 0.0f;
    _forceUp = 0.0f;
    _forceDown = 0.0f;

    _horizontal = 0.0f;
    _vertical   = 0.0f;
}

//  MARK: - Touch Callbacks

/**
 * Callback for the beginning of a touch event
 *
 * @param t     The touch information
 * @param event The associated event
 */
void InputController::touchBeganCB(const cugl::TouchEvent& event, bool focus) {
    if (_mtouch.touchids.empty()) {
        // Left is the floating joystick
        _mtouch.position = event.position;
        _mtouch.timestamp.mark();
        _mtouch.touchids.insert(event.touch);

        _joystick = true;
        _joycenter = touch2Screen(event.position);
        _joycenter.y += JSTICK_OFFSET;
    }
}

void InputController::touchesMovedCB(const cugl::TouchEvent& event, const cugl::Vec2& previous, bool focus) {
    Vec2 pos = event.position;
    // Only check for swipes in the main zone if there is more than one finger.
    if (!_mtouch.touchids.empty()) {
        processJoystick(pos);
    }
};

Vec2 InputController::touch2Screen(const Vec2 pos) const {
    float px = pos.x / _tbounds.size.width - _tbounds.origin.x;
    float py = pos.y / _tbounds.size.height - _tbounds.origin.y;
    Vec2 result;
    result.x = px * _sbounds.size.width + _sbounds.origin.x;
    result.y = (1 - py) * _sbounds.size.height + _sbounds.origin.y;
    return result;
}

void InputController::processJoystick(const cugl::Vec2 pos) {
    Vec2 diff = _mtouch.position - pos;
    Vec2 finishTouch = Vec2::ZERO;
    // Reset the anchor if we drifted too far
    if (diff.lengthSquared() > JSTICK_RADIUS * JSTICK_RADIUS) {
        diff.normalize();
        diff *= (JSTICK_RADIUS + JSTICK_DEADZONE) / 2;
        //    _mtouch.position = pos + diff;
    }
    //_mtouch.position.y = pos.y;
    //_joycenter = touch2Screen(_mtouch.position);
    //_joycenter.y += JSTICK_OFFSET;
    //CULog("process joystick");
    if (std::fabsf(diff.x) > JSTICK_DEADZONE || std::fabsf(diff.y) > JSTICK_DEADZONE) {
        _joystick = true;

        finishTouch.x = -diff.x / JSTICK_RADIUS;
        finishTouch.y = diff.y / JSTICK_RADIUS;

        _inputMovement.x = finishTouch.x;
        _inputMovement.y = finishTouch.y;  // Touch coords
        //CULog("thrust: (%f, %f)", _inputMovement.x, _inputMovement.y);
    }
    else {
        _joystick = false;
    }

}

/**
 * Callback for the end of a touch event
 *
 * @param t     The touch information
 * @param event The associated event
 */
void InputController::touchEndedCB(const cugl::TouchEvent& event, bool focus) {
    // Remove joystick touch
    if (!_mtouch.touchids.empty()) {
        _mtouch.touchids.clear();
        _joystick = false;
        _inputMovement = Vec2::ZERO;
    }

    // Zero the movement.
    _inputMovement = Vec2::ZERO;
}
