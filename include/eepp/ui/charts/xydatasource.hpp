#ifndef EE_UI_CHARTS_XYDATASOURCE_HPP
#define EE_UI_CHARTS_XYDATASOURCE_HPP

#include <cassert>
#include <cmath>
#include <cstddef>
#include <eepp/config.hpp>
#include <eepp/core/small_vector.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <ranges>
#include <shared_mutex>
#include <span>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace EE { namespace UI { namespace Charts {

/** A non-owning numeric view. Its owner must remain stable for the entire read transaction. */
template <typename T> class StridedSpan {
  public:
	constexpr StridedSpan() = default;

	constexpr StridedSpan( const T* data, size_t size, size_t strideBytes = sizeof( T ) ) :
		mData( data ), mSize( size ), mStrideBytes( strideBytes ) {
		assert( size == 0 || ( data && strideBytes >= sizeof( T ) ) );
	}

	constexpr StridedSpan( std::span<const T> span ) : StridedSpan( span.data(), span.size() ) {}

	template <std::ranges::contiguous_range Range>
		requires std::same_as<std::remove_cv_t<std::ranges::range_value_t<Range>>, T>
	constexpr StridedSpan( Range& range ) :
		StridedSpan( std::ranges::data( range ), std::ranges::size( range ) ) {}

	constexpr size_t size() const { return mSize; }

	constexpr bool empty() const { return mSize == 0; }

	T operator[]( size_t index ) const {
		assert( index < mSize );
		return *reinterpret_cast<const T*>( reinterpret_cast<const std::byte*>( mData ) +
											index * mStrideBytes );
	}

	constexpr bool contiguous() const { return mStrideBytes == sizeof( T ); }

	constexpr size_t strideBytes() const { return mStrideBytes; }

	constexpr const T* data() const { return mData; }

	std::span<const T> span() const {
		assert( contiguous() );
		return { mData, mSize };
	}

  private:
	const T* mData{ nullptr };
	size_t mSize{ 0 };
	size_t mStrideBytes{ sizeof( T ) };
};

template <typename Object, typename Member>
StridedSpan<Member> memberSpan( std::span<const Object> objects, Member Object::* member ) {
	static_assert( std::is_standard_layout_v<Object> );
	return { objects.empty() ? nullptr : &( objects.front().*member ), objects.size(),
			 sizeof( Object ) };
}

struct LinearSpan {
	double first{ 0.0 };
	double step{ 1.0 };
	size_t count{ 0 };

	constexpr size_t size() const { return count; }

	constexpr double operator[]( size_t index ) const {
		assert( index < count );
		return first + step * static_cast<double>( index );
	}
};

using XDataView = std::variant<StridedSpan<float>, StridedSpan<double>, LinearSpan>;
using YDataView = std::variant<StridedSpan<float>, StridedSpan<double>>;

struct XYDataChunk {
	size_t logicalBegin{ 0 };
	XDataView xs;
	YDataView ys;

	size_t size() const {
		return std::visit( []( const auto& xs ) { return xs.size(); }, xs );
	}

	bool valid() const {
		return size() == std::visit( []( const auto& ys ) { return ys.size(); }, ys );
	}
};

struct ChartPoint {
	double x{ 0.0 };
	double y{ 0.0 };
};

struct DataRange {
	double min{ 0.0 };
	double max{ 0.0 };
};

struct IndexRange {
	size_t begin{ 0 };
	size_t end{ 0 };
};

/** Describes the latest mutation only; consumers must check fromGeneration before using it. */
struct DataDelta {
	Uint64 fromGeneration{ 0 };
	bool reset{ true };
	size_t removedFront{ 0 };
	IndexRange appended;
	SmallVector<IndexRange, 2> modified;
};

enum class DataOrder : Uint8 { Unknown, AscendingX, DescendingX };

/** A move-only, coherent view. Chunks and their storage expire with this object. */
class EE_API XYDataRead {
  public:
	XYDataRead() = default;

	/** The backing token must own or guard every byte referenced by chunks. */
	XYDataRead( Uint64 generation, DataOrder order, SmallVector<XYDataChunk, 2> chunks,
				std::shared_ptr<const void> backing = {}, std::optional<DataDelta> delta = {} );

	XYDataRead( XYDataRead&& ) = default;
	XYDataRead& operator=( XYDataRead&& other );
	XYDataRead( const XYDataRead& ) = delete;
	XYDataRead& operator=( const XYDataRead& ) = delete;

	Uint64 generation() const { return mGeneration; }

	size_t size() const { return mSize; }

	DataOrder order() const { return mOrder; }

	const std::optional<DataDelta>& delta() const { return mDelta; }

	std::span<const XYDataChunk> chunks() const { return { mChunks.data(), mChunks.size() }; }

  private:
	friend class OwnedXYDataSource;
	friend class RingXYDataSource;
	XYDataRead( Uint64 generation, DataOrder order, SmallVector<XYDataChunk, 2> chunks,
				std::shared_ptr<const void> backing, std::shared_lock<std::shared_mutex> lock,
				std::optional<DataDelta> delta );

	Uint64 mGeneration{ 0 };
	size_t mSize{ 0 };
	DataOrder mOrder{ DataOrder::Unknown };
	std::optional<DataDelta> mDelta;
	SmallVector<XYDataChunk, 2> mChunks;
	// Destruction order matters: release the lock before destroying its owning state.
	std::shared_ptr<const void> mBacking;
	std::shared_lock<std::shared_mutex> mLock;
};

class EE_API XYDataSource {
  private:
	struct SignalState;
	struct SignalSlot;

  public:
	class EE_API Connection {
	  public:
		Connection() = default;
		~Connection() { disconnect(); }
		Connection( Connection&& ) noexcept = default;
		Connection& operator=( Connection&& other ) noexcept;
		Connection( const Connection& ) = delete;
		Connection& operator=( const Connection& ) = delete;

		void disconnect();

	  private:
		friend class XYDataSource;
		Connection( std::weak_ptr<SignalState> state, std::shared_ptr<SignalSlot> slot ) :
			mState( std::move( state ) ), mSlot( std::move( slot ) ) {}
		std::weak_ptr<SignalState> mState;
		std::shared_ptr<SignalSlot> mSlot;
	};

	XYDataSource();

	virtual ~XYDataSource() = default;

	virtual XYDataRead acquireRead() const = 0;

	/** Synchronous, thread-safe notification. UI consumers must marshal onto the UI thread. */
	Connection onChanged( std::function<void( Uint64 )> callback );

  protected:
	void notifyChanged( Uint64 generation );

  private:
	std::shared_ptr<SignalState> mSignal;
};

/** Mutable, zero-copy AoS source. Writers wait for active reads to finish. */
class EE_API OwnedXYDataSource final : public XYDataSource {
  public:
	OwnedXYDataSource();

	XYDataRead acquireRead() const override;

	void reserve( size_t capacity );

	void setPoints( std::vector<ChartPoint> points );

	void append( ChartPoint point );

	void append( std::span<const ChartPoint> points );

	void clear();

  private:
	struct State;
	std::shared_ptr<State> mState;
};

/** Fixed-capacity FIFO source; a wrapped read exposes two logical chunks. */
class EE_API RingXYDataSource final : public XYDataSource {
  public:
	explicit RingXYDataSource( size_t capacity );

	XYDataRead acquireRead() const override;

	void append( ChartPoint point );

	void clear();

  private:
	struct State;
	std::shared_ptr<State> mState;
};

/** An immutable view over shared storage. Keep all mutation outside the source's lifetime. */
class EE_API ImmutableXYDataSource final : public XYDataSource {
  public:
	ImmutableXYDataSource( XYDataChunk chunk, std::shared_ptr<const void> backing,
						   DataOrder order );

	XYDataRead acquireRead() const override;

  private:
	XYDataChunk mChunk;
	std::shared_ptr<const void> mBacking;
	DataOrder mOrder;
};

template <typename X> DataOrder detectXOrder( StridedSpan<X> xs ) {
	for ( size_t i = 0; i < xs.size(); ++i )
		if ( !std::isfinite( xs[i] ) || ( i && xs[i] < xs[i - 1] ) )
			return DataOrder::Unknown;
	return DataOrder::AscendingX;
}

/** Zero-copy shared, separate X/Y arrays. Inputs must be the same length. */
template <typename X, typename Y>
	requires( std::is_same_v<X, float> || std::is_same_v<X, double> ) &&
			(std::is_same_v<Y, float> || std::is_same_v<Y, double>)
std::shared_ptr<ImmutableXYDataSource>
makeArrayXYDataSource( std::shared_ptr<const std::vector<X>> xs,
					   std::shared_ptr<const std::vector<Y>> ys ) {
	assert( xs && ys && xs->size() == ys->size() );
	using Owners =
		std::pair<std::shared_ptr<const std::vector<X>>, std::shared_ptr<const std::vector<Y>>>;
	auto owners = std::make_shared<Owners>( std::move( xs ), std::move( ys ) );
	StridedSpan<X> xView( *owners->first );
	return std::make_shared<ImmutableXYDataSource>(
		XYDataChunk{ 0, xView, StridedSpan<Y>( *owners->second ) }, owners, detectXOrder( xView ) );
}

/** Zero-copy shared AoS storage with typed member views. */
template <typename Object, typename X, typename Y>
	requires( std::is_same_v<X, float> || std::is_same_v<X, double> ) &&
			(std::is_same_v<Y, float> || std::is_same_v<Y, double>)
std::shared_ptr<ImmutableXYDataSource>
makeMemberXYDataSource( std::shared_ptr<const std::vector<Object>> samples, X Object::* xMember,
						Y Object::* yMember ) {
	assert( samples );
	const std::span<const Object> span( *samples );
	auto xView = memberSpan( span, xMember );
	return std::make_shared<ImmutableXYDataSource>(
		XYDataChunk{ 0, xView, memberSpan( span, yMember ) }, std::move( samples ),
		detectXOrder( xView ) );
}

/** Finite paired values only; NaN and Inf form gaps. */
EE_API std::optional<DataRange> xExtent( const XYDataRead& read );

EE_API std::optional<DataRange> yExtent( const XYDataRead& read );

/** Scans only the raw samples in an ascending-X interval. */
EE_API std::optional<DataRange> visibleYExtent( const XYDataRead& read, DataRange xRange );

/** Ascending-X reads only; returns the first index whose X is at least value. */
EE_API size_t lowerBoundX( const XYDataRead& read, double value );

/** Ascending-X reads only; includes samples in [minimum, maximum]. */
EE_API IndexRange visibleLogicalRange( const XYDataRead& read, double minimum, double maximum );

EE_API ChartPoint pointAt( const XYDataRead& read, size_t logicalIndex );

}}} // namespace EE::UI::Charts

#endif
