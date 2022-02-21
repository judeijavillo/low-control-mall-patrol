//
//  CUColor4.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides support for a standard colors.  It provides both a
//  float based color solution and a byte-based color solution.  The former
//  is better for blending and calculations.  The later is better for storage.
//
//  Because math objects are intended to be on the stack, we do not provide
//  any shared pointer support in this class.
//
//  This module is based on an original file from GamePlay3D: http://gameplay3d.org.
//  It has been modified to support the CUGL framework.
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
//  Version: 5/30/16
#include <SDL/SDL.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <map>

#include <cugl/util/CUDebug.h>
#include <cugl/util/CUStrings.h>
#include <cugl/math/CUColor4.h>
#include <cugl/math/CUVec3.h>
#include <cugl/math/CUVec4.h>

using namespace cugl;

/**
 * The standard TK color name space
 */
static const std::map<std::string, Uint32> colornames = {
    { "alice blue", 0xF0F8FFFF },
    { "AliceBlue", 0xF0F8FFFF },
    { "antique white", 0xFAEBD7FF },
    { "AntiqueWhite", 0xFAEBD7FF },
    { "AntiqueWhite1", 0xFFEFDBFF },
    { "AntiqueWhite2", 0xEEDFCCFF },
    { "AntiqueWhite3", 0xCDC0B0FF },
    { "AntiqueWhite4", 0x8B8378FF },
    { "aquamarine", 0x7FFFD4FF },
    { "aquamarine1", 0x7FFFD4FF },
    { "aquamarine2", 0x76EEC6FF },
    { "aquamarine3", 0x66CDAAFF },
    { "aquamarine4", 0x458B74FF },
    { "azure", 0xF0FFFFFF },
    { "azure1", 0xF0FFFFFF },
    { "azure2", 0xE0EEEEFF },
    { "azure3", 0xC1CDCDFF },
    { "azure4", 0x838B8BFF },
    { "beige", 0xF5F5DCFF },
    { "bisque", 0xFFE4C4FF },
    { "bisque1", 0xFFE4C4FF },
    { "bisque2", 0xEED5B7FF },
    { "bisque3", 0xCDB79EFF },
    { "bisque4", 0x8B7D6BFF },
    { "black", 0x000000FF },
    { "blanched almond", 0xFFEBCDFF },
    { "BlanchedAlmond", 0xFFEBCDFF },
    { "blue", 0x0000FFFF },
    { "blue violet", 0x8A2BE2FF },
    { "blue1", 0x0000FFFF },
    { "blue2", 0x0000EEFF },
    { "blue3", 0x0000CDFF },
    { "blue4", 0x00008BFF },
    { "BlueViolet", 0x8A2BE2FF },
    { "brown", 0xA52A2AFF },
    { "brown1", 0xFF4040FF },
    { "brown2", 0xEE3B3BFF },
    { "brown3", 0xCD3333FF },
    { "brown4", 0x8B2323FF },
    { "burlywood", 0xDEB887FF },
    { "burlywood1", 0xFFD39BFF },
    { "burlywood2", 0xEEC591FF },
    { "burlywood3", 0xCDAA7DFF },
    { "burlywood4", 0x8B7355FF },
    { "cadet blue", 0x5F9EA0FF },
    { "CadetBlue", 0x5F9EA0FF },
    { "CadetBlue1", 0x98F5FFFF },
    { "CadetBlue2", 0x8EE5EEFF },
    { "CadetBlue3", 0x7AC5CDFF },
    { "CadetBlue4", 0x53868BFF },
    { "chartreuse", 0x7FFF00FF },
    { "chartreuse1", 0x7FFF00FF },
    { "chartreuse2", 0x76EE00FF },
    { "chartreuse3", 0x66CD00FF },
    { "chartreuse4", 0x458B00FF },
    { "chocolate", 0xD2691EFF },
    { "chocolate1", 0xFF7F24FF },
    { "chocolate2", 0xEE7621FF },
    { "chocolate3", 0xCD661DFF },
    { "chocolate4", 0x8B4513FF },
    { "coral", 0xFF7F50FF },
    { "coral1", 0xFF7256FF },
    { "coral2", 0xEE6A50FF },
    { "coral3", 0xCD5B45FF },
    { "coral4", 0x8B3E2FFF },
    { "cornflower blue", 0x6495EDFF },
    { "CornflowerBlue", 0x6495EDFF },
    { "cornsilk", 0xFFF8DCFF },
    { "cornsilk1", 0xFFF8DCFF },
    { "cornsilk2", 0xEEE8CDFF },
    { "cornsilk3", 0xCDC8B1FF },
    { "cornsilk4", 0x8B8878FF },
    { "cyan", 0x00FFFFFF },
    { "cyan1", 0x00FFFFFF },
    { "cyan2", 0x00EEEEFF },
    { "cyan3", 0x00CDCDFF },
    { "cyan4", 0x008B8BFF },
    { "dark blue", 0x00008BFF },
    { "dark cyan", 0x008B8BFF },
    { "dark goldenrod", 0xB8860BFF },
    { "dark gray", 0xA9A9A9FF },
    { "dark green", 0x006400FF },
    { "dark grey", 0xA9A9A9FF },
    { "dark khaki", 0xBDB76BFF },
    { "dark magenta", 0x8B008BFF },
    { "dark olive green", 0x556B2FFF },
    { "dark orange", 0xFF8C00FF },
    { "dark orchid", 0x9932CCFF },
    { "dark red", 0x8B0000FF },
    { "dark salmon", 0xE9967AFF },
    { "dark sea green", 0x8FBC8FFF },
    { "dark slate blue", 0x483D8BFF },
    { "dark slate gray", 0x2F4F4FFF },
    { "dark slate grey", 0x2F4F4FFF },
    { "dark turquoise", 0x00CED1FF },
    { "dark violet", 0x9400D3FF },
    { "DarkBlue", 0x00008BFF },
    { "DarkCyan", 0x008B8BFF },
    { "DarkGoldenrod", 0xB8860BFF },
    { "DarkGoldenrod1", 0xFFB90FFF },
    { "DarkGoldenrod2", 0xEEAD0EFF },
    { "DarkGoldenrod3", 0xCD950CFF },
    { "DarkGoldenrod4", 0x8B6508FF },
    { "DarkGray", 0xA9A9A9FF },
    { "DarkGreen", 0x006400FF },
    { "DarkGrey", 0xA9A9A9FF },
    { "DarkKhaki", 0xBDB76BFF },
    { "DarkMagenta", 0x8B008BFF },
    { "DarkOliveGreen", 0x556B2FFF },
    { "DarkOliveGreen1", 0xCAFF70FF },
    { "DarkOliveGreen2", 0xBCEE68FF },
    { "DarkOliveGreen3", 0xA2CD5AFF },
    { "DarkOliveGreen4", 0x6E8B3DFF },
    { "DarkOrange", 0xFF8C00FF },
    { "DarkOrange1", 0xFF7F00FF },
    { "DarkOrange2", 0xEE7600FF },
    { "DarkOrange3", 0xCD6600FF },
    { "DarkOrange4", 0x8B4500FF },
    { "DarkOrchid", 0x9932CCFF },
    { "DarkOrchid1", 0xBF3EFFFF },
    { "DarkOrchid2", 0xB23AEEFF },
    { "DarkOrchid3", 0x9A32CDFF },
    { "DarkOrchid4", 0x68228BFF },
    { "DarkRed", 0x8B0000FF },
    { "DarkSalmon", 0xE9967AFF },
    { "DarkSeaGreen", 0x8FBC8FFF },
    { "DarkSeaGreen1", 0xC1FFC1FF },
    { "DarkSeaGreen2", 0xB4EEB4FF },
    { "DarkSeaGreen3", 0x9BCD9BFF },
    { "DarkSeaGreen4", 0x698B69FF },
    { "DarkSlateBlue", 0x483D8BFF },
    { "DarkSlateGray", 0x2F4F4FFF },
    { "DarkSlateGray1", 0x97FFFFFF },
    { "DarkSlateGray2", 0x8DEEEEFF },
    { "DarkSlateGray3", 0x79CDCDFF },
    { "DarkSlateGray4", 0x528B8BFF },
    { "DarkSlateGrey", 0x2F4F4FFF },
    { "DarkTurquoise", 0x00CED1FF },
    { "DarkViolet", 0x9400D3FF },
    { "deep pink", 0xFF1493FF },
    { "deep sky blue", 0x00BFFFFF },
    { "DeepPink", 0xFF1493FF },
    { "DeepPink1", 0xFF1493FF },
    { "DeepPink2", 0xEE1289FF },
    { "DeepPink3", 0xCD1076FF },
    { "DeepPink4", 0x8B0A50FF },
    { "DeepSkyBlue", 0x00BFFFFF },
    { "DeepSkyBlue1", 0x00BFFFFF },
    { "DeepSkyBlue2", 0x00B2EEFF },
    { "DeepSkyBlue3", 0x009ACDFF },
    { "DeepSkyBlue4", 0x00688BFF },
    { "dim gray", 0x696969FF },
    { "dim grey", 0x696969FF },
    { "DimGray", 0x696969FF },
    { "DimGrey", 0x696969FF },
    { "dodger blue", 0x1E90FFFF },
    { "DodgerBlue", 0x1E90FFFF },
    { "DodgerBlue1", 0x1E90FFFF },
    { "DodgerBlue2", 0x1C86EEFF },
    { "DodgerBlue3", 0x1874CDFF },
    { "DodgerBlue4", 0x104E8BFF },
    { "firebrick", 0xB22222FF },
    { "firebrick1", 0xFF3030FF },
    { "firebrick2", 0xEE2C2CFF },
    { "firebrick3", 0xCD2626FF },
    { "firebrick4", 0x8B1A1AFF },
    { "floral white", 0xFFFAF0FF },
    { "FloralWhite", 0xFFFAF0FF },
    { "forest green", 0x228B22FF },
    { "ForestGreen", 0x228B22FF },
    { "gainsboro", 0xDCDCDCFF },
    { "ghost white", 0xF8F8FFFF },
    { "GhostWhite", 0xF8F8FFFF },
    { "gold", 0xFFD700FF },
    { "gold1", 0xFFD700FF },
    { "gold2", 0xEEC900FF },
    { "gold3", 0xCDAD00FF },
    { "gold4", 0x8B7500FF },
    { "goldenrod", 0xDAA520FF },
    { "goldenrod1", 0xFFC125FF },
    { "goldenrod2", 0xEEB422FF },
    { "goldenrod3", 0xCD9B1DFF },
    { "goldenrod4", 0x8B6914FF },
    { "gray", 0xBEBEBEFF },
    { "gray0", 0x000000FF },
    { "gray1", 0x030303FF },
    { "gray2", 0x050505FF },
    { "gray3", 0x080808FF },
    { "gray4", 0x0A0A0AFF },
    { "gray5", 0x0D0D0DFF },
    { "gray6", 0x0F0F0FFF },
    { "gray7", 0x121212FF },
    { "gray8", 0x141414FF },
    { "gray9", 0x171717FF },
    { "gray10", 0x1A1A1AFF },
    { "gray11", 0x1C1C1CFF },
    { "gray12", 0x1F1F1FFF },
    { "gray13", 0x212121FF },
    { "gray14", 0x242424FF },
    { "gray15", 0x262626FF },
    { "gray16", 0x292929FF },
    { "gray17", 0x2B2B2BFF },
    { "gray18", 0x2E2E2EFF },
    { "gray19", 0x303030FF },
    { "gray20", 0x333333FF },
    { "gray21", 0x363636FF },
    { "gray22", 0x383838FF },
    { "gray23", 0x3B3B3BFF },
    { "gray24", 0x3D3D3DFF },
    { "gray25", 0x404040FF },
    { "gray26", 0x424242FF },
    { "gray27", 0x454545FF },
    { "gray28", 0x474747FF },
    { "gray29", 0x4A4A4AFF },
    { "gray30", 0x4D4D4DFF },
    { "gray31", 0x4F4F4FFF },
    { "gray32", 0x525252FF },
    { "gray33", 0x545454FF },
    { "gray34", 0x575757FF },
    { "gray35", 0x595959FF },
    { "gray36", 0x5C5C5CFF },
    { "gray37", 0x5E5E5EFF },
    { "gray38", 0x616161FF },
    { "gray39", 0x636363FF },
    { "gray40", 0x666666FF },
    { "gray41", 0x696969FF },
    { "gray42", 0x6B6B6BFF },
    { "gray43", 0x6E6E6EFF },
    { "gray44", 0x707070FF },
    { "gray45", 0x737373FF },
    { "gray46", 0x757575FF },
    { "gray47", 0x787878FF },
    { "gray48", 0x7A7A7AFF },
    { "gray49", 0x7D7D7DFF },
    { "gray50", 0x7F7F7FFF },
    { "gray51", 0x828282FF },
    { "gray52", 0x858585FF },
    { "gray53", 0x878787FF },
    { "gray54", 0x8A8A8AFF },
    { "gray55", 0x8C8C8CFF },
    { "gray56", 0x8F8F8FFF },
    { "gray57", 0x919191FF },
    { "gray58", 0x949494FF },
    { "gray59", 0x969696FF },
    { "gray60", 0x999999FF },
    { "gray61", 0x9C9C9CFF },
    { "gray62", 0x9E9E9EFF },
    { "gray63", 0xA1A1A1FF },
    { "gray64", 0xA3A3A3FF },
    { "gray65", 0xA6A6A6FF },
    { "gray66", 0xA8A8A8FF },
    { "gray67", 0xABABABFF },
    { "gray68", 0xADADADFF },
    { "gray69", 0xB0B0B0FF },
    { "gray70", 0xB3B3B3FF },
    { "gray71", 0xB5B5B5FF },
    { "gray72", 0xB8B8B8FF },
    { "gray73", 0xBABABAFF },
    { "gray74", 0xBDBDBDFF },
    { "gray75", 0xBFBFBFFF },
    { "gray76", 0xC2C2C2FF },
    { "gray77", 0xC4C4C4FF },
    { "gray78", 0xC7C7C7FF },
    { "gray79", 0xC9C9C9FF },
    { "gray80", 0xCCCCCCFF },
    { "gray81", 0xCFCFCFFF },
    { "gray82", 0xD1D1D1FF },
    { "gray83", 0xD4D4D4FF },
    { "gray84", 0xD6D6D6FF },
    { "gray85", 0xD9D9D9FF },
    { "gray86", 0xDBDBDBFF },
    { "gray87", 0xDEDEDEFF },
    { "gray88", 0xE0E0E0FF },
    { "gray89", 0xE3E3E3FF },
    { "gray90", 0xE5E5E5FF },
    { "gray91", 0xE8E8E8FF },
    { "gray92", 0xEBEBEBFF },
    { "gray93", 0xEDEDEDFF },
    { "gray94", 0xF0F0F0FF },
    { "gray95", 0xF2F2F2FF },
    { "gray96", 0xF5F5F5FF },
    { "gray97", 0xF7F7F7FF },
    { "gray98", 0xFAFAFAFF },
    { "gray99", 0xFCFCFCFF },
    { "gray100", 0xFFFFFFFF },
    { "green", 0x00FF00FF },
    { "green yellow", 0xADFF2FFF },
    { "green1", 0x00FF00FF },
    { "green2", 0x00EE00FF },
    { "green3", 0x00CD00FF },
    { "green4", 0x008B00FF },
    { "GreenYellow", 0xADFF2FFF },
    { "grey", 0xBEBEBEFF },
    { "grey0", 0x000000FF },
    { "grey1", 0x030303FF },
    { "grey2", 0x050505FF },
    { "grey3", 0x080808FF },
    { "grey4", 0x0A0A0AFF },
    { "grey5", 0x0D0D0DFF },
    { "grey6", 0x0F0F0FFF },
    { "grey7", 0x121212FF },
    { "grey8", 0x141414FF },
    { "grey9", 0x171717FF },
    { "grey10", 0x1A1A1AFF },
    { "grey11", 0x1C1C1CFF },
    { "grey12", 0x1F1F1FFF },
    { "grey13", 0x212121FF },
    { "grey14", 0x242424FF },
    { "grey15", 0x262626FF },
    { "grey16", 0x292929FF },
    { "grey17", 0x2B2B2BFF },
    { "grey18", 0x2E2E2EFF },
    { "grey19", 0x303030FF },
    { "grey20", 0x333333FF },
    { "grey21", 0x363636FF },
    { "grey22", 0x383838FF },
    { "grey23", 0x3B3B3BFF },
    { "grey24", 0x3D3D3DFF },
    { "grey25", 0x404040FF },
    { "grey26", 0x424242FF },
    { "grey27", 0x454545FF },
    { "grey28", 0x474747FF },
    { "grey29", 0x4A4A4AFF },
    { "grey30", 0x4D4D4DFF },
    { "grey31", 0x4F4F4FFF },
    { "grey32", 0x525252FF },
    { "grey33", 0x545454FF },
    { "grey34", 0x575757FF },
    { "grey35", 0x595959FF },
    { "grey36", 0x5C5C5CFF },
    { "grey37", 0x5E5E5EFF },
    { "grey38", 0x616161FF },
    { "grey39", 0x636363FF },
    { "grey40", 0x666666FF },
    { "grey41", 0x696969FF },
    { "grey42", 0x6B6B6BFF },
    { "grey43", 0x6E6E6EFF },
    { "grey44", 0x707070FF },
    { "grey45", 0x737373FF },
    { "grey46", 0x757575FF },
    { "grey47", 0x787878FF },
    { "grey48", 0x7A7A7AFF },
    { "grey49", 0x7D7D7DFF },
    { "grey50", 0x7F7F7FFF },
    { "grey51", 0x828282FF },
    { "grey52", 0x858585FF },
    { "grey53", 0x878787FF },
    { "grey54", 0x8A8A8AFF },
    { "grey55", 0x8C8C8CFF },
    { "grey56", 0x8F8F8FFF },
    { "grey57", 0x919191FF },
    { "grey58", 0x949494FF },
    { "grey59", 0x969696FF },
    { "grey60", 0x999999FF },
    { "grey61", 0x9C9C9CFF },
    { "grey62", 0x9E9E9EFF },
    { "grey63", 0xA1A1A1FF },
    { "grey64", 0xA3A3A3FF },
    { "grey65", 0xA6A6A6FF },
    { "grey66", 0xA8A8A8FF },
    { "grey67", 0xABABABFF },
    { "grey68", 0xADADADFF },
    { "grey69", 0xB0B0B0FF },
    { "grey70", 0xB3B3B3FF },
    { "grey71", 0xB5B5B5FF },
    { "grey72", 0xB8B8B8FF },
    { "grey73", 0xBABABAFF },
    { "grey74", 0xBDBDBDFF },
    { "grey75", 0xBFBFBFFF },
    { "grey76", 0xC2C2C2FF },
    { "grey77", 0xC4C4C4FF },
    { "grey78", 0xC7C7C7FF },
    { "grey79", 0xC9C9C9FF },
    { "grey80", 0xCCCCCCFF },
    { "grey81", 0xCFCFCFFF },
    { "grey82", 0xD1D1D1FF },
    { "grey83", 0xD4D4D4FF },
    { "grey84", 0xD6D6D6FF },
    { "grey85", 0xD9D9D9FF },
    { "grey86", 0xDBDBDBFF },
    { "grey87", 0xDEDEDEFF },
    { "grey88", 0xE0E0E0FF },
    { "grey89", 0xE3E3E3FF },
    { "grey90", 0xE5E5E5FF },
    { "grey91", 0xE8E8E8FF },
    { "grey92", 0xEBEBEBFF },
    { "grey93", 0xEDEDEDFF },
    { "grey94", 0xF0F0F0FF },
    { "grey95", 0xF2F2F2FF },
    { "grey96", 0xF5F5F5FF },
    { "grey97", 0xF7F7F7FF },
    { "grey98", 0xFAFAFAFF },
    { "grey99", 0xFCFCFCFF },
    { "grey100", 0xFFFFFFFF },
    { "honeydew", 0xF0FFF0FF },
    { "honeydew1", 0xF0FFF0FF },
    { "honeydew2", 0xE0EEE0FF },
    { "honeydew3", 0xC1CDC1FF },
    { "honeydew4", 0x838B83FF },
    { "hot pink", 0xFF69B4FF },
    { "HotPink", 0xFF69B4FF },
    { "HotPink1", 0xFF6EB4FF },
    { "HotPink2", 0xEE6AA7FF },
    { "HotPink3", 0xCD6090FF },
    { "HotPink4", 0x8B3A62FF },
    { "indian red", 0xCD5C5CFF },
    { "IndianRed", 0xCD5C5CFF },
    { "IndianRed1", 0xFF6A6AFF },
    { "IndianRed2", 0xEE6363FF },
    { "IndianRed3", 0xCD5555FF },
    { "IndianRed4", 0x8B3A3AFF },
    { "ivory", 0xFFFFF0FF },
    { "ivory1", 0xFFFFF0FF },
    { "ivory2", 0xEEEEE0FF },
    { "ivory3", 0xCDCDC1FF },
    { "ivory4", 0x8B8B83FF },
    { "khaki", 0xF0E68CFF },
    { "khaki1", 0xFFF68FFF },
    { "khaki2", 0xEEE685FF },
    { "khaki3", 0xCDC673FF },
    { "khaki4", 0x8B864EFF },
    { "lavender", 0xE6E6FAFF },
    { "lavender blush", 0xFFF0F5FF },
    { "LavenderBlush", 0xFFF0F5FF },
    { "LavenderBlush1", 0xFFF0F5FF },
    { "LavenderBlush2", 0xEEE0E5FF },
    { "LavenderBlush3", 0xCDC1C5FF },
    { "LavenderBlush4", 0x8B8386FF },
    { "lawn green", 0x7CFC00FF },
    { "LawnGreen", 0x7CFC00FF },
    { "lemon chiffon", 0xFFFACDFF },
    { "LemonChiffon", 0xFFFACDFF },
    { "LemonChiffon1", 0xFFFACDFF },
    { "LemonChiffon2", 0xEEE9BFFF },
    { "LemonChiffon3", 0xCDC9A5FF },
    { "LemonChiffon4", 0x8B8970FF },
    { "light blue", 0xADD8E6FF },
    { "light coral", 0xF08080FF },
    { "light cyan", 0xE0FFFFFF },
    { "light goldenrod", 0xEEDD82FF },
    { "light goldenrod yellow", 0xFAFAD2FF },
    { "light gray", 0xD3D3D3FF },
    { "light green", 0x90EE90FF },
    { "light grey", 0xD3D3D3FF },
    { "light pink", 0xFFB6C1FF },
    { "light salmon", 0xFFA07AFF },
    { "light sea green", 0x20B2AAFF },
    { "light sky blue", 0x87CEFAFF },
    { "light slate blue", 0x8470FFFF },
    { "light slate gray", 0x778899FF },
    { "light slate grey", 0x778899FF },
    { "light steel blue", 0xB0C4DEFF },
    { "light yellow", 0xFFFFE0FF },
    { "LightBlue", 0xADD8E6FF },
    { "LightBlue1", 0xBFEFFFFF },
    { "LightBlue2", 0xB2DFEEFF },
    { "LightBlue3", 0x9AC0CDFF },
    { "LightBlue4", 0x68838BFF },
    { "LightCoral", 0xF08080FF },
    { "LightCyan", 0xE0FFFFFF },
    { "LightCyan1", 0xE0FFFFFF },
    { "LightCyan2", 0xD1EEEEFF },
    { "LightCyan3", 0xB4CDCDFF },
    { "LightCyan4", 0x7A8B8BFF },
    { "LightGoldenrod", 0xEEDD82FF },
    { "LightGoldenrod1", 0xFFEC8BFF },
    { "LightGoldenrod2", 0xEEDC82FF },
    { "LightGoldenrod3", 0xCDBE70FF },
    { "LightGoldenrod4", 0x8B814CFF },
    { "LightGoldenrodYellow", 0xFAFAD2FF },
    { "LightGray", 0xD3D3D3FF },
    { "LightGreen", 0x90EE90FF },
    { "LightGrey", 0xD3D3D3FF },
    { "LightPink", 0xFFB6C1FF },
    { "LightPink1", 0xFFAEB9FF },
    { "LightPink2", 0xEEA2ADFF },
    { "LightPink3", 0xCD8C95FF },
    { "LightPink4", 0x8B5F65FF },
    { "LightSalmon", 0xFFA07AFF },
    { "LightSalmon1", 0xFFA07AFF },
    { "LightSalmon2", 0xEE9572FF },
    { "LightSalmon3", 0xCD8162FF },
    { "LightSalmon4", 0x8B5742FF },
    { "LightSeaGreen", 0x20B2AAFF },
    { "LightSkyBlue", 0x87CEFAFF },
    { "LightSkyBlue1", 0xB0E2FFFF },
    { "LightSkyBlue2", 0xA4D3EEFF },
    { "LightSkyBlue3", 0x8DB6CDFF },
    { "LightSkyBlue4", 0x607B8BFF },
    { "LightSlateBlue", 0x8470FFFF },
    { "LightSlateGray", 0x778899FF },
    { "LightSlateGrey", 0x778899FF },
    { "LightSteelBlue", 0xB0C4DEFF },
    { "LightSteelBlue1", 0xCAE1FFFF },
    { "LightSteelBlue2", 0xBCD2EEFF },
    { "LightSteelBlue3", 0xA2B5CDFF },
    { "LightSteelBlue4", 0x6E7B8BFF },
    { "LightYellow", 0xFFFFE0FF },
    { "LightYellow1", 0xFFFFE0FF },
    { "LightYellow2", 0xEEEED1FF },
    { "LightYellow3", 0xCDCDB4FF },
    { "LightYellow4", 0x8B8B7AFF },
    { "lime green", 0x32CD32FF },
    { "LimeGreen", 0x32CD32FF },
    { "linen", 0xFAF0E6FF },
    { "magenta", 0xFF00FFFF },
    { "magenta1", 0xFF00FFFF },
    { "magenta2", 0xEE00EEFF },
    { "magenta3", 0xCD00CDFF },
    { "magenta4", 0x8B008BFF },
    { "maroon", 0xB03060FF },
    { "maroon1", 0xFF34B3FF },
    { "maroon2", 0xEE30A7FF },
    { "maroon3", 0xCD2990FF },
    { "maroon4", 0x8B1C62FF },
    { "medium aquamarine", 0x66CDAAFF },
    { "medium blue", 0x0000CDFF },
    { "medium orchid", 0xBA55D3FF },
    { "medium purple", 0x9370DBFF },
    { "medium sea green", 0x3CB371FF },
    { "medium slate blue", 0x7B68EEFF },
    { "medium spring green", 0x00FA9AFF },
    { "medium turquoise", 0x48D1CCFF },
    { "medium violet red", 0xC71585FF },
    { "MediumAquamarine", 0x66CDAAFF },
    { "MediumBlue", 0x0000CDFF },
    { "MediumOrchid", 0xBA55D3FF },
    { "MediumOrchid1", 0xE066FFFF },
    { "MediumOrchid2", 0xD15FEEFF },
    { "MediumOrchid3", 0xB452CDFF },
    { "MediumOrchid4", 0x7A378BFF },
    { "MediumPurple", 0x9370DBFF },
    { "MediumPurple1", 0xAB82FFFF },
    { "MediumPurple2", 0x9F79EEFF },
    { "MediumPurple3", 0x8968CDFF },
    { "MediumPurple4", 0x5D478BFF },
    { "MediumSeaGreen", 0x3CB371FF },
    { "MediumSlateBlue", 0x7B68EEFF },
    { "MediumSpringGreen", 0x00FA9AFF },
    { "MediumTurquoise", 0x48D1CCFF },
    { "MediumVioletRed", 0xC71585FF },
    { "midnight blue", 0x191970FF },
    { "MidnightBlue", 0x191970FF },
    { "mint cream", 0xF5FFFAFF },
    { "MintCream", 0xF5FFFAFF },
    { "misty rose", 0xFFE4E1FF },
    { "MistyRose", 0xFFE4E1FF },
    { "MistyRose1", 0xFFE4E1FF },
    { "MistyRose2", 0xEED5D2FF },
    { "MistyRose3", 0xCDB7B5FF },
    { "MistyRose4", 0x8B7D7BFF },
    { "moccasin", 0xFFE4B5FF },
    { "navajo white", 0xFFDEADFF },
    { "NavajoWhite", 0xFFDEADFF },
    { "NavajoWhite1", 0xFFDEADFF },
    { "NavajoWhite2", 0xEECFA1FF },
    { "NavajoWhite3", 0xCDB38BFF },
    { "NavajoWhite4", 0x8B795EFF },
    { "navy", 0x000080FF },
    { "navy blue", 0x000080FF },
    { "NavyBlue", 0x000080FF },
    { "old lace", 0xFDF5E6FF },
    { "OldLace", 0xFDF5E6FF },
    { "olive drab", 0x6B8E23FF },
    { "OliveDrab", 0x6B8E23FF },
    { "OliveDrab1", 0xC0FF3EFF },
    { "OliveDrab2", 0xB3EE3AFF },
    { "OliveDrab3", 0x9ACD32FF },
    { "OliveDrab4", 0x698B22FF },
    { "orange", 0xFFA500FF },
    { "orange red", 0xFF4500FF },
    { "orange1", 0xFFA500FF },
    { "orange2", 0xEE9A00FF },
    { "orange3", 0xCD8500FF },
    { "orange4", 0x8B5A00FF },
    { "OrangeRed", 0xFF4500FF },
    { "OrangeRed1", 0xFF4500FF },
    { "OrangeRed2", 0xEE4000FF },
    { "OrangeRed3", 0xCD3700FF },
    { "OrangeRed4", 0x8B2500FF },
    { "orchid", 0xDA70D6FF },
    { "orchid1", 0xFF83FAFF },
    { "orchid2", 0xEE7AE9FF },
    { "orchid3", 0xCD69C9FF },
    { "orchid4", 0x8B4789FF },
    { "pale goldenrod", 0xEEE8AAFF },
    { "pale green", 0x98FB98FF },
    { "pale turquoise", 0xAFEEEEFF },
    { "pale violet red", 0xDB7093FF },
    { "PaleGoldenrod", 0xEEE8AAFF },
    { "PaleGreen", 0x98FB98FF },
    { "PaleGreen1", 0x9AFF9AFF },
    { "PaleGreen2", 0x90EE90FF },
    { "PaleGreen3", 0x7CCD7CFF },
    { "PaleGreen4", 0x548B54FF },
    { "PaleTurquoise", 0xAFEEEEFF },
    { "PaleTurquoise1", 0xBBFFFFFF },
    { "PaleTurquoise2", 0xAEEEEEFF },
    { "PaleTurquoise3", 0x96CDCDFF },
    { "PaleTurquoise4", 0x668B8BFF },
    { "PaleVioletRed", 0xDB7093FF },
    { "PaleVioletRed1", 0xFF82ABFF },
    { "PaleVioletRed2", 0xEE799FFF },
    { "PaleVioletRed3", 0xCD687FFF },
    { "PaleVioletRed4", 0x8B475DFF },
    { "papaya whip", 0xFFEFD5FF },
    { "PapayaWhip", 0xFFEFD5FF },
    { "peach puff", 0xFFDAB9FF },
    { "PeachPuff", 0xFFDAB9FF },
    { "PeachPuff1", 0xFFDAB9FF },
    { "PeachPuff2", 0xEECBADFF },
    { "PeachPuff3", 0xCDAF95FF },
    { "PeachPuff4", 0x8B7765FF },
    { "peru", 0xCD853FFF },
    { "pink", 0xFFC0CBFF },
    { "pink1", 0xFFB5C5FF },
    { "pink2", 0xEEA9B8FF },
    { "pink3", 0xCD919EFF },
    { "pink4", 0x8B636CFF },
    { "plum", 0xDDA0DDFF },
    { "plum1", 0xFFBBFFFF },
    { "plum2", 0xEEAEEEFF },
    { "plum3", 0xCD96CDFF },
    { "plum4", 0x8B668BFF },
    { "powder blue", 0xB0E0E6FF },
    { "PowderBlue", 0xB0E0E6FF },
    { "purple", 0xA020F0FF },
    { "purple1", 0x9B30FFFF },
    { "purple2", 0x912CEEFF },
    { "purple3", 0x7D26CDFF },
    { "purple4", 0x551A8BFF },
    { "red", 0xFF0000FF },
    { "red1", 0xFF0000FF },
    { "red2", 0xEE0000FF },
    { "red3", 0xCD0000FF },
    { "red4", 0x8B0000FF },
    { "rosy brown", 0xBC8F8FFF },
    { "RosyBrown", 0xBC8F8FFF },
    { "RosyBrown1", 0xFFC1C1FF },
    { "RosyBrown2", 0xEEB4B4FF },
    { "RosyBrown3", 0xCD9B9BFF },
    { "RosyBrown4", 0x8B6969FF },
    { "royal blue", 0x4169E1FF },
    { "RoyalBlue", 0x4169E1FF },
    { "RoyalBlue1", 0x4876FFFF },
    { "RoyalBlue2", 0x436EEEFF },
    { "RoyalBlue3", 0x3A5FCDFF },
    { "RoyalBlue4", 0x27408BFF },
    { "saddle brown", 0x8B4513FF },
    { "SaddleBrown", 0x8B4513FF },
    { "salmon", 0xFA8072FF },
    { "salmon1", 0xFF8C69FF },
    { "salmon2", 0xEE8262FF },
    { "salmon3", 0xCD7054FF },
    { "salmon4", 0x8B4C39FF },
    { "sandy brown", 0xF4A460FF },
    { "SandyBrown", 0xF4A460FF },
    { "sea green", 0x2E8B57FF },
    { "SeaGreen", 0x2E8B57FF },
    { "SeaGreen1", 0x54FF9FFF },
    { "SeaGreen2", 0x4EEE94FF },
    { "SeaGreen3", 0x43CD80FF },
    { "SeaGreen4", 0x2E8B57FF },
    { "seashell", 0xFFF5EEFF },
    { "seashell1", 0xFFF5EEFF },
    { "seashell2", 0xEEE5DEFF },
    { "seashell3", 0xCDC5BFFF },
    { "seashell4", 0x8B8682FF },
    { "sienna", 0xA0522DFF },
    { "sienna1", 0xFF8247FF },
    { "sienna2", 0xEE7942FF },
    { "sienna3", 0xCD6839FF },
    { "sienna4", 0x8B4726FF },
    { "sky blue", 0x87CEEBFF },
    { "SkyBlue", 0x87CEEBFF },
    { "SkyBlue1", 0x87CEFFFF },
    { "SkyBlue2", 0x7EC0EEFF },
    { "SkyBlue3", 0x6CA6CDFF },
    { "SkyBlue4", 0x4A708BFF },
    { "slate blue", 0x6A5ACDFF },
    { "slate gray", 0x708090FF },
    { "slate grey", 0x708090FF },
    { "SlateBlue", 0x6A5ACDFF },
    { "SlateBlue1", 0x836FFFFF },
    { "SlateBlue2", 0x7A67EEFF },
    { "SlateBlue3", 0x6959CDFF },
    { "SlateBlue4", 0x473C8BFF },
    { "SlateGray", 0x708090FF },
    { "SlateGray1", 0xC6E2FFFF },
    { "SlateGray2", 0xB9D3EEFF },
    { "SlateGray3", 0x9FB6CDFF },
    { "SlateGray4", 0x6C7B8BFF },
    { "SlateGrey", 0x708090FF },
    { "snow", 0xFFFAFAFF },
    { "snow1", 0xFFFAFAFF },
    { "snow2", 0xEEE9E9FF },
    { "snow3", 0xCDC9C9FF },
    { "snow4", 0x8B8989FF },
    { "spring green", 0x00FF7FFF },
    { "SpringGreen", 0x00FF7FFF },
    { "SpringGreen1", 0x00FF7FFF },
    { "SpringGreen2", 0x00EE76FF },
    { "SpringGreen3", 0x00CD66FF },
    { "SpringGreen4", 0x008B45FF },
    { "steel blue", 0x4682B4FF },
    { "SteelBlue", 0x4682B4FF },
    { "SteelBlue1", 0x63B8FFFF },
    { "SteelBlue2", 0x5CACEEFF },
    { "SteelBlue3", 0x4F94CDFF },
    { "SteelBlue4", 0x36648BFF },
    { "tan", 0xD2B48CFF },
    { "tan1", 0xFFA54FFF },
    { "tan2", 0xEE9A49FF },
    { "tan3", 0xCD853FFF },
    { "tan4", 0x8B5A2BFF },
    { "thistle", 0xD8BFD8FF },
    { "thistle1", 0xFFE1FFFF },
    { "thistle2", 0xEED2EEFF },
    { "thistle3", 0xCDB5CDFF },
    { "thistle4", 0x8B7B8BFF },
    { "tomato", 0xFF6347FF },
    { "tomato1", 0xFF6347FF },
    { "tomato2", 0xEE5C42FF },
    { "tomato3", 0xCD4F39FF },
    { "tomato4", 0x8B3626FF },
    { "turquoise", 0x40E0D0FF },
    { "turquoise1", 0x00F5FFFF },
    { "turquoise2", 0x00E5EEFF },
    { "turquoise3", 0x00C5CDFF },
    { "turquoise4", 0x00868BFF },
    { "violet", 0xEE82EEFF },
    { "violet red", 0xD02090FF },
    { "VioletRed", 0xD02090FF },
    { "VioletRed1", 0xFF3E96FF },
    { "VioletRed2", 0xEE3A8CFF },
    { "VioletRed3", 0xCD3278FF },
    { "VioletRed4", 0x8B2252FF },
    { "wheat", 0xF5DEB3FF },
    { "wheat1", 0xFFE7BAFF },
    { "wheat2", 0xEED8AEFF },
    { "wheat3", 0xCDBA96FF },
    { "wheat4", 0x8B7E66FF },
    { "white", 0xFFFFFFFF },
    { "white smoke", 0xF5F5F5FF },
    { "WhiteSmoke", 0xF5F5F5FF },
    { "yellow", 0xFFFF00FF },
    { "yellow green", 0x9ACD32FF },
    { "yellow1", 0xFFFF00FF },
    { "yellow2", 0xEEEE00FF },
    { "yellow3", 0xCDCD00FF },
    { "yellow4", 0x8B8B00FF },
    { "YellowGreen", 0x9ACD32FF }
};

/**
 * Returns the component factor for the given hue h
 *
 * This is essentially the formula given on
 *
 *      https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 * for HSL to RGB where m2 is L+C/2 and m1 is L-C/2.
 *
 * @param h     The hue in range [0,1]
 * @param m1    The value L-C/2
 * @param m2    The value L+C/2
 *
 * @return the component factor for the given hue h
 */
static float hue_factor(float h, float m1, float m2) {
    if (h < 0) h += 1;
    if (h > 1) h -= 1;
    if (h < 1.0f/6.0f)
        return m1 + (m2 - m1) * h * 6.0f;
    else if (h < 3.0f/6.0f)
        return m2;
    else if (h < 4.0f/6.0f)
        return m1 + (m2 - m1) * (2.0f/3.0f - h) * 6.0f;
    return m1;
}

#pragma mark -
#pragma mark Color with Float Attributes

/**
 * Constructs a new color initialized to the specified values.
 *
 * The color values must all in the range 0..1.
 *
 * @param r The red color.
 * @param g The green color.
 * @param b The blue color.
 * @param a The alpha value (optional).
 */
Color4f::Color4f(float r, float g, float b, float a) {
    CUAssertLog(0 <= r && r <= 1, "Red value out of range: %.3f", r);
    CUAssertLog(0 <= g && g <= 1, "Green value out of range: %.3f", g);
    CUAssertLog(0 <= b && b <= 1, "Blue value out of range: %.3f", b);
    CUAssertLog(0 <= a && a <= 1, "Alpha value out of range: %.3f", a);
    this->r = r; this->g = g; this->b = b; this->a = a;
}

/**
 * Constructs a new color from the values in the specified array.
 *
 * The color values must all in the range 0..1.
 *
 * @param array An array containing the color values in the order r, g, b, a.
 */
Color4f::Color4f(const float* array) {
    CUAssertLog(0 <= array[0] && array[0] <= 1, "Red value out of range: %.3f", array[0]);
    CUAssertLog(0 <= array[1] && array[1] <= 1, "Green value out of range: %.3f", array[1]);
    CUAssertLog(0 <= array[2] && array[2] <= 1, "Blue value out of range: %.3f", array[2]);
    CUAssertLog(0 <= array[3] && array[3] <= 1, "Alpha value out of range: %.3f", array[3]);
    this->r = array[0]; this->g = array[1]; this->b = array[2]; this->a = array[3];
}

/**
 * Creates a new color from an integer interpreted as an RGBA value.
 *
 * This method converts the RGBA value to a Color4 and then converts the
 * result to a Color4f. This representation is endian dependent. Do not
 * serialize this value.
 *
 * @param color The integer to interpret as an RGBA value.
 */
Color4f::Color4f(unsigned int color) {
    set(color);
}

/**
 * Creates a new color from an string represenation
 *
 * A string representation may either be an explicit name or a (modified)
 * HTML code.  The supported color names are the classic TCL/TK colors,
 * which are listed here:
 *
 *     https://www.tcl.tk/man/tcl8.6/TkCmd/colors.htm
 *
 * An HTML code is a string that starts with #, followed by the characters
 * 0-9, A-F. This is the hex representation of the color, as specified here:
 *
 *   https://htmlcolorcodes.com
 *
 * We support 3, 4, 6, and 8 character codes (not including the #). The
 * standard code is 6 characters, but that does not specify the alpha value.
 * If you specify eight characters, the last two characters specify the
 * byte values for alpha, following the same rules for red, green, blue.
 *
 * Finally, the 3 and 4 character versions are the abbreviated 6 and 8
 * character versions, respectively.  In these versions, the byte values
 * are repeated.  So #fa2 becomes #ffaa22 and #fad3 becomes #ffaadd33.
 *
 * @param color The integer to interpret as an RGBA value.
 */
Color4f::Color4f(const std::string name) {
    set(name);
}

#pragma mark Setters
/**
 * Sets the elements of this color to the specified values.
 *
 * The color values must all in the range 0..1.
 *
 * @param r The red color.
 * @param g The green color.
 * @param b The blue color.
 * @param a The alpha value (optional).
 *
 * @return A reference to this (modified) Color4f for chaining.
 */
Color4f& Color4f::set(float r, float g, float b, float a) {
    CUAssertLog(0 <= r && r <= 1, "Red value out of range: %.3f", r);
    CUAssertLog(0 <= g && g <= 1, "Green value out of range: %.3f", g);
    CUAssertLog(0 <= b && b <= 1, "Blue value out of range: %.3f", b);
    CUAssertLog(0 <= a && a <= 1, "Alpha value out of range: %.3f", a);
    this->r = r; this->g = g; this->b = b; this->a = a;
    return *this;
}

/**
 * Sets the elements of this color from the values in the specified array.
 *
 * The color values must all in the range 0..1.
 *
 * @param array An array containing the color values in the order r, g, b, a.
 *
 * @return A reference to this (modified) Color4f for chaining.
 */
Color4f& Color4f::set(const float* array) {
    CUAssertLog(0 <= array[0] && array[0] <= 1, "Red value out of range: %.3f", array[0]);
    CUAssertLog(0 <= array[1] && array[1] <= 1, "Green value out of range: %.3f", array[1]);
    CUAssertLog(0 <= array[2] && array[2] <= 1, "Blue value out of range: %.3f", array[2]);
    CUAssertLog(0 <= array[3] && array[3] <= 1, "Alpha value out of range: %.3f", array[3]);
    this->r = array[0]; this->g = array[1]; this->b = array[2]; this->a = array[3];
    return *this;
}

/**
 * Sets this color to an integer interpreted as an RGBA value.
 *
 * This method converts the RGBA value to a Color4 and then converts the
 * result to a Color4f. This representation is endian dependent. Do not
 * serialize this value.
 *
 * @param color The integer to interpret as an RGBA value.
 *
 * @return A reference to this (modified) Color4f for chaining.
 */
Color4f& Color4f::set(unsigned int color) {
    Color4 temp(color);
    r = COLOR_BYTE_TO_FLOAT(temp.r);
    g = COLOR_BYTE_TO_FLOAT(temp.g);
    b = COLOR_BYTE_TO_FLOAT(temp.b);
    a = COLOR_BYTE_TO_FLOAT(temp.a);
    return *this;
}

/**
 * Sets the elements of this color to match the string represenation
 *
 * A string representation may either be an explicit name or a (modified)
 * HTML code.  The supported color names are the classic TCL/TK colors,
 * which are listed here:
 *
 *     https://www.tcl.tk/man/tcl8.6/TkCmd/colors.htm
 *
 * An HTML code is a string that starts with #, followed by the characters
 * 0-9, A-F. This is the hex representation of the color, as specified here:
 *
 *   https://htmlcolorcodes.com
 *
 * We support 3, 4, 6, and 8 character codes (not including the #). The
 * standard code is 6 characters, but that does not specify the alpha value.
 * If you specify eight characters, the last two characters specify the
 * byte values for alpha, following the same rules for red, green, blue.
 *
 * Finally, the 3 and 4 character versions are the abbreviated 6 and 8
 * character versions, respectively.  In these versions, the byte values
 * are repeated.  So #fa2 becomes #ffaa22 and #fad3 becomes #ffaadd33.
 *
 * @param name      The string name
 *
 * @return A reference to this (modified) Color4f for chaining.
 */
Color4f& Color4f::set(const std::string name) {
    Color4 temp(name);
    r = COLOR_BYTE_TO_FLOAT(temp.r);
    g = COLOR_BYTE_TO_FLOAT(temp.g);
    b = COLOR_BYTE_TO_FLOAT(temp.b);
    a = COLOR_BYTE_TO_FLOAT(temp.a);
    return *this;
}

/**
 * Sets this color to have the given hue-saturation-value
 *
 * This method will convert to hue-staturation-lightness and then apply the
 * formula given on this page:
 *
 *      https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 * @param h     The hue (in radians)
 * @param s     The saturation (in range 0..1)
 * @param v     The brightness value (in range 0..1)
 * @param a     The alpha component (in range 0..1)
 *
 * @return A reference to this (modified) Color4f for chaining.
 */
Color4f& Color4f::setHSV(float h, float s, float v, float a) {
    float l = v*(1-s/2);
    float s0 = l == 0 || l == 1 ? 0 : (v-l)/std::min(l,1-l);
    return setHSL(h,s0,l,a);
}

/**
 * Sets this color to have the given hue-saturation-lightness
 *
 * This method will apply the formula given on this page:
 *
 *      https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 * @param h     The hue (in radians)
 * @param s     The saturation (in range 0..1)
 * @param v     The lightness (in range 0..1)
 * @param a     The alpha component (in range 0..1)
 *
 * @return A reference to this (modified) Color4f for chaining.
 */
Color4f& Color4f::setHSL(float h, float s, float l, float a) {
    this->a = a;

    h = fmodf(h/(M_PI*2),1.0f);
    if (h < 0.0f) h += 1.0f;
    s = clampf(s, 0.0f, 1.0f);
    l = clampf(l, 0.0f, 1.0f);
    
    float m2 = l <= 0.5f ? (l * (1 + s)) : (l + s - l * s);
    float m1 = 2 * l - m2;
    
    r = clampf(hue_factor(h + 1.0f/3.0f, m1, m2), 0.0f, 1.0f);
    g = clampf(hue_factor(h, m1, m2), 0.0f, 1.0f);
    b = clampf(hue_factor(h - 1.0f/3.0f, m1, m2), 0.0f, 1.0f);
    return *this;
}


#pragma mark Comparisons
/**
 * Returns true if this color is less than the given color.
 *
 * This comparison uses lexicographical order of rgba.  To test if all
 * components in this color are less than those of c, use the method
 * darkerThan().
 *
 * @param c The color to compare against.
 *
 * @return True if this color is less than the given color.
 */
bool Color4f::operator<(const Color4f c) const {
    bool result = r < c.r || (r == c.r && g < c.g);
    result = result ||  (r == c.r && g == c.g && b < c.b);
    result = result ||  (r == c.r && g == c.g && b == c.b && a < c.a);
    return result;
}

/**
 * Returns true if this color is greater than the given color.
 *
 * This comparison uses lexicographical order of rgba.  To test if all
 * components in this color are greater than those of c, use the method
 * lighterThan().
 *
 * @param c The color to compare against.
 *
 * @return True if this color is greater than the given color.
 */
bool Color4f::operator>(const Color4f c) const {
    bool result = r > c.r || (r == c.r && g > c.g);
    result = result ||  (r == c.r && g == c.g && b > c.b);
    result = result ||  (r == c.r && g == c.g && b == c.b && a > c.a);
    return result;
}

#pragma mark Arithmetic

/**
 * Clamps this color within the given range.
 *
 * @param min The minimum value.
 * @param max The maximum value.
 *
 * @return A reference to this (modified) Color4f for chaining.
 */
Color4f& Color4f::clamp(const Color4f min, const Color4f max) {
    r = clampf(r,min.r,max.r);
    g = clampf(g,min.g,max.g);
    b = clampf(b,min.b,max.b);
    a = clampf(a,min.a,max.a);
    return *this;
}

/**
 * Interpolates the two colors c1 and c2, and stores the result in dst.
 *
 * If alpha is 0, the result is c1.  If alpha is 1, the color is c2.
 * Otherwise it is a value in c1..c2.  If alpha is outside of the
 * range 0 to 1, it is clamped to the nearest value.
 *
 * This method just implements standard linear interpolation.  It does
 * not attempt to give any blending semantics to it.
 *
 * @param c1    The first color.
 * @param c2    The second color.
 * @param alpha The interpolation value in 0..1
 * @param dst   The color to store the result in
 *
 * @return A reference to dst for chaining
 */
Color4f* Color4f::lerp(const Color4f c1, const Color4f c2, float alpha, Color4f* dst) {
    if (dst == nullptr) { return nullptr; }
    float x = clampf(alpha,0,1);
    *dst = c1 * (1.f - x) + c2 * x;
    return dst;
}

/**
 * Blends the two colors c1 and c2, assuming they are not premultiplied.
 *
 * The blending is the standard over operation with color c1 as the source
 * and c2 as the destination. It assumes that the color values are not
 * premultiplied.
 *
 * @param c1    The source color.
 * @param c2    The destination color.
 * @param dst   The color to store the result in
 *
 * @return A reference to dst for chaining
 */
Color4f* Color4f::blend(const Color4f c1, const Color4f c2, Color4f* dst) {
    float a1 = c2.a*(1-c1.a);
    float a2 = c1.a+a1;
    dst->r = (c1.r*c1.a+c2.r*a1)/a2;
    dst->g = (c1.g*c1.a+c2.g*a1)/a2;
    dst->b = (c1.b*c1.a+c2.b*a1)/a2;
    dst->a = a2;
    return dst;
}

/**
 * Blends the two colors c1 and c2, assuming they are premultiplied.
 *
 * The blending is the standard over operation with color c1 as the source
 * and c2 as the destination. It assumes that the color values are
 * premultiplied.
 *
 * @param c1    The source color.
 * @param c2    The destination color.
 * @param dst   The color to store the result in
 *
 * @return A reference to dst for chaining
 */
Color4f* Color4f::blendPre(const Color4f c1, const Color4f c2, Color4f* dst) {
    float a = c1.a+c2.a*(1-c1.a);
    dst->r = (c1.r+c2.r*(1-c1.a));
    dst->g = (c1.g+c2.g*(1-c1.a));
    dst->b = (c1.b+c2.b*(1-c1.a));
    dst->a = a;
    return dst;
}

#pragma mark Conversions

/**
 * Returns a string representation of this vector for debuggging purposes.
 *
 * If verbose is true, the string will include class information.  This
 * allows us to unambiguously identify the class.
 *
 * @param verbose Whether to include class information
 *
 * @return a string representation of this vector for debuggging purposes.
 */
std::string Color4f::toString(bool verbose) const {
    std::stringstream ss;
    ss << (verbose ? "cugl::Color4f[r=" : "[r=");
    ss << strtool::to_string(r);
    ss << ",g=";
    ss << strtool::to_string(g);
    ss << ",b=";
    ss << strtool::to_string(b);
    ss << ",a=";
    ss << strtool::to_string(a);
    ss << "]";
    return ss.str();
}

/** Cast from Color4f to a vector. */
Color4f::operator Vec4() const {
    return Vec4(r,g,b,a);
}

/**
 * Creates a color from the given vector.
 *
 * The attributes are read in the order x,y,z,w.
 *
 * @param vector    The vector to convert
 */
Color4f::Color4f(const Vec4 vector) {
    CUAssertLog(0 <= vector.x && vector.x <= 1, "Red value out of range: %.3f", vector.x);
    CUAssertLog(0 <= vector.y && vector.y <= 1, "Green value out of range: %.3f", vector.y);
    CUAssertLog(0 <= vector.z && vector.z <= 1, "Blue value out of range: %.3f", vector.z);
    CUAssertLog(0 <= vector.w && vector.w <= 1, "Alpha value out of range: %.3f", vector.w);
    r = vector.x; g = vector.y; b = vector.z; a = vector.w;
}

/**
 * Sets the coordinates of this color to those of the given vector.
 *
 * The attributes are read in the order x,y,z,w.
 *
 * @param vector    The vector to convert
 *
 * @return A reference to this (modified) Color4f for chaining.
 */
Color4f& Color4f::operator= (const Vec4 vector) {
    CUAssertLog(0 <= vector.x && vector.x <= 1, "Red value out of range: %.3f", vector.x);
    CUAssertLog(0 <= vector.y && vector.y <= 1, "Green value out of range: %.3f", vector.y);
    CUAssertLog(0 <= vector.z && vector.z <= 1, "Blue value out of range: %.3f", vector.z);
    CUAssertLog(0 <= vector.w && vector.w <= 1, "Alpha value out of range: %.3f", vector.w);
    r = vector.x; g = vector.y; b = vector.z; a = vector.w;
    return *this;
}

/** Cast from Color4f to a vector. */
Color4f::operator Vec3() const {
    return Vec3(r,g,b);
}

/**
 * Creates a color from the given vector.
 *
 * The attributes are read in the order x,y,z. The alpha value is 1.
 *
 * @param vector    The vector to convert
 */
Color4f::Color4f(const Vec3 vector) {
    CUAssertLog(0 <= vector.x && vector.x <= 1, "Red value out of range: %.3f", vector.x);
    CUAssertLog(0 <= vector.y && vector.y <= 1, "Green value out of range: %.3f", vector.y);
    CUAssertLog(0 <= vector.z && vector.z <= 1, "Blue value out of range: %.3f", vector.z);
    r = vector.x; g = vector.y; b = vector.z; a = 1;
}

/**
 * Sets the coordinates of this color to those of the given vector.
 *
 * The attributes are read in the order x,y,z. The alpha value is 1.
 *
 * @param vector    The vector to convert
 *
 * @return A reference to this (modified) Color4f for chaining.
 */
Color4f& Color4f::operator= (const Vec3 vector) {
    CUAssertLog(0 <= vector.x && vector.x <= 1, "Red value out of range: %.3f", vector.x);
    CUAssertLog(0 <= vector.y && vector.y <= 1, "Green value out of range: %.3f", vector.y);
    CUAssertLog(0 <= vector.z && vector.z <= 1, "Blue value out of range: %.3f", vector.z);
    r = vector.x; g = vector.y; b = vector.z; a = 1;
    return *this;
}

/** Cast from Color4f to a byte-based Color4. */
Color4f::operator Color4() const {
    return Color4(COLOR_FLOAT_TO_BYTE(r),COLOR_FLOAT_TO_BYTE(g),
                 COLOR_FLOAT_TO_BYTE(b),COLOR_FLOAT_TO_BYTE(a));
}

/**
 * Creates a float-based color from the given byte-based color.
 *
 * The attributes are read in the order r,g,b,a. They are all divided by
 * 255.0f before assignment.
 *
 * @param color The color to convert
 */
Color4f::Color4f(Color4 color) {
    r = COLOR_BYTE_TO_FLOAT(color.r);
    g = COLOR_BYTE_TO_FLOAT(color.g);
    b = COLOR_BYTE_TO_FLOAT(color.b);
    a = COLOR_BYTE_TO_FLOAT(color.a);
}

/**
 * Sets the attributes of this color from the given byte-based color.
 *
 * The attributes are read in the order r,g,b,a. They are all divided by
 * 255.0f before assignment.
 *
 * @param color The color to convert.
 *
 * @return A reference to this (modified) Color4f for chaining.
 */
Color4f& Color4f::operator= (Color4 color) {
    r = COLOR_BYTE_TO_FLOAT(color.r);
    g = COLOR_BYTE_TO_FLOAT(color.g);
    b = COLOR_BYTE_TO_FLOAT(color.b);
    a = COLOR_BYTE_TO_FLOAT(color.a);
    return *this;
}

/**
 * Writes the color values into a float array.
 *
 * The attributes are read into the array in the order r,g,b,a. All values
 * are between 0 and 1.
 *
 * @param array The array to store the color values.
 *
 * @return A reference to the array for chaining.
 */
float* Color4f::get(float* array) const {
    array[0] = r;
    array[1] = g;
    array[2] = b;
    array[3] = a;
    return array;
}

/**
 * Returns the packed integer representation of this color
 *
 * This method converts the color to a Color4 and returns the packed color
 * of that result. This representation returned is native to the platform.
 *
 * In this representation, red will always be the highest order byte and
 * alpha will always be the lowest order byte.
 *
 * @return the packed integer representation of this color
 */
GLuint Color4f::getRGBA() const {
    return Color4(*this).getRGBA();
}


/**
 * Returns the packed integer representation of this color
 *
 * This representation returned is native to the platform. In other
 * words, it is guaranteed to be the same value as if you were to
 * access the rgba attribute directly. That makes this value the
 * correct one to send to OpenGL.
 *
 * However, this value is not necessarily the same value that you
 * assigned the constructor, because of endianness issues.  To
 * get that value, you should call {@link #getRGBA} instead.
 *
 * @return the packed integer representation of this color
 */
GLuint Color4f::getPacked() const {
    return Color4(*this).getPacked();
}

#pragma mark Constants

/** The Clear color (0,0,0,0) */
const Color4f Color4f::CLEAR(0.0f,0.0f,0.0f,0.0f);
/** The White color (1,1,1,1) */
const Color4f Color4f::WHITE(1.0f,1.0f,1.0f,1.0f);
/** The Black color (0,0,0,1) */
const Color4f Color4f::BLACK(0.0f,0.0f,0.0f,1.0f);
/** The Yellow color (1,1,0,1) */
const Color4f Color4f::YELLOW(1.0f,1.0f,0.0f,1.0f);
/** The Blue color (0,0,1,1) */
const Color4f Color4f::BLUE(0.0f,0.0f,1.0f,1.0f);
/** The Green color (0,1,0,1) */
const Color4f Color4f::GREEN(0.0f,1.0f,0.0f,1.0f);
/** The Red color (1,0,0,1) */
const Color4f Color4f::RED(1.0f,0.0f,0.0f,1.0f);
/** The Magenta color (1,0,1,1) */
const Color4f Color4f::MAGENTA(1.0f,0.0f,1.0f,1.0f);
/** The Cyan color (0,1,1,1) */
const Color4f Color4f::CYAN(0.0f,1.0f,1.0f,1.0f);
/** The Orange color (1,0.5,0,1) */
const Color4f Color4f::ORANGE(1.0f,0.5f,0.0f,1.0f);
/** The Gray color (0.65,0.65,0.65,1) */
const Color4f Color4f::GRAY(0.65f,0.65f,0.65f,1.0f);
/** The classic XNA color (0.392f,0.584f,0.93f,1,0f) */
const Color4f Color4f::CORNFLOWER(0.392f,0.584f,0.93f,1.0f);
/** The Playing Fields color (0.8f,0.8f,0.5f,1.0f) */
const Color4f Color4f::PAPYRUS(0.8f,0.8f,0.5f,1.0f);



#pragma mark -
#pragma mark Color with Byte Attributes
/**
 * Creates a new color from an string represenation
 *
 * A string representation may either be an explicit name or a (modified)
 * HTML code.  The supported color names are the classic TCL/TK colors,
 * which are listed here:
 *
 *     https://www.tcl.tk/man/tcl8.6/TkCmd/colors.htm
 *
 * An HTML code is a string that starts with #, followed by the characters
 * 0-9, A-F. This is the hex representation of the color, as specified here:
 *
 *   https://htmlcolorcodes.com
 *
 * We support 3, 4, 6, and 8 character codes (not including the #). The
 * standard code is 6 characters, but that does not specify the alpha value.
 * If you specify eight characters, the last two characters specify the
 * byte values for alpha, following the same rules for red, green, blue.
 *
 * Finally, the 3 and 4 character versions are the abbreviated 6 and 8
 * character versions, respectively.  In these versions, the byte values
 * are repeated.  So #fa2 becomes #ffaa22 and #fad3 becomes #ffaadd33.
 *
 * @param color The integer to interpret as an RGBA value.
 */
Color4::Color4(const std::string name) {
    set(name);
}

/**
 * Constructs a new color from the values in the specified array.
 *
 * The color values must all be in the range 0..1. They are multiplied
 * by 255.0 and rounded up
 *
 * @param array An array containing the color values in the order r, g, b, a.
 */
Color4::Color4(const float* array) {
    CUAssertLog(0 <= array[0] && array[0] <= 1, "Red value out of range: %.3f", array[0]);
    CUAssertLog(0 <= array[1] && array[1] <= 1, "Green value out of range: %.3f", array[1]);
    CUAssertLog(0 <= array[2] && array[2] <= 1, "Blue value out of range: %.3f", array[2]);
    CUAssertLog(0 <= array[3] && array[3] <= 1, "Alpha value out of range: %.3f", array[3]);
    this->r = COLOR_FLOAT_TO_BYTE(array[0]);
    this->g = COLOR_FLOAT_TO_BYTE(array[1]);
    this->b = COLOR_FLOAT_TO_BYTE(array[2]);
    this->a = COLOR_FLOAT_TO_BYTE(array[3]);
}

#pragma mark Setters
/**
 * Sets this color to the integer interpreted as an RGBA value.
 *
 * This setter processes the integer in RGBA order, independent of the
 * endianness of the platform. Hence, 0xff0000ff represents red or the
 * color (1, 0, 0, 1).
 *
 * @param color The integer to interpret as an RGBA value.
 *
 * @return A reference to this (modified) Color4 for chaining.
 */
Color4& Color4::set(GLuint color) {
    rgba = marshall(color);
    return *this;
}

/**
 * Sets the elements of this color from the values in the specified array.
 *
 * The color values must all be in the range 0..1. They are multiplied
 * by 255.0 and rounded up
 *
 * @param array An array containing the color values in the order r, g, b, a.
 *
 * @return A reference to this (modified) Color4f for chaining.
 */
Color4& Color4::set(const float* array) {
    CUAssertLog(0 <= array[0] && array[0] <= 1, "Red value out of range: %.3f", array[0]);
    CUAssertLog(0 <= array[1] && array[1] <= 1, "Green value out of range: %.3f", array[1]);
    CUAssertLog(0 <= array[2] && array[2] <= 1, "Blue value out of range: %.3f", array[2]);
    CUAssertLog(0 <= array[3] && array[3] <= 1, "Alpha value out of range: %.3f", array[3]);
    this->r = COLOR_FLOAT_TO_BYTE(array[0]);
    this->g = COLOR_FLOAT_TO_BYTE(array[1]);
    this->b = COLOR_FLOAT_TO_BYTE(array[2]);
    this->a = COLOR_FLOAT_TO_BYTE(array[3]);
    return *this;
}

/**
 * Sets the elements of this color to match the string represenation
 *
 * A string representation may either be an explicit name or a (modified)
 * HTML code.  The supported color names are the classic TCL/TK colors,
 * which are listed here:
 *
 *     https://www.tcl.tk/man/tcl8.6/TkCmd/colors.htm
 *
 * An HTML code is a string that starts with #, followed by the characters
 * 0-9, A-F. This is the hex representation of the color, as specified here:
 *
 *   https://htmlcolorcodes.com
 *
 * We support 3, 4, 6, and 8 character codes (not including the #). The
 * standard code is 6 characters, but that does not specify the alpha value.
 * If you specify eight characters, the last two characters specify the
 * byte values for alpha, following the same rules for red, green, blue.
 *
 * Finally, the 3 and 4 character versions are the abbreviated 6 and 8
 * character versions, respectively.  In these versions, the byte values
 * are repeated.  So #fa2 becomes #ffaa22 and #fad3 becomes #ffaadd33.
 *
 * @param name      The string name
 *
 * @return A reference to this (modified) Color4f for chaining.
 */
Color4& Color4::set(const std::string name) {
    if (name == "") {
        set((GLuint)0);
    } else if (name[0] == '#') {
        std::string color;
        if (name.size() == 4) {
            for(size_t ii = 1; ii < 4; ii++) {
                color.push_back(name[ii]);
                color.push_back(name[ii]);
            }
            color.push_back('f');
            color.push_back('f');
        } else if (name.size() == 5) {
            for(size_t ii = 1; ii < 5; ii++) {
                color.push_back(name[ii]);
                color.push_back(name[ii]);
            }
        } else if (name.size() == 7) {
            color = name.substr(1);
            color.push_back('f');
            color.push_back('f');
        } else if (name.size() == 9) {
            color = name.substr(1);
        } else {
            CUAssertLog(false,"Unrecognized color code '%s'",name.c_str());
        }
        GLuint value;
        std::stringstream ss;
        ss << std::hex << color;
        ss >> value;
        set(value);
    } else if (colornames.find(name) != colornames.end()) {
        set(colornames.find(name)->second);
    } else {
        CUAssertLog(false,"Unrecognized color name '%s'",name.c_str());
    }
    return *this;
}

/**
 * Sets this color to have the given hue-saturation-value
 *
 * This method will convert to hue-staturation-lightness and then apply the
 * formula given on this page:
 *
 *      https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 * @param h     The hue (in radians)
 * @param s     The saturation (in range 0..1)
 * @param v     The brightness value (in range 0..1)
 * @param a     The alpha component (in range 0..255)
 *
 * @return A reference to this (modified) Color4f for chaining.
 */
Color4& Color4::setHSV(float h, float s, float v, GLubyte a) {
    float l = v*(1-s/2);
    float s0 = l == 0 || l == 1 ? 0 : (v-l)/std::min(l,1-l);
    return setHSL(h,s0,l,a);
}

/**
 * Sets this color to have the given hue-saturation-lightness
 *
 * This method will apply the formula given on this page:
 *
 *      https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 * @param h     The hue (in radians)
 * @param s     The saturation (in range 0..1)
 * @param v     The lightness (in range 0..1)
 * @param a     The alpha component (in range 0..255)
 *
 * @return A reference to this (modified) Color4f for chaining.
 */
Color4& Color4::setHSL(float h, float s, float l, GLubyte a) {
    this->a = a;

    h = fmodf(h/(M_PI*2),1.0f);
    if (h < 0.0f) h += 1.0f;
    s = clampf(s, 0.0f, 1.0f);
    l = clampf(l, 0.0f, 1.0f);
    
    float m2 = l <= 0.5f ? (l * (1 + s)) : (l + s - l * s);
    float m1 = 2 * l - m2;
    
    r = COLOR_FLOAT_TO_BYTE(clampf(hue_factor(h + 1.0f/3.0f, m1, m2), 0.0f, 1.0f));
    g = COLOR_FLOAT_TO_BYTE(clampf(hue_factor(h, m1, m2), 0.0f, 1.0f));
    b = COLOR_FLOAT_TO_BYTE(clampf(hue_factor(h - 1.0f/3.0f, m1, m2), 0.0f, 1.0f));
    return *this;
}


#pragma mark Comparisons
/**
 * Returns true if this color is less than the given color.
 *
 * This comparison uses lexicographical order of rgba.  To test if all
 * components in this color are less than those of c, use the method
 * darkerThan().
 *
 * @param c The color to compare against.
 *
 * @return True if this color is less than the given color.
 */
bool Color4::operator<(Color4 c) const {
    bool result = r < c.r || (r == c.r && g < c.g);
    result = result ||  (r == c.r && g == c.g && b < c.b);
    result = result ||  (r == c.r && g == c.g && b == c.b && a < c.a);
    return result;
}

/**
 * Returns true if this color is greater than the given color.
 *
 * This comparison uses lexicographical order of rgba.  To test if all
 * components in this color are greater than those of c, use the method
 * lighterThan().
 *
 * @param c The color to compare against.
 *
 * @return True if this color is greater than the given color.
 */
bool Color4::operator>(Color4 c) const {
    bool result = r > c.r || (r == c.r && g > c.g);
    result = result ||  (r == c.r && g == c.g && b > c.b);
    result = result ||  (r == c.r && g == c.g && b == c.b && a > c.a);
    return result;
}

#pragma mark Arithmetic

/**
 * Clamps this color within the given range.
 *
 * @param min The minimum value.
 * @param max The maximum value.
 *
 * @return A reference to this (modified) Color4f for chaining.
 */
Color4& Color4::clamp(Color4 min, Color4 max) {
    r = clampb(r,min.r,max.r);
    g = clampb(g,min.g,max.g);
    b = clampb(b,min.b,max.b);
    a = clampb(a,min.a,max.a);
    return *this;
}

/**
 * Modifies this color to be the linear interpolation with other.
 *
 * If alpha is 0, the color is unchanged.  If alpha is 1, the color is
 * other.  Otherwise it is a value in between.  If alpha is outside of the
 * range 0 to 1, it is clamped to the nearest value.
 *
 * This method just implements standard linear interpolation.  It does
 * not attempt to give any blending semantics to it.
 *
 * @param other The color to interpolate with.
 * @param alpha The interpolation value in 0..1
 *
 * @return This color, after the interpolation.
 */
Color4& Color4::lerp(const Color4f other, float alpha) {
    float x = clampf(alpha,0,1);
    *this *= (1.f - x);
    return *this += other * x;
}

/**
 * Interpolates the two colors c1 and c2, and stores the result in dst.
 *
 * If alpha is 0, the result is c1.  If alpha is 1, the color is c2.
 * Otherwise it is a value in c1..c2.  If alpha is outside of the
 * range 0 to 1, it is clamped to the nearest value.
 *
 * This method just implements standard linear interpolation.  It does
 * not attempt to give any blending semantics to it.
 *
 * @param c1    The first color.
 * @param c2    The second color.
 * @param alpha The interpolation value in 0..1
 * @param dst   The color to store the result in
 *
 * @return A reference to dst for chaining
 */
Color4* Color4::lerp(Color4 c1, Color4 c2, float alpha, Color4* dst) {
    if (dst == nullptr) { return nullptr; }
    float x = clampf(alpha,0,1);
    *dst = c1 * (1.f - x) + c2 * x;
    return dst;
}

/**
 * Blends this color with the other one, storing the new value in place.
 *
 * The blending is the standard over operation with this color as the
 * destination. It assumes that the color values are not premultiplied.
 *
 * @param other The color to interpolate with.
 *
 * @return This color, after the blending.
 */
Color4& Color4::blend(Color4 other) {
    float srca = COLOR_BYTE_TO_FLOAT(other.a);
    float a1 = COLOR_BYTE_TO_FLOAT(a)*(1-srca);
    float a2 = srca+a1;
    r = clampb((GLubyte)((other.r*srca+r*a1)/a2),0,255);
    g = clampb((GLubyte)((other.g*srca+g*a1)/a2),0,255);
    b = clampb((GLubyte)((other.b*srca+b*a1)/a2),0,255);
    a = COLOR_FLOAT_TO_BYTE(a2);
    return *this;
}

/**
 * Blends the two colors c1 and c2, assuming they are not premultiplied.
 *
 * The blending is the standard over operation with color c1 as the source
 * and c2 as the destination. It assumes that the color values are not
 * premultiplied.
 *
 * @param c1    The source color.
 * @param c2    The destination color.
 * @param dst   The color to store the result in
 *
 * @return A reference to dst for chaining
 */
Color4* Color4::blend(Color4 c1, Color4 c2, Color4* dst) {
    float srca = COLOR_BYTE_TO_FLOAT(c1.a);
    float a1 = COLOR_BYTE_TO_FLOAT(c2.a)*(1-srca);
    float a2 = srca+a1;
    dst->r = clampb((GLubyte)((c1.r*srca+c2.r*a1)/a2),0,255);
    dst->g = clampb((GLubyte)((c1.g*srca+c2.g*a1)/a2),0,255);
    dst->b = clampb((GLubyte)((c1.b*srca+c2.b*a1)/a2),0,255);
    dst->a = COLOR_FLOAT_TO_BYTE(a2);
    return dst;
}

/**
 * Blends this color with the other one, storing the new value in place.
 *
 * The blending is the standard over operation with this color as the
 * destination. It assumes that this color is premultiplied.
 *
 * @param other The color to interpolate with.
 *
 * @return This color, after the blending.
 */
Color4& Color4::blendPre(Color4 other) {
    float srca = COLOR_BYTE_TO_FLOAT(other.a);
    float a1 = srca+COLOR_BYTE_TO_FLOAT(a)*(1-srca);
    r = clampb((GLubyte)(other.r+r*(1-srca)),0,255);
    g = clampb((GLubyte)(other.g+g*(1-srca)),0,255);
    b = clampb((GLubyte)(other.b+b*(1-srca)),0,255);
    a = COLOR_FLOAT_TO_BYTE(a1);
    return *this;
}

/**
 * Blends the two colors c1 and c2, assuming they are premultiplied.
 *
 * The blending is the standard over operation with color c1 as the source
 * and c2 as the destination. It assumes that the color values are
 * premultiplied.
 *
 * @param c1    The source color.
 * @param c2    The destination color.
 * @param dst   The color to store the result in
 *
 * @return A reference to dst for chaining
 */
Color4* Color4::blendPre(Color4 c1, Color4 c2, Color4* dst) {
    float srca = COLOR_BYTE_TO_FLOAT(c1.a);
    float dsta = COLOR_BYTE_TO_FLOAT(c2.a);
    float a = srca+dsta*(1-srca);
    dst->r = clampb((GLubyte)(c1.r+c2.r*(1-srca)),0,255);
    dst->g = clampb((GLubyte)(c1.g+c2.g*(1-srca)),0,255);
    dst->b = clampb((GLubyte)(c1.b+c2.b*(1-srca)),0,255);
    dst->a = COLOR_FLOAT_TO_BYTE(a);
    return dst;
}

/**
 * Returns a blend of this color with the other one.
 *
 * The blending is the standard over operation with this color as the
 * destination. It assumes that the color values are not premultiplied.
 *
 * Note: this does not modify this color.
 *
 * @param other The color to interpolate with.
 *
 * @return The newly blended color
 */
Color4 Color4::getBlend(Color4 other) const {
  float srca = COLOR_BYTE_TO_FLOAT(other.a);
  float a1 = COLOR_BYTE_TO_FLOAT(a)*(1-srca);
  float a2 = srca+a1;
  return Color4(clampb((GLubyte)((other.r*srca+r*a1)/a2), 0, 255),
                clampb((GLubyte)((other.g*srca+g*a1)/a2), 0, 255),
                clampb((GLubyte)((other.b*srca+b*a1)/a2), 0, 255),
                COLOR_FLOAT_TO_BYTE(a2));
}

/**
 * Returns a blend of this color with the other one.
 *
 * The blending is the standard over operation with this color as the
 * destination. It assumes that this color is premultiplied.
 *
 * Note: this does not modify this color.
 *
 * @param other The color to interpolate with.
 *
 * @return The newly blended color
 */
Color4 Color4::getBlendPre(Color4 other) const {
  float srca = COLOR_BYTE_TO_FLOAT(other.a);
  float dsta = COLOR_BYTE_TO_FLOAT(a);
  float a1 = srca+dsta*(1-srca);
  return Color4(clampb((GLubyte)(other.r+r*(1-srca)),0,255),
                clampb((GLubyte)(other.g+g*(1-srca)),0,255),
                clampb((GLubyte)(other.b+b*(1-srca)),0,255),
                COLOR_FLOAT_TO_BYTE(a1));
}

#pragma mark Conversions

/**
 * Returns a string representation of this vector for debuggging purposes.
 *
 * If verbose is true, the string will include class information.  This
 * allows us to unambiguously identify the class.
 *
 * @param verbose Whether to include class information
 *
 * @return a string representation of this vector for debuggging purposes.
 */
std::string Color4::toString(bool verbose) const {
    std::stringstream ss;
    ss << (verbose ? "cugl::Color4[r=" : "[r=");
    ss << strtool::to_string(r);
    ss << ",g=";
    ss << strtool::to_string(g);
    ss << ",b=";
    ss << strtool::to_string(b);
    ss << ",a=";
    ss << strtool::to_string(a);
    ss << "]";
    return ss.str();
}

/** Cast from Color4f to a vector. */
Color4::operator Vec4() const {
    return Vec4(r,g,b,a);
}

/**
 * Creates a color from the given vector.
 *
 * The attributes are read in the order x,y,z,w.  They are all multiplied
 * by 255.0f and rounded up before assignment.
 *
 * @param vector    The vector to convert
 */
Color4::Color4(const Vec4 vector) {
    CUAssertLog(0 <= vector.x && vector.x <= 1, "Red value out of range: %.3f", vector.x);
    CUAssertLog(0 <= vector.y && vector.y <= 1, "Green value out of range: %.3f", vector.y);
    CUAssertLog(0 <= vector.z && vector.z <= 1, "Blue value out of range: %.3f", vector.z);
    CUAssertLog(0 <= vector.z && vector.z <= 1, "Alpha value out of range: %.3f", vector.z);
    r = COLOR_FLOAT_TO_BYTE(vector.x);
    g = COLOR_FLOAT_TO_BYTE(vector.y);
    b = COLOR_FLOAT_TO_BYTE(vector.z);
    a = COLOR_FLOAT_TO_BYTE(vector.w);
}

/**
 * Sets the coordinates of this color to those of the given vector.
 *
 * The attributes are read in the order x,y,z,w.  They are all multiplied
 * by 255.0f and rounded up before assignment.
 *
 * @param vector    The vector to convert
 *
 * @return A reference to this (modified) Color4f for chaining.
 */
Color4& Color4::operator= (const Vec4 vector) {
    CUAssertLog(0 <= vector.x && vector.x <= 1, "Red value out of range: %.3f", vector.x);
    CUAssertLog(0 <= vector.y && vector.y <= 1, "Green value out of range: %.3f", vector.y);
    CUAssertLog(0 <= vector.z && vector.z <= 1, "Blue value out of range: %.3f", vector.z);
    CUAssertLog(0 <= vector.z && vector.z <= 1, "Alpha value out of range: %.3f", vector.z);
    r = COLOR_FLOAT_TO_BYTE(vector.x);
    g = COLOR_FLOAT_TO_BYTE(vector.y);
    b = COLOR_FLOAT_TO_BYTE(vector.z);
    a = COLOR_FLOAT_TO_BYTE(vector.w);
    return *this;
}

/**
 * Cast from Color4f to a vector.
 *
 * The attributes are all divided by 255.0.  The alpha value is dropped.
 */
Color4::operator Vec3() const {
    return Vec3(COLOR_BYTE_TO_FLOAT(r),COLOR_BYTE_TO_FLOAT(g),COLOR_BYTE_TO_FLOAT(b));
}

/**
 * Creates a color from the given vector.
 *
 * The attributes are read in the order x,y,z. The alpha value is 1.
 *
 * @param vector    The vector to convert
 */
Color4::Color4(const Vec3 vector) {
    CUAssertLog(0 <= vector.x && vector.x <= 1, "Red value out of range: %.3f", vector.x);
    CUAssertLog(0 <= vector.y && vector.y <= 1, "Green value out of range: %.3f", vector.y);
    CUAssertLog(0 <= vector.z && vector.z <= 1, "Blue value out of range: %.3f", vector.z);
    r = COLOR_FLOAT_TO_BYTE(vector.x);
    g = COLOR_FLOAT_TO_BYTE(vector.y);
    b = COLOR_FLOAT_TO_BYTE(vector.z);
    a = 255;
}

/**
 * Sets the coordinates of this color to those of the given vector.
 *
 * The attributes are read in the order x,y,z. The alpha value is 1.
 *
 * @param vector    The vector to convert
 *
 * @return A reference to this (modified) Color4f for chaining.
 */
Color4& Color4::operator= (const Vec3 vector) {
    CUAssertLog(0 <= vector.x && vector.x <= 1, "Red value out of range: %.3f", vector.x);
    CUAssertLog(0 <= vector.y && vector.y <= 1, "Green value out of range: %.3f", vector.y);
    CUAssertLog(0 <= vector.z && vector.z <= 1, "Blue value out of range: %.3f", vector.z);
    r = COLOR_FLOAT_TO_BYTE(vector.x);
    g = COLOR_FLOAT_TO_BYTE(vector.y);
    b = COLOR_FLOAT_TO_BYTE(vector.z);
    a = 255;
    return *this;
}

/** Cast from Color4f to a byte-based Color4. */
Color4::operator Color4f() const {
    return Color4f(COLOR_BYTE_TO_FLOAT(r),COLOR_BYTE_TO_FLOAT(g),
                  COLOR_BYTE_TO_FLOAT(b),COLOR_BYTE_TO_FLOAT(a));
}

/**
 * Creates a float-based color from the given byte-based color.
 *
 * The attributes are read in the order r,g,b,a. They are all multiplied by
 * 255.0f and rounded up before assignment.
 *
 * @param color The color to convert
 */
Color4::Color4(const Color4f color) {
    r = COLOR_FLOAT_TO_BYTE(color.r);
    g = COLOR_FLOAT_TO_BYTE(color.g);
    b = COLOR_FLOAT_TO_BYTE(color.b);
    a = COLOR_FLOAT_TO_BYTE(color.a);
}

/**
 * Sets the attributes of this color from the given float-based color.
 *
 * The attributes are read in the order r,g,b,a. They are all multiplied by
 * 255.0f and rounded up before assignment.
 *
 * @param color The color to convert.
 *
 * @return A reference to this (modified) Color4f for chaining.
 */
Color4& Color4::operator= (const Color4f color) {
    r = COLOR_FLOAT_TO_BYTE(color.r);
    g = COLOR_FLOAT_TO_BYTE(color.g);
    b = COLOR_FLOAT_TO_BYTE(color.b);
    a = COLOR_FLOAT_TO_BYTE(color.a);
    return *this;
}

/**
 * Writes the color values into a float array.
 *
 * The attributes are read into the array in the order r,g,b,a. All values
 * are divided by 255.0f (so they are between 0 and 1) before assignment.
 *
 * @param array The array to store the color values.
 *
 * @return A reference to the array for chaining.
 */
float* Color4::get(float* array) const {
    array[0] = COLOR_BYTE_TO_FLOAT(r);
    array[1] = COLOR_BYTE_TO_FLOAT(g);
    array[2] = COLOR_BYTE_TO_FLOAT(b);
    array[3] = COLOR_BYTE_TO_FLOAT(a);
    return array;
}

#pragma mark Constants

/** The Clear color (0,0,0,0) */
const Color4 Color4::CLEAR(0,0,0,0);
/** The White color (1,1,1,1) */
const Color4 Color4::WHITE(255,255,255,255);
/** The Black color (0,0,0,1) */
const Color4 Color4::BLACK(0,0,0,255);
/** The Yellow color (1,1,0,1) */
const Color4 Color4::YELLOW(255,255,0,255);
/** The Blue color (0,0,1,1) */
const Color4 Color4::BLUE(0,0,255,255);
/** The Green color (0,1,0,1) */
const Color4 Color4::GREEN(0,255,0,255);
/** The Red color (1,0,0,1) */
const Color4 Color4::RED(255,0,0,255);
/** The Magenta color (1,0,1,1) */
const Color4 Color4::MAGENTA(255,0,255,255);
/** The Cyan color (0,1,1,1) */
const Color4 Color4::CYAN(0,255,255,255);
/** The Orange color (1,0.5,0,1) */
const Color4 Color4::ORANGE(255,128,0,255);
/** The Gray color (0.65,0.65,0.65,1) */
const Color4 Color4::GRAY(166,166,166,255);
/** The classic XNA color (0.392f,0.584f,0.93f,1,0f) */
const Color4 Color4::CORNFLOWER(100,149,237,255);
/** The Playing Fields color (0.8f,0.8f,0.5f,1.0f) */
const Color4 Color4::PAPYRUS(204,204,128,255);
