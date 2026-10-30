#ifndef EEFM_OPERATIONSPOPUP_HPP
#define EEFM_OPERATIONSPOPUP_HPP

#include <eepp/core/small_vector.hpp>
#include <eepp/scene/eventconnection.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/vfs/transfer.hpp>
#include <functional>
#include <memory>
#include <optional>

namespace EE::UI {

class UIWindow;
class UITextView;
class UIProgressBar;
class UIPushButton;

} // namespace EE::UI

namespace eefm {

using namespace EE;
using namespace EE::Scene;
using namespace EE::UI;
using namespace EE::VFS;

class OperationsPopup : public std::enable_shared_from_this<OperationsPopup> {
  public:
	static std::shared_ptr<OperationsPopup> show( UISceneNode& scene, UIWidget& anchor, int pane,
												  std::function<void( Uint64 )> cancel );

	OperationsPopup( UISceneNode& scene, UIWidget& anchor, int pane,
					 std::function<void( Uint64 )> cancel );

	bool isOpen() const;

	int pane() const { return mPane; }

	void close();

	/** Creates widgets only for new jobs; existing rows receive coalesced progress updates. */
	void update( Uint64 id, const String& label, const TransferProgress& progress,
				 bool cancelling );

	void remove( Uint64 id );

  private:
	struct Row {
		Uint64 id;
		UIWidget* root;
		UITextView* label;
		UITextView* detail;
		UITextView* status;
		UITextView* percent;
		UIProgressBar* bar;
		UIPushButton* cancel;
		std::string path;
		std::optional<Uint64> total;
		Uint64 done{ ~Uint64( 0 ) };
		int percentage{ -1 };
		bool showingSource{ false };
		bool cancelling{ false };
	};

	void populate();

	void fit();

	UISceneNode& mScene;
	UIWidget& mAnchor;
	std::function<void( Uint64 )> mCancel;
	SmallVector<Row, 4> mRows;
	EventConnection mSceneSize;
	EventConnection mAnchorPosition;
	EventConnection mAnchorSize;
	UIWindow* mBox{ nullptr };
	UIWidget* mRowsBox{ nullptr };
	int mPane;
};

} // namespace eefm

#endif
