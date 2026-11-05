#ifndef EE_UI_UISPLITTER_HPP
#define EE_UI_UISPLITTER_HPP

#include <eepp/scene/eventconnection.hpp>
#include <eepp/ui/uilayout.hpp>

namespace EE { namespace UI {

class EE_API UISplitter : public UILayout {
  public:
	/** The physical edge of the layout axis that the split partition is measured from. It does not
	 * follow the text direction: End is the right pane of a horizontal splitter and the bottom
	 * pane of a vertical one. */
	enum class PartitionOrigin : Uint8 { Start, End };

	static PartitionOrigin partitionOriginFromString( std::string_view origin ) {
		return String::iequals( origin, "end" ) ? PartitionOrigin::End : PartitionOrigin::Start;
	}

	static std::string partitionOriginToString( PartitionOrigin origin ) {
		return origin == PartitionOrigin::End ? "end" : "start";
	}

	static UISplitter* New();

	~UISplitter();

	Uint32 getType() const;

	bool isType( const Uint32& type ) const;

	const UIOrientation& getOrientation() const;

	void setOrientation( const UIOrientation& orientation );

	const bool& alwaysShowSplitter() const;

	void setAlwaysShowSplitter( bool alwaysShowSplitter );

	const bool& hideSplitterOnEdge() const;

	void setHideSplitterOnEdge( bool hideSplitterOnEdge );

	bool isResizable() const;

	/** Enables or disables dragging the separator. A non resizable splitter keeps its separator
	 * as a decorative divider, and an active drag is cancelled. */
	void setResizable( bool resizable );

	PartitionOrigin getPartitionOrigin() const;

	/** Sets the edge the split partition is measured from. The partition keeps its value, so a
	 * 200dp partition with the End origin makes the last pane 200dp wide. */
	void setPartitionOrigin( PartitionOrigin origin );

	bool isSeparatorDragging() const;

	/** Ends an active separator drag, keeping the partition reached so far. */
	void cancelSeparatorDrag();

	/** @return The partition, measured from the partition origin. User drags keep its unit. */
	const StyleSheetLength& getSplitPartition() const;

	void setSplitPartition( const StyleSheetLength& divisionSplit );

	/** Swaps the panes. The partition origin never changes.
	 * @param swapSplitPartition Also mirrors the partition, so each pane keeps its size. */
	void swap( bool swapSplitPartition = false );

	bool isEmpty();

	bool isFull();

	UIWidget* getFirstWidget() const;

	UIWidget* getLastWidget() const;

	bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	virtual void updateLayout();

  protected:
	UIOrientation mOrientation;
	bool mAlwaysShowSplitter;
	bool mHideSplitterOnEdge;
	bool mResizable{ true };
	PartitionOrigin mPartitionOrigin{ PartitionOrigin::Start };
	StyleSheetLength mSplitPartition;
	UIWidget* mSplitter;
	UIWidget* mFirstWidget;
	UIWidget* mLastWidget;
	Scene::EventConnectionList mEventConnections;

	UISplitter();

	virtual void onChildCountChange( Node* child, const bool& removed );

	virtual Uint32 onMessage( const NodeMessage* Msg );

	void updateFromDrag();

	void updateSplitterDragFlags();

	bool shouldShowSplitter() const;

	void updateSeparatorState();

	Float getAxisContentSize() const;

	Float getUsableSplitSpace() const;

	Float getAxisMinSize( const UIWidget* widget ) const;

	bool isPartitionOnEdge( Float extent ) const;

	Float getFirstPaneSize( Float totalSpace ) const;

	StyleSheetLength partitionFromPixels( Float pixels, Float totalSpace ) const;
};

}} // namespace EE::UI

#endif // EE_UI_UISPLITTER_HPP
