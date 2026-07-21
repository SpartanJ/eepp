#ifndef EE_UI_BLOCKLAYOUTER_HPP
#define EE_UI_BLOCKLAYOUTER_HPP

#include <eepp/core/containers.hpp>
#include <eepp/graphics/richtext.hpp>
#include <eepp/ui/uilayouter.hpp>

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
	struct FragmentBucket {
		SmallVector<const RichText::InlineFragment*, 4> textRuns;
		SmallVector<const RichText::InlineFragment*, 4> boxes;
		SmallVector<const RichText::InlineFragment*, 4> atomicBoxes;

		void clear() {
			textRuns.clear();
			boxes.clear();
			atomicBoxes.clear();
		}
	};

	void positionRichTextChildren( RichText* rt );

	UnorderedMap<void*, FragmentBucket> mTextNodeFragments;
	UnorderedMap<void*, FragmentBucket> mWidgetFragments;
};

}} // namespace EE::UI

#endif
