#include <eepp/core/string.hpp>
#include <eepp/vfs/providers/localprovider.hpp>
#include <mutex>
#include <shared_mutex>

#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>
#if defined( __linux__ )
#include <sys/syscall.h>
#ifndef RENAME_NOREPLACE
#define RENAME_NOREPLACE ( 1 << 0 )
#endif
#endif
#if defined( __APPLE__ )
#include <stdio.h>
#endif
#endif

namespace EE { namespace VFS {

using System::ThreadPool;

LocalProvider::LocalProvider( std::shared_ptr<ThreadPool> pool ) : mPool( std::move( pool ) ) {}

LocalProvider::~LocalProvider() = default;

ProviderCapabilities LocalProvider::capabilities( const URI& ) const {
	ProviderCapabilities caps =
		VFSCapability::Read | VFSCapability::Write | VFSCapability::List | VFSCapability::Create |
		VFSCapability::Remove | VFSCapability::Rename | VFSCapability::Seek |
		VFSCapability::RandomAccess | VFSCapability::Truncate | VFSCapability::Symlinks |
		VFSCapability::Permissions | VFSCapability::AtomicRename | VFSCapability::AtomicReplace |
		VFSCapability::Fsync | VFSCapability::FreeSpace;
#if EE_PLATFORM != EE_PLATFORM_WIN
	caps.set( VFSCapability::HardLinks ).set( VFSCapability::Ownership );
#endif
#if EE_PLATFORM != EE_PLATFORM_WIN && EE_PLATFORM != EE_PLATFORM_MACOS
	caps.set( VFSCapability::CaseSensitive );
#endif
	return caps;
}

VFSResult<std::string> LocalProvider::toNativePath( const URI& uri ) {
	if ( uri.getScheme() != "file" )
		return Err( VFSError( VFSErrorCode::InvalidURI, uri, "Not a file:// URI" ) );
#if EE_PLATFORM != EE_PLATFORM_WIN
	if ( !uri.getHost().empty() && uri.getHost() != "localhost" )
		return Err( VFSError( VFSErrorCode::Unsupported, uri, "Remote file:// hosts" ) );
#endif
	if ( uri.getPath().empty() )
		return Err( VFSError( VFSErrorCode::InvalidURI, uri, "Empty path" ) );
	return Ok( uri.getFSPath() );
}

template <typename T, typename F>
VFSTask<T> LocalProvider::run( const URI& uri, CancellationToken token, F&& fn ) {
	if ( token.isCancellationRequested() )
		return readyError<T>( cancelledError( uri ) );
	auto path = toNativePath( uri );
	if ( !path )
		return readyError<T>( std::move( path ).error() );
	return mPool->submit( [fn = std::forward<F>( fn ), path = std::move( path ).value(),
						   token = std::move( token )]() mutable -> VFSResult<T> {
		if ( token.isCancellationRequested() )
			return Err( VFSError( VFSErrorCode::Cancelled, URI(), "Operation cancelled" ) );
		return fn( path );
	} );
}

namespace {

std::string lastSegment( const std::string& path ) {
	std::string_view view( path );
	while ( view.size() > 1 && ( view.back() == '/' || view.back() == '\\' ) )
		view.remove_suffix( 1 );
	size_t pos = view.find_last_of( "/\\" );
	if ( pos == std::string_view::npos || view.size() == 1 )
		return std::string( view );
	return std::string( view.substr( pos + 1 ) );
}

} // namespace

#if EE_PLATFORM == EE_PLATFORM_WIN

// ---------------------------------------------------------------------------------------------
// Win32
// ---------------------------------------------------------------------------------------------

namespace {

std::wstring toWide( const std::string& path ) {
	return String::fromUtf8( path ).toWideString();
}

std::string fromWide( const wchar_t* str ) {
	return String( std::wstring( str ) ).toUtf8();
}

VFSErrorCode mapWin32Error( DWORD error ) {
	switch ( error ) {
		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
		case ERROR_INVALID_DRIVE:
		case ERROR_BAD_NETPATH:
		case ERROR_BAD_NET_NAME:
			return VFSErrorCode::NotFound;
		case ERROR_FILE_EXISTS:
		case ERROR_ALREADY_EXISTS:
			return VFSErrorCode::AlreadyExists;
		case ERROR_ACCESS_DENIED:
		case ERROR_SHARING_VIOLATION:
		case ERROR_LOCK_VIOLATION:
			return VFSErrorCode::PermissionDenied;
		case ERROR_WRITE_PROTECT:
			return VFSErrorCode::ReadOnly;
		case ERROR_DIR_NOT_EMPTY:
			return VFSErrorCode::NotEmpty;
		case ERROR_DIRECTORY:
			return VFSErrorCode::NotDirectory;
		case ERROR_NOT_SAME_DEVICE:
		case ERROR_NOT_SUPPORTED:
			return VFSErrorCode::Unsupported;
		case ERROR_DISK_FULL:
		case ERROR_HANDLE_DISK_FULL:
			return VFSErrorCode::NoSpace;
		case ERROR_INVALID_NAME:
		case ERROR_FILENAME_EXCED_RANGE:
			return VFSErrorCode::InvalidURI;
		case ERROR_CANT_RESOLVE_FILENAME:
			return VFSErrorCode::TooManySymlinks;
		case ERROR_OPERATION_ABORTED:
			return VFSErrorCode::Cancelled;
		default:
			return VFSErrorCode::IO;
	}
}

VFSError lastError( const std::string& path, const char* what ) {
	DWORD error = GetLastError();
	return VFSError( mapWin32Error( error ), URI::fromFSPath( path ), what, error );
}

Int64 fileTimeToUnix( const FILETIME& ft ) {
	ULARGE_INTEGER value;
	value.LowPart = ft.dwLowDateTime;
	value.HighPart = ft.dwHighDateTime;
	return static_cast<Int64>( ( value.QuadPart - 116444736000000000ULL ) / 10000000ULL );
}

Uint64 fileTimeRaw( const FILETIME& ft ) {
	return ( static_cast<Uint64>( ft.dwHighDateTime ) << 32 ) | ft.dwLowDateTime;
}

FileType typeFromAttributes( DWORD attributes, DWORD reparseTag ) {
	if ( ( attributes & FILE_ATTRIBUTE_REPARSE_POINT ) &&
		 ( reparseTag == IO_REPARSE_TAG_SYMLINK || reparseTag == IO_REPARSE_TAG_MOUNT_POINT ) )
		return FileType::Symlink;
	return ( attributes & FILE_ATTRIBUTE_DIRECTORY ) ? FileType::Directory : FileType::Regular;
}

/** Common metadata fill for Win32 attribute data. */
void fillInfo( FileInfo& info, DWORD attributes, const FILETIME& created, const FILETIME& accessed,
			   const FILETIME& modified, Uint64 size ) {
	info.setModifiedTime( fileTimeToUnix( modified ) );
	info.setCreatedTime( fileTimeToUnix( created ) );
	info.setAccessedTime( fileTimeToUnix( accessed ) );
	if ( !( attributes & FILE_ATTRIBUTE_DIRECTORY ) )
		info.setSize( size );
	Uint32 perms = ( attributes & FILE_ATTRIBUTE_READONLY ) ? 0444 : 0666;
	if ( attributes & FILE_ATTRIBUTE_DIRECTORY )
		perms |= 0111;
	info.setPermissions( perms );
	info.hidden =
		( attributes & FILE_ATTRIBUTE_HIDDEN ) || ( !info.name.empty() && info.name[0] == '.' );
}

HANDLE openForMetadata( const std::wstring& path, bool follow ) {
	return CreateFileW(
		path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr, OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | ( follow ? 0 : FILE_FLAG_OPEN_REPARSE_POINT ), nullptr );
}

VFSResult<FileInfo> statHandle( HANDLE handle, const URI& uri, const std::string& path ) {
	BY_HANDLE_FILE_INFORMATION data;
	if ( !GetFileInformationByHandle( handle, &data ) )
		return Err( lastError( path, "GetFileInformationByHandle failed" ) );
	FileInfo info;
	info.name = lastSegment( path );
	DWORD tag = 0;
	if ( data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) {
		FILE_ATTRIBUTE_TAG_INFO tagInfo;
		if ( GetFileInformationByHandleEx( handle, FileAttributeTagInfo, &tagInfo,
										   sizeof( tagInfo ) ) )
			tag = tagInfo.ReparseTag;
	}
	info.type = typeFromAttributes( data.dwFileAttributes, tag );
	Uint64 size = ( static_cast<Uint64>( data.nFileSizeHigh ) << 32 ) | data.nFileSizeLow;
	fillInfo( info, data.dwFileAttributes, data.ftCreationTime, data.ftLastAccessTime,
			  data.ftLastWriteTime, size );
	Uint64 index = ( static_cast<Uint64>( data.nFileIndexHigh ) << 32 ) | data.nFileIndexLow;
	info.revision = FileRevision::fromValues(
		{ data.dwVolumeSerialNumber, index, fileTimeRaw( data.ftLastWriteTime ), size } );
	return Ok( std::move( info ) );
}

class LocalDirectoryImpl : public DirectoryHandleImpl,
						   public std::enable_shared_from_this<LocalDirectoryImpl> {
  public:
	LocalDirectoryImpl( std::shared_ptr<ThreadPool> pool, URI uri, std::string path,
						DirectoryOpenOptions options ) :
		mPool( std::move( pool ) ),
		mUri( std::move( uri ) ),
		mPath( std::move( path ) ),
		mOptions( options ) {}

	~LocalDirectoryImpl() override { closeNow(); }

	VFSResult<void> start() {
		std::wstring pattern = toWide( mPath );
		if ( !pattern.empty() && pattern.back() != L'\\' && pattern.back() != L'/' )
			pattern += L'\\';
		pattern += L'*';
		mFind = FindFirstFileExW( pattern.c_str(), FindExInfoBasic, &mData, FindExSearchNameMatch,
								  nullptr, FIND_FIRST_EX_LARGE_FETCH );
		if ( mFind == INVALID_HANDLE_VALUE ) {
			if ( GetLastError() == ERROR_FILE_NOT_FOUND ) {
				mEnd = true; // Empty root of a drive.
				return Ok();
			}
			return Err( lastError( mPath, "FindFirstFileExW failed" ) );
		}
		mHasPending = true;
		return Ok();
	}

	VFSTask<DirectoryBatch> read( size_t maxEntries, CancellationToken token ) override {
		return mPool->submit(
			[self = shared_from_this(), maxEntries, token]() -> VFSResult<DirectoryBatch> {
				return self->readSync( maxEntries, token );
			} );
	}

	VFSTask<void> close() override {
		std::lock_guard<std::mutex> lock( mMutex );
		closeNow();
		return readyValue<void>();
	}

  private:
	VFSResult<DirectoryBatch> readSync( size_t maxEntries, const CancellationToken& token ) {
		std::lock_guard<std::mutex> lock( mMutex );
		DirectoryBatch batch;
		if ( token.isCancellationRequested() )
			return Err( cancelledError( mUri ) );
		batch.entries.reserve( std::min<size_t>( maxEntries, 1024 ) );
		while ( !mEnd && batch.entries.size() < maxEntries ) {
			if ( !mHasPending ) {
				if ( mFind == INVALID_HANDLE_VALUE || !FindNextFileW( mFind, &mData ) ) {
					if ( mFind != INVALID_HANDLE_VALUE && GetLastError() != ERROR_NO_MORE_FILES )
						return Err( lastError( mPath, "FindNextFileW failed" ) );
					mEnd = true;
					break;
				}
			}
			mHasPending = false;
			if ( wcscmp( mData.cFileName, L"." ) == 0 || wcscmp( mData.cFileName, L".." ) == 0 )
				continue;
			// Built in place: a FileInfo is large, moving it into the batch is measurable.
			FileInfo& info = batch.entries.emplace_back();
			info.name = fromWide( mData.cFileName );
			info.type = typeFromAttributes( mData.dwFileAttributes, mData.dwReserved0 );
			Uint64 size = ( static_cast<Uint64>( mData.nFileSizeHigh ) << 32 ) | mData.nFileSizeLow;
			fillInfo( info, mData.dwFileAttributes, mData.ftCreationTime, mData.ftLastAccessTime,
					  mData.ftLastWriteTime, size );
			info.revision =
				FileRevision::fromValues( { fileTimeRaw( mData.ftLastWriteTime ), size } );
			if ( info.type == FileType::Symlink && mOptions.resolveSymlinkTargets ) {
				std::string childPath = mPath;
				if ( !childPath.empty() && childPath.back() != '\\' && childPath.back() != '/' )
					childPath += '\\';
				childPath += info.name;
				DWORD attrs = GetFileAttributesW( toWide( childPath ).c_str() );
				if ( attrs != INVALID_FILE_ATTRIBUTES )
					info.targetType = ( attrs & FILE_ATTRIBUTE_DIRECTORY ) ? FileType::Directory
																		   : FileType::Regular;
			}
		}
		batch.endOfDirectory = mEnd;
		if ( mEnd )
			closeNow();
		return Ok( std::move( batch ) );
	}

	void closeNow() {
		if ( mFind != INVALID_HANDLE_VALUE ) {
			FindClose( mFind );
			mFind = INVALID_HANDLE_VALUE;
		}
		mEnd = true;
	}

	std::shared_ptr<ThreadPool> mPool;
	URI mUri;
	std::string mPath;
	std::mutex mMutex;
	WIN32_FIND_DATAW mData{};
	HANDLE mFind{ INVALID_HANDLE_VALUE };
	DirectoryOpenOptions mOptions;
	bool mHasPending{ false };
	bool mEnd{ false };
};

class LocalFileImpl : public FileHandleImpl, public std::enable_shared_from_this<LocalFileImpl> {
  public:
	LocalFileImpl( std::shared_ptr<ThreadPool> pool, URI uri, std::string path, HANDLE handle,
				   ProviderCapabilities caps ) :
		mPool( std::move( pool ) ),
		mUri( std::move( uri ) ),
		mPath( std::move( path ) ),
		mHandle( handle ),
		mCaps( caps ) {}

	~LocalFileImpl() override {
		if ( mHandle != INVALID_HANDLE_VALUE )
			CloseHandle( mHandle );
	}

	VFSTask<size_t> read( Uint64 offset, std::span<Uint8> data, CancellationToken token ) override {
		return submit<size_t>( token, [offset, data]( LocalFileImpl& self ) -> VFSResult<size_t> {
			size_t done = 0;
			while ( done < data.size() ) {
				OVERLAPPED ov{};
				Uint64 pos = offset + done;
				ov.Offset = static_cast<DWORD>( pos );
				ov.OffsetHigh = static_cast<DWORD>( pos >> 32 );
				DWORD chunk =
					static_cast<DWORD>( std::min<size_t>( data.size() - done, 0x40000000u ) );
				DWORD got = 0;
				if ( !ReadFile( self.mHandle, data.data() + done, chunk, &got, &ov ) ) {
					if ( GetLastError() == ERROR_HANDLE_EOF )
						break;
					return Err( lastError( self.mPath, "ReadFile failed" ) );
				}
				if ( got == 0 )
					break;
				done += got;
			}
			return Ok( done );
		} );
	}

	VFSTask<size_t> write( Uint64 offset, std::span<const Uint8> data,
						   CancellationToken token ) override {
		return submit<size_t>( token, [offset, data]( LocalFileImpl& self ) -> VFSResult<size_t> {
			size_t done = 0;
			while ( done < data.size() ) {
				OVERLAPPED ov{};
				Uint64 pos = offset + done;
				ov.Offset = static_cast<DWORD>( pos );
				ov.OffsetHigh = static_cast<DWORD>( pos >> 32 );
				DWORD chunk =
					static_cast<DWORD>( std::min<size_t>( data.size() - done, 0x40000000u ) );
				DWORD put = 0;
				if ( !WriteFile( self.mHandle, data.data() + done, chunk, &put, &ov ) )
					return Err( lastError( self.mPath, "WriteFile failed" ) );
				done += put;
			}
			return Ok( done );
		} );
	}

	VFSTask<void> truncate( Uint64 size ) override {
		return submit<void>( {}, [size]( LocalFileImpl& self ) -> VFSResult<void> {
			FILE_END_OF_FILE_INFO eof;
			eof.EndOfFile.QuadPart = static_cast<LONGLONG>( size );
			if ( !SetFileInformationByHandle( self.mHandle, FileEndOfFileInfo, &eof,
											  sizeof( eof ) ) )
				return Err( lastError( self.mPath, "Truncate failed" ) );
			return Ok();
		} );
	}

	VFSTask<void> flush() override {
		return submit<void>( {}, []( LocalFileImpl& self ) -> VFSResult<void> {
			if ( !FlushFileBuffers( self.mHandle ) )
				return Err( lastError( self.mPath, "FlushFileBuffers failed" ) );
			return Ok();
		} );
	}

	VFSTask<FileInfo> stat() override {
		return submit<FileInfo>( {}, []( LocalFileImpl& self ) {
			return statHandle( self.mHandle, self.mUri, self.mPath );
		} );
	}

	VFSTask<void> close() override {
		return mPool->submit( [self = shared_from_this()]() -> VFSResult<void> {
			std::unique_lock<std::shared_mutex> lock( self->mMutex );
			if ( self->mHandle != INVALID_HANDLE_VALUE ) {
				BOOL ok = CloseHandle( self->mHandle );
				self->mHandle = INVALID_HANDLE_VALUE;
				if ( !ok )
					return Err( lastError( self->mPath, "CloseHandle failed" ) );
			}
			return Ok();
		} );
	}

	ProviderCapabilities capabilities() const override { return mCaps; }

  private:
	template <typename T, typename F> VFSTask<T> submit( CancellationToken token, F&& fn ) {
		return mPool->submit( [self = shared_from_this(), fn = std::forward<F>( fn ),
							   token = std::move( token )]() mutable -> VFSResult<T> {
			if ( token.isCancellationRequested() )
				return Err( cancelledError( self->mUri ) );
			std::shared_lock<std::shared_mutex> lock( self->mMutex );
			if ( self->mHandle == INVALID_HANDLE_VALUE )
				return Err( VFSError( VFSErrorCode::InvalidOperation, self->mUri, "File closed" ) );
			return fn( *self );
		} );
	}

	std::shared_ptr<ThreadPool> mPool;
	URI mUri;
	std::string mPath;
	std::shared_mutex mMutex;
	HANDLE mHandle;
	ProviderCapabilities mCaps;
};

} // namespace

VFSResult<FileInfo> LocalProvider::statSync( const URI& uri, const std::string& path,
											 bool followSymlinks ) {
	std::wstring wide = toWide( path );
	HANDLE handle = openForMetadata( wide, followSymlinks );
	if ( handle == INVALID_HANDLE_VALUE ) {
		WIN32_FILE_ATTRIBUTE_DATA data;
		if ( !GetFileAttributesExW( wide.c_str(), GetFileExInfoStandard, &data ) )
			return Err( lastError( path, "Stat failed" ) );
		FileInfo info;
		info.name = lastSegment( path );
		info.type = typeFromAttributes( data.dwFileAttributes, 0 );
		Uint64 size = ( static_cast<Uint64>( data.nFileSizeHigh ) << 32 ) | data.nFileSizeLow;
		fillInfo( info, data.dwFileAttributes, data.ftCreationTime, data.ftLastAccessTime,
				  data.ftLastWriteTime, size );
		info.revision = FileRevision::fromValues( { fileTimeRaw( data.ftLastWriteTime ), size } );
		return Ok( std::move( info ) );
	}
	auto info = statHandle( handle, uri, path );
	CloseHandle( handle );
	if ( info && info.value().type == FileType::Symlink ) {
		HANDLE target = openForMetadata( wide, true );
		if ( target != INVALID_HANDLE_VALUE ) {
			auto targetInfo = statHandle( target, uri, path );
			CloseHandle( target );
			if ( targetInfo )
				info.value().targetType = targetInfo.value().type;
		}
	}
	return info;
}

VFSTask<FileInfo> LocalProvider::stat( const URI& uri, const StatOptions& options,
									   CancellationToken token ) {
	return run<FileInfo>( uri, std::move( token ),
						  [uri, follow = options.followSymlinks]( const std::string& path ) {
							  return statSync( uri, path, follow );
						  } );
}

VFSTask<DirectoryHandle> LocalProvider::openDirectory( const URI& uri,
													   const DirectoryOpenOptions& options,
													   CancellationToken token ) {
	return run<DirectoryHandle>(
		uri, std::move( token ),
		[pool = mPool, uri, options]( const std::string& path ) -> VFSResult<DirectoryHandle> {
			DWORD attrs = GetFileAttributesW( toWide( path ).c_str() );
			if ( attrs == INVALID_FILE_ATTRIBUTES )
				return Err( lastError( path, "Open directory failed" ) );
			if ( !( attrs & FILE_ATTRIBUTE_DIRECTORY ) )
				return Err( VFSError( VFSErrorCode::NotDirectory, uri ) );
			auto impl = std::make_shared<LocalDirectoryImpl>( pool, uri, path, options );
			if ( auto started = impl->start(); !started )
				return Err( std::move( started ).error() );
			return Ok( DirectoryHandle( uri, std::move( impl ) ) );
		} );
}

VFSTask<FileHandle> LocalProvider::open( const URI& uri, const OpenOptions& options,
										 CancellationToken token ) {
	return run<FileHandle>(
		uri, std::move( token ),
		[pool = mPool, uri, options,
		 caps = capabilities( uri )]( const std::string& path ) -> VFSResult<FileHandle> {
			OpenFlags flags = options.flags;
			DWORD access = 0;
			if ( hasFlag( flags, OpenFlags::Read ) )
				access |= GENERIC_READ;
			if ( hasFlag( flags, OpenFlags::Write ) )
				access |= GENERIC_WRITE;
			DWORD disposition = OPEN_EXISTING;
			bool create = hasFlag( flags, OpenFlags::Create );
			bool truncate = hasFlag( flags, OpenFlags::Truncate );
			if ( create && hasFlag( flags, OpenFlags::Exclusive ) )
				disposition = CREATE_NEW;
			else if ( create && truncate )
				disposition = CREATE_ALWAYS;
			else if ( create )
				disposition = OPEN_ALWAYS;
			else if ( truncate )
				disposition = TRUNCATE_EXISTING;
			DWORD attributes = FILE_ATTRIBUTE_NORMAL;
			if ( options.createPermissions && !( *options.createPermissions & 0222 ) )
				attributes = FILE_ATTRIBUTE_READONLY;
			HANDLE handle = CreateFileW( toWide( path ).c_str(), access,
										 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
										 nullptr, disposition, attributes, nullptr );
			if ( handle == INVALID_HANDLE_VALUE )
				return Err( lastError( path, "Open failed" ) );
			BY_HANDLE_FILE_INFORMATION data;
			if ( GetFileInformationByHandle( handle, &data ) &&
				 ( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ) {
				CloseHandle( handle );
				return Err( VFSError( VFSErrorCode::IsDirectory, uri ) );
			}
			return Ok( FileHandle(
				uri, std::make_shared<LocalFileImpl>( pool, uri, path, handle, caps ) ) );
		} );
}

VFSTask<void> LocalProvider::mkdir( const URI& uri, const MkDirOptions&, CancellationToken token ) {
	return run<void>( uri, std::move( token ), []( const std::string& path ) -> VFSResult<void> {
		if ( !CreateDirectoryW( toWide( path ).c_str(), nullptr ) )
			return Err( lastError( path, "Create directory failed" ) );
		return Ok();
	} );
}

VFSTask<void> LocalProvider::remove( const URI& uri, const RemoveOptions&,
									 CancellationToken token ) {
	return run<void>( uri, std::move( token ), []( const std::string& path ) -> VFSResult<void> {
		std::wstring wide = toWide( path );
		DWORD attrs = GetFileAttributesW( wide.c_str() );
		if ( attrs == INVALID_FILE_ATTRIBUTES )
			return Err( lastError( path, "Remove failed" ) );
		// Directory symlinks/junctions are removed with RemoveDirectory, never followed.
		BOOL ok = ( attrs & FILE_ATTRIBUTE_DIRECTORY ) ? RemoveDirectoryW( wide.c_str() )
													   : DeleteFileW( wide.c_str() );
		if ( !ok )
			return Err( lastError( path, "Remove failed" ) );
		return Ok();
	} );
}

VFSTask<void> LocalProvider::rename( const URI& from, const URI& to, const RenameOptions& options,
									 CancellationToken token ) {
	auto toPath = toNativePath( to );
	if ( !toPath )
		return readyError<void>( std::move( toPath ).error() );
	return run<void>( from, std::move( token ),
					  [to = std::move( toPath ).value(), overwrite = options.overwrite](
						  const std::string& path ) -> VFSResult<void> {
						  // No MOVEFILE_COPY_ALLOWED: cross-volume moves fail with
						  // ERROR_NOT_SAME_DEVICE and the VFS falls back to copy + remove.
						  DWORD flags = 0;
						  if ( overwrite )
							  flags |= MOVEFILE_REPLACE_EXISTING;
						  if ( !MoveFileExW( toWide( path ).c_str(), toWide( to ).c_str(), flags ) )
							  return Err( lastError( path, "Rename failed" ) );
						  return Ok();
					  } );
}

VFSTask<URI> LocalProvider::canonicalize( const URI& uri, CancellationToken token ) {
	return run<URI>( uri, std::move( token ), []( const std::string& path ) -> VFSResult<URI> {
		HANDLE handle = openForMetadata( toWide( path ), true );
		if ( handle == INVALID_HANDLE_VALUE )
			return Err( lastError( path, "Canonicalize failed" ) );
		wchar_t buffer[32768];
		DWORD len = GetFinalPathNameByHandleW( handle, buffer, 32768, FILE_NAME_NORMALIZED );
		CloseHandle( handle );
		if ( len == 0 || len >= 32768 )
			return Err( lastError( path, "GetFinalPathNameByHandleW failed" ) );
		std::string result = fromWide( buffer );
		if ( String::startsWith( result, "\\\\?\\UNC\\" ) )
			result = "\\\\" + result.substr( 8 );
		else if ( String::startsWith( result, "\\\\?\\" ) )
			result = result.substr( 4 );
		return Ok( URI::fromFSPath( result ) );
	} );
}

VFSTask<void> LocalProvider::setPermissions( const URI& uri, Uint32 permissions,
											 CancellationToken token ) {
	return run<void>( uri, std::move( token ),
					  [permissions]( const std::string& path ) -> VFSResult<void> {
						  std::wstring wide = toWide( path );
						  DWORD attrs = GetFileAttributesW( wide.c_str() );
						  if ( attrs == INVALID_FILE_ATTRIBUTES )
							  return Err( lastError( path, "Set permissions failed" ) );
						  if ( permissions & 0222 )
							  attrs &= ~FILE_ATTRIBUTE_READONLY;
						  else
							  attrs |= FILE_ATTRIBUTE_READONLY;
						  if ( !SetFileAttributesW( wide.c_str(), attrs ) )
							  return Err( lastError( path, "Set permissions failed" ) );
						  return Ok();
					  } );
}

VFSTask<SpaceInfo> LocalProvider::spaceInfo( const URI& uri, CancellationToken token ) {
	return run<SpaceInfo>(
		uri, std::move( token ), []( const std::string& path ) -> VFSResult<SpaceInfo> {
			ULARGE_INTEGER available, total, free;
			if ( !GetDiskFreeSpaceExW( toWide( path ).c_str(), &available, &total, &free ) )
				return Err( lastError( path, "GetDiskFreeSpaceExW failed" ) );
			return Ok( SpaceInfo{ total.QuadPart, free.QuadPart, available.QuadPart } );
		} );
}

std::string LocalProvider::userName( Uint32 ) {
	return {};
}

std::string LocalProvider::groupName( Uint32 ) {
	return {};
}

std::string LocalProvider::userNameUncached( Uint32 ) {
	return {};
}

std::string LocalProvider::groupNameUncached( Uint32 ) {
	return {};
}

#else

// ---------------------------------------------------------------------------------------------
// POSIX
// ---------------------------------------------------------------------------------------------

namespace {

VFSErrorCode mapErrno( int error ) {
	switch ( error ) {
		case ENOENT:
			return VFSErrorCode::NotFound;
		case EEXIST:
			return VFSErrorCode::AlreadyExists;
		case ENOTDIR:
			return VFSErrorCode::NotDirectory;
		case EISDIR:
			return VFSErrorCode::IsDirectory;
		case EACCES:
		case EPERM:
			return VFSErrorCode::PermissionDenied;
		case EROFS:
			return VFSErrorCode::ReadOnly;
		case ENOTEMPTY:
			return VFSErrorCode::NotEmpty;
		case ELOOP:
			return VFSErrorCode::TooManySymlinks;
		case ENOSPC:
#ifdef EDQUOT
		case EDQUOT:
#endif
			return VFSErrorCode::NoSpace;
		case EXDEV:
		case ENOTSUP:
#if defined( EOPNOTSUPP ) && EOPNOTSUPP != ENOTSUP
		case EOPNOTSUPP:
#endif
			return VFSErrorCode::Unsupported;
		case ENAMETOOLONG:
			return VFSErrorCode::InvalidURI;
		case EINVAL:
			return VFSErrorCode::InvalidOperation;
		case ETIMEDOUT:
			return VFSErrorCode::TimedOut;
		default:
			return VFSErrorCode::IO;
	}
}

VFSError errnoError( const std::string& path, const char* what, int error = errno ) {
	std::string message( what );
	message += ": ";
	message += std::strerror( error );
	return VFSError( mapErrno( error ), URI::fromFSPath( path ), std::move( message ), error );
}

FileType typeFromMode( mode_t mode ) {
	if ( S_ISREG( mode ) )
		return FileType::Regular;
	if ( S_ISDIR( mode ) )
		return FileType::Directory;
	if ( S_ISLNK( mode ) )
		return FileType::Symlink;
	if ( S_ISBLK( mode ) )
		return FileType::BlockDevice;
	if ( S_ISCHR( mode ) )
		return FileType::CharacterDevice;
	if ( S_ISFIFO( mode ) )
		return FileType::FIFO;
	if ( S_ISSOCK( mode ) )
		return FileType::Socket;
	return FileType::Unknown;
}

Uint64 modifiedNanoseconds( const struct stat& st ) {
#if defined( __APPLE__ )
	return static_cast<Uint64>( st.st_mtimespec.tv_sec ) * 1000000000ull + st.st_mtimespec.tv_nsec;
#elif defined( __linux__ ) || defined( __FreeBSD__ ) || defined( __NetBSD__ ) || \
	defined( __OpenBSD__ )
	return static_cast<Uint64>( st.st_mtim.tv_sec ) * 1000000000ull + st.st_mtim.tv_nsec;
#else
	return static_cast<Uint64>( st.st_mtime ) * 1000000000ull;
#endif
}

void fillFromStat( FileInfo& info, const struct stat& st ) {
	info.type = typeFromMode( st.st_mode );
	if ( info.type != FileType::Directory )
		info.setSize( static_cast<Uint64>( st.st_size ) );
	info.setModifiedTime( static_cast<Int64>( st.st_mtime ) );
	info.setAccessedTime( static_cast<Int64>( st.st_atime ) );
#if defined( __APPLE__ ) || defined( __FreeBSD__ ) || defined( __NetBSD__ )
	info.setCreatedTime( static_cast<Int64>( st.st_birthtime ) );
#endif
	info.setPermissions( static_cast<Uint32>( st.st_mode & 07777 ) );
	info.setUid( static_cast<Uint32>( st.st_uid ) );
	info.setGid( static_cast<Uint32>( st.st_gid ) );
	info.revision = FileRevision::fromValues(
		{ static_cast<Uint64>( st.st_dev ), static_cast<Uint64>( st.st_ino ),
		  modifiedNanoseconds( st ), static_cast<Uint64>( st.st_size ) } );
}

std::string readLink( int dirFd, const char* name ) {
	char buffer[4096];
	ssize_t len = dirFd >= 0 ? readlinkat( dirFd, name, buffer, sizeof( buffer ) )
							 : readlink( name, buffer, sizeof( buffer ) );
	if ( len <= 0 )
		return {};
	return std::string( buffer, static_cast<size_t>( len ) );
}

std::mutex sNamesMutex;
std::unordered_map<Uint32, std::string> sUserNames;
std::unordered_map<Uint32, std::string> sGroupNames;

void fillOwnerNames( FileInfo& info ) {
	if ( auto uid = info.uid() )
		info.owner = LocalProvider::userName( *uid );
	if ( auto gid = info.gid() )
		info.group = LocalProvider::groupName( *gid );
}

class LocalDirectoryImpl : public DirectoryHandleImpl,
						   public std::enable_shared_from_this<LocalDirectoryImpl> {
  public:
	LocalDirectoryImpl( std::shared_ptr<ThreadPool> pool, URI uri, std::string path, DIR* dir,
						DirectoryOpenOptions options ) :
		mPool( std::move( pool ) ),
		mUri( std::move( uri ) ),
		mPath( std::move( path ) ),
		mDir( dir ),
		mOptions( options ) {}

	~LocalDirectoryImpl() override { closeNow(); }

	VFSTask<DirectoryBatch> read( size_t maxEntries, CancellationToken token ) override {
		return mPool->submit(
			[self = shared_from_this(), maxEntries, token]() -> VFSResult<DirectoryBatch> {
				return self->readSync( maxEntries, token );
			} );
	}

	VFSTask<void> close() override {
		std::lock_guard<std::mutex> lock( mMutex );
		closeNow();
		return readyValue<void>();
	}

  private:
	VFSResult<DirectoryBatch> readSync( size_t maxEntries, const CancellationToken& token ) {
		std::lock_guard<std::mutex> lock( mMutex );
		DirectoryBatch batch;
		if ( token.isCancellationRequested() )
			return Err( cancelledError( mUri ) );
		if ( !mDir ) {
			batch.endOfDirectory = true;
			return Ok( std::move( batch ) );
		}
		batch.entries.reserve( std::min<size_t>( maxEntries, 1024 ) );
		const int dirFd = dirfd( mDir );
		const bool full = mOptions.metadata == DirectoryMetadata::Full;
		while ( batch.entries.size() < maxEntries ) {
			errno = 0;
			struct dirent* entry = readdir( mDir );
			if ( !entry ) {
				if ( errno != 0 )
					return Err( errnoError( mPath, "readdir failed" ) );
				closeNow();
				batch.endOfDirectory = true;
				break;
			}
			const char* name = entry->d_name;
			if ( name[0] == '.' && ( name[1] == 0 || ( name[1] == '.' && name[2] == 0 ) ) )
				continue;
			// Built in place: a FileInfo is large, moving it into the batch is measurable.
			FileInfo& info = batch.entries.emplace_back();
			info.name = name;
			info.hidden = name[0] == '.';
#ifdef DT_UNKNOWN
			switch ( entry->d_type ) {
				case DT_REG:
					info.type = FileType::Regular;
					break;
				case DT_DIR:
					info.type = FileType::Directory;
					break;
				case DT_LNK:
					info.type = FileType::Symlink;
					break;
				case DT_BLK:
					info.type = FileType::BlockDevice;
					break;
				case DT_CHR:
					info.type = FileType::CharacterDevice;
					break;
				case DT_FIFO:
					info.type = FileType::FIFO;
					break;
				case DT_SOCK:
					info.type = FileType::Socket;
					break;
				default:
					break;
			}
#endif
			if ( full || info.type == FileType::Unknown ) {
				struct stat st;
				if ( fstatat( dirFd, name, &st, AT_SYMLINK_NOFOLLOW ) == 0 ) {
					fillFromStat( info, st );
					if ( full )
						fillOwnerNames( info );
				}
			}
			if ( info.type == FileType::Symlink ) {
				if ( full )
					info.symlinkTarget = readLink( dirFd, name );
				if ( mOptions.resolveSymlinkTargets ) {
					struct stat target;
					if ( fstatat( dirFd, name, &target, 0 ) == 0 )
						info.targetType = typeFromMode( target.st_mode );
				}
			}
		}
		return Ok( std::move( batch ) );
	}

	void closeNow() {
		if ( mDir ) {
			closedir( mDir );
			mDir = nullptr;
		}
	}

	std::shared_ptr<ThreadPool> mPool;
	URI mUri;
	std::string mPath;
	std::mutex mMutex;
	DIR* mDir;
	DirectoryOpenOptions mOptions;
};

class LocalFileImpl : public FileHandleImpl, public std::enable_shared_from_this<LocalFileImpl> {
  public:
	LocalFileImpl( std::shared_ptr<ThreadPool> pool, URI uri, std::string path, int fd,
				   ProviderCapabilities caps ) :
		mPool( std::move( pool ) ),
		mUri( std::move( uri ) ),
		mPath( std::move( path ) ),
		mFd( fd ),
		mCaps( caps ) {}

	~LocalFileImpl() override {
		if ( mFd >= 0 )
			::close( mFd );
	}

	VFSTask<size_t> read( Uint64 offset, std::span<Uint8> data, CancellationToken token ) override {
		return submit<size_t>( token, [offset, data]( LocalFileImpl& self ) -> VFSResult<size_t> {
			size_t done = 0;
			while ( done < data.size() ) {
				ssize_t got = ::pread( self.mFd, data.data() + done, data.size() - done,
									   static_cast<off_t>( offset + done ) );
				if ( got < 0 ) {
					if ( errno == EINTR )
						continue;
					return Err( errnoError( self.mPath, "read failed" ) );
				}
				if ( got == 0 )
					break;
				done += static_cast<size_t>( got );
			}
			return Ok( done );
		} );
	}

	VFSTask<size_t> write( Uint64 offset, std::span<const Uint8> data,
						   CancellationToken token ) override {
		return submit<size_t>( token, [offset, data]( LocalFileImpl& self ) -> VFSResult<size_t> {
			size_t done = 0;
			while ( done < data.size() ) {
				ssize_t put = ::pwrite( self.mFd, data.data() + done, data.size() - done,
										static_cast<off_t>( offset + done ) );
				if ( put < 0 ) {
					if ( errno == EINTR )
						continue;
					return Err( errnoError( self.mPath, "write failed" ) );
				}
				done += static_cast<size_t>( put );
			}
			return Ok( done );
		} );
	}

	VFSTask<void> truncate( Uint64 size ) override {
		return submit<void>( {}, [size]( LocalFileImpl& self ) -> VFSResult<void> {
			if ( ::ftruncate( self.mFd, static_cast<off_t>( size ) ) != 0 )
				return Err( errnoError( self.mPath, "truncate failed" ) );
			return Ok();
		} );
	}

	VFSTask<void> flush() override {
		return submit<void>( {}, []( LocalFileImpl& self ) -> VFSResult<void> {
#if defined( __APPLE__ )
			// fsync() on macOS does not flush the drive cache; F_FULLFSYNC does.
			if ( ::fcntl( self.mFd, F_FULLFSYNC ) == 0 )
				return Ok();
#endif
			if ( ::fsync( self.mFd ) != 0 )
				return Err( errnoError( self.mPath, "fsync failed" ) );
			return Ok();
		} );
	}

	VFSTask<FileInfo> stat() override {
		return submit<FileInfo>( {}, []( LocalFileImpl& self ) -> VFSResult<FileInfo> {
			struct stat st;
			if ( ::fstat( self.mFd, &st ) != 0 )
				return Err( errnoError( self.mPath, "fstat failed" ) );
			FileInfo info;
			info.name = lastSegment( self.mPath );
			info.hidden = !info.name.empty() && info.name[0] == '.';
			fillFromStat( info, st );
			return Ok( std::move( info ) );
		} );
	}

	VFSTask<void> close() override {
		return mPool->submit( [self = shared_from_this()]() -> VFSResult<void> {
			// Waits for in-flight operations on this descriptor.
			std::unique_lock<std::shared_mutex> lock( self->mMutex );
			if ( self->mFd >= 0 ) {
				int result = ::close( self->mFd );
				self->mFd = -1;
				if ( result != 0 && errno != EINTR )
					return Err( errnoError( self->mPath, "close failed" ) );
			}
			return Ok();
		} );
	}

	ProviderCapabilities capabilities() const override { return mCaps; }

  private:
	template <typename T, typename F> VFSTask<T> submit( CancellationToken token, F&& fn ) {
		return mPool->submit( [self = shared_from_this(), fn = std::forward<F>( fn ),
							   token = std::move( token )]() mutable -> VFSResult<T> {
			if ( token.isCancellationRequested() )
				return Err( cancelledError( self->mUri ) );
			std::shared_lock<std::shared_mutex> lock( self->mMutex );
			if ( self->mFd < 0 )
				return Err( VFSError( VFSErrorCode::InvalidOperation, self->mUri, "File closed" ) );
			return fn( *self );
		} );
	}

	std::shared_ptr<ThreadPool> mPool;
	URI mUri;
	std::string mPath;
	std::shared_mutex mMutex;
	int mFd;
	ProviderCapabilities mCaps;
};

} // namespace

std::string LocalProvider::userName( Uint32 uid ) {
	// Directory entries almost always share an owner: a per-thread last-hit cache avoids the
	// shared lock on the listing hot path.
	thread_local std::pair<Uint32, std::string> lastHit{ ~0u, {} };
	if ( lastHit.first == uid )
		return lastHit.second;
	std::string name = userNameUncached( uid );
	lastHit = { uid, name };
	return name;
}

std::string LocalProvider::userNameUncached( Uint32 uid ) {
	{
		std::lock_guard<std::mutex> lock( sNamesMutex );
		auto it = sUserNames.find( uid );
		if ( it != sUserNames.end() )
			return it->second;
	}
	std::string name;
	struct passwd pwd;
	struct passwd* result = nullptr;
	char buffer[4096];
	if ( getpwuid_r( static_cast<uid_t>( uid ), &pwd, buffer, sizeof( buffer ), &result ) == 0 &&
		 result )
		name = result->pw_name;
	std::lock_guard<std::mutex> lock( sNamesMutex );
	sUserNames[uid] = name;
	return name;
}

std::string LocalProvider::groupName( Uint32 gid ) {
	thread_local std::pair<Uint32, std::string> lastHit{ ~0u, {} };
	if ( lastHit.first == gid )
		return lastHit.second;
	std::string name = groupNameUncached( gid );
	lastHit = { gid, name };
	return name;
}

std::string LocalProvider::groupNameUncached( Uint32 gid ) {
	{
		std::lock_guard<std::mutex> lock( sNamesMutex );
		auto it = sGroupNames.find( gid );
		if ( it != sGroupNames.end() )
			return it->second;
	}
	std::string name;
	struct group grp;
	struct group* result = nullptr;
	char buffer[8192];
	if ( getgrgid_r( static_cast<gid_t>( gid ), &grp, buffer, sizeof( buffer ), &result ) == 0 &&
		 result )
		name = result->gr_name;
	std::lock_guard<std::mutex> lock( sNamesMutex );
	sGroupNames[gid] = name;
	return name;
}

VFSResult<FileInfo> LocalProvider::statSync( const URI& uri, const std::string& path,
											 bool followSymlinks ) {
	struct stat st;
	if ( ::lstat( path.c_str(), &st ) != 0 )
		return Err( errnoError( path, "stat failed" ) );
	FileInfo info;
	info.name = lastSegment( path );
	info.hidden = !info.name.empty() && info.name[0] == '.';
	if ( S_ISLNK( st.st_mode ) ) {
		info.symlinkTarget = readLink( -1, path.c_str() );
		struct stat target;
		bool resolved = ::stat( path.c_str(), &target ) == 0;
		if ( followSymlinks ) {
			if ( !resolved )
				return Err( errnoError( path, "stat failed" ) );
			fillFromStat( info, target );
		} else {
			fillFromStat( info, st );
			if ( resolved )
				info.targetType = typeFromMode( target.st_mode );
		}
	} else {
		fillFromStat( info, st );
	}
	fillOwnerNames( info );
	return Ok( std::move( info ) );
}

VFSTask<FileInfo> LocalProvider::stat( const URI& uri, const StatOptions& options,
									   CancellationToken token ) {
	return run<FileInfo>( uri, std::move( token ),
						  [uri, follow = options.followSymlinks]( const std::string& path ) {
							  return statSync( uri, path, follow );
						  } );
}

VFSTask<DirectoryHandle> LocalProvider::openDirectory( const URI& uri,
													   const DirectoryOpenOptions& options,
													   CancellationToken token ) {
	return run<DirectoryHandle>(
		uri, std::move( token ),
		[pool = mPool, uri, options]( const std::string& path ) -> VFSResult<DirectoryHandle> {
			DIR* dir = ::opendir( path.c_str() );
			if ( !dir )
				return Err( errnoError( path, "opendir failed" ) );
			return Ok( DirectoryHandle(
				uri, std::make_shared<LocalDirectoryImpl>( pool, uri, path, dir, options ) ) );
		} );
}

VFSTask<FileHandle> LocalProvider::open( const URI& uri, const OpenOptions& options,
										 CancellationToken token ) {
	return run<FileHandle>(
		uri, std::move( token ),
		[pool = mPool, uri, options,
		 caps = capabilities( uri )]( const std::string& path ) -> VFSResult<FileHandle> {
			OpenFlags flags = options.flags;
			int oflags = O_CLOEXEC;
			bool canRead = hasFlag( flags, OpenFlags::Read );
			bool canWrite = hasFlag( flags, OpenFlags::Write );
			oflags |= canRead && canWrite ? O_RDWR : ( canWrite ? O_WRONLY : O_RDONLY );
			if ( hasFlag( flags, OpenFlags::Create ) )
				oflags |= O_CREAT;
			if ( hasFlag( flags, OpenFlags::Exclusive ) )
				oflags |= O_EXCL;
			if ( hasFlag( flags, OpenFlags::Truncate ) )
				oflags |= O_TRUNC;
			mode_t mode = static_cast<mode_t>( options.createPermissions.value_or( 0666 ) );
			int fd;
			do {
				fd = ::open( path.c_str(), oflags, mode );
			} while ( fd < 0 && errno == EINTR );
			if ( fd < 0 )
				return Err( errnoError( path, "open failed" ) );
			struct stat st;
			if ( ::fstat( fd, &st ) == 0 && S_ISDIR( st.st_mode ) ) {
				::close( fd );
				return Err( VFSError( VFSErrorCode::IsDirectory, uri ) );
			}
			return Ok(
				FileHandle( uri, std::make_shared<LocalFileImpl>( pool, uri, path, fd, caps ) ) );
		} );
}

VFSTask<void> LocalProvider::mkdir( const URI& uri, const MkDirOptions& options,
									CancellationToken token ) {
	return run<void>( uri, std::move( token ),
					  [mode = options.permissions.value_or( 0777 )](
						  const std::string& path ) -> VFSResult<void> {
						  if ( ::mkdir( path.c_str(), static_cast<mode_t>( mode ) ) != 0 )
							  return Err( errnoError( path, "mkdir failed" ) );
						  return Ok();
					  } );
}

VFSTask<void> LocalProvider::remove( const URI& uri, const RemoveOptions&,
									 CancellationToken token ) {
	return run<void>( uri, std::move( token ), []( const std::string& path ) -> VFSResult<void> {
		struct stat st;
		if ( ::lstat( path.c_str(), &st ) != 0 )
			return Err( errnoError( path, "remove failed" ) );
		int result = S_ISDIR( st.st_mode ) ? ::rmdir( path.c_str() ) : ::unlink( path.c_str() );
		if ( result != 0 )
			return Err( errnoError( path, "remove failed" ) );
		return Ok();
	} );
}

namespace {

/** rename() that fails with EEXIST instead of replacing the destination. */
int renameNoReplace( const char* from, const char* to ) {
#if defined( __linux__ ) && defined( SYS_renameat2 )
	int result = static_cast<int>(
		::syscall( SYS_renameat2, AT_FDCWD, from, AT_FDCWD, to, RENAME_NOREPLACE ) );
	if ( result == 0 || ( errno != ENOSYS && errno != EINVAL ) )
		return result;
#elif defined( __APPLE__ ) && defined( RENAME_EXCL )
	return ::renamex_np( from, to, RENAME_EXCL );
#endif
	// Fallback for filesystems/kernels without an atomic no-replace rename. There is a small race
	// window between the check and the rename.
	struct stat st;
	if ( ::lstat( to, &st ) == 0 ) {
		errno = EEXIST;
		return -1;
	}
	return ::rename( from, to );
}

} // namespace

VFSTask<void> LocalProvider::rename( const URI& from, const URI& to, const RenameOptions& options,
									 CancellationToken token ) {
	auto toPath = toNativePath( to );
	if ( !toPath )
		return readyError<void>( std::move( toPath ).error() );
	return run<void>( from, std::move( token ),
					  [to = std::move( toPath ).value(), overwrite = options.overwrite](
						  const std::string& path ) -> VFSResult<void> {
						  int result = overwrite ? ::rename( path.c_str(), to.c_str() )
												 : renameNoReplace( path.c_str(), to.c_str() );
						  if ( result != 0 )
							  return Err( errnoError( path, "rename failed" ) );
						  return Ok();
					  } );
}

VFSTask<URI> LocalProvider::canonicalize( const URI& uri, CancellationToken token ) {
	return run<URI>( uri, std::move( token ), []( const std::string& path ) -> VFSResult<URI> {
		char* resolved = ::realpath( path.c_str(), nullptr );
		if ( !resolved )
			return Err( errnoError( path, "realpath failed" ) );
		URI result = URI::fromFSPath( resolved );
		::free( resolved );
		return Ok( std::move( result ) );
	} );
}

VFSTask<void> LocalProvider::setPermissions( const URI& uri, Uint32 permissions,
											 CancellationToken token ) {
	return run<void>(
		uri, std::move( token ), [permissions]( const std::string& path ) -> VFSResult<void> {
			if ( ::chmod( path.c_str(), static_cast<mode_t>( permissions & 07777 ) ) != 0 )
				return Err( errnoError( path, "chmod failed" ) );
			return Ok();
		} );
}

VFSTask<SpaceInfo> LocalProvider::spaceInfo( const URI& uri, CancellationToken token ) {
	return run<SpaceInfo>(
		uri, std::move( token ), []( const std::string& path ) -> VFSResult<SpaceInfo> {
			struct statvfs st;
			if ( ::statvfs( path.c_str(), &st ) != 0 )
				return Err( errnoError( path, "statvfs failed" ) );
			Uint64 unit = st.f_frsize ? st.f_frsize : st.f_bsize;
			return Ok( SpaceInfo{ unit * st.f_blocks, unit * st.f_bfree, unit * st.f_bavail } );
		} );
}

#endif

}} // namespace EE::VFS
