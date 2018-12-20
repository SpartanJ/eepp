#include <eepp/graphics/statelistdrawable.hpp>
#include <eepp/core/core.hpp>

namespace EE { namespace Graphics {

StateListDrawable *StateListDrawable::New() {
	return eeNew( StateListDrawable, ( ) );
}

StateListDrawable::StateListDrawable() :
	StatefulDrawable( STATELIST ),
	mDrawableOwner( false ),
	mCurrentState( 0 ),
	mCurrentDrawable( NULL )
{
}

StateListDrawable::~StateListDrawable() {
	clearDrawables();
}

void StateListDrawable::clearDrawables() {
	if ( mDrawableOwner ) {
		for ( auto it = mDrawables.begin(); it != mDrawables.end(); ++it ) {
			Drawable * drawable = it->second;
			eeSAFE_DELETE( drawable );
		}
	}

	mDrawables.clear();
}

Sizef StateListDrawable::getSize() {
	return NULL != mCurrentDrawable ? mCurrentDrawable->getSize() : Sizef();
}

void StateListDrawable::draw() {
	draw( mPosition );
}

void StateListDrawable::draw( const Vector2f& position ) {
	draw( position, getSize() );
}

void StateListDrawable::draw( const Vector2f & position, const Sizef & size ) {
	if ( NULL != mCurrentDrawable )
		mCurrentDrawable->draw( position, size );
}

bool StateListDrawable::isStateful() {
	return true;
}

StatefulDrawable * StateListDrawable::setState( Uint32 state ) {
	if ( state != mCurrentState ) {
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

StateListDrawable * StateListDrawable::setStateDrawable(Uint32 state, Drawable * drawable) {
	mDrawables[ state ] = drawable;
	return this;
}

bool StateListDrawable::hasDrawableState(Uint32 state ) {
	return mDrawables.find( state ) != mDrawables.end();
}

void StateListDrawable::setIsDrawableOwner(const bool & isOwner) {
	mDrawableOwner = isOwner;
}

const bool &StateListDrawable::isDrawableOwner() const {
	return mDrawableOwner;
}

}}
