#ifndef EE_WINDOW_TERMINAL_FRAMEDAMAGE_HPP
#define EE_WINDOW_TERMINAL_FRAMEDAMAGE_HPP

#include <eepp/math/size.hpp>

#include <cstring>
#include <vector>

namespace EE { namespace Window { namespace Private {

constexpr Int32 FrameDamageTileSize = 32;
constexpr size_t FrameDamageRowMajorThreshold = 16 * 1024 * 1024;

enum class FrameDamageResult : Uint8 { None, Rectangle, Full };

/** Finds the bounding rectangle of changed RGB24 tiles in bottom-up frame buffers. */
template <typename Rectangle>
FrameDamageResult findFrameDamage( const std::vector<Uint8>& current,
								   const std::vector<Uint8>& previous, const Math::Sizei& size,
								   std::vector<Rectangle>& rectangles ) {
	rectangles.clear();
	const Int32 tilesX = ( size.x + FrameDamageTileSize - 1 ) / FrameDamageTileSize;
	const Int32 tilesY = ( size.y + FrameDamageTileSize - 1 ) / FrameDamageTileSize;
	const size_t rowBytes = static_cast<size_t>( size.x ) * 3;
	Int32 left = size.x;
	Int32 top = size.y;
	Int32 right = 0;
	Int32 bottom = 0;

	for ( Int32 tileY = 0; tileY < tilesY; ++tileY ) {
		const Int32 y = tileY * FrameDamageTileSize;
		const Int32 height = eemin( FrameDamageTileSize, size.y - y );
		for ( Int32 tileX = 0; tileX < tilesX; ++tileX ) {
			bool changed = false;
			const Int32 x = tileX * FrameDamageTileSize;
			const Int32 width = eemin( FrameDamageTileSize, size.x - x );
			for ( Int32 row = 0; row < height && !changed; ++row ) {
				const Int32 sourceRow = size.y - 1 - ( y + row );
				const size_t offset =
					static_cast<size_t>( sourceRow ) * rowBytes + static_cast<size_t>( x ) * 3;
				changed = 0 != std::memcmp( current.data() + offset, previous.data() + offset,
											static_cast<size_t>( width ) * 3 );
			}
			if ( changed ) {
				left = eemin( left, x );
				top = eemin( top, y );
				right = eemax( right, x + width );
				bottom = eemax( bottom, y + height );
			}
		}
	}

	if ( left < right && top < bottom ) {
		rectangles.push_back( { left, top, right - left, bottom - top } );
		return left == 0 && top == 0 && right == size.x && bottom == size.y
				   ? FrameDamageResult::Full
				   : FrameDamageResult::Rectangle;
	}
	return FrameDamageResult::None;
}

/** Uses sequential row traversal for frame buffers too large for cache-friendly tile traversal. */
template <typename Rectangle>
FrameDamageResult
findFrameDamageAdaptive( const std::vector<Uint8>& current, const std::vector<Uint8>& previous,
						 const Math::Sizei& size, std::vector<Rectangle>& rectangles,
						 std::vector<Uint8>& changedTiles ) {
	if ( current.size() < FrameDamageRowMajorThreshold )
		return findFrameDamage( current, previous, size, rectangles );

	rectangles.clear();
	const Int32 tilesX = ( size.x + FrameDamageTileSize - 1 ) / FrameDamageTileSize;
	const Int32 tilesY = ( size.y + FrameDamageTileSize - 1 ) / FrameDamageTileSize;
	const size_t rowBytes = static_cast<size_t>( size.x ) * 3;
	Int32 left = size.x;
	Int32 top = size.y;
	Int32 right = 0;
	Int32 bottom = 0;
	changedTiles.resize( static_cast<size_t>( tilesX ) );

	for ( Int32 tileY = 0; tileY < tilesY; ++tileY ) {
		const Int32 y = tileY * FrameDamageTileSize;
		const Int32 height = eemin( FrameDamageTileSize, size.y - y );
		std::memset( changedTiles.data(), 0, changedTiles.size() );
		Int32 remainingTiles = tilesX;
		for ( Int32 row = 0; row < height && remainingTiles; ++row ) {
			const Int32 sourceRow = size.y - 1 - ( y + row );
			const Uint8* currentRow = current.data() + static_cast<size_t>( sourceRow ) * rowBytes;
			const Uint8* previousRow =
				previous.data() + static_cast<size_t>( sourceRow ) * rowBytes;
			for ( Int32 tileX = 0; tileX < tilesX; ++tileX ) {
				if ( changedTiles[static_cast<size_t>( tileX )] )
					continue;
				const Int32 x = tileX * FrameDamageTileSize;
				const Int32 width = eemin( FrameDamageTileSize, size.x - x );
				const size_t offset = static_cast<size_t>( x ) * 3;
				if ( 0 != std::memcmp( currentRow + offset, previousRow + offset,
									   static_cast<size_t>( width ) * 3 ) ) {
					changedTiles[static_cast<size_t>( tileX )] = 1;
					--remainingTiles;
					left = eemin( left, x );
					top = eemin( top, y );
					right = eemax( right, x + width );
					bottom = eemax( bottom, y + height );
				}
			}
		}
	}

	if ( left < right && top < bottom ) {
		rectangles.push_back( { left, top, right - left, bottom - top } );
		return left == 0 && top == 0 && right == size.x && bottom == size.y
				   ? FrameDamageResult::Full
				   : FrameDamageResult::Rectangle;
	}
	return FrameDamageResult::None;
}

}}} // namespace EE::Window::Private

#endif
