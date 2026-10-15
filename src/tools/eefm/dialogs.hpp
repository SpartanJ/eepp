#ifndef EEFM_DIALOGS_HPP
#define EEFM_DIALOGS_HPP

#include <eepp/core/executor.hpp>
#include <eepp/core/task.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/vfs/ssh/sshconfig.hpp>
#ifdef EE_VFS_SFTP
#include <eepp/vfs/sftp/sftpprovider.hpp>
#endif
#include <functional>
#include <mutex>
#include <optional>
#include <string>

namespace eefm {

using namespace EE;
using namespace EE::UI;
using namespace EE::VFS;

/** Modal text prompt. done( nullopt ) when cancelled. UI thread only. */
void promptText( UISceneNode* ui, const String& title, const String& message,
				 const std::string& initial, bool password,
				 std::function<void( std::optional<std::string> )> done );

/** OK / Cancel confirmation. UI thread only. */
void confirm( UISceneNode* ui, const String& title, const String& message,
			  std::function<void( bool )> done );

void showError( UISceneNode* ui, const String& message );

#ifdef EE_VFS_SFTP
/** SSH host key and credential prompts backed by message boxes. The VFS calls them from the SSH
 * I/O thread; dialogs are always created on the UI thread through the executor. */
class SSHPrompts : public SSHHostKeyVerifier, public SSHAuthenticationHandler {
  public:
	SSHPrompts( UISceneNode* ui, Executor& uiExecutor ) : mUI( ui ), mExecutor( &uiExecutor ) {}

	/** Refuses late requests before the executor and scene are destroyed. */
	void shutdown();

	Task<SSHHostKeyDecision> verify( const SSHHostKeyInfo& info ) override;

	Task<std::optional<std::vector<std::string>>>
	request( const SSHAuthenticationRequest& request ) override;

  private:
	UISceneNode* mUI;
	std::mutex mMutex;
	Executor* mExecutor;
};
#endif

} // namespace eefm

#endif
