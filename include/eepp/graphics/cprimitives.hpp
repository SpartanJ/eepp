#ifndef EE_GRAPHICSCPRIMITIVES_H
#define EE_GRAPHICSCPRIMITIVES_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/ctexturefactory.hpp>
#include <eepp/graphics/cbatchrenderer.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>

namespace EE { namespace Graphics {

/** @brief Basic primitives rendering class */
class EE_API cPrimitives {
	public:
		cPrimitives();

		~cPrimitives();

		/** Draw a Line on screen
		* @param x First point x axis
		* @param y First point y axis
		* @param x2 Second Point x axis
		* @param y2 Second Point y axis
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawLine(const eeFloat& x, const eeFloat& y, const eeFloat& x2, const eeFloat& y2, const eeFloat& lineWidth = 1.0f);

		/** Draw a Line on screen
		* @param p1 First Point
		* @param p2 Second Point
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawLine(const eeVector2f& p1, const eeVector2f& p2, const eeFloat& lineWidth = 1.0f);

		/** Draw a point on the screen
		* @param x Point x axis
		* @param y Point y axis
		* @param pointSize Point Size (default 1.0f )
		*/
		void DrawPoint(const eeFloat& x, const eeFloat& y, const eeFloat& pointSize = 1.0f);

		/** Draw a point on the screen
		* @param p The coordinates
		* @param pointSize Point Size (default 1.0f )
		*/
		void DrawPoint(const eeVector2f& p, const eeFloat& pointSize = 1.0f);

		/** Draw a circle on the screen
		* @param p The coordinates ( x and y represents the center of the circle )
		* @param radius The Circle Radius
		* @param points Number of points to represent the circle
		* @param fillmode Draw filled or only lines
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawCircle(const eeVector2f& p, const eeFloat& radius, Uint32 points = 360, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const eeFloat& lineWidth = 1.0f);

		/** Draw a circle on the screen
		* @param x Point x axis
		* @param y Point y axis
		* @param radius The Circle Radius
		* @param points Number of points to represent the circle
		* @param fillmode Draw filled or only lines
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawCircle(const eeFloat& x, const eeFloat& y, const eeFloat& radius, Uint32 points = 360, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const eeFloat& lineWidth = 1.0f);

		/** Draw a triangle on the screen
		* @param x1 First Point x axis
		* @param y1 First Point y axis
		* @param x2 Second Point x axis
		* @param y2 Second Point y axis
		* @param x3 Third Point x axis
		* @param y3 Third Point y axis
		* @param Color1 First Point Color
		* @param Color2 Second Point Color
		* @param Color3 Third Point Color
		* @param fillmode Draw filled or only lines
		* @param blend The Blend Mode
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawTriangle(const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2, const eeFloat& x3, const eeFloat& y3, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f);

		/** Draw a triangle on the screen
		* @param x1 First Point x axis
		* @param y1 First Point y axis
		* @param x2 Second Point x axis
		* @param y2 Second Point y axis
		* @param x3 Third Point x axis
		* @param y3 Third Point y axis
		* @param fillmode Draw filled or only lines
		* @param blend The Blend Mode
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawTriangle(const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2, const eeFloat& x3, const eeFloat& y3, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f);

		/** Draw a triangle on the screen
		* @param p1 First Point axis
		* @param p2 Second Point axis
		* @param p3 Third Point axis
		* @param Color1 First Point Color
		* @param Color2 Second Point Color
		* @param Color3 Third Point Color
		* @param fillmode Draw filled or only lines
		* @param blend The Blend Mode
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawTriangle(const eeVector2f& p1, const eeVector2f& p2, const eeVector2f& p3, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f);

		/** Draw a triangle on the screen
		* @param p1 First Point axis
		* @param p2 Second Point axis
		* @param p3 Third Point axis
		* @param fillmode Draw filled or only lines
		* @param blend The Blend Mode
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawTriangle(const eeVector2f& p1, const eeVector2f& p2, const eeVector2f& p3, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f);

		/** Draw a triangle on the screen
		* @param t The Triangle (eeTriangle2f)
		* @param fillmode Draw filled or only lines
		* @param blend The Blend Mode
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawTriangle(const eeTriangle2f& t, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f);

		/** Draw a triangle on the screen
		* @param t The Triangle (eeTriangle2f)
		* @param Color1 First Point Color
		* @param Color2 Second Point Color
		* @param Color3 Third Point Color
		* @param fillmode Draw filled or only lines
		* @param blend The Blend Mode
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawTriangle(const eeTriangle2f& t, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f);

		/** Draw a rectangle on the screen
		* @param x Screen x axis
		* @param y Screen y axis
		* @param width Rectangle Width
		* @param height Rectangle Height
		* @param Angle Rectangle Angle
		* @param Scale Rectangle Scale ( default 1.0f )
		* @param fillmode Draw filled or only lines
		* @param blend The Blend Mode
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawRectangle(const eeFloat& x, const eeFloat& y, const eeFloat& width, const eeFloat& height, const eeFloat& Angle = 0, const eeFloat& Scale = 1, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f, const eeUint& Corners = 0 );

		void DrawRoundedRectangle(const eeFloat& x, const eeFloat& y, const eeFloat& width, const eeFloat& height, const eeFloat& Angle = 0, const eeFloat& Scale = 1, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f, const eeUint& Corners = 8 );

		/** Draw a rectangle on the screen
		* @param x Screen x axis
		* @param y Screen y axis
		* @param width Rectangle Width
		* @param height Rectangle Height
		* @param TopLeft The Top Left Rectangle Color
		* @param BottomLeft The Bottom Left Rectangle Color
		* @param BottomRight The Bottom Right Rectangle Color
		* @param TopRight The Top Right Rectangle Color
		* @param Angle Rectangle Angle
		* @param Scale Rectangle Scale ( default 1.0f )
		* @param fillmode Draw filled or only lines
		* @param blend The Blend Mode
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawRectangle(const eeFloat& x, const eeFloat& y, const eeFloat& width, const eeFloat& height, const eeColorA& TopLeft, const eeColorA& BottomLeft, const eeColorA& BottomRight, const eeColorA& TopRight, const eeFloat& Angle = 0, const eeFloat& Scale = 1, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f, const eeUint& Corners = 0 );

		void DrawRoundedRectangle(const eeFloat& x, const eeFloat& y, const eeFloat& width, const eeFloat& height, const eeColorA& TopLeft, const eeColorA& BottomLeft, const eeColorA& BottomRight, const eeColorA& TopRight, const eeFloat& Angle = 0, const eeFloat& Scale = 1, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f, const eeUint& Corners = 8 );

		/** Draw a rectangle on the screen
		* @param R The Rectangle eeRectf
		* @param Angle Rectangle Angle
		* @param Scale Rectangle Scale ( default 1.0f )
		* @param fillmode Draw filled or only lines
		* @param blend The Blend Mode
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawRectangle( const eeRectf& R, const eeFloat& Angle = 0, const eeFloat& Scale = 1, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f, const eeUint& Corners = 0 );

		void DrawRoundedRectangle( const eeRectf& R, const eeFloat& Angle = 0, const eeFloat& Scale = 1, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f, const eeUint& Corners = 8 );

		/** Draw a rectangle on the screen
		* @param R The Rectangle eeRectf
		* @param TopLeft The Top Left Rectangle Color
		* @param BottomLeft The Bottom Left Rectangle Color
		* @param BottomRight The Bottom Right Rectangle Color
		* @param TopRight The Top Right Rectangle Color
		* @param Angle Rectangle Angle
		* @param Scale Rectangle Scale ( default 1.0f )
		* @param fillmode Draw filled or only lines
		* @param blend The Blend Mode
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawRectangle( const eeRectf& R, const eeColorA& TopLeft, const eeColorA& BottomLeft, const eeColorA& BottomRight, const eeColorA& TopRight, const eeFloat& Angle = 0, const eeFloat& Scale = 1, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f, const eeUint& Corners = 0 );

		void DrawRoundedRectangle( const eeRectf& R, const eeColorA& TopLeft, const eeColorA& BottomLeft, const eeColorA& BottomRight, const eeColorA& TopRight, const eeFloat& Angle = 0, const eeFloat& Scale = 1, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f, const eeUint& Corners = 8 );

		/** Draw a four edges polygon on screen
		* @param x1 First Point x axis
		* @param y1 First Point y axis
		* @param x2 Second Point x axis
		* @param y2 Second Point y axis
		* @param x3 Third Point x axis
		* @param y3 Third Point y axis
		* @param x4 Fourth Point x axis
		* @param y4 Fourth Point y axis
		* @param Color1 First Point Color
		* @param Color2 Second Point Color
		* @param Color3 Third Point Color
		* @param Color4 Fourth Point Color
		* @param fillmode Draw filled or only lines
		* @param blend The Blend Mode
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawQuad(const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2, const eeFloat& x3, const eeFloat& y3, const eeFloat& x4, const eeFloat& y4, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const eeColorA& Color4, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f, const eeFloat& OffsetX = 0, const eeFloat& OffsetY = 0);

		/** Draw a four edges polygon on screen
		* @param x1 First Point x axis
		* @param y1 First Point y axis
		* @param x2 Second Point x axis
		* @param y2 Second Point y axis
		* @param x3 Third Point x axis
		* @param y3 Third Point y axis
		* @param x4 Fourth Point x axis
		* @param y4 Fourth Point y axis
		* @param fillmode Draw filled or only lines
		* @param blend The Blend Mode
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawQuad(const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2, const eeFloat& x3, const eeFloat& y3, const eeFloat& x4, const eeFloat& y4, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f, const eeFloat& OffsetX = 0, const eeFloat& OffsetY = 0);

		/** Draw a four edges polygon on screen
		* @param p1 First Point
		* @param p2 Second Point
		* @param p3 Third Point
		* @param p4 Fourth Point
		* @param Color1 First Point Color
		* @param Color2 Second Point Color
		* @param Color3 Third Point Color
		* @param Color4 Fourth Point Color
		* @param fillmode Draw filled or only lines
		* @param blend The Blend Mode
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawQuad(const eeVector2f& p1, const eeVector2f& p2, const eeVector2f& p3, const eeVector2f& p4, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const eeColorA& Color4, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f, const eeFloat& OffsetX = 0, const eeFloat& OffsetY = 0);

		/** Draw a four edges polygon on screen
		* @param p1 First Point
		* @param p2 Second Point
		* @param p3 Third Point
		* @param p4 Fourth Point
		* @param fillmode Draw filled or only lines
		* @param blend The Blend Mode
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawQuad(const eeVector2f& p1, const eeVector2f& p2, const eeVector2f& p3, const eeVector2f& p4, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f, const eeFloat& OffsetX = 0, const eeFloat& OffsetY = 0);

		/** Draw a four edges polygon on screen
		* @param q The Quad
		* @param fillmode Draw filled or only lines
		* @param blend The Blend Mode
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawQuad(const eeQuad2f& q, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f, const eeFloat& OffsetX = 0, const eeFloat& OffsetY = 0);

		/** Draw a polygon on screen
		* @param p The Polygon
		* @param fillmode Draw filled or only lines
		* @param blend The Blend Mode
		* @param lineWidth The line width ( default 1.0f )
		*/
		void DrawPolygon(const eePolygon2f& p, const EE_FILL_MODE& fillmode = EE_DRAW_FILL, const EE_BLEND_MODE& blend = ALPHA_NORMAL, const eeFloat& lineWidth = 1.0f);

		/** Set the current color for drawing primitives */
		void SetColor( const eeColorA& Color );

		/** Forcing the draw, will force the batch renderer to draw the batched vertexs immediately ( active by default ). */
		void ForceDraw( const bool& force );

		const bool& ForceDraw() const;

		/** Force to draw the batched vertexs. */
		void DrawBatch();
	private:
		eeColorA				mColor;
		cGlobalBatchRenderer *	mBR;
		bool					mForceDraw;
};

}}

#endif
