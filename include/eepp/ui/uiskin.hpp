#ifndef EE_UICUISKIN_HPP
#define EE_UICUISKIN_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uiskinstate.hpp>
#include <eepp/graphics/statelistdrawable.hpp>

namespace EE { namespace UI {

class UITheme;

class EE_API UISkin : public StateListDrawable {
	public:
		static const char * getSkinStateName( const Uint32& State );

		static int getStateNumber(const std::string & State);

		static bool isStateName( const std::string& State );

		static UISkin * New( const std::string& name );

		explicit UISkin( const std::string& name );

		virtual ~UISkin();

		virtual Sizef getSize( const Uint32& state );

		virtual Sizef getSize();

		const std::string& getName() const;

		void setName( const std::string& name );

		const Uint32& getId() const;

		UITheme * getTheme() const;

		void setTheme( UITheme * theme );

		virtual UISkin * clone();

		virtual UISkin * clone( const std::string& NewName );

		virtual Rectf getBorderSize( const Uint32 & state );

		virtual Rectf getBorderSize();
	protected:
		friend class UISkinState;

		std::string mName;
		Uint32		mNameHash;
		UITheme * 	mTheme;
};

}}

#endif
