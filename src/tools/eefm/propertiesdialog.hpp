#ifndef EEFM_PROPERTIESDIALOG_HPP
#define EEFM_PROPERTIESDIALOG_HPP

#include <array>
#include <eepp/core/cancellationtoken.hpp>
#include <eepp/scene/eventconnection.hpp>
#include <eepp/vfs/directorystats.hpp>
#include <eepp/vfs/fileinfo.hpp>
#include <eepp/vfs/permissionchanges.hpp>
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
class UISceneNode;

}} // namespace EE::UI
namespace EE { namespace VFS {

class VFSDirectoryModel;

}} // namespace EE::VFS

namespace eefm {

using namespace EE;
using namespace EE::Scene;
using namespace EE::UI;
using namespace EE::VFS;

class App;
struct FileTab;

/** Dolphin-style properties and cancellable VFS metadata operations. The window owns this
 * controller; asynchronous deliveries borrow it weakly and cannot outlive the window. */
class PropertiesDialog : public std::enable_shared_from_this<PropertiesDialog> {
  public:
	static std::shared_ptr<PropertiesDialog> show( App& app, FileTab* tab );

	/** Whether a recursive measurement of this location can start without being asked for: never
	 * for remote locations or for a whole file system. */
	static bool measuresAutomatically( const URI& root );

	/** A measured size, as "At least ..." when the total is not final. */
	static String sizeText( UISceneNode& ui, Uint64 bytes, bool partial );

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

	void updateExecute( bool syncModes = false );

	PermissionChanges permissionChanges() const;

	void updateActions();

	void apply( bool close );

	void applyRename();

	void applyPermissions();

	void finishApply( const std::string& error = {} );

	void calculate();

	void stopCalculation();

	void updateTotals();

	App& mApp;
	std::vector<URI> mRoots;
	std::vector<VFSFileInfo> mInfo;
	std::weak_ptr<VFSDirectoryModel> mModel;
	std::string mResultMessage;
	std::string mInitialUser;
	std::string mInitialGroup;
	EventConnection mSceneResize;
	CancellationSource mCancel;
	CancellationSource mOperationCancel;
	DirectoryStats mStats;
	UIWindow* mBox{ nullptr };
	UIWidget* mGeneral{ nullptr };
	UIWidget* mPermissions{ nullptr };
	UITextInput* mName{ nullptr };
	UITextInput* mOctal{ nullptr };
	UITextInput* mUser{ nullptr };
	UITextInput* mGroup{ nullptr };
	UITextView* mStatus{ nullptr };
	UITextView* mTotals{ nullptr };
	UIPushButton* mCalculate{ nullptr };
	UIPushButton* mApply{ nullptr };
	UIPushButton* mOK{ nullptr };
	UICheckBox* mRecursive{ nullptr };
	UICheckBox* mExecute{ nullptr };
	std::array<UIDropDownList*, 6> mModes{};
	std::array<Uint32, 6> mInitialModes{};
	Uint64 mGeneration{ 0 };
	Uint32 mModeMask{ 0 };
	Uint32 mModeValue{ 0 };
	std::optional<Uint32> mNewUid;
	std::optional<Uint32> mNewGid;
	std::optional<bool> mExecutable;
	bool mUpdatingExecute{ false };
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
