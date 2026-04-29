#ifndef EE_UI_UIDROPDOWNMODELLIST_HPP
#define EE_UI_UIDROPDOWNMODELLIST_HPP

#include <eepp/ui/abstract/uiabstracttableview.hpp>
#include <eepp/ui/uidropdown.hpp>

namespace EE { namespace UI {

class EE_API UIDropDownModelList : public UIDropDown {
  public:
	using MenuWidthMode = UIDropDown::MenuWidthMode;
	using StyleConfig = UIDropDown::StyleConfig;

	static UIDropDownModelList* NewWithTag( const std::string& tag );

	static UIDropDownModelList* New();

	virtual ~UIDropDownModelList();

	virtual Uint32 getType() const;
	virtual bool isType( const Uint32& type ) const;

	UIAbstractTableView* getListView() const;

	void setListView( UIAbstractTableView* listView );

	std::shared_ptr<Model> getModel() const;

	virtual void setModel( std::shared_ptr<Model> model );

	UIDropDownModelList* showList();

	virtual UIDropDownModelList* setMaxNumVisibleItems( const Uint32& maxNumVisibleItems );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

  protected:
	UIAbstractTableView* mListView;
	Uint32 mListViewCloseCb{ 0 };
	std::shared_ptr<Model> mModel;

	UIDropDownModelList( const std::string& tag = "dropdownmodellist" );

	virtual UIWidget* getPopUpWidget() const;

	virtual void onItemSelected( const Event* Event );

	virtual Uint32 onMouseUp( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onKeyDown( const KeyEvent& Event );

	virtual void onItemClicked( const Event* Event );

	virtual void onClassChange();

	void destroyListView();

	UIWidget* createDefaultListView();

	void updateSelectionIndex();
};

}} // namespace EE::UI

#endif
