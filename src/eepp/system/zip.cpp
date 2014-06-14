#include <eepp/system/zip.hpp>
#include <eepp/helper/libzip/zip.h>
#include <eepp/helper/libzip/zipint.h>
#include <eepp/system/filesystem.hpp>

namespace EE { namespace System {

Zip::Zip() :
	mZip(NULL)
{
}

Zip::~Zip() {
	Close();
}

bool Zip::Create( const std::string& path ) {
	if ( !FileSystem::FileExists(path) ) {
		int err;
		mZip = zip_open( path.c_str(), ZIP_CREATE, &err );

		if ( 0 == CheckPack() ) {
			mZipPath = path;

			mIsOpen = true;

			return true;
		}
	} else {
		return Open( path );
	}

	return false;
}

bool Zip::Open( const std::string& path ) {
	if ( FileSystem::FileExists( path ) ) {
		int err;
		mZip = zip_open( path.c_str(), 0, &err );

		if ( 0 == CheckPack() ) {
			mZipPath = path;

			mIsOpen = true;

			return true;
		}
	} else {
		return Create( path );
	}

	return false;
}

bool Zip::Close() {
	if ( 0 == CheckPack() ) {
		zip_close( mZip );

		mIsOpen = false;

		mZipPath = "";

		mZip = NULL;

		return true;
	}

	return false;
}

bool Zip::AddFile( const std::string& path, const std::string& inpack ) {
	SafeDataPointer file;

	FileSystem::FileGet( path, file );

	return AddFile( file.Data, file.DataSize, inpack );
}

bool Zip::AddFile( const Uint8 * data, const Uint32& dataSize, const std::string& inpack ) {
	if ( 0 == CheckPack() ) {
		struct zip_source * zs = zip_source_buffer( mZip, (const void*)data, dataSize, 0 );

		if ( NULL != zs ) {
			Int32 Result = (Int32)zip_add( mZip, inpack.c_str(), zs );

			std::string path = mZipPath;
			Close();
			Open( path );

			if ( -1 != Result )
				return true;
		} else
			zip_source_free( zs );
	}

	return false;
}

bool Zip::AddFile( std::vector<Uint8>& data, const std::string& inpack ) {
	return AddFile( reinterpret_cast<const Uint8*> ( &data[0] ), (Uint32)data.size(), inpack );
}

bool Zip::AddFiles( std::map<std::string, std::string> paths ) {
	for( std::map<std::string, std::string>::iterator itr = paths.begin(); itr != paths.end(); itr++)
		if ( !AddFile( itr->first, itr->second ) )
			return false;
    return true;
}

bool Zip::EraseFile( const std::string& path ) {
	std::vector<std::string> tmpv;
	tmpv.push_back( path );

	return EraseFiles( tmpv );
}

bool Zip::EraseFiles( const std::vector<std::string>& paths ) {
	Int32 Ex;
	Uint32 i = 0;

	for ( i = 0; i < paths.size(); i++ ) {
		Ex = Exists( paths[i] );

		if ( Ex  == -1 )
			return false;
		else {
			if ( zip_delete( mZip, Ex ) == -1 )
				return false;
		}
	}

	return false;
}

bool Zip::ExtractFile( const std::string& path , const std::string& dest ) {
	Lock();

	bool Ret = false;

	SafeDataPointer data;

	Ret = ExtractFileToMemory( path, data );

	if ( Ret )
		FileSystem::FileWrite( dest, data.Data, data.DataSize );

	Unlock();

	return  Ret;
}

bool Zip::ExtractFileToMemory( const std::string& path, std::vector<Uint8>& data ) {
	Lock();

	bool Ret = false;
	Int32 Pos = Exists( path );
	int Result = 0;

	if ( 0 == CheckPack() && -1 != Pos ) {
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

	Unlock();

	return Ret;
}

bool Zip::ExtractFileToMemory( const std::string& path, SafeDataPointer& data ) {
	Lock();

	bool Ret = false;
	Int32 Pos = Exists( path );
	Int32 Result = 0;


	if ( 0 == CheckPack() && -1 != Pos ) {
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

	Unlock();

	return Ret;
}

Int32 Zip::Exists( const std::string& path ) {
	if ( IsOpen() )
		return zip_name_locate( mZip, path.c_str(), 0 );

	return -1;
}

Int8 Zip::CheckPack() {
	return NULL != mZip ? 0 : -1;
}

std::vector<std::string> Zip::GetFileList() {
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
std::string Zip::GetPackPath() {
	return mZipPath;
}

}}
