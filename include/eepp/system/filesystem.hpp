#ifndef EE_SYSTEM_FILESYSTEM_HPP
#define EE_SYSTEM_FILESYSTEM_HPP

#include <eepp/system/base.hpp>
#include <eepp/system/safedatapointer.hpp>

namespace EE { namespace System {

class EE_API FileSystem {
	public:
		/** @return The default slash path code of the current OS */
		static std::string GetOSlash();

		/** @return True if the file exists ( false if is a directory, to know if directory exists use IsDirectory ) */
		static bool FileExists( const std::string& Filepath );

		/** Copy a file to memory
		* @param path The file path
		* @param data The vector to allocate the file in memory
		* @return True if returned the file to the vector.
		*/
		static bool FileGet( const std::string& path, std::vector<Uint8>& data );

		/** Copy a file to memory
		* @param path The file path
		* @param data A SafeDataPointer to allocate the data to memory
		* @return True if returned the file to the SafeDataPointer
		*/
		static bool FileGet( const std::string& path, SafeDataPointer& data );

		/** Copy a file to location.
		* @param src Source File Path
		* @param dst Destination File Path
		* @return If success.
		*/
		static bool FileCopy( const std::string& src, const std::string& dst );

		/** @return The file extension
		* @param filepath The file path or name
		* @param lowerExt Lowercase the extension obtained? ( true by default )
		*/
		static std::string FileExtension( const std::string& filepath, const bool& lowerExt = true );

		/** @return The file name of a file path */
		static std::string FileNameFromPath( const std::string& filepath );

		/** @return Removes the file name from a path, and return the path. */
		static std::string FileRemoveFileName( const std::string& filepath );

		/** @return Removes the extension of a filepath */
		static std::string FileRemoveExtension( const std::string& filepath );

		/** Removes the process path to a file path */
		static void FilePathRemoveProcessPath( std::string& path );

		/** Write a file in binary mode and close it. */
		static bool FileWrite( const std::string& filepath, const Uint8* data, const Uint32& dataSize );

		/** Write a file in binary mode and close it. */
		static bool FileWrite( const std::string& filepath, const std::vector<Uint8>& data );

		/** Deletes a file from the file system. */
		static bool FileRemove( const std::string& filepath );

		/** @return The modification date of the file */
		static Uint32 FileGetModificationDate( const std::string& Filepath );

		/** If the directory path not end with a slash, it will add it. */
		static void DirPathAddSlashAtEnd( std::string& path );

		/** Move up from directory tree */
		static std::string RemoveLastFolderFromPath( std::string path );

		/** @return The files and sub directories contained by a directory */
		static std::vector<std::string> FilesGetInPath( const std::string& path );

		/** @return The files and sub directories contained by a directory */
		static std::vector<String> FilesGetInPath( const String& path );

		/** @return The size of a file */
		static Uint64 FileSize( const std::string& Filepath );
		
		/** @return If directory exists, and is a directory */
		static bool IsDirectory( const std::string& path );

		/** @return If directory exists, and is a directory */
		static bool IsDirectory( const String& path );

		/** Creates a new directory */
		static bool MakeDir( const std::string& path, const Uint16& mode = 0770 );

		/** Convert a size represented in bytes, to a string converted in byes/kb/mb/tb.
		* For example 10485760 -> "10.0 MB"
		*/
		static std::string SizeToString(const Int64& Size );

};

}}

#endif
