#ifndef EE_SYSTEMCZIP_HPP
#define EE_SYSTEMCZIP_HPP

#include "base.hpp"
#include "cpack.hpp"
#include "../helper/libzip/zip.h"
#include "../helper/libzip/zipint.h"

namespace EE { namespace System {

/** @brief Zip files package manager. */
class EE_API cZip : public cPack {
	public:
		cZip();

		~cZip();

		/** Creates a new pack file */
		bool Create( const std::string& path );

		/** Open a pack file */
		bool Open( const std::string& path );

		/** Close the pack file */
		bool Close();

		/** Add a file to the pack file
		* @param path Path to the file in the disk
		* @param inpack Path that will have the file inside the pak
		* @return True if success
		*/
		bool AddFile( const std::string& path, const std::string& inpack );

		/** Add a new file from memory */
		bool AddFile( const Uint8 * data, const Uint32& dataSize, const std::string& inpack );

		/** Add a map of files to the pack file ( myMap[ myFilepath ] = myInPackFilepath ) */
		bool AddFiles( std::map<std::string, std::string> paths );

		/** Erase a file from the pack file. ( This will create a new pack file without that file, so, can be slow ) */
		bool EraseFile( const std::string& path );

		/** Erase all passed files from the pack file. ( This will create a new pack file without that file, so, can be slow ) */
		bool EraseFiles( const std::vector<std::string>& paths );

		/** Extract a file from the pack file */
		bool ExtractFile( const std::string& path , const std::string& dest );

		/** Extract a file to memory from the pack file */
		bool ExtractFileToMemory( const std::string& path, std::vector<Uint8>& data );

		/** Extract a file to memory from the pakFile */
		bool ExtractFileToMemory( const std::string& path, SafePointerData& data );

		/** Check if a file exists in the pack file and return the number of the file, otherwise return -1. */
		Int32 Exists( const std::string& path );

		/** Check the integrity of the pack file. \n If return 0 integrity OK. -1 wrong indentifier. -2 wrong header. */
		Int8 CheckPack();

		/** @return a vector with all the files inside the pack file */
		std::vector<std::string> GetFileList();

		/** Add a new file from memory */
		bool AddFile( std::vector<Uint8>& data, const std::string& inpack );
	protected:
		struct zip * mZip;

		std::string mZipPath;
};

}}
#endif

