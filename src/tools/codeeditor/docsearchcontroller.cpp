#include "docsearchcontroller.hpp"
#include "codeeditor.hpp"

DocSearchController::DocSearchController( UICodeEditorSplitter* editorSplitter, App* app ) :
	mEditorSplitter( editorSplitter ), mApp( app ) {}

void DocSearchController::initSearchBar( UISearchBar* searchBar ) {
	mSearchBarLayout = searchBar;
	mSearchBarLayout->setVisible( false )->setEnabled( false );
	auto addClickListener = [&]( UIWidget* widget, std::string cmd ) {
		widget->addEventListener( Event::MouseClick, [this, cmd]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK )
				mSearchBarLayout->execute( cmd );
		} );
	};
	auto addReturnListener = [&]( UIWidget* widget, std::string cmd ) {
		widget->addEventListener( Event::OnPressEnter, [this, cmd]( const Event* ) {
			mSearchBarLayout->execute( cmd );
		} );
	};
	UITextInput* findInput = mSearchBarLayout->find<UITextInput>( "search_find" );
	UITextInput* replaceInput = mSearchBarLayout->find<UITextInput>( "search_replace" );
	UICheckBox* caseSensitiveChk = mSearchBarLayout->find<UICheckBox>( "case_sensitive" );
	UICheckBox* wholeWordChk = mSearchBarLayout->find<UICheckBox>( "whole_word" );
	UICheckBox* luaPatternChk = mSearchBarLayout->find<UICheckBox>( "lua_pattern" );

	caseSensitiveChk->addEventListener(
		Event::OnValueChange, [&, caseSensitiveChk]( const Event* ) {
			mSearchState.caseSensitive = caseSensitiveChk->isChecked();
		} );

	wholeWordChk->addEventListener( Event::OnValueChange, [&, wholeWordChk]( const Event* ) {
		mSearchState.wholeWord = wholeWordChk->isChecked();
	} );

	luaPatternChk->addEventListener( Event::OnValueChange, [&, luaPatternChk]( const Event* ) {
		mSearchState.type = luaPatternChk->isChecked() ? TextDocument::FindReplaceType::LuaPattern
													   : TextDocument::FindReplaceType::Normal;
	} );

	findInput->addEventListener( Event::OnTextChanged, [&, findInput]( const Event* ) {
		if ( mSearchState.editor && mEditorSplitter->editorExists( mSearchState.editor ) ) {
			mSearchState.text = findInput->getText();
			mSearchState.editor->setHighlightWord( mSearchState.text );
			if ( !mSearchState.text.empty() ) {
				mSearchState.editor->getDocument().setSelection( { 0, 0 } );
				if ( !findNextText( mSearchState ) ) {
					findInput->addClass( "error" );
				} else {
					findInput->removeClass( "error" );
				}
			} else {
				findInput->removeClass( "error" );
				mSearchState.editor->getDocument().setSelection(
					mSearchState.editor->getDocument().getSelection().start() );
			}
		}
	} );
	mSearchBarLayout->addCommand( "close-searchbar", [&] {
		hideSearchBar();
		if ( mEditorSplitter->getCurEditor() )
			mEditorSplitter->getCurEditor()->setFocus();
		if ( mSearchState.editor ) {
			if ( mEditorSplitter->editorExists( mSearchState.editor ) ) {
				mSearchState.editor->setHighlightWord( "" );
				mSearchState.editor->setHighlightTextRange( TextRange() );
			}
		}
	} );
	mSearchBarLayout->addCommand( "repeat-find", [this] { findNextText( mSearchState ); } );
	mSearchBarLayout->addCommand( "replace-all", [this, replaceInput] {
		replaceAll( mSearchState, replaceInput->getText() );
		replaceInput->setFocus();
	} );
	mSearchBarLayout->addCommand( "find-and-replace", [this, replaceInput] {
		findAndReplace( mSearchState, replaceInput->getText() );
	} );
	mSearchBarLayout->addCommand( "find-prev", [this] { findPrevText( mSearchState ); } );
	mSearchBarLayout->addCommand( "replace-selection", [this, replaceInput] {
		replaceSelection( mSearchState, replaceInput->getText() );
	} );
	mSearchBarLayout->addCommand( "change-case", [&, caseSensitiveChk] {
		caseSensitiveChk->setChecked( !caseSensitiveChk->isChecked() );
	} );
	mSearchBarLayout->addCommand( "change-whole-word", [&, wholeWordChk] {
		wholeWordChk->setChecked( !wholeWordChk->isChecked() );
	} );
	mSearchBarLayout->addCommand( "toggle-lua-pattern", [&, luaPatternChk] {
		luaPatternChk->setChecked( !luaPatternChk->isChecked() );
	} );
	mSearchBarLayout->getKeyBindings().addKeybindsString( { { "f3", "repeat-find" },
															{ "ctrl+g", "repeat-find" },
															{ "escape", "close-searchbar" },
															{ "ctrl+r", "replace-all" },
															{ "ctrl+s", "change-case" },
															{ "ctrl+w", "change-whole-word" },
															{ "ctrl+l", "toggle-lua-pattern" } } );
	addReturnListener( findInput, "repeat-find" );
	addReturnListener( replaceInput, "find-and-replace" );
	addClickListener( mSearchBarLayout->find<UIPushButton>( "find_prev" ), "find-prev" );
	addClickListener( mSearchBarLayout->find<UIPushButton>( "find_next" ), "repeat-find" );
	addClickListener( mSearchBarLayout->find<UIPushButton>( "replace" ), "replace-selection" );
	addClickListener( mSearchBarLayout->find<UIPushButton>( "replace_find" ), "find-and-replace" );
	addClickListener( mSearchBarLayout->find<UIPushButton>( "replace_all" ), "replace-all" );
	addClickListener( mSearchBarLayout->find<UIWidget>( "searchbar_close" ), "close-searchbar" );
	replaceInput->addEventListener( Event::OnTabNavigate,
									[findInput]( const Event* ) { findInput->setFocus(); } );
}

void DocSearchController::showFindView() {
	mApp->hideLocateBar();
	mApp->hideGlobalSearchBar();

	UICodeEditor* editor = mEditorSplitter->getCurEditor();
	if ( !editor )
		return;

	mSearchState.editor = editor;
	mSearchState.range = TextRange();
	mSearchState.caseSensitive =
		mSearchBarLayout->find<UICheckBox>( "case_sensitive" )->isChecked();
	mSearchState.wholeWord = mSearchBarLayout->find<UICheckBox>( "whole_word" )->isChecked();
	mSearchBarLayout->setEnabled( true )->setVisible( true );

	UITextInput* findInput = mSearchBarLayout->find<UITextInput>( "search_find" );
	findInput->getDocument().selectAll();
	findInput->setFocus();

	const TextDocument& doc = editor->getDocument();

	if ( doc.getSelection().hasSelection() && doc.getSelection().inSameLine() ) {
		String text = doc.getSelectedText();
		if ( !text.empty() ) {
			findInput->setText( text );
			findInput->getDocument().selectAll();
		} else if ( !findInput->getText().empty() ) {
			findInput->getDocument().selectAll();
		}
	} else if ( doc.getSelection().hasSelection() ) {
		mSearchState.range = doc.getSelection( true );
		if ( !findInput->getText().empty() )
			findInput->getDocument().selectAll();
	}
	mSearchState.text = findInput->getText();
	editor->setHighlightTextRange( mSearchState.range );
	editor->setHighlightWord( mSearchState.text );
	editor->getDocument().setActiveClient( editor );
}

bool DocSearchController::findPrevText( SearchState& search ) {
	if ( search.text.empty() )
		search.text = mLastSearch;
	if ( !search.editor || !mEditorSplitter->editorExists( search.editor ) || search.text.empty() )
		return false;

	search.editor->getDocument().setActiveClient( search.editor );
	mLastSearch = search.text;
	TextDocument& doc = search.editor->getDocument();
	TextRange range = doc.getDocRange();
	TextPosition from = doc.getSelection( true ).start();
	if ( search.range.isValid() ) {
		range = doc.sanitizeRange( search.range ).normalized();
		from = from < range.start() ? range.start() : from;
	}

	TextPosition found =
		doc.findLast( search.text, from, search.caseSensitive, search.wholeWord, search.range );
	if ( found.isValid() ) {
		doc.setSelection( { doc.positionOffset( found, search.text.size() ), found } );
		return true;
	} else {
		found = doc.findLast( search.text, range.end() );
		if ( found.isValid() ) {
			doc.setSelection( { doc.positionOffset( found, search.text.size() ), found } );
			return true;
		}
	}
	return false;
}

bool DocSearchController::findNextText( SearchState& search ) {
	if ( search.text.empty() )
		search.text = mLastSearch;
	if ( !search.editor || !mEditorSplitter->editorExists( search.editor ) || search.text.empty() )
		return false;

	search.editor->getDocument().setActiveClient( search.editor );
	mLastSearch = search.text;
	TextDocument& doc = search.editor->getDocument();
	TextRange range = doc.getDocRange();
	TextPosition from = doc.getSelection( true ).end();
	if ( search.range.isValid() ) {
		range = doc.sanitizeRange( search.range ).normalized();
		from = from < range.start() ? range.start() : from;
	}

	TextRange found =
		doc.find( search.text, from, search.caseSensitive, search.wholeWord, search.type, range );
	if ( found.isValid() ) {
		doc.setSelection( found.reversed() );
		return true;
	} else {
		found = doc.find( search.text, range.start(), search.caseSensitive, search.wholeWord,
						  search.type, range );
		if ( found.isValid() ) {
			doc.setSelection( found.reversed() );
			return true;
		}
	}
	return false;
}

bool DocSearchController::replaceSelection( SearchState& search, const String& replacement ) {
	if ( !search.editor || !mEditorSplitter->editorExists( search.editor ) ||
		 !search.editor->getDocument().hasSelection() )
		return false;
	search.editor->getDocument().setActiveClient( search.editor );
	search.editor->getDocument().replaceSelection( replacement );
	return true;
}

int DocSearchController::replaceAll( SearchState& search, const String& replace ) {
	if ( !search.editor || !mEditorSplitter->editorExists( search.editor ) )
		return 0;
	if ( search.text.empty() )
		search.text = mLastSearch;
	if ( search.text.empty() )
		return 0;
	search.editor->getDocument().setActiveClient( search.editor );
	mLastSearch = search.text;
	TextDocument& doc = search.editor->getDocument();
	TextPosition startedPosition = doc.getSelection().start();
	int count = doc.replaceAll( search.text, replace, search.caseSensitive, search.wholeWord,
								search.type, search.range );
	doc.setSelection( startedPosition );
	return count;
}

bool DocSearchController::findAndReplace( SearchState& search, const String& replace ) {
	if ( !search.editor || !mEditorSplitter->editorExists( search.editor ) )
		return false;
	if ( search.text.empty() )
		search.text = mLastSearch;
	if ( search.text.empty() )
		return false;
	search.editor->getDocument().setActiveClient( search.editor );
	mLastSearch = search.text;
	TextDocument& doc = search.editor->getDocument();
	if ( doc.hasSelection() && doc.getSelectedText() == search.text ) {
		return replaceSelection( search, replace );
	} else {
		return findNextText( search );
	}
}

void DocSearchController::hideSearchBar() {
	mSearchBarLayout->setEnabled( false )->setVisible( false );
}

void DocSearchController::onCodeEditorFocusChange( UICodeEditor* editor ) {

	if ( mSearchState.editor && mSearchState.editor != editor ) {
		String word = mSearchState.editor->getHighlightWord();
		mSearchState.editor->setHighlightWord( "" );
		mSearchState.editor->setHighlightTextRange( TextRange() );
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
