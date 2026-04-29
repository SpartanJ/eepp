#ifndef EE_UI_UILAYOUTER_HPP
#define EE_UI_UILAYOUTER_HPP

#include <cstddef>
#include <eepp/config.hpp>

namespace EE { namespace UI {

class UIWidget;

class EE_API UILayouter {
  public:
	UILayouter( UIWidget* container ) : mContainer( container ) {}
	virtual ~UILayouter() {}

	virtual void updateLayout() = 0;
	virtual void computeIntrinsicWidths() {}
	virtual Float getMinIntrinsicWidth() { return 0; }
	virtual Float getMaxIntrinsicWidth() { return 0; }

	virtual void invalidateIntrinsicWidths() { mIntrinsicWidthsDirty = true; }
	virtual bool isPacking() const { return mPacking; }

  protected:
	UIWidget* mContainer;
	bool mPacking{ false };
	size_t mResizedCount{ 0 };
	bool mIntrinsicWidthsDirty{ true };
	Float mMinIntrinsicWidth{ 0 };
	Float mMaxIntrinsicWidth{ 0 };

	void setMatchParentIfNeededVerticalGrowth();
};

}} // namespace EE::UI

#endif
