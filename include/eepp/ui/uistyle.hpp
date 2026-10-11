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

using namespace EE::UI::CSS;

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

	const StyleSheetProperty* getStatelessStyleSheetProperty( const PropertyId& propertyId ) const;

	void setStyleSheetProperties( const StyleSheetProperties& properties );

	void setStyleSheetProperty( const StyleSheetProperty& property );

	void setStyleSheetVariable( const StyleSheetVariable& variable );

	bool hasTransition( const std::string& propertyName );

	StyleSheetPropertyAnimation* getAnimation( const PropertyDefinition* propertyDef );

	bool hasAnimation( const PropertyDefinition* propertyDef );

	TransitionDefinition getTransition( const std::string& propertyName );

	const bool& isChangingState() const;

	StyleSheetVariable getVariable( const std::string& variable );

	/** Resolve a color variable without copying its stored text. */
	Color getColorVariable( const std::string& variable, Color fallback );

	bool getForceReapplyProperties() const;

	void setForceReapplyProperties( bool forceReapplyProperties );

	bool getDisableAnimations() const;

	void setDisableAnimations( bool disableAnimations );

	bool isStructurallyVolatile() const;

	void reloadFontFamily();

	void addStructurallyVolatileChild( UIWidget* widget );

	void removeStructurallyVolatileChild( UIWidget* widget );

	UnorderedSet<UIWidget*>& getStructurallyVolatileChildren();

	const StyleSheetProperty* getProperty( const PropertyId& id ) const;

	bool hasProperty( const PropertyId& propertyId ) const;

	bool hasLocalProperty( PropertyId propId ) const;

	StyleSheetProperty* getInheritedProperty( PropertyId propId,
											  UIStyle** ownerStyle = nullptr ) const;

	void resetGlobalDefinition( bool force = false );

	void resetCachedProperties();

	void applyInheritedProperties();

	const std::shared_ptr<ElementDefinition> getDefinition() const { return mDefinition; }

	void applyVarValues( StyleSheetProperty* style );

	/**
	 * @return The declaration that currently wins the cascade for @p propId in the widget's
	 * current state (element style or matched rules), without var() substitution.
	 */
	const StyleSheetProperty* getCascadedProperty( PropertyId propId ) {
		return getLocalProperty( propId );
	}

	/** Compares declarations of different IDs that map to the same computed edge. Rule order
	 * follows stylesheet attachment order; declaration order only breaks ties within a rule. */
	bool declarationWins( const StyleSheetProperty& candidate, const StyleSheetProperty& other );

	/** @return Widgets whose styles are refreshed when this widget changes state. */
	const UnorderedSet<UIWidget*>& getRelatedWidgets() const { return mRelatedWidgets; }

  protected:
	class EE_API PropertyResolution {
	  public:
		PropertyResolution( const PropertyResolution& ) = delete;
		PropertyResolution& operator=( const PropertyResolution& ) = delete;
		PropertyResolution( PropertyResolution&& other ) noexcept;
		PropertyResolution& operator=( PropertyResolution&& ) = delete;
		~PropertyResolution();

		const StyleSheetProperty* get() const { return mProperty; }

	  private:
		friend class UIStyle;
		static constexpr Uint32 NoSlot = static_cast<Uint32>( -1 );

		PropertyResolution( UIStyle* owner, const StyleSheetProperty* property,
							Uint32 slot = NoSlot ) :
			mOwner( owner ), mProperty( property ), mSlot( slot ) {}

		void release();

		UIStyle* mOwner;
		const StyleSheetProperty* mProperty;
		Uint32 mSlot;
	};

	UIWidget* mWidget;
	std::shared_ptr<StyleSheetStyle> mElementStyle;
	std::shared_ptr<ElementDefinition> mGlobalDefinition;
	std::shared_ptr<ElementDefinition> mDefinition;
	AnimationsMap mAnimations;
	UnorderedSet<UIWidget*> mRelatedWidgets;
	UnorderedSet<UIWidget*> mSubscribedWidgets;
	UnorderedSet<UIWidget*> mStructurallyVolatileChildren;
	Uint32 mStateDepthCounter{ 0 };
	Uint32 mPropertyResolutionDepth{ 0 };
	Uint64 mLoadedVersion{ 0 };
	const StyleSheet* mLoadedStyleSheet{ nullptr };
	/** Lazily allocated for styles that use substitutions. The common, non-reentrant resolution
	 * needs no container allocation; nested slots are retained for later reuse. */
	std::unique_ptr<StyleSheetProperty> mPropertyResolutionSlot;
	SmallVector<std::unique_ptr<StyleSheetProperty>, 2> mNestedPropertyResolutionSlots;
	struct PropertyFallback {
		PropertyId propertyId;
		Uint32 index;
		const PropertyDefinition* definition;
		std::string nativeValue;
		bool inherited;
	};
	SmallVector<PropertyFallback, 0> mPropertyFallbacks;
	bool mChangingState;
	bool mForceReapplyProperties;
	bool mDisableAnimations;
	bool mFirstState;

	explicit UIStyle( UIWidget* widget );

	void applyLightDarkValues( StyleSheetProperty* style );

	void applyLightDarkValue( std::string& newValue );

	const StyleSheetVariable* getVariableRef( const std::string& variable );

	void setVariableFromValue( StyleSheetProperty* property );

	void updateState();

	void subscribeNonCacheableStyles();

	void unsubscribeNonCacheableStyles();

	void subscribeRelated( UIWidget* widget );

	void unsubscribeRelated( UIWidget* widget );

	void removeFromSubscribedWidgets( UIWidget* widget );

	void removeRelatedWidgets();

	void applyStyleSheetProperty( const StyleSheetProperty& property,
								  std::shared_ptr<ElementDefinition> prevDefinition,
								  bool captureFallback = true );

	void capturePropertyFallback( const StyleSheetProperty& property );

	bool isTransientProperty( PropertyId propertyId ) const;

	bool restorePropertyFallbacks( PropertyId propertyId,
								   std::shared_ptr<ElementDefinition> prevDefinition,
								   Uint32 firstIndex = 0 );

	void clearPropertyFallbacks( PropertyId propertyId );

	void updateAnimationsPlayState();

	void updateAnimations();

	void startAnimations( const AnimationsMap& animations );

	void removeAllAnimations();

	void removeAnimation( const PropertyDefinition* propertyDefinition,
						  const Uint32& propertyIndex );

	StyleSheetProperty* getLocalProperty( PropertyId propId );

	PropertyResolution resolveProperty( const StyleSheetProperty* property );

	PropertyResolution getResolvedLocalProperty( PropertyId propId );

	void addStructurallyVolatileWidgetFromParent();

	void removeStructurallyVolatileWidgetFromParent();
};

}} // namespace EE::UI

#endif
