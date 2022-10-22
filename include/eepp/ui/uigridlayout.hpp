#ifndef EE_UI_UIGRIDLAYOUT
#define EE_UI_UIGRIDLAYOUT

#include <eepp/ui/uilayout.hpp>

namespace EE { namespace UI {

class EE_API UIGridLayout : public UILayout {
  public:
	enum ElementMode { Size, Weight };

	static UIGridLayout* New();

	UIGridLayout();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	Sizei getBoxMargin() const;

	UIGridLayout* setBoxMargin( const Sizei& span );

	ElementMode getColumnMode() const;

	UIGridLayout* setColumnMode( const ElementMode& mode );

	ElementMode getRowMode() const;

	UIGridLayout* setRowMode( const ElementMode& mode );

	Float getColumnWeight() const;

	UIGridLayout* setColumnWeight( const Float& columnWeight );

	int getColumnWidth() const;

	UIGridLayout* setColumnWidth( int columnWidth );

	int getRowHeight() const;

	UIGridLayout* setRowHeight( int rowHeight );

	Float getRowWeight() const;

	UIGridLayout* setRowWeight( const Float& rowWeight );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual void updateLayout();

  protected:
	Sizei mBoxMargin;
	ElementMode mColumnMode;
	ElementMode mRowMode;
	Float mColumnWeight;
	int mColumnWidth;
	Float mRowWeight;
	int mRowHeight;

	virtual Uint32 onMessage( const NodeMessage* Msg );

	virtual void onParentSizeChange( const Vector2f& size );

	Sizef getTargetElementSize() const;
};

}} // namespace EE::UI

#endif
