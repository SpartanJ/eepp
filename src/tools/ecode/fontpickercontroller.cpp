#include "fontpickercontroller.hpp"
#include "ecode.hpp"
#include "settingsactions.hpp"
#include <eepp/graphics/fontfamily.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/tools/uifontpickerdialog.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eterm/ui/uiterminal.hpp>
#include <memory>

namespace ecode {

struct MonospaceFontPreview {
	std::string originalPath;
	std::string previewPath;
	FontTrueType* originalFont{ nullptr };
	bool confirmed{ false };
	UnorderedMap<std::string, FontTrueTypePtr> loadedFonts;
};

void FontPickerController::openFontDialog( std::string& fontPath, bool loadingMonoFont,
										   bool terminalFont, std::function<void()> onFinish,
										   bool pickFontSize ) {
	std::string absoluteFontPath( fontPath );
	if ( FileSystem::isRelativePath( absoluteFontPath ) )
		absoluteFontPath = mApp->resPath() + fontPath;

	const auto normalizedFontPath = [this]( std::string path ) {
		if ( String::startsWith( path, mApp->resPath() ) )
			path = path.substr( mApp->resPath().size() );
		return path;
	};

	const auto applyMonospaceFont = [this, terminalFont]( FontTrueType* fontMono,
														  bool loadFamily = false ) {
		if ( !fontMono )
			return;

		if ( terminalFont )
			mApp->mTerminalFont = fontMono;
		else
			mApp->mFontMono = fontMono;

		fontMono->setEnableDynamicMonospace( true );
		fontMono->setBoldAdvanceSameAsRegular( true );
		// Related faces are published by FontFamily. Keep transient previews isolated and only
		// publish/load their family after the user confirms the selection.
		if ( loadFamily )
			FontFamily::loadFromRegular( fontMono );

		if ( !mApp->getSplitter() )
			return;

		if ( terminalFont ) {
			mApp->getSplitter()->forEachWidgetType( UI_TYPE_TERMINAL, [fontMono]( UIWidget* term ) {
				term->asType<UITerminal>()->setFont( fontMono );
			} );
		} else {
			mApp->getSplitter()->forEachEditor(
				[fontMono]( UICodeEditor* editor ) { editor->setFont( fontMono ); } );

			if ( auto buildOutputEditor =
					 mApp->uiSceneNode()->find<UICodeEditor>( "build_output_output" ) )
				buildOutputEditor->setFont( fontMono );

			if ( auto appOutputEditor =
					 mApp->uiSceneNode()->find<UICodeEditor>( "app_output_output" ) )
				appOutputEditor->setFont( fontMono );

			if ( mApp->getConfig().ui.editorFontInInputFields )
				mApp->updateInputFonts();
		}
	};

	std::shared_ptr<MonospaceFontPreview> preview;
	if ( loadingMonoFont ) {
		preview = std::make_shared<MonospaceFontPreview>();
		preview->originalPath = normalizedFontPath( fontPath );
		preview->previewPath = preview->originalPath;
		preview->originalFont = static_cast<FontTrueType*>( terminalFont ? mApp->getTerminalFont()
																		 : mApp->getFontMono() );
	}

	const auto loadPreviewFont = [this, preview]( const FontDesc& desc ) -> FontTrueTypePtr {
		if ( !preview )
			return {};

		auto found = preview->loadedFonts.find( desc.path );
		if ( found != preview->loadedFonts.end() )
			return found->second;

		FontTrueTypePtr fontMono = defaultResourceScope().getFontService().loadSystemFont( desc );
		if ( fontMono ) {
			fontMono->setHinting( mApp->getConfig().ui.fontHinting );
			fontMono->setAntialiasing( mApp->getConfig().ui.fontAntialiasing );
			preview->loadedFonts[desc.path] = fontMono;
		}
		return fontMono;
	};

	const auto publishPreviewFont = []( const std::string& path, const FontTrueTypePtr& font ) {
		if ( !font )
			return;
		auto fontName = FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( path ) );
		defaultResourceScope().publishLocalFont( std::move( fontName ), font );
	};

	const Uint32 flags = UIFontPickerDialog::ShowStyle |
						 ( pickFontSize ? UIFontPickerDialog::ShowSize : 0 ) |
						 ( loadingMonoFont ? UIFontPickerDialog::MonospaceOnly : 0 );
	UIFontPickerDialog* dialog = UIFontPickerDialog::New( flags );
	dialog->setTitle( mApp->i18n( "select_font", "Select Font" ) );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->on( Event::OnWindowClose, [this, preview, applyMonospaceFont]( const Event* ) {
		if ( !App::instance() || SceneManager::isShuttingDown() )
			return;

		if ( preview && !preview->confirmed )
			applyMonospaceFont( preview->originalFont, false );

		if ( mApp->getSplitter() && mApp->getSplitter()->getCurWidget() )
			mApp->getSplitter()->getCurWidget()->setFocus();
	} );
	if ( loadingMonoFont ) {
		dialog->setOnFontSelectionChanged(
			[preview, normalizedFontPath, loadPreviewFont,
			 applyMonospaceFont]( const UIFontSelection& selection ) {
				auto newPath = normalizedFontPath( selection.font.path );
				if ( !preview || newPath.empty() || preview->previewPath == newPath )
					return;

				if ( newPath == preview->originalPath ) {
					applyMonospaceFont( preview->originalFont, false );
					preview->previewPath = newPath;
					return;
				}

				FontTrueTypePtr fontMono = loadPreviewFont( selection.font );
				if ( fontMono ) {
					applyMonospaceFont( fontMono.get(), false );
					preview->previewPath = newPath;
				}
			} );
	}
	dialog->setOnFontPicked( [this, &fontPath, loadingMonoFont, terminalFont, onFinish, preview,
							  normalizedFontPath, pickFontSize, loadPreviewFont, publishPreviewFont,
							  applyMonospaceFont]( const UIFontSelection& selection ) {
		if ( pickFontSize ) {
			const StyleSheetLength size( selection.size, StyleSheetLength::Dp );
			if ( terminalFont )
				mApp->getSettingsActions()->setTerminalFontSize( size );
			else if ( loadingMonoFont )
				mApp->getSettingsActions()->setEditorFontSize( size );
			else
				mApp->getSettingsActions()->setUIFontSize( size );
		}
		auto newPath = normalizedFontPath( selection.font.path );
		if ( newPath.empty() )
			return;
		if ( fontPath != newPath ) {
			if ( !loadingMonoFont ) {
				fontPath = newPath;
			} else {
				FontTrueTypePtr previewFont = preview && newPath != preview->originalPath
												  ? loadPreviewFont( selection.font )
												  : FontTrueTypePtr{};
				FontTrueType* fontMono = previewFont ? previewFont.get() : preview->originalFont;
				if ( fontMono ) {
					fontPath = newPath;
					if ( preview )
						preview->confirmed = true;
					publishPreviewFont( newPath, previewFont );
					applyMonospaceFont( fontMono, previewFont != nullptr );
				}
			}
		} else if ( preview ) {
			preview->confirmed = true;
			applyMonospaceFont( preview->originalFont, false );
		}
		if ( onFinish )
			onFinish();
	} );
	if ( pickFontSize ) {
		UIFontSelection selection = dialog->getSelection();
		const Float currentSize =
			terminalFont ? mApp->getConfig().term.fontSize.asDp( 0, Sizef(), mApp->getDisplayDPI() )
			: loadingMonoFont
				? mApp->getConfig().editor.fontSize.asDp( 0, Sizef(), mApp->getDisplayDPI() )
				: mApp->getConfig().ui.fontSize.asDp( 0, Sizef(), mApp->getDisplayDPI() );
		selection.size = static_cast<Uint32>( currentSize );
		dialog->setSelection( selection );
	}
	dialog->setSelectedFont( absoluteFontPath );
	dialog->center();
	dialog->show();
}

} // namespace ecode
