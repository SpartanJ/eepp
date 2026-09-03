#ifndef ETERM_TERMINALGRAPHICS_HPP
#define ETERM_TERMINALGRAPHICS_HPP

#include <deque>
#include <eepp/math/rect.hpp>
#include <eepp/math/size.hpp>
#include <eepp/math/vector2.hpp>
#include <memory>
#include <mutex>
#include <vector>

using namespace EE;
using namespace EE::Math;

namespace eterm { namespace Terminal {

using KittyImageId = Uint32;
using KittyPlacementId = Uint32;

struct TerminalVisiblePlacement {
	KittyImageId imageId{ 0 };
	KittyPlacementId placementId{ 0 };
	Uint32 frameNumber{ 1 };
	Vector2i visibleAnchorCell;
	Rect sourcePixels;
	Uint32 columns{ 0 };
	Uint32 rows{ 0 };
	Vector2i firstCellPixelOffset;
	Int32 zIndex{ 0 };
};

struct TerminalGraphicsPlaceholderCell {
	KittyImageId imageId{ 0 };
	KittyPlacementId placementId{ 0 };
	Vector2i cell;
	Uint32 imageRow{ 0 };
	Uint32 imageColumn{ 0 };

	bool operator==( const TerminalGraphicsPlaceholderCell& other ) const {
		return imageId == other.imageId && placementId == other.placementId && cell == other.cell &&
			   imageRow == other.imageRow && imageColumn == other.imageColumn;
	}
};

/** Small immutable graphics state associated with a terminal presentation. */
struct TerminalGraphicsPresentation {
	std::vector<TerminalVisiblePlacement> placements;
	Uint64 generation{ 0 };
	Uint64 requiredUpdateSequence{ 0 };
};

enum class TerminalGraphicsUpdateType : Uint8 {
	CreateImage,
	ReplaceImage,
	UpdateRegion,
	CreateFrame,
	ReplaceFrame,
	UpdateFrameRegion,
	DeleteFrame,
	DeleteImage,
	ResetScreen,
	ResetAll,
	Resync
};

struct TerminalGraphicsUpdate {
	std::shared_ptr<const std::vector<Uint8>> rgba;
	Sizei imageSize;
	Rect region;
	Uint64 sequence{ 0 };
	KittyImageId imageId{ 0 };
	Uint32 frameNumber{ 1 };
	TerminalGraphicsUpdateType type{ TerminalGraphicsUpdateType::Resync };

	size_t payloadBytes() const { return rgba ? rgba->size() : 0; }
};

/** Bounded ordered worker-to-UI mutation queue with explicit overflow recovery. */
class TerminalGraphicsUpdateQueue {
  public:
	static constexpr size_t DefaultMaxUpdates = 1024;
	static constexpr size_t DefaultMaxBytes = 32 * 1024 * 1024;

	explicit TerminalGraphicsUpdateQueue( size_t maxUpdates = DefaultMaxUpdates,
										  size_t maxBytes = DefaultMaxBytes );

	Uint64 enqueue( TerminalGraphicsUpdate update );

	std::vector<TerminalGraphicsUpdate> drain();

	size_t queuedBytes() const;

	bool needsResync() const;

	void resetResync();

  private:
	mutable std::mutex mMutex;
	std::deque<TerminalGraphicsUpdate> mUpdates;
	size_t mMaxUpdates{ 0 };
	size_t mMaxBytes{ 0 };
	size_t mQueuedBytes{ 0 };
	Uint64 mNextSequence{ 0 };
	bool mNeedsResync{ false };
};

}} // namespace eterm::Terminal

#endif
