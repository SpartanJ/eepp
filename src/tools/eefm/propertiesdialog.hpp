#ifndef EEFM_PROPERTIESDIALOG_HPP
#define EEFM_PROPERTIESDIALOG_HPP

#include <array>
#include <eepp/core/cancellationtoken.hpp>
#include <eepp/scene/eventconnection.hpp>
#include <eepp/vfs/directorystats.hpp>
#include <eepp/vfs/fileinfo.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace EE { namespace UI {
class UIWindow;
class UIWidget;
class UITextView;
class UITextInput;
class UIPushButton;
class UIDropDownList;
class UICheckBox;
}} // namespace EE::UI
namespace EE { namespace VFS {
class VFSDirectoryModel;
}} // namespace EE::VFS

namespace eefm {

class App;
struct FileTab;

/** Dolphin-style properties and cancellable VFS metadata operations. The window owns this
 * controller; asynchronous deliveries borrow it weakly and cannot outlive the window. */
class PropertiesDialog : public std::enable_shared_from_this<PropertiesDialog> {
  public:
	static std::shared_ptr<PropertiesDialog> show( App& app, FileTab* tab );

	explicit PropertiesDialog( App& app );

	~PropertiesDialog();

	void cancel();

  private:
	void populate( FileTab* tab );

	void fitWindow();

	void updateLayout();

	void setLoading( bool loading );

	void loadNext();

	void updateInfo();

	void updateActions();

	void apply( bool close );

	void applyRename();

	void applyPermissions();

	void finishApply( const std::string& error = {} );

	void calculate();

	void stopCalculation();

	void updateTotals();

	App& mApp;
	std::vector<EE::URI> mRoots;
	std::vector<EE::VFS::VFSFileInfo> mInfo;
	std::weak_ptr<EE::VFS::VFSDirectoryModel> mModel;
	std::string mResultMessage;
	std::string mInitialUser;
	std::string mInitialGroup;
	EE::Scene::EventConnection mSceneResize;
	EE::CancellationSource mCancel;
	EE::CancellationSource mOperationCancel;
	EE::VFS::DirectoryStats mStats;
	EE::UI::UIWindow* mBox{ nullptr };
	EE::UI::UIWidget* mGeneral{ nullptr };
	EE::UI::UIWidget* mPermissions{ nullptr };
	EE::UI::UITextInput* mName{ nullptr };
	EE::UI::UITextInput* mOctal{ nullptr };
	EE::UI::UITextInput* mUser{ nullptr };
	EE::UI::UITextInput* mGroup{ nullptr };
	EE::UI::UITextView* mStatus{ nullptr };
	EE::UI::UITextView* mTotals{ nullptr };
	EE::UI::UIPushButton* mCalculate{ nullptr };
	EE::UI::UIPushButton* mApply{ nullptr };
	EE::UI::UIPushButton* mOK{ nullptr };
	EE::UI::UICheckBox* mRecursive{ nullptr };
	std::array<EE::UI::UIDropDownList*, 6> mModes{};
	std::array<EE::Uint32, 6> mInitialModes{};
	EE::Uint64 mGeneration{ 0 };
	EE::Uint32 mModeMask{ 0 };
	EE::Uint32 mModeValue{ 0 };
	std::optional<EE::Uint32> mNewUid;
	std::optional<EE::Uint32> mNewGid;
	bool mRunning{ false };
	bool mStopped{ false };
	bool mCalculated{ false };
	bool mLoading{ true };
	bool mApplying{ false };
	bool mCloseAfterApply{ false };
	bool mCanPermissions{ false };
	bool mCanOwnership{ false };
	bool mCanRename{ false };
	bool mRecursiveApply{ false };
};

} // namespace eefm

#endif
