#ifndef EE_WINDOW_TERMINAL_KITTYFRAMEPRESENTER_HPP
#define EE_WINDOW_TERMINAL_KITTYFRAMEPRESENTER_HPP

#include <condition_variable>
#include <eepp/window/framepresenter.hpp>
#include <mutex>
#include <thread>
#include <vector>

namespace EE { namespace Window {
/** Presents completed window frames using the Kitty graphics protocol.
 *
 * Presentation policy can be configured before initialization with
 * `EEPP_TERMINAL_DAMAGE_UPDATES`, `EEPP_TERMINAL_PERSISTENT_UPDATES`, and
 * `EEPP_TERMINAL_ZLIB_LEVEL`. Damage and persistent updates default to enabled. The zlib level
 * defaults to 1, accepts levels 1 through 9, and can be set to 0 to disable compression. Disabling
 * persistent updates also disables damage updates because rectangles require a persistent image.
 */
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
	struct DamageRectangle {
		Int32 x;
		Int32 y;
		Int32 width;
		Int32 height;
	};
	std::mutex mMutex;
	std::condition_variable mCondition;
	std::thread mWorker;
	Frame mPending;
	Frame mRecycle;
	Frame mPresented;
	std::vector<DamageRectangle> mDamageRectangles;
	std::vector<Uint8> mTransferPixels;
	std::vector<Uint8> mCompressedPixels;
	int mZlibCompressionLevel{ 1 };
	bool mRunning{ false };
	bool mHasPending{ false };
	bool mDamageUpdatesEnabled{ true };
	bool mPersistentUpdatesEnabled{ true };
	/** Consumes the bounded newest-frame queue until shutdown. */
	void run();
	/** Encodes and writes one RGB24 frame using chunked Kitty direct transmission. */
	bool sendFrame( const Frame& );
	/** Sends an initial image or root-image rectangle update, using compression when beneficial. */
	bool sendTransfer( const std::vector<Uint8>& pixels, const DamageRectangle& rectangle,
					   bool initial );
	/** Packs a bottom-up framebuffer rectangle into top-down Kitty row order. */
	void extractRectangle( const Frame& frame, const DamageRectangle& rectangle );
};

}} // namespace EE::Window

#endif
