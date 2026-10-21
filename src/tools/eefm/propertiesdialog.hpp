#ifndef EEFM_PROPERTIESDIALOG_HPP
#define EEFM_PROPERTIESDIALOG_HPP

#include <eepp/core/cancellationtoken.hpp>
#include <eepp/vfs/directorystats.hpp>
#include <memory>
#include <vector>

namespace EE { namespace UI {
class UIMessageBox;
class UITextView;
class UIPushButton;
}} // namespace EE::UI

namespace eefm {

class App;
struct FileTab;

/** Properties window and its cancellable recursive measurement. The window owns this controller;
 * worker deliveries only keep weak references, so closing it releases all UI state. */
class PropertiesDialog : public std::enable_shared_from_this<PropertiesDialog> {
  public:
	static std::shared_ptr<PropertiesDialog> show( App& app, FileTab* tab );

	explicit PropertiesDialog( App& app );

	~PropertiesDialog();

	void cancel();

  private:
	void populate( FileTab* tab );

	void calculate();

	void updateTotals();

	App& mApp;
	std::vector<EE::URI> mRoots;
	EE::CancellationSource mCancel;
	EE::VFS::DirectoryStats mStats;
	EE::UI::UIMessageBox* mBox{ nullptr };
	EE::UI::UITextView* mTotals{ nullptr };
	EE::UI::UIPushButton* mCalculate{ nullptr };
	EE::Uint64 mGeneration{ 0 };
	bool mRunning{ false };
	bool mStopped{ false };
	bool mCalculated{ false };
};

} // namespace eefm

#endif
