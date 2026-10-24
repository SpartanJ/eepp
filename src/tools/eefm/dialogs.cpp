#include "dialogs.hpp"
#include "location.hpp"
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/vfs/path.hpp>
#include <memory>

namespace eefm {

namespace {

/** Resolves a callback exactly once, whichever of confirm / discard happens first. */
template <typename T> struct OnceCallback {
	std::function<void( T )> fn;
	bool done{ false };

	void operator()( T value ) {
		if ( done )
			return;
		done = true;
		fn( std::move( value ) );
	}
};

} // namespace

void Dialogs::promptText( UISceneNode* ui, const String& title, const String& message,
						  const std::string& initial, bool password,
						  std::function<void( std::optional<std::string> )> done ) {
	UIMessageBox* box = UIMessageBox::New( UIMessageBox::INPUT, message );
	box->setTitle( title );
	if ( password )
		box->getTextInput()->setMode( UITextInput::TextInputMode::Password );
	box->getTextInput()->setText( initial );
	box->getTextInput()->getDocument().selectAll();
	auto once = std::make_shared<OnceCallback<std::optional<std::string>>>();
	once->fn = std::move( done );
	box->on( Event::OnConfirm, [box, once]( const Event* ) {
		( *once )( box->getTextInput()->getText().toUtf8() );
		if ( !box->isClosing() )
			box->closeWindow();
	} );
	box->on( Event::OnDiscard, [once]( const Event* ) { ( *once )( std::nullopt ); } );
	box->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	box->showWhenReady();
	(void)ui;
}

void Dialogs::confirm( UISceneNode*, const String& title, const String& message,
					   std::function<void( bool )> done ) {
	UIMessageBox* box = UIMessageBox::New( UIMessageBox::OK_CANCEL, message );
	box->setTitle( title );
	auto once = std::make_shared<OnceCallback<bool>>();
	once->fn = std::move( done );
	box->on( Event::OnConfirm, [once]( const Event* ) { ( *once )( true ); } );
	box->on( Event::OnDiscard, [once]( const Event* ) { ( *once )( false ); } );
	box->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	box->showWhenReady();
}

void Dialogs::showError( UISceneNode* ui, const String& message ) {
	showMessage( ui->i18n( "eefm_error", "Error" ), message );
}

void Dialogs::showMessage( const String& title, const String& message ) {
	UIMessageBox* box = UIMessageBox::New( UIMessageBox::OK, message );
	box->setTitle( title );
	box->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	box->showWhenReady();
}

UIMessageBox*
Dialogs::transferConflict( UISceneNode* ui, const EE::VFS::TransferConflict& conflict,
						   std::function<void( EE::VFS::ConflictResolution, bool )> done ) {
	using namespace EE::VFS;
	const bool folders =
		conflict.sourceInfo.isDirectory() && conflict.destinationInfo.isDirectory();
	const bool sameType =
		conflict.sourceInfo.isDirectory() == conflict.destinationInfo.isDirectory();
	const auto name = conflict.destination.getLastPathSegment();
	const auto instruction =
		folders ? ui->i18n(
					  "eefm_conflict_merge",
					  "Write into the existing folder? Existing files will be handled separately." )
		: sameType
			? ui->i18n( "eefm_conflict_overwrite_question",
						"Overwrite the destination, rename the incoming file, or skip it?" )
			: ui->i18n(
				  "eefm_conflict_type_mismatch",
				  "The source and destination have different types. Rename or skip this item." );
	String message =
		String::format( ui->i18n( "eefm_conflict_details",
								  "\"%s\" already exists.\n\nSource: %s\nDestination: %s\n\n%s" )
							.toUtf8(),
						name, Location::clipboardText( conflict.source ),
						Location::clipboardText( conflict.destination ), instruction.toUtf8() );
	auto* box = UIMessageBox::New( UIMessageBox::INPUT, message );
	box->getTextBox()->setMaxWidthEq( "70vw" );
	box->getTextBox()->setWordWrap( true );
	box->setId( "transfer_conflict" );
	box->setTitle( ui->i18n( "eefm_file_exists", "File Already Exists" ) );
	auto* input = box->getTextInput();
	input->setId( "conflict_name" );
	input->setText( name );
	input->getDocument().selectAll();
	auto* applyAll = UICheckBox::New();
	applyAll->setId( "conflict_apply_all" );
	applyAll->setText( ui->i18n( "eefm_apply_to_all", "Apply to All" ) );
	applyAll->setTooltipText(
		folders ? ui->i18n( "eefm_conflict_apply_folders",
							"Apply to subsequent folder conflicts in this operation. File "
							"conflicts are handled separately." )
				: ui->i18n( "eefm_conflict_apply_files",
							"Apply to subsequent file conflicts in this operation." ) );
	applyAll->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );
	applyAll->setLayoutMargin( Rectf( 0, 8, 0, 0 ) );
	applyAll->setParent( input->getParent() );
	applyAll->toPosition( 2 );
	using Answer = std::pair<ConflictResolution, bool>;
	auto once = std::make_shared<OnceCallback<Answer>>();
	once->fn = [done = std::move( done )]( Answer answer ) mutable {
		done( std::move( answer.first ), answer.second );
	};
	auto finish = [box, input, applyAll, once]( ConflictAction action ) {
		ConflictResolution decision;
		decision.action = action;
		if ( action == ConflictAction::Rename && !applyAll->isChecked() )
			decision.name = input->getText().toUtf8();
		( *once )( { std::move( decision ), applyAll->isChecked() } );
		if ( !box->isClosing() )
			box->closeWindow();
	};
	auto* overwrite = box->getButtonOK();
	overwrite->setId( "conflict_overwrite" );
	overwrite->setText( folders ? ui->i18n( "eefm_write_into", "Write Into" )
								: ui->i18n( "eefm_overwrite", "Overwrite" ) );
	auto* cancel = box->getButtonCancel();
	cancel->setId( "conflict_cancel" );
	cancel->setText( ui->i18n( "eefm_cancel", "Cancel" ) );
	auto addButton = [box]( const char* id, const String& text ) {
		auto* button = UIPushButton::New();
		button->setId( id );
		button->setText( text );
		button->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );
		button->setLayoutMargin( Rectf( 8, 0, 0, 0 ) );
		button->setParent( box->getButtonOK()->getParent() );
		return button;
	};
	auto* skip = addButton( "conflict_skip", ui->i18n( "eefm_conflict_skip", "Skip" ) );
	skip->on( Event::MouseClick, [finish]( const Event* event ) {
		if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK )
			finish( ConflictAction::Skip );
	} );
	auto* rename = addButton( "conflict_rename", ui->i18n( "eefm_conflict_rename", "Rename" ) );
	rename->on( Event::MouseClick, [finish]( const Event* event ) {
		if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK )
			finish( ConflictAction::Rename );
	} );
	UIPushButton* older = nullptr;
	if ( !folders && sameType && conflict.sourceInfo.modifiedTime() &&
		 conflict.destinationInfo.modifiedTime() ) {
		older = addButton( "conflict_older", ui->i18n( "eefm_conflict_older", "Overwrite Older" ) );
		older->on( Event::MouseClick, [finish]( const Event* event ) {
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK )
				finish( ConflictAction::OverwriteOlder );
		} );
	}
	auto updateButtons = [ui, input, applyAll, overwrite, older, rename, name, sameType,
						  parent = conflict.destination.getParent()]( const Event* ) {
		const auto value = input->getText().toUtf8();
		overwrite->setEnabled( sameType && value == name );
		if ( older )
			older->setEnabled( value == name );
		rename->setText( applyAll->isChecked() ? ui->i18n( "eefm_auto_rename", "Auto Rename" )
											   : ui->i18n( "eefm_conflict_rename", "Rename" ) );
		rename->setEnabled( applyAll->isChecked() ||
							( value != name && VFSPath::validName( value, parent ) ) );
	};
	input->on( Event::OnTextChanged, updateButtons );
	applyAll->on( Event::OnValueChange, updateButtons );
	updateButtons( nullptr );
	box->on( Event::OnConfirm, [finish, overwrite, rename]( const Event* ) {
		// Enter must honor the same filename validation as the buttons.
		if ( overwrite->isEnabled() )
			finish( ConflictAction::Overwrite );
		else if ( rename->isEnabled() )
			finish( ConflictAction::Rename );
	} );
	box->on( Event::OnDiscard, [once]( const Event* ) { ( *once )( {} ); } );
	box->on( Event::OnWindowClose, [once]( const Event* ) { ( *once )( {} ); } );
	box->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	box->showWhenReady();
	return box;
}

} // namespace eefm
