#ifndef EE_UI_UISPLITTER_HPP
#define EE_UI_UISPLITTER_HPP

#include <eepp/ui/uilayout.hpp>

namespace EE { namespace UI {

class EE_API UISplitter : public UILayout {
  public:
	static UISplitter* New();

	~UISplitter();

	Uint32 getType() const;

	bool isType( const Uint32& type ) const;

	const UIOrientation& getOrientation() const;

	void setOrientation( const UIOrientation& orientation );

	const bool& alwaysShowSplitter() const;

	void setAlwaysShowSplitter( bool alwaysShowSplitter );

	const StyleSheetLength& getSplitPartition() const;

	void setSplitPartition( const StyleSheetLength& divisionSplit );

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
	StyleSheetLength mSplitPartition;
	UIWidget* mSplitter;
	UIWidget* mFirstWidget;
	UIWidget* mLastWidget;

	UISplitter();

	virtual void onChildCountChange( Node* child, const bool& removed );

	virtual Uint32 onMessage( const NodeMessage* Msg );

	void updateFromDrag();

	void updateSplitterDragFlags();
};

}} // namespace EE::UI

#endif // EE_UI_UISPLITTER_HPP
