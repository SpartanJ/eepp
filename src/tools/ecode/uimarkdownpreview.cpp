#include "uimarkdownpreview.hpp"

namespace ecode {

UIMarkdownPreview::UIMarkdownPreview( std::string sourcePath ) :
	mSourcePath( std::move( sourcePath ) ) {
	addClass( "markdown-preview" );
	setHistoryNavigationEnabled( true );
}

const std::string& UIMarkdownPreview::getSourcePath() const {
	return mSourcePath;
}

void UIMarkdownPreview::loadHistoryDocument( const std::string& path ) {
	auto doc = mSourceDocument.lock();
	if ( doc && !doc->isLoading() && doc->getFilePath() == path )
		mMarkdownView->loadFromString( doc->toUtf8String(), path );
	else
		UIScrollableMarkdownView::loadHistoryDocument( path );
}

void UIMarkdownPreview::bindSource( UICodeEditor* editor ) {
	mMarkdownView->removeActionsByTag( (Action::UniqueID)mMarkdownView );
	mSourcePath = editor->getDocument().getFilePath();
	mSourceDocument = editor->getDocumentRef();
	mTextChangedConnection = editor->connect( Event::OnTextChanged, [this]( const Event* ) {
		mMarkdownView->debounce( [this] { updateFromSource(); }, Milliseconds( 400 ),
								 (Action::UniqueID)mMarkdownView );
	} );
	updateFromSource();
}

void UIMarkdownPreview::updateFromSource() {
	// The connection expires when the source editor is destroyed; the preview can remain open.
	if ( !mTextChangedConnection )
		return;
	auto doc = mSourceDocument.lock();
	if ( doc && !doc->isLoading() && mMarkdownView->getDocumentPath() == doc->getFilePath() )
		mMarkdownView->loadFromString( doc->toUtf8String(), doc->getFilePath() );
}

} // namespace ecode
