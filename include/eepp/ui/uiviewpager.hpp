#ifndef EE_UI_UIVIEWPAGER_HPP
#define EE_UI_UIVIEWPAGER_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIViewPager : public UIWidget {
  public:
	static UIViewPager* New();

	static UIViewPager* NewVertical();

	static UIViewPager* NewHorizontal();

	virtual ~UIViewPager();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void onChildCountChange( Node* child, const bool& removed );

	const UIOrientation& getOrientation() const;

	UIViewPager* setOrientation( const UIOrientation& orientation );

	Float getDragResistance() const;

	/** Number of dip of drag resistance (it will not start moving until this value is exceeded). */
	void setDragResistance( const Float& dragResistance );

	const Float& getChangePagePercent() const;

	/** The normalized porcentage (0..1) number that will trigger the page change after the drag is
	 * over. Default 0.33 (33%). */
	void setChangePagePercent( const Float& changePagePercent );

	const Time& getPageTransitionDuration() const;

	void setPageTransitionDuration( const Time& pageChangeAnimationTime );

	const Float& getMaxEdgeResistance() const;

	/** The normalized porcentage (0..1) that the edge pages are allowed to go outside the limits.
	 * Default 0% ( childs can't go outside the edge limits ).
	 */
	void setMaxEdgeResistance( const Float& maxEdgeResistance );

	const Ease::Interpolation& getTimingFunction() const;

	void setTimingFunction( const Ease::Interpolation& timingFunction );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	const Int32& getTotalPages() const;

	const Int32& getCurrentPage() const;

	void setCurrentPage( const Int32& currentPage, bool animate = false );

	const bool& isLocked() const;

	void setLocked( bool locked );

  protected:
	UIWidget* mContainer;
	UIOrientation mOrientation;
	bool mDragging;
	bool mLocked;
	Float mDragResistance;
	Float mInitialDisplacement;
	Float mDisplacement;
	Float mChangePagePercent;
	Float mMaxEdgeResistance;
	Time mPageTransitionDuration;
	Int32 mCurrentPage;
	Int32 mTotalPages;
	Ease::Interpolation mTimingFunction;
	Vector2f mMouseDownPos;

	UIViewPager();

	virtual void onSizeChange();

	virtual Uint32 onMessage( const NodeMessage* Msg );

	void updateChilds();

	void limitDisplacement();

	void setDisplacement( const Float& val );

	Float getLength() const;

	void moveToPage( const Int32& pageNum, bool animate = true );

	Uint32 onCalculateDrag( const Vector2f& position, const Uint32& flags );

	void onMouseDownEvent();

	void onMouseMoveEvent();

	void onMouseUpEvent();
};

}} // namespace EE::UI

#endif // EE_UI_UIVIEWPAGER_HPP
