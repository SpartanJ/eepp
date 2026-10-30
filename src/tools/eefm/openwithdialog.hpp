#ifndef EEFM_OPENWITHDIALOG_HPP
#define EEFM_OPENWITHDIALOG_HPP

#include <eepp/core/uri.hpp>
#include <eepp/scene/eventconnection.hpp>
#include <eepp/system/applications.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace EE { namespace UI {

class UIWindow;
class UIListView;
class UICheckBox;
class UITextView;
class UIPushButton;

}} // namespace EE::UI

namespace eefm {

using namespace EE;
using namespace EE::Scene;
using namespace EE::System;
using namespace EE::UI;

class App;

/** Application selection and per-user defaults. The window owns the controller; worker
 * continuations borrow it weakly, so closing the chooser discards late UI updates. */
class OpenWithDialog : public std::enable_shared_from_this<OpenWithDialog> {
  public:
	using Open = std::function<void( const std::vector<URI>&, Application )>;
	using Command = std::function<void( const std::vector<URI>& )>;

	static void show( App& app, std::vector<URI> uris, std::vector<Application> applications,
					  Open open, Command command );

	OpenWithDialog( App& app, std::vector<URI> uris, std::vector<Application> applications,
					Open open, Command command );

  private:
	struct FileType {
		std::string description;
		std::string mimeType;
		std::string extension;
		bool canSetDefault{ false };
	};

	void populate();

	void fitWindow();

	void loadFileType();

	void updateActions();

	size_t selectedIndex() const;

	void open();

	void finishOpen();

	void setDefault( bool openAfter );

	App& mApp;
	std::vector<URI> mUris;
	std::vector<Application> mApplications;
	Open mOpen;
	Command mCommand;
	FileType mType;
	EventConnection mSceneResize;
	UIWindow* mBox{ nullptr };
	UIListView* mList{ nullptr };
	UICheckBox* mAlways{ nullptr };
	UITextView* mHint{ nullptr };
	UITextView* mStatus{ nullptr };
	UIPushButton* mOpenButton{ nullptr };
	UIPushButton* mCommandButton{ nullptr };
	UIPushButton* mDefaultButton{ nullptr };
	bool mReady{ false };
	bool mBusy{ false };
};

} // namespace eefm

#endif
