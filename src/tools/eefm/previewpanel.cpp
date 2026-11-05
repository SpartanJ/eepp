#include "previewpanel.hpp"
#include "app.hpp"
#include "fileoperations.hpp"
#include "location.hpp"
#include "mime.hpp"
#include "panemanager.hpp"

#include <eepp/audio/music.hpp>
#include <eepp/audio/soundfilefactory.hpp>
#include <eepp/ui/doc/languagessyntaxhighlighting.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/tools/uiaudioplayer.hpp>
#include <eepp/ui/tools/uiimageviewer.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uiloader.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <mutex>

namespace eefm {

using namespace EE::Audio;

using namespace EE::UI::Doc;
using namespace EE::UI::Tools;

namespace {

const char* Layout =
#include "previewpanel.xml.hpp"
	;

constexpr Uint64 MaxTextSize = 2 * 1024 * 1024;
constexpr Uint64 MaxMediaSize = 128 * 1024 * 1024;
const auto LoadAction = String::hash( "eefm-preview-load" );

/** Handle file navigation before the editor consumes it as cursor movement. */
class QuickLookEditor : public UICodeEditor {
  public:
	explicit QuickLookEditor( PreviewPanel& preview ) : mPreview( preview ) {}

  protected:
	Uint32 onKeyDown( const KeyEvent& event ) override {
		if ( event.getSanitizedMod() == 0 ) {
			switch ( event.getKeyCode() ) {
				case KEY_UP:
					mPreview.moveSelection( -1 );
					return 1;
				case KEY_DOWN:
					mPreview.moveSelection( 1 );
					return 1;
				case KEY_SPACE:
				case KEY_ESCAPE:
					mPreview.show( false );
					return 1;
				default:
					break;
			}
		}
		return UICodeEditor::onKeyDown( event );
	}

  private:
	PreviewPanel& mPreview;
};

enum class PreviewKind : Uint8 { Text, Image, Audio, Unsupported };

struct Preview {
	VFSResult<std::unique_ptr<IOStreamVFS>> stream;
	std::shared_ptr<TextDocument> document;
	std::unique_ptr<Music> audio;
	PreviewKind kind;
};

PreviewKind kindForName( const std::string& name ) {
	if ( Image::isImageExtension( name ) )
		return PreviewKind::Image;
	if ( SoundFileFactory::isKnownFileExtension( name ) )
		return PreviewKind::Audio;
	return PreviewKind::Text;
}

} // namespace

PreviewPanel::PreviewPanel( App& app, UISplitter* splitter ) : mApp( app ), mSplitter( splitter ) {}

PreviewPanel::~PreviewPanel() = default;

bool PreviewPanel::isVisible() const {
	return mSplitter ? mRoot && mRoot->getParent() == mSplitter : mWindow && mWindow->isVisible();
}

void PreviewPanel::create() {
	static std::once_flag languages;
	std::call_once( languages, Language::LanguagesSyntaxHighlighting::load );
	if ( !mSplitter ) {
		// Keep the window and its lazy viewers for reuse; closing releases their resources.
		// XML flags replace the theme defaults; programmatic window flags are additive.
		mWindow =
			mApp.ui()
				->loadLayoutFromString(
					R"xml(<window id="quick_look" window-flags="close|maximize|resizeable|shadow|modal" />)xml" )
				->asType<UIWindow>();
		mWindow->show();
		mWindow->getModalWidget()->setId( "quick_look_backdrop" );
		mWindow->getModalWidget()->on( Event::MouseClick, [this]( const Event* event ) {
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK )
				show( false );
		} );
		mWindow->setTitle( mApp.ui()->i18n( "eefm_quick_look", "Quick Look" ) );
		mWindow->setSizeWithDecoration( mApp.ui()->getSize().x * 0.8f,
										mApp.ui()->getSize().y * 0.8f );
		mWindow->setPosition( ( mApp.ui()->getSize() - mWindow->getSize() ) / 2.f );
		mWindow->getButtonClose()->on( Event::MouseClick, [this]( const Event* event ) {
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK )
				show( false );
		} );
		mWindow->getButtonMaximize()->on( Event::MouseClick, [this]( const Event* event ) {
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK )
				mWindow->maximize();
		} );
		mWindow->addKeyBindsString( { { "space", "dismiss-preview" },
									  { "escape", "dismiss-preview" },
									  { "up", "previous-file" },
									  { "down", "next-file" } } );
		mWindow->setKeyBindingCommand( "dismiss-preview", [this] { show( false ); } );
		mWindow->setKeyBindingCommand( "previous-file", [this] { moveSelection( -1 ); } );
		mWindow->setKeyBindingCommand( "next-file", [this] { moveSelection( 1 ); } );
	}
	mRoot = mApp.ui()->loadLayoutFromString( Layout,
											 mWindow ? mWindow->getContainer() : mApp.parking() );
	if ( mWindow ) {
		mRoot->setId( "quick_look_content" );
		mRoot->find<UIWidget>( "preview_header" )->setVisible( false );
	}
	mFallback = mRoot->find<UIWidget>( "preview_fallback" );
	mContent = mRoot->find<UIWidget>( "preview_content" );
	mName = mRoot->find<UITextView>( "preview_name" );
	mOpen = mRoot->find<UIPushButton>( "preview_open" );
	mOpen->onClick( [this]( const MouseEvent* ) {
		if ( mURI && mDefaultApplication ) {
			mApp.fileOps().openWith( { *mURI }, std::move( *mDefaultApplication ) );
			show( false );
		}
	} );
	mMessage = mRoot->find<UITextView>( "preview_message" );
	mLoader = mRoot->find<UILoader>( "preview_loader" );
	mRoot->find<UIWidget>( "preview_close" )->onClick( [this]( const MouseEvent* ) {
		show( false );
	} );
}

void PreviewPanel::show( bool visible ) {
	if ( visible ) {
		if ( !mRoot )
			create();
		if ( mWindow ) {
			mWindow->show();
			mWindow->setFocus();
		} else if ( !isVisible() ) {
			mRoot->setParent( mSplitter );
			mRoot->setVisible( true );
			mSplitter->setSplitPartition( StyleSheetLength( mPartition ) );
		}
		syncSelection();
	} else if ( isVisible() ) {
		rememberSize();
		clear();
		if ( mWindow ) {
			mWindow->hide( true );
		} else {
			mRoot->setVisible( false );
			mRoot->setParent( mApp.parking() );
		}
		if ( auto* tab = mApp.panes().currentTab() )
			tab->view->setFocus();
	}
}

void PreviewPanel::moveSelection( int direction ) {
	auto* tab = mApp.panes().currentTab();
	if ( !tab || tab->model->rowCount() == 0 )
		return;
	const Int64 row = eeclamp<Int64>( tab->view->getSelection().first().row() + direction, 0,
									  static_cast<Int64>( tab->model->rowCount() ) - 1 );
	if ( mWindow )
		mWindow->setFocus();
	tab->view->setSelection( tab->model->index( row ) );
}

void PreviewPanel::refreshIcon() {
	if ( isVisible() && mFileIcon ) {
		mRoot->find<UIImage>( "preview_icon" )
			->setDrawable( mFileIcon->getSource( PixelDensity::dpToPxI( 96 ) ) );
	}
}

void PreviewPanel::rememberSize() {
	if ( !mSplitter || !isVisible() || mSplitter->getPixelsSize().x <= 0 )
		return;
	const Float files = mSplitter->getFirstWidget()->getPixelsSize().x;
	mPartition = String::format(
		"%.2f%%", eeclamp( files / mSplitter->getPixelsSize().x * 100.f, 20.f, 85.f ) );
}

void PreviewPanel::cancel() {
	++mGeneration;
	if ( mCancellation ) {
		mCancellation->cancel();
		mCancellation.reset();
	}
	if ( mRoot )
		mRoot->removeActionsByTag( LoadAction );
	if ( mImage )
		mImage->reset();
	if ( mAudio )
		mAudio->stop();
}

void PreviewPanel::clear() {
	cancel();
	mURI.reset();
	mFileIcon = nullptr;
	mLoader->setVisible( false );
	mFallback->setVisible( false );
	mDetails.clear();
	mDefaultApplication.reset();
	mOpen->setVisible( false );
	if ( mEditor && mEditor->isVisible() ) {
		mEditor->setVisible( false );
		mEditor->getDocument().reset();
	}
	if ( mImage )
		mImage->setVisible( false );
	if ( mAudio && mAudio->isVisible() ) {
		mAudio->setVisible( false );
		mAudio->reset();
	}
}

void PreviewPanel::showMessage( const String& text ) {
	mLoader->setVisible( false );
	if ( mImage )
		mImage->setVisible( false );
	mRoot->find<UIImage>( "preview_icon" )->setVisible( !mDetails.empty() );
	mMessage->setText( mDetails.empty() ? text : mDetails + "\n\n" + text );
	mFallback->setVisible( true );
}

void PreviewPanel::syncSelection() {
	if ( !isVisible() || !mApp.panes().ready() )
		return;
	auto* tab = mApp.panes().currentTab();
	if ( !tab )
		return;
	const auto& selection = tab->view->getSelection();
	const auto index = selection.first();
	if ( selection.size() != 1 || index.row() < 0 ||
		 static_cast<size_t>( index.row() ) >= tab->model->rowCount() ) {
		if ( mURI )
			clear();
		mName->setText( "" );
		mName->setTooltipText( "" );
		showMessage( mApp.ui()->i18n( "eefm_preview_select_file", "Select one file to preview." ) );
		return;
	}
	const auto& info = tab->model->entry( index.row() );
	URI uri = tab->model->entryURI( index.row() );
	if ( mURI && *mURI == uri && mRevision == info.revision && mSize == info.size().value_or( 0 ) )
		return;
	clear();
	mURI = std::move( uri );
	mRevision = info.revision;
	mSize = info.size().value_or( 0 );
	mName->setText( String::fromUtf8( info.name ) );
	mName->setTooltipText( Location::text( *mURI ) );
	mDetails =
		String::fromUtf8( MimeDatabase::instance().describe( info, &mApp.ui()->getTranslator() ) );
	if ( info.size() )
		mDetails += "\n" + FileSystem::sizeToString( *info.size() );
	if ( info.modifiedTime() ) {
		mDetails +=
			"\n" + String::fromUtf8( String::format(
					   mApp.ui()->i18n( "eefm_preview_modified", "Modified: %s" ).toUtf8(),
					   Sys::epochToString( static_cast<Uint64>( *info.modifiedTime() ) ) ) );
	}
	mDetails += "\n" + String::fromUtf8( Location::clipboardText( *mURI ) );
	mFileIcon = mApp.fileIcon( info );
	refreshIcon();
	if ( info.isDirectory() || ( !info.isRegularFile() && !info.isSymlink() ) ) {
		showMessage(
			mApp.ui()->i18n( "eefm_preview_not_file", "Select a regular file to preview." ) );
		return;
	}
	mFallback->setVisible( false );
	mLoader->setVisible( true );
	// Arrow-key navigation need not start a read and decode for every row crossed.
	mRoot->runOnMainThread( [this] { load(); }, Milliseconds( 100 ), LoadAction );
}

void PreviewPanel::loadApplication( std::string mime ) {
	const Uint64 generation = mGeneration;
	// Discovery can query native file metadata; isolate network mounts as other VFS reads do.
	auto discover = [uri = *mURI, mime = std::move( mime )]() {
		return VFSResult<std::optional<Application>>(
			Ok( Applications::defaultFor( uri, MimeDatabase::instance().withParents( mime ) ) ) );
	};
	auto task = mURI->getScheme() == "file"
					? mApp.vfs()->localIO().submitResolved<std::optional<Application>>(
						  mURI->getFSPath(), true, std::move( discover ) )
					: mApp.pool()->submit( std::move( discover ) );
	std::move( task ).then(
		mApp.executor(), [this, generation]( VFSResult<std::optional<Application>>&& result ) {
			if ( generation != mGeneration || !isVisible() || !result || !result.value() )
				return;
			mDefaultApplication = std::move( result ).value();
			mOpen->setText( String::format(
				mApp.ui()->i18n( "eefm_open_with_application", "Open with %s" ).toUtf8(),
				mDefaultApplication->name ) );
			mApp.setApplicationIcon( mOpen, *mDefaultApplication );
			mOpen->setVisible( true );
		} );
}

void PreviewPanel::load() {
	std::string name = mURI->getLastPathSegment();
	if ( mWindow )
		loadApplication( MimeDatabase::instance().typeForName( name ) );
	const PreviewKind kind = kindForName( name );
	const Uint64 limit = kind == PreviewKind::Text ? MaxTextSize : MaxMediaSize;
	if ( mSize > limit ) {
		showMessage( String::format(
			mApp.ui()
				->i18n( "eefm_preview_size_limit", "Preview is limited to files up to %s." )
				.toUtf8(),
			FileSystem::sizeToString( limit ) ) );
		return;
	}
	mCancellation.emplace();
	const auto token = mCancellation->token();
	const Uint64 generation = mGeneration;
	mApp.vfs()
		->openStream( *mURI, {}, token, limit )
		.then( *mApp.pool(),
			   [kind, token,
				name = std::move( name )]( VFSResult<std::unique_ptr<IOStreamVFS>>&& stream ) {
				   Preview preview{ std::move( stream ), {}, {}, kind };
				   if ( !preview.stream || token.isCancellationRequested() )
					   return preview;
				   if ( kind == PreviewKind::Audio ) {
					   preview.audio = std::make_unique<Music>();
					   if ( !preview.audio->openFromStream( std::move( preview.stream.value() ) ) )
						   preview.audio.reset();
				   } else if ( kind == PreviewKind::Text ) {
					   auto& input = *preview.stream.value();
					   if ( TextDocument::fileMightBeBinary( input, name ) ) {
						   preview.kind = PreviewKind::Unsupported;
					   } else if ( !input.error() ) {
						   preview.document = std::make_shared<TextDocument>( false );
						   preview.document->loadFromStream( input );
					   }
					   auto closed = input.close();
					   if ( !closed )
						   preview.stream = Err( std::move( closed ).error() );
					   else
						   preview.stream.value().reset();
				   }
				   return preview;
			   } )
		.then( mApp.executor(), [this, generation]( Preview&& preview ) {
			if ( generation != mGeneration || !isVisible() )
				return;
			if ( !preview.stream ) {
				showMessage( String::format(
					mApp.ui()
						->i18n( "eefm_preview_read_failed", "Could not load preview:\n%s" )
						.toUtf8(),
					preview.stream.error().toString() ) );
				return;
			}
			if ( preview.kind == PreviewKind::Text ) {
				preview.document->setSyntaxDefinition( SyntaxDefinitionManager::instance()->findPtr(
					mURI->getLastPathSegment(),
					preview.document->line( 0 ).getText().substr( 0, 256 ).toUtf8() ) );
				if ( !mEditor ) {
					mEditor = mWindow ? eeNew( QuickLookEditor, ( *this ) ) : UICodeEditor::New();
					mEditor->setId( "preview_editor" );
					mEditor->setParent( mContent );
					mEditor->setLayoutSizePolicy( SizePolicy::MatchParent,
												  SizePolicy::MatchParent );
					mEditor->setLocked( true );
					mEditor->setDynamicTheming( true );
					mEditor->setShowLineNumber( true );
					mEditor->setShowFoldingRegion( false );
					mEditor->setHighlightCurrentLine( false );
					mEditor->setInteractiveLinks( false );
				}

				mEditor->setDocument( std::move( preview.document ) );
				mEditor->setVisible( true );
				mLoader->setVisible( false );
			} else if ( preview.kind == PreviewKind::Image ) {
				if ( !mImage ) {
					mImage = UIImageViewer::New();
					mImage->setId( "preview_image" );
					mImage->setParent( mContent );
					mImage->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
					mImage->setAutoFitOnSizeChange( true );
					mImage->setUseNativeImageSize( true );
					mImage->on( Event::OnResourceLoaded,
								[this]( const Event* ) { mLoader->setVisible( false ); } );
					mImage->on( Event::OnResourceLoadFailed, [this]( const Event* ) {
						if ( isVisible() && mImage->isVisible() ) {
							showMessage( mApp.ui()->i18n( "eefm_preview_invalid_image",
														  "This image could not be decoded." ) );
						}
					} );
				}
				mImage->setVisible( true );
				mImage->loadImageAsync( std::move( preview.stream.value() ) );
				// One spinner spans the VFS read and image decode without changing style or phase.
				mImage->getLoader()->setVisible( false );
				mLoader->toFront();
			} else if ( preview.kind == PreviewKind::Audio ) {
				if ( !mAudio ) {
					mAudio = UIAudioPlayer::New();
					mAudio->setId( "preview_audio" );
					mAudio->setParent( mContent );
					mAudio->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
				}
				if ( preview.audio ) {
					mAudio->setAudio( std::move( preview.audio ), false );
					mAudio->setVisible( true );
					mLoader->setVisible( false );
				} else {
					mAudio->reset();
					showMessage( mApp.ui()->i18n( "eefm_preview_invalid_audio",
												  "This audio file could not be decoded." ) );
				}
			} else {
				showMessage( mApp.ui()->i18n( "eefm_preview_unsupported",
											  "No preview is available for this file type." ) );
			}
		} );
}

} // namespace eefm
