#include <eepp/graphics/drawableresource.hpp>

namespace EE { namespace Graphics {

DrawableResourceConnection::DrawableResourceConnection(
	std::weak_ptr<DrawableResourceCallbackState> state, Uint32 id ) :
	mState( std::move( state ) ), mId( id ) {}

DrawableResourceConnection::~DrawableResourceConnection() {
	disconnect();
}

DrawableResourceConnection::DrawableResourceConnection(
	DrawableResourceConnection&& other ) noexcept :
	mState( std::move( other.mState ) ), mId( other.mId ) {
	other.mId = 0;
}

DrawableResourceConnection&
DrawableResourceConnection::operator=( DrawableResourceConnection&& other ) noexcept {
	if ( this != &other ) {
		disconnect();
		mState = std::move( other.mState );
		mId = other.mId;
		other.mId = 0;
	}
	return *this;
}

void DrawableResourceConnection::disconnect() {
	if ( mId != 0 ) {
		if ( auto state = mState.lock() )
			state->callbacks.erase( mId );
	}
	mState.reset();
	mId = 0;
}

DrawableResourceConnection::operator bool() const {
	return mId != 0 && !mState.expired();
}

DrawableResource::DrawableResource( Type drawableType ) :
	Drawable( drawableType ),
	mId( 0 ),
	mCallbackState( std::make_shared<DrawableResourceCallbackState>() ) {
	createUnnamed();
}

DrawableResource::DrawableResource( Type drawableType, const std::string& name ) :
	Drawable( drawableType ),
	mId( 0 ),
	mCallbackState( std::make_shared<DrawableResourceCallbackState>() ) {
	setName( name );
}

DrawableResource::~DrawableResource() {}

const String::HashType& DrawableResource::getId() const {
	return mId;
}

const std::string DrawableResource::getName() const {
	return mName;
}

void DrawableResource::setName( const std::string& name ) {
	mName = name;
	mId = String::hash( mName );
}

void DrawableResource::createUnnamed() {
	if ( !mName.size() )
		setName( std::string( "unnamed" ) );
}

bool DrawableResource::isDrawableResource() const {
	return true;
}

void DrawableResource::onResourceChange() {
	sendResourceChanged();
}

void DrawableResource::sendResourceChanged() {
	std::vector<OnResourceChangeCallback> callbacks;
	callbacks.reserve( mCallbackState->callbacks.size() );
	for ( const auto& callback : mCallbackState->callbacks )
		callbacks.emplace_back( callback.second );
	for ( const auto& callback : callbacks )
		callback( *this );
}

DrawableResourceConnection
DrawableResource::connectResourceChange( OnResourceChangeCallback callback ) {
	Uint32 id = ++mCallbackState->nextId;
	mCallbackState->callbacks.emplace( id, std::move( callback ) );
	return DrawableResourceConnection( mCallbackState, id );
}

}} // namespace EE::Graphics
