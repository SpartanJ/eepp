#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreampak.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/pak.hpp>

namespace EE { namespace System {

Pak* Pak::New() {
	return eeNew( Pak, () );
}

Pak::Pak() : Pack() {
	mPak.fs = NULL;
}

Pak::~Pak() {
	close();
}

bool Pak::create( const std::string& path ) {
	if ( !FileSystem::fileExists( path ) ) {
		pakFile Pak;

		Pak.header.head[0] = 'P';
		Pak.header.head[1] = 'A';
		Pak.header.head[2] = 'C';
		Pak.header.head[3] = 'K';

		Pak.header.dir_offset = sizeof( Pak.header.head ) + 1;
		Pak.header.dir_length = 1;

		eeSAFE_DELETE( mPak.fs );

		mPak.fs = IOStreamFile::New( path, "wb" ); // Open the PAK file

		mPak.fs->write( reinterpret_cast<const char*>( &Pak.header ), sizeof( Pak.header ) );

		eeSAFE_DELETE( mPak.fs );

		open( path );

		return true;
	} else {
		return open( path );
	}

	return false;
}

bool Pak::open( const std::string& path ) {
	if ( FileSystem::fileExists( path ) ) {
		mPak.pakPath = path;

		eeSAFE_DELETE( mPak.fs );

		mPak.fs = IOStreamFile::New( path, "rwb" ); // Open the PAK file

		mPak.fs->read( reinterpret_cast<char*>( &mPak.header ),
					   sizeof( pakHeader ) ); // Read the PAK header

		if ( checkPack() == 0 ) {
			mPak.pakFilesNum = mPak.header.dir_length / 64; // Number of files in the PAK

			mPak.fs->seek( mPak.header.dir_offset ); // Seek to read the pakEntrys

			for ( Uint32 i = 0; i < mPak.pakFilesNum; i++ ) { // Read all the pakEntrys
				pakEntry Entry;

				mPak.fs->read( reinterpret_cast<char*>( &Entry ), sizeof( pakEntry ) );

				mPakFiles.push_back( Entry );
			}

			mIsOpen = true;

			onPackOpened();

			return true;
		}
	}

	return false;
}

bool Pak::close() {
	if ( mIsOpen ) {
		eeSAFE_DELETE( mPak.fs );

		mPakFiles.clear();

		mIsOpen = false;

		onPackClosed();

		return true;
	}

	return false;
}

Int8 Pak::checkPack() {
	if ( NULL != mPak.fs && mPak.fs->isOpen() ) {
		if ( mPak.header.head[0] != 'P' || mPak.header.head[1] != 'A' ||
			 mPak.header.head[2] != 'C' || mPak.header.head[3] != 'K' )
			return -1; // Ident corrupt

		if ( mPak.header.dir_offset < ( sizeof( mPak.header.head ) + 1 ) ||
			 mPak.header.dir_length < 1 )
			return -2; // Header corrupt
	}

	return 0;
}

Int32 Pak::exists( const std::string& path ) {
	if ( isOpen() ) {
		for ( Uint32 i = 0; i < mPakFiles.size(); i++ )
			if ( std::strncmp( path.c_str(), mPakFiles[i].filename, path.size() ) == 0 )
				return i;
	}

	return -1;
}

bool Pak::extractFile( const std::string& path, const std::string& dest ) {
	if ( NULL == mPak.fs || !mPak.fs->isOpen() ) {
		return false;
	}

	lock();

	bool Ret = false;

	Int32 Pos = exists( path );

	if ( Pos != -1 ) {
		ScopedBuffer data;

		if ( extractFileToMemory( path, data ) ) {
			FileSystem::fileWrite( path, data.get(), data.length() );
		}

		Ret = true;
	}

	unlock();

	return Ret;
}

bool Pak::extractFileToMemory( const std::string& path, std::vector<Uint8>& data ) {
	if ( NULL == mPak.fs || !mPak.fs->isOpen() ) {
		return false;
	}

	lock();

	bool Ret = false;

	Int32 Pos = exists( path );

	if ( Pos != -1 ) {
		data.clear();
		data.resize( mPakFiles[Pos].file_length );

		mPak.fs->seek( mPakFiles[Pos].file_position );
		mPak.fs->read( reinterpret_cast<char*>( &data[0] ), mPakFiles[Pos].file_length );

		Ret = true;
	}

	unlock();

	return Ret;
}

bool Pak::extractFileToMemory( const std::string& path, ScopedBuffer& data ) {
	if ( NULL == mPak.fs || !mPak.fs->isOpen() ) {
		return false;
	}

	lock();

	bool Ret = false;

	Int32 Pos = exists( path );

	if ( Pos != -1 ) {
		data.reset( mPakFiles[Pos].file_length );

		mPak.fs->seek( mPakFiles[Pos].file_position );
		mPak.fs->read( reinterpret_cast<char*>( data.get() ), data.length() );

		Ret = true;
	}

	unlock();

	return Ret;
}

bool Pak::addFile( const Uint8* data, const Uint32& dataSize, const std::string& inpack ) {
	if ( dataSize < 1 )
		return false;

	Uint32 fsize = dataSize;

	if ( NULL != mPak.fs && mPak.fs->isOpen() ) {
		if ( mPak.header.dir_length == 1 ) {
			mPak.header.dir_offset = sizeof( pakHeader ) + fsize;
			mPak.header.dir_length = sizeof( pakEntry );
			mPak.pakFilesNum = 1;

			mPak.fs->seek( 4 ); // seek after head (PACK)
			mPak.fs->write( reinterpret_cast<const char*>( &mPak.header.dir_offset ),
							sizeof( mPak.header.dir_offset ) );
			mPak.fs->write( reinterpret_cast<const char*>( &mPak.header.dir_length ),
							sizeof( mPak.header.dir_length ) );

			mPak.fs->write( reinterpret_cast<const char*>( &data[0] ), fsize );

			pakEntry newFile;
			String::strCopy( newFile.filename, inpack.c_str(), 56 );
			newFile.file_position = sizeof( pakHeader );
			newFile.file_length = fsize;

			mPak.fs->write( reinterpret_cast<const char*>( &newFile ), sizeof( pakEntry ) );

			mPakFiles.push_back( newFile );

			return true;
		} else {
			if ( exists( inpack ) != -1 ) // If the file already exists exit
				return false;

			if ( mPak.header.dir_length % 64 != 0 ) // Corrupted file?
				return false;

			std::vector<pakEntry> pakE;
			pakE.resize( mPak.pakFilesNum +
						 1 ); // Alloc space for all the pakEntrys and the new one

			mPak.fs->seek( mPak.header.dir_offset ); // seek to the file pakEntrys
			mPak.fs->read( reinterpret_cast<char*>( &pakE[0] ),
						   sizeof( pakEntry ) * mPak.pakFilesNum ); // get all the pakEntrys

			mPak.header.dir_offset = mPak.header.dir_offset + fsize; // Update the new dir_offset
			mPak.header.dir_length =
				mPak.header.dir_length + sizeof( pakEntry ); // Update the new dir_length

			mPak.fs->seek( 4 ); // Update the new dir_offset and dir_length to the pakFile
			mPak.fs->write( reinterpret_cast<const char*>( &mPak.header.dir_offset ),
							sizeof( mPak.header.dir_offset ) );
			mPak.fs->write( reinterpret_cast<const char*>( &mPak.header.dir_length ),
							sizeof( mPak.header.dir_length ) );

			mPak.fs->seek( mPak.header.dir_offset - fsize ); // Seek to the file allocation zone
			mPak.fs->write( reinterpret_cast<const char*>( &data[0] ), fsize ); // Alloc the file

			// Fill the new file data on the pakEntry
			String::strCopy( pakE[mPak.pakFilesNum].filename, inpack.c_str(), 56 );

			pakE[mPak.pakFilesNum].file_position = mPak.header.dir_offset - fsize;
			pakE[mPak.pakFilesNum].file_length = fsize;

			// Update the new pakEntrys on pakFile
			mPak.fs->write( reinterpret_cast<const char*>( &pakE[0] ),
							(ios_size)( sizeof( pakEntry ) * pakE.size() ) );

			mPakFiles.push_back( pakE[mPak.pakFilesNum] );
			mPak.pakFilesNum += 1;

			pakE.clear();

			return true;
		}
	}

	return false;
}

bool Pak::addFile( std::vector<Uint8>& data, const std::string& inpack ) {
	return addFile( reinterpret_cast<const Uint8*>( &data[0] ), (Uint32)data.size(), inpack );
}

bool Pak::addFile( const std::string& path, const std::string& inpack ) {
	if ( path.size() > 56 )
		return false;

	ScopedBuffer file;

	FileSystem::fileGet( path, file );

	return addFile( file.get(), file.length(), inpack );
}

bool Pak::addFiles( std::map<std::string, std::string> paths ) {
	for ( std::map<std::string, std::string>::iterator itr = paths.begin(); itr != paths.end();
		  ++itr )
		if ( !addFile( itr->first, itr->second ) )
			return false;
	return true;
}

bool Pak::eraseFile( const std::string& path ) {
	std::vector<std::string> tmpv;
	tmpv.push_back( path );

	return eraseFiles( tmpv );
}

bool Pak::eraseFiles( const std::vector<std::string>& paths ) {
	std::vector<Int32> files;
	Int32 Ex;
	Uint32 total_offset = 0, i = 0;
	pakFile nPf;
	std::vector<pakEntry> uEntry;

	for ( i = 0; i < paths.size(); i++ ) {
		Ex = exists( paths[i] );
		if ( Ex == -1 )
			return false;
		else
			files.push_back( Ex );
	}

	nPf.pakPath = std::string( mPak.pakPath + ".new" );

	nPf.fs = IOStreamFile::New( nPf.pakPath.c_str(), "wb" );

	for ( i = 0; i < mPakFiles.size(); i++ ) {
		bool Remove = false;

		for ( Uint32 u = 0; u < files.size(); u++ ) {
			if ( files[u] == static_cast<Int32>( i ) )
				Remove = true;
		}

		if ( !Remove ) {
			uEntry.push_back( mPakFiles[i] );
			total_offset += mPakFiles[i].file_length;
		}
	}

	nPf.pakFilesNum = (Uint32)uEntry.size();
	nPf.header.head[0] = 'P';
	nPf.header.head[1] = 'A';
	nPf.header.head[2] = 'C';
	nPf.header.head[3] = 'K';
	nPf.header.dir_offset = total_offset + sizeof( pakHeader );
	nPf.header.dir_length = (Uint32)uEntry.size() * sizeof( pakEntry );

	nPf.fs->write( reinterpret_cast<const char*>( &nPf.header ), sizeof( pakHeader ) );

	std::vector<Uint8> data;
	for ( i = 0; i < uEntry.size(); i++ )
		if ( extractFileToMemory( std::string( uEntry[i].filename ), data ) ) {
			uEntry[i].file_position = nPf.fs->tell();
			uEntry[i].file_length = (Uint32)data.size();
			nPf.fs->write( reinterpret_cast<const char*>( &data[0] ), (ios_size)data.size() );
		}

	nPf.fs->write( reinterpret_cast<const char*>( &uEntry[0] ),
				   (ios_size)( sizeof( pakEntry ) * uEntry.size() ) );

	eeSAFE_DELETE( nPf.fs );

	remove( mPak.pakPath.c_str() );
	rename( nPf.pakPath.c_str(), mPak.pakPath.c_str() );

	close();

	open( mPak.pakPath );

	return true;
}

std::vector<std::string> Pak::getFileList() {
	std::vector<std::string> tmpv;

	tmpv.resize( mPakFiles.size() );

	for ( Uint32 i = 0; i < mPakFiles.size(); i++ )
		tmpv[i] = std::string( mPakFiles[i].filename );

	return tmpv;
}

bool Pak::isOpen() const {
	return mIsOpen;
}

std::string Pak::getPackPath() {
	return mPak.pakPath;
}

IOStream* Pak::getFileStream( const std::string& path ) {
	return eeNew( IOStreamPak, ( this, path ) );
}

Pak::pakEntry Pak::getPackEntry( Uint32 index ) {
	if ( isOpen() && index < mPakFiles.size() ) {
		return mPakFiles[index];
	}

	return pakEntry();
}

}} // namespace EE::System
