#include <eepp/vfs/error.hpp>

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

}} // namespace EE::VFS
