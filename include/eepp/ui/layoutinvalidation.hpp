#ifndef EE_UI_LAYOUTINVALIDATION_HPP
#define EE_UI_LAYOUTINVALIDATION_HPP

#include <eepp/config.hpp>

namespace EE { namespace Scene {
class NodeMessage;
}} // namespace EE::Scene

namespace EE { namespace UI {

enum class LayoutInvalidationReason : Uint32 {
	None = 0,
	SelfGeometry = 1u << 0,
	NormalFlowChild = 1u << 1,
	OutOfFlowChild = 1u << 2,
	IntrinsicSize = 1u << 3,
	FormattingContext = 1u << 4,
	Style = 1u << 5,
	DocumentExtent = 1u << 6,
	Viewport = 1u << 7,
	PaintOnly = 1u << 8,
};

using LayoutInvalidationFlags = Uint32;

inline constexpr LayoutInvalidationFlags
toLayoutInvalidationFlags( LayoutInvalidationReason reason ) {
	return static_cast<LayoutInvalidationFlags>( reason );
}

LayoutInvalidationFlags layoutInvalidationFromMessage( const Scene::NodeMessage* msg );

namespace LayoutInvalidation {

// Conservative self-change: geometry and intrinsic size may be stale.
inline constexpr LayoutInvalidationFlags Self =
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::SelfGeometry ) |
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::IntrinsicSize );

// Container property change: layout algorithm for children changed.
inline constexpr LayoutInvalidationFlags ContainerLayout =
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::Style ) |
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::SelfGeometry ) |
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::NormalFlowChild ) |
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::IntrinsicSize );

// Parent notification: normal-flow child geometry or intrinsic contribution may have changed.
inline constexpr LayoutInvalidationFlags ParentChildChange =
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::NormalFlowChild ) |
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::IntrinsicSize );

// Text/formatting change: formatting stream and intrinsic contributions stale.
inline constexpr LayoutInvalidationFlags TextFormatting =
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::FormattingContext ) |
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::IntrinsicSize );

// Replaced content change (image, etc.).
inline constexpr LayoutInvalidationFlags ReplacedContent =
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::SelfGeometry ) |
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::IntrinsicSize );

// Parent notification for replaced content / formatting-context child.
inline constexpr LayoutInvalidationFlags ParentReplacedFormatting =
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::NormalFlowChild ) |
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::IntrinsicSize ) |
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::FormattingContext );

// Out-of-flow position change.
inline constexpr LayoutInvalidationFlags OutOfFlow =
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::OutOfFlowChild ) |
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::SelfGeometry );

// Document/viewport extent change.
inline constexpr LayoutInvalidationFlags Document =
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::DocumentExtent ) |
	static_cast<LayoutInvalidationFlags>( LayoutInvalidationReason::Viewport );
} // namespace LayoutInvalidation

}} // namespace EE::UI

#endif
