#include <algorithm>
#include <cmath>
#include <eepp/ui/charts/xydatasource.hpp>
#include <mutex>
#include <utility>

namespace EE { namespace UI { namespace Charts {

namespace {

SmallVector<XYDataChunk, 2> chunksFor( std::span<const ChartPoint> points,
									   size_t logicalBegin = 0 ) {
	SmallVector<XYDataChunk, 2> chunks;
	if ( !points.empty() ) {
		chunks.emplace_back( XYDataChunk{ logicalBegin, memberSpan( points, &ChartPoint::x ),
										  memberSpan( points, &ChartPoint::y ) } );
	}
	return chunks;
}

DataOrder orderAfter( DataOrder order, double previous, double next ) {
	if ( !std::isfinite( previous ) || !std::isfinite( next ) )
		return DataOrder::Unknown;
	if ( order == DataOrder::AscendingX && next < previous )
		return DataOrder::Unknown;
	return order;
}

DataOrder orderOf( std::span<const ChartPoint> points ) {
	DataOrder order = DataOrder::AscendingX;
	for ( size_t i = 0; i < points.size(); ++i ) {
		if ( !std::isfinite( points[i].x ) )
			return DataOrder::Unknown;
		if ( i )
			order = orderAfter( order, points[i - 1].x, points[i].x );
	}
	return order;
}

template <typename Predicate> size_t boundX( const XYDataRead& read, Predicate before ) {
	assert( read.order() == DataOrder::AscendingX );
	size_t low = 0;
	size_t high = read.size();
	while ( low < high ) {
		const size_t mid = low + ( high - low ) / 2;
		const double x = pointAt( read, mid ).x;
		if ( before( x ) )
			low = mid + 1;
		else
			high = mid;
	}
	return low;
}

template <typename Axis> std::optional<DataRange> extent( const XYDataRead& read, Axis axis ) {
	std::optional<DataRange> result;
	for ( const auto& chunk : read.chunks() ) {
		std::visit(
			[&]( const auto& xs, const auto& ys ) {
				for ( size_t i = 0; i < xs.size(); ++i ) {
					const double x = xs[i];
					const double y = ys[i];
					if ( !std::isfinite( x ) || !std::isfinite( y ) )
						continue;
					const double value = axis( x, y );
					if ( !result )
						result = DataRange{ value, value };
					else {
						result->min = std::min( result->min, value );
						result->max = std::max( result->max, value );
					}
				}
			},
			chunk.xs, chunk.ys );
	}
	return result;
}

} // namespace

struct XYDataSource::SignalSlot {
	std::recursive_mutex mutex;
	std::function<void( Uint64 )> callback;
	bool connected{ true };
};

struct XYDataSource::SignalState {
	std::mutex mutex;
	SmallVector<std::shared_ptr<SignalSlot>, 4> slots;
};

XYDataSource::XYDataSource() : mSignal( std::make_shared<SignalState>() ) {}

XYDataSource::Connection& XYDataSource::Connection::operator=( Connection&& other ) noexcept {
	if ( this != &other ) {
		disconnect();
		mState = std::move( other.mState );
		mSlot = std::move( other.mSlot );
	}
	return *this;
}

void XYDataSource::Connection::disconnect() {
	if ( !mSlot )
		return;
	{
		std::lock_guard lock( mSlot->mutex );
		mSlot->connected = false;
	}
	if ( auto state = mState.lock() ) {
		std::lock_guard lock( state->mutex );
		auto found = std::find( state->slots.begin(), state->slots.end(), mSlot );
		if ( found != state->slots.end() )
			state->slots.erase( found );
	}
	mSlot.reset();
	mState.reset();
}

XYDataSource::Connection XYDataSource::onChanged( std::function<void( Uint64 )> callback ) {
	auto slot = std::make_shared<SignalSlot>();
	slot->callback = std::move( callback );
	{
		std::lock_guard lock( mSignal->mutex );
		mSignal->slots.emplace_back( slot );
	}
	return Connection( mSignal, std::move( slot ) );
}

void XYDataSource::notifyChanged( Uint64 generation ) {
	SmallVector<std::shared_ptr<SignalSlot>, 4> snapshot;
	{
		std::lock_guard lock( mSignal->mutex );
		snapshot = mSignal->slots;
	}
	for ( const auto& slot : snapshot ) {
		std::lock_guard lock( slot->mutex );
		if ( slot->connected )
			slot->callback( generation );
	}
}

XYDataRead::XYDataRead( Uint64 generation, DataOrder order, SmallVector<XYDataChunk, 2> chunks,
						std::shared_ptr<const void> backing, std::optional<DataDelta> delta ) :
	XYDataRead( generation, order, std::move( chunks ), std::move( backing ), {},
				std::move( delta ) ) {}

XYDataRead::XYDataRead( Uint64 generation, DataOrder order, SmallVector<XYDataChunk, 2> chunks,
						std::shared_ptr<const void> backing,
						std::shared_lock<std::shared_mutex> lock, std::optional<DataDelta> delta ) :
	mGeneration( generation ),
	mOrder( order ),
	mDelta( std::move( delta ) ),
	mChunks( std::move( chunks ) ),
	mBacking( std::move( backing ) ),
	mLock( std::move( lock ) ) {
	for ( const auto& chunk : mChunks ) {
		assert( chunk.valid() && chunk.logicalBegin == mSize );
		mSize += chunk.size();
	}
}

XYDataRead& XYDataRead::operator=( XYDataRead&& other ) {
	if ( this != &other ) {
		mLock = std::move( other.mLock );
		mBacking = std::move( other.mBacking );
		mChunks = std::move( other.mChunks );
		mGeneration = other.mGeneration;
		mSize = other.mSize;
		mOrder = other.mOrder;
		mDelta = std::move( other.mDelta );
	}
	return *this;
}

struct OwnedXYDataSource::State {
	mutable std::shared_mutex mutex;
	std::vector<ChartPoint> points;
	Uint64 generation{ 0 };
	DataOrder order{ DataOrder::AscendingX };
	std::optional<DataDelta> delta;
};

OwnedXYDataSource::OwnedXYDataSource() : mState( std::make_shared<State>() ) {}

XYDataRead OwnedXYDataSource::acquireRead() const {
	auto state = mState;
	std::shared_lock lock( state->mutex );
	auto chunks = chunksFor( std::span<const ChartPoint>( state->points ) );
	const Uint64 generation = state->generation;
	const DataOrder order = state->order;
	const auto delta = state->delta;
	return XYDataRead( generation, order, std::move( chunks ), std::move( state ),
					   std::move( lock ), delta );
}

void OwnedXYDataSource::reserve( size_t capacity ) {
	std::unique_lock lock( mState->mutex );
	mState->points.reserve( capacity );
}

void OwnedXYDataSource::setPoints( std::vector<ChartPoint> points ) {
	Uint64 generation;
	{
		std::unique_lock lock( mState->mutex );
		mState->order = orderOf( points );
		mState->points = std::move( points );
		mState->delta = DataDelta{ mState->generation, true };
		generation = ++mState->generation;
	}
	notifyChanged( generation );
}

void OwnedXYDataSource::append( ChartPoint point ) {
	append( std::span<const ChartPoint>( &point, 1 ) );
}

void OwnedXYDataSource::append( std::span<const ChartPoint> points ) {
	if ( points.empty() )
		return;
	Uint64 generation;
	{
		std::unique_lock lock( mState->mutex );
		const auto& existing = mState->points;
		if ( !existing.empty() )
			mState->order = orderAfter( mState->order, existing.back().x, points.front().x );
		if ( mState->order == DataOrder::AscendingX )
			mState->order = orderOf( points );
		mState->delta = DataDelta{
			mState->generation, false, 0, { existing.size(), existing.size() + points.size() } };
		mState->points.insert( mState->points.end(), points.begin(), points.end() );
		generation = ++mState->generation;
	}
	notifyChanged( generation );
}

void OwnedXYDataSource::clear() {
	Uint64 generation;
	{
		std::unique_lock lock( mState->mutex );
		mState->points.clear();
		mState->order = DataOrder::AscendingX;
		mState->delta = DataDelta{ mState->generation, true };
		generation = ++mState->generation;
	}
	notifyChanged( generation );
}

struct RingXYDataSource::State {
	mutable std::shared_mutex mutex;
	std::vector<ChartPoint> points;
	size_t start{ 0 };
	size_t size{ 0 };
	Uint64 generation{ 0 };
	DataOrder order{ DataOrder::AscendingX };
	std::optional<DataDelta> delta;
};

RingXYDataSource::RingXYDataSource( size_t capacity ) : mState( std::make_shared<State>() ) {
	mState->points.resize( capacity );
}

XYDataRead RingXYDataSource::acquireRead() const {
	auto state = mState;
	std::shared_lock lock( state->mutex );
	SmallVector<XYDataChunk, 2> chunks;
	if ( state->size ) {
		const size_t firstSize = std::min( state->size, state->points.size() - state->start );
		const std::span<const ChartPoint> first( state->points.data() + state->start, firstSize );
		chunks = chunksFor( first );
		if ( firstSize < state->size ) {
			const std::span<const ChartPoint> second( state->points.data(),
													  state->size - firstSize );
			chunks.emplace_back( XYDataChunk{ firstSize, memberSpan( second, &ChartPoint::x ),
											  memberSpan( second, &ChartPoint::y ) } );
		}
	}
	const Uint64 generation = state->generation;
	const DataOrder order = state->order;
	const auto delta = state->delta;
	return XYDataRead( generation, order, std::move( chunks ), std::move( state ),
					   std::move( lock ), delta );
}

void RingXYDataSource::append( ChartPoint point ) {
	Uint64 generation;
	{
		std::unique_lock lock( mState->mutex );
		const size_t capacity = mState->points.size();
		if ( !capacity )
			return;
		if ( mState->size ) {
			const size_t last = ( mState->start + mState->size - 1 ) % capacity;
			mState->order = orderAfter( mState->order, mState->points[last].x, point.x );
		} else if ( !std::isfinite( point.x ) ) {
			mState->order = DataOrder::Unknown;
		}
		const size_t oldSize = mState->size;
		const size_t next = ( mState->start + mState->size ) % capacity;
		mState->points[next] = point;
		if ( mState->size == capacity )
			mState->start = ( mState->start + 1 ) % capacity;
		else
			++mState->size;
		mState->delta = DataDelta{ mState->generation,
								   false,
								   oldSize == capacity ? 1u : 0u,
								   { mState->size - 1, mState->size } };
		generation = ++mState->generation;
	}
	notifyChanged( generation );
}

void RingXYDataSource::clear() {
	Uint64 generation;
	{
		std::unique_lock lock( mState->mutex );
		mState->start = 0;
		mState->size = 0;
		mState->order = DataOrder::AscendingX;
		mState->delta = DataDelta{ mState->generation, true };
		generation = ++mState->generation;
	}
	notifyChanged( generation );
}

ImmutableXYDataSource::ImmutableXYDataSource( XYDataChunk chunk,
											  std::shared_ptr<const void> backing,
											  DataOrder order ) :
	mChunk( std::move( chunk ) ), mBacking( std::move( backing ) ), mOrder( order ) {
	assert( mChunk.valid() && mBacking );
}

XYDataRead ImmutableXYDataSource::acquireRead() const {
	SmallVector<XYDataChunk, 2> chunks;
	if ( mChunk.size() )
		chunks.emplace_back( mChunk );
	return XYDataRead( 1, mOrder, std::move( chunks ), mBacking );
}

std::optional<DataRange> xExtent( const XYDataRead& read ) {
	return extent( read, []( double x, double ) { return x; } );
}

std::optional<DataRange> yExtent( const XYDataRead& read ) {
	return extent( read, []( double, double y ) { return y; } );
}

std::optional<DataRange> visibleYExtent( const XYDataRead& read, DataRange xRange ) {
	if ( read.order() != DataOrder::AscendingX )
		return yExtent( read );
	const IndexRange visible = visibleLogicalRange( read, xRange.min, xRange.max );
	std::optional<DataRange> result;
	for ( const auto& chunk : read.chunks() ) {
		const size_t begin = std::max( visible.begin, chunk.logicalBegin );
		const size_t end = std::min( visible.end, chunk.logicalBegin + chunk.size() );
		if ( begin >= end )
			continue;
		std::visit(
			[&]( const auto& xs, const auto& ys ) {
				for ( size_t index = begin; index < end; ++index ) {
					const size_t i = index - chunk.logicalBegin;
					const double x = xs[i];
					const double y = ys[i];
					if ( !std::isfinite( x ) || !std::isfinite( y ) )
						continue;
					if ( !result )
						result = DataRange{ y, y };
					else {
						result->min = std::min( result->min, y );
						result->max = std::max( result->max, y );
					}
				}
			},
			chunk.xs, chunk.ys );
	}
	return result;
}

ChartPoint pointAt( const XYDataRead& read, size_t logicalIndex ) {
	assert( logicalIndex < read.size() );
	for ( const auto& chunk : read.chunks() ) {
		if ( logicalIndex < chunk.logicalBegin + chunk.size() ) {
			const size_t i = logicalIndex - chunk.logicalBegin;
			return { std::visit( [i]( const auto& xs ) { return static_cast<double>( xs[i] ); },
								 chunk.xs ),
					 std::visit( [i]( const auto& ys ) { return static_cast<double>( ys[i] ); },
								 chunk.ys ) };
		}
	}
	return {};
}

size_t lowerBoundX( const XYDataRead& read, double value ) {
	return boundX( read, [value]( double x ) { return x < value; } );
}

IndexRange visibleLogicalRange( const XYDataRead& read, double minimum, double maximum ) {
	if ( minimum > maximum )
		return {};
	return { lowerBoundX( read, minimum ),
			 boundX( read, [maximum]( double x ) { return x <= maximum; } ) };
}

}}} // namespace EE::UI::Charts
