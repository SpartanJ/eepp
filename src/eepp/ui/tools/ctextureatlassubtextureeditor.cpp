#include <eepp/ui/tools/ctextureatlassubtextureeditor.hpp>
#include <eepp/ui/tools/ctextureatlaseditor.hpp>
#include <eepp/graphics/cprimitives.hpp>
#include <eepp/ui/cuimanager.hpp>

namespace EE { namespace UI { namespace Tools {

cTextureAtlasSubTextureEditor::cTextureAtlasSubTextureEditor( const cUIComplexControl::CreateParams& Params, cTextureAtlasEditor * Editor ) :
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

cTextureAtlasSubTextureEditor::~cTextureAtlasSubTextureEditor() {
}

void cTextureAtlasSubTextureEditor::Draw() {
	cPrimitives P;
	P.SetColor( ColorA( 255, 0, 0, mAlpha ) );
	P.DrawLine( Line2f( Vector2f( mScreenPos.x, mScreenPos.y + mUICenter.y ), Vector2f( mScreenPos.x + mSize.Width(), mScreenPos.y + mUICenter.y ) ) );
	P.DrawLine( Line2f( Vector2f( mScreenPos.x + mUICenter.x, mScreenPos.y ), Vector2f( mScreenPos.x + mUICenter.x, mScreenPos.y + mSize.Height() ) ) );

	cUIComplexControl::Draw();
}

void cTextureAtlasSubTextureEditor::Update() {
	Vector2i Pos = mDrag->Pos();

	cUIComplexControl::Update();

	if ( NULL != mGfx->SubTexture() && mDrag->DragEnable() && mDrag->Dragging() && Pos != mDrag->Pos() ) {
		Vector2i Diff = -( Pos - mDrag->Pos() );

		mGfx->SubTexture()->Offset( Vector2i( mGfx->SubTexture()->Offset().x + Diff.x, mGfx->SubTexture()->Offset().y + Diff.y ) );

		mEditor->SpinOffX()->Value( mGfx->SubTexture()->Offset().x );
		mEditor->SpinOffY()->Value( mGfx->SubTexture()->Offset().y );
	}

	mGfx->Pos( mUICenter );
}

void cTextureAtlasSubTextureEditor::OnSizeChange() {
	GetCenter();
}

cSubTexture * cTextureAtlasSubTextureEditor::SubTexture() const {
	return mGfx->SubTexture();
}

void cTextureAtlasSubTextureEditor::SubTexture( cSubTexture * subTexture ) {
	mGfx->SubTexture( subTexture );
}

cUIGfx * cTextureAtlasSubTextureEditor::Gfx() const {
	return mGfx;
}

void cTextureAtlasSubTextureEditor::GetCenter() {
	mUICenter = Vector2i( mSize.Width() / 2, mSize.Height() / 2 );
}

}}}
