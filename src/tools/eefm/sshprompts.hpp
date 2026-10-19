#ifndef EEFM_SSHPROMPTS_HPP
#define EEFM_SSHPROMPTS_HPP

#include <eepp/config.hpp>

#ifdef EE_VFS_SFTP

#include <eepp/core/executor.hpp>
#include <eepp/core/task.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/vfs/sftp/sftpprovider.hpp>
#include <eepp/vfs/ssh/sshconfig.hpp>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace eefm {

using namespace EE;
using namespace EE::UI;
using namespace EE::VFS;

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

} // namespace eefm

#endif

#endif
