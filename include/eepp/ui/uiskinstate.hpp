#ifndef EE_UI_UISKINSTATE_HPP
#define EE_UI_UISKINSTATE_HPP

#include <eepp/ui/uistate.hpp>

namespace EE { namespace UI {

class UISkin;

class EE_API UISkinState : public UIState {
  public:
	static UISkinState* New( UISkin* skin );

	explicit UISkinState( UISkin* Skin );

	virtual ~UISkinState();

	UISkin* getSkin() const;

	void draw( const Float& X, const Float& Y, const Float& Width, const Float& Height,
			   const Uint32& Alpha );

	bool stateExists( const Uint32& State ) const;

	void setStateColor( const Uint32& state, const Color& color );

	Color getStateColor( const Uint32& state ) const;

	bool hasStateColor( const Uint32& state ) const;

  protected:
	UISkin* mSkin;
	std::map<Uint32, Color> mColors;
	Color mCurrentColor;

	void updateState();

	void onStateChange();
};

}} // namespace EE::UI

#endif
