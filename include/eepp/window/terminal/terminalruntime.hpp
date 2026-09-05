#ifndef EE_WINDOW_TERMINAL_TERMINALRUNTIME_HPP
#define EE_WINDOW_TERMINAL_TERMINALRUNTIME_HPP

#include <atomic>
#include <eepp/math/size.hpp>
#include <eepp/window/base.hpp>
#include <mutex>
#include <thread>

namespace EE { namespace Window {
class Window;

/** Process-level terminal transport and input runtime.
 *
 * Owns terminal mode negotiation, input parsing, synchronized protocol output, and attachment to
 * the single top-level Window currently supported by terminal mode.
 */
class EE_API TerminalRuntime {
  public:
	/** @return The process terminal runtime instance. */
	static TerminalRuntime& instance();

	~TerminalRuntime();

	/** Opens the controlling terminal, enables terminal protocols, and starts input processing.
	 * @return True if the terminal transport was initialized or was already initialized.
	 */
	bool initialize();

	/** Stops input processing and restores the terminal state. */
	void shutdown();

	/** Writes a complete protocol fragment to the terminal without interleaving other writers.
	 * @return True if all bytes were written.
	 */
	bool write( const char* data, size_t size );

	/** @return The terminal viewport size in pixels, or an empty size when unavailable. */
	Math::Sizei pixelSize() const;

	/** Attaches the top-level window that receives decoded terminal input and resize events. */
	void attach( Window& window );

	/** Detaches the currently attached window. */
	void detach();

	/** @return Whether the host terminal currently reports input focus. */
	bool isFocused() const { return mFocused.load( std::memory_order_relaxed ); }

  private:
	TerminalRuntime() = default;

	TerminalRuntime( const TerminalRuntime& ) = delete;

	TerminalRuntime& operator=( const TerminalRuntime& ) = delete;

	struct State;
	State* mState{ nullptr };
	std::mutex mWriteMutex;
	std::thread mInputThread;
	std::atomic<bool> mReading{ false };
	std::atomic<bool> mFocused{ true };
	Window* mWindow{ nullptr };
#if defined( EE_PLATFORM_POSIX )
	int mFd{ -1 };
#endif

	/** Reads terminal protocol input and enqueues translated neutral InputEvents. */
	void readInput();
};

}} // namespace EE::Window

#endif
