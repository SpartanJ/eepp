#include <eepp/window/view.hpp>

namespace EE { namespace Window {

View::View() : mNeedUpdate(true) {
	mView.Left = 0;
	mView.Right = 0;
	mView.Top = 0;
	mView.Bottom = 0;

	calcCenter();
}

View::View( const int& X, const int& Y, const int& Width, const int& Height ) : mNeedUpdate(true) {
	setView( X, Y, Width, Height );
}

View::View( const Recti& View ) : mNeedUpdate(true) {
	mView = View;

	calcCenter();
}

View::~View() {
}

void View::setView( const int& X, const int& Y, const int& Width, const int& Height ) {
	mView.Left = X;
	mView.Top = Y;
	mView.Right = Width;
	mView.Bottom = Height;
	calcCenter();
	mNeedUpdate = true;
}

void View::calcCenter() {
	mCenter.x = ( ( mView.Left + mView.Right ) - mView.Left ) * 0.5f;
	mCenter.y = ( ( mView.Top + mView.Bottom ) - mView.Top ) * 0.5f;
}

Vector2i View::center() const {
	return Vector2i( (int)mCenter.x, (Int32)mCenter.y );
}

void View::center( const Vector2i& Center ) {
	mCenter.x = (Float)Center.x;
	mCenter.y = (Float)Center.y;
	mView.Left = static_cast<int> ( mCenter.x - (Float)mView.Right * 0.5f );
	mView.Top = static_cast<int> ( mCenter.y - (Float)mView.Bottom * 0.5f );

	mNeedUpdate = true;
}

void View::move( const int& OffsetX, const int& OffsetY ) {
	mView.Left += OffsetX;
	mView.Top += OffsetY;

	calcCenter();
	mNeedUpdate = true;
}

void View::move( const Vector2i& Offset ) {
	move( Offset.x, Offset.y );
}

void View::scale( const Vector2f& Factor ) {
	Vector2f v( mView.Right * 0.5f, mView.Bottom * 0.5f );

	mView.Left = mView.Left + static_cast<int> ( v.x - v.x * Factor.x );
	mView.Top = mView.Top + static_cast<int> ( v.y - v.y * Factor.y );
	mView.Right = static_cast<Int32>( (Float)mView.Right * Factor.x );
	mView.Bottom = static_cast<Int32>( (Float)mView.Bottom * Factor.y );

	calcCenter();
	mNeedUpdate = true;
}

void View::scale( const Float& Factor ) {
	scale( Vector2f( Factor, Factor ) );
}

void View::setPosition( const int& X, const int& Y ) {
	mView.Left = X;
	mView.Top = Y;

	calcCenter();
	mNeedUpdate = true;
}

void View::setSize( const int& Width, const int& Height ) {
	mView.Right = Width;
	mView.Bottom = Height;

	calcCenter();
	mNeedUpdate = true;
}

bool View::needUpdate() const {
	bool Need = mNeedUpdate;

	if ( Need )
		const_cast<View*>(this)->mNeedUpdate = false;

	return Need;
}

}}
