#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/scene/inputmethod.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/window/window.hpp>

namespace EE { namespace Scene {

void InputMethod::setLocation( Rect rect ) {
	mSceneNode->getWindow()->setTextInputRect( rect );
}

bool InputMethod::isEditing() const {
	return mEditing;
}

void InputMethod::reset() {
	mEditing = false;
	mState = {};
}

void InputMethod::stop() {
	if ( mEditing ) {
		mSceneNode->getWindow()->clearComposition();
		mSceneNode->getEventDispatcher()->sendTextEditing( "", 0, 0 );
		reset();
	}
}

void InputMethod::onTextEditing( const String& text, const Int32& start, const Int32& length ) {
	if ( text.empty() ) {
		reset();
	} else {
		mEditing = true;
		mState = { text, start, length };
	}
}

const InputMethod::State& InputMethod::getState() const {
	return mState;
}

void InputMethod::draw( const Vector2f& screenPos, const Float& lineHeight,
						const FontStyleConfig& fontStyle, const Color& backgroundColor,
						const Color& lineColor ) {
	Float width = Text::getTextWidth( mState.text, fontStyle );
	Primitives p;

	if ( backgroundColor != Color::Transparent ) {
		p.setColor( backgroundColor );
		p.drawRectangle( { screenPos, { width, lineHeight } } );
	}

	if ( lineColor != Color::Transparent ) {
		Float lh = PixelDensity::dpToPx( 2 );
		p.setColor( lineColor );
		p.drawRectangle(
			Rectf( { screenPos.x, screenPos.y + lineHeight - lh * 0.5f }, { width, lh } ) );
	}

	p.drawBatch();

	Text::draw( mState.text, screenPos, fontStyle );
}

}} // namespace EE::Scene
