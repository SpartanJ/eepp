#include "previewpanel.hpp"
#include "app.hpp"
#include "location.hpp"
#include "panemanager.hpp"

#include <eepp/audio/music.hpp>
#include <eepp/audio/soundfilefactory.hpp>
#include <eepp/ui/doc/languagessyntaxhighlighting.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/tools/uiaudioplayer.hpp>
#include <eepp/ui/tools/uiimageviewer.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uiloader.hpp>

namespace eefm {

using namespace EE::UI::Doc;
using namespace EE::UI::Tools;

namespace {

const char* Layout =
#include "previewpanel.xml.hpp"
	;

constexpr Uint64 MaxTextSize = 2 * 1024 * 1024;
constexpr Uint64 MaxMediaSize = 128 * 1024 * 1024;
const auto LoadAction = String::hash( "eefm-preview-load" );

enum class PreviewKind : Uint8 { Text, Image, Audio, Unsupported };

struct Preview {
	VFSResult<std::unique_ptr<EE::VFS::IOStreamVFS>> stream;
	std::shared_ptr<TextDocument> document;
	std::unique_ptr<Music> audio;
	PreviewKind kind;
};

PreviewKind kindForName( const std::string& name ) {
	if ( Image::isImageExtension( name ) )
		return PreviewKind::Image;
	if ( EE::Audio::SoundFileFactory::isKnownFileExtension( name ) )
		return PreviewKind::Audio;
	return PreviewKind::Text;
}

} // namespace

PreviewPanel::PreviewPanel( App& app, UISplitter* splitter ) : mApp( app ), mSplitter( splitter ) {
	static_assert( sizeof( PreviewPanel ) <= sizeof( URI ) + 192 );
}

PreviewPanel::~PreviewPanel() = default;

bool PreviewPanel::isVisible() const {
	return mRoot && mRoot->getParent() == mSplitter;
}

void PreviewPanel::create() {
	Language::LanguagesSyntaxHighlighting::load();
	mApp.ui()->loadLayoutFromString( Layout, mApp.parking() );
	mRoot = mApp.ui()->find<UIWidget>( "quick_preview" );
	mContent = mRoot->find<UIWidget>( "preview_content" );
	mName = mRoot->find<UITextView>( "preview_name" );
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
		if ( !isVisible() ) {
			mRoot->setParent( mSplitter );
			mRoot->setVisible( true );
			mSplitter->setSplitPartition( StyleSheetLength( mPartition ) );
		}
		syncSelection();
	} else if ( isVisible() ) {
		rememberSize();
		clear();
		mRoot->setVisible( false );
		mRoot->setParent( mApp.parking() );
		if ( auto* tab = mApp.panes().currentTab() )
			tab->view->setFocus();
	}
}

void PreviewPanel::rememberSize() {
	if ( !isVisible() || mSplitter->getPixelsSize().x <= 0 )
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
	mLoader->setVisible( false );
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
	mMessage->setText( text );
	mMessage->setVisible( true );
}

void PreviewPanel::syncSelection() {
	if ( !isVisible() || !mApp.panes().ready() )
		return;
	auto* tab = mApp.panes().currentTab();
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
	if ( info.isDirectory() || ( !info.isRegularFile() && !info.isSymlink() ) ) {
		showMessage(
			mApp.ui()->i18n( "eefm_preview_not_file", "Select a regular file to preview." ) );
		return;
	}
	mMessage->setVisible( false );
	mLoader->setVisible( true );
	// Arrow-key navigation need not start a read and decode for every row crossed.
	mRoot->runOnMainThread( [this] { load(); }, Milliseconds( 100 ), LoadAction );
}

void PreviewPanel::load() {
	std::string name = mURI->getLastPathSegment();
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
			   [kind, token, name = std::move( name )](
				   VFSResult<std::unique_ptr<EE::VFS::IOStreamVFS>>&& stream ) {
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
					mEditor = UICodeEditor::New();
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
					mImage->on( Event::OnResourceLoadFailed, [this]( const Event* ) {
						if ( isVisible() && mImage->isVisible() ) {
							showMessage( mApp.ui()->i18n( "eefm_preview_invalid_image",
														  "This image could not be decoded." ) );
						}
					} );
				}
				mImage->setVisible( true );
				mImage->loadImageAsync( std::move( preview.stream.value() ) );
				mLoader->setVisible( false );
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
