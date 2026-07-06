#ifndef EE_UI_UIHTMLWIDGET_HPP
#define EE_UI_UIHTMLWIDGET_HPP

#include <eepp/core/small_vector.hpp>
#include <eepp/ui/csslayouttypes.hpp>
#include <eepp/ui/uilayout.hpp>

namespace EE { namespace Graphics {
class RichText;
}} // namespace EE::Graphics

namespace EE { namespace UI {

class UILayouter;

struct UIHTMLWidgetFlexState {
	CSSFlexDirection direction{ CSSFlexDirection::Row };
	CSSFlexWrap wrap{ CSSFlexWrap::NoWrap };
	CSSJustifyContent justifyContent{ CSSJustifyContent::FlexStart };
	CSSAlignItems alignItems{ CSSAlignItems::Stretch };
	CSSAlignContent alignContent{ CSSAlignContent::Stretch };
	CSSAlignSelf alignSelf{ CSSAlignSelf::Auto };
	Float flexGrow{ 0.f };
	Float flexShrink{ 1.f };
	std::string flexBasis{ "auto" };
	int order{ 0 };
	std::string rowGap{ "0" };
	std::string columnGap{ "0" };
};

struct UIHTMLWidgetGridState {
	std::string templateRows{ "none" };
	std::string templateColumns{ "none" };
	std::string templateAreas{ "none" };
	std::string autoRows{ "auto" };
	std::string autoColumns{ "auto" };
	CSSGridAutoFlow autoFlow{ CSSGridAutoFlow::Row };
	bool autoFlowDense{ false };
	std::string rowStart{ "auto" };
	std::string rowEnd{ "auto" };
	std::string columnStart{ "auto" };
	std::string columnEnd{ "auto" };
	std::string area{ "auto" };
	CSSJustifyItems justifyItems{ CSSJustifyItems::Normal };
	CSSJustifySelf justifySelf{ CSSJustifySelf::Auto };
};

class EE_API UIHTMLWidget : public UILayout {
  public:
	static UIHTMLWidget* New();

	UIHTMLWidget( const std::string& tag = "htmlwidget" );

	virtual ~UIHTMLWidget();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	UILayouter* getLayouter();

	virtual bool isPacking() const;

	virtual void onDisplayChange();

	CSSDisplay getDisplay() const { return mDisplay; }

	void setDisplay( CSSDisplay display );

	bool isFlex() const;

	bool isGrid() const;

	CSSPosition getCSSPosition() const { return mPosition; }

	void setCSSPosition( CSSPosition position );

	CSSFloat getCSSFloat() const { return mFloat; }

	void setCSSFloat( CSSFloat cssFloat );

	CSSClear getCSSClear() const { return mClear; }

	void setCSSClear( CSSClear cssClear );

	CSSBoxSizing getBoxSizing() const { return mBoxSizing; }

	void setBoxSizing( CSSBoxSizing boxSizing );

	Rectf getNormalFlowLayoutPixelsMargin() const;

	const CSSBaselineAlignValue& getBaselineAlign() const { return mBaselineAlign; }

	void setBaselineAlign( const CSSBaselineAlignValue& baselineAlign );

	const Rectf& getOffsets() const { return mOffsets; }

	void setOffsets( const Rectf& offsets );

	int getZIndex() const { return mZIndex; }

	void setZIndex( int zIndex );

	bool getNeedsOrderSort() const { return mNeedsOrderSort; }

	void setNeedsOrderSort( bool val );

	UIHTMLWidgetFlexState* getFlexState() const { return mFlexState; }

	UIHTMLWidgetFlexState* ensureFlexState() {
		if ( !mFlexState )
			mFlexState = eeNew( UIHTMLWidgetFlexState, () );
		return mFlexState;
	}

	UIHTMLWidgetGridState* getGridState() const { return mGridState; }

	UIHTMLWidgetGridState* ensureGridState() {
		if ( !mGridState )
			mGridState = eeNew( UIHTMLWidgetGridState, () );
		return mGridState;
	}

	CSSFlexDirection getFlexDirection() const {
		return mFlexState ? mFlexState->direction : CSSFlexDirection::Row;
	}
	void setFlexDirection( CSSFlexDirection val );

	CSSFlexWrap getFlexWrap() const { return mFlexState ? mFlexState->wrap : CSSFlexWrap::NoWrap; }
	void setFlexWrap( CSSFlexWrap val );

	CSSJustifyContent getJustifyContent() const {
		return mFlexState ? mFlexState->justifyContent : CSSJustifyContent::FlexStart;
	}
	void setJustifyContent( CSSJustifyContent val );

	CSSAlignItems getAlignItems() const {
		return mFlexState ? mFlexState->alignItems : CSSAlignItems::Stretch;
	}
	void setAlignItems( CSSAlignItems val );

	CSSAlignContent getAlignContent() const {
		return mFlexState ? mFlexState->alignContent : CSSAlignContent::Stretch;
	}
	void setAlignContent( CSSAlignContent val );

	CSSAlignSelf getAlignSelf() const {
		return mFlexState ? mFlexState->alignSelf : CSSAlignSelf::Auto;
	}
	void setAlignSelf( CSSAlignSelf val );

	CSSVisibility getVisibility() const { return mVisibility; }
	void setVisibility( CSSVisibility val );

	Float getFlexGrow() const { return mFlexState ? mFlexState->flexGrow : 0.f; }
	void setFlexGrow( Float val );

	Float getFlexShrink() const { return mFlexState ? mFlexState->flexShrink : 1.f; }
	void setFlexShrink( Float val );

	const std::string& getFlexBasis() const {
		static const std::string sDefault( "auto" );
		return mFlexState ? mFlexState->flexBasis : sDefault;
	}
	void setFlexBasis( const std::string& val );

	int getOrder() const { return mFlexState ? mFlexState->order : 0; }
	void setOrder( int val );

	const std::string& getGridTemplateRows() const {
		static const std::string sDefault( "none" );
		return mGridState ? mGridState->templateRows : sDefault;
	}
	void setGridTemplateRows( const std::string& val );

	const std::string& getGridTemplateColumns() const {
		static const std::string sDefault( "none" );
		return mGridState ? mGridState->templateColumns : sDefault;
	}
	void setGridTemplateColumns( const std::string& val );

	const std::string& getGridTemplateAreas() const {
		static const std::string sDefault( "none" );
		return mGridState ? mGridState->templateAreas : sDefault;
	}
	void setGridTemplateAreas( const std::string& val );

	const std::string& getGridAutoRows() const {
		static const std::string sDefault( "auto" );
		return mGridState ? mGridState->autoRows : sDefault;
	}
	void setGridAutoRows( const std::string& val );

	const std::string& getGridAutoColumns() const {
		static const std::string sDefault( "auto" );
		return mGridState ? mGridState->autoColumns : sDefault;
	}
	void setGridAutoColumns( const std::string& val );

	CSSGridAutoFlow getGridAutoFlow() const {
		return mGridState ? mGridState->autoFlow : CSSGridAutoFlow::Row;
	}
	void setGridAutoFlow( CSSGridAutoFlow val );

	bool getGridAutoFlowDense() const { return mGridState ? mGridState->autoFlowDense : false; }
	void setGridAutoFlowDense( bool val );

	const std::string& getGridRowStart() const {
		static const std::string sDefault( "auto" );
		return mGridState ? mGridState->rowStart : sDefault;
	}
	void setGridRowStart( const std::string& val );

	const std::string& getGridRowEnd() const {
		static const std::string sDefault( "auto" );
		return mGridState ? mGridState->rowEnd : sDefault;
	}
	void setGridRowEnd( const std::string& val );

	const std::string& getGridColumnStart() const {
		static const std::string sDefault( "auto" );
		return mGridState ? mGridState->columnStart : sDefault;
	}
	void setGridColumnStart( const std::string& val );

	const std::string& getGridColumnEnd() const {
		static const std::string sDefault( "auto" );
		return mGridState ? mGridState->columnEnd : sDefault;
	}
	void setGridColumnEnd( const std::string& val );

	const std::string& getGridArea() const {
		static const std::string sDefault( "auto" );
		return mGridState ? mGridState->area : sDefault;
	}
	void setGridArea( const std::string& val );

	CSSJustifyItems getJustifyItems() const {
		return mGridState ? mGridState->justifyItems : CSSJustifyItems::Normal;
	}
	void setJustifyItems( CSSJustifyItems val );

	CSSJustifySelf getJustifySelf() const {
		return mGridState ? mGridState->justifySelf : CSSJustifySelf::Auto;
	}
	void setJustifySelf( CSSJustifySelf val );

	const std::string& getRowGap() const {
		static const std::string sDefault( "0" );
		return mFlexState ? mFlexState->rowGap : sDefault;
	}
	void setRowGap( const std::string& val );

	const std::string& getColumnGap() const {
		static const std::string sDefault( "0" );
		return mFlexState ? mFlexState->columnGap : sDefault;
	}
	void setColumnGap( const std::string& val );

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	using UIWidget::getPropertyString;

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& state = 0 ) const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual void updateLayout();

	virtual void drawChildren();

	virtual Node* overFind( const Vector2f& point );

	virtual void onChildCountChange( Node* child, const bool& removed );

	void updateZIndexSortFlag();

	void buildDrawOrderVector( SmallVector<Node*, 127>& out ) const;

	Float getBaseline() const;

	Float getContainingBlockContentWidth() const;

	Float getContainingBlockContentHeight() const;

	Float lengthFromValueForCSS( const StyleSheetProperty& property,
								 const Float& defaultValue = 0 ) const;

	Float cssResolvedLengthToBorderBoxWidth( const Float& resolvedLength ) const;

	Float cssResolvedLengthToBorderBoxHeight( const Float& resolvedLength ) const;

	Float cssWidthPropertyToBorderBoxWidth( const StyleSheetProperty& property ) const;

	Float cssHeightPropertyToBorderBoxHeight( const StyleSheetProperty& property ) const;

	void updateCSSContentBoxFixedSize();

	virtual void onParentChange();

	virtual void onPositionChange();

	UIWidget* getContainingBlock();

	void positionOutOfFlowChildren();

	void updateOutOfFlowPosition();

	void updateStickyPosition();

	virtual RichText* getRichTextPtr() { return nullptr; }

	virtual bool isInline() const { return false; }

	virtual String getFormValue() const { return String(); }

	virtual void invalidateIntrinsicSize();

	bool isOutOfFlow() const;

	bool establishesBlockFormattingContext() const;

	bool hasDataProperty( const std::string& name ) const;

	const StyleSheetProperty* getDataProperty( const std::string& name ) const;

	std::string getDataPropertyString( const std::string& name,
									   const std::string& defaultValue = "" ) const;

	void setDataProperty( const StyleSheetProperty& property );

	void setDataProperty( const std::string& name, const std::string& value );

	void removeDataProperty( const std::string& name );

	const UnorderedMap<std::string, StyleSheetProperty>& getDataProperties() const {
		return mDataProperties;
	}

  protected:
	CSSDisplay mDisplay{ CSSDisplay::Block };
	CSSPosition mPosition{ CSSPosition::Static };
	CSSFloat mFloat{ CSSFloat::None };
	CSSClear mClear{ CSSClear::None };
	CSSBoxSizing mBoxSizing{ CSSBoxSizing::ContentBox };
	CSSBaselineAlignValue mBaselineAlign;
	CSSVisibility mVisibility{ CSSVisibility::Visible };
	std::string mTopEq{ "auto" };
	std::string mRightEq{ "auto" };
	std::string mBottomEq{ "auto" };
	std::string mLeftEq{ "auto" };
	Rectf mOffsets{ 0, 0, 0, 0 };
	int mZIndex{ 0 };
	bool mOverflowCreatesBlockFormattingContext{ false };
	bool mNeedsOrderSort{ false };
	bool mNeedsZIndexSort{ false };
	UILayouter* mLayouter{ nullptr };
	UIHTMLWidgetFlexState* mFlexState{ nullptr };
	UIHTMLWidgetGridState* mGridState{ nullptr };
	UnorderedMap<std::string, StyleSheetProperty> mDataProperties;

	Uint32 mScrollCb{ 0 };
	Node* mScrollTarget{ nullptr };
	Vector2f mStickyBasePos;
	bool mIsUpdatingScroll{ false };

	void updateScrollListeners();
	void onScrollTargetPositionChange();
};

}} // namespace EE::UI

#endif
