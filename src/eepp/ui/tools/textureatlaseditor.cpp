#include <eepp/ui/tools/textureatlaseditor.hpp>
#include <eepp/ui/tools/textureatlassubtextureeditor.hpp>
#include <eepp/ui/tools/textureatlasnew.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uicommondialog.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>
#include <eepp/system/filesystem.hpp>
#include <algorithm>

namespace EE { namespace UI { namespace Tools {

UIWidget * TextureAtlasEditor::createTextureAtlasSubTextureEditor( std::string name ) {
	mSubTextureEditor = TextureAtlasSubTextureEditor::New( this );
	return mSubTextureEditor;
}

TextureAtlasEditor::TextureAtlasEditor( UIWindow * AttatchTo, const TGEditorCloseCb& callback ) :
	mUIWindow( AttatchTo ),
	mCloseCb( callback ),
	mTexturePacker( NULL ),
	mTextureAtlasLoader( NULL ),
	mCurSubTexture( NULL )
{
	if ( NULL == UIThemeManager::instance()->getDefaultTheme() ) {
		eePRINTL( "TextureAtlasEditor needs a default theme assigned to work." );
		return;
	}

	mTheme = UIThemeManager::instance()->getDefaultTheme();

	if ( NULL == mUIWindow ) {
		mUIWindow = UIManager::instance()->getMainControl();
		mUIWindow->setThemeSkin( mTheme, "winback" );
	}

	if ( UIManager::instance()->getMainControl() == mUIWindow ) {
		mUIContainer = mUIWindow;
	} else {
		mUIContainer = mUIWindow->getContainer();
	}

	std::string layout =
	"<LinearLayout orientation='vertical' layout_width='match_parent' layout_height='match_parent'>"
	"	<WinMenu layout_width='match_parent' layout_height='wrap_content'>"
	"		<Menu id='fileMenu' text='File'>"
	"			<item text='New...' icon='document-new' />"
	"			<item text='Open...' icon='document-open' />"
	"			<separator />"
	"			<item text='Save' icon='document-save' />"
	"			<separator />"
	"			<item text='Close' icon='document-close' />"
	"			<separator />"
	"			<item text='Quit' icon='quit' />"
	"		</Menu>"
	"	</WinMenu>"
	"	<LinearLayout layout_width='match_parent' layout_height='match_parent' orientation='horizontal'>"
	"		<TextureAtlasSubTextureEditor layout_width='match_parent' layout_height='match_parent' layout_weight='1' "
	"										flags='clip' backgroundColor='#00000032' borderWidth='1' borderColor='#000000FF' />"
	"		<LinearLayout orientation='vertical' layout_width='205dp' layout_height='match_parent' layout_marginLeft='8dp' layout_marginRight='8dp'>"
	"			<TextView text='SubTexture List:' fontStyle='shadow' layout_marginTop='4dp' layout_marginBottom='8dp' />"
	"			<ListBox id='SubTextureList' layout_width='match_parent' layout_height='144dp' />"
	"			<TextView text='Current SubTexture:' fontStyle='shadow' layout_marginTop='16dp' layout_marginBottom='16dp' />"
	"			<LinearLayout orientation='horizontal' layout_width='match_parent' layout_height='wrap_content'>"
	"				<TextView text='Offset X:' fontStyle='shadow' layout_width='match_parent' layout_height='wrap_content' layout_weight='1' "
	"								layout_marginRight='8dp' layout_gravity='center' gravity='right|center_vertical' />"
	"				<SpinBox id='offX' layout_width='100dp' layout_height='wrap_content' minValue='-32000' maxValue='32000' />"
	"			</LinearLayout>"
	"			<LinearLayout orientation='horizontal' layout_width='match_parent' layout_height='wrap_content'>"
	"				<TextView text='Offset Y:' fontStyle='shadow' layout_width='match_parent' layout_height='wrap_content' layout_weight='1' "
	"								layout_marginRight='8dp' layout_gravity='center' gravity='right|center_vertical' />"
	"				<SpinBox id='offY' layout_width='100dp' layout_height='wrap_content' minValue='-32000' maxValue='32000' />"
	"			</LinearLayout>"
	"			<LinearLayout orientation='horizontal' layout_width='match_parent' layout_height='wrap_content'>"
	"				<TextView text='Dest. Width' fontStyle='shadow' layout_width='match_parent' layout_height='wrap_content' layout_weight='1' "
	"								layout_marginRight='8dp' layout_gravity='center' gravity='right|center_vertical' />"
	"				<SpinBox id='destW' layout_width='100dp' layout_height='wrap_content' minValue='0' maxValue='32000' />"
	"			</LinearLayout>"
	"			<LinearLayout orientation='horizontal' layout_width='match_parent' layout_height='wrap_content'>"
	"				<TextView text='Dest. Height' fontStyle='shadow' layout_width='match_parent' layout_height='wrap_content' layout_weight='1' "
	"								layout_marginRight='8dp' layout_gravity='center' gravity='right|center_vertical' />"
	"				<SpinBox id='destH' layout_width='100dp' layout_height='wrap_content' minValue='0' maxValue='32000' />"
	"			</LinearLayout>"
	"			<PushButton id='resetDest' text='Reset Dest. Size' layout_width='match_parent' layout_height='wrap_content' layout_marginBottom='8dp' layout_marginTop='8dp' />"
	"			<PushButton id='resetOff' text='Reset Offset' layout_width='match_parent' layout_height='wrap_content' layout_marginBottom='8dp' />"
	"			<PushButton id='centerOff' text='Centered Offset' layout_width='match_parent' layout_height='wrap_content' layout_marginBottom='8dp' />"
	"			<PushButton id='hbotOff' text='Half-Bottom Offset' layout_width='match_parent' layout_height='wrap_content' layout_marginBottom='8dp' />"
	"		</LinearLayout>"
	"	</LinearLayout>"
	"</LinearLayout>";

	UIWidgetCreator::addCustomWidgetCallback( "TextureAtlasSubTextureEditor", cb::Make1( this, &TextureAtlasEditor::createTextureAtlasSubTextureEditor ) );

	UIManager::instance()->loadLayoutFromString( layout, mUIContainer );

	UIWidgetCreator::removeCustomWidgetCallback( "TextureAtlasSubTextureEditor" );

	mUIContainer->bind( "SubTextureList", mSubTextureList );
	mSubTextureList->addEventListener( UIEvent::OnItemSelected, cb::Make1( this, &TextureAtlasEditor::onSubTextureChange ) );

	mUIContainer->bind( "offX", mSpinOffX );
	mSpinOffX->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &TextureAtlasEditor::onOffXChange ) );

	mUIContainer->bind( "offY", mSpinOffY );
	mSpinOffY->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &TextureAtlasEditor::onOffYChange ) );

	mUIContainer->bind( "destW", mSpinDestW );
	mSpinDestW->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &TextureAtlasEditor::onDestWChange ) );

	mUIContainer->bind( "destH", mSpinDestH );
	mSpinDestH->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &TextureAtlasEditor::onDestHChange ) );

	mUIContainer->find<UIPushButton>( "resetDest" )->addEventListener( UIEvent::MouseClick, cb::Make1( this, &TextureAtlasEditor::onResetDestSize ) );

	mUIContainer->find<UIPushButton>( "resetOff" )->addEventListener( UIEvent::MouseClick, cb::Make1( this, &TextureAtlasEditor::onResetOffset ) );

	mUIContainer->find<UIPushButton>( "centerOff" )->addEventListener( UIEvent::MouseClick, cb::Make1( this, &TextureAtlasEditor::onCenterOffset ) );

	mUIContainer->find<UIPushButton>( "hbotOff" )->addEventListener( UIEvent::MouseClick, cb::Make1( this, &TextureAtlasEditor::onHBOffset ) );

	mUIContainer->find<UIPopUpMenu>("fileMenu")->addEventListener( UIEvent::OnItemClicked, cb::Make1( this, &TextureAtlasEditor::fileMenuClick ) );

	mUIWindow->setTitle( "Texture Atlas Editor" );
	mUIWindow->addEventListener( UIEvent::OnWindowClose, cb::Make1( this, &TextureAtlasEditor::windowClose ) );

	mTGEU = eeNew( UITGEUpdater, ( this ) );
}

TextureAtlasEditor::~TextureAtlasEditor() {
	eeSAFE_DELETE( mTexturePacker );
	eeSAFE_DELETE( mTextureAtlasLoader );

	if ( !UIManager::instance()->isShootingDown() ) {
		mTGEU->close();
	}
}

void TextureAtlasEditor::onResetDestSize( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( NULL != mCurSubTexture && MouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		Sizef RealSize( mCurSubTexture->getRealSize().getWidth() * mCurSubTexture->getPixelDensity(), mCurSubTexture->getRealSize().getHeight() * mCurSubTexture->getPixelDensity() );

		mCurSubTexture->setOriDestSize( Sizef( RealSize.x, RealSize.y ) );

		mSpinDestW->setValue( RealSize.getWidth() );
		mSpinDestH->setValue( RealSize.getHeight() );
	}
}

void TextureAtlasEditor::onResetOffset( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( NULL != mCurSubTexture && MouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		mSpinOffX->setValue( 0 );
		mSpinOffY->setValue( 0 );
	}
}

void TextureAtlasEditor::onCenterOffset( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( NULL != mCurSubTexture && MouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		Sizei NSize( -( (Int32)mCurSubTexture->getDpSize().x / 2 ), -( (Int32)mCurSubTexture->getDpSize().y / 2 ) );

		mSpinOffX->setValue( NSize.x );
		mSpinOffY->setValue( NSize.y );
	}
}

void TextureAtlasEditor::onHBOffset( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( NULL != mCurSubTexture && MouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		Sizei NSize( -( (Int32)mCurSubTexture->getDpSize().x / 2 ), -(Int32)mCurSubTexture->getDpSize().y );

		mSpinOffX->setValue( NSize.x );
		mSpinOffY->setValue( NSize.y );
	}
}

void TextureAtlasEditor::onOffXChange( const UIEvent * Event ) {
	if ( NULL != mCurSubTexture ) {
		mCurSubTexture->setOffset( Vector2i( (Int32)mSpinOffX->getValue(), mCurSubTexture->getOffset().y ) );
	}
}

void TextureAtlasEditor::onOffYChange( const UIEvent * Event ) {
	if ( NULL != mCurSubTexture ) {
		mCurSubTexture->setOffset( Vector2i( mCurSubTexture->getOffset().x, (Int32)mSpinOffY->getValue() ) );
	}
}

void TextureAtlasEditor::onDestWChange( const UIEvent * Event ) {
	if ( NULL != mCurSubTexture ) {
		mCurSubTexture->setOriDestSize( Sizef( (Int32)mSpinDestW->getValue(), mCurSubTexture->getDpSize().y ) );
		mSubTextureEditor->getGfx()->setSize( (Int32)mSpinDestW->getValue(), mSubTextureEditor->getGfx()->getSize().getHeight() );
	}
}

void TextureAtlasEditor::onDestHChange( const UIEvent * Event ) {
	if ( NULL != mCurSubTexture ) {
		mCurSubTexture->setOriDestSize( Sizef( mCurSubTexture->getDpSize().x, (Int32)mSpinDestH->getValue() ) );
		mSubTextureEditor->getGfx()->setSize( mSubTextureEditor->getGfx()->getSize().getWidth(), (Int32)mSpinDestH->getValue() );
	}
}

void TextureAtlasEditor::windowClose( const UIEvent * Event ) {
	if ( mCloseCb.IsSet() )
		mCloseCb();

	eeDelete( this );
}

void TextureAtlasEditor::fileMenuClick( const UIEvent * Event ) {
	if ( !Event->getControl()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getControl() )->getText();

	if ( "New..." == txt ) {
		eeNew( TextureAtlasNew, ( cb::Make1( this, &TextureAtlasEditor::onTextureAtlasCreate ) ) );
	} else if ( "Open..." == txt ) {
		UICommonDialog * TGDialog = UICommonDialog::New( UI_CDL_DEFAULT_FLAGS, std::string( "*" ) + EE_TEXTURE_ATLAS_EXTENSION );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Open Texture Atlas" );
		TGDialog->addEventListener( UIEvent::OpenFile, cb::Make1( this, &TextureAtlasEditor::openTextureAtlas ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Save" == txt ) {
		if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded() ) {
			mTextureAtlasLoader->updateTextureAtlas();
		}
	} else if ( "Close" == txt ) {
		if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded()  ) {
			UIMessageBox * MsgBox = UIMessageBox::New( MSGBOX_OKCANCEL, "Do you really want to close the current texture atlas?\nAll changes will be lost." );
			MsgBox->addEventListener( UIEvent::MsgBoxConfirmClick, cb::Make1( this, &TextureAtlasEditor::onTextureAtlasClose ) );
			MsgBox->setTitle( "Close Texture Atlas?" );
			MsgBox->center();
			MsgBox->show();
		} else {
			onTextureAtlasClose( NULL );
		}
	} else if ( "Quit" == txt ) {
		if ( mUIWindow == UIManager::instance()->getMainControl() ) {
			UIManager::instance()->getWindow()->close();
		} else {
			mUIWindow->closeWindow();
		}
	}
}

void TextureAtlasEditor::onTextureAtlasCreate( TexturePacker * TexPacker ) {
	eeSAFE_DELETE( mTexturePacker );
	mTexturePacker = TexPacker;

	eeSAFE_DELETE( mTextureAtlasLoader );

	std::string FPath( FileSystem::fileRemoveExtension( mTexturePacker->getFilepath() + EE_TEXTURE_ATLAS_EXTENSION ) );

	mTextureAtlasLoader = eeNew( TextureAtlasLoader, ( FPath, true, cb::Make1( this, &TextureAtlasEditor::onTextureAtlasLoaded ) ) );
}

void TextureAtlasEditor::updateControls() {
	if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded()  ) {
		fillSubTextureList();
	}
}

void TextureAtlasEditor::fillSubTextureList() {
	if ( NULL == mTextureAtlasLoader || NULL == mTextureAtlasLoader->getTextureAtlas() || !mTextureAtlasLoader->isLoaded()  )
		return;

	std::list<SubTexture*>& Res = mTextureAtlasLoader->getTextureAtlas()->getResources();

	mSubTextureList->clear();

	std::vector<String> items;

	for ( std::list<SubTexture*>::iterator it = Res.begin(); it != Res.end(); it++ ) {
			items.push_back( (*it)->getName() );
	}

	if ( items.size() ) {
		std::sort( items.begin(), items.end() );

		mSubTextureList->addListBoxItems( items );
		mSubTextureList->setSelected( 0 );
	}

	mSubTextureList->getVerticalScrollBar()->setClickStep( 8.f / (Float)mSubTextureList->getCount() );
}

void TextureAtlasEditor::onSubTextureChange( const UIEvent * Event ) {
	if ( NULL != mTextureAtlasLoader && NULL != mTextureAtlasLoader->getTextureAtlas() ) {
		mCurSubTexture = mTextureAtlasLoader->getTextureAtlas()->getByName( mSubTextureList->getItemSelectedText() );

		if ( NULL != mCurSubTexture ) {
			mSubTextureEditor->setSubTexture( mCurSubTexture );
			mSpinOffX->setValue( mCurSubTexture->getOffset().x );
			mSpinOffY->setValue( mCurSubTexture->getOffset().y );
			mSpinDestW->setValue( mCurSubTexture->getDpSize().x );
			mSpinDestH->setValue( mCurSubTexture->getDpSize().y );
		}
	}
}

void TextureAtlasEditor::update() {
	if ( NULL != mTextureAtlasLoader && !mTextureAtlasLoader->isLoaded() ) {
		mTextureAtlasLoader->update();
	}
}

void TextureAtlasEditor::openTextureAtlas( const UIEvent * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->getControl() );

	eeSAFE_DELETE( mTextureAtlasLoader );
	bool threaded = true;
	#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	threaded = false;
	#endif

	mTextureAtlasLoader = eeNew( TextureAtlasLoader, ( CDL->getFullPath(), threaded, cb::Make1( this, &TextureAtlasEditor::onTextureAtlasLoaded ) ) );
}

void TextureAtlasEditor::onTextureAtlasLoaded( TextureAtlasLoader * TGLoader ) {
	if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded() ) {
		updateControls();
	}
}

void TextureAtlasEditor::saveTextureAtlas( const UIEvent * Event ) {
	if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded() ) {
		mTextureAtlasLoader->updateTextureAtlas();
	}
}

void TextureAtlasEditor::onTextureAtlasClose( const UIEvent * Event ) {
	eeSAFE_DELETE( mTextureAtlasLoader );
	mSubTextureList->clear();
	mSpinOffX->setValue( 0 );
	mSpinOffY->setValue( 0 );
	mSpinDestW->setValue( 0 );
	mSpinDestH->setValue( 0 );
	mSubTextureEditor->setSubTexture( NULL );
	mCurSubTexture = NULL;
}

}}}
