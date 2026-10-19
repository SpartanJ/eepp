#ifndef EE_VFS_SFTP_SSHIOCONTEXT_HPP
#define EE_VFS_SFTP_SSHIOCONTEXT_HPP

#include "sshwakesignal.hpp"
#include <atomic>
#include <eepp/core/small_unique_function.hpp>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace EE { namespace VFS { namespace sftp {

class SSHConnection;

/** @brief Single I/O thread that owns and drives SSH sessions.
 *
 * Every libssh2 call of a registered connection happens on this thread. Other threads interact
 * only through post(), which queues a command and wakes the poll loop.
 */
class SSHIOContext : public std::enable_shared_from_this<SSHIOContext> {
  public:
	using Command = SmallUniqueFunction<void(), 64>;

	SSHIOContext();

	~SSHIOContext();

	void start();

	void stop();

	void post( Command command );

	void wake() { mWake.signal(); }

	bool isIOThread() const { return std::this_thread::get_id() == mThreadId; }

	/** Registers a connection (I/O thread only). */
	void addConnection( std::shared_ptr<SSHConnection> connection );

	/** Fails registered sessions, including sessions still awaiting authentication. */
	void closeConnections();

  private:
	void loop();

	SSHWakeSignal mWake;
	std::thread mThread;
	std::thread::id mThreadId;
	std::mutex mMutex;
	std::vector<Command> mCommands;
	std::vector<std::shared_ptr<SSHConnection>> mConnections;
	std::atomic<bool> mStopping{ false };
};

}}} // namespace EE::VFS::sftp

#endif
