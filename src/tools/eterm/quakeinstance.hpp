#ifndef ETERM_QUAKEINSTANCE_HPP
#define ETERM_QUAKEINSTANCE_HPP

#include <atomic>
#include <eepp/system/filelock.hpp>
#include <eepp/system/ipc.hpp>

namespace eterm {

class QuakeInstance {
  public:
	enum class Start { Owner, Forwarded, Failed };

	Start start();

	void windowReady();

	bool takeToggleRequest();

	void stopListening();

  private:
	std::atomic<bool> mToggleRequested{ false };
	std::atomic<bool> mWindowReady{ false };
	EE::System::FileLock mLock;
	EE::System::IPC mIPC;
};

} // namespace eterm

#endif
