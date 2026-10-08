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

	virtual void invalidateIntrinsicWidths() {
		mIntrinsicWidthsDirty = true;
		mInlineContentDirty = true;
	}

	virtual bool isPacking() const { return mPacking; }

	/** True while the inline formatting owner assigns its computed fragment boxes. */
	bool isPositioningInlineFragments() const { return mPositioningInlineFragments; }

  protected:
	UIWidget* mContainer;
	bool mPacking{ false };
	// These flags occupy existing padding, preserving the size and member offsets of layouters.
	bool mInlineContentDirty{ true };
	bool mInlineContentReusable{ false };
	bool mPositioningInlineFragments{ false };
	size_t mResizedCount{ 0 };
	bool mIntrinsicWidthsDirty{ true };
	Float mMinIntrinsicWidth{ 0 };
	Float mMaxIntrinsicWidth{ 0 };
	Uint32 mPositionedFragmentsGeneration{ 0 };

	void setMatchParentIfNeededVerticalGrowth();
};

}} // namespace EE::UI

#endif
