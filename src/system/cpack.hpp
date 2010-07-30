#ifndef EE_SYSTEMCPACK_HPP
#define EE_SYSTEMCPACK_HPP

#include "base.hpp"
#include "cmutex.hpp"

namespace EE { namespace System {

/** @brief Base class for al packing classes */
class EE_API cPack : protected cMutex {
	public:
		cPack();

		virtual ~cPack();

		/** Creates a new pack file */
		virtual bool Create( const std::string& path ) = 0;

		/** Open a pack file */
		virtual bool Open( const std::string& path ) = 0;

		/** Close the pack file */
		virtual bool Close() = 0;

		/** Add a file to the pack file
		* @param path Path to the file in the disk
		* @param inpack Path that will have the file inside the pak
		* @return True if success
		*/
		virtual bool AddFile( const std::string& path, const std::string& inpack ) = 0;

		/** Add a new file from memory */
		virtual bool AddFile( std::vector<Uint8>& data, const std::string& inpack ) = 0;

		/** Add a new file from memory */
		virtual bool AddFile( const Uint8 * data, const Uint32& dataSize, const std::string& inpack ) = 0;

		/** Add a map of files to the pack file ( myMap[ myFilepath ] = myInPackFilepath ) */
		virtual bool AddFiles( std::map<std::string, std::string> paths ) = 0;

		/** Erase a file from the pack file. ( This will create a new pack file without that file, so, can be slow ) */
		virtual bool EraseFile( const std::string& path ) = 0;

		/** Erase all passed files from the pack file. ( This will create a new pack file without that file, so, can be slow ) */
		virtual bool EraseFiles( const std::vector<std::string>& paths ) = 0;

		/** Extract a file from the pack file */
		virtual bool ExtractFile( const std::string& path , const std::string& dest ) = 0;

		/** Extract a file to memory from the pack file */
		virtual bool ExtractFileToMemory( const std::string& path, std::vector<Uint8>& data ) = 0;

		/** Extract a file to memory from the pack file */
		virtual bool ExtractFileToMemory( const std::string& path, Uint8** data, Uint32* dataSize ) = 0;

		/** Check if a file exists in the pack file and return the number of the file, otherwise return -1. */
		virtual Int32 Exists( const std::string& path ) = 0;

		/** Check the integrity of the pack file. \n If return 0 integrity OK. -1 wrong indentifier. -2 wrong header. */
		virtual Int8 CheckPack() = 0;

		/** @return a vector with all the files inside the pack file */
		virtual std::vector<std::string> GetFileList() = 0;

		/** @return If the pack file is open */
		virtual bool IsOpen() const;
	protected:
		bool mIsOpen;
};

}}
#endif
