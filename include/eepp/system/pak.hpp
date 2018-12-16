#ifndef EE_SYSTEMCPAK_HPP
#define EE_SYSTEMCPAK_HPP

#include <eepp/system/pack.hpp>
#include <eepp/system/iostreamfile.hpp>

namespace EE { namespace System {

/** @brief Quake 2 PAK handler */
class EE_API Pak : public Pack {
	public:
		Pak();
		~Pak();

		/** Creates a new pakFile */
		bool create( const std::string& path );

		/** Open a pakFile */
		bool open( const std::string& path );

		/** Close the pakFile */
		bool close();

		/** Add a file to the pakFile
		* @param path Path to the file in the disk
		* @param inpack Path that will have the file inside the pak
		* @return True if success
		*/
		bool addFile( const std::string& path, const std::string& inpack );

		/** Add a new file from memory */
		bool addFile( std::vector<Uint8>& data, const std::string& inpack );

		/** Add a new file from memory */
		bool addFile( const Uint8 * data, const Uint32& dataSize, const std::string& inpack );

		/** Add a map of files to the pakFile ( myMap[ myFilepath ] = myInPakFilepath ) */
		bool addFiles( std::map<std::string, std::string> paths );

		/** Erase a file from the pakFile. ( This will create a new pakFile without that file, so, can be slow ) */
		bool eraseFile( const std::string& path );

		/** Erase all passed files from the pakFile. ( This will create a new pakFile without that file, so, can be slow ) */
		bool eraseFiles( const std::vector<std::string>& paths );

		/** Extract a file from the pakFile */
		bool extractFile( const std::string& path , const std::string& dest );

		/** Extract a file to memory from the pakFile */
		bool extractFileToMemory( const std::string& path, std::vector<Uint8>& data );

		/** Extract a file to memory from the pakFile */
		bool extractFileToMemory( const std::string& path, SafeDataPointer& data );

		/** Check if a file exists in the pakFile and return the number of the file, otherwise return -1. */
		Int32 exists( const std::string& path );

		/** Check the integrity of the pakFile. \n If return 0 integrity OK. -1 wrong indentifier. -2 wrong header. */
		Int8 checkPack();

		/** @return a vector with all the files inside the pakFile */
		std::vector<std::string> getFileList();

		/** @return If the PAK file is open */
		bool isOpen() const;

		/** @return The file path of the opened package */
		std::string getPackPath();

		IOStream * getFileStream( const std::string& path );
	protected:
		friend class IOStreamPak;

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
			IOStreamFile * fs;
			pakHeader header;
			Uint32 pakFilesNum;
			std::string pakPath;
		} pakFile;

		pakFile					mPak;
		std::vector<pakEntry>	mPakFiles;

		pakEntry getPackEntry( Uint32 index );
};

}}
#endif
