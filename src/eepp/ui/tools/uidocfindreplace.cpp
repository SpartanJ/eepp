#include <eepp/scene/actions/actions.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/tools/uidocfindreplace.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/window.hpp>

namespace EE { namespace UI { namespace Tools {

const char* DOC_FIND_REPLACE_CSS_MARKER = "ce_find_replace_box_marker";

const char DOC_FIND_REPLACE_CSS[] = R"css(
.ce_find_replace_box {
	background-color: var(--list-back);
}
.ce_find_replace_box > .find_replace_toggle,
.ce_find_replace_box pushbutton {
	background-color: var(--list-back);
	border-width: 1dp;
	border-radius: 4dp;
	border-color: transparent;
	tint: var(--font);
	text-as-fallback: true;
}
.ce_find_replace_box pushbutton:hover {
	background-color: var(--list-back);
	border-color: var(--primary);
}
.ce_find_replace_box > .find_replace_toggle:hover {
	background-color: var(--list-back);
	tint: var(--primary);
}
.ce_find_replace_box > linearlayout {
	padding-top: 2dp;
	padding-bottom: 2dp;
	padding-right: 2dp;
}
.ce_find_replace_box > .expander {
	background-color: var(--back);
}
.ce_find_replace_box .replace_box {
	visible: false;
	margin-top: 2dp;
}
.ce_find_replace_box .replace_box {
	visible: false;
}
.ce_find_replace_box .replace_box.enabled {
	visible: true;
}
.ce_find_replace_box > .find_replace_toggle {
	icon: icon(arrow-right-s, 16dp);
	padding: 2dp;
}
.ce_find_replace_box > .find_replace_toggle.enabled {
	icon: icon(arrow-down-s, 16dp);
}
.ce_find_replace_box .input-find {
	padding-right: 88dp;
	clip: padding-box;
}
.ce_find_replace_box selectbutton {
	background-color: transparent;
	border-color: transparent;
	padding: 2dp;
	select-on-click: true;
	transition: tint 0.2s;
	border-radius: 2dp;
}
.ce_find_replace_box selectbutton:hover {
	border-color: transparent;
	tint: var(--primary);
	color: var(--primary);
}
.ce_find_replace_box selectbutton:selected {
	border-color: var(--primary);
	background-color: rgba(var(--primary), 0.25);
	color: var(--font);
}
.ce_find_replace_box selectbutton.match-case {
	icon: url("data:image/svg,<svg fill='#ffffff' width='16' height='16' viewBox='0 0 16 16'><path d='M8.85352 11.7021H7.85449L7.03809 9.54297H3.77246L3.00439 11.7021H2L4.9541 4H5.88867L8.85352 11.7021ZM6.74268 8.73193L5.53418 5.4502C5.49479 5.34277 5.4554 5.1709 5.41602 4.93457H5.39453C5.35872 5.15299 5.31755 5.32487 5.271 5.4502L4.07324 8.73193H6.74268Z'/><path d='M13.756 11.7021H12.8752V10.8428H12.8537C12.4706 11.5016 11.9066 11.8311 11.1618 11.8311C10.6139 11.8311 10.1843 11.686 9.87273 11.396C9.56479 11.106 9.41082 10.721 9.41082 10.2412C9.41082 9.21354 10.016 8.61556 11.2262 8.44727L12.8752 8.21631C12.8752 7.28174 12.4974 6.81445 11.7419 6.81445C11.0794 6.81445 10.4815 7.04004 9.94793 7.49121V6.58887C10.4886 6.24512 11.1117 6.07324 11.8171 6.07324C13.1097 6.07324 13.756 6.75716 13.756 8.125V11.7021ZM12.8752 8.91992L11.5485 9.10254C11.1403 9.15983 10.8324 9.26188 10.6247 9.40869C10.417 9.55192 10.3132 9.80794 10.3132 10.1768C10.3132 10.4453 10.4081 10.6655 10.5978 10.8374C10.7912 11.0057 11.0472 11.0898 11.3659 11.0898C11.8027 11.0898 12.1626 10.9377 12.4455 10.6333C12.7319 10.3254 12.8752 9.93685 12.8752 9.46777V8.91992Z'/></svg>");
}
.ce_find_replace_box selectbutton.luapattern {
	icon: url("data:image/svg,<svg width='16' height='16' fill='none'><path d='m 4.8848115,11.097205 a 3.7631475,3.7631475 0 0 0 5.3533025,0 3.840684,3.840684 0 0 0 0,-5.3992983 3.7631475,3.7631475 0 0 0 -5.3533025,0 3.840684,3.840684 0 0 0 0,5.3992983 z m -0.7300257,0.73594 c -1.8819023,-1.8970156 -1.8819023,-4.9741625 0,-6.8711778 a 4.7895202,4.7895202 0 0 1 6.8133542,0 c 1.881245,1.8976724 1.881245,4.9741622 0,6.8711778 a 4.7895202,4.7895202 0 0 1 -6.8140113,0 z M 12.954519,4.5204035 a 1.0263727,1.0263727 0 0 1 -1.460051,0 1.0473995,1.0473995 0 0 1 0,-1.4718788 1.0263727,1.0263727 0 0 1 1.460051,0 1.0473995,1.0473995 0 0 1 0,1.4718788 z' fill='#ffffff' style='stroke-width:0.657089' /><path d='m 10.525919,8.4419094 a 2.0527454,2.0527454 0 0 1 -2.9201028,0 2.0947991,2.0947991 0 0 1 0,-2.9444148 2.0527454,2.0527454 0 0 1 2.9201028,0 2.0947991,2.0947991 0 0 1 0,2.9444148 z M 8.3351847,7.70597 a 1.0263727,1.0263727 0 0 0 1.4600513,0 1.0473995,1.0473995 0 0 0 0,-1.4718789 1.0263727,1.0263727 0 0 0 -1.4600513,0 1.0473995,1.0473995 0 0 0 0,1.4718789 z' fill='#ffffff' style='stroke-width:0.657089' /></svg>");
}
.ce_find_replace_box selectbutton.regex {
	icon: url("data:image/svg,<svg fill='#ffffff' width='16' height='16' viewBox='0 0 16 16'><path fill-rule='evenodd' clip-rule='evenodd' d='M10.012 2h.976v3.113l2.56-1.557.486.885L11.47 6l2.564 1.559-.485.885-2.561-1.557V10h-.976V6.887l-2.56 1.557-.486-.885L9.53 6 6.966 4.441l.485-.885 2.561 1.557V2zM2 10h4v4H2v-4z'/></svg>");
}
.ce_find_replace_box selectbutton.whole-word {
	icon: url("data:image/svg,<svg fill='#ffffff' width='16' height='16' viewBox='0 0 16 16'><path fill-rule='evenodd' clip-rule='evenodd' d='M0 11H1V13H15V11H16V14H15H1H0V11Z'/><path d='M6.84048 11H5.95963V10.1406H5.93814C5.555 10.7995 4.99104 11.1289 4.24625 11.1289C3.69839 11.1289 3.26871 10.9839 2.95718 10.6938C2.64924 10.4038 2.49527 10.0189 2.49527 9.53906C2.49527 8.51139 3.10041 7.91341 4.3107 7.74512L5.95963 7.51416C5.95963 6.57959 5.58186 6.1123 4.82632 6.1123C4.16389 6.1123 3.56591 6.33789 3.03238 6.78906V5.88672C3.57307 5.54297 4.19612 5.37109 4.90152 5.37109C6.19416 5.37109 6.84048 6.05501 6.84048 7.42285V11ZM5.95963 8.21777L4.63297 8.40039C4.22476 8.45768 3.91682 8.55973 3.70914 8.70654C3.50145 8.84977 3.39761 9.10579 3.39761 9.47461C3.39761 9.74316 3.4925 9.96338 3.68228 10.1353C3.87564 10.3035 4.13166 10.3877 4.45035 10.3877C4.8872 10.3877 5.24706 10.2355 5.52994 9.93115C5.8164 9.62321 5.95963 9.2347 5.95963 8.76562V8.21777Z'/><path d='M9.3475 10.2051H9.32601V11H8.44515V2.85742H9.32601V6.4668H9.3475C9.78076 5.73633 10.4146 5.37109 11.2489 5.37109C11.9543 5.37109 12.5057 5.61816 12.9032 6.1123C13.3042 6.60286 13.5047 7.26172 13.5047 8.08887C13.5047 9.00911 13.2809 9.74674 12.8333 10.3018C12.3857 10.8532 11.7734 11.1289 10.9964 11.1289C10.2695 11.1289 9.71989 10.821 9.3475 10.2051ZM9.32601 7.98682V8.75488C9.32601 9.20964 9.47282 9.59635 9.76644 9.91504C10.0636 10.2301 10.4396 10.3877 10.8944 10.3877C11.4279 10.3877 11.8451 10.1836 12.1458 9.77539C12.4502 9.36719 12.6024 8.79964 12.6024 8.07275C12.6024 7.46045 12.4609 6.98063 12.1781 6.6333C11.8952 6.28597 11.512 6.1123 11.0286 6.1123C10.5166 6.1123 10.1048 6.29134 9.7933 6.64941C9.48177 7.00391 9.32601 7.44971 9.32601 7.98682Z'/></svg>");
}
.ce_find_replace_box selectbutton.escape-sequences {
	font-style: bold;
	min-width: 20dp;
}
.ce_find_replace_box pushbutton.replace-button {
	icon: url("data:image/svg,<svg width='16' height='16' viewBox='0 0 16 16' fill='#ffffff'><path fill-rule='evenodd' clip-rule='evenodd' d='M3.221 3.739l2.261 2.269L7.7 3.784l-.7-.7-1.012 1.007-.008-1.6a.523.523 0 0 1 .5-.526H8V1H6.48A1.482 1.482 0 0 0 5 2.489V4.1L3.927 3.033l-.706.706zm6.67 1.794h.01c.183.311.451.467.806.467.393 0 .706-.168.94-.503.236-.335.353-.78.353-1.333 0-.511-.1-.913-.301-1.207-.201-.295-.488-.442-.86-.442-.405 0-.718.194-.938.581h-.01V1H9v4.919h.89v-.386zm-.015-1.061v-.34c0-.248.058-.448.175-.601a.54.54 0 0 1 .445-.23.49.49 0 0 1 .436.233c.104.154.155.368.155.643 0 .33-.056.587-.169.768a.524.524 0 0 1-.47.27.495.495 0 0 1-.411-.211.853.853 0 0 1-.16-.532zM9 12.769c-.256.154-.625.231-1.108.231-.563 0-1.02-.178-1.369-.533-.349-.355-.523-.813-.523-1.374 0-.648.186-1.158.56-1.53.374-.376.875-.563 1.5-.563.433 0 .746.06.94.179v.998a1.26 1.26 0 0 0-.792-.276c-.325 0-.583.1-.774.298-.19.196-.283.468-.283.816 0 .338.09.603.272.797.182.191.431.287.749.287.282 0 .558-.092.828-.276v.946zM4 7L3 8v6l1 1h7l1-1V8l-1-1H4zm0 1h7v6H4V8z'/></svg>");
}
.ce_find_replace_box pushbutton.replace-all-button {
	icon: url("data:image/svg,<svg width='16' height='16' viewBox='0 0 16 16' fill='#ffffff'><path fill-rule='evenodd' clip-rule='evenodd' d='M11.6 2.677c.147-.31.356-.465.626-.465.248 0 .44.118.573.353.134.236.201.557.201.966 0 .443-.078.798-.235 1.067-.156.268-.365.402-.627.402-.237 0-.416-.125-.537-.374h-.008v.31H11V1h.593v1.677h.008zm-.016 1.1a.78.78 0 0 0 .107.426c.071.113.163.169.274.169.136 0 .24-.072.314-.216.075-.145.113-.35.113-.615 0-.22-.035-.39-.104-.514-.067-.124-.164-.187-.29-.187-.12 0-.219.062-.297.185a.886.886 0 0 0-.117.48v.272zM4.12 7.695L2 5.568l.662-.662 1.006 1v-1.51A1.39 1.39 0 0 1 5.055 3H7.4v.905H5.055a.49.49 0 0 0-.468.493l.007 1.5.949-.944.656.656-2.08 2.085zM9.356 4.93H10V3.22C10 2.408 9.685 2 9.056 2c-.135 0-.285.024-.45.073a1.444 1.444 0 0 0-.388.167v.665c.237-.203.487-.304.75-.304.261 0 .392.156.392.469l-.6.103c-.506.086-.76.406-.76.961 0 .263.061.473.183.631A.61.61 0 0 0 8.69 5c.29 0 .509-.16.657-.48h.009v.41zm.004-1.355v.193a.75.75 0 0 1-.12.436.368.368 0 0 1-.313.17.276.276 0 0 1-.22-.095.38.38 0 0 1-.08-.248c0-.222.11-.351.332-.389l.4-.067zM7 12.93h-.644v-.41h-.009c-.148.32-.367.48-.657.48a.61.61 0 0 1-.507-.235c-.122-.158-.183-.368-.183-.63 0-.556.254-.876.76-.962l.6-.103c0-.313-.13-.47-.392-.47-.263 0-.513.102-.75.305v-.665c.095-.063.224-.119.388-.167.165-.049.315-.073.45-.073.63 0 .944.407.944 1.22v1.71zm-.64-1.162v-.193l-.4.068c-.222.037-.333.166-.333.388 0 .1.027.183.08.248a.276.276 0 0 0 .22.095.368.368 0 0 0 .312-.17c.08-.116.12-.26.12-.436zM9.262 13c.321 0 .568-.058.738-.173v-.71a.9.9 0 0 1-.552.207.619.619 0 0 1-.5-.215c-.12-.145-.181-.345-.181-.598 0-.26.063-.464.189-.612a.644.644 0 0 1 .516-.223c.194 0 .37.069.528.207v-.749c-.129-.09-.338-.134-.626-.134-.417 0-.751.14-1.001.422-.249.28-.373.662-.373 1.148 0 .42.116.764.349 1.03.232.267.537.4.913.4zM2 9l1-1h9l1 1v5l-1 1H3l-1-1V9zm1 0v5h9V9H3zm3-2l1-1h7l1 1v5l-1 1V7H6z'/></svg>");
}
.ce_find_replace_box .input-find.error,
.ce_find_replace_box .input-replace.error {
	border-color: #ff4040;
}
)css";

const char DOC_FIND_REPLACE_XML[] = R"xml(
<hbox class="ce_find_replace_box" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="right|top" layout-margin-right="32dp">
	<Widget class="expander" layout_width="2dp" layout_height="match_parent" />
	<button class="find_replace_toggle" layout_width="16dp" layout_height="wrap_content" padding="2dp" layout_gravity="center" />
	<vbox layout_width="wrap_content" layout_height="wrap_content">
		<hbox layout_width="340dp" layout_height="wrap_content" paddingRight="1dp" min-width="200dp" clip="border-box">
			<RelativeLayout layout_width="0dp" layout_weight="1" layout_height="25dp" marginRight="1dp">
				<input class="input-find" layout_width="match_parent" layout_height="wrap_content" hint="Find" />
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
			<button class="prev-button" layout_width="wrap_content" layout_height="24dp" text="Prev." icon="icon(arrow-up, 16dp)" />
			<button class="next-button" layout_width="wrap_content" layout_height="24dp" text="Next" icon="icon(arrow-down, 16dp)" />
			<button class="exit-button" layout_width="wrap_content" layout_height="24dp" text="Close" icon="icon(cancel, 16dp)" />
		</hbox>
		<hbox class="replace_box" layout_width="match_parent" layout_height="wrap_content">
			<input class="input-replace" layout_width="200dp" layout_height="wrap_content" hint="Replace" marginRight="1dp" />

			<button class="replace-button" layout_width="wrap_content" layout_height="wrap_content" tooltip="@string(replace, Replace)" />

			<button class="replace-all-button" layout_width="wrap_content" layout_height="wrap_content" tooltip="@string(replace_all, Replace All)" />
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

	if ( !parent->getUISceneNode()->getStyleSheet().markerExists(
			 String::hash( DOC_FIND_REPLACE_CSS_MARKER ) ) ) {
		CSS::StyleSheetParser parser;
		parser.loadFromMemory( (const Uint8*)DOC_FIND_REPLACE_CSS,
							   eeARRAY_SIZE( DOC_FIND_REPLACE_CSS ) );
		parent->getUISceneNode()->getStyleSheet().combineStyleSheet( parser.getStyleSheet() );
	}

	parent->getUISceneNode()->loadLayoutFromMemory( DOC_FIND_REPLACE_XML,
													eeARRAY_SIZE( DOC_FIND_REPLACE_XML ), this );

	setParent( parent );

	mFindReplaceToggle = querySelector( ".find_replace_toggle" );
	mReplaceBox = querySelector( ".replace_box" );
	mToggle = querySelector( ".find_replace_toggle" );
	mToggle->addEventListener( Event::MouseClick, [this]( const Event* event ) {
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

	mFindInput->addEventListener( Event::OnTextChanged,
								  [this, editor]( const Event* ) { refreshHighlight( editor ); } );
	mFindInput->addEventListener( Event::OnTextPasted, [this]( const Event* ) {
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
		widget->addEventListener( Event::MouseClick, [this, cmd]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK )
				execute( cmd );
		} );
	};
	auto addReturnListener = [this]( UIWidget* widget, std::string cmd ) {
		widget->addEventListener( Event::OnPressEnter,
								  [this, cmd]( const Event* ) { execute( cmd ); } );
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
	mFindInput->addEventListener( Event::OnTabNavigate, [this]( const Event* ) {
		if ( mReplaceDisabled )
			return;
		if ( !mToggle->hasClass( "enabled" ) ) {
			mToggle->addClass( "enabled" );
			mReplaceBox->addClass( "enabled" );
		}
		mReplaceInput->setFocus();
	} );

	mReplaceInput->addEventListener( Event::OnTabNavigate,
									 [this]( const Event* ) { mFindInput->setFocus(); } );

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

	getParent()->addEventListener( Event::OnSizeChange, [this]( const Event* ) {
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
