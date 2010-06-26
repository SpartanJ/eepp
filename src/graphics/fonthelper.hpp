#ifndef EE_GRAPHICSFONTHELPER_HPP
#define EE_GRAPHICSFONTHELPER_HPP

namespace EE { namespace Graphics {

#define FONT_DRAW_LEFT			(0 << 0)
#define FONT_DRAW_RIGHT			(1 << 0)
#define FONT_DRAW_CENTER		(2 << 0)
#define FONT_DRAW_HALIGN_MASK	(3 << 0)

inline Uint32 FontHAlignGet( Uint32 Flags ) {
	return Flags & FONT_DRAW_HALIGN_MASK;
}

#define FONT_DRAW_TOP			(0 << 2)
#define FONT_DRAW_BOTTOM		(1 << 2)
#define FONT_DRAW_MIDDLE		(2 << 2)
#define FONT_DRAW_VALIGN_MASK	(3 << 2)

inline Uint32 FontVAlignGet( Uint32 Flags ) {
	return Flags & FONT_DRAW_VALIGN_MASK;
}

#define FONT_DRAW_SHADOW		(1 << 5)
#define FONT_DRAW_ALIGN_MASK	( FONT_DRAW_VALIGN_MASK | FONT_DRAW_HALIGN_MASK )


/** Basic Glyph structure used by the engine */
typedef struct {
	Int32 MinX, MaxX, MinY, MaxY, Advance;
	Uint16 CurX, CurY, CurW, CurH, GlyphH;
} eeGlyph;

typedef struct {
	eeFloat TexCoords[2];
	eeFloat Vertex[2];
} eeVertexCoords;

typedef struct {
	eeFloat TexCoords[8];
	eeFloat Vertex[8];
} eeTexCoords;

}}

#endif
