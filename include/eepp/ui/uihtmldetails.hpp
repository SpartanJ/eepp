#ifndef EE_UI_UIHTMLDETAILS_HPP
#define EE_UI_UIHTMLDETAILS_HPP

#include <eepp/graphics/text.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <memory>

namespace EE { namespace UI {

class UIHTMLSummary;

class EE_API UIHTMLDetails : public UIRichText {
  public:
	static UIHTMLDetails* New();

	virtual Uint32 getType() const override;

	virtual bool isType( const Uint32& type ) const override;

	virtual void loadFromXmlNode( const pugi::xml_node& node ) override;

	virtual bool applyProperty( const StyleSheetProperty& attribute ) override;

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const override;

	virtual std::vector<PropertyId> getPropertiesImplemented() const override;

	virtual void updateLayout() override;

	bool isOpen() const;

	void setOpen( bool open );

	UIHTMLSummary* findSummaryChild() const;

	bool isActiveSummary( const UIHTMLSummary* summary ) const;

	void markSummaryMarkersDirty();

  protected:
	bool mOpen{ false };
	UIHTMLSummary* mAutoSummary{ nullptr };
	UnorderedMap<Node*, bool> mForcedHiddenChildren;
	bool mSyncingDetailsChildren{ false };

	UIHTMLDetails();

	void ensureAutoSummary();

	void syncDetailsChildrenVisibility();

	virtual void onChildCountChange( Node* child, const bool& removed ) override;
};

class EE_API UIHTMLSummary : public UIRichText {
  public:
	static UIHTMLSummary* New();

	virtual Uint32 getType() const override;

	virtual bool isType( const Uint32& type ) const override;

	virtual void draw() override;

	virtual Uint32 onMouseClick( const Vector2i& position, const Uint32& flags ) override;

	virtual Uint32 onKeyDown( const KeyEvent& event ) override;

	void setDisclosureMarkerDirty();

	CSSListStyleType getListStyleType() const { return mListStyleType; }

	void setListStyleType( CSSListStyleType type );

	virtual bool applyProperty( const StyleSheetProperty& attribute ) override;

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const override;

	virtual std::vector<PropertyId> getPropertiesImplemented() const override;

	UIHTMLDetails* findParentDetails() const;

	bool toggleParentDetails();

  protected:
	CSSListStyleType mListStyleType{ CSSListStyleType::DisclosureClosed };
	std::unique_ptr<Graphics::Text> mListMarkerText;
	bool mUsingDefaultListStylePadding{ true };
	bool mApplyingDefaultListStylePadding{ false };

	UIHTMLSummary();

	CSSListStyleType getDisclosureMarkerType() const;

	int countPrecedingSummarySiblings() const;

	void invalidateListMarker();

	void syncDefaultListStylePadding();

	virtual void onFontChanged() override;

	virtual void onFontStyleChanged() override;
};

}} // namespace EE::UI

#endif
