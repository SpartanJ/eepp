#ifndef EE_SYSTEMCPACK_HPP
#define EE_SYSTEMCPACK_HPP

#include <eepp/system/base.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/safedatapointer.hpp>
#include <eepp/system/iostream.hpp>

namespace EE { namespace System {

/** @brief Base class for all packing classes */
class EE_API Pack : protected Mutex {
	public:
		Pack();

		virtual ~Pack();

		/** Creates a new pack file */
		virtual bool create( const std::string& path ) = 0;

		/** Open a pack file */
		virtual bool open( const std::string& path ) = 0;

		/** Close the pack file */
		virtual bool close() = 0;

		/** Add a file to the pack file
		* @param path Path to the file in the disk
		* @param inpack Path that will have the file inside the pak
		* @return True if success
		*/
		virtual bool addFile( const std::string& path, const std::string& inpack ) = 0;

		/** Add a new file from memory */
		virtual bool addFile( std::vector<Uint8>& data, const std::string& inpack ) = 0;

		/** Add a new file from memory */
		virtual bool addFile( const Uint8 * data, const Uint32& dataSize, const std::string& inpack ) = 0;

		/** Add a map of files to the pack file ( myMap[ myFilepath ] = myInPackFilepath ) */
		virtual bool addFiles( std::map<std::string, std::string> paths ) = 0;

		/** Erase a file from the pack file. ( This will create a new pack file without that file, so, can be slow ) */
		virtual bool eraseFile( const std::string& path ) = 0;

		/** Erase all passed files from the pack file. ( This will create a new pack file without that file, so, can be slow ) */
		virtual bool eraseFiles( const std::vector<std::string>& paths ) = 0;

		/** Extract a file from the pack file */
		virtual bool extractFile( const std::string& path , const std::string& dest ) = 0;

		/** Extract a file to memory from the pack file */
		virtual bool extractFileToMemory( const std::string& path, std::vector<Uint8>& data ) = 0;

		/** Extract a file to memory from the pack file */
		virtual bool extractFileToMemory( const std::string& path, SafeDataPointer& data ) = 0;

		/** Check if a file exists in the pack file and return the number of the file, otherwise return -1. */
		virtual Int32 exists( const std::string& path ) = 0;

		/** Check the integrity of the pack file. \n If return 0 integrity OK. -1 wrong indentifier. -2 wrong header. */
		virtual Int8 checkPack() = 0;

		/** @return a vector with all the files inside the pack file */
		virtual std::vector<std::string> getFileList() = 0;

		/** @return If the pack file is open */
		virtual bool isOpen() const;

		/** @return The file path of the opened package */
		virtual std::string getPackPath() = 0;

		/** Open a file stream for reading */
		virtual IOStream * getFileStream( const std::string& path ) = 0;
	protected:
		bool mIsOpen;
};

}}
#endif
