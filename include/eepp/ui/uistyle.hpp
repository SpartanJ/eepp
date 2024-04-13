#ifndef EE_UI_UISTYLE_HPP
#define EE_UI_UISTYLE_HPP

#include <eepp/graphics/fontstyleconfig.hpp>
#include <eepp/math/ease.hpp>
#include <eepp/ui/css/animationdefinition.hpp>
#include <eepp/ui/css/elementdefinition.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetstyle.hpp>
#include <eepp/ui/css/transitiondefinition.hpp>
#include <eepp/ui/uistate.hpp>

namespace EE { namespace Graphics {
class Font;
}} // namespace EE::Graphics

namespace EE { namespace UI { namespace CSS {
class StyleSheetPropertyAnimation;
class StyleSheet;
}}} // namespace EE::UI::CSS

namespace EE { namespace UI {

class UIWidget;

class EE_API UIStyle : public UIState {
  public:
	static UIStyle* New( UIWidget* widget );

	explicit UIStyle( UIWidget* widget );

	virtual ~UIStyle();

	bool stateExists( const Uint32& state ) const;

	void load();

	void onStateChange();

	const CSS::StyleSheetProperty* getStatelessStyleSheetProperty( const Uint32& propertyId ) const;

	void setStyleSheetProperties( const CSS::StyleSheetProperties& properties );

	void setStyleSheetProperty( const CSS::StyleSheetProperty& property );

	bool hasTransition( const std::string& propertyName );

	CSS::StyleSheetPropertyAnimation* getAnimation( const CSS::PropertyDefinition* propertyDef );

	bool hasAnimation( const CSS::PropertyDefinition* propertyDef );

	CSS::TransitionDefinition getTransition( const std::string& propertyName );

	const bool& isChangingState() const;

	CSS::StyleSheetVariable getVariable( const std::string& variable );

	bool getForceReapplyProperties() const;

	void setForceReapplyProperties( bool forceReapplyProperties );

	bool getDisableAnimations() const;

	void setDisableAnimations( bool disableAnimations );

	bool isStructurallyVolatile() const;

	void reloadFontFamily();

	void addStructurallyVolatileChild( UIWidget* widget );

	void removeStructurallyVolatileChild( UIWidget* widget );

	UnorderedSet<UIWidget*>& getStructurallyVolatileChilds();

	bool hasProperty( const CSS::PropertyId& propertyId ) const;

	void resetGlobalDefinition();

  protected:
	UIWidget* mWidget;
	std::shared_ptr<CSS::StyleSheetStyle> mElementStyle;
	std::shared_ptr<CSS::ElementDefinition> mGlobalDefinition;
	std::shared_ptr<CSS::ElementDefinition> mDefinition;
	CSS::TransitionsMap mTransitions;
	CSS::AnimationsMap mAnimations;
	UnorderedSet<UIWidget*> mRelatedWidgets;
	UnorderedSet<UIWidget*> mSubscribedWidgets;
	UnorderedSet<UIWidget*> mStructurallyVolatileChilds;
	Uint32 mStateDepthCounter{ 0 };
	Uint64 mLoadedVersion{ 0 };
	const CSS::StyleSheet* mLoadedStyleSheet{ nullptr };
	bool mChangingState;
	bool mForceReapplyProperties;
	bool mDisableAnimations;
	bool mFirstState;

	void applyVarValues( CSS::StyleSheetProperty* style );

	void setVariableFromValue( CSS::StyleSheetProperty* property, const std::string& value );

	void updateState();

	void subscribeNonCacheableStyles();

	void unsubscribeNonCacheableStyles();

	void subscribeRelated( UIWidget* widget );

	void unsubscribeRelated( UIWidget* widget );

	void removeFromSubscribedWidgets( UIWidget* widget );

	void removeRelatedWidgets();

	void applyStyleSheetProperty( const CSS::StyleSheetProperty& property,
								  std::shared_ptr<CSS::ElementDefinition> prevDefinition );

	void updateAnimationsPlayState();

	void updateAnimations();

	void startAnimations( const CSS::AnimationsMap& animations );

	void removeAllAnimations();

	void removeAnimation( const CSS::PropertyDefinition* propertyDefinition,
						  const Uint32& propertyIndex );

	CSS::StyleSheetProperty* getLocalProperty( Uint32 propId );

	void addStructurallyVolatileWidgetFromParent();

	void removeStructurallyVolatileWidgetFromParent();
};

}} // namespace EE::UI

#endif
