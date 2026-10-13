#include <eepp/vfs/error.hpp>
#include <eepp/vfs/fileinfo.hpp>

namespace EE { namespace VFS {

const char* toString( VFSErrorCode code ) {
	switch ( code ) {
		case VFSErrorCode::None:
			return "None";
		case VFSErrorCode::NotFound:
			return "NotFound";
		case VFSErrorCode::AlreadyExists:
			return "AlreadyExists";
		case VFSErrorCode::NotDirectory:
			return "NotDirectory";
		case VFSErrorCode::IsDirectory:
			return "IsDirectory";
		case VFSErrorCode::PermissionDenied:
			return "PermissionDenied";
		case VFSErrorCode::ReadOnly:
			return "ReadOnly";
		case VFSErrorCode::InvalidURI:
			return "InvalidURI";
		case VFSErrorCode::Unsupported:
			return "Unsupported";
		case VFSErrorCode::InvalidOperation:
			return "InvalidOperation";
		case VFSErrorCode::NotEmpty:
			return "NotEmpty";
		case VFSErrorCode::TooManySymlinks:
			return "TooManySymlinks";
		case VFSErrorCode::ChangedExternally:
			return "ChangedExternally";
		case VFSErrorCode::ConnectionLost:
			return "ConnectionLost";
		case VFSErrorCode::AuthenticationFailed:
			return "AuthenticationFailed";
		case VFSErrorCode::HostKeyRejected:
			return "HostKeyRejected";
		case VFSErrorCode::TimedOut:
			return "TimedOut";
		case VFSErrorCode::Cancelled:
			return "Cancelled";
		case VFSErrorCode::ProtocolError:
			return "ProtocolError";
		case VFSErrorCode::NoSpace:
			return "NoSpace";
		case VFSErrorCode::IO:
			return "IO";
		case VFSErrorCode::Unknown:
			break;
	}
	return "Unknown";
}

std::string VFSError::toString() const {
	std::string str( VFS::toString( code ) );
	if ( !message.empty() ) {
		str += ": ";
		str += message;
	}
	if ( !uri.empty() ) {
		str += " (";
		str += uri.toString();
		str += ')';
	}
	return str;
}

const char* toString( FileType type ) {
	switch ( type ) {
		case FileType::Regular:
			return "Regular";
		case FileType::Directory:
			return "Directory";
		case FileType::Symlink:
			return "Symlink";
		case FileType::BlockDevice:
			return "BlockDevice";
		case FileType::CharacterDevice:
			return "CharacterDevice";
		case FileType::FIFO:
			return "FIFO";
		case FileType::Socket:
			return "Socket";
		case FileType::Unknown:
			break;
	}
	return "Unknown";
}

std::string FileRevision::toHexString() const {
	static constexpr char Hex[] = "0123456789abcdef";
	std::string str;
	str.reserve( mSize * 2 );
	for ( size_t i = 0; i < mSize; ++i ) {
		str += Hex[mData[i] >> 4];
		str += Hex[mData[i] & 0xF];
	}
	return str;
}

std::optional<URI> FileInfo::symlinkTargetURI( const URI& linkURI ) const {
	if ( symlinkTarget.empty() )
		return std::nullopt;
	std::string target( symlinkTarget );
	bool absolute = target[0] == '/';
#if EE_PLATFORM == EE_PLATFORM_WIN
	for ( auto& ch : target )
		if ( ch == '\\' )
			ch = '/';
	if ( linkURI.getScheme() == "file" && target.size() >= 2 && target[1] == ':' ) {
		target.insert( target.begin(), '/' );
		absolute = true;
	}
#endif
	URI resolved( absolute ? linkURI : linkURI.getParent() );
	resolved.setQuery( "" );
	resolved.setFragment( "" );
	// Join textually so names containing URI delimiters ('#', '?', '%') stay verbatim, then
	// collapse "." and ".." syntactically (no filesystem access).
	std::string path( absolute ? std::string() : resolved.getPath() );
	if ( !path.empty() && path.back() != '/' )
		path += '/';
	path += absolute ? target.substr( 1 ) : target;
	std::vector<std::string_view> out;
	size_t start = 0;
	while ( start <= path.size() ) {
		size_t end = path.find( '/', start );
		if ( end == std::string::npos )
			end = path.size();
		std::string_view seg( path.data() + start, end - start );
		if ( seg == ".." ) {
			if ( !out.empty() )
				out.pop_back();
		} else if ( !seg.empty() && seg != "." ) {
			out.push_back( seg );
		}
		start = end + 1;
	}
	std::string normalized;
	for ( const auto& seg : out ) {
		normalized += '/';
		normalized.append( seg );
	}
	if ( normalized.empty() )
		normalized = "/";
	resolved.setRawPath( std::move( normalized ) );
	return resolved;
}

}} // namespace EE::VFS
