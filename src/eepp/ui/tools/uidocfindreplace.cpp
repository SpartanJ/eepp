#include <eepp/scene/actions/actions.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/tools/uidocfindreplace.hpp>
#include <eepp/ui/tools/uifindbarstyle.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/window.hpp>

namespace EE { namespace UI { namespace Tools {

const char DOC_FIND_REPLACE_XML[] = R"xml(
<hbox class="ce_find_replace_box" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="right|top" margin_right="32dp">
	<Widget class="expander" layout_width="2dp" layout_height="match_parent" />
	<PushButton class="find_replace_toggle" layout_width="16dp" layout_height="wrap_content" padding="2dp" layout_gravity="center" />
	<vbox layout_width="wrap_content" layout_height="wrap_content">
		<hbox layout_width="340dp" layout_height="wrap_content" paddingRight="1dp" min-width="200dp" clip="border-box">
			<RelativeLayout layout_width="0dp" layout_weight="1" layout_height="25dp" marginRight="1dp">
				<TextInput class="input-find" layout_width="match_parent" layout_height="wrap_content" hint="Find" />
				<selectbutton id="ce_find_replace_box-escape-sequences"
							  layout_width="wrap_content" layout_height="wrap_content" class="escape-sequences"
							  layout_gravity="right|center_vertical" tooltip="@string(escape_sequences, Escape Sequences)"
							  marginRight="2dp" text="\n" />
				<selectbutton id="ce_find_replace_box-match-case" class="match-case"
							  layout_width="wrap_content" layout_height="wrap_content"
							  layout_gravity="right|center_vertical" tooltip="@string(match_case, Match Case)" marginRight="2dp"
							  layout_to_left_of="ce_find_replace_box-escape-sequences" />
				<selectbutton id="ce_find_replace_box-whole-word" class="whole-word"
							  layout_width="wrap_content" layout_height="wrap_content"
							  layout_gravity="right|center_vertical" tooltip="@string(whole_word, Whole Word)"
							  layout_to_left_of="ce_find_replace_box-match-case" marginRight="2dp" />
				<selectbutton id="ce_find_replace_box-regex" class="regex" layout_width="wrap_content"
							  layout_height="wrap_content" layout_gravity="right|center_vertical"
							  tooltip="@string(regex_match, Regular Expression Match)" layout_to_left_of="ce_find_replace_box-whole-word"
							  marginRight="2dp" />
				<selectbutton id="ce_find_replace_box-luapattern" class="luapattern" layout_width="wrap_content"
							  layout_height="wrap_content" layout_gravity="right|center_vertical"
							  tooltip="@string(lua_pattern_match, Lua Pattern Match)" layout_to_left_of="ce_find_replace_box-regex"
							  marginRight="2dp" />
			</RelativeLayout>
			<PushButton class="prev-button" layout_width="wrap_content" layout_height="24dp" text="Prev." icon="icon(arrow-up, 16dp)" />
			<PushButton class="next-button" layout_width="wrap_content" layout_height="24dp" text="Next" icon="icon(arrow-down, 16dp)" />
			<PushButton class="exit-button" layout_width="wrap_content" layout_height="24dp" text="Close" icon="icon(cancel, 16dp)" />
		</hbox>
		<hbox class="replace_box" layout_width="match_parent" layout_height="wrap_content">
			<TextInput class="input-replace" layout_width="200dp" layout_height="wrap_content" hint="Replace" marginRight="1dp" />

			<PushButton class="replace-button" layout_width="wrap_content" layout_height="wrap_content" tooltip="@string(replace, Replace)" />

			<PushButton class="replace-all-button" layout_width="wrap_content" layout_height="wrap_content" tooltip="@string(replace_all, Replace All)" />
		</hbox>
	</vbox>
</hbox>
)xml";

UIDocFindReplace*
UIDocFindReplace::New( UIWidget* parent, const std::shared_ptr<Doc::TextDocument>& doc,
					   std::unordered_map<std::string, std::string> keybindings ) {
	return eeNew( UIDocFindReplace, ( parent, doc, keybindings ) );
}

const std::shared_ptr<Doc::TextDocument>& UIDocFindReplace::getDoc() const {
	return mDoc;
}

void UIDocFindReplace::setDoc( const std::shared_ptr<Doc::TextDocument>& doc ) {
	mDoc = doc;
}

UIDocFindReplace::UIDocFindReplace( UIWidget* parent, const std::shared_ptr<Doc::TextDocument>& doc,
									std::unordered_map<std::string, std::string> keybindings ) :
	UILinearLayout( "docfindreplace", UIOrientation::Horizontal ),
	WidgetCommandExecuter( getInput() ),
	mDoc( doc ) {
	mFlags |= UI_OWNS_CHILDREN_POSITION;

	getKeyBindings().addKeybindsStringUnordered( keybindings );

	UIFindBarStyle::ensure( parent->getUISceneNode() );

	parent->getUISceneNode()->loadLayoutFromMemory( DOC_FIND_REPLACE_XML,
													eeARRAY_SIZE( DOC_FIND_REPLACE_XML ), this );

	setParent( parent );

	mFindReplaceToggle = querySelector( ".find_replace_toggle" );
	mReplaceBox = querySelector( ".replace_box" );
	mToggle = querySelector( ".find_replace_toggle" );
	mToggle->on( Event::MouseClick, [this]( const Event* event ) {
		if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK ) {
			if ( mToggle->hasClass( "enabled" ) ) {
				mToggle->removeClass( "enabled" );
				mReplaceBox->removeClass( "enabled" );
			} else {
				mToggle->addClass( "enabled" );
				mReplaceBox->addClass( "enabled" );
			}
		}
	} );
	bind( "ce_find_replace_box-match-case", mCaseSensitive );
	bind( "ce_find_replace_box-escape-sequences", mEscapeSequences );
	bind( "ce_find_replace_box-whole-word", mWholeWord );
	bind( "ce_find_replace_box-luapattern", mLuaPattern );
	bind( "ce_find_replace_box-regex", mRegEx );

	mFindInput = querySelector<UITextInput>( ".input-find" );
	mFindInput->setEscapePastedText( true );
	UICodeEditor* editor =
		getParent()->isType( UI_TYPE_CODEEDITOR ) ? getParent()->asType<UICodeEditor>() : nullptr;

	mFindInput->on( Event::OnTextChanged,
					[this, editor]( const Event* ) { refreshHighlight( editor ); } );
	mFindInput->on( Event::OnTextPasted, [this]( const Event* ) {
		if ( mFindInput->getUISceneNode()->getWindow()->getClipboard()->getText().find( '\n' ) !=
			 String::InvalidPos ) {
			if ( !mEscapeSequences->isSelected() )
				mEscapeSequences->setSelected( true );
		}
	} );

	setCommand( "close-find-replace", [this] { hide(); } );
	setCommand( "repeat-find", [this] { findNextText( mSearchState ); } );
	setCommand( "replace-all", [this] {
		if ( mReplaceDisabled )
			return;
		/*size_t count = */ replaceAll( mSearchState, mReplaceInput->getText() );
		mReplaceInput->setFocus();
	} );
	setCommand( "find-and-replace", [this] {
		if ( mReplaceDisabled )
			return;
		findAndReplace( mSearchState, mReplaceInput->getText() );
	} );
	setCommand( "find-prev", [this] { findPrevText( mSearchState ); } );
	setCommand( "replace-selection", [this] {
		if ( mReplaceDisabled )
			return;
		replaceSelection( mSearchState, mReplaceInput->getText() );
	} );
	setCommand( "change-case", [this, editor] {
		mCaseSensitive->setSelected( !mCaseSensitive->isSelected() );
		refreshHighlight( editor );
	} );
	setCommand( "change-whole-word", [this, editor] {
		mWholeWord->setSelected( !mWholeWord->isSelected() );
		refreshHighlight( editor );
	} );
	setCommand( "change-escape-sequence", [this, editor] {
		mEscapeSequences->setSelected( !mEscapeSequences->isSelected() );
		refreshHighlight( editor );
	} );
	setCommand( "toggle-lua-pattern", [this, editor] {
		mLuaPattern->setSelected( !mLuaPattern->isSelected() );
		refreshHighlight( editor );
	} );
	setCommand( "toggle-regex", [this, editor] {
		mRegEx->setSelected( !mRegEx->isSelected() );
		refreshHighlight( editor );
	} );

	mCaseSensitive->setTooltipText( mCaseSensitive->getTooltipText() + " (" +
									getKeyBindings().getCommandKeybindString( "change-case" ) +
									")" );
	mWholeWord->setTooltipText( mWholeWord->getTooltipText() + " (" +
								getKeyBindings().getCommandKeybindString( "change-whole-word" ) +
								")" );
	mEscapeSequences->setTooltipText(
		mEscapeSequences->getTooltipText() + " (" +
		getKeyBindings().getCommandKeybindString( "change-escape-sequence" ) + ")" );
	mLuaPattern->setTooltipText( mLuaPattern->getTooltipText() + " (" +
								 getKeyBindings().getCommandKeybindString( "toggle-lua-pattern" ) +
								 ")" );
	mRegEx->setTooltipText( mRegEx->getTooltipText() + " (" +
							getKeyBindings().getCommandKeybindString( "toggle-regex" ) + ")" );
	mReplaceInput = querySelector<UITextInput>( ".input-replace" );

	auto addClickListener = [this]( UIWidget* widget, std::string cmd ) {
		if ( !widget )
			return;
		widget->setTooltipText( getKeyBindings().getCommandKeybindString( cmd ) );
		widget->on( Event::MouseClick, [this, cmd]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK )
				execute( cmd );
		} );
	};
	auto addReturnListener = [this]( UIWidget* widget, std::string cmd ) {
		widget->on( Event::OnPressEnter, [this, cmd]( const Event* ) { execute( cmd ); } );
	};
	addReturnListener( mFindInput, "repeat-find" );
	addReturnListener( mReplaceInput, "find-and-replace" );
	addClickListener( querySelector( ".ce_find_replace_box .prev-button" ), "find-prev" );
	addClickListener( querySelector( ".ce_find_replace_box .next-button" ), "repeat-find" );
	addClickListener( querySelector( ".ce_find_replace_box .replace-button" ),
					  "replace-selection" );
	addClickListener( querySelector( ".ce_find_replace_box .replace-all-button" ), "replace-all" );
	addClickListener( querySelector( ".ce_find_replace_box .exit-button" ), "close-find-replace" );

	mFindInput->setTabStop();
	mReplaceInput->setTabStop();

	mFindInput->setSelectAllDocOnTabNavigate( false );
	mReplaceInput->setSelectAllDocOnTabNavigate( false );
	mFindInput->on( Event::OnTabNavigate, [this]( const Event* ) {
		if ( mReplaceDisabled )
			return;
		if ( !mToggle->hasClass( "enabled" ) ) {
			mToggle->addClass( "enabled" );
			mReplaceBox->addClass( "enabled" );
		}
		mReplaceInput->setFocus();
	} );

	mReplaceInput->on( Event::OnTabNavigate, [this]( const Event* ) { mFindInput->setFocus(); } );

	mDataBinds.emplace_back( UIDataBindBool::New( &mSearchState.caseSensitive, mCaseSensitive ) );
	mDataBinds.emplace_back( UIDataBindBool::New( &mSearchState.wholeWord, mWholeWord ) );
	mDataBinds.emplace_back(
		UIDataBindBool::New( &mSearchState.escapeSequences, mEscapeSequences ) );

	mLuaPattern->on( Event::OnValueChange, [this, editor]( const Event* ) {
		if ( mChangingPattern )
			return;
		BoolScopedOp op( mChangingPattern, true );
		mSearchState.type = mLuaPattern->isSelected() ? TextDocument::FindReplaceType::LuaPattern
													  : TextDocument::FindReplaceType::Normal;
		mRegEx->setSelected( false );
		refreshHighlight( editor );
	} );

	mRegEx->on( Event::OnValueChange, [this, editor]( const Event* ) {
		if ( mChangingPattern )
			return;
		BoolScopedOp op( mChangingPattern, true );
		mSearchState.type = mRegEx->isSelected() ? TextDocument::FindReplaceType::RegEx
												 : TextDocument::FindReplaceType::Normal;
		mLuaPattern->setSelected( false );
		refreshHighlight( editor );
	} );

	auto valueChangeCb = [this, editor]( const auto& ) { refreshHighlight( editor ); };

	for ( const auto& db : mDataBinds )
		db->onValueChangeCb = valueChangeCb;

	setVisible( false );

	runOnMainThread( [this] { mReady = true; } );

	getParent()->on( Event::OnSizeChange, [this]( const Event* ) {
		Float startX = eemax( 0.f, getParent()->getSize().getWidth() - getSize().getWidth() );
		setPosition( startX, getPosition().y );
	} );
}

void UIDocFindReplace::show( bool expanded ) {
	if ( !mReady ) {
		runOnMainThread( [this] { show(); } );
		return;
	}

	if ( !isVisible() ) {
		setVisible( true );
		Float startX = eemax( 0.f, getParent()->getSize().getWidth() - getSize().getWidth() );
		setPosition( startX, -getSize().getHeight() );
		runAction( Actions::Move::New( { startX, getPosition().y }, { startX, 0 }, Seconds( 0.2f ),
									   Ease::QuadraticIn ) );
	}

	UICodeEditor* editor =
		getParent()->isType( UI_TYPE_CODEEDITOR ) ? getParent()->asType<UICodeEditor>() : nullptr;

	mSearchState.range = TextRange();

	mFindInput->getDocument().selectAll();
	mFindInput->setFocus();

	if ( mDoc->getSelection().hasSelection() ) {
		String text = mDoc->getSelectedText();

		if ( !mDoc->getSelection().inSameLine() )
			mSearchState.range = mDoc->getSelection( true );

		if ( !text.empty() && mDoc->getSelection().inSameLine() ) {
			mFindInput->setText( text );
			mFindInput->getDocument().selectAll();
		} else if ( !mFindInput->getText().empty() ) {
			mFindInput->getDocument().selectAll();
		}
	}

	mSearchState.text = mFindInput->getText();

	if ( !expanded ) {
		mToggle->removeClass( "enabled" );
		mReplaceBox->removeClass( "enabled" );
	} else {
		mToggle->addClass( "enabled" );
		mReplaceBox->addClass( "enabled" );
	}

	if ( editor ) {
		editor->setHighlightTextRange( mSearchState.range );
		editor->setHighlightWord( mSearchState );
		mDoc->setActiveClient( editor );
	}
}

void UIDocFindReplace::hide() {
	runAction( Actions::Sequence::New(
		Actions::Move::New( getPosition(), { getPosition().x, -getSize().getHeight() },
							Seconds( 0.2f ), Ease::QuadraticOut ),
		Actions::Visible::New( false ) ) );

	UICodeEditor* editor =
		getParent()->isType( UI_TYPE_CODEEDITOR ) ? getParent()->asType<UICodeEditor>() : nullptr;

	mSearchState.range = TextRange();
	mSearchState.text = "";
	if ( editor ) {
		editor->setHighlightWord( { "" } );
		editor->setHighlightTextRange( TextRange() );
	}

	getParent()->setFocus();
}

bool UIDocFindReplace::isReplaceDisabled() const {
	return mReplaceDisabled;
}

void UIDocFindReplace::setReplaceDisabled( bool replaceDisabled ) {
	if ( replaceDisabled != mReplaceDisabled ) {
		mReplaceDisabled = replaceDisabled;
		mFindReplaceToggle->setVisible( !mReplaceDisabled );
	}
}

bool UIDocFindReplace::findPrevText( TextSearchParams& search ) {
	if ( search.text.empty() )
		search.text = mLastSearch;

	mLastSearch = search.text;
	TextRange range = mDoc->getDocRange();
	TextPosition from = mDoc->getSelection( true ).start();
	if ( search.range.isValid() ) {
		range = mDoc->sanitizeRange( search.range ).normalized();
		from = from < range.start() ? range.start() : from;
	}

	String txt( search.text );
	if ( search.escapeSequences )
		txt.unescape();

	TextRange found = mDoc->findLast( txt, from, search.caseSensitive, search.wholeWord,
									  search.type, search.range )
						  .result;
	if ( found.isValid() ) {
		mDoc->setSelection( found );
		mFindInput->removeClass( "error" );
		return true;
	} else {
		found = mDoc->findLast( txt, range.end(), search.caseSensitive, search.wholeWord,
								search.type, range )
					.result;
		if ( found.isValid() ) {
			mDoc->setSelection( found );
			mFindInput->removeClass( "error" );
			return true;
		}
	}
	mFindInput->addClass( "error" );
	return false;
}

bool UIDocFindReplace::findNextText( TextSearchParams& search ) {
	if ( search.text.empty() )
		search.text = mLastSearch;

	mLastSearch = search.text;

	TextRange range = mDoc->getDocRange();
	TextPosition from = mDoc->getSelection( true ).end();
	if ( search.range.isValid() ) {
		range = mDoc->sanitizeRange( search.range ).normalized();
		from = from < range.start() ? range.start() : from;
	}

	String txt( search.text );
	if ( search.escapeSequences )
		txt.unescape();

	TextRange found =
		mDoc->find( txt, from, search.caseSensitive, search.wholeWord, search.type, range ).result;
	if ( found.isValid() ) {
		mDoc->setSelection( found.reversed() );
		mFindInput->removeClass( "error" );
		return true;
	} else {
		found = mDoc->find( txt, range.start(), search.caseSensitive, search.wholeWord, search.type,
							range )
					.result;
		if ( found.isValid() ) {
			mDoc->setSelection( found.reversed() );
			mFindInput->removeClass( "error" );
			return true;
		}
	}
	mFindInput->addClass( "error" );
	return false;
}

int UIDocFindReplace::replaceAll( TextSearchParams& search, const String& replace ) {
	if ( search.text.empty() )
		search.text = mLastSearch;
	if ( search.text.empty() )
		return 0;

	mLastSearch = search.text;
	TextPosition startedPosition = mDoc->getSelection().start();

	String txt( search.text );
	String repl( replace );
	if ( search.escapeSequences ) {
		txt.unescape();
		repl.unescape();
	}

	int count = mDoc->replaceAll( txt, repl, search.caseSensitive, search.wholeWord, search.type,
								  search.range );
	mDoc->setSelection( startedPosition );
	return count;
}

Uint32 UIDocFindReplace::onKeyDown( const KeyEvent& event ) {
	return WidgetCommandExecuter::onKeyDown( event );
}

bool UIDocFindReplace::findAndReplace( TextSearchParams& search, const String& replace ) {
	if ( search.text.empty() )
		search.text = mLastSearch;
	if ( search.text.empty() )
		return false;

	mLastSearch = search.text;

	String txt( search.text );
	String repl( replace );
	if ( search.escapeSequences ) {
		txt.unescape();
		repl.unescape();
	}

	if ( mDoc->hasSelection() && mDoc->getSelectedText() == txt ) {
		replaceSelection( search, repl );
		return true;
	} else {
		return findNextText( search );
	}
}

void UIDocFindReplace::refreshHighlight( UICodeEditor* editor ) {
	mSearchState.text = mFindInput->getText();
	if ( editor )
		editor->setHighlightWord( mSearchState );
	if ( !mSearchState.text.empty() ) {
		mDoc->setSelection( { 0, 0 } );
		if ( !findNextText( mSearchState ) ) {
			mFindInput->addClass( "error" );
		} else {
			mFindInput->removeClass( "error" );
		}
	} else {
		mFindInput->removeClass( "error" );
		mDoc->setSelection( mDoc->getSelection().start() );
	}
};

bool UIDocFindReplace::replaceSelection( TextSearchParams& search, const String& replacement ) {
	UICodeEditor* editor =
		getParent()->isType( UI_TYPE_CODEEDITOR ) ? getParent()->asType<UICodeEditor>() : nullptr;

	if ( !editor || !editor->getDocument().hasSelection() )
		return false;
	editor->getDocument().setActiveClient( editor );
	editor->getDocument().replace( search.text, replacement,
								   editor->getDocument().getSelection().normalized().start(),
								   search.caseSensitive, search.wholeWord, search.type,
								   editor->getDocument().getSelection().normalized() );
	return true;
}

}}} // namespace EE::UI::Tools
