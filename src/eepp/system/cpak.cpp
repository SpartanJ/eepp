#include <eepp/system/cpak.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/clog.hpp>

namespace EE { namespace System {

cPak::cPak() :
	cPack()
{
	mPak.fs = NULL;
}

cPak::~cPak() {
	Close();
}

bool cPak::Create( const std::string& path ) {
	if ( !FileSystem::FileExists(path) ) {
		pakFile Pak;

		Pak.header.head[0] = 'P';
		Pak.header.head[1] = 'A';
		Pak.header.head[2] = 'C';
		Pak.header.head[3] = 'K';

		Pak.header.dir_offset = sizeof(Pak.header.head) + 1;
		Pak.header.dir_length = 1;

		eeSAFE_DELETE( mPak.fs );

		mPak.fs = eeNew( cIOStreamFile, ( path , std::ios::out | std::ios::binary ) ); // Open the PAK file

		mPak.fs->Write( reinterpret_cast<const char*> (&Pak.header), sizeof(Pak.header) );

		eeSAFE_DELETE( mPak.fs );

		Open( path );

		return true;
	} else {
		return Open( path );
	}

	return false;
}

bool cPak::Open( const std::string& path ) {
	if ( FileSystem::FileExists(path) ) {
		mPak.pakPath = path;

		eeSAFE_DELETE( mPak.fs );

		mPak.fs = eeNew( cIOStreamFile, ( path , std::ios::in | std::ios::out | std::ios::binary ) ); // Open the PAK file

		mPak.fs->Read( reinterpret_cast<char*> (&mPak.header), sizeof(pakHeader) ); // Read the PAK header

		if ( CheckPack() == 0 ) {
			mPak.pakFilesNum = mPak.header.dir_length / 64; // Number of files in the PAK

			mPak.fs->Seek( mPak.header.dir_offset ); // Seek to read the pakEntrys

			for ( Uint32 i = 0; i < mPak.pakFilesNum; i++ ) { // Read all the pakEntrys
				pakEntry Entry;

				mPak.fs->Read( reinterpret_cast<char*> (&Entry), sizeof(pakEntry) );

				mPakFiles.push_back( Entry );
			}

			mIsOpen = true;

			return true;
		}
	}

    return false;
}

bool cPak::Close() {
	if ( mIsOpen ) {
		eeSAFE_DELETE( mPak.fs );

		mPakFiles.clear();

		mIsOpen = false;

		return true;
	}

	return false;
}

Int8 cPak::CheckPack() {
	if ( NULL != mPak.fs && mPak.fs->IsOpen() ) {
		if ( mPak.header.head[0] != 'P' || mPak.header.head[1] != 'A' || mPak.header.head[2] != 'C' || mPak.header.head[3] != 'K')
    		return -1;	// Ident corrupt

		if ( mPak.header.dir_offset < (sizeof(mPak.header.head) + 1) || mPak.header.dir_length < 1)
    		return -2;	// Header corrupt
	}

  	return 0;
}

Int32 cPak::Exists( const std::string& path ) {
	if ( IsOpen() ) {
		for ( Uint32 i = 0; i < mPakFiles.size(); i++ )
			if ( strcmp( path.c_str(), mPakFiles[i].filename ) == 0 )
				return i;
	}

	return -1;
}

bool cPak::ExtractFile( const std::string& path , const std::string& dest ) {
	if ( NULL == mPak.fs || !mPak.fs->IsOpen() ) {
		return false;
	}

	Lock();

	bool Ret = false;

	Int32 Pos = Exists( path );

	if ( Pos != -1 ) {
		SafeDataPointer data;

		if ( ExtractFileToMemory( path, data ) ) {
			FileSystem::FileWrite( path, data.Data, data.DataSize );
		}

		Ret = true;
	}

	Unlock();

	return Ret;
}

bool cPak::ExtractFileToMemory( const std::string& path, std::vector<Uint8>& data ) {
	if ( NULL == mPak.fs || !mPak.fs->IsOpen() ) {
		return false;
	}

	Lock();

	bool Ret = false;

	Int32 Pos = Exists( path );

	if ( Pos != -1 ) {
		data.clear();
		data.resize( mPakFiles[Pos].file_length );

		mPak.fs->Seek( mPakFiles[Pos].file_position );
		mPak.fs->Read( reinterpret_cast<char*> (&data[0]), mPakFiles[Pos].file_length );

		Ret = true;
	}

	Unlock();

	return Ret;
}

bool cPak::ExtractFileToMemory( const std::string& path, SafeDataPointer& data ) {
	if ( NULL == mPak.fs || !mPak.fs->IsOpen() ) {
		return false;
	}

	Lock();

	bool Ret = false;

	Int32 Pos = Exists( path );

	if ( Pos != -1 ) {
		data.DataSize	= mPakFiles[Pos].file_length;
		data.Data		= eeNewArray( Uint8, ( data.DataSize ) );

		mPak.fs->Seek( mPakFiles[Pos].file_position );
		mPak.fs->Read( reinterpret_cast<char*> ( data.Data ), mPakFiles[Pos].file_length );

		Ret = true;
	}

	Unlock();

	return Ret;
}

bool cPak::AddFile( const Uint8 * data, const Uint32& dataSize, const std::string& inpack ) {
	if ( dataSize < 1 )
		return false;

	Uint32 fsize = dataSize;

	if ( NULL != mPak.fs && mPak.fs->IsOpen() ) {
		if ( mPak.header.dir_length == 1 ) {
			mPak.header.dir_offset = sizeof(pakHeader) + fsize;
			mPak.header.dir_length = sizeof(pakEntry);
			mPak.pakFilesNum = 1;

			mPak.fs->Seek( 4 ); // seek after head (PACK)
			mPak.fs->Write( reinterpret_cast<const char*> (&mPak.header.dir_offset), sizeof( mPak.header.dir_offset ) );
			mPak.fs->Write( reinterpret_cast<const char*> (&mPak.header.dir_length), sizeof( mPak.header.dir_length ) );

			mPak.fs->Write( reinterpret_cast<const char*> (&data[0]), fsize );

			pakEntry newFile;
			String::StrCopy( newFile.filename, inpack.c_str(), 56 );
			newFile.file_position = sizeof(pakHeader);
			newFile.file_length = fsize;

			mPak.fs->Write( reinterpret_cast<const char*> (&newFile), sizeof( pakEntry ) );

			mPakFiles.push_back( newFile );

			return true;
		} else {
			if ( Exists( inpack ) != -1 ) // If the file already exists exit
				return false;

			if ( mPak.header.dir_length % 64 != 0 ) // Corrupted file?
				return false;

			std::vector<pakEntry> pakE;
			pakE.resize( mPak.pakFilesNum + 1 );	// Alloc space for all the pakEntrys and the new one

			mPak.fs->Seek( mPak.header.dir_offset ); 	// seek to the file pakEntrys
			mPak.fs->Read( reinterpret_cast<char*> (&pakE[0]), sizeof(pakEntry) * mPak.pakFilesNum ); 	// get all the pakEntrys

			mPak.header.dir_offset = mPak.header.dir_offset + fsize; 	// Update the new dir_offset
			mPak.header.dir_length = mPak.header.dir_length + sizeof(pakEntry); // Update the new dir_length

			mPak.fs->Seek( 4 ); // Update the new dir_offset and dir_length to the pakFile
			mPak.fs->Write( reinterpret_cast<const char*> (&mPak.header.dir_offset), sizeof( mPak.header.dir_offset ) );
			mPak.fs->Write( reinterpret_cast<const char*> (&mPak.header.dir_length), sizeof( mPak.header.dir_length ) );

			mPak.fs->Seek( mPak.header.dir_offset - fsize ); // Seek to the file allocation zone
			mPak.fs->Write( reinterpret_cast<const char*> (&data[0]), fsize ); // Alloc the file

			// Fill the new file data on the pakEntry
			String::StrCopy (pakE[ mPak.pakFilesNum ].filename, inpack.c_str(), 56 );

			pakE[ mPak.pakFilesNum ].file_position = mPak.header.dir_offset - fsize;
			pakE[ mPak.pakFilesNum ].file_length = fsize;

			// Update the new pakEntrys on pakFile
			mPak.fs->Write( reinterpret_cast<const char*>(&pakE[0]), (std::streamsize)( sizeof(pakEntry) * pakE.size() ) );

			mPakFiles.push_back( pakE[ mPak.pakFilesNum ] );
			mPak.pakFilesNum += 1;

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

	FileSystem::FileGet( path, file );

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

	nPf.pakPath = std::string ( mPak.pakPath + ".new" );

	nPf.fs = eeNew( cIOStreamFile, ( nPf.pakPath.c_str() , std::ios::out | std::ios::binary ) );

	for ( i = 0; i < mPakFiles.size(); i++ ) {
		Remove = false;

		for ( Uint32 u = 0; u < files.size(); u++ ) {
			if ( files[u] == static_cast<Int32>(i) )
				Remove = true;
		}

		if ( !Remove ) {
			uEntry.push_back( mPakFiles[i] );
			total_offset += mPakFiles[i].file_length;
		}
	}

	nPf.pakFilesNum = (Uint32)uEntry.size();
	nPf.header.head[0] = 'P'; nPf.header.head[1] = 'A'; nPf.header.head[2] = 'C'; nPf.header.head[3] = 'K';
	nPf.header.dir_offset = total_offset + sizeof(pakHeader);
	nPf.header.dir_length = (Uint32)uEntry.size() * sizeof( pakEntry );

	nPf.fs->Write( reinterpret_cast<const char*>(&nPf.header), sizeof(pakHeader) );

	std::vector<Uint8> data;
	for ( i = 0; i < uEntry.size(); i++ )
		if ( ExtractFileToMemory( std::string( uEntry[i].filename ), data ) ) {
			uEntry[i].file_position = nPf.fs->Tell();
			uEntry[i].file_length = (Uint32)data.size();
			nPf.fs->Write( reinterpret_cast<const char*>(&data[0]), (std::streamsize)data.size() );
		}

	nPf.fs->Write( reinterpret_cast<const char*>(&uEntry[0]), (std::streamsize)( sizeof(pakEntry) * uEntry.size() ) );

	eeSAFE_DELETE( nPf.fs );

	remove( mPak.pakPath.c_str() );
	rename( nPf.pakPath.c_str(), mPak.pakPath.c_str() );

	Close();

	Open( mPak.pakPath );

	return true;

}

std::vector<std::string> cPak::GetFileList() {
	std::vector<std::string> tmpv;

	tmpv.resize( mPakFiles.size() );

	for ( Uint32 i = 0; i < mPakFiles.size(); i++ )
		tmpv[i] = std::string ( mPakFiles[i].filename );

	return tmpv;
}

}}
