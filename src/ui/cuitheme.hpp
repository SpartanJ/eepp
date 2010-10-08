#ifndef EE_UICUITHEME_HPP
#define EE_UICUITHEME_HPP

#include "base.hpp"
#include "cuiskin.hpp"

namespace EE { namespace UI {

class EE_API cUITheme : public tResourceManager<cUISkin> {
	public:
		cUITheme( const std::string& Name, const std::string& Abbr );

		virtual ~cUITheme();

		const std::string& Name() const;

		void Name( const std::string& name );

		const Uint32& Id() const;

		const std::string& Abbr() const;
	protected:
		std::string 		mName;
		Uint32				mNameHash;

		std::string			mAbbr;
};

}}

#endif
