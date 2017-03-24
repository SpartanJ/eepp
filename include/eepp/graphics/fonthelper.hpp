#ifndef EE_GRAPHICSFONTHELPER_HPP
#define EE_GRAPHICSFONTHELPER_HPP

namespace EE { namespace Graphics {

enum EE_FONT_TYPE {
	FONT_TYPE_TTF = 1,
	FONT_TYPE_TEX = 2
};

enum EE_FONT_HALIGN {
	TEXT_ALIGN_LEFT		= (0 << 0),
	TEXT_ALIGN_RIGHT	= (1 << 0),
	TEXT_ALIGN_CENTER	= (2 << 0),
	TEXT_HALIGN_MASK	= (3 << 0)
};

enum EE_FONT_VALIGN {
	TEXT_ALIGN_TOP		= (0 << 2),
	TEXT_ALIGN_BOTTOM	= (1 << 2),
	TEXT_ALIGN_MIDDLE	= (2 << 2),
	TEXT_VALIGN_MASK	= (3 << 2)
};

inline Uint32 fontHAlignGet( Uint32 Flags ) {
	return Flags & TEXT_HALIGN_MASK;
}

inline Uint32 fontVAlignGet( Uint32 Flags ) {
	return Flags & TEXT_VALIGN_MASK;
}

#define TEXT_ALIGN_MASK	( TEXT_VALIGN_MASK | TEXT_HALIGN_MASK )

#define EE_TTF_FONT_MAGIC ( ( 'E' << 0 ) | ( 'E' << 8 ) | ( 'F' << 16 ) | ( 'N' << 24 ) )

}}

#endif
