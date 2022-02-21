//
//  CUStrings.h
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
#ifndef __CU_STRINGS_H_
#define __CU_STRINGS_H_
#include <SDL/SDL.h>
#include <vector>
#include <string>
#include <locale>
#include <initializer_list>
#include <algorithm>

namespace cugl {
	/**
	 * A platform independent module for generating strings.
	 *
	 * The functions in this namespace are necessary because Android does not support 
	 * a lot of the built-in string methods.  Therefore, we need alternate definitions 
	 * that are platform agnostic. Note that these functions have names that are very 
	 * similar to those in the std namespace, but all live in the cugl::strtool namespace.
	 *
 	 * Note that this module does not refer to the integral types as short, int,
     * long, etc.  Those types are NOT cross-platform.  For example, a long is
	 * 8 bytes on Unix/OS X, but 4 bytes on some Win32 platforms. Hence we use the 
	 * SDL naming convention instead.
	 */
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
    std::string to_string(Uint8 value);

    /**
     * Returns a string equivalent to the given signed 16 bit integer
     *
     * @param  value    the numeric value to convert
     *
     * @return a string equivalent to the given signed 16 bit integer
     */
    std::string to_string(Sint16 value);
    
    /**
     * Returns a string equivalent to the given unsigned 16 bit integer
     *
     * @param  value    the numeric value to convert
     *
     * @return a string equivalent to the given unsigned 16 bit integer
     */
    std::string to_string(Uint16 value);
    
    /**
     * Returns a string equivalent to the given signed 32 bit integer
     *
     * @param  value    the numeric value to convert
     *
     * @return a string equivalent to the given signed 32 bit integer
     */
    std::string to_string(Sint32 value);
    
    /**
     * Returns a string equivalent to the given unsigned 32 bit integer
     *
     * @param  value    the numeric value to convert
     *
     * @return a string equivalent to the given unsigned 32 bit integer
     */
    std::string to_string(Uint32 value );
    
    /**
     * Returns a string equivalent to the given signed 64 bit integer
     *
     * @param  value    the numeric value to convert
     *
     * @return a string equivalent to the given signed 64 bit integer
     */
    std::string to_string(Sint64 value );
    
    /**
     * Returns a string equivalent to the given unsigned 64 bit integer
     *
     * @param  value    the numeric value to convert
     *
     * @return a string equivalent to the given unsigned 64 bit integer
     */
    std::string to_string(Uint64 value );
    
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
    std::string to_string(float value, int precision=-1);
    
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
    std::string to_string(double value, int precision=-1);

    
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
    std::string to_string(Uint8* array, size_t length, size_t offset=0);
    
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
    std::string to_string(Sint16* array, size_t length, size_t offset=0);
    
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
    std::string to_string(Uint16* array, size_t length, size_t offset=0);
    
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
    std::string to_string(Sint32* array, size_t length, size_t offset=0);
    
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
    std::string to_string(Uint32* array, size_t length, size_t offset=0);
    
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
    std::string to_string(Sint64* array, size_t length, size_t offset=0);
    
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
    std::string to_string(Uint64* array, size_t length, size_t offset=0);
    
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
     * @param precision the number of digits to display after the decimal
     *
     * @return a string equivalent to the given float array
     */
    std::string to_string(float* array, size_t length, size_t offset=0, int precision=-1);
    
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
     * @param precision the number of digits to display after the decimal
     *
     * @return a string equivalent to the given double array
     */
    std::string to_string(double* array, size_t length, size_t offset=0, int precision=-1);
    
    
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
    Uint8 stou8(const std::string str, std::size_t* pos = 0, int base = 10);
    
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
    Sint16 stos16(const std::string str, std::size_t* pos = 0, int base = 10);
    
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
    Uint16 stou16(const std::string str, std::size_t* pos = 0, int base = 10);
    
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
    Sint32 stos32(const std::string str, std::size_t* pos = 0, int base = 10);
    
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
    Uint32 stou32(const std::string str, std::size_t* pos = 0, int base = 10);
    
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
    Sint64 stos64(const std::string str, std::size_t* pos = 0, int base = 10);
    
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
    Uint64 stou64(const std::string str, std::size_t* pos = 0, int base = 10);
    
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
    float stof(const std::string str, std::size_t* pos = 0);
    
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
    double stod(const std::string str, std::size_t* pos = 0);

    
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
    bool isalpha(const std::string str);

    /**
     * Returns true if the string only contains alphabetic and numeric characters.
     *
     * This function uses the current C++ locale.
     *
     * @param str   The string to check
     *
     * @return true if the string only contains alphabetic and numeric characters.
     */
    bool isalphanum(const std::string str);

    /**
     * Returns true if the string only contains numeric characters.
     *
     * This function uses the current C++ locale.
     *
     * @param str   The string to check
     *
     * @return true if the string only contains numeric characters.
     */
    bool isnumeric(const std::string str);

    /**
     * Returns true if the string can safely be converted to a number (double)
     *
     * This function uses the current C++ locale.
     *
     * @param str   The string to check
     *
     * @return true if the string can safely be converted to a number (double)
     */
    bool isnumber(const std::string str);

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
    int count(const std::string str, const std::string a);

    /**
     * Returns true if str starts with the substring a.
     *
     * @param str   The string to query
     * @param a     The substring to match
     *
     * @return true if str starts with the substring a.
     */
    bool starts_with(const std::string str, const std::string a);
    
    /**
     * Returns true if str ends with the substring a.
     *
     * @param str   The string to query
     * @param a     The substring to match
     *
     * @return true if str ends with the substring a.
     */
    bool ends_with(const std::string str, const std::string a);
    
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
    bool islower(const std::string str);

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
    bool isupper(const std::string str);
    
    
#pragma mark -
#pragma mark UTILITY FUNCTIONS
    /**
     * Returns a list of substrings separate by the given separator
     *
     * The separator is interpretted exactly; no whitespace is removed around
     * the separator.  If the separator is the empty string, this function
     * will return a list of the characters in str.
     *
     * @param str   The string to split
     * @param sep   The splitting delimeter
     *
     * @return a list of substrings separate by the given separator
     */
    std::vector<std::string> split(const std::string str, const std::string sep);
    
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
    std::vector<std::string> splitlines(const std::string str);
    
    /**
     * Returns a string that is the concatenation of elts.
     *
     * The string sep is placed between each concatenated item of elts.
     * If elts is one element or none, then sep is ignored.
     *
     * @param sep   The join separator
     * @param elts  The strings to join
     *
     * @return a string that is the concatentation of elts.
     */
    std::string join(const std::string sep, std::initializer_list<std::string> elts);

    /**
     * Returns a string that is the concatenation of elts.
     *
     * The string sep is placed between each concatenated item of elts.
     * If elts is one element or none, then sep is ignored.
     *
     * @param sep   The join separator
     * @param elts  The strings to join
     *
     * @return a string that is the concatentation of elts.
     */
    std::string join(const std::string sep, std::vector<std::string> elts);

    /**
     * Returns a string that is the concatenation of elts.
     *
     * The string sep is placed between each concatenated item of elts.
     * If elts is one element or none, then sep is ignored.
     *
     * @param sep   The join separator
     * @param elts  The strings to join
     * @param size  The number of items in elts.
     *
     * @return a string that is the concatentation of elts.
     */
    std::string join(const std::string sep, const std::string* elts, size_t size);

    /**
     * Returns a copy of str with any leading and trailing whitespace removed.
     *
     * This function uses the current C++ locale.
     *
     * @param str   The string to trim
     *
     * @return a copy of str with any leading and trailing whitespace removed.
     */
    std::string trim(const std::string str);

    /**
     * Returns a copy of str with any leading whitespace removed.
     *
     * This function uses the current C++ locale.
     *
     * @param str   The string to trim
     *
     * @return a copy of str with any leading whitespace removed.
     */
    std::string ltrim(const std::string str);

    /**
     * Returns a copy of str with any trailing whitespace removed.
     *
     * This function uses the current C++ locale.
     *
     * @param str   The string to trim
     *
     * @return a copy of str with any trailing whitespace removed.
     */
    std::string rtrim(const std::string str);
    
    /**
     * Returns a new string from the given formatter and arguments
     *
     * This funciton is similar to the C-function `sprintf` except that it supports
     * C++ style strings.
     *
     * @param format    The formatting string
     * @param args      The argument list
     *
     * @return a new string from the given formatter and arguments
     */
    template<typename ... Args>
    std::string format(const std::string format, Args ... args ) {
        size_t size = snprintf(nullptr, 0, format.c_str(), args ... ) + 1;
        char* buffer = new char[size];
        snprintf(buffer, size, format.c_str(), args ... );
        std::string result(buffer);
        delete[] buffer;
        return result;
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
    std::string tolower(std::string str);
    
    /**
     * Returns an upper case copy of str.
     *
     * This function uses the current C++ locale.
     *
     * @param str   The string to convert
     *
     * @return an upper case copy of str.
     */
    std::string toupper(const std::string str);
    
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
    std::string replace(const std::string str, const std::string a, const std::string b);

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
    std::string replaceall(const std::string str, const std::string a, const std::string b);
    
#pragma mark -
#pragma mark UNICODE PROCESSING
    /**
     * This enum categorizes the type of unicode characters we might encounter
     *
     * Unicode character types are independent of font choice. Identifying
     * non-printable characters (or spacing) is useful for when we need to
     * break up text over multiple lines.
     */
    enum UnicodeType : int {
        /**
         * A basic space character
         *
         * This type include tabs (which CUGL converts to spaces) but not
         * newlines or other control characters. It also includes the
         * unicode no-break space.
         */
        SPACE   = 0,
        /**
         * A newline character
         *
         * This includes both the special characters `\n` and `\r`. It
         * also includes the unicode "next line" (0x0085). It does not
         * include form feeds or any other control characters.
         */
        NEWLINE = 1,
        /**
         * An unprintable character
         *
         * This includes any unprintable character that is not a newline.
         */
        CONTROL = 2,
        /**
         * A CJK (Chinese, Japanese, Korean) character.
         *
         * These unicode characters are of special note as each character
         * is a word. This is useful for when deciding how to break up a
         * line of text.
         */
        CJK  = 3,
        /**
         * An other type of printable character.
         *
         * This is the standard type of character. It includes all printable
         * ASCII and Latin supplementals. It also includes emojis.
         */
        CHAR = 4,
    };
    
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
    UnicodeType getUnicodeType(Uint32 code);

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
    UnicodeType getUnicodeType(const std::string str);

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
    UnicodeType getUnicodeType(const char* substr, const char* end);

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
    std::vector<Uint32> getCodePoints(const std::string str);

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
     * @param substr    The start of the string to test
     * @param end       The end of the string to test
     *
     * @return the code points for the elements of substr.
     */
    std::vector<Uint32> getCodePoints(const char* substr, const char* end);
    
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
    size_t getUTF8Length(const std::string str);
    
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
     * @param substr    The start of the string to test
     * @param end       The end of the string to test
     *
     * @return the length of substr in UTF8 encoding
     */
    size_t getUTF8Length(const char* substr, const char* end);


    }
}

#endif /* __CU_STRINGS_H_ */
