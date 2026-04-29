#ifndef EE_UI_NONELAYOUTER_HPP
#define EE_UI_NONELAYOUTER_HPP

#include <eepp/ui/uilayouter.hpp>

namespace EE { namespace UI {

class EE_API NoneLayouter : public UILayouter {
  public:
	NoneLayouter( UIWidget* container ) : UILayouter( container ) {}
	void updateLayout() override {}
	void computeIntrinsicWidths() override {}
};

}} // namespace EE::UI

#endif
