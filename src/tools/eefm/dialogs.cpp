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

void promptText( UISceneNode* ui, const String& title, const String& message,
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

void confirm( UISceneNode*, const String& title, const String& message,
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

void showError( UISceneNode* ui, const String& message ) {
	UIMessageBox* box = UIMessageBox::New( UIMessageBox::OK, message );
	box->setTitle( ui->i18n( "eefm_error", "Error" ) );
	box->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	box->showWhenReady();
}

#ifdef EE_VFS_SFTP

void SSHPrompts::shutdown() {
	std::lock_guard<std::mutex> lock( mMutex );
	mExecutor = nullptr;
}

Task<SSHHostKeyDecision> SSHPrompts::verify( const SSHHostKeyInfo& info ) {
	auto source = std::make_shared<TaskSource<SSHHostKeyDecision>>();
	auto task = source->task();
	std::lock_guard<std::mutex> lock( mMutex );
	if ( !mExecutor ) {
		source->resolve( SSHHostKeyDecision::Reject );
		return task;
	}
	UISceneNode* ui = mUI;
	// If the executor drops this (shutdown) the source is abandoned and the key rejected.
	mExecutor->execute( [ui, info, source] {
		String message = String::format(
			ui->i18n( "eefm_unknown_host_key",
					  "The authenticity of host '%s' (port %d) can't be established.\n%s key "
					  "fingerprint is:\n%s\n\nTrust this host and save its key to %s?" )
				.toUtf8(),
			info.host.c_str(), static_cast<int>( info.port ), info.keyType.c_str(),
			info.fingerprint.c_str(), info.knownHostsFile.c_str() );
		confirm( ui, ui->i18n( "eefm_host_key_title", "Unknown Host Key" ), message,
				 [source]( bool accept ) {
					 source->resolve( accept ? SSHHostKeyDecision::AcceptAndSave
											 : SSHHostKeyDecision::Reject );
				 } );
	} );
	return task;
}

Task<std::optional<std::vector<std::string>>>
SSHPrompts::request( const SSHAuthenticationRequest& request ) {
	using Answers = std::optional<std::vector<std::string>>;
	auto source = std::make_shared<TaskSource<Answers>>();
	auto task = source->task();
	std::lock_guard<std::mutex> lock( mMutex );
	if ( !mExecutor ) {
		source->resolve( std::nullopt );
		return task;
	}
	UISceneNode* ui = mUI;
	mExecutor->execute( [ui, request, source] {
		String title = ui->i18n( "eefm_authentication", "Authentication" ) + " - " +
					   String::fromUtf8( request.user + "@" + request.host );
		// Prompts are asked one after another; any cancel aborts the whole request.
		auto answers = std::make_shared<std::vector<std::string>>();
		auto ask = std::make_shared<std::function<void( size_t )>>();
		*ask = [ui, title, request, source, answers, ask]( size_t index ) {
			if ( index >= request.prompts.size() ) {
				source->resolve( Answers( std::move( *answers ) ) );
				*ask = nullptr; // Break the self-reference cycle.
				return;
			}
			std::string text = request.prompts[index].text;
			if ( index == 0 && !request.instruction.empty() )
				text = request.instruction + "\n" + text;
			promptText( ui, title, String::fromUtf8( text ), "", !request.prompts[index].echo,
						[source, answers, ask, index]( std::optional<std::string> answer ) {
							if ( !answer ) {
								source->resolve( std::nullopt );
								*ask = nullptr;
								return;
							}
							answers->push_back( std::move( *answer ) );
							( *ask )( index + 1 );
						} );
		};
		( *ask )( 0 );
	} );
	return task;
}

#endif

} // namespace eefm
