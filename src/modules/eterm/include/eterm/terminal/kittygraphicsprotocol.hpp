#ifndef ETERM_KITTYGRAPHICSPROTOCOL_HPP
#define ETERM_KITTYGRAPHICSPROTOCOL_HPP

#include <eepp/config.hpp>
#include <eepp/system/clock.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <eterm/terminal/terminalgraphics.hpp>

using namespace EE;

namespace eterm { namespace Terminal {

enum class KittyGraphicsError : Uint8 {
	None,
	InvalidArgument,
	Unsupported,
	InvalidData,
	TooLarge,
	DecodeFailed,
	NotFound,
	NoSpace,
	NoParent,
	Cycle,
	TooDeep
};

struct KittyGraphicsCommandData {
	std::string_view payload;
	std::optional<Uint32> format;
	std::optional<Uint32> dataSize;
	std::optional<Uint32> dataOffset;
	std::optional<Uint32> more;
	std::optional<Uint32> imageId;
	std::optional<Uint32> imageNumber;
	std::optional<Uint32> usageHint;
	std::optional<Uint32> placementId;
	std::optional<Uint32> quiet;
	std::optional<Uint32> width;
	std::optional<Uint32> height;
	std::optional<Uint32> x;
	std::optional<Uint32> y;
	std::optional<Uint32> sourceWidth;
	std::optional<Uint32> sourceHeight;
	std::optional<Uint32> columns;
	std::optional<Uint32> rows;
	std::optional<Uint32> xOffset;
	std::optional<Uint32> yOffset;
	std::optional<Int32> zIndex;
	std::optional<Uint32> cursorMovement;
	std::optional<Uint32> virtualPlacement;
	std::optional<Uint32> parentImageId;
	std::optional<Uint32> parentPlacementId;
	std::optional<Int32> parentOffsetX;
	std::optional<Int32> parentOffsetY;
	char transmission{ 'd' };
	char compression{ 0 };
	char deletion{ 'a' };
};

struct KittyTransmitCommand {
	KittyGraphicsCommandData data;
	bool display{ false };
};

struct KittyPutCommand {
	KittyGraphicsCommandData data;
};

struct KittyDeleteCommand {
	KittyGraphicsCommandData data;
};

struct KittyFrameCommand {
	KittyGraphicsCommandData data;
};

struct KittyAnimationCommand {
	KittyGraphicsCommandData data;
};

struct KittyComposeCommand {
	KittyGraphicsCommandData data;
};

struct KittyQueryCommand {
	KittyGraphicsCommandData data;
};

using KittyGraphicsCommand =
	std::variant<KittyTransmitCommand, KittyPutCommand, KittyDeleteCommand, KittyFrameCommand,
				 KittyAnimationCommand, KittyComposeCommand, KittyQueryCommand>;

struct KittyGraphicsParseResult {
	std::optional<KittyGraphicsCommand> command;
	KittyGraphicsError error{ KittyGraphicsError::None };
};

struct KittyGraphicsHandleResult {
	KittyGraphicsHandleResult() = default;
	KittyGraphicsHandleResult( std::string response, KittyGraphicsError error, bool changed ) :
		response( std::move( response ) ), error( error ), changed( changed ) {}

	std::string response;
	Vector2i cursorMovement;
	KittyGraphicsError error{ KittyGraphicsError::None };
	bool changed{ false };
};

struct KittyGraphicsStats {
	Uint64 decodedBytes{ 0 };
	Uint64 fullImageUpdates{ 0 };
	Uint64 rectangleUpdates{ 0 };
	Uint64 evictions{ 0 };
};

class KittyGraphicsProtocol {
  public:
	explicit KittyGraphicsProtocol( size_t maxStorageBytes = 320 * 1024 * 1024,
									size_t maxImages = 4096, size_t maxPlacements = 65536 );

	static KittyGraphicsParseResult parse( std::string_view command );

	KittyGraphicsHandleResult handle( std::string_view command, Vector2i cursor = {} );

	std::vector<TerminalGraphicsUpdate> takeUpdates();

	std::shared_ptr<TerminalGraphicsPresentation> takePresentation();

	bool hasPendingPresentation() const { return mPresentationDirty; }

	const std::vector<Uint8>* imagePixels( KittyImageId imageId ) const;

	size_t imageCount() const { return mImages.size(); }

	const KittyGraphicsStats& stats() const { return mStats; }

	bool hasVirtualPlacements() const;

	void reset();

	void resync();

	void clearScreen();

	void setAlternateScreen( bool alternate );

	void scrollScreen( int top, int bottom, int rows, bool preserveHistory );

	void setViewport( int scrollPosition, int historyLength, int screenRows );

	bool updateAnimations();

	void setCellPixelSize( Uint32 width, Uint32 height );

	void setPlaceholderCells( std::vector<TerminalGraphicsPlaceholderCell> cells );

  private:
	struct Image {
		struct Frame {
			std::vector<Uint8> rgba;
			Int32 gapMs{ 40 };
			Uint32 usageHint{ 0 };
		};
		std::shared_ptr<std::vector<Uint8>> pixels;
		std::unordered_map<Uint32, Frame> frames;
		Sizei size;
		Uint64 creationSerial{ 0 };
		EE::System::Clock frameClock;
		Uint32 imageNumber{ 0 };
		Uint32 usageHint{ 0 };
		Uint32 currentFrame{ 1 };
		Uint32 loopCount{ 1 };
		Uint32 loopsCompleted{ 0 };
		Int32 rootGapMs{ 0 };
		Uint8 animationState{ 1 };
		Uint8 channels{ 4 };
		bool anonymous{ false };
	};

	struct PendingTransfer {
		KittyGraphicsCommandData data;
		std::vector<Uint8> decodedData;
		bool display{ false };
		bool query{ false };
		bool frame{ false };
		bool active{ false };
	};

	struct Placement {
		TerminalVisiblePlacement visible;
		Uint64 internalId{ 0 };
		bool virtualPlacement{ false };
		KittyImageId parentImageId{ 0 };
		KittyPlacementId parentPlacementId{ 0 };
	};

	KittyGraphicsHandleResult handleTransmit( const KittyGraphicsCommandData& data, bool display,
											  bool query, bool frame, Vector2i cursor );
	KittyGraphicsHandleResult finishTransfer( PendingTransfer transfer, Vector2i cursor );
	KittyGraphicsHandleResult put( const KittyGraphicsCommandData& data, Vector2i cursor );
	KittyGraphicsHandleResult remove( const KittyGraphicsCommandData& data, Vector2i cursor );
	KittyGraphicsHandleResult controlAnimation( const KittyGraphicsCommandData& data );
	KittyGraphicsHandleResult composeFrames( const KittyGraphicsCommandData& data );
	KittyImageId allocateImageId();
	KittyImageId resolveImageId( const KittyGraphicsCommandData& data ) const;
	bool ensureCapacity( size_t bytes, KittyImageId replacingId, bool addingImage );
	bool ensureRootRGBA( KittyImageId imageId, Image& image );
	bool isImagePlaced( KittyImageId imageId ) const;
	void eraseImage( KittyImageId imageId );
	std::string response( const KittyGraphicsCommandData& data, KittyGraphicsError error,
						  KittyImageId imageId = 0 ) const;

	std::unordered_map<KittyImageId, Image> mImages;
	std::vector<Placement> mPlacements;
	std::vector<Placement> mPrimaryPlacements;
	std::vector<TerminalGraphicsPlaceholderCell> mPlaceholderCells;
	std::vector<TerminalGraphicsUpdate> mUpdates;
	PendingTransfer mPending;
	std::vector<Uint8> mDecodedScratch;
	std::vector<Uint8> mPixelScratch;
	std::vector<Uint8> mEncodedScratch;
	std::vector<Uint8> mComposeSourceScratch;
	size_t mStorageBytes{ 0 };
	size_t mFrameStorageBytes{ 0 };
	Uint64 mCreationSerial{ 0 };
	Uint64 mPresentationGeneration{ 0 };
	Uint64 mPlacementSerial{ 0 };
	KittyImageId mNextImageId{ 1 };
	bool mPresentationDirty{ false };
	int mScrollPosition{ 0 };
	int mHistoryLength{ 0 };
	int mScreenRows{ 0 };
	Uint32 mCellPixelWidth{ 0 };
	Uint32 mCellPixelHeight{ 0 };
	size_t mMaxStorageBytes{ 0 };
	size_t mMaxImages{ 0 };
	size_t mMaxPlacements{ 0 };
	KittyGraphicsStats mStats;
};

}} // namespace eterm::Terminal

#endif
