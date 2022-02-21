//
//  CUTextLayout.h
//  Cornell University Game Library (CUGL)
//
//  This module is used to solve two problems. First, it allows us to finally
//  support multi-line text, which was missing in earlier versions of CUGL
//  (and has been one of the most requested features). More importantly, it
//  allows us to decouple text formatting from the Label class. Because of
//  this class we can draw text directly to a sprite batch without having to
//  use the scene graph API.
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
//  Version: 7/29/21
//
#include <cugl/render/CUTextLayout.h>
#include <cugl/render/CUFont.h>
#include <cugl/render/CUGlyphRun.h>
#include <cugl/util/CUStrings.h>

#define SHRINK 2

using namespace cugl;
using namespace cugl::strtool;

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
#pragma mark Rows
/**
 * Creates a new (empty) row with default values.
 */
TextLayout::Row::Row() :
paragraph(false),
begin(0),
end(0) {
}

/**
 * Deletes the contents of this row
 */
TextLayout::Row::~Row() {
    paragraph = false;
    begin = 0;
    end = 0;
}

#pragma mark -
#pragma mark Constructors
/**
 * Creates a degenerate text layout with no data.
 *
 * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
 * the heap, use one of the static constructors instead.
 */
TextLayout::TextLayout() :
_breakline(0),
_spacing(1),
_halign(HorizontalAlign::LEFT),
_valign(VerticalAlign::BASELINE) {
}

/**
 * Deletes the layout resources and resets all attributes.
 *
 * You must reinitialize the text layout to use it.
 */
void TextLayout::dispose() {
    _rows.clear();
    _text.clear();
    _font = nullptr;
    _breakline = 0;
    _spacing = 1;
}

/**
 * Initializes an empty text layout
 *
 * You will need to add both text and a font, as well as call the
 * method {@link #layout} to properly use this object.
 *
 * This layout will have a size of 0, which menas that this method
 * will only break lines at newlines. This guarantees multi-line
 * text support without taking control away from the programmer.
 *
 * @return true if initialization is successful.
 */
bool TextLayout::init() {
    _text = "";
    _font = nullptr;
    return true;
}

/**
 * Initializes an empty text layout with the given width.
 *
 * You will need to add both text and a font, as well as call the
 * method {@link #layout} to properly use this object.
 *
 * A width of 0 will guarantee that breaks only happen at newlines,
 * while a negative width means that line breaks never happen at all.
 * A postive width will force the layout manager to break up lines
 * so that no individual line exceeds that width.
 *
 * @param width The width for each line
 *
 * @return true if initialization is successful.
 */
bool TextLayout::initWithWidth(float width) {
    _text = "";
    _font = nullptr;
    _breakline = width;
    return true;
}

/**
 * Initializes a text layout with the given text and font.
 *
 * This layout will have a size of 0, which menas that this method
 * will only break lines at newlines. This guarantees multi-line
 * text support without taking control away from the programmer.
 *
 * Note that this method does not actually arrange the text. You
 * must call {@link #layout} to complete the layout.  This gives
 * you time to change the horizontal or vertical alignment before
 * performing the layout.
 *
 * @param text  The text to layout
 * @param font  The font to arrange the text
 *
 * @return true if initialization is successful.
 */
bool TextLayout::initWithText(const std::string text, const std::shared_ptr<Font>& font) {
    _text = text;
    std::string::iterator end_it = utf8::find_invalid(_text.begin(), _text.end());
    if (end_it == _text.end()) {
        _font = font;
        return true;
    }

    CUAssertLog(false,"String '%s' has an invalid UTF-8 encoding",text.c_str());
    return false;
}

/**
 * Initializes a text layout with the given text, font, and width.
 *
 * A width of 0 will guarantee that breaks only happen at newlines,
 * while a negative width means that line breaks never happen at all.
 * A postive width will force the layout manager to break up lines
 * so that no individual line exceeds that width.
 *
 * Note that this method does not actually arrange the text. You
 * must call {@link #layout} to complete the layout.  This gives
 * you time to change the horizontal or vertical alignment before
 * performing the layout.
 *
 * @param text  The text to layout
 * @param font  The font to arrange the text
 * @param width The width for each line
 *
 * @return true if initialization is successful.
 */
bool TextLayout::initWithTextWidth(const std::string text, const std::shared_ptr<Font>& font, float width) {
    _text = text;
    std::string::iterator end_it = utf8::find_invalid(_text.begin(), _text.end());
    if (end_it == _text.end()) {
        _font = font;
        _breakline = width;
        return true;
    }

    CUAssertLog(false,"String '%s' has an invalid UTF-8 encoding",text.c_str());
    return false;
}

#pragma mark -
#pragma mark Attributes
/**
 * Sets the text associated with this layout.
 *
 * Changing this value will {@link #invalidate} the layout.
 *
 * @param text  The text associated with this layout.
 */
void TextLayout::setText(const std::string text) {
    invalidate();
    _text = text;
    std::string::iterator end_it = utf8::find_invalid(_text.begin(), _text.end());
    CUAssertLog(end_it == _text.end(),"String '%s' has an invalid UTF-8 encoding",text.c_str());
}

/**
 * Sets the font associated with this layout.
 *
 * Changing this value will {@link #invalidate} the layout.
 *
 * @param font  The font associated with this layout.
 */
void TextLayout::setFont(const std::shared_ptr<Font>& font) {
    invalidate();
    _font = font;
}

/**
 * Sets the line width of this layout.
 *
 * This value will determine how the layout breaks up lines to arrange
 * text. A width of 0 will guarantee that breaks only happen at newlines,
 * while a negative width means that line breaks never happen at all. A
 * postive width will force the text layout to break up lines so that no
 * individual line exceeds that width.
 *
 * Changing this value will {@link #invalidate} the layout.
 *
 * @param width     The line width of this layout.
 */
void TextLayout::setWidth(float width) {
    invalidate();
    _breakline = width;
}

/**
 * Sets the line spacing of this layout.
 *
 * This value is multiplied by the font size to determine the space
 * between lines in the layout. So a value of 1 is single-spaced text,
 * while a value of 2 is double spaced. The value should be positive.
 *
 * Changing this value will {@link #invalidate} the layout.
 *
 * @param spacing   The line spacing of this layout.
 */
void TextLayout::setSpacing(float spacing) {
    invalidate();
    _spacing = spacing;
}


/**
 * Sets the horizontal alignment of the text.
 *
 * The horizontal alignment has two meanings. First, it is the relationship
 * of the relative alignment of multiple lines. In addition, it defines the
 * x-coordinate origin of the text layout. The later is relevant even when
 * the text layout is a single line.
 *
 * Changing this value will {@link #invalidate} the layout.
 *
 * @param halign    The horizontal alignment of the text.
 */
void TextLayout::setHorizontalAlignment(HorizontalAlign halign) {
    invalidate();
    _halign = halign;
}

/**
 * Sets the horizontal alignment of the text.
 *
 * The vertical defines the y-coordinate origin of this text layout. In the
 * case of multiple lines, the alignment is (often) with respect to the
 * entire block of text, not just the first line.
 *
 * Changing this value will {@link #invalidate} the layout.
 *
 * @param valign    The horizontal alignment of the text.
 */
void TextLayout::setVerticalAlignment(VerticalAlign valign) {
    invalidate();
    _valign = valign;
}

/**
 * Returns the tightest bounds of the text layout
 *
 * This rectangle is in the coordinate space whose origin is defined by
 * the horizontal and vertical alignment. This rectangle has zero size if
 * the {@link #layout} method has not been called or if the layout has been
 * invalidated.
 *
 * Unlike {@link #getBounds}, this rectangle sits tight against the text,
 * ignoring any natural spacing such as the ascent or descent. However, it
 * does not include any tracking that the layout may apply.
 *
 * @return the tightest bounds of the text layout
 */
const Rect TextLayout::getTightBounds() const {
    if (_rows.size() == 0) {
        return Rect::ZERO;
    } else if (_rows.size() == 1) {
        return _rows.back().interior;
    }
    
    Rect bounds = _rows.front().interior;
    for(auto it = _rows.begin()+1; it != _rows.end(); ++it) {
        bounds.merge(it->interior);
    }
    return bounds;
}

/**
 * Returns the true bounds of the text layout, including tracking
 *
 * This rectangle is in the coordinate space whose origin is defined by
 * the horizontal and vertical alignment. This rectangle has zero size if
 * the {@link #layout} method has not been called or if the layout has been
 * invalidated.
 *
 * The method is similar to {@link #getTightBounds}, except that it also
 * includes any tracking that is applied to the layout.
 *
 * @return the true bounds of the text layout, including tracking
 */
const Rect TextLayout::getTrueBounds() const {
    if (_rows.size() == 0) {
        return Rect::ZERO;
    } else if (_rows.size() == 1) {
        return _rows.back().interior;
    }
    
    Rect bounds;
    bool first = true;
    for(auto it = _rows.begin(); it != _rows.end(); ++it) {
        bool track = false;
        if (_breakline > 0) {
            if (_halign == HorizontalAlign::JUSTIFY) {
                auto jt = it+1;
                if (jt != _rows.end() && !jt->paragraph) {
                    track = true;
                }
            } else if (it->exterior.size.width > _breakline) {
                track = true;
            }
        }
        
        const char* begin = _text.c_str()+it->begin;
        const char* end   = _text.c_str()+it->end;
        Uint32 pcode = 0;
        Uint32 ccode = 0;
        Font::Metrics metrics;

        Rect line;
        std::vector<int> adjusts;
        if (track) {
            adjusts = _font->getTracking(begin,end,_breakline);
            adjusts.push_back(0);
        }
        
        bool start = true;
        size_t tpos = 0;
        while (begin != end) {
            ccode = utf8::next(begin, end);
            if (_font->hasGlyph(ccode)) {
                metrics = _font->getMetrics(ccode);
                if (start) {
                    line.set(metrics.minx,metrics.miny,metrics.maxx,metrics.advance);
                    start = false;
                } else {
                    line.size.width += metrics.advance;
                }
                if (pcode != 0) {
                    line.size.width -= _font->getKerning(pcode, ccode);
                    if (metrics.miny < line.origin.y) {
                        line.origin.y = metrics.miny;
                    }
                    if (metrics.maxy > line.size.height) {
                        line.size.height = metrics.maxy;
                    }
                }
                if (track) {
                    line.size.width += adjusts[tpos];
                }
            } else {
                ccode = 0;
            }
            pcode = ccode;
            tpos++;
        }
            
        // Pull off for last character and resize
        if (pcode != 0) {
            metrics = _font->getMetrics(pcode);
            line.size.width -= (metrics.advance-metrics.maxx);
        }
        
        line.size.width  -= line.origin.x;
        line.size.height -= line.origin.y;
        
        // Now adjust it using information from the exterior bounds
        line.origin.x += it->exterior.origin.x;
        line.origin.y += (it->exterior.origin.y-_font->getDescent());
        
        if (first) {
            bounds = line;
            first = false;
        } else {
            bounds.merge(line);
        }
    }

    return bounds;
}

#pragma mark -
#pragma mark Glyph Querying
/**
 * Returns the glyph bounds of the character at the given index
 *
 * The rectangle will be in the coordinate system of this text layout. In
 * addition to the size, it will accurately reflect the position of the
 * character in the layout, including any possible tracking.
 *
 * The index represents a position in the layout text string. The index
 * must be the first byte of a valid UTF8 character. If it is a successive
 * byte (and hence undecodable as a unicode character), this method will
 * return the empty rectangle.
 *
 * @param index The position in the layout text
 *
 * @return the glyph bounds of the character at the given index
 */
Rect TextLayout::getGlyphBounds(size_t index) const {
    if (_rows.size() == 0) {
        return Rect::ZERO;
    }
    
    const Row* row = nullptr;
    bool track = false;
    for(auto it = _rows.begin(); it != _rows.end(); ) {
        if (index <= it->end) {
            track = false;
            if (_breakline > 0) {
                if (_halign == HorizontalAlign::JUSTIFY) {
                    auto jt = it+1;
                    if (jt != _rows.end() && !jt->paragraph) {
                        track = true;
                    }
                } else if (it->exterior.size.width > _breakline) {
                    track = true;
                }
            }
            row = &(*it);
            it = _rows.end();
        } else {
            ++it;
        }
    }
    
    if (row == nullptr || index < row->begin) {
        return Rect::ZERO;
    }
    unsigned char value = _text[index];
    if (value > 127 && value < 192) {
        CUAssertLog(false,"Position %zu is not a valid unicode offset.",index);
        return Rect::ZERO;
    }

    const char* cursr = _text.c_str()+index;
    const char* begin = cursr;
    const char* end   = _text.c_str()+row->end;
    Uint32 pcode = 0;
    Uint32 ccode = utf8::next(begin,end);
    if (!_font->hasGlyph(ccode)) {
        return Rect::ZERO;
    }
    Font::Metrics metrics = _font->getMetrics(ccode);
    Rect bounds(metrics.minx,metrics.miny,metrics.maxx-metrics.minx,metrics.maxy-metrics.miny);
    
    begin = _text.c_str()+row->begin;
    std::vector<int> adjusts;
    if (track) {
        adjusts = _font->getTracking(begin, end, _breakline);
        adjusts.push_back(0);
    }
    
    float width = 0;
    size_t tpos = 0;
    while (begin != cursr) {
        ccode = utf8::next(begin,cursr);
        if (_font->hasGlyph(ccode)) {
            metrics = _font->getMetrics(ccode);
            width += metrics.advance;
            if (pcode != 0) {
                width -= _font->getKerning(pcode, ccode);
            }
            if (track) {
                width += adjusts[tpos];
            }
        } else {
            ccode = 0;
        }
        pcode = ccode;
        tpos++;
    }
    
    bounds.origin.x += width+row->exterior.origin.x;
    bounds.origin.y += (row->exterior.origin.y-_font->getDescent());
    return bounds;
}

/**
 * Returns the index of the character whose glyph is located at (x,y)
 *
 * If the point (x,y) is not on top of a glyph, this method will return
 * the size of the text. Use {@link #getNearestIndex} for cases in which
 * the point is out of bounds.
 *
 * The point (x,y) is assumed to be in the coordinate system of this
 * layout. This method will never return the index of white space
 * "swallowed" at the end of multiline text, even when this point is
 * beyond the edges of the text.
 *
 * @param x     The x-coordinate in layout space
 * @param y     The y-coordinate in layout space
 *
 * @return the index of the character whose glyph is located at (x,y)
 */
size_t TextLayout::getGlyphIndex(float x, float y) const {
    if (_rows.size() == 0) {
        return _text.size();
    }
    
    const Row* row = nullptr;
    bool track = false;
    float interspace = (_spacing > 1) ? (_spacing-1)*_font->getHeight() : 0;
    interspace /= 2;
    for(auto it = _rows.begin(); it != _rows.end(); ) {
        if (y >= it->exterior.origin.y-interspace) {
            track = false;
            if (_breakline > 0) {
                if (_halign == HorizontalAlign::JUSTIFY) {
                    auto jt = it+1;
                    if (jt != _rows.end() && !jt->paragraph) {
                        track = true;
                    }
                } else if (it->exterior.size.width > _breakline) {
                    track = true;
                }
            }
            row = &(*it);
            it = _rows.end();
        } else {
            ++it;
        }
    }
    
    // Last line if no hit
    if (row == nullptr) {
        row = &(_rows.back());
        track = (_breakline > 0 && row->exterior.size.width > _breakline);
    }
    
    if (row->begin == row->end || !row->exterior.contains(Vec2(x,y))) {
        return _text.size();
    }
    
    x -= row->exterior.origin.x;
    const char* begin = _text.c_str()+row->begin;
    const char* end   = _text.c_str()+row->end;
    std::vector<int> adjusts;
    if (track) {
        adjusts = _font->getTracking(begin, end, _breakline);
        adjusts.push_back(0);
    }
    
    float width = 0;
    Uint32 pcode = 0;
    Uint32 ccode = 0;
    size_t index = 0;
    while (begin != end) {
        ccode = utf8::next(begin,end);
        float advance = 0;
        if (_font->hasGlyph(ccode)) {
            advance = _font->getMetrics(ccode).advance;
            if (pcode != 0) {
                advance -= _font->getKerning(pcode, ccode);
            }
            if (track) {
                advance += adjusts[index];
            }
        } else {
            ccode = 0;
        }
        if (width+advance >= x) {
            return width <= x ? row->begin+index : _text.size();
        } else {
            width += advance;
            pcode = ccode;
            index++;
        }
    }
    
    return _text.size();
}
    
/**
 * Returns the index of the character whose glyph is nearest (x,y)
 *
 * The point (x,y) is assumed to be in the coordinate system of this
 * layout. This method will never return the index of white space
 * "swallowed" at the end of multiline text, even when this point is
 * beyond the edges of the text.
 *
 * @param x     The x-coordinate in layout space
 * @param y     The y-coordinate in layout space
 *
 * @return the index of the character whose glyph is nearest (x,y)
 */
size_t TextLayout::getNearestIndex(float x, float y) const {
    if (_rows.size() == 0) {
        return _text.size();
    }
    
    const Row* row = nullptr;
    bool track = false;
    float interspace = (_spacing > 1) ? (_spacing-1)*_font->getHeight() : 0;
    interspace /= 2;
    for(auto it = _rows.begin(); it != _rows.end(); ) {
        if (y >= it->exterior.origin.y-interspace) {
            track = false;
            if (_breakline > 0) {
                if (_halign == HorizontalAlign::JUSTIFY) {
                    auto jt = it+1;
                    if (jt != _rows.end() && !jt->paragraph) {
                        track = true;
                    }
                } else if (it->exterior.size.width > _breakline) {
                    track = true;
                }
            }
            row = &(*it);
            it = _rows.end();
        } else {
            ++it;
        }
    }
    
    // Last line if no hit
    if (row == nullptr) {
        row = &(_rows.back());
        track = (_breakline > 0 && row->exterior.size.width > _breakline);
    }
    
    if (row->begin == row->end) {
        return row->begin;
    }
    
    x -= row->exterior.origin.x;
    const char* begin = _text.c_str()+row->begin;
    const char* end   = _text.c_str()+row->end;
    std::vector<int> adjusts;
    if (track) {
        adjusts = _font->getTracking(begin, end, _breakline);
        adjusts.push_back(0);
    }
    
    float width = 0;
    Uint32 pcode = 0;
    Uint32 ccode = 0;
    size_t index = 0;
    bool done = false;
    while (!done && begin != end) {
        ccode = utf8::next(begin,end);
        if (_font->hasGlyph(ccode)) {
            width += _font->getMetrics(ccode).advance;
            if (pcode != 0) {
                width -= _font->getKerning(pcode, ccode);
            }
            if (track) {
                width += adjusts[index];
            }
        } else {
            ccode = 0;
        }
        if (width >= x) {
            done = true;
        } else {
            pcode = ccode;
            index++;
        }
    }
    if (!done) {
        begin = _text.c_str()+row->begin;
        while (!done && begin != end) {
            if (index > 0) { index--; }
            ccode = utf8::prior(end, begin);
            if (_font->hasGlyph(ccode)) {
                done = true;
            }
        }
    }
    return row->begin+index;
}

#pragma mark -
#pragma mark Glyph Generation
/**
 * Returns a set of glyph runs to render the text layout
 *
 * Each glyph run will consist of a quad mesh and a texture to render
 * those quads. Rendering all of the glyph runs together will render
 * the entire string.  Generally the quads are non-overlapping, so
 * any blending mode is supported. However, if the atlas padding is
 * non-zero (to support font blur), the quads will overlap at the
 * padding intervals.  Therefore, we recommend alpha blending when
 * you render a string.
 *
 * The keys for the glyph runs are the {@link Texture#getBuffer} values
 * for the appropriate atlas texture. This, combined with the method
 * {@link Font#getAtlases} allows you to identify the atlas for each run.
 *
 * The origin of the glyph runs will agree with that of the text layout.
 * This method will return the empty map if {@link #layout} has not been
 * called or the layout has been invalidated.
 *
 * @return a set of glyph runs to render the text layout
 */
std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> TextLayout::getGlyphs() const {
    std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> result;
    getGlyphs(result);
    return result;
}

/**
 * Returns a set of glyph runs to render the text layout
 *
 * Each glyph run will consist of a quad mesh and a texture to render
 * those quads. Rendering all of the glyph runs together will render
 * the entire string.  Generally the quads are non-overlapping, so
 * any blending mode is supported. However, if the atlas padding is
 * non-zero (to support font blur), the quads will overlap at the
 * padding intervals.  Therefore, we recommend alpha blending when
 * you render a string.
 *
 * The keys for the glyph runs are the {@link Texture#getBuffer} values
 * for the appropriate atlas texture. This, combined with the method
 * {@link Font#getAtlases} allows you to identify the atlas for each run.
 *
 * The quad sequence is adjusted so that all of the vertices fit in the
 * provided rectangle. The primary use-case for this is to guarantee
 * that glyphs do not spill outside of a window. This may mean that some
 * of the glyphs will be truncrated or even omitted.
 *
 * The origin of the glyph runs will agree with that of the text layout.
 * This method will return the empty map if {@link #layout} has not been
 * called or the layout has been invalidated.
 *
 * @param rect      The bounding box for the quads
 *
 * @return a set of glyph runs to render the text layout
 */
std::unordered_map<GLuint,std::shared_ptr<GlyphRun>> TextLayout::getGlyphs(Rect rect) const {
    std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> result;
    getGlyphs(result,rect);
    return result;
}

/**
 * Stores the glyph runs to render the text layout in the given map
 *
 * Each glyph run will consist of a quad mesh and a texture to render
 * those quads. Rendering all of the glyph runs together will render
 * the entire string.  Generally the quads are non-overlapping, so
 * any blending mode is supported. However, if the atlas padding is
 * non-zero (to support font blur), the quads will overlap at the
 * padding intervals.  Therefore, we recommend alpha blending when
 * you render a string.
 *
 * The keys for the glyph runs are the {@link Texture#getBuffer} values
 * for the appropriate atlas texture. This, combined with the method
 * {@link #getAtlases} allows you to identify the atlas for each run.
 * If the map is non-empty, the glyph run data will be appended to the
 * relevant existing glyph run (if possible).
 *
 * The origin of the glyph runs will agree with that of the text layout.
 * This method will do nothing if {@link #layout} has not been called or
 * the layout has been invalidated.
 *
 * @param runs      The map to store the glyph runs
 *
 * @return the number of glyphs successfully processed
 */
size_t TextLayout::getGlyphs(std::unordered_map<GLuint,std::shared_ptr<GlyphRun>>& runs) const {
    Rect bounds = getBounds();
    size_t total = 0;
    for(auto it = _rows.begin(); it != _rows.end(); ++it) {
        float track = 0;
        if (_halign == HorizontalAlign::JUSTIFY) {
            auto jt = it+1;
            if (jt != _rows.end() && !jt->paragraph) {
                track = _breakline;
            }
        } else if (_breakline > 0 && it->exterior.size.width > _breakline) {
            track = _breakline;
        }
        const char* begin = _text.c_str()+it->begin;
        const char* end = _text.c_str()+it->end;
        total += _font->getGlyphs(runs, begin, end, it->exterior.origin, bounds, track);
    }
    return total;
}

/**
 * Stores the glyph runs to render the text layout in the given map
 *
 * Each glyph run will consist of a quad mesh and a texture to render
 * those quads. Rendering all of the glyph runs together will render
 * the entire string.  Generally the quads are non-overlapping, so
 * any blending mode is supported. However, if the atlas padding is
 * non-zero (to support font blur), the quads will overlap at the
 * padding intervals.  Therefore, we recommend alpha blending when
 * you render a string.
 *
 * The keys for the glyph runs are the {@link Texture#getBuffer} values
 * for the appropriate atlas texture. This, combined with the method
 * {@link #getAtlases} allows you to identify the atlas for each run.
 * If the map is non-empty, the glyph run data will be appended to the
 * relevant existing glyph run (if possible).
 *
 * The quad sequence is adjusted so that all of the vertices fit in the
 * provided rectangle. The primary use-case for this is to guarantee
 * that glyphs do not spill outside of a window. This may mean that some
 * of the glyphs will be truncrated or even omitted.
 *
 * The origin of the glyph runs will agree with that of the text layout.
 * This method will do nothing if {@link #layout} has not been called or
 * the layout has been invalidated.
 *
 * @param runs      The map to store the glyph runs
 * @param rect      The bounding box for the quads
 *
 * @return the number of glyphs successfully processed
 */
size_t TextLayout::getGlyphs(std::unordered_map<GLuint,std::shared_ptr<GlyphRun>>& runs, Rect rect) const {
    Rect bounds = getBounds();
    bounds.intersect(rect);
    size_t total = 0;
    for(auto it = _rows.begin(); it != _rows.end(); ++it) {
        float track = 0;
        if (_halign == HorizontalAlign::JUSTIFY) {
            auto jt = it+1;
            if (jt != _rows.end() && !jt->paragraph) {
                track = _breakline;
            }
        } else if (_breakline > 0 && it->exterior.size.width > _breakline) {
            track = _breakline;
        }
        const char* begin = _text.c_str()+it->begin;
        const char* end = _text.c_str()+it->end;
        total += _font->getGlyphs(runs, begin, end, it->exterior.origin, bounds, track);
    }
    return total;
}


#pragma mark -
#pragma mark Layout Processing
/**
 * Arranges the text according to the given font and settings
 *
 * Changing any of the layout attributes will obviously invalidate the text
 * layout. For performance reasons, we do not automatically recompute the
 * layout in that case. Instead, the user must call {@link #layout} to
 * arrange the text.
 */
void TextLayout::layout() {
    if (_rows.size() > 0) {
        return;
    } else if (_font == nullptr) {
        _rows.push_back(Row());
        Row* row = &(_rows.back());
        row->begin = 0;
        row->end = _text.size();
        return;
    }
    
    if (_breakline >= 0) {
        breakLines();
    } else {
        // Will only have one line
        _rows.push_back(Row());
        Row* row = &(_rows.back());
        row->begin = 0;
        row->end = _text.size();
        resizeRow(0);
    }
    resetHorizontal();
    resetVertical();
    computeBounds();
}

/**
 * Invalidates the text layout
 *
 * This deletes all rows (so the line count is 0). You will need to call
 * {@link #layout} to reperform the layout.
 */
void TextLayout::invalidate() {
    _rows.clear();
    _bounds.set(0,0,0,0);
}

/**
 * Returns the text for the given line.
 *
 * Note that line breaking will omit any white space on the ends. Hence
 * adding the text for each line together may not produce the original
 * text.
 *
 * @param line    The line number
 *
 * @return the text for the given line.
 */
std::string TextLayout::getLine(size_t line) const {
    std::string result;
    const Row* row = &(_rows.at(line));
    result.reserve(row->end-row->begin+1);
    const char* begin = _text.c_str()+row->begin;
    const char* end = _text.c_str()+row->end;
    while (begin != end) {
        result.push_back(*begin++);
    }
    result.push_back('\0');
    return result;
}

/**
 * Resets the horizontal alignment.
 *
 * This method recomputes the horizontal position of each line.
 */
void TextLayout::resetHorizontal() {
    if (_rows.empty()) {
        return;
    }
    
    // Reset everything to normal left
    for(auto it = _rows.begin(); it != _rows.end(); ++it) {
        it->interior.origin.x -= it->exterior.origin.x;
        it->exterior.origin.x = 0;
    }
    
    switch (_halign) {
        case HorizontalAlign::LEFT:
        case HorizontalAlign::JUSTIFY:
            // Do nothing
            return;
        case HorizontalAlign::RIGHT:
            if (_breakline > 0) {
                for(auto it = _rows.begin(); it != _rows.end(); ++it) {
                    it->exterior.origin.x = -std::min(it->exterior.size.width,_breakline);
                    it->interior.origin.x += it->exterior.origin.x;
                }
            } else if (!_rows.empty()) {
                Row* row = &(_rows.back());
                row->exterior.origin.x = -row->exterior.size.width;
                row->interior.origin.x += row->exterior.origin.x;
            }
            break;
        case HorizontalAlign::CENTER:
            if (_breakline > 0) {
                for(auto it = _rows.begin(); it != _rows.end(); ++it) {
                    it->exterior.origin.x = -std::min(it->exterior.size.width,_breakline)/2;
                    it->interior.origin.x += it->exterior.origin.x;
                }
            } else if (!_rows.empty()) {
                Row* row = &(_rows.back());
                row->exterior.origin.x = -row->exterior.size.width/2;
                row->interior.origin.x += row->exterior.origin.x;
            }
            break;
        case HorizontalAlign::HARD_LEFT:
            if (_breakline > 0) {
                for(auto it = _rows.begin(); it != _rows.end(); ++it) {
                    it->exterior.origin.x = -it->interior.origin.x;
                    it->interior.origin.x = 0;
                }
            } else if (!_rows.empty()) {
                Row* row = &(_rows.back());
                row->exterior.origin.x = -row->interior.origin.x;
                row->interior.origin.x = 0;
            }
            break;
        case HorizontalAlign::HARD_RIGHT:
            if (_breakline > 0) {
                for(auto it = _rows.begin(); it != _rows.end(); ++it) {
                    it->exterior.origin.x = -std::min(it->interior.size.width,_breakline);
                    it->interior.origin.x += it->exterior.origin.x;
                }
            } else if (!_rows.empty()) {
                Row* row = &(_rows.back());
                row->exterior.origin.x = -row->interior.size.width;
                row->interior.origin.x += row->exterior.origin.x;
            }
            break;
        case HorizontalAlign::TRUE_CENTER:
            if (_breakline > 0) {
                for(auto it = _rows.begin(); it != _rows.end(); ++it) {
                    it->exterior.origin.x = -std::min(it->interior.size.width,_breakline)/2;
                    it->interior.origin.x += it->exterior.origin.x;
                }
            } else if (!_rows.empty()) {
                Row* row = &(_rows.back());
                row->exterior.origin.x = -row->interior.size.width/2;
                row->interior.origin.x += row->exterior.origin.x;
            }
            break;
    }
}

/**
 * Resets the vertical alignment.
 *
 * This method recomputes the vertical position of each line.
 */
void TextLayout::resetVertical() {
    if (_rows.empty()) {
        return;
    }
    
    // Reset everything to baseline (with spacing applied)
    Row* first = &(_rows.front());
    first->interior.origin.y -= first->exterior.origin.y-_font->getDescent();
    first->exterior.origin.y = _font->getDescent();

    for(int ii = 1; ii < _rows.size(); ii++) {
        float offset = _spacing*_font->getHeight()*ii;

        Row* row = &(_rows[ii]);
        row->interior.origin.y -= row->exterior.origin.y-_font->getDescent();
        row->exterior.origin.y = _font->getDescent();

        // Going DOWN
        row->exterior.origin.y -= offset;
        row->interior.origin.y -= offset;
    }
    
    // For the internal offsets
    switch (_valign) {
        case VerticalAlign::BASELINE:
            // Do nothing
            return;
        case VerticalAlign::BOTTOM:
        {
            float off = _rows.back().exterior.origin.y;
            for(auto it = _rows.begin(); it != _rows.end(); ++it) {
                it->exterior.origin.y -= off;
                it->interior.origin.y -= off;
            }
        }
            break;
        case VerticalAlign::MIDDLE:
        {
            float top = (first->exterior.size.height+first->exterior.origin.y);
            float bot = _rows.back().exterior.origin.y;
            float off = (bot+top)/2;
            for(auto it = _rows.begin(); it != _rows.end(); ++it) {
                it->exterior.origin.y -= off;
                it->interior.origin.y -= off;
            }
        }
            break;
        case VerticalAlign::TOP:
        {
            Rect* top = &(first->exterior);
            float off = top->origin.y+top->size.height;
            for(auto it = _rows.begin(); it != _rows.end(); ++it) {
                it->exterior.origin.y -= off;
                it->interior.origin.y -= off;
            }
        }
            break;
        case VerticalAlign::HARD_BOTTOM:
        {
            float bot = _rows.back().interior.origin.y;
            for(auto it = _rows.begin(); it != _rows.end(); ++it) {
                it->interior.origin.y -= bot;
                it->exterior.origin.y -= bot;
            }
        }
            break;
        case VerticalAlign::HARD_TOP:
        {
            Rect* top = &(first->interior);
            float off = top->origin.y+top->size.height;
            for(auto it = _rows.begin(); it != _rows.end(); ++it) {
                it->exterior.origin.y -= off;
                it->interior.origin.y -= off;
            }
        }
            break;
        case VerticalAlign::TRUE_MIDDLE:
        {
            float top = (first->interior.size.height+first->interior.origin.y);
            float bot = _rows.back().interior.origin.y;
            float off = (bot+top)/2;
            for(auto it = _rows.begin(); it != _rows.end(); ++it) {
                it->exterior.origin.y -= off;
                it->interior.origin.y -= off;
            }
        }
            break;
    }
}

/**
 * Recomputes the bounding box of this text layout
 */
void TextLayout::computeBounds() {
    if (_rows.empty()) {
        _bounds = Rect::ZERO;
    }
    
    _bounds = _rows[0].exterior;
    for(auto it = _rows.begin()+1; it != _rows.end(); ++it) {
        _bounds.merge(it->exterior);
    }
    
    if (_breakline > 0) {
        // In case anything was stretched
        if (_halign == HorizontalAlign::JUSTIFY || _bounds.size.width > _breakline) {
            _bounds.size.width = _breakline;
        }
    }
}

/**
 * Breaks up the text into multiple lines.
 *
 * When breaking up lines, whitespace at the beginning and end of each
 * line will be "swallowed", causing it to be ignored for purposes of
 * alignment. The exception is at the beginning and end of a paragraph.
 * Whitespace there will be preserved. A paragraph is defined as any
 * piece of text separated by a newline. So the first part of the string
 * before a newline is a paragraph, and each substring after a newline
 * is also a paragraph.
 *
 * This algorithm in this method is heavily inspired by nanovg by Mikko
 * Mononen (memon@inside.org). However, this version includes many
 * optimizations as well as the paragraph-specific behavior (which is
 * more natural for editable text).
 *
 * This method will not be called if the width is negative.
 */
void TextLayout::breakLines() {
    // First thing we do is to break into lines
    _rows.push_back(Row());
    Row* row = &(_rows.back());
    row->begin = 0;
    row->end = 0;
    row->paragraph = true;
    row->exterior.origin.y = _font->getDescent();
    row->exterior.size.height = _font->getAscent()-_font->getDescent();
    float wordMinX  = 0;
    float wordMaxX  = 0;
    float wordMinY  = 0;
    float wordMaxY  = 0;
    float wordLeft  = 0;
    float wordRight = 0;
    float lineWidth = 0;
    
    const char* rowBegin  = nullptr;
    const char* wordBegin = nullptr;
    const char* wordEnd   = nullptr;
    const char* next = _text.c_str();
    const char* curr = next;
    const char* textEnd = curr+_text.size();
    
    Uint32 pcode = 0;
    bool rowStart = true;
    UnicodeType ptype = UnicodeType::SPACE;

    while (curr != textEnd) {
        Uint32 code = utf8::next(next, textEnd);
        UnicodeType type = classify(code,pcode);

        // ALWAYS break at newlines
        if (type == UnicodeType::NEWLINE) {
            // We include everything (even spaces) to a newline
            row->end = curr-_text.c_str();
            if (rowBegin != nullptr) {
                row->begin = rowBegin-_text.c_str();
            }
            row->exterior.size.width = lineWidth;
            row->interior.size.width = (wordMaxX == 0) ? 0 : wordMaxX-row->interior.origin.x;
            if (wordMaxY > row->interior.size.height) {
                row->interior.size.height = wordMaxY;
            }
            if (wordMinY < row->interior.origin.y) {
                row->interior.origin.y = wordMinY;
            }
            row->interior.size.height -= row->interior.origin.y;


            // Set up a new row (and paragraph!)
            size_t last = row->end;
            rowStart = true;
            _rows.push_back(Row());
            row = &(_rows.back());
            row->begin = last;
            row->end = last;
            row->paragraph = true;
            row->exterior.origin.y = _font->getDescent();
            row->exterior.size.height = _font->getAscent()-_font->getDescent();
            rowBegin  = nullptr;
            wordBegin = nullptr;
            wordEnd = nullptr;
            lineWidth = 0;
            wordMinX  = wordMaxX  = 0;
            wordMinY  = wordMaxX  = 0;
            wordLeft  = wordRight = 0;
            pcode = code;
            ptype = type;
        } else if (_font->hasGlyph(code)) {
            Font::Metrics metrics = _font->getMetrics(code);
            if (rowStart) {
                // Skip white space until the beginning of the line
                if (row->paragraph || (type != UnicodeType::SPACE && type != UnicodeType::NEWLINE)) {
                    rowStart = false;
                    
                    // Initialize the row
                    rowBegin = curr;
                    row->begin = curr-_text.c_str();
                    row->end = row->begin;
                    row->exterior.origin.x = 0;
                    row->interior.origin.x = metrics.minx;
                    row->exterior.origin.y = _font->getDescent();
                    row->exterior.size.height = _font->getAscent()-_font->getDescent();
                    
                    // Start tracking this word
                    wordBegin = curr;
                    wordEnd = next;
                    
                    // No kerning for first character
                    wordMinX  = metrics.minx; wordMaxX  = metrics.maxx;
                    wordMinY  = metrics.miny; wordMaxY  = metrics.maxy;
                    wordLeft  = 0;            wordRight = metrics.advance;
                    lineWidth = metrics.advance;
                }
            } else {
                // Compute the next width
                float kerning   = _font->getKerning(pcode, code);
                float theWidth  = metrics.advance - kerning;
                float nextWidth = lineWidth+theWidth;
                
                // Break to new line when a character is beyond break width.
                if (_breakline > 0 && (type == UnicodeType::CHAR || type == UnicodeType::CJK) && nextWidth > _breakline) {
                    // The current word is longer than the row length, just break it from here.
                    if (wordBegin == rowBegin) {
                        row->end = curr-_text.c_str();
                        row->interior.size.width = wordMaxX-row->interior.origin.x;
                        row->exterior.size.width = lineWidth;
                        if (wordMaxY > row->interior.size.height) {
                            row->interior.size.height = wordMaxY;
                        }
                        if (wordMinY < row->interior.origin.y) {
                            row->interior.origin.y = wordMinY;
                        }
                        row->interior.size.height -= row->interior.origin.y;

                        // Set up a new row
                        rowStart = false;
                        _rows.push_back(Row());
                        row = &(_rows.back());
                        rowBegin = curr;
                        row->begin = curr-_text.c_str();
                        row->end = row->begin;
                        row->exterior.origin.x = 0;
                        row->exterior.origin.y = _font->getDescent();
                        row->exterior.size.height = _font->getAscent()-_font->getDescent();
                        row->interior.origin.x = metrics.minx;
                        lineWidth = 0;
                        
                        // No kerning for first character
                        wordBegin = curr;
                        wordEnd   = next;
                        wordMinX  = metrics.minx; wordMaxX  = metrics.maxx;
                        wordMinY  = metrics.miny; wordMaxY  = metrics.maxy;
                        wordLeft  = 0;            wordRight = metrics.advance;
                        nextWidth = wordRight;
                    } else {
                        // Check if we can squeeze this one in
                        bool squeeze = *next == '\0';
                        const char* check = next;
                        if (!squeeze) {
                            Uint32 ncode = utf8::next(check, textEnd);
                            UnicodeType ntype = classify(ncode,code);
                            if (ntype == UnicodeType::SPACE || ntype == UnicodeType::NEWLINE) {
                                float value = nextWidth/(_breakline-1);
                                squeeze = value <= _font->getShrinkLimit();
                            }
                        }
                        
                        if (squeeze) {
                            row->end = next-_text.c_str();
                            if (wordMaxY > row->interior.size.height) {
                                row->interior.size.height = wordMaxY;
                            }
                            if (metrics.maxy > row->interior.size.height) {
                                row->interior.size.height = metrics.maxy;
                            }
                            if (wordMinY < row->interior.origin.y) {
                                row->interior.origin.y = wordMinY;
                            }
                            if (metrics.miny < row->interior.origin.y) {
                                row->interior.origin.y = metrics.miny;
                            }
                            row->exterior.size.width = nextWidth;
                            row->interior.size.width = nextWidth-(metrics.advance-metrics.maxx+kerning);
                            wordBegin = nullptr;
                            wordEnd = nullptr;
                        } else {
                            row->interior.size.height -= row->interior.origin.y;
                        }
                        
                        
                        // Break the line from the end of the last word.
                        // Start new line from the beginning of the new.
                        rowStart = false;
                        _rows.push_back(Row());
                        row = &(_rows.back());
                        
                        rowBegin = nullptr;
                        row->exterior.origin.x = 0;
                        row->exterior.origin.y = _font->getDescent();
                        row->exterior.size.height = _font->getAscent()-_font->getDescent();
                        if (wordBegin != nullptr) {
                            rowBegin = wordBegin;
                            row->begin = rowBegin-_text.c_str();
                            row->end = row->begin;
                            row->interior.origin.x = wordMinX-wordLeft;
                            lineWidth = wordRight-wordLeft;
                            nextWidth = lineWidth+theWidth;
                            wordEnd = next;
                            wordMaxX = lineWidth+metrics.maxx-kerning;
                            wordRight = nextWidth;
                            if (metrics.miny < wordMinY) {
                                wordMinY = metrics.miny;
                            }
                            if (metrics.maxy > wordMaxY) {
                                wordMaxY = metrics.maxy;
                            }
                        } else if (!squeeze) {
                            rowBegin = curr;
                            row->begin = rowBegin-_text.c_str();
                            row->end = row->begin;

                            wordMinX = metrics.minx; wordMaxX = metrics.maxx;
                            wordMinY = metrics.miny; wordMaxY = metrics.maxy;
                            wordLeft = 0;
                            wordRight = metrics.advance;
                            
                            row->interior.origin.x = wordMinX;
                            wordBegin = curr;
                            wordEnd = next;
                            lineWidth = wordLeft;
                            nextWidth = wordRight;
                        } else {
                            rowBegin = next;
                            row->begin = rowBegin-_text.c_str();
                            row->end = row->begin;

                            wordMinX = wordMaxX = 0;
                            wordMinY = wordMaxY = 0;
                            wordLeft = wordRight = 0;
                            rowStart = true;
                            lineWidth = 0;
                            nextWidth = 0;
                        }
                    }
                } else if ((ptype == UnicodeType::CHAR && type == UnicodeType::SPACE) || ptype == UnicodeType::CJK) {
                    row->end = wordEnd-_text.c_str();
                    if (wordMaxY > row->interior.size.height) {
                        row->interior.size.height = wordMaxY;
                    }
                    if (wordMinY < row->interior.origin.y) {
                        row->interior.origin.y = wordMinY;
                    }
                    row->interior.size.width = wordMaxX;
                    row->exterior.size.width = wordRight;
                    wordBegin = nullptr;
                    wordEnd = nullptr;
                    wordMinX  = wordMaxX  = 0;
                    wordMinY  = wordMaxY  = 0;
                    wordLeft  = wordRight = 0;
                } else if ((ptype == UnicodeType::SPACE && type == UnicodeType::CHAR) || ptype == UnicodeType::CJK) {
                    // track last beginning of a word
                    wordBegin = curr;
                    wordEnd = next;
                    wordMinX = lineWidth+metrics.minx-kerning;
                    wordMaxX = lineWidth+metrics.maxx-kerning;
                    wordMinY = metrics.miny;
                    wordMaxY = metrics.maxy;
                    wordLeft = lineWidth;
                    wordRight = nextWidth;
                } else if (type == UnicodeType::CHAR) {
                    wordEnd = next;
                    wordMaxX = lineWidth+metrics.maxx-kerning;
                    wordRight = nextWidth;
                    if (metrics.miny < wordMinY) {
                        wordMinY = metrics.miny;
                    }
                    if (metrics.maxy > wordMaxY) {
                        wordMaxY = metrics.maxy;
                    }
                }
                lineWidth = nextWidth;
            }
            pcode = code;
            ptype = type;
        }
        curr = next;
    }
    
    // We include everything (even spaces) after the last line
    row->end = curr-_text.c_str();
    if (rowBegin != nullptr) {
        row->begin = rowBegin-_text.c_str();
    }
    row->exterior.size.width = lineWidth;
    row->interior.size.width = (wordMaxX == 0) ? 0 : wordMaxX-row->interior.origin.x;
    if (wordMaxY > row->interior.size.height) {
        row->interior.size.height = wordMaxY;
    }
    if (wordMinY < row->interior.origin.y) {
        row->interior.origin.y = wordMinY;
    }
    row->interior.size.height -= row->interior.origin.y;
}

/**
 * Recomputes the size of the given row, indicating if it is overwidth.
 *
 * This method is useful for when insertions are made into the middle
 * of text. It will not break up the row, but will indicated if the
 * row should be broken up.
 *
 * Note that this method will adjust the rectangles to fit the row, but
 * it will not apply any horizontal or vertical alignment.
 *
 * @param row   The row to recompute
 *
 * @return true if the new size is overwidth
 */
bool TextLayout::resizeRow(size_t row) {
    Row* line = &(_rows[row]);
    if (line->begin == line->end) {
        line->exterior.set(0,_font->getDescent(),0,_font->getHeight());
        line->interior.set(0,0,0,0);
        return false;
    }
        
    const char* text = _text.c_str()+line->begin;
    const char* end  = _text.c_str()+line->end;
    Uint32 pcode = utf8::next(text,end);
    Font::Metrics metrics = _font->getMetrics(pcode);
    float minX = metrics.minx;
    float maxX = metrics.maxx;
    float minY = metrics.miny;
    float maxY = metrics.maxy;
    float width = metrics.advance;
    
    Uint32 ccode = 0;
    while (text != end) {
        ccode = utf8::next(text,end);
        float kerning = _font->getKerning(pcode, ccode);
        if (_font->hasGlyph(ccode)) {
            metrics = _font->getMetrics(ccode);
            maxX = width+metrics.maxx-kerning;
            width += metrics.advance-kerning;
            if (metrics.miny < minY) {
                minY = metrics.miny;
            }
            if (metrics.maxy > maxY) {
                maxY = metrics.maxy;
            }
        }
    }

    line->exterior.origin.set(0,_font->getDescent());
    line->exterior.size.set(width,_font->getAscent()-_font->getDescent());
    line->interior.origin.set(minX,minY);
    line->interior.size.set(maxX-minX,maxY-minY);
    
    size_t space = line->end-line->begin-1;
    return line->exterior.size.width < _breakline+space*_font->getShrinkLimit();
}

/**
 * Returns true if this row applies tracking.
 *
 * Tracking is applied if the text is multiline and either justified or
 * squeezed to fit within a line.
 *
 * @param row   The row to check
 *
 * @return true if this row applies tracking.
 */
bool TextLayout::doesTrack(size_t row) const {
    if (_breakline <= 0) {
        return false;
    }
    
    float track = 0;
    const Row* curr = &(_rows[row]);
    if (_halign == HorizontalAlign::JUSTIFY) {
        const Row* next = row < _rows.size()-1 ? &(_rows[row+1]) : nullptr;
        if (next != nullptr && !next->paragraph) {
            track = _breakline;
        }
    } else if (curr->exterior.size.width > _breakline) {
        track = _breakline;
    }
    return track > 0;
}
