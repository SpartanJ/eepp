#include <eepp/core/core.hpp>
#include <eepp/graphics/statelistdrawable.hpp>

namespace EE { namespace Graphics {

StateListDrawable* StateListDrawable::New( const std::string& name ) {
	return eeNew( StateListDrawable, ( name ) );
}

StateListDrawable::StateListDrawable( Type type, const std::string& name ) :
	StatefulDrawable( type, name ), mCurrentState( 0 ), mCurrentDrawable( NULL ) {}

StateListDrawable::StateListDrawable( const std::string& name ) :
	StatefulDrawable( STATELIST, name ), mCurrentState( 0 ), mCurrentDrawable( NULL ) {}

StateListDrawable::~StateListDrawable() {}

void StateListDrawable::clearDrawables() {
	mCurrentDrawable = nullptr;
	mDrawables.clear();
}

DrawablePtr StateListDrawable::clone() const {
	auto instance = ResourcePtr<StateListDrawable>( eeNew( StateListDrawable, ( mName ) ),
													ResourceDeleter<StateListDrawable>() );
	instance->setColor( mColor );
	instance->setPosition( mPosition );
	for ( const auto& state : mDrawables ) {
		if ( state.second ) {
			DrawablePtr drawable = state.second->clone();
			if ( !drawable )
				return {};
			instance->setStateDrawable( state.first, std::move( drawable ) );
		}
	}
	instance->mDrawableColors = mDrawableColors;
	instance->setState( mCurrentState );
	return instance;
}

Sizef StateListDrawable::getSize() {
	return NULL != mCurrentDrawable ? mCurrentDrawable->getSize() : Sizef();
}

Sizef StateListDrawable::getPixelsSize() {
	return mCurrentDrawable ? mCurrentDrawable->getPixelsSize() : Sizef();
}

Sizef StateListDrawable::getSize( const Uint32& state ) {
	auto it = mDrawables.find( state );

	if ( it != mDrawables.end() ) {
		return it->second->getSize();
	}

	return Sizef();
}

Sizef StateListDrawable::getPixelsSize( const Uint32& state ) {
	auto it = mDrawables.find( state );

	if ( it != mDrawables.end() ) {
		return it->second->getPixelsSize();
	}

	return Sizef();
}

void StateListDrawable::draw() {
	draw( mPosition );
}

void StateListDrawable::draw( const Vector2f& position ) {
	draw( position, getSize() );
}

void StateListDrawable::draw( const Vector2f& position, const Sizef& size ) {
	if ( NULL != mCurrentDrawable ) {
		if ( mColor.a != 255 || mCurrentDrawable->getAlpha() != 255 ) {
			Color color = mCurrentDrawable->getColor();
			Uint8 tempAlpha = static_cast<Uint8>( mColor.a * color.a / 255.f );

			mCurrentDrawable->setAlpha( tempAlpha );

			mCurrentDrawable->draw( position, size );

			mCurrentDrawable->setColor( color );
		} else {
			mCurrentDrawable->draw( position, size );
		}
	}
}

bool StateListDrawable::isStateful() {
	return true;
}

StatefulDrawable* StateListDrawable::setState( Uint32 state ) {
	auto current = mDrawables.find( state );
	Drawable* stateDrawable = current != mDrawables.end() ? current->second.get() : nullptr;
	if ( state != mCurrentState || mCurrentDrawable != stateDrawable ) {
		mCurrentState = state;
		mCurrentDrawable = stateDrawable;
	}

	return this;
}

const Uint32& StateListDrawable::getState() const {
	return mCurrentState;
}

Drawable* StateListDrawable::getStateDrawable( const Uint32& state ) {
	if ( hasDrawableState( state ) )
		return mDrawables[state].get();

	return NULL;
}

StateListDrawable* StateListDrawable::setStateDrawable( const Uint32& state,
														DrawablePtr drawable ) {
	if ( NULL != drawable ) {
		if ( hasDrawableState( state ) && mCurrentDrawable == mDrawables[state].get() )
			mCurrentDrawable = NULL;

		mDrawables[state] = std::move( drawable );

		if ( hasDrawableStateColor( state ) )
			mDrawables[state]->setColor( mDrawableColors[state] );

		if ( state == mCurrentState )
			setState( state );
	}

	return this;
}

Sizef StateListDrawable::getStateSize( const Uint32& state ) {
	if ( hasDrawableState( state ) )
		return mDrawables[state]->getSize();

	return Sizef::Zero;
}

StateListDrawable* StateListDrawable::setStateColor( const Uint32& state, const Color& color ) {
	mDrawableColors[state] = color;

	if ( hasDrawableState( state ) )
		mDrawables[state]->setColor( color );

	return this;
}

Color StateListDrawable::getStateColor( const Uint32& state ) {
	if ( hasDrawableStateColor( state ) )
		return mDrawableColors[state];

	return Color::Transparent;
}

StateListDrawable* StateListDrawable::setStateAlpha( const Uint32& state, const Uint8& alpha ) {
	if ( hasDrawableState( state ) ) {
		mDrawables[state]->setAlpha( alpha );
	}

	return this;
}

Uint8 StateListDrawable::getStateAlpha( const Uint32& state ) {
	if ( hasDrawableState( state ) )
		return mDrawables[state]->getAlpha();

	return 255;
}

bool StateListDrawable::hasDrawableState( const Uint32& state ) const {
	return mDrawables.find( state ) != mDrawables.end();
}

bool StateListDrawable::hasDrawableStateColor( const Uint32& state ) const {
	return mDrawableColors.find( state ) != mDrawableColors.end();
}

void StateListDrawable::onColorFilterChange() {
	for ( auto it = mDrawables.begin(); it != mDrawables.end(); ++it ) {
		Drawable* drawable = it->second.get();
		if ( drawable )
			drawable->setColor( mColor );
	}
}

}} // namespace EE::Graphics
