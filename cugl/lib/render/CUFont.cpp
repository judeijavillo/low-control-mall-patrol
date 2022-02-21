//
//  CUFont.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a robust font asset with atlas support.  Unlike other
//  systems we decided to merge fonts and font atlases because it helps with
//  asset management.
//
//  This module makes heavy use of the cross-platform UTF8 utilities by
//  Nemanja Trifunovic ( https://github.com/nemtrif/utfcpp ).
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
//  Version: 3/1/21

#include <deque>
#include <algorithm>
#include <utf8/utf8.h>
#include <cugl/util/CUDebug.h>
#include <cugl/util/CUFiletools.h>
#include <cugl/util/CUStrings.h>
#include <cugl/render/CUTexture.h>
#include <cugl/render/CUFont.h>

using namespace cugl;

/** The amount of border to put around a glyph to prevent bleeding. */
#define GLYPH_BORDER    2

/** The maximum size of an individual atlas texture */
#define MAX_ATLAS_SIZE  512

/** The value of a tab character (becomes four spaces) */
#define TAB_CHAR        9
/** The value of an ASCII space character */
#define SPACE_CHAR      32
/** The number of spaces to a tab character */
#define TAB_SPACE       4

/**
 * Returns true if thechar is a Unicode control character
 *
 * Control characters 
 * @param thechar   The character to test
 *
 * @return true if thechar is a Unicode control character
 */
static bool is_control(Uint32 thechar) {
    return thechar == 0 || (thechar >= 9 && thechar <= 13) || (0x001c <= thechar && thechar <= 0x001f) || thechar == 0x085;
}

/**
 * Returns true if thechar is a non-visible character
 *
 * Non-visible characters include spaces, newlines, and control characters.
 *
 * @param thechar   The character to test
 *
 * @return true if thechar is a non-visible character
 */
static bool is_whitespace(Uint32 thechar) {
    return (thechar == 0 || (thechar >= 9 && thechar <= 13) || thechar == 32 || thechar == 0x00a0 ||
            (0x001c <= thechar && thechar <= 0x001f) || thechar == 0x085);
}


#pragma mark -
#pragma mark Atlas
/**
 * Creates an uninitialized atlas with no parent font.
 *
 * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
 * the heap, use one of the static constructors instead.
 */
Font::Atlas::Atlas() :
_parent(nullptr),
_surface(nullptr),
texture(nullptr) {
}

/**
 * Deletes the atlas resources and resets all attributes.
 *
 * This will delete the parent font as well. You must reinitialize the
 * atlas to use it.
 */
void Font::Atlas::dispose() {
	if (_surface != nullptr) {
		SDL_FreeSurface(_surface);
		_surface = nullptr;
	}
	_parent = nullptr;
	_size = Size::ZERO;
    texture = nullptr;
	glyphmap.clear();
}
        
/**
 * Initializes an atlas for the given font and glyphset
 *
 * This initializer will perform the layout computation, but it will
 * not create any textures or SDL surfaces. It will consume glyphs
 * from the provided glyphset as it adds them to the atlas. So if it
 * successfully adds all glyphs, the value glyphset will be emptied.
 *
 * It is possible for the atlas to reject some glyphs. This is
 * typically because the resulting texture size would exceed the
 * maximum allowable texture size. In that case, the remaining
 * elements in glyphset are glyphs that must be processed by another
 * atlas.
 *
 * If this atlas cannot process any of the elements in glyphset
 * (because they are unsupported), then this method returns false.
 *
 * @param parent    The parent font of this atlas
 * @param glyphset  The glyphs to add to this atlas
 *
 * @return true if the atlas was successfully initialized
 */
bool Font::Atlas::init(Font* parent, std::deque<Uint32>& glyphset)  {
    this->_parent = parent;
    layout(glyphset);
    return glyphmap.size() > 0;
}

/**
 * Returns true if this font has a glyph for the given (UNICODE) character.
 *
 * The Unicode representation uses the endianness native to the platform.
 * Therefore, this value should not be serialized.  Use UTF8 to represent
 * unicode in a platform-independent manner.
 *
 * Note that control characters (like newline) never have glyphs. However,
 * spaces do.
 *
 * @param a     The Unicode character to check.
 *
 * @return true if this font has a glyph for the given (UNICODE) character.
 */
bool Font::Atlas::hasGlyph(Uint32 a) const {
    return (a == TAB_CHAR && glyphmap.find(SPACE_CHAR) != glyphmap.end()) || (glyphmap.find(a) != glyphmap.end());
}

/**
 * Returns true if this atlas has all of the given glyphs
 *
 * We assume that the string represents the glyphs in a UTF8 encoding.
 *
 * @param glyphs    The UTF8 glyphs to check.
 *
 * @return true if this atlas has all of the given glyphs
 */
bool Font::Atlas::hasGlyphs(const std::string& glyphs) const {
    const char* begin = glyphs.c_str();
    const char* check = glyphs.c_str();
    const char* end   = begin+glyphs.size();
    CUAssertLog(utf8::find_invalid(check, end) == end, "String '%s' has an invalid UTF-8 encoding",begin);
    
    while (begin != end) {
        if (!hasGlyph(utf8::next(begin, end))) { return false; }
    }
    return true;
}

/**
 * Returns true if this atlas has all of the given unicode glyphs
 *
 * The Unicode representation uses the endianness native to the platform.
 * Therefore, this value should not be serialized.  Use UTF8 to represent
 * unicode in a platform-independent manner.
 *
 * Note that control characters (like newline) never have glyphs. However,
 * spaces do.
 *
 * @param glyphs    The Unicode characters to check.
 *
 * @return true if this atlas has all of the given unicode glyphs
 */
bool Font::Atlas::hasGlyphs(const std::vector<Uint32>& glyphs) const {
    for(auto it = glyphs.begin(); it != glyphs.end(); ++it) {
        if (!hasGlyph(*it)) { return false; }
    }
    return true;
}

/**
 * Creates a single quad to render this character and stores it in mesh
 *
 * This method will append the vertices to the provided mesh and update
 * the indices to include these new vertices. Once the quad is generated,
 * the offset will be adjusted to contain the next place to render a
 * character. This method will not generate anything if the character is
 * not supported by this atlas.
 *
 * The quad is adjusted so that all of the vertices fit in the provided
 * rectangle.  This may mean that no quad is generated at all.
 *
 * @param thechar   The character to convert to render data
 * @param offset    The (unkerned) starting position of the quad
 * @param rect      The bounding box for the quad
 * @param mesh      The mesh to store the vertices
 */
bool Font::Atlas::getQuad(Uint32 thechar, Vec2& offset, Mesh<SpriteVertex2>& mesh, const Rect rect) const {
    CUAssertLog(mesh.command == GL_TRIANGLES, "The mesh is not formatted for triangles");

    // Technically, this answer is correct
    if (!hasGlyph(thechar)) {
        return true;
    }
    
    // Expand tabs
    if (thechar == TAB_CHAR) {
        for(int ii = 0; ii < TAB_SPACE; ii++) {
            if (!getQuad(SPACE_CHAR, offset, mesh, rect)) {
                return false;
            }
        }
        return true;
    }
    
    Rect bounds = glyphmap.at(thechar);
    Rect quad(offset,bounds.size);

    // Skip over glyph, but recognize we may have later glyphs
    if (!rect.doesIntersect(quad)) {
        offset.x += bounds.size.width;
        return quad.getMaxX() <= rect.getMaxX();
    }
    
    // Compute intersection and adjust cookie cutter
    quad.intersect(rect);
    bool result = quad.getMaxX() <= rect.getMaxX();
    
    // REMEMBER! Bounds and rect have different y-orientations.
    bounds.origin.x += quad.origin.x-offset.x;
    bounds.origin.y -= quad.origin.y+quad.size.height-offset.y-bounds.size.height;
    
    float padding = _parent->_atlasPadding;
    offset.x += bounds.size.width-2*padding;
    quad.origin.x -= padding;
    quad.origin.y -= padding;
    bounds.size = quad.size;
    
    int width  = texture->getWidth();
    int height = texture->getHeight();

    SpriteVertex2 temp;
    GLuint size = (GLuint)mesh.vertices.size();
    
    // Bottom left
    GLuint white = Color4::WHITE.getPacked();
    temp.position = quad.origin;
    temp.color = white;
    temp.texcoord.x = bounds.origin.x/(float)width;
    temp.texcoord.y = (bounds.origin.y+bounds.size.height)/(float)height;
    mesh.vertices.push_back(temp);
    
    // Bottom right
    temp.position = quad.origin;
    temp.position.x += bounds.size.width;
    temp.color = white;
    temp.texcoord.x = (bounds.origin.x+bounds.size.width)/(float)width;
    temp.texcoord.y = (bounds.origin.y+bounds.size.height)/(float)height;
    mesh.vertices.push_back(temp);
    
    // Top right
    temp.position = quad.origin+bounds.size;
    temp.color = white;
    temp.texcoord.x = (bounds.origin.x+bounds.size.width)/(float)width;
    temp.texcoord.y = bounds.origin.y/(float)height;
    mesh.vertices.push_back(temp);
    
    // Top left
    temp.position = quad.origin;
    temp.position.y += bounds.size.height;
    temp.color = white;
    temp.texcoord.x = bounds.origin.x/(float)width;
    temp.texcoord.y = bounds.origin.y/(float)height;
    mesh.vertices.push_back(temp);
    
    // Add the quad indices
    mesh.indices.push_back(size);
    mesh.indices.push_back(size+1);
    mesh.indices.push_back(size+2);
    mesh.indices.push_back(size+2);
    mesh.indices.push_back(size+3);
    mesh.indices.push_back(size);
    
    return result;
}

/**
 * Creates a single quad to render this character and stores it in mesh
 *
 * This method will append the vertices to the provided mesh and update
 * the indices to include these new vertices. Once the quad is generated,
 * the offset will be adjusted to contain the next place to render a
 * character. This method will not generate anything if the character is
 * not supported by this atlas.
 *
 * @param thechar   The character to convert to render data
 * @param offset    The (unkerned) starting position of the quad
 * @param mesh      The mesh to store the vertices
 */
void Font::Atlas::getQuad(Uint32 thechar, Vec2& offset, Mesh<SpriteVertex2>& mesh) const {
    CUAssertLog(mesh.command == GL_TRIANGLES, "The mesh is not formatted for triangles");

    // Expand tabs
    if (thechar == TAB_CHAR) {
        for(int ii = 0; ii < TAB_SPACE; ii++) {
            getQuad(SPACE_CHAR, offset, mesh);
        }
        return;
    }
    
    Rect bounds = glyphmap.at(thechar);
    Rect quad(offset,bounds.size);

    // REMEMBER! Bounds and rect have different y-orientations.
    bounds.origin.x += quad.origin.x-offset.x;
    bounds.origin.y -= quad.origin.y+quad.size.height-offset.y-bounds.size.height;
    
    float padding = _parent->_atlasPadding;
    offset.x += bounds.size.width-2*padding;
    quad.origin.x -= padding;
    quad.origin.y -= padding;
    bounds.size = quad.size;
    
    int width  = texture->getWidth();
    int height = texture->getHeight();

    SpriteVertex2 temp;
    GLuint size = (GLuint)mesh.vertices.size();
    
    // Bottom left
    GLuint white = Color4::WHITE.getPacked();
    temp.position = quad.origin;
    temp.color = white;
    temp.texcoord.x = bounds.origin.x/(float)width;
    temp.texcoord.y = (bounds.origin.y+bounds.size.height)/(float)height;
    mesh.vertices.push_back(temp);
    
    // Bottom right
    temp.position = quad.origin;
    temp.position.x += bounds.size.width;
    temp.color = white;
    temp.texcoord.x = (bounds.origin.x+bounds.size.width)/(float)width;
    temp.texcoord.y = (bounds.origin.y+bounds.size.height)/(float)height;
    mesh.vertices.push_back(temp);
    
    // Top right
    temp.position = quad.origin+bounds.size;
    temp.color = white;
    temp.texcoord.x = (bounds.origin.x+bounds.size.width)/(float)width;
    temp.texcoord.y = bounds.origin.y/(float)height;
    mesh.vertices.push_back(temp);
    
    // Top left
    temp.position = quad.origin;
    temp.position.y += bounds.size.height;
    temp.color = white;
    temp.texcoord.x = bounds.origin.x/(float)width;
    temp.texcoord.y = bounds.origin.y/(float)height;
    mesh.vertices.push_back(temp);
    
    // Add the quad indices
    mesh.indices.push_back(size);
    mesh.indices.push_back(size+1);
    mesh.indices.push_back(size+2);
    mesh.indices.push_back(size+2);
    mesh.indices.push_back(size+3);
    mesh.indices.push_back(size);
}
 
/**
 * Builds the texture data for this given atlas.
 *
 * This method does not generate the OpenGL texture, but does all other
 * work in creates the atlas. In particular it creates the image buffer
 * so that texture creation is just one OpenGL call. This creation will
 * happen once {@link materialize()} is called. As a result, it is safe
 * to call this method outside of the main thread.
 *
 * @return true if atlas creation was successful
 */
bool Font::Atlas::build() {
    _surface = allocSurface(_size.width, _size.height);
    if (_surface == nullptr) {
        return false;
    }
    
    SDL_Rect srcrect, dstrect;
    SDL_Color color;
    color.r = color.g = color.b = color.a = 255;
    SDL_Color bkgrd;
    bkgrd.r = bkgrd.g = bkgrd.b = bkgrd.a = 0;
    
    // Add a 2 patch at the beginning
    srcrect.x = srcrect.y = 0;
    srcrect.w = srcrect.h = 2;
    SDL_FillRect(_surface,&srcrect,SDL_MapRGBA(_surface->format, 255, 255, 255, 255));
    
    for(auto it = glyphmap.begin(); it != glyphmap.end(); ++it) {
        SDL_Surface* temp = TTF_RenderGlyph_Blended(_parent->_data, it->first, color);
        if (temp == nullptr) {
            return false;
        }
        
        // Resize the boundary now that spacing is safe.
        it->second.origin.x += GLYPH_BORDER/2;
        it->second.origin.y += GLYPH_BORDER/2;
        it->second.size.width  -= GLYPH_BORDER;
        it->second.size.height -= GLYPH_BORDER;
        
        // Convert to SDL rects
        float padding = _parent->_atlasPadding;
        dstrect.x = (int)it->second.origin.x+padding;
        dstrect.y = (int)it->second.origin.y+padding;
        srcrect.x = srcrect.y = 0;
        dstrect.w = srcrect.w = (int)it->second.size.width-2*padding;
        dstrect.h = srcrect.h = (int)it->second.size.height-2*padding;
        
        // Blit on to atlas
        SDL_SetSurfaceBlendMode(temp, SDL_BLENDMODE_NONE);
        SDL_BlitSurface(temp,&srcrect,_surface,&dstrect);
        SDL_FreeSurface(temp);
    }
    
    return true;
}

/**
 * Creates the OpenGL texture for this atlas.
 *
 * This method must be called on the main thread. It is only safe to call
 * this method after a succesful call to {@link #build()}.
 *
 * @return true if texture creation was successful.
 */
bool Font::Atlas::materialize() {
    if (_surface != nullptr) {
        texture = Texture::allocWithData(_surface->pixels, _surface->w, _surface->h);
        texture->bind();
        texture->buildMipMaps();
        texture->unbind();
        SDL_FreeSurface(_surface);
        _surface = nullptr;
    }
    return texture != nullptr;
}

/**
 * Lays out the glyphs in reasonably efficient packing.
 *
 * This method computes both the size of the atlas and the placement of the
 * individual glyphs. This method will consume glyphs from the provided
 * glyphset as it assigns them a position. So if it successfully adds all
 * glyphs, the value glyphset will be emptied.
 */
void Font::Atlas::layout(std::deque<Uint32>& glyphset) {
    // Find the largest glyph in the set
    int maxwidth = 0;
    for(auto it = glyphset.begin(); it != glyphset.end(); ++it) {
        Metrics metrics = _parent->getMetrics(*it);
        if (metrics.advance > maxwidth) {
            maxwidth = metrics.advance;
        }
    }

    float fontHeight = _parent->_fontHeight;
    float padding = _parent->_atlasPadding;
    _size.width  = nextPOT(maxwidth+GLYPH_BORDER+2*padding);
    _size.height = nextPOT(fontHeight+GLYPH_BORDER+2*padding);
    
    // Copy the glyphs to make a visited set
    int nrows  = 1;
    int line = 0;
    bool avail = true;
    
    std::deque<Uint32> glyphque;
    std::vector<float> used;
    used.push_back(2);  // Give us a spot for a 2-patch
    while (avail && !glyphset.empty()) {
        // We have finished the line
        if (used[line] >= _size.width) {
            // There is no more room
            if (line+1 >= nrows) {
                if (_size.width < _size.height) {
                    _size.width *= 2;
                    line = 0;
                } else if (_size.height < MAX_ATLAS_SIZE) {
                    int orows = nrows;
                    _size.height *= 2;
                    nrows = _size.height/(fontHeight+GLYPH_BORDER+2*padding);
                    used.reserve(nrows);
                    for(int ii = orows; ii < nrows; ii++) {
                        used.push_back(0);
                    }
                    line++;
                } else {
                    avail = false;
                }
            } else {
                line++;
            }
        }
        
        // Fit the largest glyph possible on this line
        if (avail) {
            bool found = false;
            auto pos = glyphset.begin();
            for(auto it = glyphset.begin(); !found && it != glyphset.end(); ++it) {
                float advance = _parent->getMetrics(*it).advance;
                if (advance+GLYPH_BORDER+2*padding < _size.width-used[line]) {
                    float w = advance+GLYPH_BORDER+2*padding;
                    float h = fontHeight+GLYPH_BORDER+2*padding;
                    float x = used[line];
                    float y = line*h;
                    glyphmap.emplace(*it, Rect(x,y,w,h));

                    used[line] += w;
                    pos = it;
                    found = true;
                }
            }
            
            if (!found) {
                used[line] = _size.width;
            } else {
                // Gobble
                glyphset.erase(pos);
            }
        }
    }
}

/**
 * Allocates a blank surface of the given size.
 *
 * This method is necessary because SDL surface allocation is quite
 * involved when you want proper alpha support.
 *
 * @return a blank surface of the given size.
 */
SDL_Surface* Font::Atlas::allocSurface(int width, int height) {
    SDL_Surface* result;
    
    // Masks appear to be necessary for alpha support
    Uint32 rmask, gmask, bmask, amask;
    
    // Unfortunately, masks are endian
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
    
    result = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32, rmask, gmask, bmask, amask);
    SDL_SetSurfaceBlendMode(result, SDL_BLENDMODE_BLEND);
    SDL_FillRect(result, NULL, SDL_MapRGBA(result->format, 0, 0, 0, 0));
    return result;
}


#pragma mark -
#pragma mark Font
/**
 * Creates a degenerate font with no data.
 *
 * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
 * the heap, use one of the static constructors instead.
 */
Font::Font() :
_name(""),
_stylename(""),
_fontSize(0),
_data(nullptr),
_fontHeight(0),
_fontAscent(0),
_fontDescent(0),
_fontLineSkip(0),
_atlasPadding(0),
_shrinkLimit(0),
_stretchLimit(0),
_fallback(false),
_fixedWidth(false),
_useKerning(true),
_style(Style::NORMAL),
_hints(Hinting::NORMAL){ }

/**
 * Deletes the font resources and resets all attributes.
 *
 * You must reinitialize the font to use it.
 */
void Font::dispose() {
    if (_data != nullptr) {
        TTF_CloseFont(_data);
        _data = nullptr;
    }
    
    _name = "";
    _stylename = "";
    _fontSize = 0;
    _fontHeight = 0;
    _fontAscent = 0;
    _fontDescent = 0;
    _fontLineSkip = 0;
    _atlasPadding = 0;
    _fixedWidth = false;
    _useKerning = true;
    _style  = Style::NORMAL;
    _hints  = Hinting::NORMAL;
    _glyphsize.clear();
    _kernmap.clear();
    _atlases.clear();
    _atlasmap.clear();
}

/**
 * Initializes a font of the given size from the file.
 *
 * The font size is fixed on initialization.  It cannot be changed without
 * disposing of the entire font.  However, all other attributes may be
 * changed.
 *
 * @param file  The file with the font asset
 * @param size  The font size in points
 *
 * @return true if initialization is successful.
 */
bool Font::init(const std::string file, Uint32 size) {
    if (_data != nullptr) {
        CUAssertLog(false,"Font %s already loaded", _name.c_str());
        return false;
    }
    std::string fullpath = filetool::normalize_path(file);
    _data = TTF_OpenFont(fullpath.c_str(), size);
    if (_data == nullptr) {
        CUAssertLog(false, "Font initialization error: %s", TTF_GetError());
        return false;
    }
    _fontSize = size;
    char* strng = TTF_FontFaceFamilyName(_data);
    _name = std::string(strng);

    strng = TTF_FontFaceStyleName(_data);
    _stylename = std::string(strng);
    
    _fontHeight   = TTF_FontHeight(_data);
    _fontAscent   = TTF_FontAscent(_data);
    _fontDescent  = TTF_FontDescent(_data);
    _fontLineSkip = TTF_FontLineSkip(_data);
    _fixedWidth   = TTF_FontFaceIsFixedWidth(_data) != 0;

    return true;
}



#pragma mark -
#pragma mark Attributes
/**
 * Returns true if this font has a glyph for the given (UNICODE) character.
 *
 * The Unicode representation uses the endianness native to the platform.
 * Therefore, this value should not be serialized.  Use UTF8 to represent
 * unicode in a platform-independent manner.
 *
 * If the font has an associated atlas, this will return true only if the
 * character is in the atlas.  You will need to clear the atlas to get the
 * full range of characters.
 *
 * @param thechar   The Unicode character to check.
 *
 * @return true if this font has a glyph for the given (UNICODE) character.
 */
bool Font::hasGlyph(Uint32 a) const {
    return a == TAB_CHAR || TTF_GlyphIsProvided(_data, a) != 0;
}

/**
 * Returns true if this font can successfuly render the given glyphs.
 *
 * This method is not an indication of whether or not there is a font
 * atlas for this font. It is simply an indication whether or not these
 * glyphs are present in the font source. This method will return false
 * if just one glyph is missing.
 *
 * The glyph identifiers may either be in UTF8 or ASCII; the method will
 * handle conversion automatically.
 *
 * @param glyphs  The glyph identifiers to check.
 *
 * @return true if this font can successfuly render the given glyphs.
 */
bool Font::hasGlyphs(const std::string text) const {
    const char* begin = text.c_str();
    const char* check = text.c_str();
    const char* end   = begin+text.size();
    CUAssertLog(utf8::find_invalid(check, end) == end, "String '%s' has an invalid UTF-8 encoding",begin);
    
    while (begin != end) {
        if (!hasGlyph(utf8::next(begin, end))) { return false; }
    }
    return true;
}

/**
 * Sets whether this font atlas uses kerning when rendering.
 *
 * Without kerning, each character is guaranteed to take up its enitre
 * advance when rendered.  This may make spacing look awkard. This
 * value is true by default.
 *
 * Reseting this value will clear any existing atlas collection.
 *
 * @param kerning   Whether this font atlas uses kerning when rendering.
 */
void Font::setKerning(bool kerning) {
    if (_useKerning != kerning) {
        _useKerning = kerning;
        TTF_SetFontKerning(_data, _useKerning);
        clearAtlases();
    }
}

/**
 * Sets the style for this font.
 *
 * Changing this value will delete any atlas that is present.  The atlas
 * must be regenerated.
 *
 * With the exception of normal style (which is an absent of any style), all
 * of the styles may be combined.  So it is possible to have a bold, italic,
 * underline font with strikethrough. To combine styles, simply treat the
 * Style value as a bitmask, and combine them with bitwise operations.
 *
 * Reseting this value will clear any existing atlas collection.
 *
 * @param style The style for this font.
 */
void Font::setStyle(Style style) {
    if (_style != style) {
        _style = style;
        TTF_SetFontStyle(_data, (int)style);
        clearAtlases();
    }
}

/**
 * Sets the rasterization hints
 *
 * Changing this value will delete any atlas that is present.  The atlas
 * must be regenerated.
 *
 * Hinting is used to align the font to a rasterized grid. At low screen
 * resolutions, hinting is critical for producing clear, legible text
 * (particularly if you are not supporting antialiasing).
 *
 * Reseting this value will clear any existing atlas collection.
 *
 * @param hinting   The rasterization hints
 */
void Font::setHinting(Hinting hinting) {
    if (_hints != hinting) {
        _hints = hinting;
        TTF_SetFontHinting(_data, (int)hinting);
        clearAtlases();
    }
}

/**
 * Sets the atlas padding
 *
 * The atlas padding the guaranteed padding between glyphs in the textures
 * for the atlas collection. By default, glyphs are no more than a pixel
 * apart from each other, to minimize texture size. This value represents
 * the individual pixels to add along all four borders of the glyph.
 *
 * However, this prevents font blur effects, as the individual glyphs will
 * blur into each other. If you plan to render a font with a non-zero value
 * to {@link SpriteBatch#blurRadius}, then you must add padding equal to
 * or exceeding the radius.
 *
 * Reseting this value will clear any existing atlas collection.
 *
 * @param padding   The additional atlas padding
 */
void Font::setPadding(Uint32 padding) {
    if (_atlasPadding != padding) {
        _atlasPadding = padding;
        clearAtlases();
    }
}

#pragma mark -
#pragma mark Measurements
/**
 * Returns the glyph metrics for the given (Unicode) character.
 *
 * See {@link Metrics} for an explanation of the data provided by this
 * method. This method will fail if the glyph is not in this font. In
 * particular, control characters (e.g. newlines) will fail while spaces
 * will not.
 *
 * The Unicode representation uses the endianness native to the platform.
 * Therefore, this value should not be serialized.  Use UTF8 to represent
 * unicode in a platform-independent manner. See the function
 * {@link strtool::getCodePoint} for how to get a unicode codepoint from
 * a UTF8 string.
 *
 * @param thechar   The Unicode character to measure.
 *
 * @return the glyph metrics for the given (Unicode) character.
 */
const Font::Metrics Font::getMetrics(Uint32 thechar) const {
    if (_glyphsize.find(thechar) != _glyphsize.end()) {
        return _glyphsize.at(thechar);
    }
    
    if (thechar == TAB_CHAR) {
        Font::Metrics metrics = getMetrics(SPACE_CHAR);
        if (metrics.maxx > 0) {
            metrics.maxx += (TAB_SPACE-1)*metrics.advance;
        }
        metrics.advance *= TAB_SPACE;
        return metrics;
    }
    
    CUAssertLog(TTF_GlyphIsProvided(_data, thechar), "Character '%c' is not supported", thechar);
    return computeMetrics(thechar);
}

/**
 * Returns the kerning adjustment between the two (Unicode) characters.
 *
 * This value is the amount of overlap (in pixels) between any two adjacent
 * character glyphs rendered by this font.  If the value is 0, there is no
 * kerning for this pair.
 *
 * The Unicode representation uses the endianness native to the platform.
 * Therefore, this value should not be serialized.  Use UTF8 to represent
 * unicode in a platform-independent manner. See the function
 * {@link strtool::getCodePoint} for how to get a unicode codepoint from
 * a UTF8 string.
 *
 * @param a     The first Unicode character in the pair
 * @param b     The second Unicode character in the pair
 *
 * @return the kerning adjustment between the two (Unicode) characters.
 */
unsigned int Font::getKerning(Uint32 a, Uint32 b) const {
    if (_glyphsize.find(a) != _glyphsize.end() && _glyphsize.find(b) != _glyphsize.end()) {
        return _kernmap.at(a).at(b);
    }

    if (is_control(a) || is_control(b)) {
        return 0;
    }

    CUAssertLog(TTF_GlyphIsProvided(_data, a), "Character '%c' is not supported", a);
    CUAssertLog(TTF_GlyphIsProvided(_data, b), "Character '%c' is not supported", b);
    return computeKerning(a, b);
}

/**
 * Returns the size (in pixels) necessary to render this string.
 *
 * This size is a conservative estimate to render the string. The height
 * is guaranteed to be the maximum height of the font, regardless of the
 * text measured.  In addition, the measurement will include the full
 * advance of the both the first and last characters.  This means that
 * there may be some font-specific padding around these characters.
 *
 * The y-origin of this rectangle is guaranteed to be {@link #getDescent}.
 * That is because glyphs will use the baseline as the origin when
 * rendering the text.
 *
 * This measurement does not actually render the string. This method will
 * not fail if it includes glyphs that are not present in the font, but it
 * will drop them when measuring the size.
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically.
 *
 * @param text  The string to measure
 *
 * @return the size (in pixels) necessary to render this string.
 */
Size Font::getSize(const std::string text) const {
    return getSize(text.c_str(),text.c_str()+text.size()+1);
}

/**
 * Returns the size (in pixels) necessary to render this string.
 *
 * This size is a conservative estimate to render the string.  The height
 * is guaranteed to be the maximum height of the font, regardless of the
 * text measured.  In addition, the measurement will include the full
 * advance of the both the first and last characters.  This means that
 * there may be some font-specific padding around these characters.
 *
 * The y-origin of this rectangle is guaranteed to be {@link #getDescent}.
 * That is because glyphs will use the baseline as the origin when
 * rendering the text.
 *
 * This measurement does not actually render the string. This method will
 * not fail if it includes glyphs that are not present in the font, but it
 * will drop them when measuring the size.
 *
 * The C-style string substr need not be null-terminated. Instead,
 * the termination is indicated by the parameter end. This provides
 * efficient substring processing. The string may either be in UTF8
 * or ASCII; the method will handle conversion automatically.
 *
 * @param substr    The start of the string to measure
 * @param end       The end of the string to measure
 *
 * @return the size (in pixels) necessary to render this string.
 */
Size Font::getSize(const char* substr, const char* end) const {
    Size result(0, (float)_fontHeight);
    
    const char* begin = substr;
    Uint32 prvchar = 0;
    while (begin != end) {
        Uint32 thechar = utf8::next(begin,end);
        if (_glyphsize.find(thechar) != _glyphsize.end()) {
            if (prvchar > 0 && _glyphsize.find(prvchar) != _glyphsize.end()) {
                result.width -= _kernmap.at(prvchar).at(thechar);
            }
            result.width += _glyphsize.at(thechar).advance;
        } else {
            if (prvchar > 0) {
                result.width -= computeKerning(prvchar,thechar);
            }
            result.width +=computeMetrics(thechar).advance;
        }
        prvchar = thechar;
    }
    return result;
}

/**
 * Returns the pixel offset of the glyphs inside a rendered string.
 *
 * The result of {@link getSize()} is very conservative.  Even if no
 * character uses the maximum height, it provides the full height of
 * the font.  Furthermore, if the last character does not use the full
 * advance, there will be padding after that character.
 *
 * The rectangle returned by this method provide the internal bounds of
 * the rendered text.  The value is in "text space".  If a string is
 * rendered at position (0,0), this is the bounding box for all of the
 * glyphs that are actually rendered.  It is the tightest bounding box
 * that can fit all of the generated glyph.  You can use this rectangle
 * to eliminate any font-specific spacing that may have been placed
 * around the glyphs.
 *
 * For example, suppose the string is "ah".  In many fonts, these two
 * glyphs would not dip below the baseline. Therefore, the y value of
 * the returned rectangle would be at the font baseline (which is always
 * 0), indicating that it is safe to start rendering there.
 *
 * This measurement does not actually render the string. This method will
 * not fail if it includes glyphs that are not present in the font, but it
 * will drop them when measuring the size.
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically.
 *
 * @param text  The string to measure
 *
 * @return the size of the quad sequence generated for this string.
 */
Rect Font::getInternalBounds(const std::string text) const {
    return getInternalBounds(text.c_str(),text.c_str()+text.size()+1);
}

/**
 * Returns the pixel offset of the glyphs inside a rendered string.
 *
 * The result of {@link getSize()} is very conservative.  Even if no
 * character uses the maximum height, it provides the full height of
 * the font.  Furthermore, if the last character does not use the full
 * advance, there will be padding after that character.
 *
 * The rectangle returned by this method provide the internal bounds of
 * the rendered text.  The value is in "text space".  If a string is
 * rendered at position (0,0), this is the bounding box for all of the
 * glyphs that are actually rendered.  It is the tightest bounding box
 * that can fit all of the generated glyph.  You can use this rectangle
 * to eliminate any font-specific spacing that may have been placed
 * around the glyphs.
 *
 * For example, suppose the string is "ah".  In many fonts, these two
 * glyphs would not dip below the baseline.  Therefore, the y value of
 * the returned rectangle would be at the font baseline (which is always
 * 0), indicating that it is safe to start rendering there.
 *
 * This measurement does not actually render the string. This method will
 * not fail if it includes glyphs that are not present in the font, but it
 * will drop them when measuring the size.
 *
 * The C-style string substr need not be null-terminated. Instead,
 * the termination is indicated by the parameter end. This provides
 * efficient substring processing. The string may either be in UTF8
 * or ASCII; the method will handle conversion automatically.
 *
 * @param substr    The start of the string to measure
 * @param end       The end of the string to measure
 *
 * @return the size of the quad sequence generated for this string.
 */
Rect Font::getInternalBounds(const char* substr, const char* end) const {
    Rect result;
    Metrics metrics;
    
    // These values allow us to skip over characters
    Uint32 first = 0;
    Uint32 last  = 0;
    
    // To track the height
    int maxy = 0;
    int miny = 0;
    
    // First character
    const char* begin = substr;
    while(first == 0 && begin != end) {
        Uint32 ch = utf8::next(begin,end);
        if (hasGlyph(ch)) {
            metrics = (_glyphsize.find(ch) != _glyphsize.end() ? _glyphsize.at(ch) : computeMetrics(ch));
            result.origin.x = (float)metrics.minx;
            result.size.width = (float)(metrics.advance-metrics.minx);
            maxy = (metrics.maxy > maxy ? metrics.maxy : maxy);
            miny = (metrics.miny < miny ? metrics.miny : miny);
            first = ch;
        }
    }

    if (first == 0) {
        return result;
    }
    
    // Later characters
    last = first;
    while (begin != end) {
        Uint32 ch = utf8::next(begin,end);
        if (hasGlyph(ch)) {
            bool present = _glyphsize.find(ch) != _glyphsize.end();
            result.size.width -= (present ? _kernmap.at(last).at(ch) : computeKerning(last, ch));
            metrics = (present ? _glyphsize.at(ch) : computeMetrics(ch));
            result.size.width += metrics.advance;
            maxy = (metrics.maxy > maxy ? metrics.maxy : maxy);
            miny = (metrics.miny < miny ? metrics.miny : miny);
            last = ch;
        }
    }
    
    // Adjust for last character
    if (last != SPACE_CHAR) {
        result.size.width -= metrics.advance-metrics.maxx;
        result.origin.y = (float)(-getDescent()+miny);
        result.size.height = (float)(maxy-miny);
    }
    return result;
}

/**
 * Returns the tracking adjustments to fit the text in the given width
 *
 * Unlike kerning, tracking is used to dynamically adjust the spaces between
 * characters. The purpose is to fix the text to the given width exactly (or
 * as close as possible).  Usually this means shrinking the space when the
 * text is larger than the width.  But in the case of justification, it may
 * also be used to increase the space. The number of tracking measurements
 * is one less than the number of characters.
 *
 * All tracking is measured in integer offsets. That is because text looks
 * more uniform when glyph positions are at integral values (otherwise the
 * texture may shimmer on movement). Whenever possible, the algorithm will
 * try to track the text to within 1 unit of the width (under, not over).
 * In the case of shrinking, this may not be possible if the shrink limit
 * is too low.
 *
 * Tracking adjustments will be uniform between non-space characters. If
 * any non-uniform adjustments need to be made, they will be made around
 * white-space.
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically.
 *
 * @param text      The start of the string to measure
 * @param end       The end of the string to measure
 * @param width     The line width
 *
 * @return the tracking adjustments to fit the text in the given width
 */
std::vector<int> Font::getTracking(const std::string text, float width) {
    return getTracking(text.c_str(), text.c_str()+text.size(), width);
}

/**
 * Returns the tracking adjustments to fit the text in the given width
 *
 * Unlike kerning, tracking is used to dynamically adjust the spaces between
 * characters. The purpose is to fix the text to the given width exactly (or
 * as close as possible).  Usually this means shrinking the space when the
 * text is larger than the width.  But in the case of justification, it may
 * also be used to increase the space. The number of tracking measurements
 * is one less than the number of characters.
 *
 * All tracking is measured in integer offsets. That is because text looks
 * more uniform when glyph positions are at integral values (otherwise the
 * texture may shimmer on movement). Whenever possible, the algorithm will
 * try to track the text to within 1 unit of the width (under, not over).
 * In the case of shrinking, this may not be possible if the shrink limit
 * is too low.
 *
 * Tracking adjustments will be uniform between non-space characters. If
 * any non-uniform adjustments need to be made, they will be made around
 * white-space.
 *
 * The C-style string substr need not be null-terminated. Instead, the
 * termination is indicated by the parameter end. This provides efficient
 * substring processing. The string may either be in UTF8 or ASCII; the
 * method will handle conversion automatically.
 *
 * @param substr    The start of the string to measure
 * @param end       The end of the string to measure
 * @param width     The line width
 *
 * @return the tracking adjustments to fit the text in the given width
 */
std::vector<int> Font::getTracking(const char* substr, const char* end, float width) {
    // Get the number of characters and the number of spaces;
    size_t length = 0;
    size_t spaces = 0;
    const char* begin = substr;
    Uint32 prvchar = 0;
    Uint32 thechar = 0;
    while (begin != end) {
        if (prvchar > 0 && is_whitespace(prvchar)) {
            spaces++;
        }
        thechar = utf8::next(begin,end);
        if (is_whitespace(thechar)) {
            spaces++;
        }
        length++;
        prvchar = thechar;
    }
    
    if (length < 2) {
        return std::vector<Sint32>();
    }

    Size size = getSize(substr,end);
    std::vector<Sint32> result;
    result.reserve(length-1);
    if (size.width > width) {
        float diff = size.width-width;
        
        // See if spaces are good enough;
        Sint32 unit = (Sint32)round(diff/(spaces));
        if (unit < _shrinkLimit) {
            // Shrink time
            begin = substr;
            prvchar = utf8::next(begin,end);
            while (begin != end) {
                Sint32 amt = 0;
                if (is_whitespace(prvchar)) {
                    unit = std::min(std::min((Sint32)round(diff/(spaces)),_shrinkLimit),(Sint32)diff);
                    amt += unit;
                    diff -= unit; spaces--;
                }
                thechar = utf8::next(begin,end);
                if (is_whitespace(thechar)) {
                    unit = std::min(std::min((Sint32)round(diff/(spaces)),_shrinkLimit),(Sint32)diff);
                    amt += unit;
                    diff -= unit; spaces--;
                }
                result.push_back(-amt);
                prvchar = thechar;
            }
        } else {
            // Need word spacing too
            unit = std::min((Sint32)(diff/(length-1)),_shrinkLimit);
            diff -= unit*(length-1);

            // Shrink limit prevents us from doing more, even to spaces
            Sint32 left;
            
            // Shrink time
            begin = substr;
            prvchar = utf8::next(begin,end);
            while (begin != end) {
                Sint32 amt = unit;
                if (is_whitespace(prvchar)) {
                    left = std::min(std::min((Sint32)round(diff/(spaces)),_shrinkLimit-unit),(Sint32)diff);
                    amt += left;
                    diff -= left; spaces--;
                }
                thechar = utf8::next(begin,end);
                if (is_whitespace(thechar)) {
                    left = std::min(std::min((Sint32)round(diff/(spaces)),_shrinkLimit-unit),(Sint32)diff);
                    amt += left;
                    diff -= left; spaces--;
                }
                result.push_back(-amt);
                prvchar = thechar;
            }
        }
        // Add in at back if anything remaining
        size_t pos = result.size()-1;
        while (diff > 0 && pos > 0) {
            if (-result[pos] < _shrinkLimit) {
                result[pos]--;
                diff--;
            }
            pos--;
        }
    } else if (size.width < width) {
        float diff = width-size.width;
        Uint32 unit = std::min((Sint32)(diff/(length-1)),_stretchLimit);
        diff -= unit*(length-1);
        Uint32 left;

        // Grow time
        result.reserve(length-1);
        begin = substr;
        prvchar = utf8::next(begin,end);
        while (begin != end) {
            Sint32 amt = unit;
            if (is_whitespace(prvchar)) {
                left = std::min((Sint32)round(diff/(spaces)),(Sint32)diff);
                amt += left;
                diff -= left; spaces--;
            }
            thechar = utf8::next(begin,end);
            if (is_whitespace(thechar)) {
                left = std::min((Sint32)round(diff/(spaces)),(Sint32)diff);
                amt += left;
                diff -= left; spaces--;
            }
            result.push_back(amt);
            prvchar = thechar;
        }
    }
    return result;
}

#pragma mark -
#pragma mark Atlas Support
/**
 * Deletes the current collections of atlases
 *
 * Until a new font atlas is created, any attempt to use this font
 * will result in adhoc atlases (e.g. one-off atlases associated
 * with a single glyph run).
 */
void Font::clearAtlases() {
    _atlases.clear();
}

/**
 * Creates an atlas collection for the ASCII characters in this font.
 *
 * Only the ASCII characters are added to the atlases, even if the font has
 * support for more characters. You should use a character set method
 * if you want Unicode characters supported.
 *
 * If there is already an active atlas collection, this method will preserve
 * those atlas textures. Instead, it will only generate atlases for the
 * characters that are not currently supported.
 *
 * The character atlas textures are generated immediately, so the method
 * {@link getAtlases()} may be called with no delay.
 *
 * WARNING: This initializer is not thread safe.  It generates OpenGL
 * textures, which means that it may only be called in the main thread.
 *
 * @return true if the atlases were successfully created.
 */
bool Font::buildAtlases() {
    bool result = buildAtlasesAsync();
    result = result && storeAtlases();
    return result && (!_atlases.empty());
}

/**
 * Creates an atlas collection for the given character set.
 *
 * The atlases generated contains characters in the provided character set,
 * and will omit all other chacters. This includes ASCII characters that may
 * be missing from the character set. The character set string must either
 * be in ASCII or UTF8 encoding. It will handle both automatically, but no
 * other encoding (e.g. Latin1) is accepted.
 *
 * If there is already an active atlas collection, this method will preserve
 * those atlas textures. Instead, it will only generate atlases for the
 * characters that are not currently supported.
 *
 * The character atlas textures are generated immediately, so the method
 * {@link getAtlases()} may be called with no delay.
 *
 * WARNING: This initializer is not thread safe. It generates an OpenGL
 * texture, which means that it may only be called in the main thread.
 *
 * @param charset   The set of characters in the atlas
 *
 * @return true if the atlases were successfully created.
 */
bool Font::buildAtlases(const std::string charset) {
    bool result = buildAtlasesAsync(charset);
    result = result && storeAtlases();
    return result && (!_atlases.empty());
}

/**
 * Creates an atlas collection for the given character set.
 *
 * The atlases generated contains characters in the provided character set,
 * and will omit all other chacters. This includes ASCII characters that may
 * be missing from the character set. The character set provided must be
 * a collection of UNICODE encodings. The Unicode representation uses the
 * endianness native to the platform. Therefore, this value should not be
 * serialized. Use UTF8 to represent unicode in a platform-independent manner.
 *
 * If there is already an active atlas collection, this method will preserve
 * those atlas textures. Instead, it will only generate atlases for the
 * characters that are not currently supported.
 *
 * The character atlas textures are generated immediately, so the method
 * {@link getAtlases()} may be called with no delay.
 *
 * WARNING: This initializer is not thread safe. It generates an OpenGL
 * texture, which means that it may only be called in the main thread.
 *
 * @param charset   The set of characters in the atlas
 *
 * @return true if the atlases were successfully created.
 */
bool Font::buildAtlases(const std::vector<Uint32>& charset) {
    bool result = buildAtlasesAsync(charset);
    result = result && storeAtlases();
    return result && (!_atlases.empty());
}

/**
 * Creates an atlas collection for the ASCII characters in this font.
 *
 * Only the ASCII characters are added to the atlases, even if the font has
 * support for more characters. You should use a character set method
 * if you want Unicode characters supported.
 *
 * If there is already an active atlas collection, this method will preserve
 * those atlas textures. Instead, it will only generate atlases for the
 * characters that are not currently supported.
 *
 * This method does not generate any OpenGL textures, but does all other
 * work in creates the atlases. In particular it creates the image buffers
 * so that texture creation is just one OpenGL call. This creation will
 * happen the first time that {@link getAtlases()} is called.
 *
 * As a result, this method is thread safe. It may be called in any
 * thread, including threads other than the main one.
 *
 * @return true if the atlases were successfully created.
 */
bool Font::buildAtlasesAsync() {
    std::deque<Uint32> glyphs = gatherGlyphs();
    if (glyphs.empty()) {
        return false;
    }
    
    gatherKerning(glyphs);
    bool success = true;
    while (success && glyphs.size() > 0) {
        std::shared_ptr<Atlas> atlas = Atlas::alloc(this, glyphs);
        if (atlas != nullptr) {
            success = atlas->build();
            size_t pos = _atlases.size();
            _atlases.push_back(atlas);
            for(auto it = atlas->glyphmap.begin(); it != atlas->glyphmap.end(); ++it) {
                _atlasmap.emplace(it->first,pos);
                if (it->first == SPACE_CHAR) {
                    _atlasmap.emplace(TAB_CHAR,pos);
                }
            }
        } else {
            success = false;
        }
    }
    
    return success;
}

/**
 * Creates an atlas collection for the given character set.
 *
 * The atlas only contains characters in the provided character set, and
 * will omit all other chacters.  This includes ASCII characters that may
 * be missing from the character set. The character set string must either
 * be in ASCII or UTF8 encoding. It will handle both automatically, but no
 * other encoding (e.g. Latin1) is accepted.
 *
 * If there is already an active atlas collection, this method will preserve
 * those atlas textures. Instead, it will only generate atlases for the
 * characters that are not currently supported.
 *
 * This method does not generate any OpenGL textures, but does all other
 * work in creates the atlases. In particular it creates the image buffers
 * so that texture creation is just one OpenGL call. This creation will
 * happen the first time that {@link #storeAtlases()} is called.
 *
 * As a result, this method is thread safe. It may be called in any
 * thread, including threads other than the main one.
 *
 * @param charset   The set of characters in the atlas
 *
 * @return true if the atlases were successfully created.
 */
bool Font::buildAtlasesAsync(const std::string charset) {
    std::deque<Uint32> glyphs = gatherGlyphs(charset);
    gatherKerning(glyphs);
    bool success = true;
    while (success && glyphs.size() > 0) {
        std::shared_ptr<Atlas> atlas = Atlas::alloc(this, glyphs);
        if (atlas != nullptr) {
            success = atlas->build();
            size_t pos = _atlases.size();
            _atlases.push_back(atlas);
            for(auto it = atlas->glyphmap.begin(); it != atlas->glyphmap.end(); ++it) {
                _atlasmap.emplace(it->first,pos);
                if (it->first == SPACE_CHAR) {
                    _atlasmap.emplace(TAB_CHAR,pos);
                }
            }
        } else {
            success = false;
        }
    }
    
    return success;
}

/**
 * Creates an atlas collection for the given character set.
 *
 * The atlases generated contains characters in the provided character set,
 * and will omit all other chacters. This includes ASCII characters that may
 * be missing from the character set. The character set provided must be
 * a collection of UNICODE encodings. The Unicode representation uses the
 * endianness native to the platform. Therefore, this value should not be
 * serialized. Use UTF8 to represent unicode in a platform-independent manner.
 *
 * If there is already an active atlas collection, this method will preserve
 * those atlas textures. Instead, it will only generate atlases for the
 * characters that are not currently supported.
 *
 * This method does not generate any OpenGL textures, but does all other
 * work in creates the atlases. In particular it creates the image buffers
 * so that texture creation is just one OpenGL call. This creation will
 * happen the first time that {@link #storeAtlases()} is called.
 *
 * As a result, this method is thread safe. It may be called in any
 * thread, including threads other than the main one.
 *
 * @param charset   The set of characters in the atlas
 *
 * @return true if the atlases were successfully created.
 */
bool Font::buildAtlasesAsync(const std::vector<Uint32>& charset) {
    std::deque<Uint32> glyphs = gatherGlyphs(charset);
    gatherKerning(glyphs);
    bool success = true;
    while (success && glyphs.size() > 0) {
        std::shared_ptr<Atlas> atlas = Atlas::alloc(this, glyphs);
        if (atlas != nullptr) {
            success = atlas->build();
            size_t pos = _atlases.size();
            _atlases.push_back(atlas);
            for(auto it = atlas->glyphmap.begin(); it != atlas->glyphmap.end(); ++it) {
                _atlasmap.emplace(it->first,pos);
                if (it->first == SPACE_CHAR) {
                    _atlasmap.emplace(TAB_CHAR,pos);
                }
            }
        } else {
            success = false;
        }
    }
    
    return success;
}

/**
 * Creates an OpenGL texture for each atlas in the collection.
 *
 * This method should be called to finalize the work of {@link #buildAtlasesAsync}.
 * This method must be called on the main thread.
 */
bool Font::storeAtlases() {
    bool success = true;
    int count = 0;
    for(auto it = _atlases.begin(); success && it != _atlases.end(); ++it) {
        success = (*it)->materialize();
        auto texture = (*it)->texture;
        texture->bind();
        texture->unbind();
        count++;
    }
    
    return success;
}
/**
 * Returns the OpenGL textures for the associated atlas collection.
 *
 * When combined with a quad sequence generated by the associate atlas,
 * each texture can be used to draw a font in a {@link SpriteBatch}. If
 * there is no atlas collection, this method returns an empty vector.
 *
 * @return the OpenGL textures for the associated atlas collection.
 */
const std::vector<std::shared_ptr<Texture>> Font::getAtlases() {
    std::vector<std::shared_ptr<Texture>> result;
    bool success = true;
    for(auto it = _atlases.begin(); success && it != _atlases.end(); ++it) {
        success = (*it)->materialize();
        result.push_back((*it)->texture);
    }
    
    if (!success) {
        result.clear();
    }
    return result;
}

/**
 * Returns true if the given characters have atlas support.
 *
 * If this method is true, then {@link #getGlyphRuns()} is guaranteed
 * to succeed and be thread safe, whenever the text is consists of
 * provided characters.
 *
 * The characters provided should be in UTF8 or ASCII format.
 *
 * @param charset   The characters to test (as UTF8 or ASCII)
 *
 * @return true if the given characters have atlas support.
 */
bool Font::hasAtlases(const std::string charset) const {
    const char* begin = charset.c_str();
    const char* check = charset.c_str();
    const char* end   = begin+charset.size();
    CUAssertLog(utf8::find_invalid(check, end) == end, "String '%s' has an invalid UTF-8 encoding",begin);
    
    while (begin != end) {
        if (!hasAtlas(utf8::next(begin,end))) { return false; }
    }
    return true;
}

/**
 * Returns true if the given characters have atlas support.
 *
 * If this method is true, then {@link #getGlyphRuns()} is guaranteed
 * to succeed and be thread safe, whenever the text is consists of
 * provided characters.
 *
 * The characters provided should be represented by UNICODE values.
 * The Unicode representation uses the endianness native to the platform.
 * Therefore, this value should not be serialized. Use UTF8 to represent
 * unicode in a platform-independent manner.
 *
 * @param charset   The characters to test (as UNICODE)
 *
 * @return true if the given characters have atlas support.
 */
bool Font::hasAtlases(const std::vector<Uint32>& charset) const {
    for(auto it = charset.begin(); it != charset.end(); ++it) {
        if (!hasAtlas(*it)) { return false; }
    }
    return true;
}

#pragma mark -
#pragma mark Glyph Generation
/**
 * Returns a set of glyph runs to render the given string
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
 *
 * If a character in the string is not represented by a glyph in
 * the atlas collection, then it will be skipped unless the value
 * {@link #setFallbackAtlas} is set to true. In that case, this method
 * will generate a one-time atlas for these characters and return its
 * texture as part of the set. However, this atlas will not be stored
 * for future use. In addition, forcing this creation of a fallback
 * atlas makes this method no longer safe to be used outside of the
 * main thread (this is not an issue if {@link #getFallbackAtlas} is
 * false).
 *
 * The origin value determines the position of the bottom of the text
 * baseline. You should use the methods {@link #getDescent} and
 * {@link #getAscent} to place either the bottom or top of the text,
 * respectively.
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically.
 *
 * @param text      The string to convert to render data.
 * @param origin    The position of the first character
 *
 * @return a set of glyph runs to render the given string
 */
std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> Font::getGlyphs(const std::string text, const Vec2 origin) {
    std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> result;
    const char* begin = text.c_str();
    const char* end = begin+text.size();
    Rect bounds(origin,getSize(begin,end));
    getGlyphs(result, begin, end, origin, bounds, 0);
    return result;
}

/**
 * Returns a set of glyph runs to render the given (sub)string
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
 *
 * If a character in the string is not represented by a glyph in
 * the atlas collection, then it will be skipped unless the value
 * {@link #setFallbackAtlas} is set to true. In that case, this method
 * will generate a one-time atlas for these characters and return its
 * texture as part of the set. However, this atlas will not be stored
 * for future use. In addition, forcing this creation of a fallback
 * atlas makes this method no longer safe to be used outside of the
 * main thread (this is not an issue if {@link #getFallbackAtlas} is
 * false). Note that control characters (e.g. newlines) have no glyphs.
 *
 * The origin value determines the position of the bottom of the text
 * baseline. You should use the methods {@link #getDescent} and
 * {@link #getAscent} to place either the bottom or top of the text,
 * respectively.
 *
 * The C-style string substr need not be null-terminated. Instead,
 * the termination is indicated by the parameter end. This provides
 * efficient substring processing. The string may either be in UTF8
 * or ASCII; the method will handle conversion automatically.
 *
 * @param substr    The start of the string for glyph generation
 * @param end       The end of the string for glyph generation
 * @param origin    The position of the first character
 *
 * @return a set of glyph runs to render the given string
 */
std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> Font::getGlyphs(const char* substr, const char* end, const Vec2 origin) {
    std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> result;
    Rect bounds(origin,getSize(substr,end));
    getGlyphs(result, substr, end, origin, bounds, 0);
    return result;
}

/**
 * Returns a set of glyph runs to render the given string
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
 *
 * If a character in the string is not represented by a glyph in
 * the atlas collection, then it will be skipped unless the value
 * {@link #setFallbackAtlas} is set to true. In that case, this method
 * will generate a one-time atlas for these characters and return its
 * texture as part of the set. However, this atlas will not be stored
 * for future use. In addition, forcing this creation of a fallback
 * atlas makes this method no longer safe to be used outside of the
 * main thread (this is not an issue if {@link #getFallbackAtlas} is
 * false). Note that control characters (e.g. newlines) have no glyphs.
 *
 * The origin value determines the position of the bottom of the text
 * baseline. You should use the methods {@link #getDescent} and
 * {@link #getAscent} to place either the bottom or top of the text,
 * respectively.
 *
 * The quad sequence is adjusted so that all of the vertices fit in the
 * provided rectangle. The primary use-case for this is to guarantee
 * that glyphs do not spill outside of a window. This may mean that some
 * of the glyphs will be truncrated or even omitted.
 *
 * The tracking width is used to justify text in multi-line formats. If
 * track is positive, the spacing between the glyphs will be adjusted to
 * fit that width exactly. Once again, this may result in glyphs being
 * truncated if either the track width is greater then the rectangle width,
 * or if the font shrink limit is insufficent.
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically.
 *
 * @param text      The string for glyph generation
 * @param origin    The position of the first character
 * @param rect      The bounding box for the quads
 * @param track     The tracking width (if positive)
 *
 * @return a set of glyph runs to render the given string
 */
std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> Font::getGlyphs(const std::string text, const Vec2 origin, const Rect rect, float track) {
    std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> result;
    const char* begin = text.c_str();
    const char* end = begin+text.size();
    getGlyphs(result, begin, end, origin, rect, track);
    return result;
}

/**
 * Returns a set of glyph runs to render the given string
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
 *
 * If a character in the string is not represented by a glyph in
 * the atlas collection, then it will be skipped unless the value
 * {@link #setFallbackAtlas} is set to true. In that case, this method
 * will generate a one-time atlas for these characters and return its
 * texture as part of the set. However, this atlas will not be stored
 * for future use. In addition, forcing this creation of a fallback
 * atlas makes this method no longer safe to be used outside of the
 * main thread (this is not an issue if {@link #getFallbackAtlas} is
 * false). Note that control characters (e.g. newlines) have no glyphs.
 *
 * The origin value determines the position of the bottom of the text
 * baseline. You should use the methods {@link #getDescent} and
 * {@link #getAscent} to place either the bottom or top of the text,
 * respectively.
 *
 * The quad sequence is adjusted so that all of the vertices fit in the
 * provided rectangle. The primary use-case for this is to guarantee
 * that glyphs do not spill outside of a window. This may mean that some
 * of the glyphs will be truncrated or even omitted.
 *
 * The tracking width is used to justify text in multi-line formats. If
 * track is positive, the spacing between the glyphs will be adjusted to
 * fit that width exactly. Once again, this may result in glyphs being
 * truncated if either the track width is greater then the rectangle width,
 * or if the font shrink limit is insufficent.
 *
 * The C-style string substr need not be null-terminated. Instead,
 * the termination is indicated by the parameter end. This provides
 * efficient substring processing. The string may either be in UTF8
 * or ASCII; the method will handle conversion automatically.
 *
 * @param substr    The start of the string for glyph generation
 * @param end       The end of the string for glyph generation
 * @param origin    The position of the first character
 * @param rect      The bounding box for the quads
 * @param track     The tracking width (if positive)
 *
 * @return a set of glyph runs to render the given string
 */
std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> Font::getGlyphs(const char* substr, const char* end, const Vec2 origin,
                                                                      const Rect rect, float track) {
    std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> result;
    getGlyphs(result, substr, end, origin, rect, track);
    return result;
}

/**
 * Stores the glyph runs to render the given string in the given map
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
 * If a character in the string is not represented by a glyph in
 * the atlas collection, then it will be skipped unless the value
 * {@link #setFallbackAtlas} is set to true. In that case, this method
 * will generate a one-time atlas for these characters and return its
 * texture as part of the set. However, this atlas will not be stored
 * for future use. In addition, forcing this creation of a fallback
 * atlas makes this method no longer safe to be used outside of the
 * main thread (this is not an issue if {@link #getFallbackAtlas} is
 * false).
 *
 * The origin value determines the position of the bottom of the text
 * baseline. You should use the methods {@link #getDescent} and
 * {@link #getAscent} to place either the bottom or top of the text,
 * respectively.
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically.
 *
 * @param runs      The map to store the glyph runs
 * @param text      The string to convert to render data.
 * @param origin    The position of the first character
 *
 * @return the number of glyphs successfully processed
 */
size_t Font::getGlyphs(std::unordered_map<GLuint, std::shared_ptr<GlyphRun>>& runs, const std::string text, const Vec2 origin) {
    const char* begin = text.c_str();
    const char* end = begin+text.size();
    Rect bounds(origin,getSize(begin, end));
    return getGlyphs(runs, begin, end, origin, bounds, 0);
}

/**
 * Stores the glyph runs to render the given string in the given map
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
 * If a character in the string is not represented by a glyph in
 * the atlas collection, then it will be skipped unless the value
 * {@link #setFallbackAtlas} is set to true. In that case, this method
 * will generate a one-time atlas for these characters and return its
 * texture as part of the set. However, this atlas will not be stored
 * for future use. In addition, forcing this creation of a fallback
 * atlas makes this method no longer safe to be used outside of the
 * main thread (this is not an issue if {@link #getFallbackAtlas} is
 * false). Note that control characters (e.g. newlines) have no glyphs.
 *
 * The origin value determines the position of the bottom of the text
 * baseline. You should use the methods {@link #getDescent} and
 * {@link #getAscent} to place either the bottom or top of the text,
 * respectively.
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically.
 *
 * @param runs      The map to store the glyph runs
 * @param substr    The start of the string for glyph generation
 * @param end       The end of the string for glyph generation
 * @param origin    The position of the first character
 *
 * @return the number of glyphs successfully processed
 */
size_t Font::getGlyphs(std::unordered_map<GLuint, std::shared_ptr<GlyphRun>>& runs, const char* substr, const char* end, const Vec2 origin) {
    Rect bounds(origin,getSize(substr, end));
    return getGlyphs(runs, substr, end, origin, bounds, 0);
}

/**
 * Stores the glyph runs to render the given string in the given map
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
 * If a character in the string is not represented by a glyph in
 * the atlas collection, then it will be skipped unless the value
 * {@link #setFallbackAtlas} is set to true. In that case, this method
 * will generate a one-time atlas for these characters and return its
 * texture as part of the set. However, this atlas will not be stored
 * for future use. In addition, forcing this creation of a fallback
 * atlas makes this method no longer safe to be used outside of the
 * main thread (this is not an issue if {@link #getFallbackAtlas} is
 * false). Note that control characters (e.g. newlines) have no glyphs.
 *
 * The origin value determines the position of the bottom of the text
 * baseline. You should use the methods {@link #getDescent} and
 * {@link #getAscent} to place either the bottom or top of the text,
 * respectively.
 *
 * The quad sequence is adjusted so that all of the vertices fit in the
 * provided rectangle. The primary use-case for this is to guarantee
 * that glyphs do not spill outside of a window. This may mean that some
 * of the glyphs will be truncrated or even omitted.
 *
 * The tracking width is used to justify text in multi-line formats. If
 * track is positive, the spacing between the glyphs will be adjusted to
 * fit that width exactly. Once again, this may result in glyphs being
 * truncated if either the track width is greater then the rectangle width,
 * or if the font shrink limit is insufficent.
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically.
 *
 * @param runs      The map to store the glyph runs
 * @param text      The string for glyph generation
 * @param origin    The position of the first character
 * @param rect      The bounding box for the quads
 * @param track     The tracking width (if positive)
 *
 * @return the number of glyphs successfully processed
 */
size_t Font::getGlyphs(std::unordered_map<GLuint, std::shared_ptr<GlyphRun>>& runs, const std::string text, const Vec2 origin,
                       const Rect rect, float track) {
    const char* begin = text.c_str();
    const char* end = begin+text.size();
    return getGlyphs(runs, begin, end, origin, rect, track);
}

/**
 * Stores the glyph runs to render the given string in the given map
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
 * If a character in the string is not represented by a glyph in
 * the atlas collection, then it will be skipped unless the value
 * {@link #setFallbackAtlas} is set to true. In that case, this method
 * will generate a one-time atlas for these characters and return its
 * texture as part of the set. However, this atlas will not be stored
 * for future use. In addition, forcing this creation of a fallback
 * atlas makes this method no longer safe to be used outside of the
 * main thread (this is not an issue if {@link #getFallbackAtlas} is
 * false). Note that control characters (e.g. newlines) have no glyphs.
 *
 * The origin value determines the position of the bottom of the text
 * baseline. You should use the methods {@link #getDescent} and
 * {@link #getAscent} to place either the bottom or top of the text,
 * respectively.
 *
 * The quad sequence is adjusted so that all of the vertices fit in the
 * provided rectangle. The primary use-case for this is to guarantee
 * that glyphs do not spill outside of a window. This may mean that some
 * of the glyphs will be truncrated or even omitted.
 *
 * The tracking width is used to justify text in multi-line formats. If
 * track is positive, the spacing between the glyphs will be adjusted to
 * fit that width exactly. Once again, this may result in glyphs being
 * truncated if either the track width is greater then the rectangle width,
 * or if the font shrink limit is insufficent.
 *
 * The C-style string substr need not be null-terminated. Instead,
 * the termination is indicated by the parameter end. This provides
 * efficient substring processing. The string may either be in UTF8
 * or ASCII; the method will handle conversion automatically.
 *
 * @param runs      The map to store the glyph runs
 * @param substr    The start of the string for glyph generation
 * @param end       The end of the string for glyph generation
 * @param origin    The position of the first character
 * @param rect      The bounding box for the quads
 * @param track     The tracking width (if positive)
 *
 * @return the number of glyphs successfully processed
 */
size_t Font::getGlyphs(std::unordered_map<GLuint, std::shared_ptr<GlyphRun>>& runs, const char* substr, const char* end,
                       const Vec2 origin, const Rect rect, float track) {
    Rect bounds = rect;
    bounds.origin.x -= _atlasPadding;
    bounds.origin.y -= _atlasPadding;
    bounds.size.width  += 2*_atlasPadding;
    bounds.size.height += 2*_atlasPadding;

    Vec2 offset = origin;
    const char* begin = substr;
    const char* check = substr;

    CUAssertLog(utf8::find_invalid(check, end) == end, "String '%s' has an invalid UTF-8 encoding",begin);
    std::vector<Sint32> adjusts;
    if (track > 0) {
        adjusts = getTracking(substr, end, track);
    }
    size_t total = 0;
    if (_fallback) {
        // See which any characters are missing
        std::vector<Uint32> missing;
        while (begin != end) {
            Uint32 thechar = utf8::next(begin,end);
            if (_atlasmap.find(thechar) == _atlasmap.end()) {
                missing.push_back(thechar);
            }
        }
        std::unordered_map<Uint32, size_t> localmap;
        std::vector<std::shared_ptr<Atlas>> locals;
        if (missing.size() > 0) {
            buildLocalAtlases(missing,locals,localmap);
        }

        begin = substr;
        Uint32 prvchar = 0;
        Uint32 pos = 0;
        while (begin != end) {
            Uint32 thechar = utf8::next(begin,end);
            if (prvchar > 0) {
                offset.x -= _kernmap[prvchar][thechar];
                if (track > 0 && pos < adjusts.size()) {
                    offset.x += adjusts[pos++];
                }
            }
            prvchar = thechar;

            std::shared_ptr<GlyphRun> grun;
            std::shared_ptr<Atlas> atlas;

            bool found = false;
            if (_atlasmap.find(thechar) != _atlasmap.end()) {
                size_t index = _atlasmap[thechar];
                atlas = _atlases[index];
                GLuint key = atlas->texture->getBuffer();
                auto find = runs.find(key);
                if (find == runs.end()) {
                    grun = GlyphRun::alloc();
                    grun->texture = atlas->texture;
                    runs[key] = grun;
                } else {
                    grun = find->second;
                }
                found = true;
            } else if (localmap.find(thechar) != localmap.end()) {
                size_t index = localmap[thechar];
                atlas = locals[index];
                GLuint key = atlas->texture->getBuffer();
                auto find = runs.find(key);
                if (find == runs.end()) {
                    grun = GlyphRun::alloc();
                    grun->texture = atlas->texture;
                    runs[key] = grun;
                } else {
                    grun = find->second;
                }
                found = true;
            }
     
            if (found && atlas->getQuad(thechar,offset,grun->mesh,bounds)) {
                grun->contents.emplace(thechar);
                total++;
            }
        }
    } else {
        begin = substr;
        Uint32 prvchar = 0;
        Uint32 pos = 0;
        while (begin != end) {
            Uint32 thechar = utf8::next(begin,end);
            if (prvchar > 0) {
                offset.x -= _kernmap[prvchar][thechar];
                if (track > 0 && pos < adjusts.size()) {
                    offset.x += adjusts[pos++];
                }
            }
            prvchar = thechar;
            
            std::shared_ptr<GlyphRun> grun;
            std::shared_ptr<Atlas> atlas;

            bool found = false;
            if (_atlasmap.find(thechar) != _atlasmap.end()) {
                size_t index = _atlasmap[thechar];
                atlas = _atlases[index];
                GLuint key = atlas->texture->getBuffer();
                auto find = runs.find(key);
                if (find == runs.end()) {
                    grun = GlyphRun::alloc();
                    grun->texture = atlas->texture;
                    runs[key] = grun;
                } else {
                    grun = find->second;
                }
                found = true;
            }
     
            if (found && atlas->getQuad(thechar,offset,grun->mesh,bounds)) {
                grun->contents.emplace(thechar);
                total++;
            }
        }
    }

    return total;
}

/**
 * Returns a single glyph run quad to render this character.
 *
 * The glyph run will consist of a single quad and the texture to render
 * a quad. If the character is not represented by a glyph in the atlas
 * collection, the glyph run will be empty unless {@link #setFallbackAtlas}
 * is set to true. In that case, this method will generate a one-time
 * atlas for this character and return its texture as part of the glyph
 * run. However, this atlas will not be stored for future use. In addition,
 * forcing this creation of a fallback atlas makes this method no longer
 * safe to be used outside of the main thread (this is not an issue if
 * {@link #getFallbackAtlas} is false). Note that control characters (e.g.
 * newlines) have no glyphs.
 *
 * This method will append the vertices to the provided mesh and update
 * the indices to include these new vertices. In addition, it will return
 * the texture that should be used with these vertices. Finally, once the
 * glyph is generated, the offset will be adjusted to contain the next
 * place to render a character.
 *
 * The offset determines the position of the bottom of the text baseline.
 * You should use the methods {@link #getDescent} and {@link #getAscent}
 * to place either the bottom or top of the text, respectively.
 *
 * The character should be represented by a UNICODE value. For ASCII
 * characters, this agrees with the ASCII code.
 *
 * @param thechar   The character to convert to render data
 * @param offset    The (unkerned) starting position of the quad
 *
 * @return a single glyph run quad to render this character.
 */
std::shared_ptr<GlyphRun> Font::getGlyph(Uint32 thechar, Vec2& offset) {
    std::shared_ptr<GlyphRun> grun = nullptr;
    if (_atlasmap.find(thechar) != _atlasmap.end()) {
        std::shared_ptr<Atlas> atlas = _atlases[_atlasmap[thechar]];
        grun = GlyphRun::alloc();
        grun->texture = atlas->texture;
        atlas->getQuad(thechar, offset, grun->mesh);
    } else if (_fallback) {
        std::vector<Uint32> charset;
        charset.push_back(thechar);
        std::deque<Uint32> glyphs = gatherGlyphs(charset);
        gatherKerning(glyphs);
        std::shared_ptr<Atlas> atlas = Atlas::alloc(this, glyphs);
        grun = GlyphRun::alloc();
        grun->texture = atlas->texture;
        atlas->getQuad(thechar, offset, grun->mesh);
    }
    
    return grun;
}

/**
 * Returns a single glyph run quad to render this character.
 *
 * The glyph run will consist of a single quad and the texture to render
 * a quad. If the character is not represented by a glyph in the atlas
 * collection, the glyph run will be empty unless {@link #setFallbackAtlas}
 * is set to true. In that case, this method will generate a one-time
 * atlas for this character and return its texture as part of the glyph
 * run. However, this atlas will not be stored for future use. In addition,
 * forcing this creation of a fallback atlas makes this method no longer
 * safe to be used outside of the main thread (this is not an issue if
 * {@link #getFallbackAtlas} is false). Note that control characters (e.g.
 * newlines) have no glyphs.
 *
 * This method will append the vertices to the provided mesh and update
 * the indices to include these new vertices. In addition, it will return
 * the texture that should be used with these vertices. Finally, once the
 * glyph is generated, the offset will be adjusted to contain the next
 * place to render a character.
 *
 * The offset determines the position of the bottom of the text baseline.
 * You should use the methods {@link #getDescent} and {@link #getAscent}
 * to place either the bottom or top of the text, respectively.
 *
 * The quad is adjusted so that all of the vertices fit in the provided
 * rectangle.  This may mean that no quad is generated at all.
 *
 * The character should be represented by a UNICODE value. For ASCII
 * characters, this agrees with the ASCII code.
 *
 * @param thechar   The character to convert to render data
 * @param offset    The (unkerned) starting position of the quad
 * @param rect      The bounding box for the quad
 *
 * @return a single glyph run quad to render this character.
 */
std::shared_ptr<GlyphRun> Font::getGlyph(Uint32 thechar, Vec2& offset, const Rect rect) {
    std::shared_ptr<GlyphRun> grun = nullptr;
    if (_atlasmap.find(thechar) != _atlasmap.end()) {
        std::shared_ptr<Atlas> atlas = _atlases[_atlasmap[thechar]];
        grun = GlyphRun::alloc();
        grun->texture = atlas->texture;
        atlas->getQuad(thechar, offset, grun->mesh, rect);
    } else if (_fallback) {
        std::vector<Uint32> charset;
        charset.push_back(thechar);
        std::deque<Uint32> glyphs = gatherGlyphs(charset);
        gatherKerning(glyphs);
        std::shared_ptr<Atlas> atlas = Atlas::alloc(this, glyphs);
        grun = GlyphRun::alloc();
        grun->texture = atlas->texture;
        atlas->getQuad(thechar, offset, grun->mesh, rect);
    }
    
    return grun;
}

#pragma mark -
#pragma mark Glyph Debugging
/**
 * Returns a (line) mesh of the quad outlines for the text glyphs.
 *
 * This method is useful for debugging. When this mesh is drawn together
 * with a glyph run sequence, it shows the bounding box for each glyph.
 * However, these bounding boxes are determined by the glyph metrics,
 * and do not take into account atlas padding. So they do not represent
 * potential overlaps when the padding is non-zero.
 *
 * If a character in the string is not represented by a glyph in
 * the atlas collection, then it will be skipped unless the value
 * {@link #setFallbackAtlas} is set to true. In that case, this method
 * will generate a one-time atlas for these characters and return its
 * texture as part of the set. However, this atlas will not be stored
 * for future use. In addition, forcing this creation of a fallback
 * atlas makes this method no longer safe to be used outside of the
 * main thread (this is not an issue if {@link #getFallbackAtlas} is
 * false). Note that control characters (e.g. newlines) have no glyphs.
 *
 * The origin value determines the position of the bottom of the text
 * baseline. You should use the methods {@link #getDescent} and
 * {@link #getAscent} to place either the bottom or top of the text,
 * respectively.
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically.
 *
 * @param text      The string for glyph generation
 * @param origin    The position of the first character
 *
 * @return a (line) mesh of the quad outlines for the text glyphs
 */
Mesh<SpriteVertex2> Font::getGlyphBoxes(const std::string text, const Vec2 origin) {
    const char* begin = text.c_str();
    const char* end = begin+text.size();
    Rect bounds(origin,getSize(begin,end));
    return getGlyphBoxes(begin,end,origin,bounds,0);
}

/**
 * Returns a (line) mesh of the quad outlines for the text glyphs.
 *
 * This method is useful for debugging. When this mesh is drawn together
 * with a glyph run sequence, it shows the bounding box for each glyph.
 * However, these bounding boxes are determined by the glyph metrics,
 * and do not take into account atlas padding. So they do not represent
 * potential overlaps when the padding is non-zero.
 *
 * If a character in the string is not represented by a glyph in
 * the atlas collection, then it will be skipped unless the value
 * {@link #setFallbackAtlas} is set to true. In that case, this method
 * will generate a one-time atlas for these characters and return its
 * texture as part of the set. However, this atlas will not be stored
 * for future use. In addition, forcing this creation of a fallback
 * atlas makes this method no longer safe to be used outside of the
 * main thread (this is not an issue if {@link #getFallbackAtlas} is
 * false). Note that control characters (e.g. newlines) have no glyphs.
 *
 * The origin value determines the position of the bottom of the text
 * baseline. You should use the methods {@link #getDescent} and
 * {@link #getAscent} to place either the bottom or top of the text,
 * respectively.
 *
 * The origin value determines the position of the bottom of the text
 * baseline. You should use the methods {@link #getDescent} and
 * {@link #getAscent} to place either the bottom or top of the text,
 * respectively.
 *
 * The C-style string substr need not be null-terminated. Instead,
 * the termination is indicated by the parameter end. This provides
 * efficient substring processing. The string may either be in UTF8
 * or ASCII; the method will handle conversion automatically.
 *
 * @param substr    The start of the string for glyph generation
 * @param end       The end of the string for glyph generation
 * @param origin    The position of the first character
 *
 * @return a (line) mesh of the quad outlines for the text glyphs
 */
Mesh<SpriteVertex2> Font::getGlyphBoxes(const char* substr, const char* end, const Vec2 origin) {
    Rect bounds(origin,getSize(substr,end));
    return getGlyphBoxes(substr,end,origin,bounds,0);
}

/**
 * Returns a (line) mesh of the quad outlines for the text glyphs.
 *
 * This method is useful for debugging. When this mesh is drawn together
 * with a glyph run sequence, it shows the bounding box for each glyph.
 * However, these bounding boxes are determined by the glyph metrics,
 * and do not take into account atlas padding. So they do not represent
 * potential overlaps when the padding is non-zero.
 *
 * If a character in the string is not represented by a glyph in
 * the atlas collection, then it will be skipped unless the value
 * {@link #setFallbackAtlas} is set to true. In that case, this method
 * will generate a one-time atlas for these characters and return its
 * texture as part of the set. However, this atlas will not be stored
 * for future use. In addition, forcing this creation of a fallback
 * atlas makes this method no longer safe to be used outside of the
 * main thread (this is not an issue if {@link #getFallbackAtlas} is
 * false). Note that control characters (e.g. newlines) have no glyphs.
 *
 * The origin value determines the position of the bottom of the text
 * baseline. You should use the methods {@link #getDescent} and
 * {@link #getAscent} to place either the bottom or top of the text,
 * respectively.
 *
 * The mesh sequence is adjusted so that all of the vertices fit in the
 * provided rectangle. The primary use-case for this is to guarantee
 * that glyph boxes do not spill outside of a window. This may mean that
 * some of the boxes will be truncrated or even omitted.
 *
 * The tracking width is used to justify text in multi-line formats. If
 * track is positive, the spacing between the boxes will be adjusted to
 * fit that width exactly. Once again, this may result in boxes being
 * truncated if either the track width is greater then the rectangle
 * width, or if the font shrink limit is insufficent.
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically.
 *
 * @param text      The string to convert to render data.
 * @param origin    The position of the first character
 * @param rect      The bounding box for the mesh
 * @param track     The tracking width (if positive)
 *
 * @return a (line) mesh of the quad outlines for the text glyphs
 */
Mesh<SpriteVertex2> Font::getGlyphBoxes(const std::string text, const Vec2 origin,
                                        const Rect rect, float track) {
    const char* begin = text.c_str();
    const char* end = begin+text.size();
    return getGlyphBoxes(begin,end,origin,rect,track);
}

/**
 * Returns a (line) mesh of the quad outlines for the text glyphs.
 *
 * This method is useful for debugging. When this mesh is drawn together
 * with a glyph run sequence, it shows the bounding box for each glyph.
 * However, these bounding boxes are determined by the glyph metrics,
 * and do not take into account atlas padding. So they do not represent
 * potential overlaps when the padding is non-zero.
 *
 * If a character in the string is not represented by a glyph in
 * the atlas collection, then it will be skipped unless the value
 * {@link #setFallbackAtlas} is set to true. In that case, this method
 * will generate a one-time atlas for these characters and return its
 * texture as part of the set. However, this atlas will not be stored
 * for future use. In addition, forcing this creation of a fallback
 * atlas makes this method no longer safe to be used outside of the
 * main thread (this is not an issue if {@link #getFallbackAtlas} is
 * false). Note that control characters (e.g. newlines) have no glyphs.
 *
 * The origin value determines the position of the bottom of the text
 * baseline. You should use the methods {@link #getDescent} and
 * {@link #getAscent} to place either the bottom or top of the text,
 * respectively.
 *
 * The mesh sequence is adjusted so that all of the vertices fit in the
 * provided rectangle. The primary use-case for this is to guarantee
 * that glyph boxes do not spill outside of a window. This may mean that
 * some of the boxes will be truncrated or even omitted.
 *
 * The tracking width is used to justify text in multi-line formats. If
 * track is positive, the spacing between the boxes will be adjusted to
 * fit that width exactly. Once again, this may result in boxes being
 * truncated if either the track width is greater then the rectangle
 * width, or if the font shrink limit is insufficent.
 *
 * The C-style string substr need not be null-terminated. Instead,
 * the termination is indicated by the parameter end. This provides
 * efficient substring processing. The string may either be in UTF8
 * or ASCII; the method will handle conversion automatically.
 *
 * @param substr    The start of the string for glyph generation
 * @param end       The end of the string for glyph generation
 * @param origin    The position of the first character
 * @param rect      The bounding box for the mesh
 * @param track     The tracking width (if positive)
 *
 * @return a (line) mesh of the quad outlines for the text glyphs
 */
Mesh<SpriteVertex2> Font::getGlyphBoxes(const char* substr, const char* end,
                                        const Vec2 origin, const Rect rect, float track) {
    Mesh<SpriteVertex2> mesh;
    mesh.command = GL_LINES;
    getGlyphBoxes(mesh, substr, end, origin, rect, track);
    return mesh;
}

/**
 * Stores the quad outlines for the text glyphs in the given mesh.
 *
 * This method is useful for debugging. When this mesh is drawn together
 * with a glyph run sequence, it shows the bounding box for each glyph.
 * However, these bounding boxes are determined by the glyph metrics,
 * and do not take into account atlas padding. So they do not represent
 * potential overlaps when the padding is non-zero.
 *
 * If a character in the string is not represented by a glyph in
 * the atlas collection, then it will be skipped unless the value
 * {@link #setFallbackAtlas} is set to true. In that case, this method
 * will generate a one-time atlas for these characters and return its
 * texture as part of the set. However, this atlas will not be stored
 * for future use. In addition, forcing this creation of a fallback
 * atlas makes this method no longer safe to be used outside of the
 * main thread (this is not an issue if {@link #getFallbackAtlas} is
 * false). Note that control characters (e.g. newlines) have no glyphs.
 *
 * The origin value determines the position of the bottom of the text
 * baseline. You should use the methods {@link #getDescent} and
 * {@link #getAscent} to place either the bottom or top of the text,
 * respectively.
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically.
 *
 * @param mesh      The mesh to store the new quads
 * @param text      The string for glyph generation
 * @param origin    The position of the first character
 *
 * @return the number of quads generated
 */
size_t Font::getGlyphBoxes(Mesh<SpriteVertex2>& mesh, const std::string text, const Vec2 origin) {
    const char* begin = text.c_str();
    const char* end = begin+text.size();
    Rect bounds(origin,getSize(begin,end));
    return getGlyphBoxes(mesh, begin, end, origin, bounds, 0);
}

/**
 * Stores the quad outlines for the text glyphs in the given mesh.
 *
 * This method is useful for debugging. When this mesh is drawn together
 * with a glyph run sequence, it shows the bounding box for each glyph.
 * However, these bounding boxes are determined by the glyph metrics,
 * and do not take into account atlas padding. So they do not represent
 * potential overlaps when the padding is non-zero.
 *
 * If a character in the string is not represented by a glyph in
 * the atlas collection, then it will be skipped unless the value
 * {@link #setFallbackAtlas} is set to true. In that case, this method
 * will generate a one-time atlas for these characters and return its
 * texture as part of the set. However, this atlas will not be stored
 * for future use. In addition, forcing this creation of a fallback
 * atlas makes this method no longer safe to be used outside of the
 * main thread (this is not an issue if {@link #getFallbackAtlas} is
 * false). Note that control characters (e.g. newlines) have no glyphs.
 *
 * The origin value determines the position of the bottom of the text
 * baseline. You should use the methods {@link #getDescent} and
 * {@link #getAscent} to place either the bottom or top of the text,
 * respectively.
 *
 * The origin value determines the position of the bottom of the text
 * baseline. You should use the methods {@link #getDescent} and
 * {@link #getAscent} to place either the bottom or top of the text,
 * respectively.
 *
 * The C-style string substr need not be null-terminated. Instead,
 * the termination is indicated by the parameter end. This provides
 * efficient substring processing. The string may either be in UTF8
 * or ASCII; the method will handle conversion automatically.
 *
 * @param mesh      The mesh to store the new quads
 * @param substr    The start of the string for glyph generation
 * @param end       The end of the string for glyph generation
 * @param origin    The position of the first character
 *
 * @return the number of quads generated
 */
size_t Font::getGlyphBoxes(Mesh<SpriteVertex2>& mesh, const char* substr, const char* end, const Vec2 origin) {
    Rect bounds(origin,getSize(substr,end));
    return getGlyphBoxes(mesh, substr, end, origin, bounds, 0);
}

/**
 * Stores the quad outlines for the text glyphs in the given mesh.
 *
 * This method is useful for debugging. When this mesh is drawn together
 * with a glyph run sequence, it shows the bounding box for each glyph.
 * However, these bounding boxes are determined by the glyph metrics,
 * and do not take into account atlas padding. So they do not represent
 * potential overlaps when the padding is non-zero.
 *
 * If a character in the string is not represented by a glyph in
 * the atlas collection, then it will be skipped unless the value
 * {@link #setFallbackAtlas} is set to true. In that case, this method
 * will generate a one-time atlas for these characters and return its
 * texture as part of the set. However, this atlas will not be stored
 * for future use. In addition, forcing this creation of a fallback
 * atlas makes this method no longer safe to be used outside of the
 * main thread (this is not an issue if {@link #getFallbackAtlas} is
 * false). Note that control characters (e.g. newlines) have no glyphs.
 *
 * The origin value determines the position of the bottom of the text
 * baseline. You should use the methods {@link #getDescent} and
 * {@link #getAscent} to place either the bottom or top of the text,
 * respectively.
 *
 * The mesh sequence is adjusted so that all of the vertices fit in the
 * provided rectangle. The primary use-case for this is to guarantee
 * that glyph boxes do not spill outside of a window. This may mean that
 * some of the boxes will be truncrated or even omitted.
 *
 * The tracking width is used to justify text in multi-line formats. If
 * track is positive, the spacing between the boxes will be adjusted to
 * fit that width exactly. Once again, this may result in boxes being
 * truncated if either the track width is greater then the rectangle
 * width, or if the font shrink limit is insufficent.
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically.
 *
 * @param mesh      The mesh to store the new quads
 * @param text      The string to convert to render data.
 * @param origin    The position of the first character
 * @param rect      The bounding box for the mesh
 * @param track     The tracking width (if positive)
 *
 * @return the number of quads generated
 */
size_t Font::getGlyphBoxes(Mesh<SpriteVertex2>& mesh, const std::string text,
                           const Vec2 origin, const Rect rect, float track) {
    const char* begin = text.c_str();
    const char* end = begin+text.size();
    return getGlyphBoxes(mesh, begin, end, origin, rect, track);
}

/**
 * Stores the quad outlines for the text glyphs in the given mesh.
 *
 * This method is useful for debugging. When this mesh is drawn together
 * with a glyph run sequence, it shows the bounding box for each glyph.
 * However, these bounding boxes are determined by the glyph metrics,
 * and do not take into account atlas padding. So they do not represent
 * potential overlaps when the padding is non-zero.
 *
 * If a character in the string is not represented by a glyph in
 * the atlas collection, then it will be skipped unless the value
 * {@link #setFallbackAtlas} is set to true. In that case, this method
 * will generate a one-time atlas for these characters and return its
 * texture as part of the set. However, this atlas will not be stored
 * for future use. In addition, forcing this creation of a fallback
 * atlas makes this method no longer safe to be used outside of the
 * main thread (this is not an issue if {@link #getFallbackAtlas} is
 * false). Note that control characters (e.g. newlines) have no glyphs.
 *
 * The origin value determines the position of the bottom of the text
 * baseline. You should use the methods {@link #getDescent} and
 * {@link #getAscent} to place either the bottom or top of the text,
 * respectively.
 *
 * The mesh sequence is adjusted so that all of the vertices fit in the
 * provided rectangle. The primary use-case for this is to guarantee
 * that glyph boxes do not spill outside of a window. This may mean that
 * some of the boxes will be truncrated or even omitted.
 *
 * The tracking width is used to justify text in multi-line formats. If
 * track is positive, the spacing between the boxes will be adjusted to
 * fit that width exactly. Once again, this may result in boxes being
 * truncated if either the track width is greater then the rectangle
 * width, or if the font shrink limit is insufficent.
 *
 * The C-style string substr need not be null-terminated. Instead,
 * the termination is indicated by the parameter end. This provides
 * efficient substring processing. The string may either be in UTF8
 * or ASCII; the method will handle conversion automatically.
 *
 * @param mesh      The mesh to store the new quads
 * @param substr    The start of the string for glyph generation
 * @param end       The end of the string for glyph generation
 * @param origin    The position of the first character
 * @param rect      The bounding box for the mesh
 * @param track     The tracking width (if positive)
 *
 * @return the number of quads generated
 */
size_t Font::getGlyphBoxes(Mesh<SpriteVertex2>& mesh, const char* substr, const char* end,
                           const Vec2 origin, const Rect rect, float track) {
    Rect bounds = rect;
    bounds.origin.x -= _atlasPadding;
    bounds.origin.y -= _atlasPadding;
    bounds.size.width  += 2*_atlasPadding;
    bounds.size.height += 2*_atlasPadding;

    Vec2 offset = origin;
    const char* begin = substr;
    const char* check = substr;

    CUAssertLog(utf8::find_invalid(check, end) == end, "String '%s' has an invalid UTF-8 encoding",begin);
    std::vector<Sint32> adjusts;
    if (track > 0) {
        adjusts = getTracking(substr, end, track);
    }
    
    size_t total = 0;
    Uint32 prvchar = 0;
    Uint32 pos = 0;
    while (begin != end) {
        Uint32 thechar = utf8::next(begin,end);
        if (_glyphsize.find(thechar) != _glyphsize.end()) {
            if (prvchar > 0) {
                offset.x -= _kernmap[prvchar][thechar];
                if (track > 0 && pos < adjusts.size()) {
                    offset.x += adjusts[pos++];
                }
            }
            getOutline(prvchar,offset,mesh,rect);
            total++;
        } else if (_fallback && hasGlyph(thechar)) {
            if (prvchar > 0) {
                offset.x -= computeKerning(prvchar,thechar);
                if (track > 0 && pos < adjusts.size()) {
                    offset.x += adjusts[pos++];
                }
            }
            getOutline(prvchar,offset,mesh,rect);
            total++;
        }
    }
    return total;
}


#pragma mark -
#pragma mark Internals
/**
 * Gathers glyph size information for the ASCII characters
 *
 * This method only gathers glyph size information if it was not
 * gathered previously (e.g. those characters are not in an existing
 * atlas). The returned deque stores only those characters that were
 * processed. This allows the atlas creation tools to determine which
 * glyphs are new, and not previously supported.
 *
 * @return the characters successfully processed
 */
std::deque<Uint32> Font::gatherGlyphs() {
    std::deque<Uint32> added;
    for(Uint32 ii = 32; ii < 127; ii++) {
        if (_atlasmap.find(ii) == _atlasmap.end() && TTF_GlyphIsProvided(_data, ii)) {
            Metrics metrics = computeMetrics(ii);
            _glyphsize.emplace(ii,metrics);
            added.push_back(ii);
        }
    }

    // Tabs for good measure
    if ((_atlasmap.find(TAB_CHAR) == _atlasmap.end()) && (_atlasmap.find(SPACE_CHAR) != _atlasmap.end())) {
        Metrics metrics = computeMetrics(TAB_CHAR);
        _glyphsize.emplace(TAB_CHAR,metrics);
        // NOT added to atlas, since space is a proxy
    }
    
    // Sort them by width
    std::sort(added.begin(),added.end(),[&](Uint32 a, Uint32 b) {
        int aad = _glyphsize[a].advance;
        int bad = _glyphsize[b].advance;
        return (aad > bad || (aad == bad && a > b));
    });
    
    return added;
}

/**
 * Gathers glyph size information for the character set
 *
 * This method only gathers glyph size information if it was not
 * gathered previously (e.g. those characters are not in an existing
 * atlas). The returned deque stores only those characters that were
 * processed. This allows the atlas creation tools to determine which
 * glyphs are new, and not previously supported.
 *
 * The character set may either be in UTF8 or ASCII; the method
 * will handle conversion automatically.
 *
 * @param charset   The characters to measure
 *
 * @return the characters successfully processed
 */
std::deque<Uint32> Font::gatherGlyphs(const std::string& charset) {
    std::deque<Uint32> added;
    const char* begin = charset.c_str();
    const char* end = begin+charset.size();
    while (begin != end) {
        Uint32 thechar = utf8::next(begin,end);
        if (thechar == TAB_CHAR && _atlasmap.find(thechar) == _atlasmap.end()) {
            if (_atlasmap.find(SPACE_CHAR) == _atlasmap.end() && TTF_GlyphIsProvided(_data, SPACE_CHAR)) {
                Metrics metrics = computeMetrics(SPACE_CHAR);
                _glyphsize.emplace(SPACE_CHAR,metrics);
                added.push_back(SPACE_CHAR);
            }
            Metrics metrics = computeMetrics(TAB_CHAR);
            _glyphsize.emplace(TAB_CHAR,metrics);
            // NOT added to atlas, since space is a proxy
        } else if (_atlasmap.find(thechar) == _atlasmap.end() && TTF_GlyphIsProvided(_data, thechar)) {
            Metrics metrics = computeMetrics(thechar);
            _glyphsize.emplace(thechar,metrics);
            added.push_back(thechar);
        }
    }
    
    // Sort them by width
    std::sort(added.begin(),added.end(),[&](Uint32 a, Uint32 b) {
        int aad = _glyphsize[a].advance;
        int bad = _glyphsize[b].advance;
        return (aad > bad || (aad == bad && a > b));
    });
    
    return added;
}

/**
 * Gathers glyph size information for the character set
 *
 * This method only gathers glyph size information if it was not
 * gathered previously (e.g. those characters are not in an existing
 * atlas). The returned deque stores only those characters that
 * were processed. This allows the atlas creation tools to determine
 * which glyphs are new, and not previously supported.
 *
 * The character set should be all UNICODE values.
 *
 * @param charset   The characters to measure
 *
 * @return the characters successfully processed
 */
std::deque<Uint32> Font::gatherGlyphs(const std::vector<Uint32>& charset) {
    std::deque<Uint32> added;
    for(auto it = charset.begin(); it != charset.end(); ++it) {
        if (*it == TAB_CHAR && _atlasmap.find(*it) == _atlasmap.end()) {
            if (_atlasmap.find(SPACE_CHAR) == _atlasmap.end() && TTF_GlyphIsProvided(_data, SPACE_CHAR)) {
                Metrics metrics = computeMetrics(SPACE_CHAR);
                _glyphsize.emplace(SPACE_CHAR,metrics);
                added.push_back(SPACE_CHAR);
            }
            Metrics metrics = computeMetrics(TAB_CHAR);
            _glyphsize.emplace(TAB_CHAR,metrics);
            // NOT added to atlas, since space is a proxy
        } else if (_atlasmap.find(*it) == _atlasmap.end() && TTF_GlyphIsProvided(_data, *it)) {
            Metrics metrics = computeMetrics(*it);
            _glyphsize.emplace(*it,metrics);
            added.push_back(*it);
        }
    }
    
    // Sort them by width
    std::sort(added.begin(),added.end(),[&](Uint32 a, Uint32 b) {
        int aad = _glyphsize[a].advance;
        int bad = _glyphsize[b].advance;
        return (aad > bad || (aad == bad && a > b));
    });
    
    return added;
}

/**
 * Gathers the kerning information for given characters.
 *
 * These characters will not only be kerned against each other, but
 * they will also be kerned against any existing characters.
 *
 * @param glyphs    The glyphs to acquire kerning data for
 */
void Font::gatherKerning(const std::deque<Uint32>& glyphs) {
    for(auto it = _glyphsize.begin(); it != _glyphsize.end(); ++it) {
        if (_kernmap.find(it->first) == _kernmap.end()) {
            _kernmap.emplace(it->first, std::unordered_map<Uint32, Uint32>());
        }
        for(auto jt = _glyphsize.begin(); jt != _glyphsize.end(); ++jt) {
            _kernmap[it->first].emplace(jt->first, computeKerning(it->first, jt->first));
        }
    }
}

/**
 * Returns the metrics for the given character if available.
 *
 * This method returns a metric with all zeroes if no data is fount.
 *
 * @return the metrics for the given character if available.
 */
Font::Metrics Font::computeMetrics(Uint32 thechar) const {
    if (thechar == TAB_CHAR) {
        Font::Metrics metrics = computeMetrics(SPACE_CHAR);
        if (metrics.maxx > 0) {
            metrics.maxx += (TAB_SPACE-1)*metrics.advance;
        }
        metrics.advance *= TAB_SPACE;
        return metrics;
    }
    
    Metrics metrics;
    int success = TTF_GlyphMetrics(_data, thechar, &metrics.minx, &metrics.maxx,
                                   &metrics.miny,  &metrics.maxy, &metrics.advance);
    
    // Only store if we have metrics
    if (success != -1) {
        // Fix because there is a render difference.
        Uint32 str[2];
        str[0] = thechar; str[1] = 0;
        
        int w = 0;
        int h = 0;
        TTF_SizeUNICODE(_data, str, &w, &h);
        if (w != metrics.advance) {
            int diff = w-metrics.advance;
            metrics.minx += diff/2; 
            metrics.maxx += diff/2;
            metrics.advance += diff;
        }
    }
    
    return metrics;
}

/**
 * Returns the kerning between the two characters if available.
 *
 * The method will return -1 if there either of the two characters are
 * not supported by this font.
 *
 * @return the kerning between the two characters if available.
 */
int Font::computeKerning(Uint32 a, Uint32 b) const {
    if (is_control(a) || is_control(b)) {
        return 0;
    }
    
    Uint32 str[3];
    str[0] = a;
    str[1] = b;
    str[2] = 0;

    int w1, w2;
    TTF_SizeUNICODE(_data, str, &w1, &w2);
    w2 =  (_glyphsize.find(a) != _glyphsize.end() ? _glyphsize.at(a).advance : computeMetrics(a).advance);
    w2 += (_glyphsize.find(b) != _glyphsize.end() ? _glyphsize.at(b).advance : computeMetrics(b).advance);
    
    return w2-w1;
}

/**
 * Creates an local atlas collection for the given character set.
 *
 * The atlases generated contains characters in the provided character set,
 * and will omit all other chacters. This includes ASCII characters that may
 * be missing from the character set. The character set string must either
 * be in ASCII or UTF8 encoding. It will handle both automatically, but no
 * other encoding (e.g. Latin1) is accepted.
 *
 * The atlas collection and its corresponding map are stored in the appropriate
 * reference variables. As a local collection, these atlases are not stored
 * they will be deleted once the shared pointers are released.  However, the
 * glyph metric and kerning information will be preserved for future use.
 *
 * WARNING: This method is not thread safe. It generates an OpenGL texture,
 * which means that it may only be called in the main thread.
 *
 * @param charset   The set of characters in the atlas
 * @param atlas       Vector to store the atlas collection
 * @param map       Map to store the atlas directory
 *
 * @return true if the atlases were successfully created.
 */
bool Font::buildLocalAtlases(const std::vector<Uint32>& charset,
                             std::vector<std::shared_ptr<Atlas>>& atlases,
                             std::unordered_map<Uint32, size_t>& map) {
    
    std::deque<Uint32> glyphs = gatherGlyphs(charset);
    gatherKerning(glyphs);
    bool success = true;
    while (success && glyphs.size() > 0) {
        std::shared_ptr<Atlas> atlas = Atlas::alloc(this, glyphs);
        if (atlas != nullptr) {
            success = atlas->build();
            success = success && atlas->materialize();
            if (success) {
                size_t pos = atlases.size();
                atlases.push_back(atlas);
                for(auto it = atlas->glyphmap.begin(); it != atlas->glyphmap.end(); ++it) {
                    map.emplace(it->first,pos);
                }
            }
        } else {
            success = false;
        }
    }
    
    return success;
}

/**
 * Creates a quad outline of this character and stores it in mesh
 *
 * This method will append the vertices to the provided mesh and update
 * the indices to include these new vertices. Once the quad is generated,
 * the offset will be adjusted to contain the next place to render a
 * character.
 *
 * The dimensionons of the quad are determined by the metrics. Hence this
 * is a bounding box of the glyph, but it does not align with the actual
 * vertices of a rendered glyph.  That is, this quad outline is guaranteed
 * to fit within the bounds of a rendered glyph, but not necessarly to
 * match it.
 *
 * The quad is adjusted so that all of the vertices fit in the provided
 * rectangle.  This may mean that no quad is generated at all.
 *
 * @param thechar   The character to convert to render data
 * @param offset    The (unkerned) starting position of the quad
 * @param mesh      The mesh to store the vertices
 */
bool Font::getOutline(Uint32 thechar, Vec2& offset, Mesh<SpriteVertex2>& mesh, const Rect rect) {
    CUAssertLog(mesh.command == GL_LINES, "The mesh is not formatted for lines");

    // Technically, this answer is correct
    if (!hasGlyph(thechar)) { return true; }
    
    Metrics metrics;
    if (_glyphsize.find(thechar) != _glyphsize.end()) {
        metrics = _glyphsize[thechar];
    } else {
        metrics = computeMetrics(thechar);
    }
    Rect quad(offset,Size(metrics.advance,metrics.maxy-metrics.miny));
    quad.origin.y += metrics.miny-_fontDescent;
    

    // Skip over glyph, but recognize we may have later glyphs
    if (!rect.doesIntersect(quad)) {
        offset.x += quad.size.width;
        return quad.getMaxX() <= rect.getMaxX();
    }
    
    // Compute intersection and adjust cookie cutter
    quad.intersect(rect);
    bool result = quad.getMaxX() <= rect.getMaxX();
    offset.x += metrics.advance;
    
    SpriteVertex2 temp;
    GLuint size = (GLuint)mesh.vertices.size();

    // Bottom left
    GLuint white = Color4::WHITE.getPacked();
    temp.position = quad.origin;
    temp.color = white;
    temp.texcoord.x = 0;
    temp.texcoord.y = 0;
    mesh.vertices.push_back(temp);
    
    // Bottom right
    temp.position = quad.origin;
    temp.position.x += quad.size.width;
    temp.color = white;
    temp.texcoord.x = 0;
    temp.texcoord.y = 0;
    mesh.vertices.push_back(temp);
    
    // Top right
    temp.position = quad.origin+quad.size;
    temp.color = white;
    temp.texcoord.x = 0;
    temp.texcoord.y = 0;
    mesh.vertices.push_back(temp);
    
    // Top left
    temp.position = quad.origin;
    temp.position.y += quad.size.height;
    temp.color = white;
    temp.texcoord.x = 0;
    temp.texcoord.y = 0;
    mesh.vertices.push_back(temp);
    
    // Add the quad indices
    mesh.indices.push_back(size);
    mesh.indices.push_back(size+1);
    mesh.indices.push_back(size+1);
    mesh.indices.push_back(size+2);
    mesh.indices.push_back(size+2);
    mesh.indices.push_back(size+3);
    mesh.indices.push_back(size+3);
    mesh.indices.push_back(size);
    
    return result;
}
