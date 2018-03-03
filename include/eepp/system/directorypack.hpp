#ifndef EE_SYSTEM_DIRECTORYPACK_HPP
#define EE_SYSTEM_DIRECTORYPACK_HPP

#include <eepp/system/pack.hpp>

namespace EE { namespace System {

/** @brief A local file system directory. */
class EE_API DirectoryPack : public Pack {
	public:
		DirectoryPack();

		~DirectoryPack();

		/** Creates a new pack file */
		bool create( const std::string& path );

		/** Open a pack file */
		bool open( const std::string& path );

		/** Close the pack file */
		bool close();

		/** Add a file to the pack file
		* @param path Path to the file in the disk
		* @param inpack Path that will have the file inside the pak
		* @return True if success
		*/
		bool addFile( const std::string& path, const std::string& inpack );

		/** Add a new file from memory */
		bool addFile( const Uint8 * data, const Uint32& dataSize, const std::string& inpack );

		/** Add a new file from memory */
		bool addFile( std::vector<Uint8>& data, const std::string& inpack );

		/** Add a map of files to the pack file ( myMap[ myFilepath ] = myInPackFilepath ) */
		bool addFiles( std::map<std::string, std::string> paths );

		/** Erase a file from the pack file.*/
		bool eraseFile( const std::string& path );

		/** Erase all passed files from the pack file. */
		bool eraseFiles( const std::vector<std::string>& paths );

		/** Extract a file from the pack file */
		bool extractFile( const std::string& path , const std::string& dest );

		/** Extract a file to memory from the pack file */
		bool extractFileToMemory( const std::string& path, std::vector<Uint8>& data );

		/** Extract a file to memory from the pakFile */
		bool extractFileToMemory( const std::string& path, SafeDataPointer& data );

		/** Check if a file exists in the pack file and return the number of the file, otherwise return -1. */
		Int32 exists( const std::string& path );

		/** Check the integrity of the pack file. \n If return 0 integrity OK. -1 wrong indentifier. -2 wrong header. */
		Int8 checkPack();

		/** @return a vector with all the files inside the pack file */
		std::vector<std::string> getFileList();

		/** @return The file path of the opened package */
		std::string getPackPath();

		IOStream * getFileStream( const std::string& path );
	protected:
		std::string mPath;

		void getDirectoryFiles(std::vector<std::string> & files, std::string directory);
};

}}
#endif

