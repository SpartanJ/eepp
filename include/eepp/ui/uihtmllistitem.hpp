#ifndef EE_UI_UIHTMLLISTITEM_HPP
#define EE_UI_UIHTMLLISTITEM_HPP

#include <eepp/graphics/text.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <memory>

namespace EE { namespace UI {

class EE_API UIHTMLListItem : public UIRichText {
  public:
	static UIHTMLListItem* New();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void draw();

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	CSSListStyleType getListStyleType() const { return mListStyleType; }

	void setListStyleType( CSSListStyleType type );

	CSSListStylePosition getListStylePosition() const { return mListStylePosition; }

	void setListStylePosition( CSSListStylePosition pos );

  protected:
	UIHTMLListItem();

	CSSListStyleType mListStyleType{ CSSListStyleType::None };
	CSSListStylePosition mListStylePosition{ CSSListStylePosition::Outside };
	std::unique_ptr<Graphics::Text> mListMarkerText;

	int countPrecedingLiSiblings() const;

	String::View getListMarkerString() const;

	void invalidateList();
};

}} // namespace EE::UI

#endif
