#include <cstdio>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/logfilefamily.hpp>
#include <utility>

namespace EE { namespace System {

LogFileFamily::~LogFileFamily() = default;

const std::string& LogFileFamily::acquire( const std::string& logPath ) {
	if ( !mPath.empty() || logPath.empty() )
		return mPath;

	// Keep each sidecar in place so cooperating processes always lock the same file.
	const auto [name, extension] =
		FileSystem::fileNameSplitExtension( FileSystem::fileNameFromPath( logPath ) );
	if ( name.empty() )
		return mPath;
	const auto slotPosition = logPath.size() - extension.size();
	constexpr unsigned MaxSlots = 32;
	for ( unsigned slot = 1; slot <= MaxSlots; ++slot ) {
		auto path = logPath;
		if ( slot > 1 )
			path.insert( slotPosition, ".instance-" + std::to_string( slot ) );
		switch ( mLock.tryLock( path + ".lock" ) ) {
			case FileLock::Result::Acquired:
				mPath = std::move( path );
				return mPath;
			case FileLock::Result::Busy:
				break;
			case FileLock::Result::Error:
				std::fputs( "eepp log: could not establish file log ownership\n", stderr );
				return mPath;
		}
	}
	std::fputs( "eepp log: all file log slots are occupied\n", stderr );
	return mPath;
}

}} // namespace EE::System
