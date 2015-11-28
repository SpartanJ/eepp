#include <eepp/window/view.hpp>

namespace EE { namespace Window {

View::View() : mNeedUpdate(true) {
	mView.Left = 0;
	mView.Right = 0;
	mView.Top = 0;
	mView.Bottom = 0;

	CalcCenter();
}

View::View( const int& X, const int& Y, const int& Width, const int& Height ) : mNeedUpdate(true) {
	SetView( X, Y, Width, Height );
}

View::View( const Recti& View ) : mNeedUpdate(true) {
	mView = View;

	CalcCenter();
}

View::~View() {
}

void View::SetView( const int& X, const int& Y, const int& Width, const int& Height ) {
	mView.Left = X;
	mView.Top = Y;
	mView.Right = Width;
	mView.Bottom = Height;
	CalcCenter();
	mNeedUpdate = true;
}

void View::CalcCenter() {
	mCenter.x = ( ( mView.Left + mView.Right ) - mView.Left ) * 0.5f;
	mCenter.y = ( ( mView.Top + mView.Bottom ) - mView.Top ) * 0.5f;
}

Vector2i View::Center() const {
	return Vector2i( (int)mCenter.x, (Int32)mCenter.y );
}

void View::Center( const Vector2i& Center ) {
	mCenter.x = (Float)Center.x;
	mCenter.y = (Float)Center.y;
	mView.Left = static_cast<int> ( mCenter.x - (Float)mView.Right * 0.5f );
	mView.Top = static_cast<int> ( mCenter.y - (Float)mView.Bottom * 0.5f );

	mNeedUpdate = true;
}

void View::Move( const int& OffsetX, const int& OffsetY ) {
	mView.Left += OffsetX;
	mView.Top += OffsetY;

	CalcCenter();
	mNeedUpdate = true;
}

void View::Move( const Vector2i& Offset ) {
	Move( Offset.x, Offset.y );
}

void View::Scale( const Vector2f& Factor ) {
	Vector2f v( mView.Right * 0.5f, mView.Bottom * 0.5f );

	mView.Left = mView.Left + static_cast<int> ( v.x - v.x * Factor.x );
	mView.Top = mView.Top + static_cast<int> ( v.y - v.y * Factor.y );
	mView.Right = static_cast<Int32>( (Float)mView.Right * Factor.x );
	mView.Bottom = static_cast<Int32>( (Float)mView.Bottom * Factor.y );

	CalcCenter();
	mNeedUpdate = true;
}

void View::Scale( const Float& Factor ) {
	Scale( Vector2f( Factor, Factor ) );
}

void View::SetPosition( const int& X, const int& Y ) {
	mView.Left = X;
	mView.Top = Y;

	CalcCenter();
	mNeedUpdate = true;
}

void View::SetSize( const int& Width, const int& Height ) {
	mView.Right = Width;
	mView.Bottom = Height;

	CalcCenter();
	mNeedUpdate = true;
}

bool View::NeedUpdate() const {
	bool Need = mNeedUpdate;

	if ( Need )
		const_cast<View*>(this)->mNeedUpdate = false;

	return Need;
}

}}
