#include <eepp/scene/actions/actions.hpp>
#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/ui/uipopup.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

namespace {

Vector2f clampPosition( Vector2f position, const Sizef& size, const Rectf& bounds ) {
	position.x =
		eeclamp( position.x, bounds.Left, eemax( bounds.Left, bounds.Right - size.getWidth() ) );
	position.y =
		eeclamp( position.y, bounds.Top, eemax( bounds.Top, bounds.Bottom - size.getHeight() ) );
	return position;
}

} // namespace

Vector2f UIPopUp::findBestPosition( UIWidget* popup, const Vector2f& position, Node* anchor,
									bool trySides ) {
	if ( !popup || !popup->getSceneNode() )
		return position;

	const Sizef size( popup->getPixelsSize() );
	const Rectf bounds( popup->getUISceneNode()->getVisibleWorldBounds() );
	if ( anchor ) {
		const Rectf field( anchor->getScreenRect() );
		// Field popups stay vertically attached even when their left edge must be clamped.
		const Float x = trySides ? position.x : clampPosition( position, size, bounds ).x;
		const Vector2f candidates[] = { { x, field.Bottom },
										{ x, field.Top - size.getHeight() },
										{ field.Right, field.Top },
										{ field.Left - size.getWidth(), field.Top } };
		for ( size_t i = 0; i < ( trySides ? 4u : 2u ); ++i ) {
			const Rectf candidate( candidates[i], size );
			// Dropdowns clamp horizontally independently of the below/above choice. Even a
			// list wider than the scene must remain below the field when its height fits.
			const bool fits =
				trySides ? bounds.contains( candidate )
						 : candidate.Top >= bounds.Top && candidate.Bottom <= bounds.Bottom;
			if ( fits )
				return candidates[i];
		}
		// Trigger menus retain their requested point if no side fits; field popups use the
		// bottom scene edge when neither below nor above has enough space.
		return clampPosition( trySides ? position : Vector2f( x, bounds.Bottom - size.getHeight() ),
							  size, bounds );
	}

	// Cursor menus prefer down/right, up/right, up/left, then down/left.
	const Vector2f candidates[] = { position,
									{ position.x, position.y - size.getHeight() },
									{ position.x - size.getWidth(), position.y - size.getHeight() },
									{ position.x - size.getWidth(), position.y } };
	for ( const auto& candidate : candidates ) {
		if ( bounds.contains( Rectf( candidate, size ) ) )
			return candidate;
	}
	return clampPosition( { position.x, bounds.Bottom - size.getHeight() }, size, bounds );
}

void UIPopUp::align( UIWidget* anchor, UIWidget* popup, bool popUpToRoot, bool centered ) {
	if ( nullptr == anchor || nullptr == popup || nullptr == anchor->getUISceneNode() )
		return;

	if ( !popUpToRoot )
		popup->setParent( anchor->getWindowContainer() );
	else
		popup->setParent( anchor->getUISceneNode()->getRoot() );

	popup->toFront();

	// The placement is decided in screen pixels, where the field, the popup and the scene bounds
	// are directly comparable. Deriving it from the anchor's own dp position previously mixed
	// coordinate spaces (the candidate point was expressed in the parent's space but converted
	// through the field's own nodeToWorld, shifting the test rectangle by the field's offset), so
	// the "fits below" check failed for any field away from its parent's origin and the popup was
	// flipped above the field even when there was no room there.

	const Rectf field( anchor->getScreenRect() );
	const Sizef popUpSize( popup->getPixelsSize() );
	const Float x = centered
						? field.Left + eefloor( ( field.getWidth() - popUpSize.getWidth() ) * 0.5f )
						: field.Left;
	Vector2f pos = findBestPosition( popup, { x, field.Bottom }, anchor );
	// worldToNode already converts world pixels back to parent dp; do not apply density again.
	popup->getParent()->worldToNode( pos );
	popup->setPosition( pos );
}

bool UIPopUp::hasFocus( UIWidget* anchor, UIWidget* popup ) {
	if ( !anchor || !anchor->getEventDispatcher() )
		return false;
	// Focus-loss events are emitted before the old node clears its focus flag. The dispatcher
	// already names the new node, so test its ancestry instead of cached hasFocus() flags.
	const auto* focus = anchor->getEventDispatcher()->getFocusNode();
	return focus && ( focus == anchor || anchor->isParentOf( focus ) ||
					  ( popup && ( focus == popup || popup->isParentOf( focus ) ) ) );
}

void UIPopUp::show( UIWidget* popup ) {
	if ( nullptr == popup )
		return;

	popup->removeActionsByTag( String::hash( "eepp-popup-visibility" ) );
	popup->setEnabled( true );
	popup->setVisible( true );

	if ( nullptr != popup->getUISceneNode() &&
		 popup->getUISceneNode()->getUIThemeManager()->getDefaultEffectsEnabled() ) {
		auto* action = Actions::Sequence::New(
			Actions::Fade::New(
				255.f == popup->getAlpha() ? 0.f : popup->getAlpha(), 255.f,
				popup->getUISceneNode()->getUIThemeManager()->getWidgetsFadeOutTime() ),
			Actions::Spawn::New( Actions::Enable::New(), Actions::Visible::New( true ) ) );
		action->setTag( String::hash( "eepp-popup-visibility" ) );
		popup->runAction( action );
	} else {
		// A previous fade-out can leave the popup transparent after effects are disabled.
		popup->setAlpha( 255.f );
	}
}

void UIPopUp::hide( UIWidget* popup ) {
	if ( nullptr == popup )
		return;
	popup->removeActionsByTag( String::hash( "eepp-popup-visibility" ) );
	popup->setEnabled( false );

	if ( nullptr != popup->getUISceneNode() &&
		 popup->getUISceneNode()->getUIThemeManager()->getDefaultEffectsEnabled() ) {
		auto* action = Actions::Sequence::New(
			Actions::FadeOut::New(
				popup->getUISceneNode()->getUIThemeManager()->getWidgetsFadeOutTime() ),
			Actions::Spawn::New( Actions::Disable::New(), Actions::Visible::New( false ) ) );
		action->setTag( String::hash( "eepp-popup-visibility" ) );
		popup->runAction( action );
	} else {
		popup->setEnabled( false );
		popup->setVisible( false );
	}
}

}} // namespace EE::UI
