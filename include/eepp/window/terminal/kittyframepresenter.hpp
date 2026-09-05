#ifndef EE_WINDOW_TERMINAL_KITTYFRAMEPRESENTER_HPP
#define EE_WINDOW_TERMINAL_KITTYFRAMEPRESENTER_HPP

#include <condition_variable>
#include <eepp/window/framepresenter.hpp>
#include <mutex>
#include <thread>
#include <vector>

namespace EE { namespace Window {
/** Presents completed window frames using the Kitty graphics protocol. */
class KittyFramePresenter final : public FramePresenter {
  public:
	~KittyFramePresenter();

	/** Initializes the terminal transport and sender worker. */
	bool initialize( Window& ) override;

	/** Captures and queues the newest complete RGB24 frame. */
	void present( Window& ) override;

	/** Stops the sender worker and restores the terminal transport. */
	void shutdown( Window& ) override;

  private:
	struct Frame {
		std::vector<Uint8> pixels;
		Math::Sizei size;
	};
	std::mutex mMutex;
	std::condition_variable mCondition;
	std::thread mWorker;
	Frame mPending;
	Frame mRecycle;
	bool mRunning{ false };
	bool mHasPending{ false };
	/** Consumes the bounded newest-frame queue until shutdown. */
	void run();
	/** Encodes and writes one RGB24 frame using chunked Kitty direct transmission. */
	void sendFrame( const Frame& );
};

}} // namespace EE::Window

#endif
