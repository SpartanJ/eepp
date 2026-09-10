#include <eterm/ui/uiterminalfind.hpp>

#include <eepp/scene/actions/actions.hpp>
#include <eepp/ui/tools/uifindbarstyle.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eterm/ui/uiterminal.hpp>

using namespace EE::Scene;
using namespace EE::UI::Tools;
using namespace eterm::Terminal;

namespace eterm { namespace UI {

static constexpr auto SEARCH_DEBOUNCE_TAG = String::hash( "UITerminalFind::search" );

static constexpr char FIND_LAYOUT[] = R"xml(
<hbox class="ce_find_replace_box" layout_width="wrap_content" lh="wc" layout_gravity="right|top" margin_right="32dp">
  <Widget class="expander" layout_width="2dp" layout_height="match_parent" />
  <vbox layout_width="wrap_content" lh="wc">
    <hbox layout_width="340dp" lh="wc" paddingRight="1dp" min-width="200dp" clip="border-box">
      <RelativeLayout layout_width="0dp" layout_weight="1" layout_height="25dp" marginRight="1dp">
        <TextInput class="input-find" layout_width="match_parent" lh="wc" hint="@string(find, Find)" />
        <hbox lw="wc" lh="wc" layout_gravity="right|center_vertical">
          <TextView class="status" text="0/0" lw="wc" min-width="32dp" lh="wc" layout_gravity="right|center_vertical"
            gravity="right" marginRight="2dp" font-family="monospace" />
          <selectbutton id="terminal-find-luapattern" class="luapattern" lw="wc" lh="wc"
            layout_gravity="right|center_vertical" tooltip="@string(lua_pattern_match, Lua Pattern Match)" marginRight="2dp" />
          <selectbutton id="terminal-find-regex" class="regex" lw="wc" lh="wc"
            layout_gravity="right|center_vertical" tooltip="@string(regex_match, Regular Expression Match)" marginRight="2dp" />
          <selectbutton id="terminal-find-whole-word" class="whole-word" lw="wc" lh="wc"
            layout_gravity="right|center_vertical" tooltip="@string(whole_word, Whole Word)" marginRight="2dp" />
          <selectbutton id="terminal-find-match-case" class="match-case" lw="wc" lh="wc"
            layout_gravity="right|center_vertical" tooltip="@string(match_case, Match Case)" marginRight="2dp" />
        </hbox>
      </RelativeLayout>
      <PushButton class="prev-button" lw="wc" layout_height="24dp" text="@string(prev, Prev.)"
        icon="icon(arrow-up, 16dp)" />
      <PushButton class="next-button" lw="wc" layout_height="24dp" text="@string(next, Next)"
        icon="icon(arrow-down, 16dp)" />
      <PushButton class="exit-button" lw="wc" layout_height="24dp" text="@string(close, Close)"
        icon="icon(cancel, 16dp)" />
    </hbox>
  </vbox>
</hbox>
)xml";

UITerminalFind* UITerminalFind::New( UITerminal* terminal ) {
	return eeNew( UITerminalFind, ( terminal ) );
}

UITerminalFind::UITerminalFind( UITerminal* terminal ) :
	UILinearLayout( "terminalfind", UIOrientation::Horizontal ), mTerminal( terminal ) {
	mFlags |= UI_OWNS_CHILDREN_POSITION;
	UIFindBarStyle::ensure( terminal->getUISceneNode() );
	terminal->getUISceneNode()->loadLayoutFromMemory( FIND_LAYOUT, sizeof( FIND_LAYOUT ), this );
	setParent( terminal );
	mInput = querySelector<UITextInput>( ".ce_find_replace_box .input-find" );
	mMatchCase = querySelector<UISelectButton>( ".ce_find_replace_box .match-case" );
	mWholeWord = querySelector<UISelectButton>( ".ce_find_replace_box .whole-word" );
	mRegEx = querySelector<UISelectButton>( ".ce_find_replace_box .regex" );
	mLuaPattern = querySelector<UISelectButton>( ".ce_find_replace_box .luapattern" );
	mStatus = querySelector<UITextView>( ".ce_find_replace_box .status" );
	mInput->on( Event::OnTextChanged, [this]( const Event* ) { updateQuery(); } );
	mInput->on( Event::OnPressEnter, [this]( const Event* ) { navigateSearch( 1 ); } );
	mMatchCase->on( Event::OnValueChange, [this]( const Event* ) { updateQuery(); } );
	mWholeWord->on( Event::OnValueChange, [this]( const Event* ) { updateQuery(); } );
	mRegEx->on( Event::OnValueChange, [this]( const Event* ) {
		if ( mChangingPattern )
			return;
		mChangingPattern = true;
		mLuaPattern->setSelected( false );
		mChangingPattern = false;
		updateQuery();
	} );
	mLuaPattern->on( Event::OnValueChange, [this]( const Event* ) {
		if ( mChangingPattern )
			return;
		mChangingPattern = true;
		mRegEx->setSelected( false );
		mChangingPattern = false;
		updateQuery();
	} );
	querySelector( ".ce_find_replace_box .prev-button" )
		->on( Event::MouseClick, [this]( const Event* event ) {
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK )
				navigateSearch( -1 );
		} );
	querySelector( ".ce_find_replace_box .next-button" )
		->on( Event::MouseClick, [this]( const Event* event ) {
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK )
				navigateSearch( 1 );
		} );
	querySelector( ".ce_find_replace_box .exit-button" )
		->on( Event::MouseClick, [this]( const Event* event ) {
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK )
				hide();
		} );
	setVisible( false );
	runOnMainThread( [this] { mReady = true; } );
}

void UITerminalFind::show() {
	if ( !mReady ) {
		runOnMainThread( [this] { show(); } );
		return;
	}
	if ( !isVisible() ) {
		setVisible( true );
		const Float startX = eemax( 0.f, mTerminal->getSize().getWidth() - getSize().getWidth() );
		setPosition( startX, -getSize().getHeight() );
		runAction( Actions::Move::New( { startX, getPosition().y }, { startX, 0 }, Seconds( 0.2f ),
									   Ease::QuadraticIn ) );
	}
	mInput->getDocument().selectAll();
	mInput->setFocus();
	if ( !mInput->getText().empty() )
		updateQuery();
}

void UITerminalFind::hide() {
	mInput->removeActionsByTag( SEARCH_DEBOUNCE_TAG );
	mQueryPending = false;
	runAction( Actions::Sequence::New(
		Actions::Move::New( getPosition(), { getPosition().x, -getSize().getHeight() },
							Seconds( 0.2f ), Ease::QuadraticOut ),
		Actions::Visible::New( false ) ) );
	mTerminal->getTerm()->clearSearch();
	mTerminal->setFocus();
}

void UITerminalFind::updateQuery() {
	mInput->removeActionsByTag( SEARCH_DEBOUNCE_TAG );
	if ( mInput->getText().size() < TerminalSearch::MinimumQueryLength ) {
		submitQuery();
		return;
	}
	mQueryPending = true;
	mInput->debounce( [this] { submitQuery(); }, Milliseconds( 150 ), SEARCH_DEBOUNCE_TAG );
}

void UITerminalFind::submitQuery() {
	mInput->removeActionsByTag( SEARCH_DEBOUNCE_TAG );
	mQueryPending = false;
	TerminalSearchType type = TerminalSearchType::Normal;
	if ( mRegEx->isSelected() )
		type = TerminalSearchType::RegEx;
	else if ( mLuaPattern->isSelected() )
		type = TerminalSearchType::LuaPattern;
	mTerminal->getTerm()->setSearchQuery( { mInput->getText(), ++mRequestId,
											mMatchCase->isSelected(), mWholeWord->isSelected(),
											type } );
}

void UITerminalFind::navigateSearch( int direction ) {
	if ( mQueryPending ) {
		submitQuery();
		return;
	}
	mTerminal->getTerm()->navigateSearch( direction );
}

void UITerminalFind::refreshStatus() {
	if ( mTerminal->getTerm()->getSearchRequestId() != mRequestId )
		return;
	const Uint32 count = mTerminal->getTerm()->getSearchMatchCount();
	const Int32 current = mTerminal->getTerm()->getCurrentSearchMatch();
	mStatus->setText( String::format( "%d/%u", current >= 0 ? current + 1 : 0, count ) );
	if ( count == 0 && !mInput->getText().empty() )
		mInput->addClass( "error" );
	else
		mInput->removeClass( "error" );
}

Uint32 UITerminalFind::onKeyDown( const KeyEvent& event ) {
	if ( event.getKeyCode() == KEY_ESCAPE ) {
		hide();
		return 1;
	}
	if ( event.getKeyCode() == KEY_RETURN || event.getKeyCode() == KEY_KP_ENTER ) {
		navigateSearch( event.getMod() & KEYMOD_SHIFT ? -1 : 1 );
		return 1;
	}
	const std::string command =
		mTerminal->getKeyBindings().getCommandFromKeyBind( { event.getKeyCode(), event.getMod() } );
	if ( command == "terminal-find-next" )
		navigateSearch( 1 );
	else if ( command == "terminal-find-previous" )
		navigateSearch( -1 );
	else if ( command == "terminal-find-close" )
		hide();
	else if ( command == "terminal-find" )
		show();
	return 1;
}

Uint32 UITerminalFind::onKeyUp( const KeyEvent& ) {
	return 1;
}

Uint32 UITerminalFind::onTextInput( const TextInputEvent& ) {
	return 1;
}

Uint32 UITerminalFind::onTextEditing( const TextEditingEvent& ) {
	return 1;
}

}} // namespace eterm::UI
