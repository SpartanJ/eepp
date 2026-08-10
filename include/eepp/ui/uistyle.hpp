#ifndef EE_UI_UISTYLE_HPP
#define EE_UI_UISTYLE_HPP

#include <eepp/graphics/fontstyleconfig.hpp>
#include <eepp/math/ease.hpp>
#include <eepp/ui/css/animationdefinition.hpp>
#include <eepp/ui/css/elementdefinition.hpp>
#include <eepp/ui/css/propertyidset.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetstyle.hpp>
#include <eepp/ui/css/transitiondefinition.hpp>
#include <eepp/ui/uistate.hpp>
#include <memory>

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

	UIStyle( const UIStyle& ) = delete;
	UIStyle& operator=( const UIStyle& ) = delete;
	UIStyle( UIStyle&& ) = delete;
	UIStyle& operator=( UIStyle&& ) = delete;

	virtual ~UIStyle();

	bool stateExists( const Uint32& state ) const;

	void load();

	void onStateChange();

	const CSS::StyleSheetProperty*
	getStatelessStyleSheetProperty( const CSS::PropertyId& propertyId ) const;

	void setStyleSheetProperties( const CSS::StyleSheetProperties& properties );

	void setStyleSheetProperty( const CSS::StyleSheetProperty& property );

	void setStyleSheetVariable( const CSS::StyleSheetVariable& variable );

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

	UnorderedSet<UIWidget*>& getStructurallyVolatileChildren();

	const CSS::StyleSheetProperty* getProperty( const CSS::PropertyId& id ) const;

	bool hasProperty( const CSS::PropertyId& propertyId ) const;

	bool hasLocalProperty( CSS::PropertyId propId ) const;

	CSS::StyleSheetProperty* getInheritedProperty( CSS::PropertyId propId,
												   UIStyle** ownerStyle = nullptr ) const;

	void resetGlobalDefinition( bool force = false );

	void resetCachedProperties();

	void applyInheritedProperties();

	const std::shared_ptr<CSS::ElementDefinition> getDefinition() const { return mDefinition; }

	void applyVarValues( CSS::StyleSheetProperty* style );

  protected:
	class EE_API PropertyResolution {
	  public:
		PropertyResolution( const PropertyResolution& ) = delete;
		PropertyResolution& operator=( const PropertyResolution& ) = delete;
		PropertyResolution( PropertyResolution&& other ) noexcept;
		PropertyResolution& operator=( PropertyResolution&& ) = delete;
		~PropertyResolution();

		const CSS::StyleSheetProperty* get() const { return mProperty; }

	  private:
		friend class UIStyle;
		static constexpr Uint32 NoSlot = static_cast<Uint32>( -1 );

		PropertyResolution( UIStyle* owner, const CSS::StyleSheetProperty* property,
							Uint32 slot = NoSlot ) :
			mOwner( owner ), mProperty( property ), mSlot( slot ) {}

		void release();

		UIStyle* mOwner;
		const CSS::StyleSheetProperty* mProperty;
		Uint32 mSlot;
	};

	UIWidget* mWidget;
	std::shared_ptr<CSS::StyleSheetStyle> mElementStyle;
	std::shared_ptr<CSS::ElementDefinition> mGlobalDefinition;
	std::shared_ptr<CSS::ElementDefinition> mDefinition;
	CSS::AnimationsMap mAnimations;
	UnorderedSet<UIWidget*> mRelatedWidgets;
	UnorderedSet<UIWidget*> mSubscribedWidgets;
	UnorderedSet<UIWidget*> mStructurallyVolatileChildren;
	Uint32 mStateDepthCounter{ 0 };
	Uint32 mPropertyResolutionDepth{ 0 };
	Uint64 mLoadedVersion{ 0 };
	const CSS::StyleSheet* mLoadedStyleSheet{ nullptr };
	/** Lazily allocated for styles that use substitutions. The common, non-reentrant resolution
	 * needs no container allocation; nested slots are retained for later reuse. */
	std::unique_ptr<CSS::StyleSheetProperty> mPropertyResolutionSlot;
	std::vector<std::unique_ptr<CSS::StyleSheetProperty>> mNestedPropertyResolutionSlots;
	bool mChangingState;
	bool mForceReapplyProperties;
	bool mDisableAnimations;
	bool mFirstState;

	explicit UIStyle( UIWidget* widget );

	void applyLightDarkValues( CSS::StyleSheetProperty* style );

	void applyLightDarkValue( std::string& newValue );

	const CSS::StyleSheetVariable* getVariableRef( const std::string& variable );

	void setVariableFromValue( CSS::StyleSheetProperty* property );

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

	CSS::StyleSheetProperty* getLocalProperty( CSS::PropertyId propId );

	PropertyResolution resolveProperty( const CSS::StyleSheetProperty* property );

	PropertyResolution getResolvedLocalProperty( CSS::PropertyId propId );

	void addStructurallyVolatileWidgetFromParent();

	void removeStructurallyVolatileWidgetFromParent();
};

}} // namespace EE::UI

#endif
