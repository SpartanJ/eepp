#include "cpak.hpp"

namespace EE { namespace System {

cPak::cPak() :
	cPack()
{
}

cPak::~cPak() {
	Close();
}

bool cPak::Create( const std::string& path ) {
	if ( !FileExists(path) ) {
		pakFile Pak;

		Pak.header.head[0] = 'P';
		Pak.header.head[1] = 'A';
		Pak.header.head[2] = 'C';
		Pak.header.head[3] = 'K';

		Pak.header.dir_offset = sizeof(Pak.header.head) + 1;
		Pak.header.dir_length = 1;

		myPak.fs.open( path.c_str() , std::ios::out | std::ios::binary ); // Open the PAK file

		myPak.fs.write( reinterpret_cast<const char*> (&Pak.header), sizeof(Pak.header) );

		myPak.fs.close();

		Open( path );

		return true;
	} else {
		return Open( path );
	}

	return false;
}

bool cPak::Open( const std::string& path ) {
	if ( FileExists(path) ) {
		myPak.pakPath = path;

		myPak.fs.open( path.c_str() , std::ios::in | std::ios::out | std::ios::binary ); // Open the PAK file

		myPak.fs.read( reinterpret_cast<char*> (&myPak.header), sizeof(pakHeader) ); // Read the PAK header

		if ( CheckPack() == 0 ) {
			myPak.pakFilesNum = myPak.header.dir_length / 64; // Number of files in the PAK

			myPak.fs.seekg( myPak.header.dir_offset, std::ios::beg ); // Seek to read the pakEntrys

			for ( Uint32 i = 0; i < myPak.pakFilesNum; i++ ) { // Read all the pakEntrys
				pakEntry Entry;
				myPak.fs.read( reinterpret_cast<char*> (&Entry), sizeof(pakEntry) );
				pakFiles.push_back(Entry);
			}

			mIsOpen = true;
			return true;
		}
	}
    return false;
}

bool cPak::Close() {
	if ( myPak.fs.is_open() ) {
		myPak.fs.close();

		pakFiles.clear();

		mIsOpen = false;

		return true;
	}
	return false;
}

Int8 cPak::CheckPack() {
	if ( myPak.fs.is_open() ) {
  		if( myPak.header.head[0] != 'P' || myPak.header.head[1] != 'A' || myPak.header.head[2] != 'C' || myPak.header.head[3] != 'K')
    		return -1;	// Ident corrupt

  		if( myPak.header.dir_offset < (sizeof(myPak.header.head) + 1) || myPak.header.dir_length < 1)
    		return -2;	// Header corrupt
	}
  	return 0;
}

Int32 cPak::Exists( const std::string& path ) {
	for ( Uint32 i = 0; i < pakFiles.size(); i++ )
		if ( strcmp( path.c_str(), pakFiles[i].filename ) == 0 )
			return i;

	return -1;
}

bool cPak::ExtractFile( const std::string& path , const std::string& dest ) {
	Lock();

	bool Ret = false;

	Int32 Pos = Exists( path );
	if ( Pos != -1 ) {
		std::fstream fs ( dest.c_str() , std::ios::out | std::ios::binary );

		std::vector<Uint8> tmpv;
		if ( ExtractFileToMemory( path, tmpv ) )
			fs.write( reinterpret_cast<const char*> (&tmpv[0]), pakFiles[Pos].file_length );

		fs.close();
		Ret = true;
	}

	Unlock();

	return Ret;
}

bool cPak::ExtractFileToMemory( const std::string& path, std::vector<Uint8>& data ) {
	Lock();

	bool Ret = false;

	Int32 Pos = Exists( path );

	if ( Pos != -1 ) {
		data.clear();
		data.resize( pakFiles[Pos].file_length );

		myPak.fs.seekg( pakFiles[Pos].file_position, std::ios::beg );
		myPak.fs.read( reinterpret_cast<char*> (&data[0]), pakFiles[Pos].file_length );

		Ret = true;
	}

	Unlock();

	return Ret;
}

bool cPak::ExtractFileToMemory( const std::string& path, SafeDataPointer& data ) {
	Lock();

	bool Ret = false;

	Int32 Pos = Exists( path );

	if ( Pos != -1 ) {
		data.DataSize	= pakFiles[Pos].file_length;
		data.Data		= eeNewArray( Uint8, ( data.DataSize ) );

		myPak.fs.seekg( pakFiles[Pos].file_position, std::ios::beg );
		myPak.fs.read( reinterpret_cast<char*> ( data.Data ), pakFiles[Pos].file_length );

		Ret = true;
	}

	Unlock();

	return Ret;
}

bool cPak::AddFile( const Uint8 * data, const Uint32& dataSize, const std::string& inpack ) {
	if ( dataSize < 1 )
		return false;

	Uint32 fsize = dataSize;

	if ( myPak.fs.is_open() ) {
		if ( myPak.header.dir_length == 1 ) {
			myPak.header.dir_offset = sizeof(pakHeader) + fsize;
			myPak.header.dir_length = sizeof(pakEntry);
			myPak.pakFilesNum = 1;

			myPak.fs.seekg( 4 , std::ios::beg ); // seek after head (PACK)
			myPak.fs.write( reinterpret_cast<const char*> (&myPak.header.dir_offset), sizeof( myPak.header.dir_offset ) );
			myPak.fs.write( reinterpret_cast<const char*> (&myPak.header.dir_length), sizeof( myPak.header.dir_length ) );

			myPak.fs.write( reinterpret_cast<const char*> (&data[0]), fsize );

			pakEntry newFile;
			StrCopy( newFile.filename, inpack.c_str(), 56 );
			newFile.file_position = sizeof(pakHeader);
			newFile.file_length = fsize;

			myPak.fs.write( reinterpret_cast<const char*> (&newFile), sizeof( pakEntry ) );

			pakFiles.push_back( newFile );

			return true;
		} else {
			if ( Exists( inpack ) != -1 ) // If the file already exists exit
				return false;

			if ( myPak.header.dir_length % 64 != 0 ) // Corrupted file?
				return false;

			std::vector<pakEntry> pakE;
			pakE.resize( myPak.pakFilesNum + 1 );	// Alloc space for all the pakEntrys and the new one

			myPak.fs.seekg( myPak.header.dir_offset, std::ios::beg ); 	// seek to the file pakEntrys
			myPak.fs.read( reinterpret_cast<char*> (&pakE[0]), sizeof(pakEntry) * myPak.pakFilesNum ); 	// get all the pakEntrys

			myPak.header.dir_offset = myPak.header.dir_offset + fsize; 	// Update the new dir_offset
			myPak.header.dir_length = myPak.header.dir_length + sizeof(pakEntry); // Update the new dir_length

			myPak.fs.seekg( 4 , std::ios::beg ); // Update the new dir_offset and dir_length to the pakFile
			myPak.fs.write( reinterpret_cast<const char*> (&myPak.header.dir_offset), sizeof( myPak.header.dir_offset ) );
			myPak.fs.write( reinterpret_cast<const char*> (&myPak.header.dir_length), sizeof( myPak.header.dir_length ) );

			myPak.fs.seekg( (myPak.header.dir_offset - fsize), std::ios::beg ); // Seek to the file allocation zone
			myPak.fs.write( reinterpret_cast<const char*> (&data[0]), fsize ); // Alloc the file

			// Fill the new file data on the pakEntry
			StrCopy (pakE[ myPak.pakFilesNum ].filename, inpack.c_str(), 56 );

			pakE[ myPak.pakFilesNum ].file_position = myPak.header.dir_offset - fsize;
			pakE[ myPak.pakFilesNum ].file_length = fsize;

			// Update the new pakEntrys on pakFile
			myPak.fs.write( reinterpret_cast<const char*>(&pakE[0]), (std::streamsize)( sizeof(pakEntry) * pakE.size() ) );

			pakFiles.push_back( pakE[ myPak.pakFilesNum ] );
			myPak.pakFilesNum += 1;

			pakE.clear();

			return true;
		}
	}

	return false;
}

bool cPak::AddFile( std::vector<Uint8>& data, const std::string& inpack ) {
	return AddFile( reinterpret_cast<const Uint8*> ( &data[0] ), (Uint32)data.size(), inpack );
}

bool cPak::AddFile( const std::string& path, const std::string& inpack ) {
	if ( path.size() > 56 )
		return false;

	std::vector<Uint8> file;

	FileGet( path, file );

	return AddFile( file, inpack );
}

bool cPak::AddFiles( std::map<std::string, std::string> paths ) {
	for( std::map<std::string, std::string>::iterator itr = paths.begin(); itr != paths.end(); itr++)
		if ( !AddFile( itr->first, itr->second ) )
			return false;
    return true;
}

bool cPak::EraseFile( const std::string& path ) {
	std::vector<std::string> tmpv;
	tmpv.push_back( path );

	return EraseFiles( tmpv );
}

bool cPak::EraseFiles( const std::vector<std::string>& paths ) {
	std::vector<Int32> files;
	Int32 Ex;
	Uint32 total_offset = 0, i = 0;
	pakFile nPf;
	std::vector<pakEntry> uEntry;
	bool Remove;

	for ( i = 0; i < paths.size(); i++ ) {
		Ex = Exists( paths[i] );
		if ( Ex  == -1 )
			return false;
		else
			files.push_back( Ex );
	}

	nPf.pakPath = std::string ( myPak.pakPath + ".new" );

	nPf.fs.open ( nPf.pakPath.c_str() , std::ios::out | std::ios::binary );

	for ( i = 0; i < pakFiles.size(); i++ ) {
		Remove = false;

		for ( Uint32 u = 0; u < files.size(); u++ ) {
			if ( files[u] == static_cast<Int32>(i) )
				Remove = true;
		}

		if ( !Remove ) {
			uEntry.push_back( pakFiles[i] );
			total_offset += pakFiles[i].file_length;
		}
	}

	nPf.pakFilesNum = (Uint32)uEntry.size();
	nPf.header.head[0] = 'P'; nPf.header.head[1] = 'A'; nPf.header.head[2] = 'C'; nPf.header.head[3] = 'K';
	nPf.header.dir_offset = total_offset + sizeof(pakHeader);
	nPf.header.dir_length = (Uint32)uEntry.size() * sizeof( pakEntry );

	nPf.fs.write( reinterpret_cast<const char*>(&nPf.header), sizeof(pakHeader) );

	std::vector<Uint8> data;
	for ( i = 0; i < uEntry.size(); i++ )
		if ( ExtractFileToMemory( std::string( uEntry[i].filename ), data ) ) {
			uEntry[i].file_position = nPf.fs.tellg();
			uEntry[i].file_length = (Uint32)data.size();
			nPf.fs.write( reinterpret_cast<const char*>(&data[0]), (std::streamsize)data.size() );
		}

	nPf.fs.write( reinterpret_cast<const char*>(&uEntry[0]), (std::streamsize)( sizeof(pakEntry) * uEntry.size() ) );

	nPf.fs.close();

	remove( myPak.pakPath.c_str() );
	rename( nPf.pakPath.c_str(), myPak.pakPath.c_str() );

	Close();
	Open( myPak.pakPath );

	return true;

}

std::vector<std::string> cPak::GetFileList() {
	std::vector<std::string> tmpv;

	tmpv.resize( pakFiles.size() );

	for ( Uint32 i = 0; i < pakFiles.size(); i++ )
		tmpv[i] = std::string ( pakFiles[i].filename );

	return tmpv;
}

}}
