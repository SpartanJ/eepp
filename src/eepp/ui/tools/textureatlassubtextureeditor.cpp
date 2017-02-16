#include <eepp/ui/tools/textureatlassubtextureeditor.hpp>
#include <eepp/ui/tools/textureatlaseditor.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI { namespace Tools {

TextureAtlasSubTextureEditor::TextureAtlasSubTextureEditor( const UIComplexControl::CreateParams& Params, TextureAtlasEditor * Editor ) :
	UIComplexControl( Params ),
	mGfx( NULL ),
	mEditor( Editor )
{
	if ( NULL == UIThemeManager::instance()->DefaultTheme() ) {
		return;
	}

	mTheme = UIThemeManager::instance()->DefaultTheme();

	mGfx = mTheme->CreateGfx( NULL, this );

	UIDragable::CreateParams DragParams;
	DragParams.Parent( this );
	DragParams.SizeSet( 500000, 500000 );
	mDrag = eeNew( UIDragable, ( DragParams ) );
	mDrag->Enabled( true );
	mDrag->Visible( true );
	mDrag->DragEnable( true );

	GetCenter();
}

TextureAtlasSubTextureEditor::~TextureAtlasSubTextureEditor() {
}

void TextureAtlasSubTextureEditor::Draw() {
	Primitives P;
	P.setColor( ColorA( 255, 0, 0, mAlpha ) );
	P.drawLine( Line2f( Vector2f( mScreenPos.x, mScreenPos.y + mUICenter.y ), Vector2f( mScreenPos.x + mSize.width(), mScreenPos.y + mUICenter.y ) ) );
	P.drawLine( Line2f( Vector2f( mScreenPos.x + mUICenter.x, mScreenPos.y ), Vector2f( mScreenPos.x + mUICenter.x, mScreenPos.y + mSize.height() ) ) );

	UIComplexControl::Draw();
}

void TextureAtlasSubTextureEditor::Update() {
	Vector2i Pos = mDrag->Pos();

	UIComplexControl::Update();

	if ( NULL != mGfx->SubTexture() && mDrag->DragEnable() && mDrag->Dragging() && Pos != mDrag->Pos() ) {
		Vector2i Diff = -( Pos - mDrag->Pos() );

		mGfx->SubTexture()->offset( Vector2i( mGfx->SubTexture()->offset().x + Diff.x, mGfx->SubTexture()->offset().y + Diff.y ) );

		mEditor->SpinOffX()->Value( mGfx->SubTexture()->offset().x );
		mEditor->SpinOffY()->Value( mGfx->SubTexture()->offset().y );
	}

	mGfx->Pos( mUICenter );
}

void TextureAtlasSubTextureEditor::OnSizeChange() {
	GetCenter();
}

Graphics::SubTexture * TextureAtlasSubTextureEditor::SubTexture() const {
	return mGfx->SubTexture();
}

void TextureAtlasSubTextureEditor::SubTexture( Graphics::SubTexture * subTexture ) {
	mGfx->SubTexture( subTexture );
}

UIGfx * TextureAtlasSubTextureEditor::Gfx() const {
	return mGfx;
}

void TextureAtlasSubTextureEditor::GetCenter() {
	mUICenter = Vector2i( mSize.width() / 2, mSize.height() / 2 );
}

}}}
