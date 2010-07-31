#ifndef EE_UTILSCUTILS_H
#define EE_UTILSCUTILS_H

#include "base.hpp"

namespace EE { namespace Utils {
	/** @return True if the file exists */
	bool EE_API FileExists(const std::string& filepath);

	/** @return The number of milliseconds since the EE++ library initialization. Note that this value wraps if the program runs for more than ~49 days. */
	Uint32 EE_API eeGetTicks();

	/** Wait a specified number of milliseconds before returning. */
	void EE_API eeSleep( const Uint32& ms );

	/** @return The application path ( the executable path ) */
	std::string EE_API AppPath();

	/** @return The files and sub directories contained by a directory */
	std::vector<std::string> EE_API FilesGetInPath( const std::string& path );

	/** @return The size of a file */
	Uint32 EE_API FileSize( const std::string& Filepath);

	/** @return The System Time */
	eeDouble EE_API GetSystemTime();

	/** @return If directory exists, and is a directory */
	bool EE_API IsDirectory( const std::string& path );

	/** Creates a new directory */
	bool EE_API MakeDir( const std::string& path, const Uint16& mode = 0770 );

	/** @return The default windows directory */
	std::string EE_API GetWindowsPath();

	/** @return djb2 wstring hash */
	Uint32 EE_API MakeHash( const std::wstring& str );

	/** @return djb2 string hash */
	Uint32 EE_API MakeHash( const std::string& str );

	/** @return djb2 string hash */
	Uint32 EE_API MakeHash( const Int8 *str );

	/** Copy a file to memory
	* @param path The file path
	* @param data The vector to allocate the file in memory
	* @return True if returned the file to the vector.
	*/
	bool FileGet( const std::string& path, std::vector<Uint8>& data );

	/** Copy a file to location.
	* @param src Source File Path
	* @param dst Destination File Path
	* @return If success.
	*/
	bool FileCopy( const std::string& src, const std::string& dst );

	/** @return The file extension
	* @param filepath The file path or name
	* @param lowerExt Lowercase the extension obtained? ( true by default )
	*/
	std::string FileExtension( const std::string& filepath, const bool& lowerExt = true );

	/** @return The Number of CPUs of the system. */
	eeInt GetNumCPUs();
}

}
#endif
