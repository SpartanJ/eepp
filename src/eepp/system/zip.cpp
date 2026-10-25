#include <eepp/system/zip.hpp>

#include <algorithm>
#include <cerrno>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamzip.hpp>
#include <eepp/system/lock.hpp>
#include <libzip/zip.h>
#include <limits>

namespace EE { namespace System {

Zip* Zip::New() {
	return eeNew( Zip, () );
}

Zip::Zip() : Zip( true ) {}

Zip::Zip( bool registerResource ) : Pack( registerResource ), mZip( nullptr ) {}

Zip::~Zip() {
	close();
}

bool Zip::create( const std::string& path ) {
	if ( !close() )
		return false;
	mError = Error::None;
	mErrorMessage.clear();
	mOpenMode = OpenMode::ReadWrite;
	int error = ZIP_ER_OK;
	mZip = zip_open( path.c_str(), ZIP_CREATE, &error );
	if ( !mZip ) {
		setError( error, errno );
		return false;
	}
	mZipPath = path;
	mIsOpen = true;
	onPackOpened();
	return true;
}

bool Zip::open( const std::string& path ) {
	return open( path, OpenMode::ReadWrite );
}

bool Zip::open( const std::string& path, OpenMode mode ) {
	if ( !close() )
		return false;
	mError = Error::None;
	mErrorMessage.clear();
	mOpenMode = mode;
	// libzip opens UTF-8 paths as wide paths on Windows.
	int error = ZIP_ER_OK;
	mZip = zip_open( path.c_str(), mode == OpenMode::ReadOnly ? ZIP_RDONLY : 0, &error );
	if ( !mZip ) {
		// zip_open() reports the system error or inconsistency detail through errno.
		setError( error, errno );
		return false;
	}
	mZipPath = path;
	mIsOpen = true;
	onPackOpened();
	return true;
}

bool Zip::close() {
	if ( mZip ) {
		// zip_close() writes the pending changes. When that fails the archive is left allocated
		// (and unchanged on disk): release it without writing and report the failure.
		bool written = zip_close( mZip ) == 0;

		if ( !written )
			zip_discard( mZip );

		mIsOpen = false;

		mZipPath = "";

		mZip = NULL;

		onPackClosed();

		return written;
	}

	return true;
}

bool Zip::addFile( const std::string& path, const std::string& inpack ) {
	ScopedBuffer file;

	if ( !writable() || !FileSystem::fileGet( path, file ) )
		return false;

	return addFile( file.get(), file.length(), inpack );
}

bool Zip::addFile( const Uint8* data, const Uint32& dataSize, const std::string& inpack ) {
	if ( !writable() )
		return false;
	if ( 0 == checkPack() ) {
		struct zip_source* zs = zip_source_buffer( mZip, (const void*)data, dataSize, 0 );

		if ( NULL != zs ) {
			if ( zip_file_add( mZip, inpack.c_str(), zs, ZIP_FL_ENC_GUESS ) < 0 ) {
				zip_source_free( zs );
				return false;
			}

			std::string path = mZipPath;
			bool written = close();
			bool reopened = open( path );

			if ( written && reopened )
				return true;
		}
	}

	return false;
}

bool Zip::addFile( std::vector<Uint8>& data, const std::string& inpack ) {
	return addFile( data.data(), (Uint32)data.size(), inpack );
}

bool Zip::addFiles( std::map<std::string, std::string> paths ) {
	if ( !writable() )
		return false;
	for ( std::map<std::string, std::string>::iterator itr = paths.begin(); itr != paths.end();
		  ++itr )
		if ( !addFile( itr->first, itr->second ) )
			return false;
	return true;
}

bool Zip::eraseFile( const std::string& path ) {
	if ( !writable() )
		return false;
	std::vector<std::string> tmpv;
	tmpv.push_back( path );

	return eraseFiles( tmpv );
}

bool Zip::eraseFiles( const std::vector<std::string>& paths ) {
	if ( !writable() )
		return false;
	Int32 Ex;
	Uint32 i = 0;

	for ( i = 0; i < paths.size(); i++ ) {
		Ex = exists( paths[i] );

		if ( Ex == -1 )
			return false;
		else {
			if ( zip_delete( mZip, Ex ) == -1 )
				return false;
		}
	}

	return true;
}

bool Zip::extractFile( const std::string& path, const std::string& dest ) {
	ScopedBuffer data;
	return extractFileToMemory( path, data ) &&
		   FileSystem::fileWrite( dest, data.get(), data.length() );
}

bool Zip::extractFileToMemory( const std::string& path, std::vector<Uint8>& data ) {
	Lock guard( *this );
	const auto index = exists( path );
	if ( index < 0 )
		return false;
	IOStreamZip stream( this, index );
	if ( !stream.isOpen() || static_cast<Uint64>( stream.getSize() ) > data.max_size() )
		return false;
	data.resize( static_cast<size_t>( stream.getSize() ) );
	return extractStream( stream, data.data(), data.size() );
}

bool Zip::extractFileToMemory( const std::string& path, ScopedBuffer& data ) {
	Lock guard( *this );
	const auto index = exists( path );
	if ( index < 0 )
		return false;
	IOStreamZip stream( this, index );
	if ( !stream.isOpen() ||
		 static_cast<Uint64>( stream.getSize() ) > std::numeric_limits<size_t>::max() )
		return false;
	data.reset( static_cast<size_t>( stream.getSize() ) );
	return extractStream( stream, data.get(), data.size() );
}

bool Zip::extractStream( IOStream& stream, Uint8* data, size_t size ) {
	size_t done = 0;
	while ( done < size ) {
		const auto count = stream.read( reinterpret_cast<char*>( data + done ),
										std::min<size_t>( size - done, 65536 ) );
		if ( count <= 0 ) {
			setError( ZIP_ER_READ );
			return false;
		}
		done += static_cast<size_t>( count );
	}
	char extra;
	if ( stream.read( &extra, 1 ) != 0 ) {
		setError( ZIP_ER_CRC );
		return false;
	}
	return true;
}

Int32 Zip::exists( const std::string& path ) {
	if ( isOpen() )
		return static_cast<Int32>( zip_name_locate( mZip, path.c_str(), 0 ) );

	return -1;
}

Int8 Zip::checkPack() {
	return NULL != mZip ? 0 : -1;
}

std::vector<std::string> Zip::getFileList() {
	std::vector<std::string> files;
	const auto count = entryCount();
	files.reserve( static_cast<size_t>( count ) );
	Entry info;
	for ( Uint64 i = 0; i < count; ++i ) {
		if ( entry( i, info ) && !info.name.ends_with( '/' ) )
			files.push_back( std::move( info.name ) );
	}
	return files;
}

/** @return The file path of the opened package */
std::string Zip::getPackPath() {
	return mZipPath;
}

IOStream* Zip::getFileStream( const std::string& path ) {
	return eeNew( IOStreamZip, ( this, path ) );
}

zip* Zip::getZip() {
	return mZip;
}

Uint64 Zip::entryCount() const {
	if ( !mZip )
		return 0;
	const auto count = zip_get_num_entries( mZip, 0 );
	return count < 0 ? 0 : static_cast<Uint64>( count );
}

bool Zip::entry( Uint64 index, Entry& result ) {
	struct zip_stat info;
	zip_stat_init( &info );
	if ( !mZip || zip_stat_index( mZip, index, 0, &info ) != 0 ) {
		setError( ZIP_ER_INCONS );
		return false;
	}
	result.name = info.name;
	result.size = info.size;
	result.modified = static_cast<Int64>( info.mtime );
	result.crc = info.crc;
	return true;
}

std::unique_ptr<IOStream> Zip::openStream( Uint64 index ) {
	struct zip_stat info;
	zip_stat_init( &info );
	if ( !mZip || zip_stat_index( mZip, index, 0, &info ) != 0 ) {
		setError( ZIP_ER_INCONS );
		return {};
	}
	if ( info.size > static_cast<Uint64>( std::numeric_limits<ios_size>::max() ) ) {
		setError( ZIP_ER_COMPNOTSUPP );
		return {};
	}
	auto stream = std::make_unique<IOStreamZip>( this, index );
	if ( !stream->isOpen() ) {
		const zip_error_t* error = zip_get_error( mZip );
		setError( zip_error_code_zip( error ), zip_error_code_system( error ) );
		return {};
	}
	return stream;
}

void Zip::setError( int code, int systemCode ) {
	zip_error_t error;
	zip_error_init( &error );
	zip_error_set( &error, code, systemCode );
	if ( code == ZIP_ER_NOENT )
		mError = Error::NotFound;
	else if ( zip_error_system_type( &error ) == ZIP_ET_SYS &&
			  ( systemCode == EACCES || systemCode == EPERM ) )
		mError = Error::PermissionDenied;
	else if ( code == ZIP_ER_NOZIP || code == ZIP_ER_INCONS || code == ZIP_ER_EOF ||
			  code == ZIP_ER_TRUNCATED_ZIP || code == ZIP_ER_COMPRESSED_DATA ||
			  code == ZIP_ER_DATA_LENGTH )
		mError = Error::InvalidArchive;
	else if ( code == ZIP_ER_COMPNOTSUPP || code == ZIP_ER_MULTIDISK ||
			  code == ZIP_ER_ENCRNOTSUPP || code == ZIP_ER_NOPASSWD || code == ZIP_ER_WRONGPASSWD ||
			  code == ZIP_ER_OPNOTSUPP )
		mError = Error::Unsupported;
	else
		mError = Error::IO;
	mErrorMessage = zip_error_strerror( &error );
	zip_error_fini( &error );
}

bool Zip::writable() {
	if ( mOpenMode != OpenMode::ReadWrite ) {
		mError = Error::ReadOnly;
		mErrorMessage = "ZIP archive is read-only";
		return false;
	}
	return mZip != nullptr;
}

}} // namespace EE::System
