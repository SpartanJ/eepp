#include <eepp/system/zip.hpp>
#include <libzip/zip.h>
#include <libzip/zipint.h>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamzip.hpp>

namespace EE { namespace System {

Zip::Zip() :
	mZip(NULL)
{
}

Zip::~Zip() {
	close();
}

bool Zip::create( const std::string& path ) {
	if ( !FileSystem::fileExists(path) ) {
		int err;
		mZip = zip_open( path.c_str(), ZIP_CREATE, &err );

		if ( 0 == checkPack() ) {
			mZipPath = path;

			mIsOpen = true;

			return true;
		}
	} else {
		return open( path );
	}

	return false;
}

bool Zip::open( const std::string& path ) {
	if ( FileSystem::fileExists( path ) ) {
		int err;
		mZip = zip_open( path.c_str(), 0, &err );

		if ( 0 == checkPack() ) {
			mZipPath = path;

			mIsOpen = true;

			return true;
		}
	} else {
		return create( path );
	}

	return false;
}

bool Zip::close() {
	if ( 0 == checkPack() ) {
		zip_close( mZip );

		mIsOpen = false;

		mZipPath = "";

		mZip = NULL;

		return true;
	}

	return false;
}

bool Zip::addFile( const std::string& path, const std::string& inpack ) {
	SafeDataPointer file;

	FileSystem::fileGet( path, file );

	return addFile( file.Data, file.DataSize, inpack );
}

bool Zip::addFile( const Uint8 * data, const Uint32& dataSize, const std::string& inpack ) {
	if ( 0 == checkPack() ) {
		struct zip_source * zs = zip_source_buffer( mZip, (const void*)data, dataSize, 0 );

		if ( NULL != zs ) {
			Int32 Result = (Int32)zip_add( mZip, inpack.c_str(), zs );

			std::string path = mZipPath;
			close();
			open( path );

			if ( -1 != Result )
				return true;
		} else
			zip_source_free( zs );
	}

	return false;
}

bool Zip::addFile( std::vector<Uint8>& data, const std::string& inpack ) {
	return addFile( reinterpret_cast<const Uint8*> ( &data[0] ), (Uint32)data.size(), inpack );
}

bool Zip::addFiles( std::map<std::string, std::string> paths ) {
	for( std::map<std::string, std::string>::iterator itr = paths.begin(); itr != paths.end(); itr++)
		if ( !addFile( itr->first, itr->second ) )
			return false;
	return true;
}

bool Zip::eraseFile( const std::string& path ) {
	std::vector<std::string> tmpv;
	tmpv.push_back( path );

	return eraseFiles( tmpv );
}

bool Zip::eraseFiles( const std::vector<std::string>& paths ) {
	Int32 Ex;
	Uint32 i = 0;

	for ( i = 0; i < paths.size(); i++ ) {
		Ex = exists( paths[i] );

		if ( Ex  == -1 )
			return false;
		else {
			if ( zip_delete( mZip, Ex ) == -1 )
				return false;
		}
	}

	return false;
}

bool Zip::extractFile( const std::string& path , const std::string& dest ) {
	lock();

	bool Ret = false;

	SafeDataPointer data;

	Ret = extractFileToMemory( path, data );

	if ( Ret )
		FileSystem::fileWrite( dest, data.Data, data.DataSize );

	unlock();

	return  Ret;
}

bool Zip::extractFileToMemory( const std::string& path, std::vector<Uint8>& data ) {
	lock();

	bool Ret = false;
	Int32 Pos = exists( path );
	int Result = 0;

	if ( 0 == checkPack() && -1 != Pos ) {
		data.clear();

		struct zip_stat zs;
		int err = zip_stat( mZip, path.c_str(), 0, &zs );

		if ( !err ) {
			struct zip_file * zf = zip_fopen_index( mZip, zs.index, 0 );

			if ( NULL != zf ) {
				data.resize( zs.size );

				Result = (Int32)zip_fread( zf, reinterpret_cast<void*> (&data[0]), data.size() );

				zip_fclose(zf);

				if ( -1 != Result )
					Ret = true;
			}
		}
	}

	unlock();

	return Ret;
}

bool Zip::extractFileToMemory( const std::string& path, SafeDataPointer& data ) {
	lock();

	bool Ret = false;
	Int32 Pos = exists( path );
	Int32 Result = 0;


	if ( 0 == checkPack() && -1 != Pos ) {
		struct zip_stat zs;
		int err = zip_stat( mZip, path.c_str(), 0, &zs );

		if ( !err ) {
			struct zip_file * zf = zip_fopen_index( mZip, zs.index, 0 );

			if ( NULL != zf ) {
				data.DataSize	= (Uint32)zs.size;
				data.Data		= eeNewArray( Uint8, ( data.DataSize ) );

				Result = (Int32)zip_fread( zf, (void*)data.Data, data.DataSize );

				zip_fclose(zf);

				if ( -1 != Result )
					Ret = true;
			}
		}
	}

	unlock();

	return Ret;
}

Int32 Zip::exists( const std::string& path ) {
	if ( isOpen() )
		return zip_name_locate( mZip, path.c_str(), 0 );

	return -1;
}

Int8 Zip::checkPack() {
	return NULL != mZip ? 0 : -1;
}

std::vector<std::string> Zip::getFileList() {
	std::vector<std::string> tmpv;

	Int32 numfiles = zip_get_num_files( mZip );

	tmpv.resize( numfiles );

	for ( Int32 i = 0; i < numfiles; i++ ) {
		struct zip_stat zs;

		if ( -1 != zip_stat_index( mZip, i, 0, &zs ) )
			tmpv[i] = std::string ( zs.name );
	}

	return tmpv;
}

/** @return The file path of the opened package */
std::string Zip::getPackPath() {
	return mZipPath;
}

IOStream * Zip::getFileStream( const std::string & path ) {
	return eeNew( IOStreamZip, ( this, path ) );
}

zip * Zip::getZip()
{
	return mZip;
}

}}
