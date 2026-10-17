#include "libarchive.hpp"
#include <algorithm>
#include <cerrno>
#include <eepp/system/sys.hpp>
#include <limits>

namespace EE { namespace VFS { namespace detail {

namespace {
constexpr std::array<const char*, 11> Formats = { "tar",	 "cpio", "ar",	"7zip", "rar", "rar5",
												  "iso9660", "cab",	 "lha", "xar",	"zip" };
constexpr std::array<const char*, 11> Filters = { "none", "gzip", "bzip2",	  "xz",	 "lzma", "lzip",
												  "zstd", "lz4",  "compress", "rpm", "uu" };
} // namespace

VFSResult<std::shared_ptr<LibArchive>> LibArchive::load( const std::string& path, const URI& uri ) {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN || EE_PLATFORM == EE_PLATFORM_IOS
	(void)path;
#else
	auto library = std::make_shared<LibArchive>();
	if ( !path.empty() ) {
		library->mLibrary = System::Sys::loadObject( path );
	} else {
#if EE_PLATFORM == EE_PLATFORM_WIN
		const char* names[] = { "archive.dll", "libarchive-13.dll", "libarchive.dll" };
#elif EE_PLATFORM == EE_PLATFORM_MACOS
		const char* names[] = { "/usr/lib/libarchive.2.dylib", "libarchive.13.dylib",
								"libarchive.dylib" };
#else
		const char* names[] = { "libarchive.so.13", "libarchive.so" };
#endif
		for ( const char* name : names ) {
			library->mLibrary = System::Sys::loadObject( name );
			if ( library->mLibrary )
				break;
		}
	}
	if ( library->mLibrary && library->bind() )
		return Ok( std::move( library ) );
#endif
	return Err( VFSError( VFSErrorCode::Unsupported, uri,
						  "A compatible libarchive 3.x runtime library is not available" ) );
}

LibArchive::~LibArchive() {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN && EE_PLATFORM != EE_PLATFORM_IOS
	if ( mLibrary )
		System::Sys::unloadObject( mLibrary );
#endif
}

bool LibArchive::bind() {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN || EE_PLATFORM == EE_PLATFORM_IOS
	return false;
#else
	auto bind = [this]( auto& fn, const std::string& name ) {
		fn = reinterpret_cast<std::decay_t<decltype( fn )>>(
			System::Sys::loadFunction( mLibrary, name ) );
		return fn != nullptr;
	};
	if ( !bind( mVersion, "archive_version_number" ) || mVersion() < 3000000 ||
		 mVersion() >= 4000000 )
		return false;
#define EE_ARCHIVE_BIND( member, symbol ) \
	if ( !bind( member, symbol ) )        \
	return false
	EE_ARCHIVE_BIND( mVersionString, "archive_version_string" );
	EE_ARCHIVE_BIND( readNew, "archive_read_new" );
	EE_ARCHIVE_BIND( readFree, "archive_read_free" );
	EE_ARCHIVE_BIND( readOpen, "archive_read_open2" );
	EE_ARCHIVE_BIND( setSeek, "archive_read_set_seek_callback" );
	EE_ARCHIVE_BIND( nextHeader, "archive_read_next_header" );
	EE_ARCHIVE_BIND( readData, "archive_read_data" );
	EE_ARCHIVE_BIND( errorString, "archive_error_string" );
	EE_ARCHIVE_BIND( errorNumber, "archive_errno" );
	EE_ARCHIVE_BIND( formatCode, "archive_format" );
	EE_ARCHIVE_BIND( filterCount, "archive_filter_count" );
	EE_ARCHIVE_BIND( filterCode, "archive_filter_code" );
	EE_ARCHIVE_BIND( entryPath, "archive_entry_pathname_utf8" );
	EE_ARCHIVE_BIND( entryLink, "archive_entry_symlink_utf8" );
	EE_ARCHIVE_BIND( entryHardLink, "archive_entry_hardlink_utf8" );
	EE_ARCHIVE_BIND( entrySize, "archive_entry_size" );
	EE_ARCHIVE_BIND( entrySizeSet, "archive_entry_size_is_set" );
	EE_ARCHIVE_BIND( entryType, "archive_entry_filetype" );
	EE_ARCHIVE_BIND( entryPermissions, "archive_entry_perm" );
	EE_ARCHIVE_BIND( entryTime, "archive_entry_mtime" );
	EE_ARCHIVE_BIND( entryTimeSet, "archive_entry_mtime_is_set" );
#undef EE_ARCHIVE_BIND
	// Probe each decoder on its own disposable reader. A WARN can register an external-command
	// fallback, so that reader must never be used to open untrusted input.
	auto probe = [&]( auto& functions, const auto& names, const char* prefix ) {
		for ( size_t i = 0; i < names.size(); ++i ) {
			Enable enable = nullptr;
			if ( !bind( enable, std::string( prefix ) + names[i] ) )
				continue;
			archive* reader = readNew();
			if ( !reader )
				continue;
			if ( enable( reader ) == 0 )
				functions[i] = enable;
			readFree( reader );
		}
	};
	probe( mFormats, Formats, "archive_read_support_format_" );
	probe( mFilters, Filters, "archive_read_support_filter_" );
	return true;
#endif
}

ArchiveSupport LibArchive::support() const {
	ArchiveSupport support;
	support.version = mVersionString();
	for ( size_t i = 0; i < Formats.size(); ++i ) {
		if ( mFormats[i] )
			support.formats.emplace_back( Formats[i] );
	}
	for ( size_t i = 0; i < Filters.size(); ++i ) {
		if ( mFilters[i] )
			support.filters.emplace_back( Filters[i] );
	}
	return support;
}

VFSResult<void> LibArchive::configure( archive* reader, const URI& uri ) const {
	if ( !reader )
		return Err( VFSError( VFSErrorCode::IO, uri, "Could not allocate archive reader" ) );
	for ( auto enable : mFormats ) {
		if ( enable && enable( reader ) != 0 )
			return Err( error( reader, uri ) );
	}
	for ( auto enable : mFilters ) {
		if ( enable && enable( reader ) != 0 )
			return Err( error( reader, uri ) );
	}
	return Ok();
}

VFSError LibArchive::error( archive* reader, const URI& uri ) const {
	const char* message = errorString( reader );
	int number = errorNumber( reader );
	return VFSError( number == EACCES || number == EPERM ? VFSErrorCode::PermissionDenied
														 : VFSErrorCode::IO,
					 uri, message ? message : "Could not read archive", number );
}

LibArchiveReader::LibArchiveReader( std::shared_ptr<LibArchive> library, const std::string& path,
									URI uri ) :
	mLibrary( std::move( library ) ), mFile( path ), mUri( std::move( uri ) ) {}

LibArchiveReader::~LibArchiveReader() {
	if ( mReader )
		mLibrary->readFree( mReader );
}

VFSResult<void> LibArchiveReader::open( CancellationToken token ) {
	mToken = std::move( token );
	if ( mToken.isCancellationRequested() )
		return Err( cancelledError( mUri ) );
	if ( !mFile.isOpen() )
		return Err( VFSError( VFSErrorCode::IO, mUri, "Could not open archive backing file" ) );
	mReader = mLibrary->readNew();
	auto configured = mLibrary->configure( mReader, mUri );
	if ( !configured )
		return configured;
	if ( mLibrary->setSeek( mReader, &seekCallback ) != 0 ||
		 mLibrary->readOpen( mReader, this, nullptr, &readCallback, &skipCallback, nullptr ) != 0 )
		return Err( error() );
	return Ok();
}

VFSResult<bool> LibArchiveReader::next( CancellationToken token ) {
	mToken = std::move( token );
	if ( mToken.isCancellationRequested() )
		return Err( cancelledError( mUri ) );
	int status = mLibrary->nextHeader( mReader, &mEntry );
	if ( status == 1 )
		return Ok( false );
	// WARN can mean an entry was only partially decoded. Never silently publish that index.
	if ( status != 0 )
		return Err( error() );
	return Ok( true );
}

VFSResult<size_t> LibArchiveReader::read( std::span<Uint8> data, CancellationToken token ) {
	mToken = std::move( token );
	if ( mToken.isCancellationRequested() )
		return Err( cancelledError( mUri ) );
	auto size = mLibrary->readData( mReader, data.data(), data.size() );
	if ( size < 0 )
		return Err( error() );
	return Ok( static_cast<size_t>( size ) );
}

bool LibArchiveReader::canReuse() const {
	if ( ( mLibrary->formatCode( mReader ) & 0xff0000 ) != 0x30000 )
		return false;
	for ( int i = 0; i < mLibrary->filterCount( mReader ); ++i ) {
		const int code = mLibrary->filterCode( mReader, i );
		if ( code != 0 && code != 1 )
			return false;
	}
	return true;
}

VFSResult<void> LibArchiveReader::resume( CancellationToken token ) {
	mToken = std::move( token );
	if ( mToken.isCancellationRequested() )
		return Err( cancelledError( mUri ) );
	if ( !mFile.resume() )
		return Err( VFSError( VFSErrorCode::IO, mUri, "Could not reopen archive input" ) );
	return Ok();
}

VFSResult<VFSFileInfo> LibArchiveReader::info() const {
	VFSFileInfo info;
	// POSIX archive type values, independent of the host's stat constants.
	switch ( mLibrary->entryType( mEntry ) ) {
		case 0100000:
			info.type = FileType::Regular;
			break;
		case 0040000:
			info.type = FileType::Directory;
			break;
		case 0120000:
			info.type = FileType::Symlink;
			break;
		case 0010000:
			info.type = FileType::FIFO;
			break;
		case 0020000:
			info.type = FileType::CharacterDevice;
			break;
		case 0060000:
			info.type = FileType::BlockDevice;
			break;
		default:
			break;
	}
	if ( const char* link = mLibrary->entryLink( mEntry ) ) {
		if ( std::char_traits<char>::length( link ) > 4096 )
			return Err(
				VFSError( VFSErrorCode::InvalidOperation, mUri, "Archive link is too long" ) );
		info.symlinkTarget = link;
	}
	if ( mLibrary->entrySizeSet( mEntry ) && mLibrary->entrySize( mEntry ) >= 0 )
		info.setSize( static_cast<Uint64>( mLibrary->entrySize( mEntry ) ) );
	if ( mLibrary->entryTimeSet( mEntry ) )
		info.setModifiedTime( mLibrary->entryTime( mEntry ) );
	info.setPermissions( mLibrary->entryPermissions( mEntry ) & 07777 );
	return Ok( std::move( info ) );
}

VFSError LibArchiveReader::error() const {
	return mToken.isCancellationRequested() ? cancelledError( mUri )
											: mLibrary->error( mReader, mUri );
}

LibArchive::Count LibArchiveReader::readCallback( archive*, void* context, const void** buffer ) {
	auto& self = *static_cast<LibArchiveReader*>( context );
	if ( self.mToken.isCancellationRequested() )
		return -1;
	*buffer = self.mBuffer.data();
	return self.mFile.read( self.mBuffer.data(), self.mBuffer.size() );
}

LibArchive::Offset LibArchiveReader::skipCallback( archive*, void* context,
												   LibArchive::Offset amount ) {
	auto& self = *static_cast<LibArchiveReader*>( context );
	if ( self.mToken.isCancellationRequested() || amount <= 0 )
		return 0;
	const auto position = self.mFile.tell();
	const auto size = self.mFile.size();
	if ( position < 0 || size < position )
		return 0;
	const auto skip = std::min<Int64>( amount, size - position );
	return self.mFile.seek( position + skip ) == position + skip ? skip : 0;
}

LibArchive::Offset LibArchiveReader::seekCallback( archive*, void* context,
												   LibArchive::Offset offset, int whence ) {
	auto& self = *static_cast<LibArchiveReader*>( context );
	if ( self.mToken.isCancellationRequested() )
		return -30;
	Int64 base = whence == SEEK_SET	  ? 0
				 : whence == SEEK_CUR ? self.mFile.tell()
									  : self.mFile.size();
	if ( base < 0 || ( offset < 0 && offset < -base ) ||
		 ( offset > 0 && offset > std::numeric_limits<Int64>::max() - base ) )
		return -25;
	return self.mFile.seek( base + offset );
}

}}} // namespace EE::VFS::detail
