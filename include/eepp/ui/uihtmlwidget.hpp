#ifndef EE_UI_UIHTMLWIDGET_HPP
#define EE_UI_UIHTMLWIDGET_HPP

#include <eepp/ui/csslayouttypes.hpp>
#include <eepp/ui/uilayout.hpp>

namespace EE { namespace Graphics {
class RichText;
}} // namespace EE::Graphics

namespace EE { namespace UI {

class UILayouter;

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

	CSSPosition getCSSPosition() const { return mPosition; }
	void setCSSPosition( CSSPosition position );

	const Rectf& getOffsets() const { return mOffsets; }
	void setOffsets( const Rectf& offsets );

	int getZIndex() const { return mZIndex; }
	void setZIndex( int zIndex );

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& state = 0 ) const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual void updateLayout();

	UIWidget* getContainingBlock();

	void positionOutOfFlowChildren();

	virtual RichText* getRichTextPtr() { return nullptr; }

	virtual bool isMergeable() const { return false; }

	virtual String getFormValue() const { return String(); }

	virtual void invalidateIntrinsicSize();

	bool isOutOfFlow() const;

  protected:
	CSSDisplay mDisplay{ CSSDisplay::Block };
	CSSPosition mPosition{ CSSPosition::Static };
	std::string mTopEq{ "auto" };
	std::string mRightEq{ "auto" };
	std::string mBottomEq{ "auto" };
	std::string mLeftEq{ "auto" };
	Rectf mOffsets{ 0, 0, 0, 0 };
	int mZIndex{ 0 };
	UILayouter* mLayouter{ nullptr };
	UnorderedMap<std::string, StyleSheetProperty> mDataProperties;
};

}} // namespace EE::UI

#endif
