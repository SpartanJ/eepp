#ifndef EE_SYSTEM_FILELOCK_HPP
#define EE_SYSTEM_FILELOCK_HPP

#include <eepp/core/noncopyable.hpp>
#include <string>

namespace EE { namespace System {

/** @brief Owns an exclusive, nonblocking OS lock for coordination between processes.
 *
 * Cooperating processes must open the same file. Use a dedicated lock file and keep it in place
 * while any process might use it. The file is left on disk after the lock is released.
 * Windows locks the first byte of that file. POSIX platforms use an advisory file lock.
 */
class EE_API FileLock : NonCopyable {
  public:
	enum class Result { Acquired, Busy, Error };

	FileLock() = default;

	/** Releases the lock and closes the native handle. */
	~FileLock();

	/** @brief Opens or creates a lock file and tries to lock it without waiting for another owner.
	 * @param path UTF-8 path to the lock file in an existing directory.
	 * @return Busy when another handle owns the lock, or Error when the path or platform
	 * prevents acquisition. If this object already owns a lock, returns Error and retains it. */
	Result tryLock( const std::string& path );

  private:
#if EE_PLATFORM == EE_PLATFORM_WIN
	void* mHandle{ nullptr };
#elif defined( EE_PLATFORM_POSIX ) && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	int mHandle{ -1 };
#endif
};

}} // namespace EE::System

#endif
