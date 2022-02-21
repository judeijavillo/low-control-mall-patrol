//
//  CUFiletools.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides several tools for querying and constructing file paths in
//  OS indepedent way.  It is largely a collection of namespaced functions, much
//  like the strings module. It is based off the the os.path module in Python.
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
//  Version: 2/10/20
//
#include <stdio.h>
#include <sys/stat.h>
#include <sstream>
#include <SDL/SDL.h>
#include <cugl/base/CUBase.h>
#include <cugl/util/CUFiletools.h>
#include <cugl/util/CUStrings.h>
#include <cugl/util/CUDebug.h>
#include <cugl/base/CUApplication.h>

#if defined (__ANDROID__)
    #include <sys/vfs.h>
     #define statvfs statfs
     #define fstatvfs fstatfs
    #include <unistd.h>
    #include <dirent.h>
#elif defined (__WINDOWS__)
    #include <locale>
    #include <codecvt>
    #include <io.h>
    #define WINDOWS_TICK 10000000
    #define SEC_TO_UNIX_EPOCH 11644473600LL
#else
    #include <sys/statvfs.h>
    #include <unistd.h>
    #include <dirent.h>
#endif



namespace cugl {
namespace filetool {

#pragma mark -
#pragma mark Internal Helpers
/**
 * Returns true if c is a potential separator (on any plaform)
 *
 * @param c The character to check
 *
 * @return true if c is a potential separator (on any plaform)
 */
bool is_sep(char c) {
    return c == '/' || c == '\\';
}

/**
 * Returns an absolute (and normalized) path equivalent to path.
 *
 * In CUGL, all relative paths are interpretted as relative to the asset directory.
 * which is a read-only directory.
 *
 * @param path  The path to convert
 *
 * @return an absolute (and normalized) path equivalent to path.
 */
std::string to_absolute(std::string path) {
    if (cugl::filetool::is_absolute(path)) {
        return cugl::filetool::normalize_path(path);
    }
    
    std::string prefix;
#if defined (__WINDOWS__)
    char currDir[255];
    GetCurrentDirectoryA(255, currDir);
    prefix = currDir;
    prefix.push_back(path_sep);
#else
    char* base = SDL_GetBasePath();
    if (base) {
        prefix.append(base);
        SDL_free(base);
    }
#endif
    return cugl::filetool::normalize_path(prefix+path);
}

#pragma mark -
#pragma mark Path Queries
/**
 * Returns true if the file denoted by this path name is a normal file.
 *
 * This function will return false is the file does not exist. If the path
 * is a relative path, this function will use the asset directory as the
 * working directory.
 *
 * @param path    The file path name
 *
 * @return true if the file denoted by this path name is a normal file.
 */
bool is_file(const std::string path) {
    const std::string fullpath = to_absolute(path);
#if defined (__WINDOWS__)
    DWORD ftyp = GetFileAttributesA(fullpath.c_str());
    return !(ftyp == INVALID_FILE_ATTRIBUTES) && (ftyp & FILE_ATTRIBUTE_NORMAL);
#else
    struct stat status;
    int err = stat(fullpath.c_str(), &status);
    return (err == 0 && S_ISREG(status.st_mode));
#endif
}

/**
 * Returns true if the file denoted by this path name is a directory.
 *
 * This function will return false is the file does not exist. If the path
 * is a relative path, this function will use the asset directory as the
 * working directory.
 *
 * @param path    The file path name
 *
 * @return true if the file denoted by this path name is a directory.
 */
bool is_dir(const std::string path) {
    const std::string fullpath = to_absolute(path);
#if defined (__WINDOWS__)
    DWORD ftyp = GetFileAttributesA(fullpath.c_str());
    return !(ftyp == INVALID_FILE_ATTRIBUTES) && (ftyp & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat status;
    int err = stat(fullpath.c_str(), &status);
    return (err == 0 && S_ISDIR(status.st_mode));
#endif
}

/**
 * Returns true if the file named by this path name is a hidden file (starts with .).
 *
 * This function does not require that the file exist. It only checks the
 * naming convention of the file referenced by this path.
 *
 * @param path    The file path name
 *
 * @return true if the file named by this path name is a hidden file.
 */
bool is_hidden(const std::string path) {
    const std::string base = base_name(path);
    return base.size() > 0 && base[0] == '.';
}

/**
 * Returns true if this path name is absolute.
 *
 * An absolute path name has an explicit volume and path from the
 * volume. This function does not require that the file exist. It only
 * checks the naming convention of the file referenced by this path.
 *
 * @return true if this path name is absolute.
 */
bool is_absolute(const std::string path) {
    // Be OS agnostic
    if (path.size() == 0) {
        return false;
    } else if (path.size() > 2 && path[1] == ':') {
        return is_sep(path[2]);
    }
    
    return is_sep(path[0]);
}

/**
 * Returns true if the file or directory denoted by this path name exists.
 *
 * This function will return false is the file does not exist. If the path
 * is a relative path, this function will use the asset directory as the
 * working directory.
 *
 * @param path    The file path name
 *
 * @return true if the file or directory denoted by this path name exists.
 */
bool file_exists(const std::string path) {
#if defined (__ANDROID__)
    std::string fullpath = to_absolute(path);
    SDL_RWops *file = SDL_RWFromFile(fullpath.c_str(), "r");
    if (file != NULL) {
        SDL_RWclose(file);
        return true;
    }
    return false;
#else
    std::string fullpath = to_absolute(path);
    struct stat status;
    return stat(fullpath.c_str(), &status) == 0;
#endif
}

/**
 * Returns the volume prefix for this path name.
 *
 * There does not have to be a valid file at the given path name for this function to
 * return a value. If the path name is a relative one, it will return the volume of
 * the asset directory. On some platforms (particularly Android) this may return
 * the empty string.
 *
 * @param path    The file path name
 *
 * @return the volume prefix for this path.
 */
const std::string file_vol(const std::string path) {
    const std::string fullpath = to_absolute(path);
    
    if (fullpath.size() > 1 && fullpath[1] == ':') {
        return fullpath.substr(0,2)+path_sep;
    } else if (is_sep(fullpath[0])) {
        return fullpath.substr(0,1);
    }
    
    return "";
}

/**
 * Returns the length of the file denoted by this path name.
 *
 * The value is measured in bytes.  This function returns 0 if there
 * is no file at the given path name. If the path is a relative path, this
 * function will use the asset directory as the working directory.
 *
 * @param path    The file path name
 *
 * @return the length of the file denoted by this path name.
 */
size_t file_size(const std::string path) {
    const std::string fullpath = to_absolute(path);

    size_t result = 0;
    Uint8 buf[256];
    SDL_RWops* rw = SDL_RWFromFile(fullpath.c_str(), "rb");
    
    if (rw) {
        // This is really the only reliable way to do this
        size_t amt = SDL_RWread(rw, buf, 1, sizeof (buf));
        while (amt > 0) {
            result += amt;
            amt = SDL_RWread(rw, buf, 1, sizeof (buf));
        }
        SDL_RWclose(rw);
    }
    return result;
}

/**
 * Returns the time that the file for this path name was last modified.
 *
 * The value is in seconds since the last epoch (e.g. January 1, 1970 on
 * Unix systems).  This function returns 0 if there is no file at the given
 * path name. If the path is a relative path, this function will use the
 * asset directory as the working directory.
 *
 * @param path    The file path name
 *
 * @return the time that the file for this path name was last modified.
 */
Uint64 file_timestamp(const std::string path) {
    const std::string fullpath = to_absolute(path);
#if defined (__WINDOWS__)
    WIN32_FIND_DATA search_data;
    memset(&search_data, 0, sizeof(WIN32_FIND_DATA));

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(fullpath);
    HANDLE handle = FindFirstFile(wide.c_str(), &search_data);

    Uint64 result = 0;
    FILETIME creationTime;
    FILETIME lastAccessTime;
    FILETIME lastWriteTime;
    bool ok = GetFileTime(handle, &creationTime, &lastAccessTime, &lastWriteTime);
    if (ok) {
        FILETIME localFileTime;
        FileTimeToLocalFileTime(&lastWriteTime, &localFileTime);
        SYSTEMTIME sysTime;
        FileTimeToSystemTime(&localFileTime, &sysTime);
        struct tm tmtime = { 0 };
        tmtime.tm_year = sysTime.wYear - 1900;
        tmtime.tm_mon = sysTime.wMonth - 1;
        tmtime.tm_mday = sysTime.wDay;
        tmtime.tm_hour = sysTime.wHour;
        tmtime.tm_min = sysTime.wMinute;
        tmtime.tm_sec = sysTime.wSecond;
        tmtime.tm_wday = 0;
        tmtime.tm_yday = 0;
        tmtime.tm_isdst = -1;
        time_t ret = mktime(&tmtime);
        result = static_cast<Uint64> (ret);
    }

    FindClose(handle);
    return result;
#else
    struct stat status;
    int err = stat(fullpath.c_str(), &status);
    if (err == 0) {
        return (Uint64)status.st_mtime;
    }
    return 0;
#endif
}


#pragma mark -
#pragma mark Path Manipulation
/**
 * The system-dependent path separator for this platform.
 */
#if defined (__WINDOWS__)
    char path_sep = '\\';
#else
    char path_sep = '/';
#endif

/**
 * Returns the path name of the parent directory for this file.
 *
 * This value is the same as the first element returned by the function
 * {@link split_path}. If path is a relative reference to a file with no
 * parent directory then this function will return the empty string.
 *
 * @param path    The file path name
 *
 * @return the suffix for the leaf file of this path.
 */
const std::string dir_name(const std::string path) {
    if (path.empty()) {
        return "";
    }
    bool goon = true;
    size_t pos;
    for(pos = path.size(); pos > 0 && goon; ) {
        if (is_sep(path[pos-1])) {
            goon = false;
        } else {
            pos--;
        }
    }
    
    if (goon) {
        return "";
    } else {
        return path.substr(0,pos-1);
    }
}

/**
 * Returns the name of the leaf file of this path.
 *
 * This value is the same as the second element returned by the function
 * {@link split_path}. If the path names ends in a path separator, this
 * will be ignored when determining the leaf.
 *
 * @param path    The file path name
 *
 * @return the suffix for the leaf file of this path.
 */
const std::string base_name(const std::string path) {
    if (path.empty()) {
        return "";
     }
     bool goon = true;
     size_t pos;
     for(pos = path.size(); pos > 0 && goon; ) {
         if (is_sep(path[pos-1])) {
             goon = false;
         } else {
             pos--;
         }
     }
     
     if (goon) {
         return path;
     } else {
         return path.substr(pos);
     }
}

/**
 * Returns the pair of a leaf file and its parent directory.
 *
 * The parent directory will be the first element of the pair.  If path is a
 * relative reference to a file with no parent directory then the first element
 * will be the empty string. If the path names ends in a path separator, this
 * will be ignored when determining the leaf.
 *
 * @param path    The file path name
 *
 * @return the pair of a leaf file and its parent directory.
 */
std::pair<std::string, std::string> split_path(const std::string path) {
    if (path.empty()) {
        return std::pair<std::string, std::string>("","");
    }
    bool goon = true;
    size_t pos;
    for(pos = path.size(); pos > 0 && goon; ) {
        if (is_sep(path[pos-1])) {
            goon = false;
        } else {
            pos--;
        }
    }
    
    if (goon) {
        return std::pair<std::string, std::string>("",path);
    } else {
        return std::pair<std::string, std::string>(path.substr(0,pos-1),path.substr(pos));
    }
}

/**
 * Returns the path name broken up into individual elements.
 *
 * The last element of the vector will be be the leaf file of the path name.
 * All other elements (if they exist) will be directories in the path name.
 * If the path name is absolute then the first element of the vector will
 * include the volume.
 *
 * @param path    The file path name
 *
 * @return the pair of a leaf file and its parent directory.
 */
std::vector<std::string> fullsplit_path(const std::string path) {
    // Split by ANY path separator after first
    std::vector<std::string> result;
    size_t last = 0;
    size_t pos = path.find(":");
    pos = pos == std::string::npos ? 1 : pos+1;
    for(; pos < path.size(); pos++) {
        if (is_sep(path[pos])) {
            result.push_back(path.substr(last,pos-last));
            last = pos+1;
        }
    }
    if (last < path.size()) {
        result.push_back(path.substr(last,path.size()-last));
    }
    return result;
}

/**
 * Returns the suffix for the leaf file of this path.
 *
 * A suffix is any part of the file name after a final period. If there is no
 * suffix, this function returns the empty string.
 *
 * This value is the same as the second element returned by the function
 * {@link split_base}.
 *
 * @param path    The file path name
 *
 * @return the suffix for the leaf file of this path.
 */
const std::string base_prefix(const std::string path) {
    if (path.empty()) {
        return "";
    }
    bool goon  = true;
    size_t pos0;
    for(pos0 = path.size(); pos0 > 0 && goon; ) {
        if (path[pos0-1] == '.') {
            goon = false;
        } else {
            pos0--;
        }
    }
    
    if (goon) {
        return path;
    } else if (pos0 == 0) {
        return "";
    }

    size_t pos1;
    goon = true;
    for(pos1 = pos0; pos1 > 0 && goon; ) {
        if (is_sep(path[pos1-1])) {
            goon = false;
        } else {
            pos1--;
        }
    }
    
    if (goon) {
        return path.substr(0,pos0-1);
    } else {
        return path.substr(pos1,pos0-pos1-1);
    }
}

/**
 * Returns the suffix for the leaf file of this path.
 *
 * A suffix is any part of the file name after a final period. If there is no
 * suffix, this function returns the empty string.
 *
 * This value is the same as the second element returned by the function
 * {@link split_base}.
 *
 * @param path    The file path name
 *
 * @return the suffix for the leaf file of this path.
 */
const std::string base_suffix(const std::string path) {
    if (path.empty()) {
        return "";
    }
    bool goon  = true;
    size_t pos0;
    for(pos0 = path.size(); pos0 > 0 && goon; ) {
        if (path[pos0-1] == '.') {
            goon = false;
        } else {
            pos0--;
        }
    }
    
    if (goon) {
        return "";
    } else if (pos0 == 0) {
        return path.substr(1);
    }

    return path.substr(pos0);
}

/**
 * Returns a copy of the path name with the given suffix.
 *
 * A suffix is any part of the file name after a final period. If there is
 * already a suffix in the path name, this function will replace it with
 * the new one.
 *
 * This function only affects the path name.  It does not affect any file
 * associated with the path name.
 *
 * @param path    The file path name
 *
 * @return a copy of the path name with the given suffix.
 */
const std::string set_suffix(const std::string path, const std::string suffix) {
    if (path.empty()) {
        return "";
    } else if (suffix.empty()) {
        return path;
    }
    
    bool goon  = true;
    size_t pos0;
    for(pos0 = path.size(); pos0 > 0 && goon; ) {
        if (path[pos0-1] == '.') {
            goon = false;
        } else {
            pos0--;
        }
    }
    
    std::string prefix = "";
    if (goon) {
        prefix = path;
    } else if (pos0 != 0) {
        prefix = path.substr(0,pos0-1);
    }

    if (suffix[0] == '.') {
        return prefix+suffix;
    } else {
        return prefix+"."+suffix;
    }
}

/**
 * Returns a pair of the prefix and suffix of the leaf file of the path.
 *
 * A suffix is any  part of the file name after a final period.  If the path name
 * contains any directories other than the base file, they are ignored.
 *
 * @param path    The file path name
 *
 * @return a pair of the prefix and suffix of the leaf file of the path.
 */
std::pair<std::string, std::string> split_base(const std::string path) {
    if (path.empty()) {
        return std::pair<std::string, std::string>("","");
    }
    bool goon  = true;
    size_t pos0;
    for(pos0 = path.size(); pos0 > 0 && goon; ) {
        if (path[pos0-1] == '.') {
            goon = false;
        } else {
            pos0--;
        }
    }
    
    if (goon) {
        return std::pair<std::string, std::string>(path,"");
    } else if (pos0 == 0) {
        return std::pair<std::string, std::string>("",path.substr(1));
    }

    size_t pos1;
    goon = true;
    for(pos1 = pos0; pos1 > 0 && goon; ) {
        if (is_sep(path[pos1-1])) {
            goon = false;
        } else {
            pos1--;
        }
    }
    
    if (goon) {
        return std::pair<std::string, std::string>(path.substr(0,pos0-1),path.substr(pos0));
    } else {
        return std::pair<std::string, std::string>(path.substr(pos1,pos0-pos1-1),path.substr(pos0));
    }
}

/**
 * Returns the given path, normalized to the current platform
 *
 * Normalization replaces all path separators with the correct system-
 * dependent versions.  If the path is absolute, it also normalizes the
 * path prefix (e.g. capitalizing drive letters on Windows and so on).
 *
 * In addition, normalization removes all redundant directories (e.g. the
 * directories . and ..).  However, it does not expand links or shortcuts.
 * Furthermore, this function does not convert a relative path into an
 * absolute one. Since the asset directory is not well-defined on all
 * platforms, it is not always possible to perform such a conversion.
 *
 * In order for normalization to be successful, the path name must be
 * mutliplatform safe.  That means no use of colon (:), slash (\), or
 * backslash (/) in file names.
 *
 * @param path    The file path name
 *
 * @return the given path, normalized to the current platform
 */
const std::string normalize_path(const std::string path) {
    std::vector<std::string> items = fullsplit_path(path);
    
    // Handle the redundancies
    std::vector<std::string> canonical;
    for(auto it = items.begin(); it != items.end(); ++it) {
        if (*it == "..") {
            CUAssertLog(!canonical.empty(),"Error while canonicalizing pathname");
            canonical.erase(canonical.end()-1);
        } else if (*it != "." && *it != "") {
            canonical.push_back(*it);
        }
    }
    
    if (canonical.empty()) {
        return std::string();
    }
    
    // Treat the initial part special
    std::string prefix = canonical[0];
#if defined (__WINDOWS__)
    if (prefix.size() > 0 && prefix[0] == '/') {
        char currDir[255];
        GetCurrentDirectoryA(255, currDir);
        std::string volume(currDir,3);
        if (volume[1] == ':') {
            canonical[0] = volume+prefix.substr(1);
        }
    }
#else
    if (prefix.size() > 1 && prefix[1] == ':') {
        if (prefix.size() > 2 && (prefix[2] == '/' || prefix[2] == '\\')) {
            canonical[0] = "/"+prefix.substr(3);
        } else {
            canonical[0] = prefix.substr(2);
        }
    }
#endif
    
    std::stringstream result;
    for(size_t pos = 0; pos < canonical.size(); pos++) {
        result << canonical[pos];
        if (pos < canonical.size()-1) {
            result << path_sep;
        }
    }

    return result.str();
}

/**
 * Returns the given path, canonicalized to the current platform
 *
 * Canonicalization does everything that normalization does, plus it
 * converts a relative path to its absolute equivalent. It replaces all
 * path separators with the correct system-dependent versions.  It also
 * normalizes the path prefix (e.g. capitalizing drive letters on Windows
 * and so on).
 *
 * In addition, canonicalization removes all redundant directories (e.g.
 * the directories . and ..).  However, it does not expand links or
 * shortcuts as is often the case with path canonicalization.
 *
 * @param path  The path to canonicalize.
 *
 * @return the given path, canonicalized to the current platform
 */
const std::string canonicalize_path(const std::string path) {
    std::string result = normalize_path(path);
    if (!is_absolute(path)) {
        result = Application::get()->getSaveDirectory()+result;
    }
    
    // Now break it up
#if defined (__WINDOWS__)
    std::string::size_type curr = result[1] == ':' ? 3 : 2;
#else
    std::string::size_type curr = 1;
#endif
    
    // Compute the components.
    std::string volume = result.substr(0,curr);
    std::vector<std::string> components;
    while (curr != std::string::npos) {
        std::string::size_type next = result.find(path_sep,curr);
        if (next == std::string::npos) {
            components.push_back(result.substr(curr));
        } else {
            components.push_back(result.substr(curr,next-curr));
        }
        curr = next == std::string::npos ? next : next+1;
    }
    
    // Handle the redundancies
    std::vector<std::string> canonical;
    for(auto it = components.begin(); it != components.end(); ++it) {
        if (*it == "..") {
            CUAssertLog(!canonical.empty(),"Error while canonicalizing pathname");
            canonical.erase(canonical.end()-1);
        } else if (*it != ".") {
            canonical.push_back(*it);
        }
    }
    
    // Build the final path
    result = volume;
    for(auto it = canonical.begin(); it != canonical.end(); ++it) {
        result.append(*it+path_sep);
    }
    result.erase(result.end()-1);
    return result;
}

/**
 * Returns the common subpath of the given paths.
 *
 * If there is no common prefix, or if the paths are a mixture of absolute and
 * relative paths, then this function returns the empty string.
 *
 * @param paths    The collection of paths to search
 *
 * @return the common subpath of the given paths.
 */
const std::string common_path(const std::initializer_list<std::string> paths) {
    const std::string* path = nullptr;
    size_t len = 0;
    for(auto it = paths.begin(); it != paths.end(); ++it) {
        if (path == nullptr) {
            path = it;
            len = it->size();
        } else {
            bool goon = true;
            for(size_t ii = 0; ii < len && ii < it->size() && goon; ii++) {
                if ((*it)[ii] != (*path)[ii]) {
                    len = ii;
                    goon = false;
                }
            }
        }
    }
    if (path == nullptr) {
        return "";
    } else if (len > 1 && is_sep((*path)[len-1])) {
        return path->substr(0,len-1);
    }
    return path->substr(0,len);
}

/**
 * Returns the common subpath of the given paths.
 *
 * If there is no common prefix, or if the paths are a mixture of absolute and
 * relative paths, then this function returns the empty string.
 *
 * @param paths    The collection of paths to search
 *
 * @return the common subpath of the given paths.
 */
const std::string common_path(const std::vector<std::string> paths) {
    const std::string* path = nullptr;
    size_t len = 0;
    for(auto it = paths.begin(); it != paths.end(); ++it) {
        if (path == nullptr) {
            path = &(*it);
            len = it->size();
        } else {
            bool goon = true;
            for(size_t ii = 0; ii < len && ii < it->size() && goon; ii++) {
                if ((*it)[ii] != (*path)[ii]) {
                    len = ii;
                    goon = false;
                }
            }
        }
    }
    if (path == nullptr) {
        return "";
    } else if (len > 1 && is_sep((*path)[len-1])) {
        return path->substr(0,len-1);
    }
    return path->substr(0,len);
}

/**
 * Returns the common subpath of the given paths.
 *
 * If there is no common prefix, or if the paths are a mixture of absolute and
 * relative paths, then this function returns the empty string.
 *
 * @param paths    The collection of paths to search
 *
 * @return the common subpath of the given paths.
 */
const std::string common_path(const std::string* paths, size_t size) {
    const std::string* path = nullptr;
    size_t len = 0;
    for(size_t ii = 0; ii < size; ii++) {
        if (path == nullptr) {
            path = paths+ii;
            len = path->size();
        } else {
            bool goon = true;
            for(size_t jj = 0; jj < len && jj < path->size() && goon; jj++) {
                if (paths[ii][jj] != (*path)[jj]) {
                    len = jj;
                    goon = false;
                }
            }
        }
    }
    if (path == nullptr) {
        return "";
    } else if (len > 1 && is_sep((*path)[len-1])) {
        return path->substr(0,len-1);
    }
    return path->substr(0,len);
}

/**
 * Returns a path that is the concatentation of elts.
 *
 * The path elements will be concatenated using the platform-specific separator.
 * To create an absolute path the first element should include the volume.
 *
 * The path returned will not be normalized.  Call {@link normalize_path} if any
 * additional normalization is necessary.
 *
 * @param elts  The strings to join
 *
 * @return a string that is the concatentation of elts.
 */
const std::string join_path(std::initializer_list<std::string> elts) {
    std::stringstream result;
    size_t size = elts.size();
    size_t pos = 0;
    for(auto it = elts.begin(); it != elts.end(); ++it) {
        result << *it;
        if (pos < size-1) {
            result << path_sep;
        }
        pos++;
    }
    return result.str();
}

/**
 * Returns a path that is the concatentation of elts.
 *
 * The path elements will be concatenated using the platform-specific separator.
 * To create an absolute path the first element should include the volume.
 *
 * The path returned will not be normalized.  Call {@link normalize_path} if any
 * additional normalization is necessary.
 *
 * @param elts  The strings to join
 *
 * @return a string that is the concatentation of elts.
 */
const std::string join_path(std::vector<std::string> elts) {
    std::stringstream result;
    size_t size = elts.size();
    size_t pos = 0;
    for(auto it = elts.begin(); it != elts.end(); ++it) {
        result << *it;
        if (pos < size-1) {
            result << path_sep;
        }
        pos++;
    }
    return result.str();
}

/**
 * Returns a path that is the concatentation of elts.
 *
 * The path elements will be concatenated using the platform-specific separator.
 * To create an absolute path the first element should include the volume.
 *
 * The path returned will not be normalized.  Call {@link normalize_path} if any
 * additional normalization is necessary.
 *
 * @param elts  The strings to join
 * @param size  The number of items in elts.
 *
 * @return a string that is the concatentation of elts.
 */
const std::string join_path(const std::string* elts, size_t size) {
    std::stringstream result;
    for(size_t pos = 0; pos < size; pos++) {
        result << elts[pos];
        if (pos < size-1) {
            result << path_sep;
        }
    }
    return result.str();
}


#pragma mark -
#pragma mark File Manipulation
/**
 * Creates a new, empty file named by this path name
 *
 * This function succeeds if and only if a file with this name does not yet
 * exist.  The file will be an empty, regular file.
 *
 * This function will fail if the path name is relative.  Relative path names
 * refer to the asset directory, which is a read-only directory.
 *
 * @param path    The file path name
 *
 * @return true if the file was successfully created.
 */
bool file_create(const std::string path) {
    if (!is_absolute(path)) {
        CUAssertLog(false,"Attempt to write to \"%s\" in read-only directory.",path.c_str());
        return false;
    }
    
    std::string fullpath = normalize_path(path);
    struct stat status;

    if (stat(fullpath.c_str(), &status) != 0) {
        SDL_RWops* stream = SDL_RWFromFile(fullpath.c_str(), "w");
        if (stream) {
            SDL_RWclose(stream);
            return true;
        }
    }
    return false;
}

/**
 * Deletes the file denoted by this path name.
 *
 * This function will fail if the file is not a regular file or if it does
 * not yet exist. In addition, it will fail if the path name is relative.
 * Relative path names refer to the asset directory, which is a
 * read-only directory.
 *
 * @param path    The file path name
 *
 * @return true if the file was successfully deleted.
 */
bool file_delete(const std::string path) {
    if (!is_absolute(path)) {
        CUAssertLog(false,"Attempt to delete \"%s\" in read-only directory.",path.c_str());
        return false;
    }

    std::string fullpath = normalize_path(path);
#if defined (__WINDOWS__)
    DWORD ftyp = GetFileAttributesA(fullpath.c_str());
    bool isfile = !(ftyp == INVALID_FILE_ATTRIBUTES) && (ftyp & FILE_ATTRIBUTE_NORMAL);
#else
    struct stat status;
    int err = stat(fullpath.c_str(), &status);
    bool isfile = (err == 0 && S_ISREG(status.st_mode));
#endif
    if (isfile) {
        return std::remove(fullpath.c_str()) == 0;
    }
    return false;
}
/**
 * Returns a list of strings naming the files and directories in this path
 *
 * This function assumes that this path name denotes a valid directory. If it does
 * not, the list will be empty.
 *
 * @param path    The file path name
 *
 * @return a list of strings naming the files and directories in this path
 */
std::vector<std::string> dir_contents(const std::string path) {
    std::string fullpath = to_absolute(path);
#if defined (__WINDOWS__)
    std::vector<std::string> result;

    std::string search_path(fullpath);
    WIN32_FIND_DATA search_data;
    if (search_path[search_path.size() - 1] != path_sep) {
        search_path.push_back(path_sep);
    }

    memset(&search_data, 0, sizeof(WIN32_FIND_DATA));
    search_path.push_back('*');

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(search_path);
    HANDLE handle = FindFirstFile(wide.c_str(), &search_data);

    bool avail = true;
    while (avail && handle != INVALID_HANDLE_VALUE) {
        std::wstring wide(search_data.cFileName);
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::string file = converter.to_bytes(wide);

        if (file != "." && file != "..") {
            result.push_back(search_path + file);
        }
        avail = (FindNextFile(handle, &search_data) != FALSE);
    }

    //Close the handle after use or memory/resource leak
    FindClose(handle);
    return result;
#else
    std::vector<std::string> result;
    std::string sep = "";
    if (fullpath[fullpath.size() - 1] != path_sep) {
        sep = path_sep;
    }

    // This is POSIX.  We HOPE this works on Windows.
    DIR *directory = opendir(fullpath.c_str());

    struct dirent *contents;
    struct stat status;
    if (directory != nullptr) {
        contents = readdir(directory);
        while (contents != nullptr) {
            std::string item = contents->d_name;
            int err = stat((fullpath + sep + item).c_str(), &status);
            if (err == 0 && item != "." && item != "..") {
                result.push_back(fullpath + sep + item);
            }
            contents = readdir(directory);
        }
        closedir(directory);
    }

    return result;
#endif
}

/**
 * Returns a filtered list of strings naming the files and directories in this path
 *
 * This function assumes that this path name denotes a valid directory. If it does
 * not, the list will be empty.
 *
 * The filter will be given the normalized version of each file in the directory.
 * If the directory is specified by an absolute path, each file will be as well.
 *
 * @param path    The file path name
 *
 * @return a filtered list of strings naming the files and directories in this path
 */
std::vector<std::string> dir_contents(const std::string path, const std::function<bool(const std::string file)>& filter) {
    std::string fullpath = to_absolute(path);
#if defined (__WINDOWS__)
    std::vector<std::string> result;

    std::string search_path(fullpath);
    WIN32_FIND_DATA search_data;
    if (search_path[search_path.size() - 1] != path_sep) {
        search_path.push_back(path_sep);
    }

    memset(&search_data, 0, sizeof(WIN32_FIND_DATA));
    search_path.push_back('*');

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(search_path);
    HANDLE handle = FindFirstFile(wide.c_str(), &search_data);

    bool avail = true;
    while (avail && handle != INVALID_HANDLE_VALUE) {
        std::wstring wide(search_data.cFileName);
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::string file = converter.to_bytes(wide);

        if (file != "." && file != "..") {
            const std::string path = search_path + file;
            if (filter(path)) {
                result.push_back(path);
            }
        }
        avail = (FindNextFile(handle, &search_data) != FALSE);
    }

    //Close the handle after use or memory/resource leak
    FindClose(handle);
    return result;
#else
    std::vector<std::string> result;
    std::string sep = "";
    if (fullpath[fullpath.size() - 1] != path_sep) {
        sep = path_sep;
    }

    // This is POSIX.  We HOPE this works on Windows.
    DIR *directory = opendir(fullpath.c_str());
    
    struct dirent *contents;
    struct stat status;
    if (directory != nullptr) {
        contents = readdir(directory);
        while (contents != nullptr) {
            std::string item = contents->d_name;
            int err = stat((fullpath+sep+item).c_str(), &status);
            if (err == 0 && item != "." && item != "..") {
                const std::string found = fullpath+sep+item;
                if (filter(found)) {
                    result.push_back(found);
                }
            }
            contents = readdir(directory);
        }
        closedir (directory);
    }
    
    return result;
#endif
}

/**
 * Creates the directory named by this path name.
 *
 * This function succeeds if and only if a file or directory with this name
 * does not yet exist.
 *
 * This function will fail if the path name is relative.  Relative path names
 * refer to the asset directory, which is a read-only directory.
 *
 * @param path    The file path name
 *
 * @return true if the directory was successfully created.
 */
bool dir_create(const std::string path) {
    if (!is_absolute(path)) {
        CUAssertLog(false,"Attempt to write to \"%s\" in read-only directory.",path.c_str());
        return false;
    }
    
    std::string fullpath = normalize_path(path);
#if defined (__WINDOWS__)
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(fullpath);
    CreateDirectory(wide.c_str(), NULL);
    return ERROR_ALREADY_EXISTS != GetLastError();
#else
    struct stat status;
    if (stat(fullpath.c_str(), &status) != 0) {
        return mkdir(fullpath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0;
    }
    return false;
#endif
}

/**
 * Deletes the directory denoted by this path name.
 *
 * This function will fail if the file is not a directory or if it does not
 * yet exist.
 *
 * This function will fail if the file is not a regular file or if it does
 * not yet exist. In addition, it will fail if the path name is relative.
 * Relative path names refer to the asset directory, which is a
 * read-only directory.
 *
 * @param path    The file path name
 *
 * @return true if the directory was successfully deleted.
 */
bool dir_delete(const std::string path) {
    if (!is_absolute(path)) {
        CUAssertLog(false,"Attempt to delete \"%s\" in read-only directory.",path.c_str());
        return false;
    }
    
    std::string fullpath = normalize_path(path);
#if defined (__WINDOWS__)
    DWORD ftyp = GetFileAttributesA(fullpath.c_str());
    bool isdir = !(ftyp == INVALID_FILE_ATTRIBUTES) && (ftyp & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat status;
    int err = stat(fullpath.c_str(), &status);
    bool isdir = (err == 0 && S_ISDIR(status.st_mode));
#endif
    if (isdir) {
        return std::remove(fullpath.c_str()) == 0;
    }
    return false;
}


#pragma mark -
#pragma mark File Access
/**
 * Returns true if the application can read the file for this path name.
 *
 * This function uses the access() POSIX function. Therefore, it does not assume
 * that this application is the file owner, and correctly determines the
 * file access.
 *
 * If there is no file at the given path, this function returns false. If path
 * name is relative it is assumes it is in the asset directory.
 *
 * @param path    The file path name
 *
 * @return true if the application can read the file for this path name.
 */
bool is_readable(const std::string path) {
    std::string fullpath = to_absolute(path);
#if defined (__WINDOWS__)
    bool ronly = _access(fullpath.c_str(), 4) != -1;
    bool rdwrt = _access(fullpath.c_str(), 6) != -1;
    return ronly || rdwrt;
#else
    return access(fullpath.c_str(), R_OK);
#endif
}

/**
 * Returns true if the application can execute the file for this path name.
 *
 * Note that the only form of file execution supported by CUGL is searching
 * a directory.
 *
 * This function uses the access() POSIX function. Therefore, it does not assume
 * that this application is the file owner, and correctly determines the
 * file access.
 *
 * If there is no file at the given path, this function returns false. If path
 * name is relative it is assumes it is in the asset directory.
 *
 * @param path    The file path name
 *
 * @return true if the application can execute the file for this path name.
 */
bool is_searchable(const std::string path) {
    std::string fullpath = to_absolute(path);
#if defined (__WINDOWS__)
    bool ronly = _access(fullpath.c_str(), 4) != -1;
    bool rdwrt = _access(fullpath.c_str(), 6) != -1;
    return ronly || rdwrt;
#else
    return access(fullpath.c_str(), X_OK);
#endif
}

/**
 * Returns true if the application can modify the file for this path name.
 *
 * This function uses the access() POSIX function. Therefore, it does not assume
 * that this application is the file owner, and correctly determines the
 * file access.
 *
 * If there is no file at the given path, this function returns false. If path
 * name is relative it is assumes it is in the asset directory.
 *
 * @param path    The file path name
 *
 * @return true if the application can modify the file for this path name.
 */
bool is_writable(const std::string path) {
    std::string fullpath = to_absolute(path);
#if defined (__WINDOWS__)
    bool wonly = _access(fullpath.c_str(), 2) != -1;
    bool rdwrt = _access(fullpath.c_str(), 6) != -1;
    return wonly || rdwrt;
#else
    return access(fullpath.c_str(), W_OK);
#endif
}

/**
 * Sets the owner's read permission for this path name.
 *
 * The owner may or may not be this application. The function will return
 * false if the application does not have permission for this change.
 *
 * If there is no file at the given path, this function returns false.  This
 * function will also fail if the path name is relative.  Relative path names
 * refer to the asset directory, which is a read-only directory.
 *
 * @param path        The file path name
 * @param readable  Whether the owner may read this file
 *
 * @return true if the file permissions where successfully changed.
 */
bool set_readable(const std::string path, bool readable)  {
    if (!is_absolute(path)) {
        CUAssertLog(false,"Attempt to modify \"%s\" in read-only directory.",path.c_str());
        return false;
    }
    
#if defined (__WINDOWS__)
    CUAssertLog(false, "Windows does not support this functionality");
    return false;
#else
    struct stat status;
    std::string fullpath = normalize_path(path);
    int err = stat(fullpath.c_str(), &status);
    if (err == 0) {
        if (readable) {
            status.st_mode = status.st_mode | S_IRUSR;
        } else {
            status.st_mode = status.st_mode & ~S_IRUSR;
        }
        return chmod(fullpath.c_str(), status.st_mode) == 0;
    }
    return false;
#endif
}

/**
 * Sets the owner's or everybody's read permission for this path name.
 *
 * The owner may or may not be this application. The function will return
 * false if the application does not have permission for this change.
 *
 * If there is no file at the given path, this function returns false.  This
 * function will also fail if the path name is relative.  Relative path names
 * refer to the asset directory, which is a read-only directory.
 *
 * @param path        The file path name
 * @param readable  Whether this file may be read
 * @param ownerOnly Whether to apply this change only to the owner
 *
 * @return true if the file permissions where successfully changed.
 */
bool set_readable(const std::string path, bool readable, bool ownerOnly) {
    if (!is_absolute(path)) {
        CUAssertLog(false,"Attempt to modify \"%s\" in read-only directory.",path.c_str());
        return false;
    }

#if defined (__WINDOWS__)
    CUAssertLog(false, "Windows does not support this functionality");
    return false;
#else
    if (ownerOnly) {
        return set_readable(path,readable);
    }

    struct stat status;
    std::string fullpath = normalize_path(path);
    int err = stat(fullpath.c_str(), &status);
    if (err == 0) {
        if (readable) {
            status.st_mode = status.st_mode | S_IRUSR | S_IRGRP | S_IROTH;
        } else {
            status.st_mode = status.st_mode & ~S_IRUSR & ~S_IRGRP & ~S_IROTH;
        }
        return chmod(fullpath.c_str(), status.st_mode) == 0;
    }
    return false;
#endif
}

/**
 * Marks this file or directory so that only read operations are allowed.
 *
 * The owner may or may not be this application. The function will return
 * false if the application does not have permission for this change.
 *
 * If there is no file at the given path, this function returns false.  This
 * function will also fail if the path name is relative.  Relative path names
 * refer to the asset directory, which is a read-only directory.
 *
 * @param path The file path name
 *
 * @return true if the file permissions where successfully changed.
 */
bool set_readonly(const std::string path) {
    if (!is_absolute(path)) {
        CUAssertLog(false,"Attempt to modify \"%s\" in read-only directory.",path.c_str());
        return false;
    }
    
#if defined (__WINDOWS__)
    CUAssertLog(false, "Windows does not support this functionality");
    return false;
#else
    struct stat status;
    std::string fullpath = normalize_path(path);
    int err = stat(fullpath.c_str(), &status);
    if (err == 0) {
        status.st_mode = status.st_mode & ~S_IWUSR & ~S_IWGRP & ~S_IWOTH;
        return chmod(fullpath.c_str(), status.st_mode) == 0;
    }
    return false;
#endif
}

/**
 * Sets the owner's execution permission for this path name.
 *
 * Note that the only form of file execution supported by CUGL is searching
 * a directory.
 *
 * The owner may or may not be this application. The function will return
 * false if the application does not have permission for this change.
 *
 * If there is no file at the given path, this function returns false.  This
 * function will also fail if the path name is relative.  Relative path names
 * refer to the asset directory, which is a read-only directory.
 *
 * @param path            The file path name
 * @param searchable    Whether the owner may execute (search) this file
 *
 * @return true if the file permissions where successfully changed.
 */
bool set_searchable(const std::string path, bool searchable) {
    if (!is_absolute(path)) {
        CUAssertLog(false,"Attempt to modify \"%s\" in read-only directory.",path.c_str());
        return false;
    }
    
#if defined (__WINDOWS__)
    CUAssertLog(false, "Windows does not support this functionality");
    return false;
#else
    struct stat status;
    std::string fullpath = normalize_path(path);
    int err = stat(fullpath.c_str(), &status);
    if (err == 0) {
        if (searchable) {
            status.st_mode = status.st_mode | S_IXUSR;
        } else {
            status.st_mode = status.st_mode & ~S_IXUSR;
        }
        return chmod(fullpath.c_str(), status.st_mode) == 0;
    }
    return false;
#endif
}

/**
 * Sets the owner's or everybody's execution permission for this path name.
 *
 * Note that the only form of file execution supported by CUGL is searching
 * a directory.
 *
 * The owner may or may not be this application. The function will return
 * false if the application does not have permission for this change.
 *
 * If there is no file at the given path, this function returns false.  This
 * function will also fail if the path name is relative.  Relative path names
 * refer to the asset directory, which is a read-only directory.
 *
 * @param path            The file path name
 * @param searchable    Whether this file may be excuted
 * @param ownerOnly     Whether to apply this change only to the owner
 *
 * @return true if the file permissions where successfully changed.
 */
bool set_searchable(const std::string path, bool searchable, bool ownerOnly) {
    if (!is_absolute(path)) {
        CUAssertLog(false,"Attempt to modify \"%s\" in read-only directory.",path.c_str());
        return false;
    }
    
#if defined (__WINDOWS__)
    CUAssertLog(false, "Windows does not support this functionality");
    return false;
#else
    if (ownerOnly) {
        return set_searchable(path,searchable);
    }

    struct stat status;
    std::string fullpath = normalize_path(path);
    int err = stat(fullpath.c_str(), &status);
    if (err == 0) {
        if (searchable) {
            status.st_mode = status.st_mode | S_IXUSR | S_IXGRP | S_IXOTH;
        } else {
            status.st_mode = status.st_mode & ~S_IXUSR & ~S_IXGRP & ~S_IXOTH;
        }
        return chmod(fullpath.c_str(), status.st_mode) == 0;
    }
    return false;
#endif
}

/**
 * Sets the owner's write permission for this path name.
 *
 * The owner may or may not be this application. The function will return
 * false if the application does not have permission for this change.
 *
 * If there is no file at the given path, this function returns false.  This
 * function will also fail if the path name is relative.  Relative path names
 * refer to the asset directory, which is a read-only directory.
 *
 * @param path        The file path name
 * @param writable  Whether the owner may write to this file
 *
 * @return true if the file permissions where successfully changed.
 */
bool set_writable(const std::string path, bool writable) {
    if (!is_absolute(path)) {
        CUAssertLog(false,"Attempt to modify \"%s\" in read-only directory.",path.c_str());
        return false;
    }
    
#if defined (__WINDOWS__)
    CUAssertLog(false, "This functionality is not yet implemented in Windows");
    return false;
#else
    struct stat status;
    std::string fullpath = normalize_path(path);
    int err = stat(fullpath.c_str(), &status);
    if (err == 0) {
        if (writable) {
            status.st_mode = status.st_mode | S_IWUSR;
        } else {
            status.st_mode = status.st_mode & ~S_IWUSR;
        }
        return chmod(fullpath.c_str(), status.st_mode) == 0;
    }
    return false;
#endif
}

/**
 * Sets the owner's or everybody's write permission for this path name.
 *
 * The owner may or may not be this application. The function will return
 * false if the application does not have permission for this change.
 *
 * If there is no file at the given path, this function returns false.  This
 * function will also fail if the path name is relative.  Relative path names
 * refer to the asset directory, which is a read-only directory.
 *
 * @param path        The file path name
 * @param writable  Whether this file may be written to
 * @param ownerOnly Whether to apply this change only to the owner
 *
 * @return true if the file permissions where successfully changed.
 */
bool set_writable(const std::string path, bool writable, bool ownerOnly) {
    if (!is_absolute(path)) {
        CUAssertLog(false,"Attempt to modify \"%s\" in read-only directory.",path.c_str());
        return false;
    }
    
#if defined (__WINDOWS__)
    CUAssertLog(false, "This functionality is not yet implemented in Windows");
    return false;
#else
    if (ownerOnly) {
        return set_writable(path,writable);
    }
    
    struct stat status;
    std::string fullpath = normalize_path(path);
    int err = stat(fullpath.c_str(), &status);
    if (err == 0) {
        if (writable) {
            status.st_mode = status.st_mode | S_IWUSR | S_IWGRP | S_IWOTH;
        } else {
            status.st_mode = status.st_mode & ~S_IWUSR & ~S_IWGRP & ~S_IWOTH;
        }
        return chmod(fullpath.c_str(), status.st_mode) == 0;
    }
    return false;
#endif
}
    

#pragma mark -
#pragma mark File Volumes
/**
 * Returns the number of unallocated bytes in the partition for this path name.
 *
 * The value is for the partition containing the given file or directory. If
 * the path name is relative, it assumes that the path name is in the asset
 * directory.
 *
 * If the path name does not refer to a proper volume, this function will
 * return 0.  On some platforms (e.g. Android) the asset directory is not a
 * proper volume and this function will return 0.
 *
 * @param path  The file path name
 *
 * @return the number of unallocated bytes in the partition for this path name.
 */
size_t vol_free_space(const std::string path) {
    std::string fullpath = to_absolute(path);
#if defined (__WINDOWS__)
    ULARGE_INTEGER freeBytesAvail;
    ULARGE_INTEGER totalNumBytes;
    ULARGE_INTEGER totalBytesAvail;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(fullpath);
    bool okay = GetDiskFreeSpaceEx(wide.c_str(), &freeBytesAvail, &totalNumBytes, &totalBytesAvail);
    if (okay) {
        return (size_t)totalBytesAvail.QuadPart;
    }
    return 0;
#else
    struct statvfs status;
    int err = statvfs(fullpath.c_str(), &status);
    if (err  == 0) {
        return status.f_bfree*status.f_bsize;
    }
    return 0;
#endif
}

/**
 * Returns the number of available bytes in the partition for this path name.
 *
 * The value is for the partition containing the given file or directory. If
 * the path name is relative, it assumes that the path name is in the asset
 * directory.
 *
 * If the path name does not refer to a proper volume, this function will
 * return 0.  On some platforms (e.g. Android) the asset directory is not a
 * proper volume and this function will return 0.
 *
 * This function is similar to {@link getFreeSpace} except that it measures
 * the number of bytes available for unpriviledged users.
 *
 * @param path  The file path name
 *
 * @return the number of available bytes in the partition for this path name.
 */
size_t vol_available_space(const std::string path) {
    std::string fullpath = to_absolute(path);
#if defined (__WINDOWS__)
    ULARGE_INTEGER freeBytesAvail;
    ULARGE_INTEGER totalNumBytes;
    ULARGE_INTEGER totalBytesAvail;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(fullpath);
    bool okay = GetDiskFreeSpaceEx(wide.c_str(), &freeBytesAvail, &totalNumBytes, &totalBytesAvail);
    if (okay) {
        return (size_t)freeBytesAvail.QuadPart;
    }
    return 0;
#else
    struct statvfs status;
    int err = statvfs(fullpath.c_str(), &status);
    if (err  == 0) {
        return status.f_bavail*status.f_bsize;
    }
    return 0;
#endif
}

/**
 * Returns the size of the partition named by this path name.
 *
 * The value is for the partition containing the given file or directory. If
 * the path name is relative, it assumes that the path name is in the asset
 * directory.
 *
 * If the path name does not refer to a proper volume, this function will
 * return 0.  On some platforms (e.g. Android) the asset directory is not a
 * proper volume and this function will return 0.
 *
 * @param path  The file path name
 *
 * @return the size of the partition named by this path name.
 */
size_t vol_total_space(const std::string path) {
    std::string fullpath = to_absolute(path);
#if defined (__WINDOWS__)
    ULARGE_INTEGER freeBytesAvail;
    ULARGE_INTEGER totalNumBytes;
    ULARGE_INTEGER totalBytesAvail;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(fullpath);
    bool okay = GetDiskFreeSpaceEx(wide.c_str(), &freeBytesAvail, &totalNumBytes, &totalBytesAvail);
    if (okay) {
        return (size_t)totalNumBytes.QuadPart;
    }
    return 0;
#else
    struct statvfs status;
    int err = statvfs(fullpath.c_str(), &status);
    if (err  == 0) {
        return status.f_blocks*status.f_bsize;
    }
    return 0;
#endif
}

}
}
