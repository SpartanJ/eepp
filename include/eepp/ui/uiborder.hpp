#ifndef EE_UICUIBORDER_HPP
#define EE_UICUIBORDER_HPP

#include <eepp/ui/base.hpp>

namespace EE { namespace UI {

class EE_API UIBorder {
	public:
		static UIBorder * New();

		UIBorder();

		const ColorA& getColor() const;

		UIBorder * setColor( const ColorA& Col );

		const unsigned int& getWidth() const;

		UIBorder * setWidth( const unsigned int& width );
	protected:
		ColorA			mColor;
		unsigned int	mWidth;
};

}}

#endif
