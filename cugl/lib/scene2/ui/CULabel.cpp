//
//  CULabel.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a scene graph node that displays a single line of text.
//  We have moved multiline text support to a separate class (which has not
//  yet been implemented).
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
//  Author: Walker White
//  Version: 7/30/21
//
#include <algorithm>
#include <cugl/scene2/ui/CULabel.h>
#include <cugl/assets/CUScene2Loader.h>
#include <cugl/assets/CUAssetManager.h>
#include <cugl/render/CUFont.h>
#include <cugl/render/CUTextLayout.h>

using namespace cugl;
using namespace cugl::scene2;

/** Placeholder for JSON parsing */
#define UNKNOWN_STR "<unknown>"
/** The color for the drop shadow */
#define DROP_COLOR  Color4(0,0,0,128)

#pragma mark -
#pragma mark Constructors

/**
 * Creates an uninitialized label with no text or font information.
 *
 * You must initialize this Label before use.
 *
 * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate a Node on the
 * heap, use one of the static constructors instead.
 */
Label::Label() : SceneNode(),
_foreground(Color4::BLACK),
_background(Color4::CLEAR),
_padleft(0),
_padbot(0),
_padrght(0),
_padtop(0),
_rendered(false),
_dropShadow(false),
_dropBlur(0),
_blendEquation(GL_FUNC_ADD),
_srcFactor(GL_SRC_ALPHA),
_dstFactor(GL_ONE_MINUS_SRC_ALPHA) {
    _classname = "Label";
}

/**
 * Disposes all of the resources used by this label.
 *
 * A disposed Label can be safely reinitialized. Any children owned by this
 * node will be released.  They will be deleted if no other object owns them.
 *
 * It is unsafe to call this on a Label that is still currently inside of
 * a scene graph.
 */
void Label::dispose() {
    clearRenderData();
    _layout = nullptr;
    _font = nullptr;
    _foreground = Color4::BLACK;
    _background = Color4::CLEAR;
    _padleft = 0;
    _padbot  = 0;
    _padrght = 0;
    _padtop  = 0;
    _dropShadow = false;
    _dropBlur = 0;
    _dropOffset.setZero();
    _rendered = false;
    _blendEquation = GL_FUNC_ADD;
    _srcFactor = GL_SRC_ALPHA;
    _dstFactor = GL_ONE_MINUS_SRC_ALPHA;
    SceneNode::dispose();
}

/**
 * Initializes a label with the given size.
 *
 * The size defines the content size. The bounding box of the node is
 * (0,0,width,height) and is anchored in the bottom left corner (0,0).
 * The node is positioned at the origin in parent space.
 *
 * The label will empty, as it has no font or text.
 *
 * @param size  The size of the node in parent space
 *
 * @return true if initialization was successful.
 */
bool Label::initWithBounds(const Size size) {
    if (SceneNode::initWithBounds(size)) {
        _font = nullptr;
        _layout = TextLayout::alloc();
        _layout->setVerticalAlignment(VerticalAlign::TOP);
        return true;
    }
    return false;
}


/**
 * Initializes a node with the given bounds.
 *
 * The rectangle origin is the bottom left corner of the node in parent
 * space, and corresponds to the origin of the Node space. The size
 * defines its content width and height in node space. The node anchor
 * is placed in the bottom left corner.
 *
 * If the font is missing glyphs in this string, the characters in the text
 * may be different than those displayed. Furthermore, if this label has no
 * font, then the text will not display at all.
 *
 * @param rect  The bounds of the node in parent space
 *
 * @return true if initialization was successful.
 */
bool Label::initWithBounds(const Rect rect)  {
    if (SceneNode::initWithBounds(rect)) {
        _font = nullptr;
        _layout = TextLayout::alloc();
        _layout->setVerticalAlignment(VerticalAlign::TOP);
        return true;
    }
    return false;
}

/**
 * Initializes a label at (0,0) with the given text and font
 *
 * The label will be sized to fit the rendered text exactly. That is, the
 * height will be the maximum height of the font, and the width will be the
 * sum of the advance of the rendered characters.  That means that there
 * may be some natural spacing around the characters.
 *
 * By default, the text will formated so that the origin is on the left
 * edge of the baseline (of the top line). The text will take up a single
 * line unless there are newline characters in the string. If any glyphs
 * are missing from the font atlas, they will not be rendered.
 *
 * @param text  The text to display in the label
 * @param font  The font for this label
 *
 * @return true if initialization was successful.
 */
bool Label::initWithText(const std::string text, const std::shared_ptr<Font>& font) {
    if (font == nullptr) {
        CUAssertLog(false, "The font is undefined");
    } else if (_font != nullptr) {
        CUAssertLog(false, "Label is already initialized");
    } else if (SceneNode::initWithPosition(Vec2::ZERO)) {
        _font = font;
        _layout = TextLayout::allocWithText(text, font);
        _layout->setVerticalAlignment(VerticalAlign::TOP);
        _layout->layout();
        resize();
        reanchor();
        return true;
    }
    return false;
}

/**
 * Initializes a label at the position with the given text and font
 *
 * The label will be sized to fit the rendered text exactly. That is, the
 * height will be the maximum height of the font, and the width will be the
 * sum of the advance of the rendered characters.  That means that there
 * may be some natural spacing around the characters.
 *
 * By default, the text will formated so that the origin is on the left
 * edge of the baseline (of the top line). The text will take up a single
 * line unless there are newline characters in the string. If any glyphs
 * are missing from the font atlas, they will not be rendered.
 *
 * @param position  The label position
 * @param text      The text to display in the label
 * @param font      The font for this label
 *
 * @return true if initialization was successful.
 */
bool Label::initWithText(const Vec2 position, const std::string text, const std::shared_ptr<Font>& font) {
    if (font == nullptr) {
        CUAssertLog(false, "The font is undefined");
    } else if (_font != nullptr) {
        CUAssertLog(false, "Label is already initialized");
    } else if (SceneNode::initWithPosition(position)) {
        _font = font;
        _layout = TextLayout::allocWithText(text, font);
        _layout->setVerticalAlignment(VerticalAlign::TOP);
        _layout->layout();
        resize();
        reanchor();
        return true;
    }
    return false;
}

/**
 * Initializes a multiline label with the given dimensions
 *
 * The label will use the size specified and attempt to fit the text in
 * this region. Lines will be broken at white space locations to keep each
 * line within the size width. However, this may result in so many lines
 * that glyphs at the bottom are cut out. A label will never render text
 * outside of its content bounds.
 *
 * By default, a multiline label is aligned to the top and left. It has
 * a line spacing of 1 (single-spaced).
 *
 * @param size  The size of the label to display
 * @param text  The text to display in the label
 * @param font  The font for this label
 *
 * @return true if initialization was successful.
 */
bool Label::initWithTextBox(const Size size, const std::string text, const std::shared_ptr<Font>& font) {
    if (font == nullptr) {
        CUAssertLog(false, "The font is undefined");
    } else if (_font != nullptr) {
        CUAssertLog(false, "Label is already initialized");
    } else if (SceneNode::initWithBounds(size)) {
        _font = font;
        _layout = TextLayout::allocWithTextWidth(text, font, size.width);
        _layout->setVerticalAlignment(VerticalAlign::TOP);
        _layout->layout();
        reanchor();
        return true;
    }
    return false;
}

/**
 * Initializes a multiline label with the given dimensions
 *
 * The label will use the size specified and attempt to fit the text in
 * this region. Lines will be broken at white space locations to keep each
 * line within the size width. However, this may result in so many lines
 * that glyphs at the bottom are cut out. A label will never render text
 * outside of its content bounds.
 *
 * By default, a multiline label is aligned to the top and left. It has
 * a line spacing of 1 (single-spaced).
 *
 * @param size  The bounds of the label to display
 * @param text  The text to display in the label
 * @param font  The font for this label
 *
 * @return true if initialization was successful.
 */
bool Label::initWithTextBox(const Rect rect, const std::string text, const std::shared_ptr<Font>& font) {
    if (font == nullptr) {
        CUAssertLog(false, "The font is undefined");
    } else if (_font != nullptr) {
        CUAssertLog(false, "Label is already initialized");
    } else if (SceneNode::initWithBounds(rect)) {
        _font = font;
        _layout = TextLayout::allocWithTextWidth(text, font, rect.size.width);
        _layout->setVerticalAlignment(VerticalAlign::TOP);
        _layout->layout();
        reanchor();
        return true;
    }
    return false;
}

/**
 * Initializes a node with the given JSON specificaton.
 *
 * This initializer is designed to receive the "data" object from the
 * JSON passed to {@link SceneLoader}.  This JSON format supports all
 * of the attribute values of its parent class.  In addition, it supports
 * the following additional attributes:
 *
 *     "font":       The name of a previously loaded font asset
 *     "text":       The initial label text
 *     "foreground": Either a four-element integer array (values 0..255) or a string.
 *                   Any string should be a web color or a Tkinter color name.
 *     "background": Either a four-element integer array (values 0..255) or a string.
 *                   Any string should be a web color or a Tkinter color name.
 *     "padding":    A number or a 4-element float array.
 *     "dropshadow": A boolean indicating the presence of a drop shadow
 *     "wrap":       A boolean indicating whether to break text into lines
 *     "spacing":    A float indicating the spacing factor between lines
 *     "halign":     One of 'left', 'center', 'right', 'justify', 'hard left',
 *                   'true center' and 'hard right'.
 *     "valign":     One of 'top', 'middle', 'bottom', 'hard top', 'true middle'
 *                   and 'hard bottom'.
 *
 * All attributes are optional. There are no required attributes.  However,
 * a label without a font cannot display text.
 *
 * @param loader    The scene loader passing this JSON file
 * @param data      The JSON object specifying the node
 *
 * @return true if initialization was successful.
 */
bool Label::initWithData(const Scene2Loader* loader, const std::shared_ptr<JsonValue>& data) {
    if (_font != nullptr) {
        CUAssertLog(false, "Label is already initialized");
        return false;
    } else if (!data) {
        return init();
    } else if (!SceneNode::initWithData(loader, data)) {
        return false;
    }
    
    // All of the code that follows can corrupt the position.
    Vec2 coord = getPosition();
    
    // Set the font
    const AssetManager* assets = loader->getManager();
    auto font = assets->get<Font>(data->getString("font",UNKNOWN_STR));
    if (font == nullptr) {
        CUAssertLog(false, "The font is undefined");
        return false;
    }
    
    _font = font;
    if (data->has("text")) {
        _layout = TextLayout::allocWithText(data->getString("text"), font);
    } else {
        _layout = TextLayout::alloc();
        _layout->setFont(font);
    }

    if (data->has("foreground")) {
        JsonValue* col = data->get("foreground").get();
        if (col->isString()) {
            _foreground.set(col->asString("#ffffff"));
        } else {
            CUAssertLog(col->size() >= 4, "'color' must be a four element number array");
            _foreground.r = col->get(0)->asInt(0);
            _foreground.g = col->get(1)->asInt(0);
            _foreground.b = col->get(2)->asInt(0);
            _foreground.a = col->get(3)->asInt(0);
        }
    }
    
    if (data->has("background")) {
        JsonValue* col = data->get("background").get();
        if (col->isString()) {
            _background.set(col->asString("#ffffff"));
        } else {
            CUAssertLog(col->size() >= 4, "'color' must be a four element number array");
            _background.r = col->get(0)->asInt(0);
            _background.g = col->get(1)->asInt(0);
            _background.b = col->get(2)->asInt(0);
            _background.a = col->get(3)->asInt(0);
        }
    }

    if (data->has("padding")) {
        JsonValue* pad = data->get("padding").get();
        if (pad->isNumber()) {
            float val = pad->asFloat(0.0f);
            _padleft = val;
            _padbot  = val;
            _padrght = val;
            _padtop  = val;
        } else {
            CUAssertLog(pad->size() >= 4, "'padding' must be a 4-element array");
            _padleft = pad->get(0)->asFloat(0.0f);
            _padbot  = pad->get(1)->asFloat(0.0f);
            _padrght = pad->get(2)->asFloat(0.0f);
            _padtop  = pad->get(3)->asFloat(0.0f);
        }
    }
    
    if (data->has("halign")) {
        std::string align = data->getString("halign",UNKNOWN_STR);
        HorizontalAlign value = HorizontalAlign::LEFT;
        if (align == "center") {
            value = HorizontalAlign::CENTER;
        } else if (align == "right") {
            value = HorizontalAlign::RIGHT;
        } else if (align == "justify") {
            value = HorizontalAlign::JUSTIFY;
        } else if (align == "hard left") {
            value = HorizontalAlign::HARD_LEFT;
        } else if (align == "true center") {
            value = HorizontalAlign::TRUE_CENTER;
        } else if (align == "hard right") {
            value = HorizontalAlign::HARD_RIGHT;
        }
        _layout->setHorizontalAlignment(value);
    }
    
    if (data->has("valign")) {
        std::string align = data->getString("valign","UNKNOWN_STR");
        VerticalAlign value = VerticalAlign::TOP;
        if (align == "middle") {
            value = VerticalAlign::MIDDLE;
        } else if (align == "top") {
            value = VerticalAlign::TOP;
        } else if (align == "hard bottom") {
            value = VerticalAlign::HARD_BOTTOM;
        } else if (align == "true middle") {
            value = VerticalAlign::TRUE_MIDDLE;
        } else if (align == "hard top") {
            value = VerticalAlign::HARD_TOP;
        }
        _layout->setVerticalAlignment(value);
    } else {
        _layout->setVerticalAlignment(VerticalAlign::TOP);
    }

    if (data->has("spacing")) {
        _layout->setSpacing(data->getFloat("spacing",1));
    }
    
    _dropShadow = data->getFloat("dropshadow",0.0f);
    if (data->has("dropoffset")) {
        JsonValue* pos = data->get("dropoffset").get();
        CUAssertLog(pos->size() >= 2, "'dropoffset' must be a two element number array");
        _dropOffset.x = pos->get(0)->asFloat(0.0f);
        _dropOffset.y = pos->get(1)->asFloat(0.0f);
    }

    bool wrap = data->getBool("wrap",false);
    if (wrap) {
        float width = std::max(0.0f,_contentSize.width-_padleft-_padrght);
        _layout->setWidth(width);
    }
    
    // Format the text
    _layout->layout();
    if (!data->has("size")) {
        resize();
    }
    reanchor();

    // Now redo the position
    setPosition(coord);
    return true;
}


#pragma mark -
#pragma mark Text Attributes
/**
 * Returns the text for this label.
 *
 * The string will be in either ASCII or UTF8 format. No other string
 * encodings are supported.  As all ASCII strings are also UTF8, you can
 * this effectively means that the text must be UTF8.
 *
 * If the font is missing glyphs in this string, the characters in the text
 * may be different than those displayed. Furthermore, if this label has no
 * font, then the text will not display at all.
 *
 * @return the text for this label.
 */
const std::string& Label::getText() const {
    return _layout->getText();
}

/**
 * Sets the text for this label.
 *
 * The string must be in either ASCII or UTF8 format. No other string
 * encodings are supported.  As all ASCII strings are also UTF8, you can
 * this effectively means that the text must be UTF8.
 *
 * If the font is missing glyphs in this string, the characters in the text
 * may be different than those displayed. Furthermore, if this label has no
 * font, then the text will not display at all.
 *
 * Changing this value will regenerate the render data, and is potentially
 * expensive, particularly if the font is using a fallback atlas.
 *
 * @param text      The text for this label.
 * @param resize    Whether to resize the label to fit the new text.
 */
void Label::setText(const std::string& text, bool resize) {
    _layout->setText(text);
    _layout->layout();
    if (resize) {
        this->resize();
    }
    reanchor();
    clearRenderData();
}

/**
 * Sets the font to use this label
 *
 * Changing this value will regenerate the render data, and is potentially
 * expensive, particularly if the font does not have an atlas.
 *
 * @param font      The font to use for this label
 * @param resize    Whether to resize this label to fit the new font
 */
void Label::setFont(const std::shared_ptr<Font>& font, bool resize) {
    _font = font;
    _layout->setFont(font);
    _layout->layout();
    if (resize) {
        this->resize();
    }
    reanchor();
    clearRenderData();
}

/**
 * Returns the horizontal alignment of the text.
 *
 * Horizontal alignment serves two purposes in a label. First it is the
 * alignment of multiple lines of text to each other, as specified in
 * {@link TextLayout}. But it also represents the relationship between
 * the text and the background region of this node. In particular,
 * setting this value has the following effects:
 *
 * LEFT, HARD_LEFT, and JUSTIFY all place the left edge of the text
 * layout against the left edge of the label, after applying padding.
 * Similarly, RIGHT places the right edge of the text layout against
 * the right edge of the label, after applying padding.
 *
 * Padding is only relevant for CENTER and TRUE_CENTER if the label
 * wraps text. In that case, the word wrap places an equal amount of
 * padding around each side. In addition, the label places the text
 * layout (whose origin is the horizontal center of the text) in the
 * center of this label.
 *
 * This value is LEFT by default.
 *
 * @return the horizontal alignment of the text.
 */
HorizontalAlign Label::getHorizontalAlignment() const {
    return _layout->getHorizontalAlignment();
}

/**
 * Sets the horizontal alignment of the text.
 *
 * Horizontal alignment serves two purposes in a label. First it is the
 * alignment of multiple lines of text to each other, as specified in
 * {@link TextLayout}. But it also represents the relationship between
 * the text and the background region of this node. In particular,
 * setting this value has the following effects:
 *
 * LEFT, HARD_LEFT, and JUSTIFY all place the left edge of the text
 * layout against the left edge of the label, after applying padding.
 * Note that the meaning of the "left edge" differs between LEFT and
 * HARD_LEFT in {@link TextLayout}. Similarly, RIGHT and HARD_RIGHT
 * place the right edge of the text layout against the right edge of
 * the label, after applying padding.
 *
 * Padding is only relevant for CENTER and TRUE_CENTER if the label
 * wraps text. In that case, the word wrap places an equal amount of
 * padding around each side. In addition, the label places the text
 * layout (whose origin is the horizontal center of the text) in the
 * center of this label.
 *
 * This value is LEFT by default.
 *
 * @param halign    The horizontal alignment of the text.
 */
void Label::setHorizontalAlignment(HorizontalAlign halign) {
    _layout->setHorizontalAlignment(halign);
    _layout->layout();
    reanchor();
    clearRenderData();
}

/**
 * Returns the vertical alignment of the text.
 *
 * Vertical alignment is used to place the formatted text in against
 * the background rectangle. The options are interpretted as follows:
 *
 * TOP and HARD_TOP place the top edge of the text layout against the
 * top edge of the label, after applying padding. Note that the meaning
 * of "top edge" differs between TOP and HARD_TOP in {@link TextLayout}.
 * Similarly, BOTTOM and HARD_BOTTOM place the bottom edge of the text
 * layout against the bottom edge of the label, after applying padding.
 *
 * MIDDLE and TRUE_MIDDLE do not apply padding. Instead they place the
 * text layout (whose origin is the vertical center of the text) in
 * the center of this label.
 *
 * Finally, for BASELINE, this will place the baseline of the *bottom*
 * line (not the top, as the case with {@link TextLayout}) line at the
 * bottom edge of the label, after adjusting for padding.
 *
 * This value is TOP by default.
 *
 * @return the vertical alignment of the text.
 */
VerticalAlign Label::getVerticalAlignment() const {
    return _layout->getVerticalAlignment();
}

/**
 * Sets the vertical alignment of the text.
 *
 * Vertical alignment is used to place the formatted text in against
 * the background rectangle. The options are interpretted as follows:
 *
 * TOP and HARD_TOP place the top edge of the text layout against the
 * top edge of the label, after applying padding. Note that the meaning
 * of "top edge" differs between TOP and HARD_TOP in {@link TextLayout}.
 * Similarly, BOTTOM and HARD_BOTTOM place the bottom edge of the text
 * layout against the bottom edge of the label, after applying padding.
 *
 * MIDDLE and TRUE_MIDDLE do not apply padding. Instead they place the
 * text layout (whose origin is the vertical center of the text) in
 * the center of this label.
 *
 * Finally, for BASELINE, this will place the baseline of the *bottom*
 * line (not the top, as the case with {@link TextLayout}) line at the
 * bottom edge of the label, after adjusting for padding.
 *
 * This value is TOP by default.
 *
 * @param valign    The horizontal alignment of the text.
 */
void Label::setVerticalAlignment(VerticalAlign valign) {
    _layout->setVerticalAlignment(valign);
    _layout->layout();
    reanchor();
    clearRenderData();
}

/**
 * Returns true if this label will wrap text to fit.
 *
 * By default, label text will be displayed on one line (unless it has
 * newline characters). This could cause the text to exceed the bounds
 * of the content size and be cut off. If this option is true, the label
 * will break up lines at white space locations (or mid-word if there
 * are no white space locations) so that each line can fit in the width
 * of the content region. This could still cause text to be cut off if
 * the height of the content region is not large enough.
 *
 * Not that even if this value is false, a label will still break lines
 * at newline characters. If you do not want the label to break up a
 * string with newlines, you should remove the newline characters from
 * the string (as these are not renderable anyway).
 *
 * @return true if this label will wrap text to fit.
 */
bool Label::getWrap() const {
    return _layout->getWidth() > 0;
}

/**
 * Sets whether this label will wrap text to fit.
 *
 * By default, label text will be displayed on one line (unless it has
 * newline characters). This could cause the text to exceed the bounds
 * of the content size and be cut off. If this option is true, the label
 * will break up lines at white space locations (or mid-word if there
 * are no white space locations) so that each line can fit in the width
 * of the content region. This could still cause text to be cut off if
 * the height of the content region is not large enough.
 *
 * Not that even if this value is false, a label will still break lines
 * at newline characters. If you do not want the label to break up a
 * string with newlines, you should remove the newline characters from
 * the string (as these are not renderable anyway).
 *
 * @param wrap  Whether this label will wrap text to fit.
 */
void Label::setWrap(bool wrap) {
    float width = std::max(0.0f,_contentSize.width-_padleft-_padrght);
    if (wrap && _layout->getWidth() != width) {
        _layout->setWidth(width);
        _layout->layout();
        reanchor();
    } else if (!wrap && _layout->getWidth() != 0) {
        _layout->setWidth(0);
        _layout->layout();
        reanchor();
    }
}

/**
 * Returns the line spacing of this label.
 *
 * This value is multiplied by the font size to determine the space
 * between lines in the label. So a value of 1 is single-spaced text,
 * while a value of 2 is double spaced. The value should be positive.
 *
 * @return the line spacing of this label.
 */
float Label::getSpacing() const {
    return _layout->getSpacing();
}

/**
 * Sets the line spacing of this label.
 *
 * This value is multiplied by the font size to determine the space
 * between lines in the label. So a value of 1 is single-spaced text,
 * while a value of 2 is double spaced. The value should be positive.
 *
 * @param spacing   The line spacing of this label.
 */
void Label::setSpacing(float spacing) {
    if (spacing != _layout->getSpacing()) {
        _layout->setSpacing(spacing);
        _layout->layout();
        reanchor();
    }
}

#pragma mark -
#pragma mark Label Sizing
/**
 * Sets the padding of the label.
 *
 * Padding can be added to the bottom, left, top, and right edges of the
 * label. This padding detracts from the area in which text can appear,
 * and can influence the word wrap. We call the area of the label in
 * which the text can appear the *interior* of the label.
 *
 * Padding can also have effect on the various text alignments. See both
 * {@link #setHorizontalAlignment} and {@link #setVerticalAlignment} for
 * the affect of padding on alignment.
 *
 * @param left      The left edge padding of the label
 * @param bottom    The bottom edge padding of the label
 * @param right     The right edge padding of the label
 * @param top       The top edge padding of the label
 */
void Label::setPadding(float left, float bottom, float right, float top) {
    switch (_layout->getHorizontalAlignment()) {
        case HorizontalAlign::LEFT:
        case HorizontalAlign::HARD_LEFT:
        case HorizontalAlign::JUSTIFY:
            _offset.x += (left-_padleft);
            break;
        case HorizontalAlign::RIGHT:
        case HorizontalAlign::HARD_RIGHT:
            _offset.x -= (right-_padrght);
            break;
        default:
        {
            float ocent = (_padleft-_padrght)/2;
            float ncent = (left-right)/2;
            _offset.x += (ncent-ocent);
        }
            break;
    }
    switch (_layout->getVerticalAlignment()) {
        case VerticalAlign::BOTTOM:
        case VerticalAlign::HARD_BOTTOM:
            _offset.y += (bottom-_padbot);
            break;
        case VerticalAlign::TOP:
        case VerticalAlign::HARD_TOP:
            _offset.y -= (top-_padtop);
            break;
        default:
        {
            float omid = (_padbot-_padtop)/2;
            float nmid = (bottom-top)/2;
            _offset.y += (nmid-omid);
        }
            break;
    }
    
    _contentSize.width  += (left-_padleft)+(right-_padrght);
    _contentSize.height += (bottom-_padbot)+(top-_padtop);
    _padleft = left;
    _padbot  = bottom;
    _padrght = right;
    _padtop  = top;
    if (_layout->getWidth() > 0) {
        float width = std::max(0.0f,_contentSize.width-_padleft-_padrght);
        _layout->setWidth(width);
        _layout->layout();
    }
    reanchor();
    clearRenderData();
}

/**
 * Returns the non-padded interior of this label.
 *
 * Padding can be added to the bottom, left, top, and right edges of the
 * label. This padding detracts from the area in which text can appear,
 * and can influence the word wrap. We call the area of the label in
 * which the text can appear the *interior* of the label.
 *
 * Normally the content bounds of a scene graph node has origin (0,0)
 * and size {@link SceneNode#getContentSize}. This method shifts the
 * origin and reduces the width and height to account for the padding
 * on the edges.
 *
 * @return the non-padded interior of this label.
 */
const Rect Label::getInterior() const {
    Rect result(Vec2::ZERO,getContentSize());
    result.origin.x += _padleft;
    result.origin.y += _padbot;
    result.size.width  -= (_padleft+_padrght);
    result.size.height -= (_padbot+_padtop);
    return result;
}

/**
 * Sets the non-padded interior of this label.
 *
 * Padding can be added to the bottom, left, top, and right edges of the
 * label. This padding detracts from the area in which text can appear,
 * and can influence the word wrap. We call the area of the label in
 * which the text can appear the *interior* of the label.
 *
 * This method is essentially an alternate way to set the padding. The
 * method will add padding so that the interior appears in a region
 * with origin (0,0) and size {@link SceneNode#getContentSize}. If the
 * interior cannot fit in this region, then the interior will be the
 * intersection. This method will never change the content size of the
 * label.
 *
 * @param rect  The non-padded interior of this label.
 */
void Label::setInterior(const Rect rect) {
    Rect interior = rect;
    interior.intersect(Rect(Vec2::ZERO,_contentSize));
    float left = interior.origin.x;
    float bottom = interior.origin.y;
    float right = _contentSize.width-(interior.origin.x+interior.size.width);
    float top = _contentSize.height-(interior.origin.y+interior.size.height);
    setPadding(left,bottom,right,top);
}


/**
 * Returns the bounds of the rendered text.
 *
 * This is the bounds of the rendered text with respect to the Node space.
 * The size of the bounding box is the natural size to render the text.
 * This corresponds to {@link TextLayout#getBounds} of the underlying text
 * layout. In particular, this means there may be some natural spacing
 * around the characters.
 *
 * The origin of the bounds is determined by the padding and alignment.
 * If this rectangle extends outside the bounding box of the label (e.g.
 * the rectangle with origin (0,0) and the content size), then only the
 * part of the rectangle inside the bounding box will be rendered.
 *
 * @return the bounds of the rendered text.
 */
const Rect Label::getTextBounds() const {
    Rect bounds = _layout->getBounds();
    bounds.origin += _offset;
    return bounds;
}

/**
 * Returns the tightest bounds of the rendered text.
 *
 * This is the bounds of the rendered text, with respect to the Node space.
 * The size of the bounding box ignores any nautural spacing around the
 * characters. It also includes any tracking applied to each line. This
 * corresponds to {@link TextLayout#getTrueBounds} of the underlying text
 * layout.
 *
 * The origin of the bounds is determined by the padding and alignment.
 * If this rectangle extends outside the bounding box of the label (e.g.
 * the rectangle with origin (0,0) and the content size), then only the
 * part of the rectangle inside the bounding box will be rendered.
 *
 * @return the tightest bounds of the rendered text.
 */
const Rect Label::getTrueBounds() const {
    Rect bounds = _layout->getTrueBounds();
    bounds.origin += _offset;
    return bounds;
}

/**
 * Sets the untransformed size of the node.
 *
 * The content size remains the same no matter how the node is scaled or
 * rotated. All nodes must have a size, though it may be degenerate (0,0).
 *
 * Changing the size of a rectangle will not change the position of the
 * node.  However, if the anchor is not the bottom-left corner, it will
 * change the origin.  The Node will grow out from an anchor on an edge,
 * and equidistant from an anchor in the center.
 *
 * In addition, if the rendered text is less than the content size, it
 * may be cut off in rendering.
 *
 * @param size  The untransformed size of the node.
 */
void Label::setContentSize(const Size size) {
    SceneNode::setContentSize(size);
    if (_layout->getWidth() > 0) {
        // Force a rewrap
        setWrap(true);
    }
    reanchor();
}

#pragma mark -
#pragma mark Text Coloring
/**
 * Sets the background color of this label.
 *
 * If this color is not CLEAR (the default color), then the label will have
 * a colored backing rectangle.  The rectangle will extended from the origin
 * to the content size in Node space.
 *
 * @param color The background color of this label.
 */
void Label::setBackground(Color4 color) {
    if (_background == color) {
        return;
    } else if (_background == Color4::CLEAR || color == Color4::CLEAR) {
        clearRenderData();
    }
    _background = color;
    updateColor();
}

/**
 * Sets the drop shadow offset of this label
 *
 * A drop shadow is a blurred and/or slightly offset version of the label
 * text, drawn behind the original text. The color of the drop shadow is
 * always a slighlty transparent black. It is used to give a sense of
 * depth to the text.
 *
 * This property controls the offset of the drop shadow but does not
 * control the blur. You must use the property {@link #getShadowBlur} for
 * that. A typical drop shadow is offset down and to the right with minor
 * blurring.
 *
 * @param x     The x-offset of the drop shadow
 * @param y     The x-offset of the drop shadow
 */
void Label::setDropShadow(float x, float y) {
    _dropOffset.set(x,y);
    if (!_dropShadow && !_dropOffset.isZero()) {
        _dropShadow = true;
    }
}

/**
 * Sets the drop shadow blur of this label
 *
 * A drop shadow is a blurred and/or slightly offset version of the label
 * text, drawn behind the original text. The color of the drop shadow is
 * always a slighlty transparent black. It is used to give a sense of
 * depth to the text.
 *
 * This property controls the offset of the drop shadow but does not
 * control the blur. You must use the property {@link #getShadowBlur} for
 * that. A typical drop shadow is offset down and to the right with minor
 * blurring.
 *
 * When blurring a drop shadow, remember to use a font with the same
 * {@link Font#getPadding} as the blur size. This will prevent bleeding
 * across characters in the atlas.
 *
 * @param blur  The drop shadow blur of this label
 */

void Label::setShadowBlur(float blur) {
    _dropBlur = blur;
    _dropShadow = blur > 0 || !_dropOffset.isZero();
}



#pragma mark -
#pragma mark Rendering
/**
 * Draws this Node via the given SpriteBatch.
 *
 * This method only worries about drawing the current node.  It does not
 * attempt to render the children.
 *
 * This is the method that you should override to implement your custom
 * drawing code.  You are welcome to use any OpenGL commands that you wish.
 * You can even skip use of the SpriteBatch.  However, if you do so, you
 * must flush the SpriteBatch by calling end() at the start of the method.
 * in addition, you should remember to call begin() at the start of the
 * method.
 *
 * This method provides the correct transformation matrix and tint color.
 * You do not need to worry about whether the node uses relative color.
 * This method is called by render() and these values are guaranteed to be
 * correct.  In addition, this method does not need to check for visibility,
 * as it is guaranteed to only be called when the node is visible.
 *
 * @param batch     The SpriteBatch to draw with.
 * @param matrix    The global transformation matrix.
 * @param tint      The tint to blend with the Node color.
 */
void Label::draw(const std::shared_ptr<SpriteBatch>& batch, const Affine2& transform, Color4 tint) {
    if (!_rendered) {
        generateRenderData();
    }

    batch->setBlendEquation(_blendEquation);
    batch->setSrcBlendFunc(_srcFactor);
    batch->setDstBlendFunc(_dstFactor);
    if (_background != Color4::CLEAR) {
        batch->setTexture(Texture::getBlank());
        batch->setColor(tint*getBackground());
        batch->fill(_bounds,Vec2::ANCHOR_CENTER, transform);
    }
    if (_dropShadow) {
        batch->setBlur(_dropBlur);
        batch->setColor(tint*DROP_COLOR);
        Affine2 offset = Affine2::createTranslation(_dropOffset);
        offset *= transform;
        for(auto it = _glyphrun.begin(); it != _glyphrun.end(); ++it) {
            batch->setTexture(it->second->texture);
            batch->drawMesh(it->second->mesh, offset);
        }
        batch->setBlur(0);
    }
    batch->setColor(tint);
    for(auto it = _glyphrun.begin(); it != _glyphrun.end(); ++it) {
        batch->setTexture(it->second->texture);
        batch->drawMesh(it->second->mesh, transform);
    }
}

/**
 * Allocate the render data necessary to render this node.
 */
void Label::generateRenderData() {
    if (_rendered) {
        return;
    }
    
    // Make the backdrop
    _bounds = Rect(Vec2::ZERO,getContentSize());
    
    // Confine glyphs to label interior
    Rect legal = _bounds;
    legal.origin -= _offset;
    _layout->getGlyphs(_glyphrun,legal);
    for(auto it = _glyphrun.begin(); it != _glyphrun.end(); ++it) {
        for(auto jt = it->second->mesh.vertices.begin(); jt != it->second->mesh.vertices.end(); ++jt) {
            jt->position += _offset;
            jt->color = _foreground.getPacked();
        }
    }

    _rendered = true;
}

/**
 * Clears the render data, releasing all vertices and indices.
 */
void Label::clearRenderData() {
    _glyphrun.clear();
    _rendered = false;
}

/**
 * Updates the color value for any other data that needs it.
 *
 * This method is used to synchronize the background color and foreground
 * colors.
 */
void Label::updateColor() {
    if (!_rendered) {
        return;
    }
    
    for(auto jt = _glyphrun.begin(); jt != _glyphrun.end(); ++jt) {
        for(auto it = jt->second->mesh.vertices.begin(); it != jt->second->mesh.vertices.end(); ++it) {
            it->color = _foreground.getPacked();
        }
    }
}

/**
 * Resizes the content bounds to fit the text.
 */
void Label::resize() {
    Size size = _layout->getBounds().size;
    size.width  += _padleft+_padrght;
    size.height += _padbot+_padtop;
    SceneNode::setContentSize(size);
    _bounds = Rect(Vec2::ZERO,getContentSize());
}

void Label::reanchor() {
    clearRenderData();
    HorizontalAlign halign = _layout->getHorizontalAlignment();
    switch (halign) {
        case HorizontalAlign::LEFT:
        case HorizontalAlign::HARD_LEFT:
        case HorizontalAlign::JUSTIFY:
            _offset.x = _padleft;
            break;
        case HorizontalAlign::RIGHT:
        case HorizontalAlign::HARD_RIGHT:
            _offset.x = _contentSize.width-_padrght;
            break;
        case HorizontalAlign::CENTER:
        case HorizontalAlign::TRUE_CENTER:
            _offset.x = (_contentSize.width+(_padleft-_padrght))/2;
            break;
    }
    
    VerticalAlign valign = _layout->getVerticalAlignment();
    switch (valign) {
        case VerticalAlign::BASELINE:
            _offset.y = _padbot+(_layout->getLineCount()-1)*_layout->getSpacing()*_font->getPointSize();
            break;
        case VerticalAlign::BOTTOM:
        case VerticalAlign::HARD_BOTTOM:
            _offset.y = _padbot;
            break;
        case VerticalAlign::TOP:
        case VerticalAlign::HARD_TOP:
            _offset.y = _contentSize.height-_padtop;
            break;
        case VerticalAlign::MIDDLE:
        case VerticalAlign::TRUE_MIDDLE:
            _offset.y = (_contentSize.height+(_padbot-_padtop))/2;
            break;
    }
}
