#ifndef EE_SYSTEM_LOGFILEFAMILY_HPP
#define EE_SYSTEM_LOGFILEFAMILY_HPP

#include <eepp/system/filelock.hpp>
#include <string>

namespace EE { namespace System {

/** @brief Holds exclusive writer ownership of a log file and its numbered rotation archives.
 *
 * Cooperating processes pass the same requested path to acquire(). The first owner uses that
 * path. Further owners use the lowest available numbered slot, for example `app.instance-2.log`
 * for a requested `app.log`. There are 32 slots including the original path.
 *
 * Ownership uses a nonblocking OS lock on a sidecar such as `app.log.lock`. Each numbered family
 * has its own sidecar. Leave sidecars in place so every process locks the same file. The OS
 * releases the lock when the process exits, including after abnormal termination.
 *
 * Acquire ownership even for an append-only writer if another process may rotate that family.
 * Keep the LogFileFamily alive until the log writer has flushed and closed its file.
 *
 * @code
 * LogFileFamily family;
 * const auto& path = family.acquire( "app.log" );
 * Log::create( path, LogLevel::Info, path.empty(), !path.empty(), { 5 * 1024 * 1024, 3 } );
 * Log::info( "Application started" );
 * Log::destroySingleton();
 * @endcode
 */
class EE_API LogFileFamily {
  public:
	/** Creates an owner with no lock held. */
	LogFileFamily() = default;

	/** Releases the lock and leaves the sidecar on disk for reuse. */
	~LogFileFamily();

	/** @brief Selects and locks a file family without opening the active log.
	 * @param logPath Preferred UTF-8 path to the active log in an existing directory.
	 * @return A reference to the selected active path, valid for the lifetime of this owner.
	 * The result is empty if all slots are occupied or a lock cannot be established. After
	 * success, subsequent calls on this owner return the same path. */
	const std::string& acquire( const std::string& logPath );

  private:
	FileLock mLock;
	std::string mPath;
};

}} // namespace EE::System

#endif
