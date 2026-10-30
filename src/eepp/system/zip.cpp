#include <eepp/system/zip.hpp>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <eepp/system/fileinfo.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamzip.hpp>
#include <eepp/system/lock.hpp>
#include <libzip/zip.h>
#include <limits>

namespace EE { namespace System {

namespace {

/** One deferred entry, or a borrowed fresh output stream. Only the current input is open. */
class ZipStreamSource {
  public:
	Zip::StreamFactory open;
	std::unique_ptr<IOStream> input;
	IOStream* output{ nullptr };
	Uint64 size{ 0 };
	zip_error_t error;

	ZipStreamSource() { zip_error_init( &error ); }

	~ZipStreamSource() { zip_error_fini( &error ); }

	static zip_int64_t callback( void* context, void* data, zip_uint64_t length,
								 zip_source_cmd_t command ) {
		auto& self = *static_cast<ZipStreamSource*>( context );
		auto fail = [&]( int code ) -> zip_int64_t {
			zip_error_set( &self.error, code, 0 );
			return -1;
		};
		IOStream* stream = self.output ? self.output : self.input.get();
		switch ( command ) {
			case ZIP_SOURCE_SUPPORTS:
				return self.output
						   ? ZIP_SOURCE_SUPPORTS_WRITABLE
						   : ZIP_SOURCE_SUPPORTS_SEEKABLE |
								 ZIP_SOURCE_MAKE_COMMAND_BITMASK( ZIP_SOURCE_SUPPORTS_REOPEN );
			case ZIP_SOURCE_STAT: {
				if ( length < sizeof( zip_stat_t ) )
					return fail( ZIP_ER_INVAL );
				auto* stat = static_cast<zip_stat_t*>( data );
				zip_stat_init( stat );
				stat->size = self.size;
				stat->valid = ZIP_STAT_SIZE;
				return sizeof( *stat );
			}
			case ZIP_SOURCE_OPEN:
				if ( self.output )
					return self.output->seek( 0 ) == 0 ? 0 : fail( ZIP_ER_SEEK );
				self.input = self.open();
				if ( !self.input || !self.input->isOpen() )
					return fail( ZIP_ER_OPEN );
				if ( self.input->getSize() < 0 ||
					 static_cast<Uint64>( self.input->getSize() ) != self.size )
					return fail( ZIP_ER_DATA_LENGTH );
				return 0;
			case ZIP_SOURCE_READ: {
				if ( !stream )
					return fail( ZIP_ER_READ );
				auto count = stream->read(
					static_cast<char*>( data ),
					std::min<Uint64>( length, std::numeric_limits<ios_size>::max() ) );
				if ( count < 0 || !stream->isOpen() )
					return fail( ZIP_ER_READ );
				return count;
			}
			case ZIP_SOURCE_CLOSE:
				self.input.reset();
				return 0;
			case ZIP_SOURCE_SEEK:
			case ZIP_SOURCE_SEEK_WRITE: {
				if ( !stream || stream->tell() < 0 )
					return fail( ZIP_ER_SEEK );
				const auto size = self.size;
				auto position = zip_source_seek_compute_offset( stream->tell(), size, data, length,
																&self.error );
				return position >= 0 && stream->seek( position ) == position ? 0
																			 : fail( ZIP_ER_SEEK );
			}
			case ZIP_SOURCE_TELL:
			case ZIP_SOURCE_TELL_WRITE:
				return stream ? stream->tell() : fail( ZIP_ER_SEEK );
			case ZIP_SOURCE_BEGIN_WRITE:
				return self.output && self.size == 0 && self.output->seek( 0 ) == 0
						   ? 0
						   : fail( ZIP_ER_WRITE );
			case ZIP_SOURCE_WRITE: {
				if ( !self.output ||
					 length > static_cast<Uint64>( std::numeric_limits<ios_size>::max() ) )
					return fail( ZIP_ER_WRITE );
				auto count = self.output->write( static_cast<const char*>( data ), length );
				if ( count != static_cast<ios_size>( length ) || self.output->tell() < 0 )
					return fail( ZIP_ER_WRITE );
				self.size = std::max<Uint64>( self.size, self.output->tell() );
				return count;
			}
			case ZIP_SOURCE_COMMIT_WRITE:
				return self.output && self.output->isOpen() ? 0 : fail( ZIP_ER_WRITE );
			case ZIP_SOURCE_ROLLBACK_WRITE:
			case ZIP_SOURCE_REMOVE:
				// The output belongs to the caller's transaction, which handles rollback.
				return 0;
			case ZIP_SOURCE_ERROR:
				return zip_error_to_data( &self.error, data, length );
			case ZIP_SOURCE_FREE:
				delete &self;
				return 0;
			default:
				return fail( ZIP_ER_OPNOTSUPP );
		}
	}
};

} // namespace

bool Zip::create( IOStream& output ) {
	if ( !close() )
		return false;
	if ( mRegisterResource || !output.isOpen() || output.getSize() != 0 ) {
		setError( ZIP_ER_INVAL );
		return false;
	}
	auto context = std::make_unique<ZipStreamSource>();
	context->output = &output;
	zip_error_t error;
	zip_error_init( &error );
	auto* source = zip_source_function_create( ZipStreamSource::callback, context.get(), &error );
	if ( source ) {
		context.release();
		mZip = zip_open_from_source( source, ZIP_CREATE | ZIP_TRUNCATE, &error );
		if ( !mZip )
			zip_source_free( source );
	}
	if ( !mZip )
		setError( zip_error_code_zip( &error ), zip_error_code_system( &error ) );
	zip_error_fini( &error );
	if ( !mZip )
		return false;
	mOpenMode = OpenMode::ReadWrite;
	mError = Error::None;
	mErrorMessage.clear();
	mIsOpen = true;
	return true;
}

void Zip::discard() {
	if ( !mZip )
		return;
	zip_discard( mZip );
	mZip = nullptr;
	mIsOpen = false;
	mZipPath.clear();
	onPackClosed();
}

bool Zip::setCancellationToken( CancellationToken token ) {
	if ( !writable() )
		return false;
	auto owned = std::make_unique<CancellationToken>( std::move( token ) );
	if ( zip_register_cancel_callback_with_state(
			 mZip,
			 []( zip_t*, void* state ) {
				 return static_cast<CancellationToken*>( state )->isCancellationRequested() ? 1 : 0;
			 },
			 []( void* state ) { delete static_cast<CancellationToken*>( state ); },
			 owned.get() ) != 0 ) {
		setError( ZIP_ER_MEMORY );
		return false;
	}
	owned.release();
	return true;
}

bool Zip::setProgressCallback( std::function<void( double )> callback ) {
	if ( !writable() )
		return false;
	if ( !callback )
		return zip_register_progress_callback_with_state( mZip, 0.001, nullptr, nullptr,
														  nullptr ) == 0;
	auto owned = std::make_unique<std::function<void( double )>>( std::move( callback ) );
	if ( zip_register_progress_callback_with_state(
			 mZip, 0.001,
			 []( zip_t*, double progress, void* state ) {
				 ( *static_cast<std::function<void( double )>*>( state ) )( progress );
			 },
			 []( void* state ) { delete static_cast<std::function<void( double )>*>( state ); },
			 owned.get() ) != 0 ) {
		setError( ZIP_ER_MEMORY );
		return false;
	}
	owned.release();
	return true;
}

bool Zip::configureEntry( Int64 index, const EntryOptions& options, bool directory ) {
	if ( index < 0 ||
		 zip_set_file_compression( mZip, index, ZIP_CM_DEFLATE, options.compressionLevel ) != 0 ||
		 ( options.modified && zip_file_set_mtime( mZip, index, *options.modified, 0 ) != 0 ) ||
		 zip_file_set_external_attributes(
			 mZip, index, 0, ZIP_OPSYS_UNIX,
			 ( ( options.symbolicLink ? 0120000u
				 : directory		  ? 0040000u
									  : 0100000u ) |
			   ( options.permissions.value_or( options.symbolicLink ? 0777
											   : directory			? 0755
																	: 0644 ) &
				 07777 ) )
				 << 16 ) != 0 ) {
		auto* error = zip_get_error( mZip );
		setError( zip_error_code_zip( error ), zip_error_code_system( error ) );
		return false;
	}
	return true;
}

bool Zip::addStream( StreamFactory open, Uint64 size, const std::string& name,
					 const EntryOptions& options ) {
	if ( !writable() )
		return false;
	if ( !open || size > static_cast<Uint64>( std::numeric_limits<ios_size>::max() ) ||
		 options.compressionLevel < 0 || options.compressionLevel > 9 ) {
		setError( ZIP_ER_INVAL );
		return false;
	}
	auto context = std::make_unique<ZipStreamSource>();
	context->open = std::move( open );
	context->size = size;
	auto* source = zip_source_function( mZip, ZipStreamSource::callback, context.get() );
	if ( !source ) {
		auto* error = zip_get_error( mZip );
		setError( zip_error_code_zip( error ), zip_error_code_system( error ) );
		return false;
	}
	context.release();
	auto index = zip_file_add( mZip, name.c_str(), source, ZIP_FL_ENC_UTF_8 );
	if ( index < 0 )
		zip_source_free( source );
	return configureEntry( index, options, false );
}

bool Zip::addData( std::span<const Uint8> data, const std::string& name,
				   const EntryOptions& options ) {
	if ( !writable() )
		return false;
	if ( options.compressionLevel < 0 || options.compressionLevel > 9 ) {
		setError( ZIP_ER_INVAL );
		return false;
	}
	// Deferred memory sources own their bytes: stack buffers and temporary strings are safe.
	void* bytes = data.empty() ? nullptr : std::malloc( data.size() );
	if ( !data.empty() && !bytes ) {
		setError( ZIP_ER_MEMORY );
		return false;
	}
	if ( !data.empty() )
		std::memcpy( bytes, data.data(), data.size() );
	auto* source = zip_source_buffer( mZip, bytes, data.size(), 1 );
	if ( !source ) {
		std::free( bytes );
		auto* error = zip_get_error( mZip );
		setError( zip_error_code_zip( error ), zip_error_code_system( error ) );
		return false;
	}
	auto index = zip_file_add( mZip, name.c_str(), source, ZIP_FL_ENC_UTF_8 );
	if ( index < 0 )
		zip_source_free( source );
	return configureEntry( index, options, !name.empty() && name.back() == '/' );
}

bool Zip::addDirectory( const std::string& name, const EntryOptions& options ) {
	return addData( {}, !name.empty() && name.back() == '/' ? name : name + '/', options );
}

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

		if ( !written ) {
			auto* error = zip_get_error( mZip );
			setError( zip_error_code_zip( error ), zip_error_code_system( error ) );
			zip_discard( mZip );
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
	if ( !writable() )
		return false;
	// libzip owns the deferred filename source and uses checked 64-bit UTF-8 file I/O on Windows.
	auto* source = zip_source_file( mZip, path.c_str(), 0, ZIP_LENGTH_TO_END );
	if ( !source ) {
		auto* error = zip_get_error( mZip );
		setError( zip_error_code_zip( error ), zip_error_code_system( error ) );
		return false;
	}
	auto index = zip_file_add( mZip, inpack.c_str(), source, ZIP_FL_ENC_UTF_8 );
	if ( index < 0 )
		zip_source_free( source );
	FileInfo info( path );
	EntryOptions options;
	options.modified = info.getModificationTime();
	options.permissions = info.getPermissions() & 07777;
	return configureEntry( index, options, false );
}

bool Zip::addFile( const Uint8* data, const Uint32& dataSize, const std::string& inpack ) {
	return addData( { data, dataSize }, inpack );
}

bool Zip::addFile( std::vector<Uint8>& data, const std::string& inpack ) {
	return addData( data, inpack );
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
	zip_uint8_t system = 0;
	zip_uint32_t attributes = 0;
	result.unixMode =
		zip_file_get_external_attributes( mZip, index, 0, &system, &attributes ) == 0 &&
				system == ZIP_OPSYS_UNIX
			? attributes >> 16
			: 0;
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
	if ( code == ZIP_ER_CANCELLED )
		mError = Error::Cancelled;
	else if ( code == ZIP_ER_NOENT )
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
