#ifndef EE_UICUIBORDER_HPP
#define EE_UICUIBORDER_HPP

#include "base.hpp"

namespace EE { namespace UI {

class cUIBorder {
	public:
		cUIBorder();
		cUIBorder( const cUIBorder& border );

		const eeColorA& Color() const;
		void Color( const eeColorA& Col );

		const eeUint& Width() const;
		void Width( const eeUint& width );
	protected:
		eeColorA		mColor;
		eeUint			mWidth;
};

}}

#endif
