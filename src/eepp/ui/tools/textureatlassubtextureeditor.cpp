#include <eepp/ui/tools/textureatlassubtextureeditor.hpp>
#include <eepp/ui/tools/textureatlaseditor.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI { namespace Tools {

TextureAtlasSubTextureEditor * TextureAtlasSubTextureEditor::TextureAtlasSubTextureEditor::New( TextureAtlasEditor * Editor ) {
	return eeNew( TextureAtlasSubTextureEditor, ( Editor ) );
}

TextureAtlasSubTextureEditor::TextureAtlasSubTextureEditor( TextureAtlasEditor * Editor ) :
	UIWidget(),
	mGfx( NULL ),
	mEditor( Editor )
{
	if ( NULL == UIThemeManager::instance()->getDefaultTheme() ) {
		return;
	}

	mTheme = UIThemeManager::instance()->getDefaultTheme();

	mGfx = UISubTexture::New();
	mGfx->setParent( this );
	mGfx->setVisible( true );
	mGfx->setEnabled( true );

	mDrag = UIDragableControl::New();
	mDrag->setParent( this );
	mDrag->setSize( 64000, 64000 );
	mDrag->setEnabled( true );
	mDrag->setVisible( true );
	mDrag->setDragEnabled( true );
	mDrag->center();

	getCenter();
}

TextureAtlasSubTextureEditor::~TextureAtlasSubTextureEditor() {
}

void TextureAtlasSubTextureEditor::draw() {
	Primitives P;
	P.setColor( ColorA( 255, 0, 0, mAlpha ) );
	P.setLineWidth( PixelDensity::dpToPx( 1.f ) );

	Vector2i uiCenterPx = PixelDensity::dpToPxI( mUICenter );

	P.drawLine( Line2f( Vector2f( mScreenPos.x, mScreenPos.y + uiCenterPx.y ), Vector2f( mScreenPos.x + mRealSize.getWidth(), mScreenPos.y + uiCenterPx.y ) ) );
	P.drawLine( Line2f( Vector2f( mScreenPos.x + uiCenterPx.x, mScreenPos.y ), Vector2f( mScreenPos.x + uiCenterPx.x, mScreenPos.y + mRealSize.getHeight() ) ) );

	UIWidget::draw();
}

void TextureAtlasSubTextureEditor::update() {
	Vector2i Pos = mDrag->getRealPosition();

	UIWidget::update();

	if ( NULL != mGfx->getSubTexture() && mDrag->isDragEnabled() && mDrag->isDragging() && Pos != mDrag->getRealPosition() ) {
		Vector2i Diff = -( Pos - mDrag->getRealPosition() );

		Diff = PixelDensity::pxToDpI( Diff );

		mGfx->getSubTexture()->setOffset( Vector2i( mGfx->getSubTexture()->getOffset().x + Diff.x, mGfx->getSubTexture()->getOffset().y + Diff.y ) );

		mEditor->getSpinOffX()->setValue( mGfx->getSubTexture()->getOffset().x );
		mEditor->getSpinOffY()->setValue( mGfx->getSubTexture()->getOffset().y );
	}

	mGfx->setPosition( mUICenter );
}

void TextureAtlasSubTextureEditor::onSizeChange() {
	getCenter();
}

Graphics::SubTexture * TextureAtlasSubTextureEditor::getSubTexture() const {
	return mGfx->getSubTexture();
}

void TextureAtlasSubTextureEditor::setSubTexture( Graphics::SubTexture * subTexture ) {
	mGfx->setSubTexture( subTexture );
}

UISubTexture * TextureAtlasSubTextureEditor::getGfx() const {
	return mGfx;
}

void TextureAtlasSubTextureEditor::getCenter() {
	mUICenter = Vector2i( mSize.getWidth() / 2, mSize.getHeight() / 2 );
}

}}}
