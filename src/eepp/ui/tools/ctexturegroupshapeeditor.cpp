#include <eepp/ui/tools/ctexturegroupshapeeditor.hpp>
#include <eepp/ui/tools/ctexturegroupeditor.hpp>
#include <eepp/graphics/cprimitives.hpp>
#include <eepp/ui/cuimanager.hpp>

namespace EE { namespace UI { namespace Tools {

cTextureGroupShapeEditor::cTextureGroupShapeEditor( const cUIComplexControl::CreateParams& Params, cTextureGroupEditor * Editor ) :
	cUIComplexControl( Params ),
	mGfx( NULL ),
	mEditor( Editor )
{
	if ( NULL == cUIThemeManager::instance()->DefaultTheme() ) {
		return;
	}

	mTheme = cUIThemeManager::instance()->DefaultTheme();

	mGfx = mTheme->CreateGfx( NULL, this );

	cUIDragable::CreateParams DragParams;
	DragParams.Parent( this );
	DragParams.SizeSet( 500000, 500000 );
	mDrag = eeNew( cUIDragable, ( DragParams ) );
	mDrag->Enabled( true );
	mDrag->Visible( true );
	mDrag->DragEnable( true );

	GetCenter();
}

cTextureGroupShapeEditor::~cTextureGroupShapeEditor() {
}

void cTextureGroupShapeEditor::Draw() {
	cPrimitives P;
	P.SetColor( eeColorA( 255, 0, 0, mAlpha ) );
	P.DrawLine( mScreenPos.x, mScreenPos.y + mUICenter.y, mScreenPos.x + mSize.Width(), mScreenPos.y + mUICenter.y );
	P.DrawLine( mScreenPos.x + mUICenter.x, mScreenPos.y, mScreenPos.x + mUICenter.x, mScreenPos.y + mSize.Height() );

	cUIComplexControl::Draw();
}

void cTextureGroupShapeEditor::Update() {
	eeVector2i Pos = mDrag->Pos();

	cUIComplexControl::Update();

	if ( NULL != mGfx->Shape() && mDrag->DragEnable() && mDrag->Dragging() && Pos != mDrag->Pos() ) {
		eeVector2i Diff = -( Pos - mDrag->Pos() );

		mGfx->Shape()->OffsetX( mGfx->Shape()->OffsetX() + Diff.x );
		mGfx->Shape()->OffsetY( mGfx->Shape()->OffsetY() + Diff.y );

		mEditor->SpinOffX()->Value( mGfx->Shape()->OffsetX() );
		mEditor->SpinOffY()->Value( mGfx->Shape()->OffsetY() );
	}

	mGfx->Pos( mUICenter );
}

void cTextureGroupShapeEditor::OnSizeChange() {
	GetCenter();
}

cShape * cTextureGroupShapeEditor::Shape() const {
	return mGfx->Shape();
}

void cTextureGroupShapeEditor::Shape( cShape * shape ) {
	mGfx->Shape( shape );
}

cUIGfx * cTextureGroupShapeEditor::Gfx() const {
	return mGfx;
}

void cTextureGroupShapeEditor::GetCenter() {
	mUICenter = eeVector2i( mSize.Width() / 2, mSize.Height() / 2 );
}

}}}
