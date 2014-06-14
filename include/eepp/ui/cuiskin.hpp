#ifndef EE_UICUISKIN_HPP
#define EE_UICUISKIN_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/cuiskinstate.hpp>

namespace EE { namespace Graphics {
class cSubTexture;
}}

namespace EE { namespace UI {

class cUITheme;

class EE_API cUISkin {
	public:
		enum UISkinType {
			UISkinSimple,
			UISkinComplex,
			UISkinTypeCount
		};

		static const char * GetSkinStateName( const Uint32& State );

		cUISkin( const std::string& Name, const Uint32& Type );

		virtual ~cUISkin();

		virtual void Draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha, const Uint32& State ) = 0;

		virtual void SetSkin( const Uint32& State ) = 0;

		virtual cSubTexture * GetSubTexture( const Uint32& State ) const = 0;

		virtual void SetColor( const Uint32& State, const ColorA& Color );

		virtual const ColorA& GetColor( const Uint32& State ) const;

		virtual void SetSkins();

		const std::string& Name() const;

		void Name( const std::string& name );

		const Uint32& Id() const;

		cUITheme * Theme() const;

		void Theme( cUITheme * theme );

		virtual cUISkin * Copy() = 0;

		const Uint32& GetType() const;
	protected:
		friend class cUIControl;
		friend class cUISkinState;

		Uint32		mType;
		std::string mName;
		Uint32		mNameHash;
		Uint32		mColorDefault;
		ColorA 	mColor[ cUISkinState::StateCount ];
		cUITheme * 	mTheme;

		void StateBack( const Uint32& State );

		void SetPrevState();

		bool GetColorDefault( const Uint32& State );

		virtual void StateNormalToState( const Uint32& State ) = 0;
};

}}

#endif
