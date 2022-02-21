//
//  CUStrings.cpp
//  Cornell University Game Library (CUGL)
//
//  Android does not support a lot of the built-in string methods.  Therefore,
//  we need alternate definitions that are platform agnostic.  Note that these
//  functions have names that are very similar to those in the std namespace,
//  but all live in the cugl::strtool namespace.
//
//  Note that this module does not refer to the integral types as short, int,
//  long, etc.  Those types are NOT cross-platform.  For example, a long is
//  8 bytes on Unix/OS X, but 4 bytes on some Win32 platforms.
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
//  Version: 2/10/21
//
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <utf8/utf8.h>
#include <cugl/util/CUStrings.h>
#include <cugl/util/CUDebug.h>

#if defined (__ANDROID__)
#include <cstdlib>
#endif

namespace cugl {
namespace strtool {

#pragma mark NUMBER TO STRING FUNCTIONS
/**
 * Returns a string equivalent to the given byte
 *
 * The value is displayed as a number, not a character.
 *
 * @param  value    the numeric value to convert
 *
 * @return a string equivalent to the given byte
 */
std::string to_string(Uint8 value) {
#if defined (__ANDROID__)
    std::stringstream ss;
    ss << (Uint32)value;
    return ss.str();
#else
    return std::to_string((Uint32)value);
#endif
}

/**
 * Returns a string equivalent to the given signed 16 bit integer
 *
 * @param  value    the numeric value to convert
 *
 * @return a string equivalent to the given signed 16 bit integer
 */
std::string to_string(Sint16 value) {
#if defined (__ANDROID__)
    std::stringstream ss;
    ss << (Sint32)value;
    return ss.str();
#else
    return std::to_string((Sint32)value);
#endif
}

/**
 * Returns a string equivalent to the given unsigned 16 bit integer
 *
 * @param  value    the numeric value to convert
 *
 * @return a string equivalent to the given unsigned 16 bit integer
 */
std::string to_string(Uint16 value) {
#if defined (__ANDROID__)
    std::stringstream ss;
    ss << (Uint32)value;
    return ss.str();
#else
    return std::to_string((Uint32)value);
#endif
}

/**
 * Returns a string equivalent to the given signed 32 bit integer
 *
 * @param  value    the numeric value to convert
 *
 * @return a string equivalent to the given signed 32 bit integer
 */
std::string to_string(Sint32 value) {
#if defined (__ANDROID__)
    std::stringstream ss;
    ss << value;
    return ss.str();
#else
    return std::to_string(value);
#endif
}

/**
 * Returns a string equivalent to the given unsigned 32 bit integer
 *
 * @param  value    the numeric value to convert
 *
 * @return a string equivalent to the given unsigned 32 bit integer
 */
std::string to_string(Uint32 value ) {
#if defined (__ANDROID__)
    std::stringstream ss;
    ss << value;
    return ss.str();
#else
    return std::to_string(value);
#endif
}

/**
 * Returns a string equivalent to the given signed 64 bit integer
 *
 * @param  value    the numeric value to convert
 *
 * @return a string equivalent to the given signed 64 bit integer
 */
std::string to_string(Sint64 value) {
#if defined (__ANDROID__)
    std::stringstream ss;
    ss << value;
    return ss.str();
#else
    return std::to_string(value);
#endif
}

/**
 * Returns a string equivalent to the given unsigned 64 bit integer
 *
 * @param  value    the numeric value to convert
 *
 * @return a string equivalent to the given unsigned 64 bit integer
 */
std::string to_string(Uint64 value ) {
#if defined (__ANDROID__)
    std::stringstream ss;
    ss << value;
    return ss.str();
#else
    return std::to_string(value);
#endif
}

/**
 * Returns a string equivalent to the given float value
 *
 * This function differs from std::to_string(float) in that it allows us
 * to specify a precision (the number of digits to display after the decimal
 * point).  If precision is negative, then maximum precision will be used.
 *
 * @param  value        the numeric value to convert
 * @param  precision    the number of digits to display after the decimal
 *
 * @return a string equivalent to the given float value
 */
std::string to_string(float value, int precision) {
    int width = (precision >= 0 ? precision : std::numeric_limits<long double>::digits10 + 1);
    std::stringstream ss;
    ss << std::fixed << std::setprecision(width) << value;
    return ss.str();
}

/**
 * Returns a string equivalent to the given double value
 *
 * This function differs from std::to_string(double) in that it allows us
 * to specify a precision (the number of digits to display after the decimal
 * point).  If precision is negative, then maximum precision will be used.
 *
 * @param  value        the numeric value to convert
 * @param  precision    the number of digits to display after the decimal
 *
 * @return a string equivalent to the given double value
 */
std::string to_string(double value, int precision) {
    int width = (precision >= 0 ? precision : std::numeric_limits<long double>::digits10 + 1);
    std::stringstream ss;
    ss << std::fixed << std::setprecision(width) << value;
    return ss.str();
}


#pragma mark -
#pragma mark ARRAY TO STRING FUNCTIONS
/**
 * Returns a string equivalent to the given byte array
 *
 * The value is display as a python-style list in brackets.
 *
 * @param array     the array to convert
 * @param length    the array length
 * @param offset    the starting position in the array
 *
 * @return a string equivalent to the given byte array
 */
std::string to_string(Uint8* array, size_t length, size_t offset) {
    std::stringstream ss;
    ss << "[";
    for(int ii = 0; ii < length; ii++) {
        ss << (Uint32)array[ii+offset];
        if (ii != length-1) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
}

/**
 * Returns a string equivalent to the signed 16 bit integer array
 *
 * The value is display as a python-style list in brackets.
 *
 * @param array     the array to convert
 * @param length    the array length
 * @param offset    the starting position in the array
 *
 * @return a string equivalent to the signed 16 bit integer array
 */
std::string to_string(Sint16* array, size_t length, size_t offset) {
    std::stringstream ss;
    ss << "[";
    for(int ii = 0; ii < length; ii++) {
        ss << array[ii+offset];
        if (ii != length-1) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
}

/**
 * Returns a string equivalent to the unsigned 16 bit integer array
 *
 * The value is display as a python-style list in brackets.
 *
 * @param array     the array to convert
 * @param length    the array length
 * @param offset    the starting position in the array
 *
 * @return a string equivalent to the unsigned 16 bit integer array
 */
std::string to_string(Uint16* array, size_t length, size_t offset) {
    std::stringstream ss;
    ss << "[";
    for(int ii = 0; ii < length; ii++) {
        ss << array[ii+offset];
        if (ii != length-1) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
}


/**
 * Returns a string equivalent to the signed 32 bit integer array
 *
 * The value is display as a python-style list in brackets.
 *
 * @param array     the array to convert
 * @param length    the array length
 * @param offset    the starting position in the array
 *
 * @return a string equivalent to the signed 32 bit integer array
 */
std::string to_string(Sint32* array, size_t length, size_t offset) {
    std::stringstream ss;
    ss << "[";
    for(int ii = 0; ii < length; ii++) {
        ss << array[ii+offset];
        if (ii != length-1) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
}


/**
 * Returns a string equivalent to the unsigned 32 bit integer array
 *
 * The value is display as a python-style list in brackets.
 *
 * @param array     the array to convert
 * @param length    the array length
 * @param offset    the starting position in the array
 *
 * @return a string equivalent to the unsigned 32 bit integer array
 */
std::string to_string(Uint32* array, size_t length, size_t offset) {
    std::stringstream ss;
    ss << "[";
    for(int ii = 0; ii < length; ii++) {
        ss << array[ii+offset];
        if (ii != length-1) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
}

/**
 * Returns a string equivalent to the signed 64 bit integer array
 *
 * The value is display as a python-style list in brackets.
 *
 * @param array     the array to convert
 * @param length    the array length
 * @param offset    the starting position in the array
 *
 * @return a string equivalent to the signed 64 bit integer array
 */
std::string to_string(Sint64* array, size_t length, size_t offset) {
    std::stringstream ss;
    ss << "[";
    for(int ii = 0; ii < length; ii++) {
        ss << array[ii+offset];
        if (ii != length-1) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
}


/**
 * Returns a string equivalent to the unsigned 64 bit integer array
 *
 * The value is display as a python-style list in brackets.
 *
 * @param array     the array to convert
 * @param length    the array length
 * @param offset    the starting position in the array
 *
 * @return a string equivalent to the unsigned 64 bit integer array
 */
std::string to_string(Uint64* array, size_t length, size_t offset) {
    std::stringstream ss;
    ss << "[";
    for(int ii = 0; ii < length; ii++) {
        ss << array[ii+offset];
        if (ii != length-1) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
}


/**
 * Returns a string equivalent to the given float array
 *
 * The value is display as a python-style list in brackets.
 *
 * As with to_string(float), this function allows us to specify a precision
 * (the number of digits to display after the decimal point).  If precision is
 * negative, then maximum precision will be used.
 *
 * @param array     the array to convert
 * @param length    the array length
 * @param offset    the starting position in the array
 *
 * @return a string equivalent to the given float array
 */
std::string to_string(float* array, size_t length, size_t offset, int precision) {
    int width = (precision >= 0 ? precision : std::numeric_limits<long double>::digits10 + 1);
    std::stringstream ss;
    ss << "[" << std::fixed << std::setprecision(width);
    for(int ii = 0; ii < length; ii++) {
        ss << array[ii+offset] << 'f';
        if (ii != length-1) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
}

/**
 * Returns a string equivalent to the given double array
 *
 * The value is display as a python-style list in brackets.
 *
 * As with to_string(double), this function allows us to specify a precision
 * (the number of digits to display after the decimal point).  If precision is
 * negative, then maximum precision will be used.
 *
 * @param array     the array to convert
 * @param length    the array length
 * @param offset    the starting position in the array
 *
 * @return a string equivalent to the given double array
 */
std::string to_string(double* array, size_t length, size_t offset, int precision) {
    int width = (precision >= 0 ? precision : std::numeric_limits<long double>::digits10 + 1);
    std::stringstream ss;
    ss << "[" << std::fixed << std::setprecision(width);
    for(int ii = 0; ii < length; ii++) {
        ss << array[ii+offset];
        if (ii != length-1) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
}


#pragma mark -
#pragma mark STRING TO NUMBER FUNCTIONS

/**
 * Returns the byte equivalent to the given string
 *
 * This function discards any whitespace characters (as identified by calling isspace())
 * until the first non-whitespace character is found, then takes as many characters
 * as possible to form a valid base-n (where n=base) integer number representation
 * and converts them to an integer value.
 *
 * @param  str  the string to convert
 * @param  pos  address of an integer to store the number of characters processed
 * @param  base the number base
 *
 * @return the byte equivalent to the given string
 */
Uint8 stou8(const std::string str, std::size_t* pos, int base) {
#if defined (__ANDROID__)
    const char* start = str.c_str();
    char* end;
    int result = (Uint8)std::strtol(start, &end, base);
    *pos = (std::size_t)(end-start); // Bad but no alternative on android
    return result;
#else
    return (Uint8)std::stoi(str,pos,base);
#endif
}

/**
 * Returns the signed 16 bit integer equivalent to the given string
 *
 * This function discards any whitespace characters (as identified by calling isspace())
 * until the first non-whitespace character is found, then takes as many characters
 * as possible to form a valid base-n (where n=base) integer number representation
 * and converts them to a long value.
 *
 * @param  str  the string to convert
 * @param  pos  address of an integer to store the number of characters processed
 * @param  base the number base
 *
 * @return the signed 16 bit integer equivalent to the given string
 */
Sint16 stos16(const std::string str, std::size_t* pos, int base)  {
#if defined (__ANDROID__)
    const char* start = str.c_str();
    char* end;
    int result = (Sint16)std::strtol(start, &end, base);
    *pos = (std::size_t)(end-start); // Bad but no alternative on android
    return result;
#else
    return (Sint16)std::stoi(str,pos,base);
#endif
}

/**
 * Returns the unsigned 16 bit integer equivalent to the given string
 *
 * This function discards any whitespace characters (as identified by calling isspace())
 * until the first non-whitespace character is found, then takes as many characters
 * as possible to form a valid base-n (where n=base) integer number representation
 * and converts them to a long value.
 *
 * @param  str  the string to convert
 * @param  pos  address of an integer to store the number of characters processed
 * @param  base the number base
 *
 * @return the unsigned 16 bit integer equivalent to the given string
 */
Uint16 stou16(const std::string str, std::size_t* pos, int base) {
#if defined (__ANDROID__)
    const char* start = str.c_str();
    char* end;
    int result = (Uint16)std::strtol(start, &end, base);
    *pos = (std::size_t)(end-start); // Bad but no alternative on android
    return result;
#else
    return (Uint16)std::stol(str,pos,base);
#endif
}

/**
 * Returns the signed 32 bit integer equivalent to the given string
 *
 * This function discards any whitespace characters (as identified by calling isspace())
 * until the first non-whitespace character is found, then takes as many characters
 * as possible to form a valid base-n (where n=base) integer number representation
 * and converts them to a long value.
 *
 * @param  str  the string to convert
 * @param  pos  address of an integer to store the number of characters processed
 * @param  base the number base
 *
 * @return the signed 32 bit integer equivalent to the given string
 */
Sint32 stos32(const std::string str, std::size_t* pos, int base) {
#if defined (__ANDROID__)
    const char* start = str.c_str();
    char* end;
    int result = (Sint32)std::strtol(start, &end, base);
    *pos = (std::size_t)(end-start); // Bad but no alternative on android
    return result;
#else
    return (Sint32)std::stol(str,pos,base);
#endif
}

/**
 * Returns the unsigned 32 bit integer equivalent to the given string
 *
 * This function discards any whitespace characters (as identified by calling isspace())
 * until the first non-whitespace character is found, then takes as many characters
 * as possible to form a valid base-n (where n=base) integer number representation
 * and converts them to a long value.
 *
 * @param  str  the string to convert
 * @param  pos  address of an integer to store the number of characters processed
 * @param  base the number base
 *
 * @return the unsigned 32 bit integer equivalent to the given string
 */
Uint32 stou32(const std::string str, std::size_t* pos, int base) {
#if defined (__ANDROID__)
    const char* start = str.c_str();
    char* end;
    int result = (Uint32)std::strtoul(start, &end, base);
    *pos = (std::size_t)(end-start); // Bad but no alternative on android
    return result;
#else
    return (Uint32)std::stoul(str,pos,base);
#endif
}

/**
 * Returns the signed 64 bit integer equivalent to the given string
 *
 * This function discards any whitespace characters (as identified by calling isspace())
 * until the first non-whitespace character is found, then takes as many characters
 * as possible to form a valid base-n (where n=base) integer number representation
 * and converts them to a long value.
 *
 * @param  str  the string to convert
 * @param  pos  address of an integer to store the number of characters processed
 * @param  base the number base
 *
 * @return the signed 64 bit integer equivalent to the given string
 */
Sint64 stos64(const std::string str, std::size_t* pos, int base) {
#if defined (__ANDROID__)
    const char* start = str.c_str();
    char* end;
    int result = (Sint64)std::strtoll(start, &end, base);
    *pos = (std::size_t)(end-start); // Bad but no alternative on android
    return result;
#else
    return (Sint64)std::stoll(str,pos,base);
#endif
}


/**
 * Returns the unsigned 64 bit integer equivalent to the given string
 *
 * This function discards any whitespace characters (as identified by calling isspace())
 * until the first non-whitespace character is found, then takes as many characters
 * as possible to form a valid base-n (where n=base) integer number representation
 * and converts them to a long value.
 *
 * @param  str  the string to convert
 * @param  pos  address of an integer to store the number of characters processed
 * @param  base the number base
 *
 * @return the unsigned 64 bit integer equivalent to the given string
 */
Uint64 stou64(const std::string str, std::size_t* pos, int base) {
#if defined (__ANDROID__)
    const char* start = str.c_str();
    char* end;
    int result = (Uint64)std::strtoull(start, &end, base);
    *pos = (std::size_t)(end-start); // Bad but no alternative on android
    return result;
#else
    return (Uint64)std::stoull(str,pos,base);
#endif
}

/**
 * Returns the float equivalent to the given string
 *
 * This function discards any whitespace characters (as determined by std::isspace())
 * until first non-whitespace character is found. Then it takes as many characters as
 * possible to form a valid floating point representation and converts them to a floating
 * point value.
 *
 * @param  str  the string to convert
 * @param  pos  address of an integer to store the number of characters processed
 *
 * @return the float equivalent to the given string
 */
float  stof(const std::string str, std::size_t* pos) {
#if defined (__ANDROID__)
    const char* start = str.c_str();
    char* end;
    float result = (float)std::strtod(start, &end);
    *pos = (std::size_t)(end-start); // Bad but no alternative on android
    return result;
#else
    return std::stof(str,pos);
#endif
}

/**
 * Returns the double equivalent to the given string
 *
 * This function discards any whitespace characters (as determined by std::isspace())
 * until first non-whitespace character is found. Then it takes as many characters as
 * possible to form a valid floating point representation and converts them to a floating
 * point value.
 *
 * @param  str  the string to convert
 * @param  pos  address of an integer to store the number of characters processed
 *
 * @return the double equivalent to the given string
 */
double stod(const std::string str, std::size_t* pos) {
#if defined (__ANDROID__)
    const char* start = str.c_str();
    char* end;
    double result = std::strtod(start, &end);
    *pos = (std::size_t)(end-start); // Bad but no alternative on android
    return result;
#else
    return std::stod(str,pos);
#endif
}

#pragma mark -
#pragma mark QUERY FUNCTIONS
/**
 * Returns true if the string only contains alphabetic characters.
 *
 * This function uses the current C++ locale.
 *
 * @param str   The string to check
 *
 * @return true if the string only contains alphabetic characters.
 */
bool isalpha(const std::string str) {
    std::locale loc;
    bool result = true;
    for(auto it = str.begin(); result && it != str.end(); ++it) {
        result = result && std::isalpha(*it, loc);
    }
    return result;
}

/**
 * Returns true if the string only contains alphabetic and numeric characters.
 *
 * This function uses the current C++ locale.
 *
 * @param str   The string to check
 *
 * @return true if the string only contains alphabetic and numeric characters.
 */
bool isalphanum(const std::string str) {
    std::locale loc;
    bool result = true;
    for(auto it = str.begin(); result && it != str.end(); ++it) {
        result = result && std::isalnum(*it, loc);
    }
    return result;
}

/**
 * Returns true if the string only contains numeric characters.
 *
 * This function uses the current C++ locale.
 *
 * @param str   The string to check
 *
 * @return true if the string only contains numeric characters.
 */
bool isnumeric(const std::string str) {
    std::locale loc;
    bool result = true;
    for(auto it = str.begin(); result && it != str.end(); ++it) {
        result = result && std::isdigit(*it, loc);
    }
    return result;
}

/**
 * Returns true if the string can safely be converted to a number (double)
 *
 * This function uses the current C++ locale.
 *
 * @param str   The string to check
 *
 * @return true if the string can safely be converted to a number (double)
 */
bool isnumber(const std::string str) {
    size_t p;
    cugl::strtool::stod(str, &p);
    return p != 0;
}

/**
 * Returns the number of times substring a appears in str.
 *
 * Overlapping substring count.  So count("aaa","aa") returns 2.
 *
 * @param str   The string to count from
 * @param a     The substring to count
 *
 * @return the number of times substring a appears in str.
 */
int count(const std::string str, const std::string a) {
    int result = 0;
    size_t pos = str.find(a);
    while( pos != std::string::npos) {
        result++;
        pos = str.find(a,pos+1);
    }
    return result;
}

/**
 * Returns true if str starts with the substring a.
 *
 * @param str   The string to query
 * @param a     The substring to match
 *
 * @return true if str starts with the substring a.
 */
bool starts_with(const std::string str, const std::string a) {
    size_t pos = str.find(a);
    return pos == 0;
}

/**
 * Returns true if str ends with the substring a.
 *
 * @param str   The string to query
 * @param a     The substring to match
 *
 * @return true if str ends with the substring a.
 */
bool ends_with(const std::string str, const std::string a) {
    if (str.size() < a.size()) {
        return false;
    }

    size_t offset = str.size()-a.size();
    size_t pos = str.find(a);
    return pos == offset;
}

/**
 * Returns true if the string is lower case
 *
 * This method ignores any non-letter characters and returns true
 * if str is the empty string.  So the only way it can be false is if
 * there is an upper case letter in the string.
 *
 * This function uses the current C++ locale.
 *
 * @param str   The string to query
 *
 * @return true if the string is lower case
 */
bool islower(const std::string str) {
    bool result = true;
    std::locale loc;
    for(auto it = str.begin(); result && it != str.end(); ++it) {
        if (*it != std::tolower(*it, loc)) {
            result = false;
        }
    }
    return result;
}

/**
 * Returns true if the string is upper case
 *
 * This method ignores any non-letter characters and returns true
 * if str is the empty string.  So the only way it can be false is if
 * there is a lower case letter in the string.
 *
 * This function uses the current C++ locale.
 *
 * @param str   The string to query
 *
 * @return true if the string is upper case
 */
bool isupper(const std::string str) {
    bool result = true;
    std::locale loc;
    for(auto it = str.begin(); result && it != str.end(); ++it) {
        if (*it != std::toupper(*it, loc)) {
            result = false;
        }
    }
    return result;
}




#pragma mark -
#pragma mark CONVERSION FUNCTIONS
/**
 * Returns a list of substrings separated by the given separator
 *
 * The separator is interpretted exactly; no whitespace is remove around
 * the separator.  If the separator is the empty string, this function
 * will return a list of the characters in str.
 *
 * @param str   The string to split
 * @param sep   The splitting delimeter
 *
 * @return a list of substrings separate by the given separator
 */
std::vector<std::string> split(const std::string str, const std::string sep) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = str.find(sep,start);
    while (end != std::string::npos) {
        result.push_back(str.substr(start,end-start));
        start = end+sep.size();
        end = str.find(sep,start);
    }
    result.push_back(str.substr(start,end-start));
    return result;
}

/**
 * Returns a list of substrings separated by the line separator
 *
 * This function treats both newlines and carriage returns as line
 * separators. Windows-style line separators (CR+NL) do not
 * produce an extra line in the middle.
 *
 * @param str   The string to split
 *
 * @return a list of substrings separate by the line separator
 */
std::vector<std::string> splitlines(const std::string str) {
    std::vector<std::string> result;
    size_t start = 0;
    for (size_t end = 1; end < str.size(); end++) {
        bool push = false;
        if (str[end] == '\r') {
            push = true;
        } else if (str[end] == '\n') {
            if (!(start == end && start > 0 && str[start-1] == '\r')) {
                push = true;
            } else {
                start++;
            }
        }
        if (push) {
            result.push_back(str.substr(start,end-start));
            start = end+1;
        }
    }
    if (start < str.size()) {
        result.push_back(str.substr(start));
    }
    return result;
}

/**
 * Returns a string that is the concatentation of elts.
 *
 * The string sep is placed between each concatentated item of elts.
 * If elts is one element or none, then sep is ignored.
 *
 * @param sep   The join separate
 * @param elts  The strings to join
 *
 * @return a string that is the concatentation of elts.
 */
std::string join(const std::string sep, std::initializer_list<std::string> elts) {
    size_t s = elts.size();
    std::stringstream ss;
    for(auto it = elts.begin(); it != elts.end(); ++it) {
        ss << *it;
        if (s > 0) {
            ss << sep;
        }
        s--;
    }
    return ss.str();
}

/**
 * Returns a string that is the concatentation of elts.
 *
 * The string sep is placed between each concatentated item of elts.
 * If elts is one element or none, then sep is ignored.
 *
 * @param sep   The join separate
 * @param elts  The strings to join
 *
 * @return a string that is the concatentation of elts.
 */
std::string join(const std::string sep, std::vector<std::string> elts) {
    size_t s = elts.size();
    std::stringstream ss;
    for(auto it = elts.begin(); it != elts.end(); ++it) {
        ss << *it;
        if (s > 0) {
            ss << sep;
        }
        s--;
    }
    return ss.str();
}

/**
 * Returns a string that is the concatentation of elts.
 *
 * The string sep is placed between each concatentated item of elts.
 * If elts is one element or none, then sep is ignored.
 *
 * @param sep   The join separate
 * @param elts  The strings to join
 * @param size  The number of items in elts.
 *
 * @return a string that is the concatentation of elts.
 */
std::string join(const std::string sep, const std::string* elts, size_t size) {
    std::stringstream ss;
    for(size_t ii = 0; ii < size; ii++) {
        ss << elts[ii];
        if (ii < size-1) {
            ss << sep;
        }
    }
    return ss.str();

}

/**
 * Returns a copy of str with any leading and trailing whitespace removed.
 *
 * This function uses the current C++ locale.
 *
 * @param str   The string to trim
 *
 * @return a copy of str with any leading and trailing whitespace removed.
 */
std::string trim(const std::string str) {
    if (str.empty()) {
        return str;
    }

    std::locale loc;
    std::string::size_type pos1 = std::string::npos;
    for(std::string::size_type ii = 0; pos1 == std::string::npos && ii < str.size(); ii++) {
        if (!std::isspace(str[ii], loc)) {
            pos1 = ii;
        }
    }
    if (pos1 == std::string::npos) {
        return std::string("");
    }
    std::string::size_type pos2 = std::string::npos;
    for(std::string::size_type ii = str.size()-1; pos2 == std::string::npos && ii > 0; ii--) {
        if (!std::isspace(str[ii], loc)) {
            pos2 = ii;
        }
    }
    if (pos2 == std::string::npos && !std::isspace(str[0], loc)) {
        pos2 = 0;
    }
    return str.substr(pos1, pos2-pos1+1);
}

/**
 * Returns a copy of str with any leading whitespace removed.
 *
 * This function uses the current C++ locale.
 *
 * @param str   The string to trim
 *
 * @return a copy of str with any leading whitespace removed.
 */
std::string ltrim(const std::string str) {
    if (str.empty()) {
        return str;
    }

    std::locale loc;
    std::string::size_type pos1 = std::string::npos;
    for(std::string::size_type ii = 0; pos1 == std::string::npos && ii < str.size(); ii++) {
        if (!std::isspace(str[ii], loc)) {
            pos1 = ii;
        }
    }
    if (pos1 == std::string::npos) {
        return "";
    }
    return str.substr(pos1);
}

/**
 * Returns a copy of str with any trailing whitespace removed.
 *
 * This function uses the current C++ locale.
 *
 * @param str   The string to trim
 *
 * @return a copy of str with any trailing whitespace removed.
 */
std::string rtrim(const std::string str) {
    if (str.empty()) {
        return str;
    }

    std::locale loc;
    std::string::size_type pos2 = std::string::npos;
    for(std::string::size_type ii = str.size()-1; pos2 == std::string::npos && ii > 0; ii--) {
        if (!std::isspace(str[ii], loc)) {
            pos2 = ii;
        }
    }
    if (pos2 == std::string::npos) {
        if (!std::isspace(str[0], loc)) {
            pos2 = 0;
        } else {
            return "";
        }
    }
    return str.substr(0, pos2+1);
}

/**
 * Returns a lower case copy of str.
 *
 * This function uses the current C++ locale.
 *
 * @param str   The string to convert
 *
 * @return a lower case copy of str.
 */
std::string tolower(const std::string str) {
    std::locale loc;
    std::string result;
    for(auto it = str.begin(); it != str.end(); ++it) {
        result.push_back(std::tolower(*it, loc));
    }
    return result;
}

/**
 * Returns an upper case copy of str.
 *
 * This function uses the current C++ locale.
 *
 * @param str   The string to convert
 *
 * @return an upper case copy of str.
 */
std::string toupper(const std::string str) {
    std::locale loc;
    std::string result;
    for(auto it = str.begin(); it != str.end(); ++it) {
        result.push_back(std::tolower(*it, loc));
    }
    return result;
}

/**
 * Returns a copy of str with the first instance of a replaced by b.
 *
 * If a is not a substring of str, the function returns an unmodified copy
 * of str.
 *
 * @param str   The string to modify
 * @param a     The substring to replace
 * @param b     The substring to substitute
 *
 * @return a copy of str with the first instance of a replaced by b.
 */
std::string replace(const std::string str, const std::string a, const std::string b) {
    std::string result(str);
    size_t start_pos = result.find(a);
    if(start_pos == std::string::npos)
        return result;
    result.replace(start_pos, a.length(), b);
    return result;
}

/**
 * Returns a copy of str with all instances of a replaced by b.
 *
 * If a is not a substring of str, the function returns an unmodified copy
 * of str.
 *
 * @param str   The string to modify
 * @param a     The substring to replace
 * @param b     The substring to substitute
 *
 * @return a copy of str with all instances of a replaced by b.
 */
std::string replaceall(const std::string str, const std::string a, const std::string b) {
    std::string result(str);
    size_t start_pos = result.find(a);
    while (start_pos != std::string::npos) {
        result.replace(start_pos, a.length(), b);
        start_pos = result.find(a,start_pos+b.length());
    }
    return result;
}

#pragma mark -
#pragma mark UNICODE PROCESSING
/**
 * Returns the unicode type for the given unicode code point
 *
 * A unicode code point is the 32-bit representation of a character. It is
 * endian specific and therefore not serializable. A UTF8 representation
 * should be used for serialization.
 *
 * @param code  The unicode code point
 *
 * @return the unicode type for the given unicode code point
 */
UnicodeType getUnicodeType(Uint32 code) {
    // Quick checks
    switch (code) {
        case 9:         // \t
        case 32:        // SPACE
        case 0x00a0:    // NBSP
            return UnicodeType::SPACE;
            break;
        case 10:        // \n
        case 13:        // \r
        case 0x0085:    // NEL
            return UnicodeType::NEWLINE;
            break;
    }

    if (code == 0 || code == 11 || code == 12 ||
        (0x001c <= code && code <= 0x001f)) {
        return UnicodeType::CONTROL;
    }

    // CJK characters are special for word selection.
    if ((code >= 0x4E00 && code <= 0x9FFF) ||
               (code >= 0x3000 && code <= 0x30FF) ||
               (code >= 0xFF00 && code <= 0xFFEF) ||
               (code >= 0x1100 && code <= 0x11FF) ||
               (code >= 0x3130 && code <= 0x318F) ||
               (code >= 0xAC00 && code <= 0xD7AF)) {
        return UnicodeType::CJK;
    }
    
    return UnicodeType::CHAR;
}

/**
 * Returns the unicode type for the FIRST character of str.
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically. Note that a character of a UTF8 string
 * may comprise multiple elements of the string type.
 *
 * @param str   The string to test
 *
 * @return the unicode type for the FIRST character of str.
 */
UnicodeType getUnicodeType(const std::string str) {
    CUAssertLog(str.size() > 0, "String %s is empty", str.c_str());
    const char* begin = str.c_str();
    const char* end = begin+str.size();
    return getUnicodeType(utf8::next(begin,end));
}

/**
 * Returns the unicode type for the FIRST character of substr.
 *
 * The C-style string substr need not be null-terminated. Instead,
 * the termination is indicated by the parameter end. This provides
 * efficient substring processing.
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically. Note that a character of a UTF8 string
 * may comprise multiple elements of the string type.
 *
 * @param substr    The start of the string to test
 * @param end       The end of the string to test
 *
 * @return the unicode type for the FIRST character of substr.
 */
UnicodeType getUnicodeType(const char* substr, const char* end) {
    CUAssertLog(substr != end, "The substring is empty");
    return getUnicodeType(utf8::next(substr,end));
}

/**
 * Returns the code points for the elements of str.
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically. Note that for an ASCII string, this will
 * simply fill the vector with the elements of str.
 *
 * @param str   The string to convert
 *
 * @return the code points for the elements of str.
 */
std::vector<Uint32> getCodePoints(const std::string str) {
    std::vector<Uint32> utf32;
    const char* begin = str.c_str();
    const char* end = begin+str.size();
    utf8::utf8to32(begin, end, back_inserter(utf32));
    return utf32;
}

/**
 * Returns the code points for the elements of substr.
 *
 * The C-style string substr need not be null-terminated. Instead,
 * the termination is indicated by the parameter end. This provides
 * efficient substring processing.
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically. Note that for an ASCII string, this will
 * simply fill the vector with the elements of str.
 *
 * @param str   The string to convert
 *
 * @return the code points for the elements of substr.
 */
std::vector<Uint32> getCodePoints(const char* substr, const char* end) {
    std::vector<Uint32> utf32;
    utf8::utf8to32(substr, end, back_inserter(utf32));
    return utf32;
}

/**
 * Returns the length of str in UTF8 encoding
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically. Note that for an ASCII string, this will
 * simply return the size of the string.
 *
 * @param str   The string to convert
 *
 * @return the length of str in UTF8 encoding
 */
size_t getUTF8Length(const std::string str) {
    const char* begin = str.c_str();
    const char* end = begin+str.size();
    size_t total = 0;
    while (begin != end) {
        utf8::next(begin,end);
        total++;
    }
    return total;
}

/**
 * Returns the length of substr in UTF8 encoding
 *
 * The C-style string substr need not be null-terminated. Instead,
 * the termination is indicated by the parameter end. This provides
 * efficient substring processing.
 *
 * The string may either be in UTF8 or ASCII; the method will handle
 * conversion automatically. Note that for an ASCII string, this will
 * simply return the size of the string.
 *
 * @param str   The string to convert
 *
 * @return the length of substr in UTF8 encoding
 */
size_t getUTF8Length(const char* substr, const char* end)  {
    CUAssertLog(substr <= end, "Invalid end position for substring");
    const char* begin = substr;
    size_t total = 0;
    while (begin != end) {
        utf8::next(begin,end);
        total++;
    }
    return total;
}

}
}
