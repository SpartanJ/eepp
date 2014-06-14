#ifndef EE_UICUIBORDER_HPP
#define EE_UICUIBORDER_HPP

#include <eepp/ui/base.hpp>

namespace EE { namespace UI {

class EE_API cUIBorder {
	public:
		cUIBorder();
		cUIBorder( const cUIBorder& border );

		const eeColorA& Color() const;
		void Color( const eeColorA& Col );

		const unsigned int& Width() const;
		void Width( const unsigned int& width );
	protected:
		eeColorA		mColor;
		unsigned int			mWidth;
};

}}

#endif
