#include <eepp/config.hpp>

#ifdef EE_VFS_SFTP

#include "sshprompts.hpp"

#include "dialogs.hpp"

namespace eefm {

void SSHPrompts::shutdown() {
	std::lock_guard<std::mutex> lock( mMutex );
	mExecutor = nullptr;
}

Task<SSHHostKeyDecision> SSHPrompts::verify( const SSHHostKeyInfo& info ) {
	TaskResolver<SSHHostKeyDecision> resolve;
	auto task = Task<SSHHostKeyDecision>::create( resolve );
	std::lock_guard<std::mutex> lock( mMutex );
	if ( !mExecutor ) {
		resolve( SSHHostKeyDecision::Reject );
		return task;
	}
	UISceneNode* ui = mUI;
	// If the executor drops this (shutdown) the task is abandoned and the key rejected.
	mExecutor->execute( [ui, info, resolve] {
		String message = String::format(
			ui->i18n( "eefm_unknown_host_key",
					  "The authenticity of host '%s' (port %d) can't be established.\n%s key "
					  "fingerprint is:\n%s\n\nTrust this host and save its key to %s?" )
				.toUtf8(),
			info.host.c_str(), static_cast<int>( info.port ), info.keyType.c_str(),
			info.fingerprint.c_str(), info.knownHostsFile.c_str() );
		Dialogs::confirm( ui, ui->i18n( "eefm_host_key_title", "Unknown Host Key" ), message,
						  [resolve]( bool accept ) {
							  resolve( accept ? SSHHostKeyDecision::AcceptAndSave
											  : SSHHostKeyDecision::Reject );
						  } );
	} );
	return task;
}

Task<std::optional<std::vector<std::string>>>
SSHPrompts::request( const SSHAuthenticationRequest& request ) {
	using Answers = std::optional<std::vector<std::string>>;
	TaskResolver<Answers> resolve;
	auto task = Task<Answers>::create( resolve );
	std::lock_guard<std::mutex> lock( mMutex );
	if ( !mExecutor ) {
		resolve( std::nullopt );
		return task;
	}
	UISceneNode* ui = mUI;
	mExecutor->execute( [ui, request, resolve] {
		String title = ui->i18n( "eefm_authentication", "Authentication" ) + " - " +
					   String::fromUtf8( request.user + "@" + request.host );
		// Prompts are asked one after another; any cancel aborts the whole request.
		auto answers = std::make_shared<std::vector<std::string>>();
		auto ask = std::make_shared<std::function<void( size_t )>>();
		*ask = [ui, title, request, resolve, answers, ask]( size_t index ) {
			if ( index >= request.prompts.size() ) {
				resolve( Answers( std::move( *answers ) ) );
				*ask = nullptr; // Break the self-reference cycle.
				return;
			}
			std::string text = request.prompts[index].text;
			if ( index == 0 && !request.instruction.empty() )
				text = request.instruction + "\n" + text;
			Dialogs::promptText(
				ui, title, String::fromUtf8( text ), "", !request.prompts[index].echo,
				[resolve, answers, ask, index]( std::optional<std::string> answer ) {
					if ( !answer ) {
						resolve( std::nullopt );
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

} // namespace eefm

#endif
