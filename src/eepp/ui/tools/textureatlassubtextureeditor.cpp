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
	if ( NULL == UIThemeManager::instance()->defaultTheme() ) {
		return;
	}

	mTheme = UIThemeManager::instance()->defaultTheme();

	mGfx = mTheme->createGfx( NULL, this );

	UIDragable::CreateParams DragParams;
	DragParams.setParent( this );
	DragParams.setSize( 500000, 500000 );
	mDrag = eeNew( UIDragable, ( DragParams ) );
	mDrag->enabled( true );
	mDrag->visible( true );
	mDrag->dragEnable( true );

	getCenter();
}

TextureAtlasSubTextureEditor::~TextureAtlasSubTextureEditor() {
}

void TextureAtlasSubTextureEditor::draw() {
	Primitives P;
	P.setColor( ColorA( 255, 0, 0, mAlpha ) );
	P.drawLine( Line2f( Vector2f( mScreenPos.x, mScreenPos.y + mUICenter.y ), Vector2f( mScreenPos.x + mSize.width(), mScreenPos.y + mUICenter.y ) ) );
	P.drawLine( Line2f( Vector2f( mScreenPos.x + mUICenter.x, mScreenPos.y ), Vector2f( mScreenPos.x + mUICenter.x, mScreenPos.y + mSize.height() ) ) );

	UIComplexControl::draw();
}

void TextureAtlasSubTextureEditor::update() {
	Vector2i Pos = mDrag->position();

	UIComplexControl::update();

	if ( NULL != mGfx->subTexture() && mDrag->dragEnable() && mDrag->dragging() && Pos != mDrag->position() ) {
		Vector2i Diff = -( Pos - mDrag->position() );

		mGfx->subTexture()->offset( Vector2i( mGfx->subTexture()->offset().x + Diff.x, mGfx->subTexture()->offset().y + Diff.y ) );

		mEditor->spinOffX()->value( mGfx->subTexture()->offset().x );
		mEditor->spinOffY()->value( mGfx->subTexture()->offset().y );
	}

	mGfx->position( mUICenter );
}

void TextureAtlasSubTextureEditor::onSizeChange() {
	getCenter();
}

Graphics::SubTexture * TextureAtlasSubTextureEditor::subTexture() const {
	return mGfx->subTexture();
}

void TextureAtlasSubTextureEditor::subTexture( Graphics::SubTexture * subTexture ) {
	mGfx->subTexture( subTexture );
}

UIGfx * TextureAtlasSubTextureEditor::getGfx() const {
	return mGfx;
}

void TextureAtlasSubTextureEditor::getCenter() {
	mUICenter = Vector2i( mSize.width() / 2, mSize.height() / 2 );
}

}}}
