#ifndef ECODE_GITHISTORYTREEVIEW_HPP
#define ECODE_GITHISTORYTREEVIEW_HPP

#include <eepp/graphics/text.hpp>
#include <eepp/ui/uitreeview.hpp>

using namespace EE::Graphics;
using namespace EE::UI;

namespace ecode {

class GitHistoryTreeViewCell : public UITreeViewCell {
  public:
	static GitHistoryTreeViewCell* New() { return eeNew( GitHistoryTreeViewCell, () ); }

	void draw() override;
	void updateCell( Model* model ) override;

  protected:
	GitHistoryTreeViewCell();

	Sizef updateLayout() override;

	Text mMetadataText;
	Color mHintColor;
};

class GitHistoryTreeView : public UITreeView {
  public:
	static GitHistoryTreeView* New() { return eeNew( GitHistoryTreeView, () ); }

	UIWidget* createCell( UIWidget* rowWidget, const ModelIndex& index ) override;

  protected:
	GitHistoryTreeView() = default;
};

} // namespace ecode

#endif
