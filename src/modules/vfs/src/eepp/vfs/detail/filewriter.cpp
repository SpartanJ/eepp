#include <algorithm>
#include <atomic>
#include <eepp/core/string.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/vfs/detail/filewriter.hpp>
#include <vector>

namespace EE { namespace VFS { namespace detail {

std::string FileWriter::temporarySiblingName( std::string_view name ) {
	static std::atomic<Uint32> sCounter{ 0 };
	std::string tmp( "." );
	tmp.append( name );
	tmp += ".eepp-";
	tmp += String::toString( static_cast<Uint64>( System::Sys::getProcessID() ) );
	tmp += '-';
	tmp += String::toString( static_cast<Uint64>( System::Sys::getTicks() ) );
	tmp += '-';
	tmp += String::toString( ++sCounter );
	tmp += ".tmp";
	return tmp;
}

VFSResult<void> FileWriter::writeFully( FileHandle& handle, std::span<const Uint8> data,
										const CancellationToken& token, Uint64 offset ) {
	size_t done = 0;
	while ( done < data.size() ) {
		size_t chunk = std::min<size_t>( IOChunkSize, data.size() - done );
		auto written = await( handle.write( offset + done, data.subspan( done, chunk ), token ) );
		if ( !written )
			return Err( std::move( written ).error() );
		if ( written.value() == 0 )
			return Err( VFSError( VFSErrorCode::IO, handle.uri(), "Short write" ) );
		done += written.value();
	}
	return Ok();
}

VFSResult<void> FileWriter::makeDirs( VFSProvider& provider, const URI& uri,
									  const MkDirOptions& options,
									  const CancellationToken& token ) {
	std::vector<URI> missing;
	URI current( uri );
	while ( true ) {
		auto info = await( provider.stat( current, StatOptions{}, token ) );
		if ( info ) {
			if ( !info.value().isDirectory() )
				return Err( VFSError( VFSErrorCode::NotDirectory, current ) );
			break;
		}
		if ( !info.error().is( VFSErrorCode::NotFound ) )
			return Err( std::move( info ).error() );
		missing.push_back( current );
		URI parent( current.getParent() );
		if ( parent.getPath() == current.getPath() )
			break;
		current = std::move( parent );
	}
	for ( auto it = missing.rbegin(); it != missing.rend(); ++it ) {
		auto made = await( provider.mkdir( *it, options, token ) );
		if ( !made ) {
			if ( !made.error().is( VFSErrorCode::AlreadyExists ) )
				return made;
			// Lost a race with another creator: fine if it is a directory.
			auto info = await( provider.stat( *it, StatOptions{}, token ) );
			if ( !info || !info.value().isDirectory() )
				return made;
		}
	}
	return Ok();
}

VFSResult<URI> FileWriter::resolveTarget( VFSProvider& provider, const URI& uri,
										  const CancellationToken& token ) {
	auto linkInfo = await( provider.stat( uri, StatOptions{ false }, token ) );
	if ( !linkInfo ) {
		if ( linkInfo.error().is( VFSErrorCode::NotFound ) )
			return Ok( URI( uri ) );
		return Err( std::move( linkInfo ).error() );
	}
	if ( !linkInfo.value().isSymlink() )
		return Ok( URI( uri ) );
	auto resolved = await( provider.canonicalize( uri, token ) );
	// A dangling link is replaced by the new file.
	if ( !resolved && resolved.error().is( VFSErrorCode::NotFound ) )
		return Ok( URI( uri ) );
	return resolved;
}

}}} // namespace EE::VFS::detail
