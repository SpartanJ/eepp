#include <eepp/system/zip.hpp>

#include <algorithm>
#include <cerrno>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamzip.hpp>
#include <eepp/system/lock.hpp>
#include <libzip/zip.h>
#include <limits>

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <io.h>
#else
#include <unistd.h>
#endif

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
	if ( mode == OpenMode::ReadOnly ) {
		if ( !openReadOnly( path ) )
			return false;
	} else {
		int error = ZIP_ER_OK;
		mZip = zip_open( path.c_str(), 0, &error );
		if ( !mZip ) {
			setError( error, errno );
			return false;
		}
	}
	mZipPath = path;
	mIsOpen = true;
	onPackOpened();
	return true;
}

bool Zip::close() {
	if ( mZip ) {
		// zip_close() writes the pending changes. When that fails the archive is left allocated
		// (and unchanged on disk): release it and report the failure.
		bool written = zip_close( mZip ) == 0;

		if ( !written ) {
			// Without pending changes zip_close() only frees the archive.
			zip_unchange_all( mZip );
			zip_close( mZip );
		}

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
			Int32 Result = (Int32)zip_add( mZip, inpack.c_str(), zs );
			if ( Result < 0 ) {
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
		return zip_name_locate( mZip, path.c_str(), 0 );

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

bool Zip::openReadOnly( const std::string& path ) {
	// fopenUtf8 preserves Windows Unicode paths. zip_fdopen consumes a duplicated descriptor,
	// so the FILE owned here can always be closed normally.
	FILE* file = FileSystem::fopenUtf8( path, "rb" );
	if ( !file ) {
		setError( errno == ENOENT ? ZIP_ER_NOENT : ZIP_ER_OPEN, errno );
		return false;
	}
#if EE_PLATFORM == EE_PLATFORM_WIN
	int descriptor = _dup( _fileno( file ) );
#else
	int descriptor = dup( fileno( file ) );
#endif
	const int duplicateError = errno;
	std::fclose( file );
	if ( descriptor < 0 ) {
		setError( ZIP_ER_OPEN, duplicateError );
		return false;
	}
	int error = ZIP_ER_OK;
	mZip = zip_fdopen( descriptor, 0, &error );
	if ( !mZip ) {
		// This vendored libzip consumes descriptor after fdopen succeeds. Only its two early
		// ZIP_ER_OPEN failures (dup/fdopen) leave it for the caller to close.
		const int systemError = errno;
		if ( error == ZIP_ER_OPEN ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
			_close( descriptor );
#else
			::close( descriptor );
#endif
		}
		setError( error, systemError );
		return false;
	}
	return true;
}

Uint64 Zip::entryCount() const {
	if ( !mZip )
		return 0;
	const auto count = zip_get_num_entries( mZip, 0 );
	return count == std::numeric_limits<Uint64>::max() ? 0 : count;
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
		int code = 0;
		int systemCode = 0;
		zip_error_get( mZip, &code, &systemCode );
		setError( code, systemCode );
		return {};
	}
	return stream;
}

void Zip::setError( int code, int systemCode ) {
	if ( code == ZIP_ER_NOENT )
		mError = Error::NotFound;
	else if ( zip_error_get_sys_type( code ) == ZIP_ET_SYS &&
			  ( systemCode == EACCES || systemCode == EPERM ) )
		mError = Error::PermissionDenied;
	else if ( code == ZIP_ER_NOZIP || code == ZIP_ER_INCONS || code == ZIP_ER_EOF )
		mError = Error::InvalidArchive;
	else if ( code == ZIP_ER_COMPNOTSUPP || code == ZIP_ER_MULTIDISK ||
			  code == ZIP_ER_ENCRNOTSUPP || code == ZIP_ER_NOPASSWD || code == ZIP_ER_WRONGPASSWD )
		mError = Error::Unsupported;
	else
		mError = Error::IO;
	char message[256];
	zip_error_to_str( message, sizeof( message ), code, systemCode );
	mErrorMessage = message;
}

bool Zip::writable() {
	if ( mOpenMode == OpenMode::ReadOnly ) {
		mError = Error::ReadOnly;
		mErrorMessage = "ZIP archive is read-only";
		return false;
	}
	return mZip != nullptr;
}

}} // namespace EE::System
