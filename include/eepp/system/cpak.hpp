#ifndef EE_SYSTEMCPAK_HPP
#define EE_SYSTEMCPAK_HPP

#include <eepp/system/base.hpp>
#include <eepp/system/cpack.hpp>
#include <eepp/system/ciostreamfile.hpp>

namespace EE { namespace System {

/** @brief Quake 2 PAK handler */
class EE_API cPak : public cPack {
	public:
		cPak();
		~cPak();

		/** Creates a new pakFile */
		bool Create( const std::string& path );

		/** Open a pakFile */
		bool Open( const std::string& path );

		/** Close the pakFile */
		bool Close();

		/** Add a file to the pakFile
		* @param path Path to the file in the disk
		* @param inpack Path that will have the file inside the pak
		* @return True if success
		*/
		bool AddFile( const std::string& path, const std::string& inpack );

		/** Add a new file from memory */
		bool AddFile( std::vector<Uint8>& data, const std::string& inpack );

		/** Add a new file from memory */
		bool AddFile( const Uint8 * data, const Uint32& dataSize, const std::string& inpack );

		/** Add a map of files to the pakFile ( myMap[ myFilepath ] = myInPakFilepath ) */
		bool AddFiles( std::map<std::string, std::string> paths );

		/** Erase a file from the pakFile. ( This will create a new pakFile without that file, so, can be slow ) */
		bool EraseFile( const std::string& path );

		/** Erase all passed files from the pakFile. ( This will create a new pakFile without that file, so, can be slow ) */
		bool EraseFiles( const std::vector<std::string>& paths );

		/** Extract a file from the pakFile */
		bool ExtractFile( const std::string& path , const std::string& dest );

		/** Extract a file to memory from the pakFile */
		bool ExtractFileToMemory( const std::string& path, std::vector<Uint8>& data );

		/** Extract a file to memory from the pakFile */
		bool ExtractFileToMemory( const std::string& path, SafeDataPointer& data );

		/** Check if a file exists in the pakFile and return the number of the file, otherwise return -1. */
		Int32 Exists( const std::string& path );

		/** Check the integrity of the pakFile. \n If return 0 integrity OK. -1 wrong indentifier. -2 wrong header. */
		Int8 CheckPack();

		/** @return a vector with all the files inside the pakFile */
		std::vector<std::string> GetFileList();

		/** @return If the PAK file is open */
		bool IsOpen() const;

		/** @return The file path of the opened package */
		std::string GetPackPath();
	protected:

	private:
		typedef struct pakheader_t {
			char head[4];		//! Header of the file ( default: 'PACK' )
			Uint32 dir_offset; 	//! Offset to the first pakEntry on the pakFile
			Uint32 dir_length; 	//! Space ocuped by all the pakEntrys ( num of pakEntrys = dir_length / sizeof(pakEntry) )
		} pakHeader; //! The header of the file

		typedef struct dirsection_t {
			char filename[56];		//! File name
			Uint32 file_position;	//! The file position on the file ( in bytes )
			Uint32 file_length;		//! THe file length ( in bytes )
		} pakEntry;	//! The stored file info

		typedef struct pakfile_t {
			cIOStreamFile * fs;
			pakHeader header;
			Uint32 pakFilesNum;
			std::string pakPath;
		} pakFile;

		pakFile					mPak;
		std::vector<pakEntry>	mPakFiles;
};

}}
#endif
