#include "settingsactions.hpp"
#include "ecode.hpp"
#include "version.hpp"

namespace ecode {

void SettingsActions::checkForUpdatesResponse( Http::Response&& response, bool fromStartup ) {
	auto updatesError = [this, fromStartup]() {
		if ( fromStartup )
			return;
		UIMessageBox* msg = UIMessageBox::New(
			UIMessageBox::OK, i18n( "error_checking_version", "Failed checking for updates." ) );
		msg->setTitle( "Error" );
		msg->setCloseShortcut( { KEY_ESCAPE, 0 } );
		msg->showWhenReady();
	};

	if ( response.getStatus() != Http::Response::Status::Ok || response.getBody().empty() ) {
		updatesError();
		return;
	}

	auto addStartUpCheckbox = [this]( UIMessageBox* msg ) {
		msg->setId( "check_for_updates" );
		msg->on( Event::OnWindowReady, [this, msg]( const Event* ) {
			msg->setVisible( false );
			UICheckBox* cbox = UICheckBox::New();
			cbox->addClass( "check_at_startup" );
			cbox->setParent( msg->getLayoutCont()->getFirstChild() );
			cbox->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );
			cbox->setText( i18n( "check_for_new_updates_at_startup",
								 "Always check for new updates at startup." ) );
			cbox->setChecked( mApp->getConfig().workspace.checkForUpdatesAtStartup );
			cbox->toPosition( 1 );
			cbox->runOnMainThread( [msg]() {
				msg->setMinWindowSize( msg->getLayoutCont()->getSize() );
				msg->center();
				msg->show();
			} );
			cbox->on( Event::OnValueChange, [this, cbox]( const Event* ) {
				mApp->getConfig().workspace.checkForUpdatesAtStartup = cbox->isChecked();
			} );
		} );
	};

	nlohmann::json j;
	try {
		j = nlohmann::json::parse( response.getBody(), nullptr, true, true );

		if ( j.contains( "tag_name" ) ) {
			auto tagName( j["tag_name"].get<std::string>() );
			auto versionNum = ecode::Version::getVersionNumFromTag( tagName );
			if ( versionNum > ecode::Version::getVersionNum() ) {
				auto name( j.value( "name", tagName ) );
				UIMessageBox* msg = UIMessageBox::New(
					UIMessageBox::OK_CANCEL,
					name + i18n( "ecode_updates_available",
								 " is available!\nDo you want to download it now?" )
							   .unescape() );

				auto url( j.value( "html_url", "https://github.com/SpartanJ/ecode/releases/" ) );
				msg->on( Event::OnConfirm, [url, msg]( const Event* ) {
					Engine::instance()->openURI( url );
					msg->closeWindow();
				} );
				msg->setTitle( "ecode" );
				msg->setCloseShortcut( { KEY_ESCAPE, 0 } );
				addStartUpCheckbox( msg );
			} else if ( versionNum < ecode::Version::getVersionNum() ) {
				if ( fromStartup )
					return;
				UIMessageBox* msg = UIMessageBox::New(
					UIMessageBox::OK,
					i18n( "ecode_unreleased_version",
						  "You are running an unreleased version of ecode!\nCurrent version: " )
							.unescape() +
						ecode::Version::getVersionNumString() );
				msg->setTitle( "ecode" );
				msg->setCloseShortcut( { KEY_ESCAPE, 0 } );
				addStartUpCheckbox( msg );
			} else {
				if ( fromStartup )
					return;
				UIMessageBox* msg = UIMessageBox::New(
					UIMessageBox::OK, i18n( "ecode_no_updates_available",
											"There are currently no updates available." ) );
				msg->setTitle( "ecode" );
				msg->setCloseShortcut( { KEY_ESCAPE, 0 } );
				addStartUpCheckbox( msg );
			}
		} else {
			updatesError();
		}
	} catch ( ... ) {
		updatesError();
	}
}

void SettingsActions::checkForUpdates( bool fromStartup ) {
	Http::getAsync(
		[this, fromStartup]( const Http&, Http::Request&, Http::Response& response ) {
			if ( !SceneManager::isActive() )
				return;
			mApp->getUISceneNode()->runOnMainThread( [this, res = response, fromStartup]() mutable {
				checkForUpdatesResponse( std::move( res ), fromStartup );
			} );
		},
		"https://api.github.com/repos/SpartanJ/ecode/releases/latest", Seconds( 30 ) );
}

void SettingsActions::aboutEcode() {
	String msg( ecode::Version::getVersionFullName() + " (codename: \"" +
				ecode::Version::getCodename() + "\")" );
	UIMessageBox* msgBox = UIMessageBox::New( UIMessageBox::OK, msg );
	UIImage* image = UIImage::New();
	image->setParent( msgBox->getContainer()->getFirstChild() );
	auto tf = TextureFactory::instance();
	auto resourceScope = mApp->getUISceneNode()->getResourceScope();
	TexturePtr tex = resourceScope->findTexture( "ecode-logo" );
	if ( tex == nullptr ) {
		tex = tf->loadFromFile( mApp->resPath() + "icon/ecode.png" );
		if ( tex ) {
			tex->setName( "ecode-logo" );
			resourceScope->publishLocal( "ecode-logo", tex );
		}
	}
	image->setDrawable( std::move( tex ) );
	image->setLayoutGravity( UI_NODE_ALIGN_CENTER );
	image->setGravity( UI_NODE_ALIGN_CENTER );
	image->setScaleType( UIScaleType::FitInside );
	image->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	image->setSize( { 128, 128 } );
	image->toBack();
	msgBox->setTitle( i18n( "about_ecode", "About ecode" ) );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->showWhenReady();
}

void SettingsActions::ecodeSource() {
	Engine::instance()->openURI( "https://github.com/SpartanJ/ecode" );
}

void SettingsActions::setEditorFontSize( const StyleSheetLength& size ) {
	mApp->getConfig().editor.fontSize = size;
	const Float fontSize = size.asPixels( 0, Sizef(), mApp->getDisplayDPI() );
	mApp->getSplitter()->forEachEditor(
		[fontSize]( UICodeEditor* editor ) { editor->setFontSize( fontSize ); } );
}

void SettingsActions::setTerminalFontSize( const StyleSheetLength& size ) {
	mApp->getConfig().term.fontSize = size;
	const Float fontSize = size.asPixels( 0, Sizef(), mApp->getDisplayDPI() );
	mApp->getSplitter()->forEachWidget( [fontSize]( UIWidget* widget ) {
		if ( widget && widget->isType( UI_TYPE_TERMINAL ) )
			widget->asType<UITerminal>()->setFontSize( fontSize );
	} );
}

void SettingsActions::setUIFontSize( const StyleSheetLength& size ) {
	mApp->getConfig().ui.fontSize = size;
	const Float fontSize = size.asPixels( 0, Sizef(), mApp->getDisplayDPI() );
	UIThemeManager* manager = mApp->getUISceneNode()->getUIThemeManager();
	manager->setDefaultFontSize( fontSize );
	manager->getDefaultTheme()->setDefaultFontSize( fontSize );
	mApp->getUISceneNode()->forEachNode( [this]( Node* node ) {
		if ( node->isType( UI_TYPE_TEXTVIEW ) ) {
			UITextView* textView = node->asType<UITextView>();
			if ( !textView->getUIStyle()->hasProperty( PropertyId::FontSize ) ) {
				textView->setFontSize( mApp->getConfig().ui.fontSize.asPixels(
					node->getParent()->getPixelsSize().getWidth(), Sizef(),
					mApp->getUISceneNode()->getDPI() ) );
			}
		}
	} );
	setUIPanelFontSize( mApp->getConfig().ui.panelFontSize );
}

void SettingsActions::setUIPanelFontSize( const StyleSheetLength& size ) {
	mApp->getConfig().ui.panelFontSize = size;
	auto selsFound = mApp->getUISceneNode()->getStyleSheet().findStyleFromSelectorName(
		"#project_view > treeview::row > treeview::cell" );
	if ( !selsFound.empty() ) {
		for ( auto sel : selsFound )
			sel->updatePropertyValue( "font-size", size.toString() );
		mApp->getUISceneNode()->getStyleSheet().refreshCacheFromStyles( selsFound );
	}

	UITreeView* treeView = mApp->getUISceneNode()->find<UITreeView>( "project_view" );
	if ( treeView ) {
		treeView->reloadStyle( true, true, true, true );
		treeView->updateContentSize();
	}
}

void SettingsActions::setScreenshotSavePath() {
	std::string initialPath = mApp->getConfig().screenshot.savePath;
	if ( !FileSystem::isDirectory( initialPath ) ) {
		initialPath = mApp->getDefaultScreenshotPath();
		FileSystem::makeDir( initialPath, true );
	}
	UIFileDialog* dialog = UIFileDialog::New(
		UIFileDialog::DefaultFlags | UIFileDialog::AllowFolderSelect |
			UIFileDialog::ShowOnlyFolders |
			( mApp->getConfig().ui.nativeFileDialogs ? UIFileDialog::UseNativeFileDialog : 0 ),
		"*", initialPath );
	dialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( i18n( "screenshot_save_path", "Screenshot Save Path" ) );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->on( Event::OpenFile, [this]( const Event* event ) {
		std::string path = event->getNode()->asType<UIFileDialog>()->getFullPath();
		if ( FileSystem::isDirectory( path ) ) {
			FileSystem::dirAddSlashAtEnd( path );
			mApp->getConfig().screenshot.savePath = std::move( path );
			mApp->saveConfig();
		}
	} );
	dialog->center();
	dialog->show();
}

String SettingsActions::i18n( const std::string& key, const String& def ) {
	return mApp->i18n( key, def );
}

} // namespace ecode
