#include "docsearchcontroller.hpp"
#include "ecode.hpp"

namespace ecode {

DocSearchController::DocSearchController( UICodeEditorSplitter* editorSplitter, App* app ) :
	mSplitter( editorSplitter ), mApp( app ) {}

DocSearchController::~DocSearchController() {
	Clock clock;
	while ( mSearchingCount && clock.getElapsedTime() < Seconds( 1 ) )
		Sys::sleep( Milliseconds( 10 ) );
}

void DocSearchController::refreshHighlight() {
	if ( mSearchState.editor && mSplitter->editorExists( mSearchState.editor ) ) {
		mSearchState.text = mFindInput->getText();
		mSearchState.editor->setHighlightWord( mSearchState.toTextSearchParams() );
		if ( !mSearchState.text.empty() ) {
			findNextText( mSearchState, true );
		} else {
			mFindInput->removeClass( "error" );
			mSearchState.editor->getDocument().setSelection(
				mSearchState.editor->getDocument().getSelection().start() );
		}
	}
}

void DocSearchController::initSearchBar(
	UISearchBar* searchBar, const SearchBarConfig& searchBarConfig,
	std::unordered_map<std::string, std::string> keybindings ) {
	mSearchBarLayout = searchBar;
	mSearchBarLayout->setVisible( false )->setEnabled( false );
	auto addClickListener = [this]( UIWidget* widget, std::string cmd ) {
		widget->setTooltipText( mSearchBarLayout->getKeyBindings().getCommandKeybindString( cmd ) );
		widget->addEventListener( Event::MouseClick, [this, cmd]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK )
				mSearchBarLayout->execute( cmd );
		} );
	};
	auto addReturnListener = [this]( UIWidget* widget, std::string cmd ) {
		widget->addEventListener( Event::OnPressEnter, [this, cmd]( const Event* ) {
			mSearchBarLayout->execute( cmd );
		} );
	};

	auto& kbind = mSearchBarLayout->getKeyBindings();
	kbind.addKeybindsString(
		{ { mApp->getKeybind( "repeat-find" ), "repeat-find" },
		  { mApp->getKeybind( "find-prev" ), "find-prev" },
		  { mApp->getKeybind( "open-global-search" ), "open-global-search" },
		  { mApp->getKeybind( "select-all-results" ), "select-all-results" } } );
	kbind.addKeybindsStringUnordered( keybindings );

	mFindInput = mSearchBarLayout->find<UITextInput>( "search_find" );
	mFindInput->setEscapePastedText( true );
	mReplaceInput = mSearchBarLayout->find<UITextInput>( "search_replace" );
	mCaseSensitiveChk = mSearchBarLayout->find<UICheckBox>( "case_sensitive" );
	mEscapeSequenceChk = mSearchBarLayout->find<UICheckBox>( "escape_sequence" );
	mWholeWordChk = mSearchBarLayout->find<UICheckBox>( "whole_word" );
	mLuaPatternChk = mSearchBarLayout->find<UICheckBox>( "lua_pattern" );
	UIPushButton* replaceAllButton = mSearchBarLayout->find<UIPushButton>( "replace_all" );
	UIPushButton* findPrevButton = mSearchBarLayout->find<UIPushButton>( "find_prev" );
	UIPushButton* findNextButton = mSearchBarLayout->find<UIPushButton>( "find_next" );
	UIPushButton* replaceButton = mSearchBarLayout->find<UIPushButton>( "replace" );
	UIPushButton* findReplaceButton = mSearchBarLayout->find<UIPushButton>( "replace_find" );
	UIPushButton* selectAllButton = mSearchBarLayout->find<UIPushButton>( "select_all" );
	UIWidget* closeButton = mSearchBarLayout->find<UIWidget>( "searchbar_close" );
	mCaseSensitiveChk->setChecked( searchBarConfig.caseSensitive );
	mLuaPatternChk->setChecked( searchBarConfig.luaPattern );
	mWholeWordChk->setChecked( searchBarConfig.wholeWord );
	mEscapeSequenceChk->setChecked( searchBarConfig.escapeSequence );

	mLuaPatternChk->setTooltipText( kbind.getCommandKeybindString( "toggle-lua-pattern" ) );
	mCaseSensitiveChk->setTooltipText( kbind.getCommandKeybindString( "change-case" ) );
	mWholeWordChk->setTooltipText( kbind.getCommandKeybindString( "change-whole-word" ) );
	std::string kbindEscape = kbind.getCommandKeybindString( "change-escape-sequence" );
	if ( !kbindEscape.empty() )
		mEscapeSequenceChk->setTooltipText( mEscapeSequenceChk->getTooltipText() + " (" +
											kbindEscape + ")" );

	mCaseSensitiveChk->addEventListener( Event::OnValueChange, [this]( const Event* ) {
		mSearchState.caseSensitive = mCaseSensitiveChk->isChecked();
		refreshHighlight();
	} );

	mEscapeSequenceChk->addEventListener( Event::OnValueChange, [this]( const Event* ) {
		mSearchState.escapeSequences = mEscapeSequenceChk->isChecked();
		refreshHighlight();
	} );

	mWholeWordChk->addEventListener( Event::OnValueChange, [this]( const Event* ) {
		mSearchState.wholeWord = mWholeWordChk->isChecked();
		refreshHighlight();
	} );

	mLuaPatternChk->addEventListener( Event::OnValueChange, [this]( const Event* ) {
		mSearchState.type = mLuaPatternChk->isChecked() ? TextDocument::FindReplaceType::LuaPattern
														: TextDocument::FindReplaceType::Normal;
		refreshHighlight();
	} );

	mFindInput->addEventListener( Event::OnTextChanged,
								  [this]( const Event* ) { refreshHighlight(); } );
	mFindInput->addEventListener( Event::OnTextPasted, [this]( const Event* ) {
		if ( mFindInput->getUISceneNode()->getWindow()->getClipboard()->getText().find( '\n' ) !=
			 String::InvalidPos ) {
			if ( !mEscapeSequenceChk->isChecked() )
				mEscapeSequenceChk->setChecked( true );
		}
	} );
	mSearchBarLayout->setCommand( "close-searchbar", [this] {
		hideSearchBar();
		if ( mSplitter->getCurWidget() )
			mSplitter->getCurWidget()->setFocus();
		if ( mSearchState.editor ) {
			if ( mSplitter->editorExists( mSearchState.editor ) ) {
				mSearchState.editor->setHighlightWord( { "" } );
				mSearchState.editor->setHighlightTextRange( TextRange() );
			}
		}
	} );
	mSearchBarLayout->setCommand( "repeat-find", [this] { findNextText( mSearchState ); } );
	mSearchBarLayout->setCommand( "replace-all", [this] {
		size_t count = replaceAll( mSearchState, mReplaceInput->getText() );
		mApp->getNotificationCenter()->addNotification(
			String::format( "Replaced %zu occurrences.", count ) );
		mReplaceInput->setFocus();
	} );
	mSearchBarLayout->setCommand( "select-all-results", [this] { selectAll( mSearchState ); } );
	mSearchBarLayout->setCommand(
		"find-and-replace", [this] { findAndReplace( mSearchState, mReplaceInput->getText() ); } );
	mSearchBarLayout->setCommand( "find-prev", [this] { findPrevText( mSearchState ); } );
	mSearchBarLayout->setCommand( "replace-selection", [this] {
		replaceSelection( mSearchState, mReplaceInput->getText() );
	} );
	mSearchBarLayout->setCommand( "change-case", [this] {
		mCaseSensitiveChk->setChecked( !mCaseSensitiveChk->isChecked() );
	} );
	mSearchBarLayout->setCommand(
		"change-whole-word", [this] { mWholeWordChk->setChecked( !mWholeWordChk->isChecked() ); } );
	mSearchBarLayout->setCommand( "change-escape-sequence", [this] {
		mEscapeSequenceChk->setChecked( !mEscapeSequenceChk->isChecked() );
	} );
	mSearchBarLayout->setCommand( "toggle-lua-pattern", [this] {
		mLuaPatternChk->setChecked( !mLuaPatternChk->isChecked() );
	} );
	mSearchBarLayout->setCommand( "open-global-search",
								  [this] { mApp->showGlobalSearch( false ); } );

	addReturnListener( mFindInput, "repeat-find" );
	addReturnListener( mReplaceInput, "find-and-replace" );
	addClickListener( findPrevButton, "find-prev" );
	addClickListener( findNextButton, "repeat-find" );
	addClickListener( selectAllButton, "select-all-results" );
	addClickListener( replaceButton, "replace-selection" );
	addClickListener( findReplaceButton, "find-and-replace" );
	addClickListener( replaceAllButton, "replace-all" );
	addClickListener( closeButton, "close-searchbar" );

	mFindInput->setSelectAllDocOnTabNavigate( false );
	mReplaceInput->setSelectAllDocOnTabNavigate( false );
	mReplaceInput->addEventListener( Event::OnTabNavigate,
									 [this]( const Event* ) { mFindInput->setFocus(); } );
}

void DocSearchController::showFindView() {
	mApp->hideLocateBar();
	mApp->hideGlobalSearchBar();
	if ( !mSplitter->curEditorExistsAndFocused() )
		return;

	UICodeEditor* editor = mSplitter->getCurEditor();
	mSearchState.editor = editor;
	mSearchState.range = TextRange();
	mSearchState.caseSensitive = mCaseSensitiveChk->isChecked();
	mSearchState.wholeWord = mWholeWordChk->isChecked();
	mSearchState.escapeSequences = mEscapeSequenceChk->isChecked();
	mSearchState.type = mLuaPatternChk->isChecked() ? TextDocument::FindReplaceType::LuaPattern
													: TextDocument::FindReplaceType::Normal;
	mSearchBarLayout->setEnabled( true )->setVisible( true );

	mFindInput->getDocument().selectAll();
	mFindInput->setFocus();

	const TextDocument& doc = editor->getDocument();

	if ( doc.getSelection().hasSelection() ) {
		String text = doc.getSelectedText();

		if ( !doc.getSelection().inSameLine() )
			mSearchState.range = doc.getSelection( true );

		if ( !text.empty() && doc.getSelection().inSameLine() ) {
			mFindInput->setText( text );
			mFindInput->getDocument().selectAll();
		} else if ( !mFindInput->getText().empty() ) {
			mFindInput->getDocument().selectAll();
		}
	}
	mSearchState.text = mFindInput->getText();
	editor->setHighlightTextRange( mSearchState.range );
	editor->setHighlightWord( mSearchState.toTextSearchParams() );
	editor->getDocument().setActiveClient( editor );
}

void DocSearchController::findPrevText( SearchState& search ) {
	if ( search.text.empty() )
		search.text = mLastSearch;
	if ( !search.editor || !mSplitter->editorExists( search.editor ) || search.text.empty() )
		return;

	search.editor->getDocument().setActiveClient( search.editor );
	mLastSearch = search.text;

	Uint64 tag = String::hash( "DocSearchController::findPrevText-" +
							   search.editor->getDocument().getHashHexString() );
	mApp->getThreadPool()->removeWithTag( tag );
	mApp->getThreadPool()->run( [this, search] {
		ScopedOp op( [this] { mSearchingCount++; }, [this] { mSearchingCount--; } );
		TextDocument& doc = search.editor->getDocument();
		TextRange range = doc.getDocRange();
		TextPosition from = doc.getSelection( true ).start();
		if ( search.range.isValid() ) {
			range = doc.sanitizeRange( search.range ).normalized();
			from = from < range.start() ? range.start() : from;
		}

		String txt( search.text );
		if ( search.escapeSequences )
			txt.unescape();

		TextRange found = doc.findLast( txt, from, search.caseSensitive, search.wholeWord,
										search.type, search.range )
							  .result;
		if ( found.isValid() ) {
			doc.setSelection( found );
			mFindInput->runOnMainThread( [this, search] {
				mSplitter->addEditorPositionToNavigationHistory( search.editor );
				mFindInput->removeClass( "error" );
			} );
			return;
		} else {
			found = doc.findLast( txt, range.end(), search.caseSensitive, search.wholeWord,
								  search.type, range )
						.result;
			if ( found.isValid() ) {
				doc.setSelection( found );
				mFindInput->runOnMainThread( [this, search] {
					mSplitter->addEditorPositionToNavigationHistory( search.editor );
					mFindInput->removeClass( "error" );
				} );
				return;
			}
		}
		mFindInput->runOnMainThread( [this] { mFindInput->addClass( "error" ); } );
	} );
}

void DocSearchController::findNextText( SearchState& search, bool resetSelection ) {
	if ( search.text.empty() )
		search.text = mLastSearch;
	if ( !search.editor || !mSplitter->editorExists( search.editor ) || search.text.empty() )
		return;

	search.editor->getDocument().setActiveClient( search.editor );
	mLastSearch = search.text;

	Uint64 tag = String::hash( "DocSearchController::findNextText-" +
							   search.editor->getDocument().getHashHexString() );

	mApp->getThreadPool()->removeWithTag( tag );
	mApp->getThreadPool()->run(
		[this, search, resetSelection] {
			ScopedOp op( [this] { mSearchingCount++; }, [this] { mSearchingCount--; } );
			TextDocument& doc = search.editor->getDocument();
			TextRange range = doc.getDocRange();
			TextPosition from =
				resetSelection ? TextPosition( 0, 0 ) : doc.getSelection( true ).end();

			if ( search.range.isValid() ) {
				range = doc.sanitizeRange( search.range ).normalized();
				from = from < range.start() ? range.start() : from;
			}

			String txt( search.text );
			if ( search.escapeSequences )
				txt.unescape();

			TextRange found =
				doc.find( txt, from, search.caseSensitive, search.wholeWord, search.type, range )
					.result;

			if ( found.isValid() ) {
				doc.setSelection( found.reversed() );
				mFindInput->runOnMainThread( [this, search] {
					mSplitter->addEditorPositionToNavigationHistory( search.editor );
					mFindInput->removeClass( "error" );
				} );
				return;
			} else {
				found = doc.find( txt, range.start(), search.caseSensitive, search.wholeWord,
								  search.type, range )
							.result;
				if ( found.isValid() ) {
					doc.setSelection( found.reversed() );
					mFindInput->runOnMainThread( [this, search] {
						mSplitter->addEditorPositionToNavigationHistory( search.editor );
						mFindInput->removeClass( "error" );
					} );
					return;
				}
			}

			mFindInput->runOnMainThread( [this] { mFindInput->addClass( "error" ); } );
		},
		{}, tag );
}

bool DocSearchController::replaceSelection( SearchState& search, const String& replacement ) {
	if ( !search.editor || !mSplitter->editorExists( search.editor ) ||
		 !search.editor->getDocument().hasSelection() )
		return false;
	search.editor->getDocument().setActiveClient( search.editor );
	search.editor->getDocument().replace(
		search.text, replacement, search.editor->getDocument().getSelection().normalized().start(),
		search.caseSensitive, search.wholeWord, search.type,
		search.editor->getDocument().getSelection().normalized() );
	return true;
}

void DocSearchController::selectAll( SearchState& search ) {
	if ( !search.editor || !mSplitter->editorExists( search.editor ) )
		return;
	if ( search.text.empty() )
		search.text = mLastSearch;
	if ( search.text.empty() )
		return;
	search.editor->getDocument().setActiveClient( search.editor );
	mLastSearch = search.text;
	TextDocument& doc = search.editor->getDocument();
	auto ranges = doc.findAll( search.text, search.caseSensitive, search.wholeWord, search.type,
							   search.range );
	for ( const auto& range : ranges )
		doc.addSelection( range.result.reversed() );
}

int DocSearchController::replaceAll( SearchState& search, const String& replace ) {
	if ( !search.editor || !mSplitter->editorExists( search.editor ) )
		return 0;
	if ( search.text.empty() )
		search.text = mLastSearch;
	if ( search.text.empty() )
		return 0;
	search.editor->getDocument().setActiveClient( search.editor );
	mLastSearch = search.text;
	TextDocument& doc = search.editor->getDocument();
	TextPosition startedPosition = doc.getSelection().start();

	String txt( search.text );
	String repl( replace );
	if ( search.escapeSequences ) {
		txt.unescape();
		repl.unescape();
	}

	int count = doc.replaceAll( txt, repl, search.caseSensitive, search.wholeWord, search.type,
								search.range );
	doc.setSelection( startedPosition );
	return count;
}

void DocSearchController::findAndReplace( SearchState& search, const String& replace ) {
	if ( !search.editor || !mSplitter->editorExists( search.editor ) )
		return;
	if ( search.text.empty() )
		search.text = mLastSearch;
	if ( search.text.empty() )
		return;
	search.editor->getDocument().setActiveClient( search.editor );
	mLastSearch = search.text;
	TextDocument& doc = search.editor->getDocument();

	String txt( search.text );
	String repl( replace );
	if ( search.escapeSequences ) {
		txt.unescape();
		repl.unescape();
	}

	if ( doc.hasSelection() &&
		 ( doc.getSelectedText() == txt ||
		   ( search.type == TextDocument::FindReplaceType::LuaPattern &&
			 LuaPattern::matches( doc.getAllSelectedText().toUtf8(), txt.toUtf8() ) ) ) ) {
		replaceSelection( search, repl );
	} else {
		findNextText( search );
	}
}

void DocSearchController::hideSearchBar() {
	mSearchBarLayout->setEnabled( false )->setVisible( false );
}

void DocSearchController::onCodeEditorFocusChange( UICodeEditor* editor ) {
	if ( mSearchState.editor && mSearchState.editor != editor ) {
		TextSearchParams word;
		if ( mSplitter->editorExists( mSearchState.editor ) ) {
			word = mSearchState.editor->getHighlightWord();
			mSearchState.editor->setHighlightWord( { "" } );
			mSearchState.editor->setHighlightTextRange( TextRange() );
		} else {
			mSearchState.editor = nullptr;
		}
		mSearchState.text = "";
		mSearchState.range = TextRange();
		if ( editor ) {
			mSearchState.editor = editor;
			mSearchState.editor->setHighlightWord( word );
			mSearchState.range = TextRange();
		}
	}
}

SearchState& DocSearchController::getSearchState() {
	return mSearchState;
}

SearchBarConfig DocSearchController::getSearchBarConfig() const {
	SearchBarConfig searchBarConfig;
	searchBarConfig.caseSensitive = mCaseSensitiveChk->isChecked();
	searchBarConfig.luaPattern = mLuaPatternChk->isChecked();
	searchBarConfig.wholeWord = mWholeWordChk->isChecked();
	searchBarConfig.escapeSequence = mEscapeSequenceChk->isChecked();
	return searchBarConfig;
}

} // namespace ecode
