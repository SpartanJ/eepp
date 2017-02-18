#ifndef EE_UICUIBORDER_HPP
#define EE_UICUIBORDER_HPP

#include <eepp/ui/base.hpp>

namespace EE { namespace UI {

class EE_API UIBorder {
	public:
		UIBorder();
		UIBorder( const UIBorder& border );

		const ColorA& getColor() const;

		void setColor( const ColorA& Col );

		const unsigned int& getWidth() const;

		void setWidth( const unsigned int& width );
	protected:
		ColorA			mColor;
		unsigned int	mWidth;
};

}}

#endif
