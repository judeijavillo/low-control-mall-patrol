//
//  LCMPInputController.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#include "LCMPInputController.h"
#include "LCMPConstants.h"

using namespace cugl;

// MARK: - Constants

/** The key for the event handlers */
#define LISTENER_KEY      1

/** The max range of the accelerometer input. */
#define ACCEL_MAX 0.35f
/** The deadzone of the accelerometer. */
#define ACCEL_DEADZONE 0.05f

/** The portion of the screen used for the left zone */
#define LEFT_ZONE       0.5f
/** The portion of the screen used for the right zone */
#define RIGHT_ZONE      0.5f

//  MARK: - Constructors

/**
 * Constructs an Input Controller
 */
InputController::InputController() :
_isActive(false),
_didSwitch(false),
_didSwipe(false),
_joystickPressed(false),
_joystickOrigin(0,0),
_joystickPosition(0,0),
_acceleration(0,0),
_swipe(0,0) {}

/**
 * Disposes of all resources in this instance of Input Controller
 */
void InputController::dispose() {
#ifdef CU_TOUCH_SCREEN
    if (_isActive) {
        Touchscreen* touch = Input::get<Touchscreen>();
        touch->removeBeginListener(LISTENER_KEY);
        touch->removeEndListener(LISTENER_KEY);
        
        Accelerometer* accelerometer = Input::get<Accelerometer>();
        accelerometer->removeListener(LISTENER_KEY);
        
        // Finished disposing
        _isActive = false;
    }
#endif
}

/**
 * Initializes an Input Controller
 */
bool InputController::init(const cugl::Rect bounds) {
    clearTouchInstance(_ltouch);
    clearTouchInstance(_rtouch);
    //default accelerometer offset
    _accelOffset = Vec2(0.0, -0.45);
#ifdef CU_TOUCH_SCREEN
    _sbounds = bounds;
    _tbounds = Application::get()->getDisplayBounds();
    initZones();
    
    Touchscreen* touch = Input::get<Touchscreen>();
    touch->addBeginListener(LISTENER_KEY,
        [this](const TouchEvent& event, bool focus) {
            this->touchBeganCB(event,focus);
        });
    touch->addEndListener(LISTENER_KEY,
        [this](const TouchEvent& event, bool focus) {
            this->touchEndedCB(event,focus);
        });
    touch->addMotionListener(LISTENER_KEY,
        [this](const TouchEvent& event, const Vec2& previous, bool focus) {
            this->touchMovedCB(event, previous, focus);
        });
    
#endif
    _isActive = true;
    return true;
}

/**
 * Populates the initial values of the input TouchInstance
 */
void InputController::clearTouchInstance(TouchInstance& touchInstance) {
    touchInstance.touchids.clear();
    touchInstance.position = Vec2::ZERO;
}
    
//  MARK: - Detection

/**
 * Updates the Input Controller
 */
void InputController::update(float timestep) {
#ifdef CU_TOUCH_SCREEN
    _acceleration = Input::get<Accelerometer>()->getAcceleration() + _accelOffset;
    if (_acceleration.length() < ACCEL_DEADZONE) {
        _acceleration = Vec2::ZERO;
    }
#else
    Keyboard* keys = Input::get<Keyboard>();
    _acceleration = Vec2::ZERO;
    if (keys->keyDown(KeyCode::ARROW_LEFT)) _acceleration.x -= 1.0f;
    if (keys->keyDown(KeyCode::ARROW_RIGHT)) _acceleration.x += 1.0f;
    if (keys->keyDown(KeyCode::ARROW_DOWN)) _acceleration.y += 1.0f;
    if (keys->keyDown(KeyCode::ARROW_UP)) _acceleration.y -= 1.0f;
    if (keys->keyDown(KeyCode::SPACE)) _didSpace = true;

#endif
}

/**
 * Clears any buffered inputs so that we may start fresh.
*/
void InputController::clear() {
    _didSwipe = false;
    _didSwitch = false;
    _didSpace = false;
    _acceleration = Vec2::ZERO;
}
    
//  MARK: - Results

/**
 * Converts from touch screen coordinates to screen coordinates
 */
cugl::Vec2 const InputController::touch2Screen(const cugl::Vec2 pos) const {
    float px = pos.x/_tbounds.size.width -_tbounds.origin.x;
    float py = pos.y/_tbounds.size.height-_tbounds.origin.y;
    Vec2 result;
    result.x = px*_sbounds.size.width +_sbounds.origin.x;
    result.y = (1-py)*_sbounds.size.height+_sbounds.origin.y;
    return result;
}

/**
 * Returns the appropriate vector that determines where
 */
cugl::Vec2 const InputController::getMovementVector(bool isThief) const {
#ifdef CU_TOUCH_SCREEN
    if (isThief) {
        Vec2 dpos = Vec2(_joystickPosition - _joystickOrigin);
        dpos.lengthSquared() >= (JOYSTICK_RADIUS * JOYSTICK_RADIUS)
            ? dpos.normalize()
            : dpos = dpos / JOYSTICK_RADIUS;
        return dpos;
    }
    else {
        Vec2 accel = Vec2(_acceleration);
        accel.lengthSquared() >= (ACCEL_MAX * ACCEL_MAX)
            ? accel.normalize()
            : accel = accel / ACCEL_MAX;
        return accel;
    }
#else
    return _acceleration;
#endif
}

    
//  MARK: - Callbacks

/**
 * Callback for detecting that the player has pressed the touchscreen
 */
void InputController::touchBeganCB(const cugl::TouchEvent& event, bool focus) {
    Vec2 pos = event.position;
    Zone zone = getZone(pos);
    
    if (_mtouch.touchids.empty()) {
        // Left is the floating joystick
        _mtouch.position = event.position;
        _mtouch.timestamp.mark();
        _mtouch.touchids.insert(event.touch);
    }
    _didSwipe = false;

    switch (zone) {
        case InputController::Zone::LEFT:
            if (_ltouch.touchids.empty()) {
                // Left is the floating joystick
                _ltouch.position = event.position;
                _ltouch.timestamp.mark();
                _ltouch.touchids.insert(event.touch);
            }
            _joystickPressed = true;
            _joystickID = event.touch;
            _joystickOrigin = event.position;
            _joystickPosition = event.position;
            break;
        case InputController::Zone::RIGHT:
            if ((int)event.timestamp.ellapsedMillis(_rtouch.timestamp) < TAP_THRESHOLD) {
                _didSwitch = !_didSwitch;
            }
            _rtouch.timestamp.mark();
            break;
        default:
            break;
    }
}

/**
 * Callback for detecting that the player has released the touchscreen
 */
void InputController::touchEndedCB(const cugl::TouchEvent& event, bool focus) {
   
    // End oldest touch
    if (_mtouch.touchids.find(event.touch) != _mtouch.touchids.end()) {
        _mtouch.touchids.clear();
        _didSwipe = false;
    }
    
    // End joystick maybe
    Vec2 pos = event.position;
    Zone zone = getZone(pos);
    if (_ltouch.touchids.find(event.touch) != _ltouch.touchids.end()) {
        _ltouch.touchids.clear();
        _joystickPressed = false;
        _joystickOrigin = Vec2::ZERO;
        _joystickPosition = Vec2::ZERO;
        _didSwipe = false;
    }
}

/**
 * Callback for detecting that the player has dragged accross the touchscreen
 */
void InputController::touchMovedCB(const cugl::TouchEvent& event, const cugl::Vec2& previous, bool focus) {
    // Only check for swipes in the main zone if there is more than one finger.
    if (_ltouch.touchids.find(event.touch) != _ltouch.touchids.end()) {
        _joystickPosition = event.position;
    }
    // Only check for swipes in the main zone if there is more than one finger.
    Vec2 pos = event.position;
    if (_mtouch.touchids.find(event.touch) != _mtouch.touchids.end()
        && (_mtouch.position - pos).lengthSquared() > EVENT_SWIPE_LENGTH * EVENT_SWIPE_LENGTH) {
        _swipe = (pos - _mtouch.position);
        _didSwipe = true;
    }
    else _didSwipe = false;
}

//  MARK: - Helpers

/**
 * Initializes zones for inputs
 */
void InputController::initZones() {
    _lzone = _tbounds;
    _lzone.size.width *= LEFT_ZONE;
    _rzone = _tbounds;
    _rzone.size.width *= RIGHT_ZONE;
    _rzone.origin.x = _tbounds.origin.x+_tbounds.size.width-_rzone.size.width;
}

/**
 * Returns the correct zone for the given position.
 */
InputController::Zone InputController::getZone(const cugl::Vec2 pos) const {
    if (_lzone.contains(pos)) return Zone::LEFT;
    else if (_rzone.contains(pos)) return Zone::RIGHT;
    return Zone::UNDEFINED;
}
