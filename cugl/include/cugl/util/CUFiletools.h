//
//  CUFiletools.h
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
#ifndef __CU_FILE_TOOLS_H__
#define __CU_FILE_TOOLS_H__
#include <string>
#include <vector>
#include <functional>
#include <initializer_list>

namespace cugl {
	/**
	 * Functions for file management.
	 *
	 * This namespace provides several tools for querying and constructing file 
	 * paths in an OS independent way. It is largely a collection of namespaced 
	 * functions, much like the stringtool module. It is based off the the `os.path` 
	 * module in Python.
	 */
    namespace filetool {

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
    bool is_file(const std::string path);

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
    bool is_dir(const std::string path);

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
    bool is_hidden(const std::string path);

    /**
     * Returns true if this path name is absolute.
     *
     * An absolute path name has an explicit volume and path from the
     * volume. This function does not require that the file exist. It only
     * checks the naming convention of the file referenced by this path.
     *
     * @return true if this path name is absolute.
    */
    bool is_absolute(const std::string path);

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
    bool file_exists(const std::string path);

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
    const std::string file_vol(const std::string path);
    
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
    size_t file_size(const std::string path);

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
    Uint64 file_timestamp(const std::string path);

#pragma mark -
#pragma mark Path Manipulation
    /**
     * The system-dependent path separator for this platform.
     */
    extern char path_sep;

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
    const std::string dir_name(const std::string path);

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
    const std::string base_name(const std::string path);

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
    std::pair<std::string, std::string> split_path(const std::string path);

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
    std::vector<std::string> fullsplit_path(const std::string path);

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
    const std::string base_prefix(const std::string path);

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
    const std::string base_suffix(const std::string path);

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
     * @param suffix  The suffix to test
     *
     * @return a copy of the path name with the given suffix.
     */
    const std::string set_suffix(const std::string path, const std::string suffix);

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
    std::pair<std::string, std::string> split_base(const std::string path);

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
    const std::string normalize_path(const std::string path);

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
    const std::string canonicalize_path(const std::string path);
    
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
    const std::string common_path(const std::initializer_list<std::string> paths);

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
    const std::string common_path(const std::vector<std::string> paths);

    /**
     * Returns the common subpath of the given paths.
     *
     * If there is no common prefix, or if the paths are a mixture of absolute and
     * relative paths, then this function returns the empty string.
     *
     * @param paths    The collection of paths to search
     * @param size     The number of elements in the arrray
     *
     * @return the common subpath of the given paths.
     */
    const std::string common_path(const std::string* paths, size_t size);

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
    const std::string join_path(std::initializer_list<std::string> elts);

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
    const std::string join_path(std::vector<std::string> elts);

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
    const std::string join_path(const std::string* elts, size_t size);


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
    bool file_create(const std::string path);

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
    bool file_delete(const std::string path);
    
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
    std::vector<std::string> dir_contents(const std::string path);

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
     * @param filter    The filter to apply to file names
     *
     * @return a filtered list of strings naming the files and directories in this path
     */
    std::vector<std::string> dir_contents(const std::string path,
                                          const std::function<bool(const std::string file)>& filter);

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
    bool dir_create(const std::string path);
    
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
    bool dir_delete(const std::string path);


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
    bool is_readable(const std::string path);

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
    bool is_searchable(const std::string path);

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
    bool is_writable(const std::string path);

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
    bool set_readable(const std::string path, bool readable);
    
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
    bool set_readable(const std::string path, bool readable, bool ownerOnly);
    
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
    bool set_readonly(const std::string path);

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
    bool set_searchable(const std::string path, bool searchable);
    
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
    bool set_searchable(const std::string path, bool searchable, bool ownerOnly);

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
    bool set_writable(const std::string path, bool writable);

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
    bool set_writable(const std::string path, bool writable, bool ownerOnly);
    
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
    size_t vol_free_space(const std::string path);

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
     * This function is similar to {@link vol_free_space} except that it measures
     * the number of bytes available for unpriviledged users.
     *
     * @param path  The file path name
     *
     * @return the number of available bytes in the partition for this path name.
     */
    size_t vol_available_space(const std::string path);

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
    size_t vol_total_space(const std::string path);
    
    }
}
#endif /* __CU_FILE_TOOLS_H__ */
