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

StateListDrawable::~StateListDrawable() {
	clearDrawables();
}

void StateListDrawable::clearDrawables() {
	std::vector<Drawable*> removeOwnershipState;

	for ( auto it = mDrawables.begin(); it != mDrawables.end(); ++it ) {
		Drawable* drawable = it->second;

		if ( mDrawablesOwnership[drawable] ) {
			removeOwnershipState.push_back( drawable );
			eeSAFE_DELETE( drawable );
		}
	}

	for ( auto& removeOwnership : removeOwnershipState ) {
		mDrawablesOwnership.erase( removeOwnership );
	}

	mDrawables.clear();
}

Sizef StateListDrawable::getSize() {
	return NULL != mCurrentDrawable ? mCurrentDrawable->getSize() : Sizef();
}

Sizef StateListDrawable::getSize( const Uint32& state ) {
	auto it = mDrawables.find( state );

	if ( it != mDrawables.end() ) {
		return it->second->getSize();
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
	if ( state != mCurrentState || mCurrentDrawable == NULL ||
		 mCurrentDrawable != mDrawables[mCurrentState] ) {
		mCurrentState = state;

		auto it = mDrawables.find( state );

		if ( it != mDrawables.end() ) {
			mCurrentDrawable = it->second;
		} else {
			mCurrentDrawable = NULL;
		}
	}

	return this;
}

const Uint32& StateListDrawable::getState() const {
	return mCurrentState;
}

Drawable* StateListDrawable::getStateDrawable( const Uint32& state ) {
	if ( hasDrawableState( state ) )
		return mDrawables[state];

	return NULL;
}

StateListDrawable* StateListDrawable::setStateDrawable( const Uint32& state, Drawable* drawable,
														bool ownIt ) {
	if ( NULL != drawable ) {
		if ( hasDrawableState( state ) && mDrawablesOwnership[mDrawables[state]] ) {

			if ( mCurrentDrawable == mDrawables[state] )
				mCurrentDrawable = NULL;

			mDrawablesOwnership.erase( mDrawables[state] );
			eeDelete( mDrawables[state] );
		}

		mDrawables[state] = drawable;
		mDrawablesOwnership[drawable] = ownIt;

		if ( hasDrawableStateColor( state ) )
			drawable->setColor( mDrawableColors[state] );

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
		Drawable* drawable = it->second;

		drawable->setColor( mColor );
	}
}

}} // namespace EE::Graphics
