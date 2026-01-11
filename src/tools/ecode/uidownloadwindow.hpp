#pragma once
#include <eepp/network/uri.hpp>
#include <eepp/ui/uiwindow.hpp>

using namespace EE;
using namespace EE::Network;
using namespace EE::UI;

namespace ecode {

class UIDownloadWindow : public UIWindow {
  public:
	enum class Status {
		NotStarted,
		Running,
		Completed,
		Canceled,
	};

	static void downloadFileWeb( const std::string& url );

	static UIDownloadWindow* New();

	UIDownloadWindow();

	virtual ~UIDownloadWindow();

	UIDownloadWindow* startDownload( const std::string& url );

  protected:
	Uint64 mReqId{ 0 };
	URI mURI;
	URI mProxyURI;
	Status mStatus{ Status::NotStarted };
};

} // namespace ecode
