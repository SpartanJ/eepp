#include "settingsactions.hpp"
#include "ecode.hpp"
#include "version.hpp"

using namespace std::string_literals;

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
	Texture* tex = tf->getByName( "ecode-logo" );
	if ( tex == nullptr ) {
		tex = tf->loadFromFile( mApp->resPath() + "icon/ecode.png" );
		if ( tex )
			tex->setName( "ecode-logo" );
	}
	image->setDrawable( tex );
	image->setLayoutGravity( UI_NODE_ALIGN_CENTER );
	image->setGravity( UI_NODE_ALIGN_CENTER );
	image->setScaleType( UIScaleType::FitInside );
	image->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	image->setSize( { 128, 128 } );
	image->toBack();
	msgBox->setTitle( i18n( "about_ecode", "About ecode..." ) );
	msgBox->showWhenReady();
}

void SettingsActions::ecodeSource() {
	Engine::instance()->openURI( "https://github.com/SpartanJ/ecode" );
}

void SettingsActions::setLineBreakingColumn() {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT,
		i18n( "set_line_breaking_column", "Set Line Breaking Column:\nSet 0 to disable it.\n" )
			.unescape() );
	msgBox->setTitle( mApp->getWindowTitle() );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->getTextInput()->setAllowOnlyNumbers( true, false );
	msgBox->getTextInput()->setText( String::toString( mApp->getConfig().doc.lineBreakingColumn ) );
	msgBox->showWhenReady();
	msgBox->on( Event::OnConfirm, [this, msgBox]( const Event* ) {
		int val;
		if ( String::fromString( val, msgBox->getTextInput()->getText() ) && val >= 0 ) {
			mApp->getConfig().doc.lineBreakingColumn = val;
			mApp->getSplitter()->forEachEditor(
				[val]( UICodeEditor* editor ) { editor->setLineBreakingColumn( val ); } );
			msgBox->closeWindow();
		}
	} );
	mApp->setFocusEditorOnClose( msgBox );
}

void SettingsActions::setLineSpacing() {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT,
		i18n( "set_line_spacing", "Set Line Spacing:\nSet 0 to disable it.\n" ).unescape() );
	msgBox->setTitle( mApp->getWindowTitle() );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->getTextInput()->setText( mApp->getConfig().editor.lineSpacing.toString() );
	msgBox->showWhenReady();
	msgBox->on( Event::OnConfirm, [this, msgBox]( const Event* ) {
		mApp->getConfig().editor.lineSpacing =
			StyleSheetLength( msgBox->getTextInput()->getText() );
		mApp->getSplitter()->forEachEditor( [this]( UICodeEditor* editor ) {
			editor->setLineSpacing( mApp->getConfig().editor.lineSpacing );
		} );
	} );
	mApp->setFocusEditorOnClose( msgBox );
}

void SettingsActions::setCursorBlinkingTime() {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT,
		i18n( "set_cursor_blinking_time", "Set Cursor Blinking Time:\nSet 0 to disable it.\n" )
			.unescape() );
	msgBox->setTitle( mApp->getWindowTitle() );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->getTextInput()->setText( mApp->getConfig().editor.cursorBlinkingTime.toString() );
	msgBox->showWhenReady();
	msgBox->on( Event::OnConfirm, [this, msgBox]( const Event* ) {
		mApp->getConfig().editor.cursorBlinkingTime =
			Time::fromString( msgBox->getTextInput()->getText().toUtf8() );
		mApp->getSplitter()->forEachEditor( [this]( UICodeEditor* editor ) {
			editor->setCursorBlinkTime( mApp->getConfig().editor.cursorBlinkingTime );
		} );
		msgBox->closeWindow();
	} );
	mApp->setFocusEditorOnClose( msgBox );
}

void SettingsActions::setIndentTabCharacter() {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::COMBOBOX,
		i18n( "set_indent_tab_character", "Set the tab indentation guide character displayed." ) );
	msgBox->setId( "indent_tab_window" );
	msgBox->setTitle( mApp->getWindowTitle() );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	UIComboBox* comboBox = msgBox->getComboBox();
	UIListBox* listBox = msgBox->getComboBox()->getDropDownList()->getListBox();
	listBox->addClass( "indent_tab_listbox_item" );
	listBox->addListBoxItems( { u8"»"s, u8"→"s, u8"⇒"s, u8"↪"s, u8"⇢"s, u8"↣"s } );
	msgBox->getComboBox()->setText(
		mApp->getConfig().editor.tabIndentCharacter.empty()
			? String( u8"»"s )
			: String::fromUtf8( mApp->getConfig().editor.tabIndentCharacter ) );
	msgBox->showWhenReady();
	comboBox->on( Event::OnValueChange, [this, comboBox]( const Event* ) {
		if ( comboBox->getText().size() != 1 )
			return;
		mApp->getSplitter()->forEachEditor( [comboBox]( UICodeEditor* editor ) {
			editor->setTabIndentCharacter( comboBox->getText()[0] );
		} );
	} );
	msgBox->on( Event::OnConfirm, [this, msgBox]( const Event* ) {
		auto txt( msgBox->getComboBox()->getText() );
		if ( txt.size() == 1 ) {
			mApp->getConfig().editor.tabIndentCharacter = txt.toUtf8();
			mApp->getSplitter()->forEachEditor(
				[txt]( UICodeEditor* editor ) { editor->setTabIndentCharacter( txt[0] ); } );
			msgBox->closeWindow();
		} else {
			UIMessageBox* msgBoxAlert =
				UIMessageBox::New( UIMessageBox::OK, i18n( "it_must_be_a_single_character",
														   "It must be a single character" ) );
			msgBoxAlert->setTitle( mApp->getWindowTitle() );
			msgBoxAlert->setCloseShortcut( { KEY_ESCAPE, 0 } );
		}
	} );
	msgBox->on( Event::OnDiscard, [this]( const Event* ) {
		String txt = String::fromUtf8( mApp->getConfig().editor.tabIndentCharacter );
		if ( txt.size() != 1 )
			return;
		mApp->getSplitter()->forEachEditor(
			[&txt]( UICodeEditor* editor ) { editor->setTabIndentCharacter( txt[0] ); } );
	} );
	mApp->setFocusEditorOnClose( msgBox );
}

void SettingsActions::setFoldRefreshFreq() {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT,
		i18n( "set_fold_refresh_frequency",
			  "Set code folds refresh frequency:\nIt should be bigger than 1 second.\nFolds are "
			  "only refreshed after any document modification." )
			.unescape() );
	msgBox->setTitle( mApp->getWindowTitle() );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->getTextInput()->setText( mApp->getConfig().editor.codeFoldingRefreshFreq.toString() );
	msgBox->showWhenReady();
	msgBox->on( Event::OnConfirm, [this, msgBox]( const Event* ) {
		mApp->getConfig().editor.codeFoldingRefreshFreq =
			Time::fromString( msgBox->getTextInput()->getText().toUtf8() );
		if ( mApp->getConfig().editor.codeFoldingRefreshFreq < Seconds( 1 ) )
			mApp->getConfig().editor.codeFoldingRefreshFreq = Seconds( 1 );
		mApp->getSplitter()->forEachEditor( [this]( UICodeEditor* editor ) {
			editor->setFoldsRefreshTime( mApp->getConfig().editor.codeFoldingRefreshFreq );
		} );
		msgBox->closeWindow();
	} );
	mApp->setFocusEditorOnClose( msgBox );
}

void SettingsActions::setUIScaleFactor() {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT,
		i18n( "set_ui_scale_factor", "Set the UI scale factor (pixel density):\nMinimum value is "
									 "1, and maximum 6. Requires restart." ) );
	msgBox->setTitle( mApp->getWindowTitle() );
	msgBox->getTextInput()->setText(
		String::fromFloat( mApp->getConfig().windowState.pixelDensity ) );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->showWhenReady();
	msgBox->on( Event::OnConfirm, [this, msgBox]( const Event* ) {
		msgBox->closeWindow();
		Float val;
		if ( String::fromString( val, msgBox->getTextInput()->getText() ) && val >= 1 &&
			 val <= 6 ) {
			if ( mApp->getConfig().windowState.pixelDensity != val ) {
				mApp->getConfig().windowState.pixelDensity = val;
				UIMessageBox* msg = UIMessageBox::New(
					UIMessageBox::OK,
					i18n( "new_ui_scale_factor", "New UI scale factor assigned.\nPlease "
												 "restart the application." ) );
				msg->showWhenReady();
				mApp->setFocusEditorOnClose( msg );
			} else if ( mApp->getSplitter() && mApp->getSplitter()->getCurWidget() ) {
				mApp->getSplitter()->getCurWidget()->setFocus();
			}
		} else {
			UIMessageBox* msg = UIMessageBox::New( UIMessageBox::OK, "Invalid value!" );
			msg->showWhenReady();
			mApp->setFocusEditorOnClose( msg );
		}
	} );
}

void SettingsActions::setEditorFontSize() {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT, i18n( "set_editor_font_size", "Set the editor font size:" ) );
	msgBox->setTitle( mApp->getWindowTitle() );
	msgBox->getTextInput()->setText( mApp->getConfig().editor.fontSize.toString() );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->showWhenReady();
	msgBox->on( Event::OnConfirm, [this, msgBox]( const Event* ) {
		mApp->getConfig().editor.fontSize = StyleSheetLength( msgBox->getTextInput()->getText() );
		mApp->getSplitter()->forEachEditor( [this]( UICodeEditor* editor ) {
			editor->setFontSize(
				mApp->getConfig().editor.fontSize.asPixels( 0, Sizef(), mApp->getDisplayDPI() ) );
		} );
	} );
	mApp->setFocusEditorOnClose( msgBox );
}

void SettingsActions::setTerminalFontSize() {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT, i18n( "set_terminal_font_size", "Set the terminal font size:" ) );
	msgBox->setTitle( mApp->getWindowTitle() );
	msgBox->getTextInput()->setText( mApp->getConfig().term.fontSize.toString() );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->showWhenReady();
	msgBox->on( Event::OnConfirm, [this, msgBox]( const Event* ) {
		mApp->getConfig().term.fontSize = StyleSheetLength( msgBox->getTextInput()->getText() );
		mApp->getSplitter()->forEachWidget( [this]( UIWidget* widget ) {
			if ( widget && widget->isType( UI_TYPE_TERMINAL ) )
				widget->asType<UITerminal>()->setFontSize(
					mApp->getConfig().term.fontSize.asPixels( 0, Sizef(), mApp->getDisplayDPI() ) );
		} );
	} );
	mApp->setFocusEditorOnClose( msgBox );
}

void SettingsActions::setUIFontSize() {
	UIMessageBox* msgBox = UIMessageBox::New( UIMessageBox::INPUT,
											  i18n( "set_ui_font_size", "Set the UI font size:" ) );
	msgBox->setTitle( mApp->getWindowTitle() );
	msgBox->getTextInput()->setText( mApp->getConfig().ui.fontSize.toString() );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->showWhenReady();
	msgBox->on( Event::OnConfirm, [this, msgBox]( const Event* ) {
		mApp->getConfig().ui.fontSize = StyleSheetLength( msgBox->getTextInput()->getText() );
		Float fontSize =
			mApp->getConfig().ui.fontSize.asPixels( 0, Sizef(), mApp->getDisplayDPI() );
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
		msgBox->closeWindow();
	} );
	mApp->setFocusEditorOnClose( msgBox );
}

void SettingsActions::setUIPanelFontSize() {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT, i18n( "set_side_panel_font_size", "Set side panel font size:" ) );
	msgBox->setTitle( mApp->getWindowTitle() );
	msgBox->getTextInput()->setText( mApp->getConfig().ui.panelFontSize.toString() );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->showWhenReady();
	msgBox->on( Event::OnConfirm, [this, msgBox]( const Event* ) {
		mApp->getConfig().ui.panelFontSize = StyleSheetLength( msgBox->getTextInput()->getText() );

		// Update the CSS
		auto selsFound = mApp->getUISceneNode()->getStyleSheet().findStyleFromSelectorName(
			"#project_view > treeview::row > treeview::cell > treeview::cell::text" );
		if ( !selsFound.empty() ) {
			for ( auto sel : selsFound )
				sel->updatePropertyValue( "font-size",
										  mApp->getConfig().ui.panelFontSize.toString() );
			mApp->getUISceneNode()->getStyleSheet().refreshCacheFromStyles( selsFound );
		}

		UITreeView* treeView = mApp->getUISceneNode()->find<UITreeView>( "project_view" );
		if ( !treeView ) {
			msgBox->closeWindow();
			return;
		}
		treeView->reloadStyle( true, true, true, true );
		treeView->updateContentSize();
		msgBox->closeWindow();
	} );
	mApp->setFocusEditorOnClose( msgBox );
}

String SettingsActions::i18n( const std::string& key, const String& def ) {
	return mApp->i18n( key, def );
}

} // namespace ecode
