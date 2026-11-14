#ifndef EEFM_BULKRENAMEDIALOG_HPP
#define EEFM_BULKRENAMEDIALOG_HPP

#include <eepp/core/cancellationtoken.hpp>
#include <eepp/scene/eventconnection.hpp>
#include <eepp/vfs/batchrename.hpp>
#include <memory>

namespace EE { namespace UI {

class UIWindow;
class UIWidget;
class UITextInput;
class UITextView;
class UIDropDownList;
class UISpinBox;
class UIPushButton;

}} // namespace EE::UI
namespace EE { namespace VFS {

class VFSDirectoryModel;

}} // namespace EE::VFS

namespace eefm {

using namespace EE;
using namespace EE::UI;
using namespace EE::VFS;
using namespace EE::Scene;

class App;
struct FileTab;

/** Dolphin-style multi-selection rename. Widgets own the controller; a running batch retains
 * it until its completed prefix has been delivered, even if the window has been closed. */
class BulkRenameDialog : public std::enable_shared_from_this<BulkRenameDialog> {
  public:
	static std::shared_ptr<BulkRenameDialog> show( App& app, FileTab& tab );

	explicit BulkRenameDialog( App& app );

	~BulkRenameDialog();

	void cancel();

	bool isRunning() const { return mRunning; }

  private:
	void populate( FileTab& tab );

	void fitWindow();

	void changeMode();

	void changePosition();

	String defaultTemplate( BatchRenamePattern::Position position ) const;

	BatchRenamePattern pattern() const;

	void updatePreview();

	void apply();

	void finish( VFSResult<BatchRenameResult>&& result );

	App& mApp;
	URI mDirectory;
	std::vector<BatchRenameItem> mItems;
	std::weak_ptr<VFSDirectoryModel> mModel;
	EventConnection mSceneResize;
	CancellationSource mCancel;
	UIWindow* mBox{ nullptr };
	UIWidget* mForm{ nullptr };
	UIDropDownList* mMode{ nullptr };
	UIDropDownList* mPosition{ nullptr };
	UIDropDownList* mAddPosition{ nullptr };
	UITextInput* mTemplate{ nullptr };
	UITextInput* mSearch{ nullptr };
	UITextInput* mReplacement{ nullptr };
	UITextInput* mText{ nullptr };
	UITextInput* mPreview{ nullptr };
	UISpinBox* mStart{ nullptr };
	UITextView* mStatus{ nullptr };
	UIPushButton* mRename{ nullptr };
	bool mAllowUnnumbered{ false };
	bool mLoading{ true };
	bool mUpdating{ false };
	bool mRunning{ false };
	bool mFinished{ false };
	BatchRenamePattern::Position mLastPosition{ BatchRenamePattern::Position::Replace };
};

} // namespace eefm

#endif
