#ifndef EE_UICUISKIN_HPP
#define EE_UICUISKIN_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uiskinstate.hpp>

namespace EE { namespace Graphics {
class SubTexture;
}}

namespace EE { namespace UI {

class UITheme;

class EE_API UISkin {
	public:
		enum UISkinType {
			SkinSimple,
			SkinComplex,
			SkinTypeCount
		};

		static const char * GetSkinStateName( const Uint32& State );

		UISkin( const std::string& Name, const Uint32& Type );

		virtual ~UISkin();

		virtual void Draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha, const Uint32& State ) = 0;

		virtual void SetSkin( const Uint32& State ) = 0;

		virtual SubTexture * GetSubTexture( const Uint32& State ) const = 0;

		virtual void SetColor( const Uint32& State, const ColorA& Color );

		virtual const ColorA& GetColor( const Uint32& State ) const;

		virtual void SetSkins();

		const std::string& Name() const;

		void Name( const std::string& name );

		const Uint32& Id() const;

		UITheme * Theme() const;

		void Theme( UITheme * theme );

		virtual UISkin * Copy() = 0;

		const Uint32& GetType() const;
	protected:
		friend class UIControl;
		friend class UISkinState;

		Uint32		mType;
		std::string mName;
		Uint32		mNameHash;
		Uint32		mColorDefault;
		ColorA 	mColor[ UISkinState::StateCount ];
		UITheme * 	mTheme;

		void StateBack( const Uint32& State );

		void SetPrevState();

		bool GetColorDefault( const Uint32& State );

		virtual void StateNormalToState( const Uint32& State ) = 0;
};

}}

#endif
