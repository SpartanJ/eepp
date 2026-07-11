#ifndef EE_GRAPHICS_UILAYOUT_HPP
#define EE_GRAPHICS_UILAYOUT_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class GridLayouter;

class EE_API UILayout : public UIWidget {
  public:
	struct Metrics {
		Uint64 autoSizeChildren{ 0 };
		Uint64 childCountChanges{ 0 };
		Uint64 invalidations{ 0 };
		Uint64 richTextRebuilds{ 0 };
		Uint64 synchronousUpdates{ 0 };
		Uint64 treeUpdates{ 0 };
	};

	virtual Uint32 getType() const { return UI_TYPE_LAYOUT; }

	virtual bool isType( const Uint32& type ) const {
		return UILayout::getType() == type || UIWidget::isType( type );
	}

	virtual const Sizef& getSize() const;

	virtual void updateLayout();

	bool isGravityOwner() const;

	void setGravityOwner( bool gravityOwner );

	virtual bool isPacking() const { return mPacking; }

	bool isLayoutDirty() const { return mDirtyLayout; }

	void onAutoSizeChild( UIWidget* child );

	void setLayoutDirty();

	void setLayoutDirty( LayoutInvalidationFlags reasons );

	static void resetMetrics();

	static Metrics getMetrics();

	static void setMetricsEnabled( bool enabled );

	static void countRichTextRebuild();

  protected:
	friend class GridLayouter;
	friend class UISceneNode;
	friend class UILayouter;

	UnorderedSet<UILayout*> mLayouts;
	bool mDirtyLayout{ false };
	LayoutInvalidationFlags mDirtyReasons{ 0 };
	LayoutInvalidationFlags mCurrentLayoutReasons{ 0 };
	// True only while updateLayoutTree() is actively updating this layout and walking its
	// descendants. This is intentionally narrower than mDirtyLayout: code that receives layout
	// notifications can use it to distinguish "already scheduled for a future pass" from
	// "currently rebuilding/positioning children, so re-entering now would duplicate work."
	bool mUpdatingLayoutTree{ false };
	bool mPacking{ false };
	bool mGravityOwner{ false };

	explicit UILayout( const std::string& tag );

	virtual void onSizeChange();

	virtual void onPaddingChange();

	virtual void onParentSizeChange( const Vector2f& SizeChange );

	virtual void onChildCountChange( Node* child, const bool& removed );

	virtual void onLayoutUpdate();

	virtual void tryUpdateLayout();

	virtual void updateLayoutTree();

	virtual void updateLayoutWrappingContents();

	bool setMatchParentIfNeededVerticalGrowth();
};

}} // namespace EE::UI

#endif
