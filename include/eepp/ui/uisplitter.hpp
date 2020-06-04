#ifndef EE_UI_UISPLITTER_HPP
#define EE_UI_UISPLITTER_HPP

#include <eepp/ui/uilayout.hpp>

namespace EE { namespace UI {

class EE_API UISplitter : public UILayout {
  public:
	static UISplitter* New();

	UISplitter();

	~UISplitter();

	Uint32 getType() const;

	bool isType( const Uint32& type ) const;

	const UIOrientation& getOrientation() const;

	void setOrientation( const UIOrientation& orientation );

	const bool& alwaysShowSplitter() const;

	void setAlwaysShowSplitter( bool alwaysShowSplitter );

	const Float& getDivisionSplit() const;

	void setDivisionSplit( const Float& divisionSplit );

	void swap();

	bool isEmpty();

	bool isFull();

	UIWidget* getFirstWidget() const;

	UIWidget* getLastWidget() const;

  protected:
	UIOrientation mOrientation;
	bool mSplitOnlyWhenNeeded;
	bool mAlwaysShowSplitter;
	Float mDivisionSplit;
	UIWidget* mSplitter;
	UIWidget* mFirstWidget;
	UIWidget* mLastWidget;

	virtual void onChildCountChange( Node* child, const bool& removed );

	virtual void updateLayout();

	virtual Uint32 onMessage( const NodeMessage* Msg );

	void updateFromDrag();
};

}} // namespace EE::UI

#endif // EE_UI_UISPLITTER_HPP
