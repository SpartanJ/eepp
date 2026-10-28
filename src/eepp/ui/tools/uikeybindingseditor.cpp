#include <eepp/ui/tools/uikeybindingseditor.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <deque>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/scene/eventconnection.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/system/md5.hpp>
#include <eepp/ui/models/model.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>

namespace EE::UI::Tools {

class KeybindingsTreeModel : public Models::Model {
  public:
	static constexpr size_t Context = static_cast<size_t>( -1 );

	struct Node {
		std::array<String, 3> cells;
		std::vector<Node*> children;
		Node* parent{ nullptr };
		size_t command{ Context };
		int row{ 0 };
	};

	std::deque<Node> nodes;
	Node root;
	std::array<String, 3> headers;

	size_t rowCount( const Models::ModelIndex& parent = {} ) const {
		if ( parent.isValid() && parent.column() != 0 )
			return 0;
		return node( parent )->children.size();
	}

	size_t columnCount( const Models::ModelIndex& = {} ) const { return headers.size(); }

	std::string columnName( const size_t& column ) const { return headers[column].toUtf8(); }

	Models::ModelIndex index( int row, int column, const Models::ModelIndex& parent = {} ) const {
		auto* owner = node( parent );
		if ( row < 0 || column < 0 || column >= 3 ||
			 static_cast<size_t>( row ) >= owner->children.size() )
			return {};
		return createIndex( row, column, owner->children[row] );
	}

	Models::ModelIndex parentIndex( const Models::ModelIndex& index ) const {
		if ( !index.isValid() )
			return {};
		auto* parent = node( index )->parent;
		return parent && parent != &root ? createIndex( parent->row, 0, parent )
										 : Models::ModelIndex{};
	}

	Variant data( const Models::ModelIndex& index,
				  Models::ModelRole role = Models::ModelRole::Display ) const {
		if ( !index.isValid() )
			return {};
		if ( role == Models::ModelRole::Class && node( index )->command == Context )
			return Variant( "kb_context" );
		return role == Models::ModelRole::Display ? Variant( node( index )->cells[index.column()] )
												  : Variant();
	}

	const Node* node( const Models::ModelIndex& index ) const {
		return index.isValid() ? static_cast<Node*>( index.internalData() ) : &root;
	}

	void rebuild( const KeybindingsConfigData& configuration, const std::string& query ) {
		root.children.clear();
		nodes.clear();
		UnorderedMap<std::string, Node*> contexts;
		for ( size_t i = 0; i < configuration.commands.size(); ++i ) {
			const auto& command = configuration.commands[i];
			std::string text =
				command.context + " " + command.command + " " + command.name.toUtf8();
			for ( const auto& binding : command.bindings )
				text += " " + binding;
			if ( !query.empty() && !String::icontains( text, query ) )
				continue;
			auto& context = contexts[command.context];
			if ( !context ) {
				nodes.emplace_back();
				context = &nodes.back();
				if ( auto named = configuration.contextNames.find( command.context );
					 named != configuration.contextNames.end() ) {
					context->cells[0] = named->second;
				} else {
					std::string name = command.context;
					std::replace( name.begin(), name.end(), '_', ' ' );
					if ( !name.empty() )
						name[0] = static_cast<char>(
							std::toupper( static_cast<unsigned char>( name[0] ) ) );
					context->cells[0] = name;
				}
				context->parent = &root;
				context->row = static_cast<int>( root.children.size() );
				root.children.push_back( context );
			}
			nodes.emplace_back();
			auto& entry = nodes.back();
			entry.cells = { command.name.empty() ? String( command.command ) : command.name,
							command.bindings.empty() ? String() : String( command.bindings[0] ),
							command.bindings.size() < 2 ? String()
														: String( command.bindings[1] ) };
			entry.parent = context;
			entry.command = i;
			entry.row = static_cast<int>( context->children.size() );
			context->children.push_back( &entry );
		}
		invalidate();
	}
};

struct UIKeybindingsEditor::Impl {
	KeybindingsConfigData configuration;
	Save save;
	std::function<void()> manualEdit;
	EventConnectionList dialogConnections;
	UISceneNode* scene{ nullptr };
	UIPushButton* manualEditButton;
	UITextView* note;
	UITreeView* tree;
	std::shared_ptr<KeybindingsTreeModel> treeModel;
	UITextInput* search;
	UILinearLayout* details;
	UITextView* status;
	UICheckBox* symbolic;
	UIMessageBox* dialog{ nullptr };
	UISelectButton* recordingButton{ nullptr };
	std::array<UIDropDownList*, 2> modifiers{};
	size_t command{ 0 };
	size_t slot{ 0 };
	Keycode releaseKey{ KEY_UNKNOWN };
	bool capturing{ false };
	bool extras{ false };

	String tr( const std::string& key, const String& fallback ) const {
		return scene->i18n( key, fallback );
	}
};

// Read a keybindings file and validate it strictly. A missing file is an empty configuration.
static std::string readForEditing( const std::string& path,
								   std::span<const KeybindingsConfig::Context> contexts,
								   const Input* input, IniFile& ini ) {
	ini = IniFile( path, false );
	if ( FileSystem::fileExists( path ) && !ini.loadFromFile( path ) )
		return "Could not read keybindings: " + path;
	std::string error;
	KeybindingsConfig::validate( ini, contexts, error, input );
	return error;
}

static UIWidget* newFileErrorView( UIWidget* parent, const std::string& error,
								   std::function<void()> manualEdit ) {
	auto* container = UILinearLayout::NewVertical();
	container->setParent( parent );
	container->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	auto* message = UITextView::New();
	message->setParent( container );
	message->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
	message->setLayoutMargin( Rectf( 0, 0, 0, 8 ) );
	message->setWordWrap( true );
	message->setText( error );
	if ( manualEdit ) {
		auto* edit = UIPushButton::New();
		edit->setParent( container );
		edit->setText(
			parent->getUISceneNode()->i18n( "kb_manual_edit", "Edit keybindings manually…" ) );
		edit->onClick(
			[manualEdit = std::move( manualEdit )]( const MouseEvent* ) { manualEdit(); } );
	}
	return container;
}

UIKeybindingsEditor* UIKeybindingsEditor::New( UIWidget* parent,
											   KeybindingsConfigData configuration, Save save ) {
	return eeNew( UIKeybindingsEditor, ( parent, std::move( configuration ), std::move( save ) ) );
}

UIWidget* UIKeybindingsEditor::NewForFile( UIWidget* parent, FileSource source ) {
	auto* scene = parent->getUISceneNode();
	const auto* input = scene->getWindow()->getInput();
	// Hash before reading, so an edit made while the catalog is built also invalidates it.
	auto expected = MD5::fromFile( source.path );
	IniFile ini;
	KeybindingsConfigData configuration;
	auto error = readForEditing( source.path, source.contexts, input, ini );
	if ( error.empty() ) {
		KeybindingsConfig::readModifiers( ini, configuration.modifier,
										  configuration.secondaryModifier, error );
	}
	if ( !error.empty() )
		return newFileErrorView( parent, error, std::move( source.manualEdit ) );
	if ( source.catalog ) {
		KeybindingsConfig::Catalog catalog( ini, input, configuration );
		source.catalog( catalog );
	}
	auto save = [scene, input, path = source.path, contexts = source.contexts,
				 apply = std::move( source.apply ), validate = std::move( source.validate ),
				 expected]( const KeybindingsConfigData& proposal ) mutable -> String {
		if ( MD5::fromFile( path ) != expected ) {
			return scene->i18n( "kb_file_changed",
								"The keybindings file changed. Reopen Keyboard Shortcuts before "
								"editing." );
		}
		IniFile updated;
		if ( auto message = readForEditing( path, contexts, input, updated ); !message.empty() )
			return String( message );
		if ( auto message = validate ? validate( proposal ) : String(); !message.empty() )
			return message;
		KeybindingsConfig::writeConfiguration( updated, proposal, contexts );
		if ( std::string message;
			 !KeybindingsConfig::validate( updated, contexts, message, input ) )
			return String( message );
		if ( !updated.writeFileAtomic() )
			return scene->i18n( "kb_save_error", "Could not save keyboard shortcuts." );
		if ( auto message = apply ? apply() : String(); !message.empty() )
			return message;
		expected = MD5::fromFile( path );
		return {};
	};
	auto* editor = New( parent, std::move( configuration ), std::move( save ) );
	editor->setManualEditCallback( std::move( source.manualEdit ) );
	editor->setNote( source.note );
	return editor;
}

UIKeybindingsEditor::UIKeybindingsEditor( UIWidget* parent, KeybindingsConfigData configuration,
										  Save save ) :
	UILinearLayout( "keybindingseditor", UIOrientation::Vertical ),
	mImpl( std::make_unique<Impl>() ) {
	auto& p = *mImpl;
	p.configuration = std::move( configuration );
	p.save = std::move( save );
	setParent( parent );
	p.scene = getUISceneNode();
	setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	setFlags( UI_TAB_STOP );
	auto* layout =
		getUISceneNode()->loadLayoutFromString( R"xml(
<style>
keybindingseditor .kb_search { margin-bottom: 12dp; }
keybindingseditor .kb_modifiers { margin-bottom: 14dp; }
keybindingseditor .kb_modifier { margin-right: 20dp; }
keybindingseditor .kb_modifier_label { margin-bottom: 5dp; }
keybindingseditor .kb_details { padding: 12dp 0dp 0dp 0dp; }
keybindingseditor .kb_context { font-weight: bold; }
keybindingseditor .kb_binding_row { margin-bottom: 8dp; }
keybindingseditor .kb_binding_label { min-width: 76dp; margin-right: 8dp; }
keybindingseditor .kb_binding_button { margin-right: 6dp; }
keybindingseditor .kb_binding_row selectbutton,
keybindingseditor .kb_binding_row pushbutton {
	layout-height: 24dp;
	min-height: 24dp;
	max-height: 24dp;
	padding: 2dp 6dp;
	layout-gravity: center_vertical;
}
keybindingseditor selectbutton.kb_binding_button:selected {
	background-color: var(--tab-hover);
	border-color: var(--primary);
}
keybindingseditor .kb_actions { margin-top: 4dp; }
keybindingseditor .kb_actions pushbutton { margin-right: 8dp; }
keybindingseditor .kb_symbolic { margin-top: 12dp; }
keybindingseditor .kb_status { margin-top: 6dp; font-size: 10dp; }
keybindingseditor .kb_note { margin-top: 8dp; }
</style>
<vbox lw="mp" lh="mp">
	<TextInput id="kb_search" lw="mp" lh="wc" class="kb_search" />
	<hbox lw="mp" lh="wc" class="kb_modifiers">
		<vbox lw="0" lw8="1" lh="wc" class="kb_modifier">
			<TextView id="kb_mod_label" lw="mp" lh="wc" class="kb_modifier_label" />
			<DropDownList id="kb_mod" lw="mp" lh="wc" />
		</vbox>
		<vbox lw="0" lw8="1" lh="wc">
			<TextView id="kb_mod2_label" lw="mp" lh="wc" class="kb_modifier_label" />
			<DropDownList id="kb_mod2" lw="mp" lh="wc" />
		</vbox>
	</hbox>
	<vbox id="kb_table" lw="mp" lh="0" lw8="1" />
	<ScrollView id="kb_details_scroll" lw="mp" lh="0" hscroll-mode="off" focusable="false">
		<vbox id="kb_details" lw="mp" lh="wc" class="kb_details" />
	</ScrollView>
	<CheckBox id="kb_symbolic" lw="mp" lh="wc" class="kb_symbolic" />
	<hbox lw="mp" lh="wc" margin-top="8dp">
		<PushButton id="kb_restore_all" lw="wc" lh="wc" margin-right="8dp" />
		<PushButton id="kb_manual_edit" lw="wc" lh="wc" visible="false" enabled="false" />
	</hbox>
	<TextView id="kb_status" lw="mp" lh="wc" class="kb_status" word-wrap="true" />
	<TextView id="kb_note" lw="mp" lh="wc" class="kb_note" word-wrap="true" visible="false" />
</vbox>
)xml",
												this, String::hash( "UIKeybindingsEditor" ) );
	p.search = layout->find<UITextInput>( "kb_search" );
	p.search->setHint( p.tr( "kb_search", "Search commands or shortcuts" ) );
	p.search->on( Event::OnTextChanged, [this]( const Event* ) { refresh(); } );
	for ( bool secondary : { false, true } ) {
		auto* label = layout->find<UITextView>( secondary ? "kb_mod2_label" : "kb_mod_label" );
		label->setText( secondary ? p.tr( "kb_mod2", "Secondary modifier" )
								  : p.tr( "kb_mod", "Default modifier" ) );
		auto* dropDown = layout->find<UIDropDownList>( secondary ? "kb_mod2" : "kb_mod" );
		p.modifiers[secondary ? 1 : 0] = dropDown;
		dropDown->getListBox()->addListBoxItems( { "ctrl", "meta", "alt", "altgr", "shift" } );
		dropDown->getListBox()->setSelected( secondary ? p.configuration.secondaryModifier
													   : p.configuration.modifier );
		dropDown->on( Event::OnItemSelected, [this, dropDown, secondary]( const Event* ) {
			auto proposal = mImpl->configuration;
			auto value = dropDown->getText().toUtf8();
			if ( value == ( secondary ? proposal.secondaryModifier : proposal.modifier ) )
				return;
			( secondary ? proposal.secondaryModifier : proposal.modifier ) = value;
			if ( KeyMod::getKeyMod( proposal.modifier ) &
				 KeyMod::getKeyMod( proposal.secondaryModifier ) ) {
				dropDown->getListBox()->setSelected( secondary
														 ? mImpl->configuration.secondaryModifier
														 : mImpl->configuration.modifier );
				mImpl->status->setText(
					mImpl->tr( "kb_mod_overlap", "Choose different modifiers." ) );
				return;
			}
			propose( std::move( proposal ) );
			dropDown->getListBox()->setSelected( secondary ? mImpl->configuration.secondaryModifier
														   : mImpl->configuration.modifier );
		} );
	}
	p.tree = UITreeView::New();
	p.treeModel = std::make_shared<KeybindingsTreeModel>();
	p.treeModel->headers = { p.tr( "kb_command", "Command" ), p.tr( "kb_main", "Main" ),
							 p.tr( "kb_alternative", "Alternative" ) };
	p.tree->setModel( p.treeModel );
	p.tree->setHeadersVisible( true );
	p.tree->setColumnWidthMode( UIAbstractTableView::ColumnWidthMode::Percentage );
	p.tree->setColumnsWidthPercentage( { 0.54f, 0.23f, 0.23f } );
	p.tree->setParent( layout->find<UIWidget>( "kb_table" ) );
	p.tree->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	p.details = layout->find<UILinearLayout>( "kb_details" );
	auto* detailsScroll = layout->find<UIScrollView>( "kb_details_scroll" );
	p.details->on( Event::OnSizeChange, [details = p.details, detailsScroll]( const Event* ) {
		detailsScroll->setPixelsSize(
			detailsScroll->getPixelsSize().getWidth(),
			std::min( details->getPixelsSize().getHeight(), PixelDensity::dpToPx( 170 ) ) );
	} );
	p.symbolic = layout->find<UICheckBox>( "kb_symbolic" );
	p.symbolic->setText( p.tr( "kb_symbolic", "Use default modifier when recording" ) );
	p.symbolic->setChecked( true );
	p.status = layout->find<UITextView>( "kb_status" );
	p.note = layout->find<UITextView>( "kb_note" );
	auto* restore = layout->find<UIPushButton>( "kb_restore_all" );
	restore->setText( p.tr( "kb_restore_all", "Restore All Shortcuts…" ) );
	restore->onClick( [this]( const MouseEvent* ) { restoreDefaults(); } );
	p.manualEditButton = layout->find<UIPushButton>( "kb_manual_edit" );
	p.manualEditButton->setText( p.tr( "kb_manual_edit", "Edit keybindings manually…" ) );
	p.manualEditButton->onClick( [this]( const MouseEvent* ) {
		stopRecording();
		if ( mImpl->manualEdit )
			mImpl->manualEdit();
	} );
	p.tree->setOnSelection( [this]( const Models::ModelIndex& selected ) {
		auto& p = *mImpl;
		clearDetails();
		if ( !selected.isValid() )
			return;
		size_t index = p.treeModel->node( selected )->command;
		if ( index == KeybindingsTreeModel::Context )
			return;

		auto& cmd = p.configuration.commands[index];
		for ( size_t slot = 0;
			  slot < ( p.extras ? std::max<size_t>( 2, cmd.bindings.size() + 1 ) : 2 ); ++slot ) {
			auto* row = UILinearLayout::NewHorizontal();
			row->setParent( p.details );
			row->addClass( "kb_binding_row" );
			row->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
			auto* label = UITextView::New();
			label->setParent( row );
			label->addClass( "kb_binding_label" );
			label->setLayoutGravity( UI_VALIGN_CENTER );
			label->setText( slot == 0	? p.tr( "kb_main", "Main" )
							: slot == 1 ? p.tr( "kb_alternative", "Alternative" )
										: p.tr( "kb_extra", "Extra" ) );
			auto* button = UISelectButton::New();
			button->setId( "kb_record_" + std::to_string( slot ) );
			button->setParent( row );
			button->addClass( "kb_binding_button" );
			button->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );
			button->setLayoutWeight( 1 );
			button->setText( slot < cmd.bindings.size() ? String( cmd.bindings[slot] )
														: p.tr( "kb_record", "Record shortcut" ) );
			button->onClick( [this, index, slot]( const MouseEvent* ) { edit( index, slot ); } );
			auto* escape = UIPushButton::New();
			escape->setParent( row );
			escape->addClass( "kb_binding_button" );
			escape->setText( p.tr( "kb_escape", "Escape" ) );
			escape->onClick( [this, index, slot]( const MouseEvent* ) {
				auto proposal = mImpl->configuration;
				auto& bindings = proposal.commands[index].bindings;
				if ( slot < bindings.size() )
					bindings[slot] = "escape";
				else
					bindings.push_back( "escape" );
				proposal.commands[index].unbound = false;
				propose( std::move( proposal ) );
			} );
			auto* clear = UIPushButton::New();
			clear->setId( "kb_clear_" + std::to_string( slot ) );
			clear->setParent( row );
			clear->setText( p.tr( "kb_clear", "Clear" ) );
			clear->onClick( [this, index, slot]( const MouseEvent* ) {
				auto proposal = mImpl->configuration;
				auto& bindings = proposal.commands[index].bindings;
				if ( slot < bindings.size() ) {
					bindings.erase( bindings.begin() + slot );
					proposal.commands[index].unbound = bindings.empty();
					propose( std::move( proposal ) );
				}
			} );
		}
		auto* actions = UILinearLayout::NewHorizontal();
		actions->setParent( p.details );
		actions->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
		actions->addClass( "kb_actions" );
		auto* extras = UIPushButton::New();
		extras->setParent( actions );
		extras->setText( p.tr( "kb_extras", "Show/hide additional shortcuts" ) + " (" +
						 std::to_string( cmd.bindings.size() > 2 ? cmd.bindings.size() - 2 : 0 ) +
						 ")" );
		extras->onClick( [this, selected]( const MouseEvent* ) {
			mImpl->extras = !mImpl->extras;
			mImpl->tree->getOnSelection()( selected );
		} );
		auto* reset = UIPushButton::New();
		reset->setId( "kb_reset_command" );
		reset->setParent( actions );
		reset->setText( p.tr( "kb_reset", "Restore command defaults" ) );
		reset->onClick( [this, index]( const MouseEvent* ) {
			auto proposal = mImpl->configuration;
			proposal.commands[index].bindings = proposal.commands[index].defaults;
			proposal.commands[index].unbound = false;
			propose( std::move( proposal ) );
		} );
	} );
	refresh();
}

UIKeybindingsEditor::~UIKeybindingsEditor() {
	closeDialog( true );
}

void UIKeybindingsEditor::closeDialog( bool immediately ) {
	auto* dialog = mImpl->dialog;
	mImpl->dialog = nullptr;
	// Disconnect only this editor's listeners. The scene owns queued dialog deletion.
	mImpl->dialogConnections.clear();
	if ( !dialog )
		return;
	if ( immediately ) {
		if ( !dialog->isClosing() )
			dialog->close();
	} else {
		dialog->closeWindow();
	}
}

const KeybindingsConfigData& UIKeybindingsEditor::getConfiguration() const {
	return mImpl->configuration;
}

void UIKeybindingsEditor::restoreDefaults() {
	auto& p = *mImpl;
	if ( p.dialog )
		return;
	stopRecording();
	auto proposal = p.configuration;
	for ( auto& command : proposal.commands ) {
		command.bindings = command.defaults;
		command.unbound = false;
	}
	propose( std::move( proposal ), true );
}

void UIKeybindingsEditor::setManualEditCallback( std::function<void()> callback ) {
	mImpl->manualEdit = std::move( callback );
	const bool enabled = static_cast<bool>( mImpl->manualEdit );
	mImpl->manualEditButton->setVisible( enabled );
	mImpl->manualEditButton->setEnabled( enabled );
}

void UIKeybindingsEditor::setNote( const String& note ) {
	mImpl->note->setText( note );
	mImpl->note->setVisible( !note.empty() );
}

void UIKeybindingsEditor::refresh() {
	auto& p = *mImpl;
	const auto selected = p.tree->getSelection().first();
	const auto command =
		selected.isValid() ? p.treeModel->node( selected )->command : KeybindingsTreeModel::Context;
	clearDetails();
	p.tree->getSelection().clear();
	p.treeModel->rebuild( p.configuration, p.search->getText().toUtf8() );
	p.tree->expandAll();
	if ( command == KeybindingsTreeModel::Context )
		return;
	for ( auto& node : p.treeModel->nodes ) {
		if ( node.command == command ) {
			p.tree->getSelection().set(
				p.treeModel->index( node.row, 0, p.treeModel->index( node.parent->row, 0 ) ) );
			break;
		}
	}
}

void UIKeybindingsEditor::clearDetails() {
	stopRecording();
	while ( auto* child = mImpl->details->getFirstChild() ) {
		child->setVisible( false );
		child->setEnabled( false );
		child->setParent( this );
		child->close();
	}
}

void UIKeybindingsEditor::edit( size_t command, size_t slot ) {
	stopRecording();
	mImpl->command = command;
	mImpl->slot = slot;
	setFocus();
	mImpl->capturing = true;
	mImpl->recordingButton =
		mImpl->details->find<UISelectButton>( "kb_record_" + std::to_string( slot ) );
	if ( mImpl->recordingButton ) {
		mImpl->recordingButton->select();
		mImpl->recordingButton->setText(
			mImpl->tr( "kb_recording", "Recording… Press a shortcut" ) );
	}
	mImpl->status->setText( mImpl->tr(
		"kb_capture", "Press a shortcut. Escape cancels; use the Escape button to bind Escape." ) );
}

void UIKeybindingsEditor::stopRecording() {
	auto& p = *mImpl;
	if ( !p.capturing )
		return;
	p.capturing = false;
	if ( p.recordingButton ) {
		p.recordingButton->unselect();
		const auto& bindings = p.configuration.commands[p.command].bindings;
		p.recordingButton->setText( p.slot < bindings.size()
										? String( bindings[p.slot] )
										: p.tr( "kb_record", "Record shortcut" ) );
		p.recordingButton = nullptr;
	}
	p.status->setText( "" );
}

Uint32 UIKeybindingsEditor::onFocusLoss() {
	stopRecording();
	return UILinearLayout::onFocusLoss();
}

Uint32 UIKeybindingsEditor::onKeyDown( const KeyEvent& event ) {
	if ( !mImpl->capturing )
		return event.getKeyCode() == mImpl->releaseKey ? 1 : UILinearLayout::onKeyDown( event );
	mImpl->releaseKey = event.getKeyCode();
	if ( event.getKeyCode() == KEY_ESCAPE ) {
		stopRecording();
		return 1;
	}
	if ( event.getKeyCode() == KEY_UNKNOWN ||
		 ( event.getKeyCode() >= KEY_LCTRL && event.getKeyCode() <= KEY_RGUI ) )
		return 1;
	auto* input = getUISceneNode()->getWindow()->getInput();
	auto shortcut =
		KeyBindings::sanitizeShortcut( { event.getKeyCode(), event.getSanitizedMod() } );
	const bool symbolic = mImpl->symbolic->isChecked();
	const auto expression = KeyBindings::fromShortcut(
		input, shortcut, symbolic ? KeyMod::getKeyMod( mImpl->configuration.modifier ) : 0,
		symbolic ? KeyMod::getKeyMod( mImpl->configuration.secondaryModifier ) : 0 );
	auto proposal = mImpl->configuration;
	auto& bindings = proposal.commands[mImpl->command].bindings;
	if ( mImpl->slot < bindings.size() )
		bindings[mImpl->slot] = expression;
	else
		bindings.push_back( expression );
	proposal.commands[mImpl->command].unbound = false;
	stopRecording();
	propose( std::move( proposal ) );
	return 1;
}

Uint32 UIKeybindingsEditor::onKeyUp( const KeyEvent& event ) {
	if ( mImpl->capturing )
		return 1;
	if ( event.getKeyCode() == mImpl->releaseKey ) {
		mImpl->releaseKey = KEY_UNKNOWN;
		return 1;
	}
	return UILinearLayout::onKeyUp( event );
}

void UIKeybindingsEditor::propose( KeybindingsConfigData configuration, bool confirmRestore ) {
	auto& p = *mImpl;
	if ( p.dialog )
		return;
	auto* input = getUISceneNode()->getWindow()->getInput();
	struct Assignment {
		KeyBindings::Shortcut shortcut;
		size_t command;
		size_t binding;
		bool changed;
	};
	std::vector<Assignment> assignments;
	for ( size_t i = 0; i < configuration.commands.size(); ++i ) {
		const auto& command = configuration.commands[i];
		for ( size_t j = 0; j < command.bindings.size(); ++j ) {
			const auto& expression = command.bindings[j];
			auto shortcut = configuration.resolve( input, expression );
			if ( shortcut.key == KEY_UNKNOWN ) {
				p.status->setText( p.tr( "kb_invalid_shortcut", "Invalid shortcut" ) + ": " +
								   expression );
				return;
			}
			bool changed = std::find( p.configuration.commands[i].bindings.begin(),
									  p.configuration.commands[i].bindings.end(),
									  expression ) == p.configuration.commands[i].bindings.end() ||
						   p.configuration.resolve( input, expression ) != shortcut;
			assignments.push_back( { shortcut, i, j, changed } );
		}
	}
	// Resolve each expression once per configuration. New assignments take precedence only if the
	// user accepts the complete conflict report. Independent contexts have separate indexes.
	UnorderedMap<std::string,
				 UnorderedMap<KeyBindings::Shortcut, size_t, KeyBindings::ShortcutHash>>
		owners;
	std::vector<std::vector<size_t>> removals( configuration.commands.size() );
	String conflicts;
	for ( bool changed : { true, false } ) {
		for ( const auto& assignment : assignments ) {
			if ( assignment.changed != changed )
				continue;
			const auto& command = configuration.commands[assignment.command];
			auto& context = owners[command.context];
			auto [owner, inserted] = context.emplace( assignment.shortcut, assignment.command );
			if ( inserted )
				continue;
			removals[assignment.command].push_back( assignment.binding );
			if ( owner->second != assignment.command ) {
				const auto& winner = configuration.commands[owner->second];
				conflicts +=
					String( command.bindings[assignment.binding] + " : " + command.command + " → " +
							winner.command + " (" + command.context + ")\n" );
			}
		}
	}
	for ( size_t i = 0; i < removals.size(); ++i ) {
		auto& removed = removals[i];
		std::sort( removed.rbegin(), removed.rend() );
		for ( auto binding : removed )
			configuration.commands[i].bindings.erase( configuration.commands[i].bindings.begin() +
													  binding );
		if ( !removed.empty() && configuration.commands[i].bindings.empty() )
			configuration.commands[i].unbound = true;
	}
	if ( conflicts.empty() && !confirmRestore ) {
		commit( std::move( configuration ) );
		return;
	}
	p.dialog =
		UIMessageBox::New( UIMessageBox::OK_CANCEL,
						   ( confirmRestore ? p.tr( "kb_restore_all_confirm",
													"Restore default shortcuts for all commands? "
													"Custom shortcuts will be removed." )
											: p.tr( "kb_conflict", "Reassign these shortcuts?" ) ) +
							   "\n" + conflicts );
	p.dialog->setParent( getUISceneNode()->getRoot() );
	p.dialog->getButtonOK()->setText( confirmRestore
										  ? p.tr( "kb_restore_all_action", "Restore Defaults" )
										  : p.tr( "kb_reassign", "Reassign" ) );
	p.dialog->getButtonCancel()->setText( p.tr( "kb_cancel", "Cancel" ) );
	p.dialogConnections += p.dialog->connect(
		Event::OnConfirm,
		[this, configuration = std::move( configuration )]( const Event* ) mutable {
			closeDialog();
			commit( std::move( configuration ) );
		} );
	p.dialogConnections +=
		p.dialog->connect( Event::OnCancel, [this]( const Event* ) { closeDialog(); } );
	p.dialogConnections +=
		p.dialog->connect( Event::OnDiscard, [this]( const Event* ) { closeDialog(); } );
	p.dialogConnections += p.dialog->connect( Event::OnClose, [this]( const Event* ) {
		mImpl->dialog = nullptr;
		mImpl->dialogConnections.clear();
	} );
	p.dialog->show();
	p.dialog->getButtonCancel()->setFocus();
}

void UIKeybindingsEditor::commit( KeybindingsConfigData configuration ) {
	auto error = mImpl->save ? mImpl->save( configuration ) : String();
	mImpl->status->setText( error );
	if ( !error.empty() )
		return;
	mImpl->configuration = std::move( configuration );
	mImpl->modifiers[0]->getListBox()->setSelected( mImpl->configuration.modifier );
	mImpl->modifiers[1]->getListBox()->setSelected( mImpl->configuration.secondaryModifier );
	refresh();
}

} // namespace EE::UI::Tools
