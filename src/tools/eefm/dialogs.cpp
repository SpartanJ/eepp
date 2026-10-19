#include "dialogs.hpp"
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uitextinput.hpp>
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

} // namespace eefm
