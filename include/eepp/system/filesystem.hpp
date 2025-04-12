#ifndef EE_SYSTEM_FILESYSTEM_HPP
#define EE_SYSTEM_FILESYSTEM_HPP

#include <eepp/core.hpp>
#include <eepp/system/fileinfo.hpp>
#include <eepp/system/scopedbuffer.hpp>
#include <string>
#include <vector>

namespace EE { namespace System {

class EE_API FileSystem {
  public:
	/** @return The default slash path code of the current OS */
	static std::string getOSSlash();

	/** @return True if the file exists ( false if is a directory, to know if directory exists use
	 * isDirectory ) */
	static bool fileExists( const std::string& Filepath );

	/** Copy a file to memory
	 * @param path The file path
	 * @param data The vector to allocate the file in memory
	 * @return True if returned the file to the vector.
	 */
	static bool fileGet( const std::string& path, std::vector<Uint8>& data );

	/** Copy a file to memory
	 * @param path The file path
	 * @param data A ScopedBuffer to allocate the data to memory
	 * @return True if returned the file to the ScopedBuffer
	 */
	static bool fileGet( const std::string& path, ScopedBuffer& data );

	/** Copy a file to memory
	 * @param path The file path
	 * @param data The string to allocate the file in memory
	 * @return True if returned the file to the vector.
	 */
	static bool fileGet( const std::string& path, std::string& data );

	/** Copy a file to location.
	 * @param src Source File Path
	 * @param dst Destination File Path
	 * @return If success.
	 */
	static bool fileCopy( const std::string& src, const std::string& dst );

	/** @return The file extension
	 * @param filepath The file path or name
	 * @param lowerExt Lowercase the extension obtained? ( true by default )
	 */
	static std::string fileExtension( const std::string& filepath, const bool& lowerExt = true );

	/** @return The file name of a file path */
	static std::string fileNameFromPath( const std::string& filepath );

	/** @return Removes the file name from a path, and return the path. */
	static std::string fileRemoveFileName( const std::string& filepath );

	/** @return Removes the extension of a filepath */
	static std::string fileRemoveExtension( const std::string& filepath );

	/** Removes the process path to a file path */
	static void filePathRemoveProcessPath( std::string& path );

	/** Removes a base path from a file path */
	static void filePathRemoveBasePath( const std::string& basePath, std::string& path );

	/** Write a file in binary mode and close it. */
	static bool fileWrite( const std::string& filepath, const Uint8* data, const Uint32& dataSize );

	/** Write a file in binary mode and close it. */
	static bool fileWrite( const std::string& filepath, const std::vector<Uint8>& data );

	/** Write a file in binary mode and close it. */
	static bool fileWrite( const std::string& filepath, const std::string& data );

	/** Deletes a file from the file system. */
	static bool fileRemove( const std::string& filepath );

	/** Hides the file in the file system */
	static bool fileHide( const std::string& filepath );

	/** @return The modification date of the file */
	static Uint32 fileGetModificationDate( const std::string& filepath );

	/** @return If a file path is writeable */
	static bool fileCanWrite( const std::string& filepath );

	/** @return If the file is hidden (hidden attribute in Windows, files starting with '.' in other
	 * systems */
	static bool fileIsHidden( const std::string& filepath );

	/** If the directory path not end with a slash, it will add it. */
	static void dirAddSlashAtEnd( std::string& path );

	/** If the directory path ends with a slash, it will remove it. */
	static void dirRemoveSlashAtEnd( std::string& dir );

	/** Move up from directory tree */
	static std::string removeLastFolderFromPath( std::string path );

	/** @return The files and sub directories contained by a directory */
	static std::vector<std::string> filesGetInPath( const std::string& path,
													const bool& sortByName = false,
													const bool& foldersFirst = false,
													const bool& ignoreHidden = false,
													const std::function<bool()> shouldAbort = {} );

	/** @return The files and sub directories contained by a directory */
	static std::vector<String> filesGetInPath( const String& path, const bool& sortByName = false,
											   const bool& foldersFirst = false,
											   const bool& ignoreHidden = false,
											   const std::function<bool()> shouldAbort = {} );

	/** @return The file info of the files and sub directories contained in the directory path. */
	static std::vector<FileInfo> filesInfoGetInPath( std::string path, bool linkInfo = false,
													 const bool& sortByName = false,
													 const bool& foldersFirst = false,
													 const bool& ignoreHidden = false,
													 const std::function<bool()> shouldAbort = {} );

	/** @return The size of a file */
	static Uint64 fileSize( const std::string& Filepath );

	/** @return If directory exists, and is a directory */
	static bool isDirectory( const std::string& path );

	/** @return If directory exists, and is a directory */
	static bool isDirectory( const String& path );

	/** Creates a new directory */
	static bool makeDir( const std::string& path, bool recursive = false,
						 const Uint16& mode = 0770 );

	/** @return The absolute path of a relative path */
	static std::string getRealPath( const std::string& path );

	/** Convert a size represented in bytes, to a string converted in byes/kb/mb/tb.
	 * For example 10485760 -> "10.0 MB"
	 */
	static std::string sizeToString( const Int64& Size );

	/** Change the process current working directory */
	static bool changeWorkingDirectory( const std::string& path );

	/** Gets the current working directory */
	static std::string getCurrentWorkingDirectory();

	/** Removes the current working directory from a path */
	static void filePathRemoveCurrentWorkingDirectory( std::string& path );

	/** @return Returns free disk space for a given path in bytes */
	static Int64 getDiskFreeSpace( const std::string& path );

	/** Creates a file name available for the directory path.
	 * For file name "file-name-" will search the first available name
	 * in the file system starting from file-name-1, file-name-2, and so on.
	 * @return The file name found, otherwise empty string if error.
	 */
	static std::string fileGetNumberedFileNameFromPath( std::string directoryPath,
														const std::string& fileName,
														const std::string& separator = ".",
														const std::string& fileExtension = "" );

	/** @returns True if the path provided is relative. */
	static bool isRelativePath( const std::string& path );

	/** Opens a file path with a path and mode encoded in UTF-8 */
	static FILE* fopenUtf8( const char* path, const char* mode );

	/** Opens a file path with a path and mode encoded in UTF-8 */
	static FILE* fopenUtf8( const std::string& path, const std::string& mode );
};

}} // namespace EE::System

#endif
