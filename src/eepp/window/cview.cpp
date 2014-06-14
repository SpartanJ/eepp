#include <eepp/window/cview.hpp>

namespace EE { namespace Window {

cView::cView() : mNeedUpdate(true) {
	mView.Left = 0;
	mView.Right = 0;
	mView.Top = 0;
	mView.Bottom = 0;

	CalcCenter();
}

cView::cView( const int& X, const int& Y, const int& Width, const int& Height ) : mNeedUpdate(true) {
	SetView( X, Y, Width, Height );
}

cView::cView( const Recti& View ) : mNeedUpdate(true) {
	mView = View;

	CalcCenter();
}

cView::~cView() {
}

void cView::SetView( const int& X, const int& Y, const int& Width, const int& Height ) {
	mView.Left = X;
	mView.Top = Y;
	mView.Right = Width;
	mView.Bottom = Height;
	CalcCenter();
	mNeedUpdate = true;
}

void cView::CalcCenter() {
	mCenter.x = ( ( mView.Left + mView.Right ) - mView.Left ) * 0.5f;
	mCenter.y = ( ( mView.Top + mView.Bottom ) - mView.Top ) * 0.5f;
}

Vector2i cView::Center() const {
	return Vector2i( (int)mCenter.x, (Int32)mCenter.y );
}

void cView::Center( const Vector2i& Center ) {
	mCenter.x = (Float)Center.x;
	mCenter.y = (Float)Center.y;
	mView.Left = static_cast<int> ( mCenter.x - (Float)mView.Right * 0.5f );
	mView.Top = static_cast<int> ( mCenter.y - (Float)mView.Bottom * 0.5f );

	mNeedUpdate = true;
}

void cView::Move( const int& OffsetX, const int& OffsetY ) {
	mView.Left += OffsetX;
	mView.Top += OffsetY;

	CalcCenter();
	mNeedUpdate = true;
}

void cView::Move( const Vector2i& Offset ) {
	Move( Offset.x, Offset.y );
}

void cView::Scale( const Vector2f& Factor ) {
	Vector2f v( mView.Right * 0.5f, mView.Bottom * 0.5f );

	mView.Left = mView.Left + static_cast<int> ( v.x - v.x * Factor.x );
	mView.Top = mView.Top + static_cast<int> ( v.y - v.y * Factor.y );
	mView.Right = static_cast<Int32>( (Float)mView.Right * Factor.x );
	mView.Bottom = static_cast<Int32>( (Float)mView.Bottom * Factor.y );

	CalcCenter();
	mNeedUpdate = true;
}

void cView::Scale( const Float& Factor ) {
	Scale( Vector2f( Factor, Factor ) );
}

void cView::SetPosition( const int& X, const int& Y ) {
	mView.Left = X;
	mView.Top = Y;

	CalcCenter();
	mNeedUpdate = true;
}

void cView::SetSize( const int& Width, const int& Height ) {
	mView.Right = Width;
	mView.Bottom = Height;

	CalcCenter();
	mNeedUpdate = true;
}

bool cView::NeedUpdate() const {
	bool Need = mNeedUpdate;

	if ( Need )
		const_cast<cView*>(this)->mNeedUpdate = false;

	return Need;
}

}}
