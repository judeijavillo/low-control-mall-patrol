//
//  CUTextInput.cpp
//  Cornell University Game Library (CUGL)
//
//  This class is an object-oriented interface to the SDL text input system.
//  We have tried to keep this class as minimal as possible to make it as
//  flexible as possible. Previous versions of this class did not accurately
//  reflect SDL text input.
//
//  This class is a singleton and should never be allocated directly.  It
//  should only be accessed via the Input dispatcher.
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
//  Author: Walker White
//  Version: 8/1/21
//
#include <cugl/input/CUTextInput.h>
#include <cugl/util/CUDebug.h>
#include <utf8/utf8.h>

using namespace cugl;

#pragma mark Events
/**
 * Constructs a new text input event with the given unicode value
 *
 * The timestamp will be the time of construction.
 *
 * @param code      The unicode character
 */
TextInputEvent::TextInputEvent(Uint32 code) {
    unicode = code;
    timestamp.mark();
}

/**
 * Constructs a new text input event with the given values
 *
 * @param code      The unicode character
 * @param stamp     The timestamp for the event
 */
TextInputEvent::TextInputEvent(Uint32 code, const Timestamp& stamp) {
    unicode = code;
    timestamp = stamp;
}

/**
 * Constructs a new text edit event with the given buffer and edit positions.
 *
 * The timestamp will be the time of construction.
 *
 * @param text      The current text editing buffer
 * @param begin     The beginning edit position in the buffer
 * @param end       The ending (not inclusive) edit position
 */
TextEditEvent::TextEditEvent(const std::string text, size_t begin, size_t end) {
    buffer = text;
    this->begin = begin;
    this->end = end;
    timestamp.mark();
}

/**
 * Constructs a new text edit event with the given values
 *
 * @param text      The current text editing buffer
 * @param begin     The beginning edit position in the buffer
 * @param end       The ending (not inclusive) edit position
 * @param stamp     The timestamp for the event
 */
TextEditEvent::TextEditEvent(const std::string text, size_t begin, size_t end,
                             const Timestamp& stamp) {
    buffer = text;
    this->begin = begin;
    this->end = end;
    timestamp = stamp;
}


#pragma mark -
#pragma mark Constructors
/**
 * Creates and initializes a new text input device.
 *
 * WARNING: Never allocate a text input device directly.  Always use the
 * {@link Input#activate()} method instead.
 */
TextInput::TextInput() : InputDevice(),
_active(false) {}

/**
 * Deletes this input device, disposing of all resources
 */
void TextInput::dispose() {
    _inputListeners.clear();
    _editListeners.clear();
}

#pragma mark -
#pragma mark Activation
/**
 * Start accepting text with this device
 *
 * Until this method is called, no input will ever resolve (though the key
 * strokes may still be detected by the {@link Keyboard device}). Once the
 * method is called, input will continue resolve until the method
 * {@link end()} is called.
 *
 * This device maintains no internal state. All input is communicated
 * immediately to the listeners as soon as it resolves.
 */
void TextInput::begin() {
    _active = true;
    SDL_StartTextInput();
}

/**
 * Stop accepting text with this device
 *
 * Once the method is called, no more input will resolve (though the key
 * strokes may still be detected by the {@link Keyboard device}).
 */
void TextInput::end() {
    _active = false;
    SDL_StopTextInput();
}


#pragma mark -
#pragma mark Listeners
/**
 * Requests focus for the given identifier
 *
 * Only an active listener can have focus. This method returns false if
 * the key does not refer to an active listener (of any type). Note that
 * keys may be shared across listeners of different types, but must be
 * unique for each listener type.
 *
 * @param key   The identifier for the focus object
 *
 * @return false if key does not refer to an active listener
 */
bool TextInput::requestFocus(Uint32 key) {
    if (isListener(key)) {
        _focus = key;
        return true;
    }
    return false;
}

/**
 * Returns true if key represents a listener object
 *
 * An object is a listener if it is a listener for either editing or input.
 *
 * @param key   The identifier for the listener
 *
 * @return true if key represents a listener object
 */
bool TextInput::isListener(Uint32 key) const {
    bool result = _inputListeners.find(key) != _inputListeners.end();
    result = result || _editListeners.find(key) != _editListeners.end();
    return result;
}

/**
 * Returns the text input listener for the given object key
 *
 * If there is no listener for the given key, it returns nullptr.
 *
 * This listener is invoked when input resolves to a unicode character.
 *
 * @param key   The identifier for the listener
 *
 * @return the text input listener for the given object key
 */
const TextInput::InputListener TextInput::getInputListener(Uint32 key) const {
    if (_inputListeners.find(key) != _inputListeners.end()) {
        return (_inputListeners.at(key));
    }
    return nullptr;
}

/**
 * Returns the text editing listener for the given object key
 *
 * If there is no listener for the given key, it returns nullptr.
 *
 * This listener is invoked when the text input has received keystrokes
 * starting a unicode character, but the character has not yet resolved.
 *
 * @param key   The identifier for the listener
 *
 * @return the text editing listener for the given object key
 */
const TextInput::EditListener TextInput::getEditListener(Uint32 key) const {
    if (_editListeners.find(key) != _editListeners.end()) {
        return (_editListeners.at(key));
    }
    return nullptr;
}


/**
 * Adds a text input listener for the given object key
 *
 * There can only be one input listener for a given key (though you may
 * share keys across other listener types). If a listener already exists
 * for the key, the method will fail and return false. You must remove
 * a listener before adding a new one for the same key.
 *
 * This listener is invoked when input resolves to a unicode character.
 *
 * @param key       The identifier for the input listener
 * @param listener    The listener to add
 *
 * @return true if the listener was succesfully added
 */
bool TextInput::addInputListener(Uint32 key, InputListener listener) {
    if (_inputListeners.find(key) == _inputListeners.end()) {
        _inputListeners[key] = listener;
        return true;
    }
    return false;
}

/**
 * Adds a text editing listener for the given object key
 *
 * There can only be one edit listener for a given key (though you may
 * share keys across other listener types). If a listener already exists
 * for the key, the method will fail and return false. You must remove
 * a listener before adding a new one for the same key.
 *
 * This listener is invoked when the text input has received keystrokes
 * starting a unicode character, but the character has not yet resolved.
 *
 * @param key       The identifier for the edit listener
 * @param listener  The listener to add
 *
 * @return true if the listener was succesfully added
 */
bool TextInput::addEditListener(Uint32 key, EditListener listener) {
    if (_editListeners.find(key) == _editListeners.end()) {
        _editListeners[key] = listener;
        return true;
    }
    return false;
}

/**
 * Removes the text input listener for the given object key
 *
 * If there is no active listener for the given key, this method fails and
 * returns false.
 *
 * This listener is invoked when input resolves to a unicode character.
 *
 * @param key   The identifier for the listener
 *
 * @return true if the listener was succesfully removed
 */
bool TextInput::removeInputListener(Uint32 key) {
    if (_inputListeners.find(key) != _inputListeners.end()) {
        _inputListeners.erase(key);
        return true;
    }
    return false;
}

/**
 * Removes the text edit listener for the given object key
 *
 * If there is no active listener for the given key, this method fails and
 * returns false.
 *
 * This listener is invoked when the text input has received keystrokes
 * starting a unicode character, but the character has not yet resolved.
 *
 * @param key   The identifier for the listener
 *
 * @return true if the listener was succesfully removed
 */
bool TextInput::removeEditListener(Uint32 key) {
    if (_editListeners.find(key) != _editListeners.end()) {
        _editListeners.erase(key);
        return true;
    }
    return false;
}

#pragma mark -
#pragma mark Input Device
/**
 * Processes an SDL_Event
 *
 * The dispatcher guarantees that an input device only receives events that
 * it subscribes to.
 *
 * @param event The input event to process
 * @param stamp The event timestamp in CUGL time
 *
 * @return false if the input indicates that the application should quit.
 */
bool TextInput::updateState(const SDL_Event& event, const Timestamp& stamp) {
    switch (event.type) {
        case SDL_TEXTEDITING:
            if (_editListeners.size() > 0) {
                // Send the edit information as is
                TextEditEvent tevent(event.edit.text,event.edit.start,
                                     event.edit.start+event.edit.length,stamp);
                for(auto it = _editListeners.begin(); it  != _editListeners.end(); ++it) {
                    it->second(tevent,it->first==_focus);
                }
            }
            break;
        case SDL_TEXTINPUT:
            if (_inputListeners.size() > 0) {
                // Resolve the unicode characters as individual events
                const char* begin = event.text.text;
                const char* end = begin+strlen(event.text.text);
                while (begin != end) {
                    TextInputEvent tevent(utf8::next(begin,end),stamp);
                    for(auto it = _inputListeners.begin(); it  != _inputListeners.end(); ++it) {
                        it->second(tevent,it->first==_focus);
                    }
                }
            }
            break;
        default:
            // Do nothing
            break;
    }
    return true;
}

/**
 * Determine the SDL events of relevance and store there types in eventset.
 *
 * An SDL_EventType is really Uint32.  This method stores the SDL event
 * types for this input device into the vector eventset, appending them
 * to the end. The Input dispatcher then uses this information to set up
 * subscriptions.
 *
 * @param eventset  The set to store the event types.
 */
void TextInput::queryEvents(std::vector<Uint32>& eventset) {
    eventset.push_back((Uint32)SDL_TEXTEDITING);
    eventset.push_back((Uint32)SDL_TEXTINPUT);
}
