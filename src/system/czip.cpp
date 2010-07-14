#include "czip.hpp"

namespace EE { namespace System {

cZip::cZip() :
	mZip(NULL)
{
}

cZip::~cZip() {
	Close();
}

bool cZip::Create( const std::string& path ) {
	if ( !FileExists(path) ) {
		mZip = CreateZip( path.c_str(), 0 );

		if ( 0 == CheckPack() ) {
			mZipPath = path;

			mState = ZIP_CREATED;

			mIsOpen = true;

			return true;
		}
	} else {
		return Open( path );
	}

	return false;
}

bool cZip::Open( const std::string& path ) {
	if ( FileExists(path) ) {
		mZip = OpenZip( path.c_str(), 0 );

		if ( 0 == CheckPack() ) {
			ZIPENTRY ze;

			GetZipItem( mZip, -1 , &ze );

			Uint32 numitems = ze.index;

			for ( Uint32 zi = 0; zi < numitems; zi++ ) {
				GetZipItem( mZip, zi, &ze );

				ZIPENTRY zet = ze;

				zipFiles.push_back( zet );
			}

			mZipPath = path;

			mState = ZIP_OPEN;

			mIsOpen = true;

			return true;
		}
	}

	return false;
}

bool cZip::Close() {
	if ( 0 == CheckPack() ) {
		CloseZip( mZip );

		zipFiles.clear();

		mIsOpen = false;

		mZipPath = "";

		return true;
	}

	return false;
}

bool cZip::AddFile( const std::string& path, const std::string& inpack ) {
	std::vector<Uint8> file;

	FileGet( path, file );

	return AddFile( file, inpack );
}

bool cZip::AddFile( std::vector<Uint8>& data, const std::string& inpack ) {
	if ( 0 == CheckPack() ) {
		if ( ZIP_CREATED == mState ) {
			Int32 Result = ZipAdd( mZip , inpack.c_str(), reinterpret_cast<void*> (&data[0]), (unsigned int)data.size() );

			if ( ZR_OK == Result )
				return true;
		} else {
			std::string ZipNewPath = mZipPath + ".temp";

			cZip Zip;

			if ( Zip.Create( ZipNewPath ) ) {
				for ( eeUint i = 0; i < zipFiles.size(); i++ ) {
					std::vector<Uint8> tdata;

					ExtractFileToMemory( zipFiles[i].name, tdata );
					Zip.AddFile( tdata, zipFiles[i].name );
				}

				Zip.AddFile( data, inpack );

				Zip.Close();

				std::string ZPath = mZipPath;

				Close();

				remove( ZPath.c_str() );
				rename( ZipNewPath.c_str(), ZPath.c_str() );

				Open( ZPath );

				return true;
			}

			return false;
		}
	}

	return false;
}

bool cZip::AddFiles( std::map<std::string, std::string> paths ) {
	for( std::map<std::string, std::string>::iterator itr = paths.begin(); itr != paths.end(); itr++)
		if ( !AddFile( itr->first, itr->second ) )
			return false;
    return true;
}

bool cZip::EraseFile( const std::string& path ) {
	std::vector<std::string> tmpv;
	tmpv.push_back( path );

	return EraseFiles( tmpv );
}

bool cZip::EraseFiles( const std::vector<std::string>& paths ) {
	std::vector<Int32> files;
	Int32 Ex;
	Uint32 i = 0;
	std::vector<ZIPENTRY> uEntry;
	bool Remove;

	ChangeState( ZIP_OPEN );

	for ( i = 0; i < paths.size(); i++ ) {
		Ex = Exists( paths[i] );

		if ( Ex  == -1 )
			return false;
		else
			files.push_back( Ex );
	}

	for ( i = 0; i < zipFiles.size(); i++ ) {
		Remove = false;

		for ( Uint32 u = 0; u < files.size(); u++ ) {
			if ( files[u] == static_cast<Int32>(i) )
				Remove = true;
		}

		if ( !Remove ) {
			uEntry.push_back( zipFiles[i] );
		}
	}

	std::string ZipNewPath = mZipPath + ".temp";

	cZip Zip;

	if ( Zip.Create( ZipNewPath ) ) {
		for ( i = 0; i < uEntry.size(); i++ ) {
			std::vector<Uint8> data;

			ExtractFileToMemory( uEntry[i].name, data );
			Zip.AddFile( data, uEntry[i].name );
		}

		Zip.Close();

		std::string ZPath = mZipPath;

		Close();

		remove( ZPath.c_str() );
		rename( ZipNewPath.c_str(), ZPath.c_str() );

		Open( ZPath );

		return true;
	}

	return false;
}

bool cZip::ExtractFile( const std::string& path , const std::string& dest ) {
	Lock();

	bool Ret = false;

	ChangeState( ZIP_OPEN );

	Int32 index = Exists( path );

	if ( 0 == CheckPack() && -1 != index ) {
		if ( ZR_OK == UnzipItem( mZip, index, dest.c_str() ) )
			 Ret = true;
	}

	Unlock();

	return  Ret;
}

bool cZip::ExtractFileToMemory( const std::string& path, std::vector<Uint8>& data ) {
	Lock();

	bool Ret = false;
	Int32 Pos = Exists( path );
	Uint32 Result = 0;

	if ( 0 == CheckPack() && -1 != Pos ) {
		data.clear();
		data.resize( zipFiles[Pos].unc_size );

		Result = UnzipItem( mZip, Pos, reinterpret_cast<void*> (&data[0]), (unsigned int)zipFiles[Pos].unc_size );

		if ( ZR_OK == Result ) {
			Ret = true;
		}
	}

	Unlock();

	return Ret;
}

Int32 cZip::Exists( const std::string& path ) {
	for ( Uint32 i = 0; i < zipFiles.size(); i++ )
		if ( strcmp( path.c_str(), zipFiles[i].name ) == 0 )
			return i;

	return -1;
}

Int8 cZip::CheckPack() {
	return NULL != mZip ? 0 : -1;
}

std::vector<std::string> cZip::GetFileList() {
	std::vector<std::string> tmpv;

	tmpv.resize( zipFiles.size() );

	for ( Uint32 i = 0; i < zipFiles.size(); i++ )
		tmpv[i] = std::string ( zipFiles[i].name );

	return tmpv;
}

void cZip::ChangeState( const Uint32& State ) {
	if ( State == ZIP_OPEN ) {
		if ( ZIP_CREATED == mState ) {
			std::string path = mZipPath;
			Close();
			Open( path );
		}
	}
}

}}
