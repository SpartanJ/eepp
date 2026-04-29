#ifndef EE_UI_INLINELAYOUTER_HPP
#define EE_UI_INLINELAYOUTER_HPP

#include <eepp/ui/uilayouter.hpp>

namespace EE { namespace UI {

class EE_API InlineLayouter : public UILayouter {
  public:
	InlineLayouter( UIWidget* container ) : UILayouter( container ) {}
	void updateLayout() override {}
	void computeIntrinsicWidths() override {}
};

}} // namespace EE::UI

#endif
