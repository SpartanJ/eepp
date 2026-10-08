#ifndef ECODE_FILESYSTEMLISTENEROPTIONS_HPP
#define ECODE_FILESYSTEMLISTENEROPTIONS_HPP

#include <eepp/core.hpp>
#include <eepp/ui/models/filesystemmodel.hpp>

namespace ecode {

using namespace EE::UI::Models;
using FileEventTypeMask = EE::Uint32;

enum class FileEventThreadAffinity { Main, Worker };
enum class FileEventPathMatch { Prefix, Exact };

static constexpr FileEventTypeMask fileEventTypeMask( FileSystemEventType type ) {
	return 1u << static_cast<EE::Uint32>( type );
}

struct FileSystemListenerFilter {
	FileEventTypeMask eventTypes{ 0xFFFFFFFFu };
	std::string path;
	FileEventPathMatch pathMatch{ FileEventPathMatch::Prefix };

	bool matches( FileSystemEventType type, const std::string& filePath ) const {
		if ( ( eventTypes & fileEventTypeMask( type ) ) == 0 )
			return false;
		if ( path.empty() )
			return true;
		return pathMatch == FileEventPathMatch::Exact
				   ? filePath == path
				   : filePath.compare( 0, path.size(), path ) == 0;
	}

	bool matchesJoinedPath( FileSystemEventType type, const std::string& directory, char separator,
							const std::string& filename ) const {
		if ( ( eventTypes & fileEventTypeMask( type ) ) == 0 )
			return false;
		if ( path.empty() )
			return true;
		const std::size_t joinedSize =
			directory.size() + ( separator != 0 ? 1 : 0 ) + filename.size();
		if ( path.size() > joinedSize ||
			 ( pathMatch == FileEventPathMatch::Exact && path.size() != joinedSize ) )
			return false;

		std::size_t pathPos = 0;
		auto matchesPart = [&]( const char* data, std::size_t size ) {
			const std::size_t count = std::min( size, path.size() - pathPos );
			if ( count != 0 && path.compare( pathPos, count, data, count ) != 0 )
				return false;
			pathPos += count;
			return true;
		};
		if ( !matchesPart( directory.data(), directory.size() ) )
			return false;
		if ( separator != 0 && pathPos < path.size() ) {
			if ( !matchesPart( &separator, 1 ) )
				return false;
		}
		return pathPos == path.size() || matchesPart( filename.data(), filename.size() );
	}
};

struct FileSystemListenerOptions {
	std::vector<FileSystemListenerFilter> filters;
	FileEventThreadAffinity affinity{ FileEventThreadAffinity::Main };

	bool matches( FileSystemEventType type, const std::string& filePath ) const {
		if ( filters.empty() )
			return true;
		for ( const auto& filter : filters ) {
			if ( filter.matches( type, filePath ) )
				return true;
		}
		return false;
	}

	bool matchesJoinedPath( FileSystemEventType type, const std::string& directory, char separator,
							const std::string& filename ) const {
		if ( filters.empty() )
			return true;
		for ( const auto& filter : filters ) {
			if ( filter.matchesJoinedPath( type, directory, separator, filename ) )
				return true;
		}
		return false;
	}
};

} // namespace ecode

#endif // ECODE_FILESYSTEMLISTENEROPTIONS_HPP
