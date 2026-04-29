#ifndef EE_UI_BLOCKLAYOUTER_HPP
#define EE_UI_BLOCKLAYOUTER_HPP

#include <eepp/ui/uilayouter.hpp>

namespace EE::Graphics {
class RichText;
}

using namespace EE::Graphics;

namespace EE { namespace UI {

class EE_API BlockLayouter : public UILayouter {
  public:
	BlockLayouter( UIWidget* container ) : UILayouter( container ) {}
	void updateLayout() override;
	void computeIntrinsicWidths() override;
	Float getMinIntrinsicWidth() override;
	Float getMaxIntrinsicWidth() override;

  protected:
	void positionRichTextChildren( RichText* rt );
};

}} // namespace EE::UI

#endif
