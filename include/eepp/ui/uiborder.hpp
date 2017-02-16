#ifndef EE_UICUIBORDER_HPP
#define EE_UICUIBORDER_HPP

#include <eepp/ui/base.hpp>

namespace EE { namespace UI {

class EE_API UIBorder {
	public:
		UIBorder();
		UIBorder( const UIBorder& border );

		const ColorA& color() const;
		void color( const ColorA& Col );

		const unsigned int& width() const;
		void width( const unsigned int& width );
	protected:
		ColorA		mColor;
		unsigned int			mWidth;
};

}}

#endif
