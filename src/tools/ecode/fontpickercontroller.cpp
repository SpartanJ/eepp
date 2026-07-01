#include "fontpickercontroller.hpp"
#include "ecode.hpp"
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
	UnorderedMap<std::string, FontTrueType*> loadedFonts;
};

void FontPickerController::openFontDialog( std::string& fontPath, bool loadingMonoFont,
										   bool terminalFont, std::function<void()> onFinish ) {
	std::string absoluteFontPath( fontPath );
	if ( FileSystem::isRelativePath( absoluteFontPath ) )
		absoluteFontPath = mApp->resPath() + fontPath;

	const auto normalizedFontPath = [this]( std::string path ) {
		if ( String::startsWith( path, mApp->resPath() ) )
			path = path.substr( mApp->resPath().size() );
		return path;
	};

	const auto applyMonospaceFont = [this, terminalFont]( FontTrueType* fontMono ) {
		if ( !fontMono )
			return;

		if ( terminalFont )
			mApp->mTerminalFont = fontMono;
		else
			mApp->mFontMono = fontMono;

		fontMono->setEnableDynamicMonospace( true );
		fontMono->setBoldAdvanceSameAsRegular( true );
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

	const auto loadPreviewFont = [this, preview]( const std::string& newPath ) -> FontTrueType* {
		if ( !preview )
			return nullptr;

		auto found = preview->loadedFonts.find( newPath );
		if ( found != preview->loadedFonts.end() )
			return found->second;

		auto fontName = FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( newPath ) );
		FontTrueType* fontMono = mApp->loadFont( fontName, newPath );
		if ( fontMono )
			preview->loadedFonts[newPath] = fontMono;
		return fontMono;
	};

	const Uint32 flags = UIFontPickerDialog::DefaultFlags |
						 ( loadingMonoFont ? UIFontPickerDialog::MonospaceOnly : 0 );
	UIFontPickerDialog* dialog = UIFontPickerDialog::New( flags );
	dialog->setTitle( mApp->i18n( "select_font", "Select Font" ) );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->on( Event::OnWindowClose, [this, preview, applyMonospaceFont]( const Event* ) {
		if ( preview && !preview->confirmed )
			applyMonospaceFont( preview->originalFont );

		if ( App::instance() && mApp->getSplitter() && mApp->getSplitter()->getCurWidget() &&
			 !SceneManager::instance()->isShuttingDown() ) {
			mApp->getSplitter()->getCurWidget()->setFocus();
		}
	} );
	if ( loadingMonoFont ) {
		dialog->setOnFontSelectionChanged(
			[preview, normalizedFontPath, loadPreviewFont,
			 applyMonospaceFont]( const UIFontSelection& selection ) {
				auto newPath = normalizedFontPath( selection.font.path );
				if ( !preview || newPath.empty() || preview->previewPath == newPath )
					return;

				if ( newPath == preview->originalPath ) {
					applyMonospaceFont( preview->originalFont );
					preview->previewPath = newPath;
					return;
				}

				FontTrueType* fontMono = loadPreviewFont( newPath );
				if ( fontMono ) {
					applyMonospaceFont( fontMono );
					preview->previewPath = newPath;
				}
			} );
	}
	dialog->setOnFontPicked( [&fontPath, loadingMonoFont, onFinish, preview, normalizedFontPath,
							  loadPreviewFont,
							  applyMonospaceFont]( const UIFontSelection& selection ) {
		auto newPath = normalizedFontPath( selection.font.path );
		if ( newPath.empty() )
			return;
		if ( fontPath != newPath ) {
			if ( !loadingMonoFont ) {
				fontPath = newPath;
				if ( onFinish )
					onFinish();
				return;
			}

			FontTrueType* fontMono = preview && newPath == preview->originalPath
										 ? preview->originalFont
										 : loadPreviewFont( newPath );
			if ( fontMono ) {
				fontPath = newPath;
				if ( preview )
					preview->confirmed = true;
				applyMonospaceFont( fontMono );
			}
		} else if ( preview ) {
			preview->confirmed = true;
			applyMonospaceFont( preview->originalFont );
		}
	} );
	dialog->setSelectedFont( absoluteFontPath );
	dialog->center();
	dialog->show();
}

} // namespace ecode
