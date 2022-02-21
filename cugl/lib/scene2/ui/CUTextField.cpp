//
//  CUTextField.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides support for a single line text field. It is useful
//  for providing input forms for your application, such as saved games or
//  player settings.  Because it is only a single line, it is a subclass of
//  label.  A multiline text input would be a TextArea, and that is not
//  currently supported.
//
//  To make use of a TextField, BOTH Keyboard and TextInput input devices
//  must be activated.  In particular, TextInput allows the support of
//  virtual keyboards on mobile devices.
//
//  This class uses our standard shared-pointer architecture.
//
//  1. The constructor does not perform any initialization; it just sets all
//     attributes to their defaults.
//
//  2. All initialization takes place via init methods, which can fail if an
//     object is initialized more than once.
//
//  3. All allocation takes place via static constructors which return a shared
//     pointer.
//
//  CUGL MIT License:
//      This software is provided 'as-is', without any express or implied
//      warranty.  In no event will the authors be held liable for any damages
//      arising from the use of this software.
//
//      Permission is granted to anyone to use this software for any purpose,
//      including commercial applications, and to alter it and redistribute it
//      freely, subject to the following restrictions:
//
//      1. The origin of this software must not be misrepresented; you must not
//      claim that you wrote the original software. If you use this software
//      in a product, an acknowledgment in the product documentation would be
//      appreciated but is not required.
//
//      2. Altered source versions must be plainly marked as such, and must not
//      be misrepresented as being the original software.
//
//      3. This notice may not be removed or altered from any source distribution.
//
//  Author: Walker White and Enze Zhou
//  Version: 8/20/20
//
#include <cugl/input/cu_input.h>
#include <cugl/scene2/ui/CUTextField.h>
#include <cugl/base/CUApplication.h>
#include <cugl/util/CUStrings.h>
#include <cugl/render/CUTexture.h>
#include <cugl/render/CUFont.h>
#include <cugl/render/CUTextLayout.h>

using namespace cugl;
using namespace cugl::scene2;
using namespace cugl::strtool;

/** The pixel width of the cursor */
#define CURSOR_WIDTH  3
/** The number of frames to cycle before blinking the cursor */
#define CURSOR_PERIOD 25
/** The number of milliseconds to delay until continuous deletion */
#define DELETE_DELAY  500

#define REPEAT(obj,method,counter) [=](void){                           \
            auto field = obj.lock();                                    \
            if (field == nullptr || counter != field->_keyCount) {      \
                return false;                                           \
            }                                                           \
            field->method();                                            \
            return true;                                                \
        }


/**
 * Returns a classification of the given character code
 *
 * This method differs from {@link strtool#getUnicodeType} in that it
 * converts carriage-return/newline combinations into a single newline.
 * In addition, all control characters are converted to spaces.
 *
 * @param code  The character to classify
 * @param pcode The preceding character
 *
 * @return a classification of the given character code
 */
static UnicodeType classify(Uint32 code, Uint32 pcode) {
    // Quick checks
    switch (code) {
        case 10:        // \n
            return pcode == 13 ? UnicodeType::SPACE : UnicodeType::NEWLINE;
            break;
        case 13:        // \r
            return pcode == 10 ? UnicodeType::SPACE : UnicodeType::NEWLINE;
            break;
    }
    UnicodeType type = strtool::getUnicodeType(code);
    return type == UnicodeType::CONTROL ? UnicodeType::SPACE : type;
}

#pragma mark -
#pragma mark Constructors
/**
 * Creates an uninitialized text field with no size or font.
 *
 * You must initialize this field before use.
 *
 * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate a Node on the
 * heap, use one of the static constructors instead.
 */
TextField::TextField() :
_active(false),
_focused(false),
_mouse(true),
_altDown(false),
_metaDown(false),
_shiftDown(false),
_keyCount(0),
_showCursor(true),
_cursorBlink(0),
_cursorIndex(0),
_cursorWidth(CURSOR_WIDTH),
_cursorColor(Color4::BLACK),
_tkey(0),
_kkey(0),
_fkey(0),
_nextKey(1) {
    _classname = "TextField";
}

/**
 * Disposes all of the resources used.
 *
 * A disposed text field can be safely reinitialized. Any child will
 * be released.  They will be deleted if no other object owns them.
 *
 * It is unsafe to call this on a text field that is still currently
 * inside of a scene graph.
 */
void TextField::dispose() {
    if (_active) {
        deactivate(true);
    }
    
    _typeListeners.clear();
    _exitListeners.clear();
    _nextKey = 1;
    _tkey = 0;
    _kkey = 0;
    _fkey = 0;
	Label::dispose();
}

/**
 * Initializes a node with the given JSON specificaton.
 *
 * This initializer is designed to receive the "data" object from the
 * JSON passed to {@link Scene2Loader}.  This JSON format supports all
 * of the attribute values of its parent class. In addition, it supports
 * the following additional attributes:
 *
 *     "cursor":        A boolean indicating whether to show the cursor (when active)
 *     "cursorwidth":   A number indicating the width of the cursor rectangle
 *     "cursorcolor":   Either a four-element integer array (values 0..255) or a string
 *                      Any string should be a web color or a Tkinter color name.
 *
 * All attributes are optional. There are no required attributes.
 *
 * @param loader    The scene loader passing this JSON file
 * @param data      The JSON object specifying the node
 *
 * @return true if initialization was successful.
 */
bool TextField::initWithData(const Scene2Loader* loader, const std::shared_ptr<JsonValue>& data) {
    if (Label::initWithData(loader, data)) {
        _showCursor = data->getBool("cursor",false);
        _cursorWidth = data->getFloat("cursorwidth",CURSOR_WIDTH);
        if (data->has("cursorcolor")) {
            JsonValue* col = data->get("cursorcolor").get();
            if (col->isString()) {
                _cursorColor.set(col->asString("#ffffff"));
            } else {
                CUAssertLog(col->size() >= 4, "'color' must be a four element number array");
                _cursorColor.r = col->get(0)->asInt(0);
                _cursorColor.g = col->get(1)->asInt(0);
                _cursorColor.b = col->get(2)->asInt(0);
                _cursorColor.a = col->get(3)->asInt(0);
            }
        }
        return true;
    }
    return false;
}


#pragma mark -
#pragma mark Listeners
/**
 * Returns the type listener for the given key
 *
 * This listener is invoked when the text changes.
 *
 * If there is no listener for the given key, it returns nullptr.
 *
 * @param key   The identifier for the listener
 *
 * @return the type listener for the given key
 */
const TextField::Listener TextField::getTypeListener(Uint32 key) const {
    auto item = _typeListeners.find(key);
    if (item == _typeListeners.end()) {
        return nullptr;
    }
    return item->second;
}

/**
 * Returns all type listeners for this text field
 *
 * This listener is invoked when the text changes.
 *
 * @return the type listeners for this text field
 */
const std::vector<TextField::Listener> TextField::getTypeListeners() const {
    std::vector<Listener> result;
    result.reserve(_typeListeners.size());
    for(auto kv : _typeListeners) {
        result.push_back(kv.second);
    }
    return result;
}

/**
 * Returns a key for a type listener after adding it to this text field.
 *
 * This listener is invoked when the text changes.
 *
 * C++ cannot hash functions types.  Therefore, the listener will
 * be identified by a unique key, returned by this function.  You should
 * remember this key to remove the listener if necessary.
 *
 * @param listener  The listener to add
 *
 * @return the key for the listener
 */
Uint32 TextField::addTypeListener(Listener listener) {
    CUAssertLog(_nextKey < (Uint32)-1, "No more available listener slots");
    _typeListeners[_nextKey++] = listener;
    return _nextKey;
}

/**
 * Removes a type listener from this text field.
 *
 * This listener is invoked when the text changes.
 *
 * Listeners must be identified by the key returned by the {@link #addListener}
 * method. If this text field does not have a listener for the given key,
 * this method will fail.
 *
 * @param key  The key of the listener to remove
 *
 * @return true if the listener was succesfully removed
 */
bool TextField::removeTypeListener(Uint32 key) {
    if (_typeListeners.find(key) == _typeListeners.end()) {
        return false;
    }
    _typeListeners.erase(key);
    return true;
}

/**
 * Clears all type listeners for this text field.
 *
 * These listeners are invoked when the text changes. This method
 * does not require you to remember the keys assigned to the
 * individual listeners.
 *
 * @return true if the listener was succesfully removed
 */
void TextField::clearTypeListeners() {
    _typeListeners.clear();
}
  
/**
 * Returns the exit listener for the given key
 *
 * This listener is invoked when the field loses focus.
 *
 * If there is no listener for the given key, it returns nullptr.
 *
 * @param key   The identifier for the listener
 *
 * @return the listener for the given key
 */
const TextField::Listener TextField::getExitListener(Uint32 key) const {
    auto item = _exitListeners.find(key);
    if (item == _exitListeners.end()) {
        return nullptr;
    }
    return item->second;
}

/**
 * Returns all exit listeners for this text field
 *
 * These listeners are invoked when the field loses focus.
 *
 * @return the exits listeners for this text field
 */
const std::vector<TextField::Listener> TextField::getExitListeners() const {
    std::vector<Listener> result;
    result.reserve(_exitListeners.size());
    for(auto kv : _exitListeners) {
        result.push_back(kv.second);
    }
    return result;
}

/**
 * Returns a key for a exit listener after adding it to this field.
 *
 * This listener is invoked when the field loses focus.
 *
 * C++ cannot hash functions types.  Therefore, the listener will
 * be identified by a unique key, returned by this function.  You should
 * remember this key to remove the listener if necessary.
 *
 * @param listener  The listener to add
 *
 * @return the key for the listener
 */
Uint32 TextField::addExitListener(Listener listener) {
    CUAssertLog(_nextKey < (Uint32)-1, "No more available listener slots");
    _exitListeners[_nextKey++] = listener;
    return _nextKey;
}

/**
 * Removes a listener from this text fieldexi.
 *
 * This listener is invoked when the field loses focus.
 *
 * Listeners must be identified by the key returned by the {@link #addListener}
 * method. If this text field does not have a listener for the given key,
 * this method will fail.
 *
 * @param key  The key of the listener to remove
 *
 * @return true if the listener was succesfully removed
 */
bool TextField::removeExitListener(Uint32 key) {
    if (_exitListeners.find(key) == _exitListeners.end()) {
        return false;
    }
    _exitListeners.erase(key);
    return true;
}

/**
 * Clears all listeners for this text field.
 *
 * These listener are invoked when the field loses focus. This
 * method does not require you to remember the keys assigned to
 * the individual listeners.
 *
 * @return true if the listener was succesfully removed
 */
void TextField::clearExitListeners() {
    _exitListeners.clear();
}


#pragma mark -
#pragma mark Editing
/**
 * Sets the text for this label.
 *
 * All unprintable characters will be removed from the string.  This
 * includes tabs and newlines. They will be replaced by spaces.
 *
 * The string must be in either ASCII or UTF8 format.  No other string
 * encodings are supported.  As all ASCII strings are also UTF8, you can
 * this effectively means that the text must be UTF8.
 *
 * If the font is missing glyphs in this string, the characters in the text
 * may be different than those displayed.
 *
 * If this text field has a type listener, it will be invoked when this
 * method is called.
 *
 * @oaram text      The text for this label.
 * @oaram resize    Whether to resize the label to fit the new text.
 */
void TextField::setText(const std::string& text, bool resize) {
    Label::setText(text, resize);
    _cursorIndex = text.size();
    updateCursor();
}

/**
 * Activates this text field to enable editing.
 *
 * This method attaches a listener to either the {@link Mouse} or
 * {@link Touchscreen} inputs to monitor when the text field is pressed
 * and/or released. The text field will favor the mouse, but will use the
 * touch screen if no mouse input is active. If neither input is active,
 * this method will fail.
 *
 * It will also attach a listener to {@link TextInput} to provide access
 * to a (possibly virtual) keyboard and collect user typing.  Finally, it
 * attaches a liatener to {@link Keyboard} to monitor special keys such
 * as Alt, Meta, and the arrow keys.
 *
 * Notice that activating a text field and requesting focus is separate.
 * The field will request focus if selected by a touch or press, but it
 * cannot be editted until it has focus.
 *
 * @return true if the text field was successfully activated
 */
bool TextField::activate() {
    if (_active) {
        return false;
    }

    // Verify we have all the right handers
    TextInput* textInput = Input::get<TextInput>();
    CUAssertLog(textInput, "The TextInput device has not been enabled");
    Keyboard* keyBoard   = Input::get<Keyboard>();
    CUAssertLog(keyBoard, "The keyboard device has not been enabled");
    Mouse* mouse = Input::get<Mouse>();
    Touchscreen* touch = Input::get<Touchscreen>();
    CUAssertLog(mouse || touch,  "Neither mouse nor touch input is enabled");


    bool check;
    if (!_tkey) { _tkey = textInput->acquireKey(); }
    check = textInput->addInputListener(_tkey, [this](const TextInputEvent& event, bool focus) {
        updateInput(event,focus);
    });
    if (!check) {
        return false;
    }
    
    if (!_kkey) { _kkey = keyBoard->acquireKey(); }
    check = keyBoard->addKeyUpListener(_kkey, [this](const KeyEvent& event, bool focus) {
        updateKey(event,focus,false);
    });
    if (!check) {
        textInput->removeInputListener(_tkey);
        return false;
    }

    check = keyBoard->addKeyDownListener(_kkey, [this](const KeyEvent& event, bool focus) {
        updateKey(event,focus,true);
    });
    if (!check) {
        textInput->removeInputListener(_tkey);
        keyBoard->removeKeyUpListener(_kkey);
        return false;
    }

    if (mouse) {
    	_mouse = true;
        if (!_fkey) { _fkey = mouse->acquireKey(); }
    	check = mouse->addPressListener(_fkey, [=](const MouseEvent& event, Uint8 clicks, bool focus) {
    		updatePress(event.position, focus);
    	});
    } else {
    	_mouse = false;
        if (!_fkey) { _fkey = touch->acquireKey(); }
    	check = touch->addBeginListener(_fkey, [=](const TouchEvent& event, bool focus) {
    		updatePress(event.position, focus);
    	});
    }
    if (!check) {
    	textInput->removeInputListener(_tkey);
    	keyBoard->removeKeyUpListener(_kkey);
    	keyBoard->removeKeyDownListener(_kkey);
    	return false;
    }

    _active = true;
	return true;
}

/**
 * Deactivates this text field, ignoring any future input.
 *
 * This method removes its internal listener from either the {@link Mouse}
 * or {@link Touchscreen}, and from {@link Keyboard} and {@link TextInput}.
 *
 * When deactivated, the text field will no longer change its text on its own.
 * However, the user can still change manually with the
 * {@link setText(std::string&, bool)} method. Futhermore, the appropriate
 * type listener will be called when the text changes. However, any attempts
 * to manually acquire focus will fail.
 *
 * @return true if the text field was successfully deactivated
 */
bool TextField::deactivate(bool dispose) {
	if (!_active) {
		return false;
	}

    _keyCount++;
	bool success = true;
    if (_focused && !dispose) {
        success = releaseFocus();
    }
    TextInput* textInput = Input::get<TextInput>();
    Keyboard* keyBoard  = Input::get<Keyboard>();
    
    success = textInput->removeInputListener(_tkey) && success;
    success = keyBoard->removeKeyUpListener(_kkey) && success;
    success = keyBoard->removeKeyDownListener(_kkey) && success;
    if (_mouse) {
        Mouse* mouse = Input::get<Mouse>();
        CUAssertLog(mouse,  "Mouse input is no longer enabled");
        success = mouse->removePressListener(_fkey);
    } else {
        Touchscreen* touch = Input::get<Touchscreen>();
        CUAssertLog(touch,  "Touch input is no longer enabled");
        success = touch->removeBeginListener(_fkey);
    }
    
	_active = false;
	return success;
}

/**
 * Requests text input focus for this text field.
 *
 * When a text field is activated, it does not immediately have focus.
 * A text field without focus cannot be editted.  By either clicking
 * on the field or calling thus function, you can acquire focus and edit
 * the field.
 *
 * This method will fail if the text field is not active.
 *
 * @return true if successfully requested focus.
 */
bool TextField::requestFocus() {
	if (!_active || _focused) {
		return false;
	}

    TextInput* textInput = Input::get<TextInput>();
    CUAssertLog(textInput, "The TextInput device has not been enabled");
    Keyboard* keyBoard   = Input::get<Keyboard>();
    CUAssertLog(keyBoard, "The keyboard device has not been enabled");

    if (!textInput->requestFocus(_tkey)) {
    	return false;
    } else if (!keyBoard->requestFocus(_kkey)) {
    	textInput->releaseFocus();
    	return false;
    }
    
    textInput->begin();

    _altDown  = false;
    _metaDown = false;
    _shiftDown = false;
    _keyCount++;
    
    _focused = true;
    _cursorBlink = 0;
	_cursorIndex = _layout->_text.size();
	updateCursor();
    return true;
}

/**
 * Releases text input focus for this text field.
 *
 * When the focus is released, the label can no longer be editting.
 * Typically this means that the user has input the final value,
 * which is why the exit listener (if any exists) is called.
 *
 * In addition to calling this method manually, a user can release focus
 * either by pressing RETURN or clicking somewhere outside of the field.
 *
 * @return true if successfully released focus.
 */
bool TextField::releaseFocus() {
	if (!_focused) {
		return false;
	}

    TextInput* textInput = Input::get<TextInput>();
    CUAssertLog(textInput, "The TextInput device is no longer enabled");
    Keyboard* keyBoard   = Input::get<Keyboard>();
    CUAssertLog(keyBoard, "The keyboard device is no longer enabled");

    textInput->end();
    if (textInput->currentFocus() == _tkey) {
		textInput->releaseFocus();
    }
    if (keyBoard->currentFocus() == _kkey) {
		keyBoard->releaseFocus();
    }
    
    invokeListeners(true);
    _focused = false;
    return true;
}


#pragma mark -
#pragma mark Rendering
/**
 * Draws this text field via the given SpriteBatch.
 *
 * This method only worries about drawing the current text field.  It does not
 * attempt to render the children.
 *
 * This method provides the correct transformation matrix and tint color.
 * You do not need to worry about whether the node uses relative color.
 * This method is called by render() and these values are guaranteed to be
 * correct.  In addition, this method does not need to check for visibility,
 * as it is guaranteed to only be called when the node is visible.
 *
 * This method overrides the one from {@link Label}. It adds the drawing of
 * a blinking cursor that indicates the edit position.
 *
 * @param batch     The SpriteBatch to draw with.
 * @param transform The global transformation matrix.
 * @param tint      The tint to blend with the Node color.
 */
void TextField::draw(const std::shared_ptr<SpriteBatch>& batch, const Affine2& transform, Color4 tint) {
    Label::draw(batch, transform, tint);

	if (_focused && _showCursor) {
		_cursorBlink--;
		if (_cursorBlink < 0) {
			batch->setTexture(Texture::getBlank());
			batch->setColor(_cursorColor);
			batch->fill(_cursor,Vec2::ZERO,transform);
		}
        if (_cursorBlink == -CURSOR_PERIOD) {
			_cursorBlink = CURSOR_PERIOD;
        }
	}
}

/**
 * Allocates the render data necessary to render this node.
 */
void TextField::generateRenderData() {
    Label::generateRenderData();
    updateCursor();
}



#pragma mark -
#pragma mark Internal Helpers
/**
 * Updates the text with the given input data.
 *
 * This method is a callback for the {@link TextInput} device.
 *
 * @param event        The text input event to be handled.
 * @param focus        Whether the text field currently has text input focus.
 */
void TextField::updateInput(const TextInputEvent& event, bool focus) {
    if (!_focused) {
        return;
    }

    if (!_font->hasGlyph(event.unicode)) {
        // Freebee.  Add it
        _cursorIndex = insertChar(event.unicode, _cursorIndex);
        invokeListeners(false);
        return;
    }
    
    // Find the row
    TextLayout::Row* row = nullptr;
    if (_cursorIndex == _layout->_text.size()) {
        row = &(_layout->_rows.back());
    } else {
        for(auto it = _layout->_rows.begin(); it != _layout->_rows.end();) {
            if (_cursorIndex <= it->end) {
                row = &(*it);
                it = _layout->_rows.end();
            } else {
                ++it;
            }
        }
    }
    if (row == nullptr) {
        return;
    }
    
    // Find the elements before after
    std::string* text = &(_layout->_text);
    Uint32 prev = 0;
    Uint32 next = 0;
    if (_cursorIndex > row->begin) {
        const char* pos = text->c_str()+_cursorIndex;
        prev = utf8::prior(pos, text->c_str());
    }
    if (_cursorIndex < row->end) {
        const char* pos = text->c_str()+_cursorIndex;
        next = utf8::next(pos,text->c_str()+text->size());
    }
    
    // Compute the new space
    Font::Metrics metrics = _font->getMetrics(event.unicode);
    float space = metrics.advance;
    if (prev != 0) {
        space -= _font->getKerning(prev, event.unicode);
    }
    if (next != 0) {
        space -= _font->getKerning(event.unicode,next);
    }
    if (prev != 0 && next != 0) {
        space += _font->getKerning(prev,next);
    }
    
    Rect interior = getInterior();
    if (row->exterior.size.width+space < interior.size.width) {
        // Also painless
        _cursorIndex = insertChar(event.unicode, _cursorIndex);
        invokeListeners(false);
        return;
    }
    
    // We only proceed if we are multiline
    if (_layout->_breakline <= 0) {
        return;
    }

    size_t oldindex = _cursorIndex;
    _cursorIndex = insertChar(event.unicode, _cursorIndex);

    // Reject the input if too big (happens if too many lines)
    float height = _font->getAscent()-_font->getDescent();
    height += (_layout->_rows.size()-1)*_layout->_spacing*_font->getHeight();
    if (interior.size.height < height) {
        text->erase(oldindex,_cursorIndex-oldindex);
        _cursorIndex = oldindex;
        _layout->invalidate();
        _layout->layout();
    } else {
        invokeListeners(false);
    }
    reanchor();
}

/**
 * Updates the state of any special keys.
 *
 * This method is a callback for the {@link Keyboard} device.
 *
 * @param event     The key event to be handled.
 * @param focus     Whether the text field currently has keyboard focus
 * @param down      Whether the key is pressed down (as opposed to released)
 */
void TextField::updateKey(const KeyEvent& Event, bool focus, bool down) {
    if (!_focused) {
        return;
    }
    
    // Break any existing key repeats
    _keyCount++;
    
    // First detect the modifiers
    switch (Event.keycode) {
    case KeyCode::LEFT_ALT:
    case KeyCode::RIGHT_ALT:
        _altDown = down;
        break;
    case KeyCode::LEFT_META:
    case KeyCode::RIGHT_META:
        _metaDown = down;
        break;
    case KeyCode::LEFT_SHIFT:
    case KeyCode::RIGHT_SHIFT:
        _shiftDown = down;
        break;
    default:
        break;
    }
    
    if (!down) {
        return;
    }
    
    // Down specific functionality.
    size_t starttime = _keyCount;
    std::weak_ptr<TextField> ptr = std::dynamic_pointer_cast<TextField>(shared_from_this());
    switch (Event.keycode) {
        case KeyCode::BACKSPACE:
            deleteChar();
            Application::get()->schedule(REPEAT(ptr,deleteChar,starttime), DELETE_DELAY, CURSOR_PERIOD);
            break;
        case KeyCode::ARROW_LEFT:
            leftArrow();
            Application::get()->schedule(REPEAT(ptr,leftArrow,starttime), DELETE_DELAY, CURSOR_PERIOD);
            break;
        case KeyCode::ARROW_RIGHT:
            rightArrow();
            Application::get()->schedule(REPEAT(ptr,rightArrow,starttime), DELETE_DELAY, CURSOR_PERIOD);
            break;
        case KeyCode::ARROW_UP:
            upArrow();
            Application::get()->schedule(REPEAT(ptr,upArrow,starttime), DELETE_DELAY, CURSOR_PERIOD);
            break;
        case KeyCode::ARROW_DOWN:
            downArrow();
            Application::get()->schedule(REPEAT(ptr,downArrow,starttime), DELETE_DELAY, CURSOR_PERIOD);
            break;
        case KeyCode::ENTER:
        case KeyCode::KEYPAD_ENTER:
            releaseFocus();
            break;
        case KeyCode::RETURN:
            if (_shiftDown) {
                _cursorIndex = breakLine(_cursorIndex);
                invokeListeners(false);
            } else {
                releaseFocus();
            }
            break;
        default:
            break;
    }
}

/**
 * Responds to a touch or press, changing the field focus.
 *
 * If the press is outside text field, focus will be released. If the
 * press is within the bounds, it will request focus (if not currently
 * in focus) and move the cursor to the position pressed.
 *
 * @param pos   The screen coordinate where the event happened.
 */
void TextField::updatePress(Vec2 pos, bool focus) {
    Vec2 localPos = screenToNodeCoords(pos);
    if (!Rect(Vec2::ZERO, getContentSize()).contains(localPos)) {
        if (_focused) {
            releaseFocus();
        }
    } else {
        if (!_focused) {
            requestFocus();
        }
        
        // Convert to layout coordinates
        localPos -= _offset;
        
        // See which row we hit
        TextLayout::Row* row = nullptr;
        size_t lineno = 0;
        for(auto it = _layout->_rows.begin(); it != _layout->_rows.end();) {
            if (it->exterior.origin.y <= localPos.y) {
                row = &(*it);
                it = _layout->_rows.end();
            } else {
                lineno++;
                ++it;
            }
        }
        
        if (row == nullptr) {
            _cursorBlink = 0;
            updateCursor();
            return;
        }
        
        // Now see how far we clicked (cursor before clicked letter)
        float width  = localPos.x;
        _cursorIndex = getCharIndex(lineno, width);
        
        // We have already done the hard part of updating the cursor
        _cursor.origin.set(width,row->exterior.origin.y);
        _cursor.origin += _offset;
        _cursor.origin.x -= _cursorWidth/2.0f;
        _cursor.size.width  = _cursorWidth;
        _cursor.size.height = row->exterior.size.height;
        _cursorBlink = 0;
    }
}

/**
 * Updates the cursor position.
 *
 * This method is called whenever either the text changes or the cursor
 * moves. Notice that this must be updated even if the cursor is not
 * visible.
 */
void TextField::updateCursor() {
    Vec2 cursorPos;
    size_t lineno = getCharRow();
    TextLayout::Row* row = &(_layout->_rows[lineno]);

    const char* begin = _layout->_text.c_str()+row->begin;
    const char* end = _layout->_text.c_str()+row->end;
    bool track = _layout->doesTrack(lineno);

    std::vector<int> tracking;
    if (track) {
        tracking = _font->getTracking(begin, end, _layout->_breakline);
        tracking.push_back(0);
    }
    
    cursorPos.x = row->exterior.origin.x;
    cursorPos.y = row->exterior.origin.y;
    if (_cursorIndex >= row->end && !track) {
        cursorPos.x = row->exterior.size.width+row->exterior.origin.x;
    } else if (_cursorIndex > row->begin) {
        end = _layout->_text.c_str()+_cursorIndex;
        Uint32 pcode = 0;
        size_t tpos = 0;
        while (begin != end) {
            Uint32 ccode = utf8::next(begin,end);
            if (_font->hasGlyph(ccode)) {
                cursorPos.x += _font->getMetrics(ccode).advance;
                if (pcode > 0) {
                    cursorPos.x -= _font->getKerning(pcode,ccode);
                }
                if (track) {
                    cursorPos.x += tracking[tpos];
                }
            } else {
                ccode = 0;
            }
            pcode = ccode;
            tpos++;
        }
    }
    cursorPos += _offset;
    cursorPos.x -= _cursorWidth/2.0f;
    
    _cursor.origin = cursorPos;
    _cursor.size.width  = _cursorWidth;
    _cursor.size.height = row->exterior.size.height;
}

/**
 * Moves the cursor one word forward or backward.
 *
 * If there is any space between the cursor and the word in the correct
 * direction, it will move adjacent to the word.  Otherwise, it will
 * skip over the word.
 *
 * @param forward   Whether to move the cursor forward.
 *
 * @return the index of the new cursor position
 */
size_t TextField::skipWord(bool forward) const {
    const char* begin  = _layout->_text.c_str();
    const char* end    = begin+_layout->_text.size();
    const char* cursor = begin+_cursorIndex;
    if (forward) {
        if (_cursorIndex == _layout->_text.size()) {
            return _cursorIndex;
        }
        
        Uint32 code = utf8::next(cursor, end);
        UnicodeType type = strtool::getUnicodeType(code);
        bool tospace = type == UnicodeType::CHAR || UnicodeType::CJK;
        bool atend = false;
        
        if (cursor == end) {
            return _cursorIndex+1;
        }
        
        while (!atend && cursor != end) {
            code = utf8::next(cursor, end);
            type = strtool::getUnicodeType(code);
            if (tospace) {
                atend = !(type == UnicodeType::CHAR || UnicodeType::CJK);
            } else {
                atend = (type == UnicodeType::CHAR || UnicodeType::CJK);
            }
        }
        return cursor-begin-1;
    } else {
        if (_cursorIndex == 0) {
            return _cursorIndex;
        }
        
        Uint32 code = utf8::prior(cursor, begin);
        UnicodeType type = strtool::getUnicodeType(code);
        bool tospace = type == UnicodeType::CHAR || UnicodeType::CJK;
        bool atbegin = false;
        
        if (cursor == begin) {
            return 0;
        }
        
        while (!atbegin && cursor != begin) {
            code = utf8::prior(cursor, begin);
            type = strtool::getUnicodeType(code);
            if (tospace) {
                atbegin = !(type == UnicodeType::CHAR || UnicodeType::CJK);
            } else {
                atbegin = (type == UnicodeType::CHAR || UnicodeType::CJK);
            }
        }
        return cursor-begin+1;
    }
}

/**
 * Deletes one character before the current cursor.
 *
 * If alt is pressed, the method will delete an entire word. If meta
 * is pressed, it will delete the entire field. If the deletion key
 * is held down, this method will be called multiple times in a row
 * after a short delay, lasting until the key is released.
 */
void TextField::deleteChar() {
	if (_cursorIndex == 0) {
        return;
    }

    if (_metaDown) {
        // Clear all the things
        _layout->_text.clear();
        _layout->invalidate();
        _layout->layout();
        _cursorIndex = 0;
    } else if (_layout->_rows.size() == 1) {
        // Are we one line?  Easy
        TextLayout::Row* row = &(_layout->_rows.front());
        size_t start;
        if (_altDown) {
            start = skipWord(false);
        } else {
            // Remember, one unicode character may be may elements
            const char* prev = _layout->_text.c_str()+_cursorIndex;
            utf8::prior(prev, _layout->_text.c_str());
            start = prev-_layout->_text.c_str();
        }
        _layout->_text.erase(start,_cursorIndex-start);
        row->end -= (_cursorIndex-start);
        _layout->invalidate();
        _layout->layout();
        _cursorIndex = start;
    } else {
        // Find our row
        TextLayout::Row* row = nullptr;
        size_t lineno = 0;
        if (_cursorIndex == _layout->_text.size()) {
            row = &(_layout->_rows.back());
            lineno = _layout->_rows.size()-1;
        } else {
            for(auto it = _layout->_rows.begin(); it != _layout->_rows.end();) {
                if (_cursorIndex <= it->end) {
                    row = &(*it);
                    lineno = it-_layout->_rows.begin();
                    it = _layout->_rows.end();
                } else {
                    ++it;
                }
            }
        }
        
        size_t start;
        if (_altDown) {
            start = skipWord(false);
        } else {
            // Remember, one unicode character may be may elements
            const char* prev = _layout->_text.c_str()+_cursorIndex;
            utf8::prior(prev, _layout->_text.c_str());
            start = prev-_layout->_text.c_str();
        }

        _layout->_text.erase(start,_cursorIndex-start);
        _layout->invalidate();
        _layout->layout();
        _cursorIndex = start;
    }
    
    clearRenderData();
    reanchor();
}

/**
 * Moves the cursor one position to the left.
 *
 * If the label is multiline, this will move the cursor from the beginning
 * of line to the end of the next. Nothing will happen if the cursor is
 * at the start of the text.
 */
void TextField::leftArrow() {
    if (_cursorIndex > 0) {
        size_t left = 0;
        if (_altDown) {
            left = skipWord(false);
        } else if (!_metaDown) {
            const char* begin = _layout->_text.c_str();
            const char* pos = begin+_cursorIndex;
            utf8::prior(pos, begin);
            left = pos-begin;
        }
        _cursorIndex = left;
        _cursorBlink = 0;
        updateCursor();
    }
}

/**
 * Moves the cursor one position to the right.
 *
 * If the label is multiline, this will move the cursor from the end of a
 * line to the beginning of the next. Nothing will happen if the cursor is
 * at the end of the text.
 */
void TextField::rightArrow() {
    if (_cursorIndex < _layout->_text.size()) {
        size_t right = _layout->_text.size();
        if (_altDown) {
            right = skipWord(true);
        } else if (!_metaDown) {
            const char* begin = _layout->_text.c_str();
            const char* end = _layout->_text.c_str()+_layout->_text.size();
            const char* pos = begin+_cursorIndex;
            utf8::next(pos,end);
            right = pos-begin;
        }
        _cursorIndex = right;
        _cursorBlink = 0;
        updateCursor();
    }
}

/**
 * Moves the cursor one line up.
 *
 * The cursor is moved "visually". That is, it is moved to the edit position
 * that is closest horizontally to the original position.  In the case of
 * monospaced fonts, this insures that the cursor maintains the same number
 * of characters from the start of the line.  However, this is not the case
 * for proportional fonts.
 */
void TextField::upArrow() {
    size_t lineno = getCharRow();
    if (lineno > 0) {
        // Use the center of the cursor to location position
        float width = _cursor.origin.x+_cursor.size.width/2-_offset.x;
        _cursorIndex = getCharIndex(lineno-1, width);

        // We have already done the hard part of updating the cursor
        Rect* rect = &(_layout->_rows[lineno-1].exterior);
        _cursor.origin.set(width,rect->origin.y);
        _cursor.origin += _offset;
        _cursor.origin.x -= _cursorWidth/2.0f;
        _cursor.size.width  = _cursorWidth;
        _cursor.size.height = rect->size.height;
        _cursorBlink = 0;
    }
}

/**
 * Moves the cursor one line down.
 *
 * The cursor is moved "visually". That is, it is moved to the edit position
 * that is closest horizontally to the original position.  In the case of
 * monospaced fonts, this insures that the cursor maintains the same number
 * of characters from the start of the line.  However, this is not the case
 * for proportional fonts.
 */
void TextField::downArrow() {
    size_t lineno = getCharRow();
    if (lineno < _layout->_rows.size()-1) {
        // Use the center of the cursor to location position
        float width = _cursor.origin.x+_cursor.size.width/2-_offset.x;
        _cursorIndex = getCharIndex(lineno+1, width);

        // We have already done the hard part of updating the cursor
        Rect* rect = &(_layout->_rows[lineno+1].exterior);
        _cursor.origin.set(width,rect->origin.y);
        _cursor.origin += _offset;
        _cursor.origin.x -= _cursorWidth/2.0f;
        _cursor.size.width  = _cursorWidth;
        _cursor.size.height = rect->size.height;
        _cursorBlink = 0;

    }
}

/**
 * Inserts the given unicode character into the text
 *
 * This method will force a recomputation of the layout.
 *
 * @param unicode   The unicode character
 * @param pos       The position to insert the character
 */
size_t TextField::insertChar(Uint32 unicode, size_t pos) {
    // Get the character
    char elements[5];
    char* start = elements;
    start = utf8::append(unicode, start);
    size_t amt = start-elements;
    elements[amt] = '\0';
    
    std::string* text = &(_layout->_text);
    text->insert(pos, elements);
    
    // Update the rows
    _layout->invalidate();
    _layout->layout();
    clearRenderData();
    reanchor();
    return pos+amt;
}

/**
 * Inserts a newline character into the text
 *
 * This method will force a recomputation of the layout.
 *
 * @param pos       The position to insert the newline
 */
size_t TextField::breakLine(size_t pos) {
    size_t lines = _layout->_rows.size();
    float space = _font->getHeight()*_layout->_spacing*lines;
    space += _font->getHeight();
    if (space > getInterior().size.height) {
        return pos; // No room.  Abort
    }
    
    _layout->_text.insert(pos, "\n");
    _layout->invalidate();
    _layout->layout();
    clearRenderData();
    reanchor();
    return pos+1;
}

/**
 * Returns the index for the given row and x-coordinate
 *
 * The method is used to place the cursor position from either
 * a mouse/touch click, or an up/down arrow. The value offset
 * is adjusted to be center of the cursor, preventing us from
 * having to call {@link #updateCursor}.
 *
 * In determining the cursor position, this method find the
 * nearest character to offset. It moves the cursor to either
 * the left or right of this character depending on which side
 * of the character centerline this offset sits.
 *
 * @param row       The text row
 * @param offset    The x-coordinate to place the cursor
 *
 * @return the index for the given row and x-coordinate
 */
size_t TextField::getCharIndex(size_t row, float& offset) const {
    TextLayout::Row* line = &(_layout->_rows[row]);
    const char* begin = _layout->_text.c_str();
    const char* end = begin+line->end;
    const char* prev = begin+line->begin;
    const char* curr = prev;

    bool track = _layout->doesTrack(row);
    std::vector<int> adjusts;
    if (track) {
        adjusts = _font->getTracking(curr, end, _layout->_breakline);
        adjusts.push_back(0);
    }
    
    Uint32 pcode = 0;
    Uint32 ccode = 0;
    size_t index = 0;
    size_t tpos = 0;
    bool found = false;
    float width = line->exterior.origin.x;
    while (curr != end && !found) {
        ccode = utf8::next(curr,end);
        float advance = 0;
        if (_font->hasGlyph(ccode)) {
            advance = _font->getMetrics(ccode).advance;
            if (pcode != 0) {
                advance -= _font->getKerning(pcode, ccode);
            }
            if (track) {
                advance += adjusts[tpos];
            }
        } else {
            ccode = 0;
        }
        if (width+advance >= offset) {
            found = true;
            if (width+advance-offset < advance/2) {
                width += advance;
                index = curr-begin;
            } else {
                index = prev-begin;
            }
        } else {
            width += advance;
        }
        pcode = ccode;
        prev = curr;
        tpos++;
    }
    if (!found && width < offset) {
        index = line->end;
    }
    
    offset = width;
    return index;
}

/**
 * Returns the text row for the current cursor position.
 *
 * @return the text row for the current cursor position.
 */
size_t TextField::getCharRow() const {
    if (_cursorIndex == _layout->_text.size()) {
        return _layout->_rows.size()-1;
    } else {
        size_t lineno = 0;
        for(auto it = _layout->_rows.begin(); it != _layout->_rows.end();) {
            if (_cursorIndex <= it->end) {
                // Rare occurance due to space swallowing
                if (_cursorIndex < it->begin) {
                    lineno--;
                }
                it = _layout->_rows.end();
            } else {
                lineno++;
                ++it;
            }
        }
        return lineno;
    }
}

/**
 * Invokes the appropriate listeners for this text field.
 *
 * If exit is true, it invokes the exit listeners.  Otherwise it
 * invokes the type listeners.
 *
 * @param exit  Whether to invoke the exit listeners
 */
void TextField::invokeListeners(bool exit) {
    if (exit) {
        for(auto it = _exitListeners.begin(); it != _exitListeners.end(); ++it) {
            it->second(getName(), _layout->_text);
        }
    } else {
        for(auto it = _typeListeners.begin(); it != _typeListeners.end(); ++it) {
            it->second(getName(), _layout->_text);
        }
    }
}

