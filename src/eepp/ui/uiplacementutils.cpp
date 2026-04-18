#include <eepp/ui/uiplacementutils.hpp>

namespace EE::UI {

PopupPlacementResult UIPlacementUtils::findBestPopupPlacement(
	const PopupPlacementConfig& config,
	const std::function<Sizef( Float availableWidth )>& measureContentCb ) {
	bool hasAvoidRect =
		( config.avoidRect.getSize().getWidth() > 0 && config.avoidRect.getSize().getHeight() > 0 );
	Float bottomAvoid = hasAvoidRect ? std::max( config.targetRect.Bottom, config.avoidRect.Bottom )
									 : config.targetRect.Bottom;
	Float topAvoid = hasAvoidRect ? std::min( config.targetRect.Top, config.avoidRect.Top )
								  : config.targetRect.Top;

	struct Candidate {
		PlacementDirection direction;
		Float availableWidth;
		Float availableHeight;
		Float score;
		Float attachX;
		Float attachY;
	};

	SmallVector<Candidate, 4> candidates;

	// Populate Candidates based on supported axes
	if ( config.supportHorizontal ) {
		candidates.push_back(
			{ PlacementDirection::Right,
			  std::max( 0.f, config.areaRect.Right - config.targetRect.Right - config.margin ),
			  std::max( 0.f, config.areaRect.getHeight() - config.margin * 2 ), 0,
			  config.targetRect.Right + config.margin, config.alignRect.Top } );

		candidates.push_back(
			{ PlacementDirection::Left,
			  std::max( 0.f, config.targetRect.Left - config.areaRect.Left - config.margin ),
			  std::max( 0.f, config.areaRect.getHeight() - config.margin * 2 ), 0,
			  config.targetRect.Left - config.margin, config.alignRect.Top } );
	}

	if ( config.supportVertical ) {
		candidates.push_back(
			{ PlacementDirection::Bottom,
			  std::max( 0.f, config.areaRect.getWidth() - config.margin * 2 ),
			  std::max( 0.f, config.areaRect.Bottom - bottomAvoid - config.margin ), 0,
			  config.targetRect.Left, bottomAvoid + config.margin } );

		candidates.push_back( { PlacementDirection::Top,
								std::max( 0.f, config.areaRect.getWidth() - config.margin * 2 ),
								std::max( 0.f, topAvoid - config.areaRect.Top - config.margin ), 0,
								config.targetRect.Left, topAvoid - config.margin } );
	}

	if ( candidates.empty() ) {
		return { Rectf(), PlacementDirection::None };
	}

	// Score the candidates
	for ( auto& c : candidates ) {
		Float maxW = std::min( config.userMaxWidth, c.availableWidth );

		// Base area score
		c.score = maxW * std::min( c.availableHeight, config.maxScoreHeight );

		// Apply a massive penalty (instead of a hard zero) if space is terrible.
		// This ensures we always pick the "least bad" option on tiny screens.
		if ( maxW < 50 || c.availableHeight < config.minScoreHeight ) {
			c.score *= 0.001f;
		}

		// Apply layout biases and tie-breakers
		if ( config.layoutBias == PlacementLayout::Horizontal ) {
			if ( c.direction == PlacementDirection::Right ||
				 c.direction == PlacementDirection::Left ) {
				if ( c.availableWidth >= config.minHorizontalSpace ) {
					c.score += 1000000;
					if ( c.direction == PlacementDirection::Right )
						c.score += 100; // Tie-breaker
				}
			}
		} else if ( config.layoutBias == PlacementLayout::Vertical ) {
			if ( c.direction == PlacementDirection::Bottom ||
				 c.direction == PlacementDirection::Top ) {
				if ( c.availableHeight >= config.minVerticalSpace ) {
					c.score += 1000000;
					if ( c.direction == PlacementDirection::Bottom )
						c.score += 100; // Tie-breaker
				}
			}
		}
	}

	std::sort( candidates.begin(), candidates.end(),
			   []( const auto& a, const auto& b ) { return a.score > b.score; } );

	const auto& best = candidates.front();

	// Measurement step
	Float allocatedWidth = std::max( 0.f, std::min( config.userMaxWidth, best.availableWidth ) );
	Sizef boxSize = measureContentCb( allocatedWidth );

	// Height constraint
	boxSize.setHeight( std::min( boxSize.getHeight(), best.availableHeight ) );

	Vector2f pos;
	if ( best.direction == PlacementDirection::Right ) {
		pos.x = best.attachX;
		pos.y = best.attachY;
	} else if ( best.direction == PlacementDirection::Left ) {
		pos.x = best.attachX - boxSize.getWidth();
		pos.y = best.attachY;
	} else if ( best.direction == PlacementDirection::Bottom ) {
		pos.x = best.attachX;
		pos.y = best.attachY;
	} else { // Top
		pos.x = best.attachX;
		pos.y = best.attachY - boxSize.getHeight();
	}

	// Final Clamping
	if ( best.direction == PlacementDirection::Right ) {
		pos.x = std::min( pos.x, config.areaRect.Right - boxSize.getWidth() );
		pos.x = std::max( pos.x, config.targetRect.Right + config.margin );
		pos.y = std::max(
			config.areaRect.Top + config.margin,
			std::min( pos.y, config.areaRect.Bottom - boxSize.getHeight() - config.margin ) );
	} else if ( best.direction == PlacementDirection::Left ) {
		pos.x = std::max( pos.x, config.areaRect.Left + config.margin );
		pos.x = std::min( pos.x, config.targetRect.Left - boxSize.getWidth() - config.margin );
		pos.y = std::max(
			config.areaRect.Top + config.margin,
			std::min( pos.y, config.areaRect.Bottom - boxSize.getHeight() - config.margin ) );
	} else if ( best.direction == PlacementDirection::Bottom ) {
		pos.y = std::min( pos.y, config.areaRect.Bottom - boxSize.getHeight() - config.margin );
		pos.y = std::max( pos.y, bottomAvoid + config.margin );
		pos.x = std::max(
			config.areaRect.Left + config.margin,
			std::min( pos.x, config.areaRect.Right - boxSize.getWidth() - config.margin ) );
	} else { // Top
		pos.y = std::max( pos.y, config.areaRect.Top + config.margin );
		pos.y = std::min( pos.y, topAvoid - boxSize.getHeight() - config.margin );
		pos.x = std::max(
			config.areaRect.Left + config.margin,
			std::min( pos.x, config.areaRect.Right - boxSize.getWidth() - config.margin ) );
	}

	return { Rectf( pos, boxSize ).round(), best.direction };
}

} // namespace EE::UI
