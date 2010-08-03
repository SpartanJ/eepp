#include "cview.hpp"

namespace EE { namespace Window {

cView::cView() : mNeedUpdate(true) {
	mView.Left = 0;
	mView.Right = 0;
	mView.Top = 0;
	mView.Bottom = 0;

	CalcCenter();
}

cView::cView( const eeInt& X, const eeInt& Y, const eeInt& Width, const eeInt& Height ) : mNeedUpdate(true) {
	SetView( X, Y, Width, Height );
}

cView::cView( const eeRecti& View ) : mNeedUpdate(true) {
	mView = View;

	CalcCenter();
}

cView::~cView() {
}

void cView::SetView( const eeInt& X, const eeInt& Y, const eeInt& Width, const eeInt& Height ) {
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

eeVector2i cView::Center() const {
	return eeVector2i( (eeInt)mCenter.x, (Int32)mCenter.y );
}

void cView::Center( const eeVector2i& Center ) {
	eeVector2f LastCenter = mCenter;

	mCenter.x = (eeFloat)Center.x;
	mCenter.y = (eeFloat)Center.y;
	mView.Left = static_cast<eeInt> ( mCenter.x - (eeFloat)mView.Right * 0.5f );
	mView.Top = static_cast<eeInt> ( mCenter.y - (eeFloat)mView.Bottom * 0.5f );

	mNeedUpdate = true;
}

void cView::Move( const eeInt& OffsetX, const eeInt& OffsetY ) {
	mView.Left += OffsetX;
	mView.Top += OffsetY;

	CalcCenter();
	mNeedUpdate = true;
}

void cView::Move( const eeVector2i& Offset ) {
	Move( Offset.x, Offset.y );
}

void cView::Scale( const eeFloat& Factor ) {
	eeRecti LastView = mView;
	eeVector2f v( mView.Right * 0.5f, mView.Bottom * 0.5f );

	mView.Left = mView.Left + static_cast<eeInt> ( v.x - v.x * Factor );
	mView.Top = mView.Top + static_cast<eeInt> ( v.y - v.y * Factor );
	mView.Right = static_cast<Int32>( (eeFloat)mView.Right * Factor );
	mView.Bottom = static_cast<Int32>( (eeFloat)mView.Bottom * Factor );

	CalcCenter();
	mNeedUpdate = true;
}

void cView::SetPosition( const eeInt& X, const eeInt& Y ) {
	mView.Left = X;
	mView.Top = Y;

	CalcCenter();
	mNeedUpdate = true;
}

void cView::SetSize( const eeInt& Width, const eeInt& Height ) {
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
