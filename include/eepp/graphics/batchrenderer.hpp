#ifndef EE_GRAPHICSCBATCHRENDERER_H
#define EE_GRAPHICSCBATCHRENDERER_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/primitivetype.hpp>
#include <eepp/graphics/blendmode.hpp>
#include <eepp/math/polygon2.hpp>
#include <eepp/math/originpoint.hpp>

#include <eepp/system/color.hpp>
using namespace EE::System;

#include <eepp/graphics/texture.hpp>

namespace EE { namespace Graphics {

struct eeTexCoord {
	Float u;
	Float v;
};

struct eeVertex {
	Vector2f pos;
	eeTexCoord tex;
	Color color;
};

/** @brief A batch rendering class. */
class EE_API BatchRenderer {
	public:
		BatchRenderer();

		virtual ~BatchRenderer();

		/** Construct with a defined number of vertexs preallocated */
		BatchRenderer( const unsigned int& Prealloc );

		/** Allocate space for vertexs */
		void allocVertexs( const unsigned int& size );

		/** Set the current texture to render on the batch ( if you change the texture and you have batched something, this will be renderer immediately ) */
		void setTexture( const Texture * texture, Texture::CoordinateType coordinateType = Texture::CoordinateType::Normalized );

		/** Set the predefined blending function to use on the batch */
		void setBlendMode( const BlendMode& Blend );

		/** Set if every batch call have to be immediately rendered */
		void setBatchForceRendering( const bool& force ) { mForceRendering = force; }

		/** Get if the rendering is force on every batch call */
		bool getBatchForceRendering() const { return mForceRendering; }

		/** Force the batch rendering */
		void draw();

		/** Force the batch rendering only if BatchForceRendering is enable */
		void drawOpt();

		/** Set the rotation of the rendered vertex. */
		void setBatchRotation( const Float& Rotation ) { mRotation = Rotation; }

		/** Get the rotation of the rendered vertex. */
		Float getBatchRotation() const { return mRotation; }

		/** Set the scale of the rendered vertex. */
		void setBatchScale( const Vector2f& Scale ) { mScale = Scale; }

		/** Set the scale of the rendered vertex. */
		void setBatchScale( const Float& Scale ) { mScale = Vector2f( Scale, Scale ); }

		/** Get the scale of the rendered vertex. */
		Vector2f getBatchScale() const { return mScale; }

		/** The batch position */
		void setBatchPosition( const Vector2f Pos ) { mPosition = Pos; }

		/** @return The batch position */
		Vector2f getBatchPosition() const { return mPosition; }

		/** This will set a center position for rotating and scaling the batched vertex. */
		void setBatchCenter( const Vector2f Pos ) { mCenter = Pos; }

		/** @return The batch center position */
		Vector2f getBatchCenter() const { return mCenter; }

		/** Add to the batch a quad ( this will change your batch rendering method to PRIMITIVE_QUADS, so if you were using another one will Draw all the batched vertexs first ) */
		void batchQuadEx( Float x, Float y, Float width, Float height, Float angle = 0.0f, Vector2f scale = Vector2f::One, OriginPoint originPoint = OriginPoint(OriginPoint::OriginCenter) );

		/** Add to the batch a quad ( this will change your batch rendering method to PRIMITIVE_QUADS, so if you were using another one will Draw all the batched vertexs first ) */
		void batchQuad( const Float& x, const Float& y, const Float& width, const Float& height, const Float& angle = 0.0f );

		/** Add to the batch a quad with the vertex freely seted ( this will change your batch rendering method to PRIMITIVE_QUADS, so if you were using another one will Draw all the batched vertexs first ) */
		void batchQuadFree( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2, const Float& x3, const Float& y3 );

		/** Add to the batch a quad with the vertex freely seted ( this will change your batch rendering method to PRIMITIVE_QUADS, so if you were using another one will Draw all the batched vertexs first ) */
		void batchQuadFreeEx( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2, const Float& x3, const Float& y3, const Float& Angle = 0.0f, const Float& Scale = 1.0f );

		/** This will set as the default batch rendering to GL_QUADS. WIll reset the texture TexCoord rendering to the whole texture. Will reset the default color rendering to ColorA(255,255,255,255). */
		void quadsBegin();

		/** Set the texture sector to be rendered */
		void quadsSetTexCoord( const Float& tl_u, const Float& tl_v, const Float& br_u, const Float& br_v );

		/** Set the texture sector to be rendered but freely seted */
		void quadsSetTexCoordFree( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2, const Float& x3, const Float& y3 );

		/** Set the quad color */
		void quadsSetColor( const Color& Color );

		/** Set the quad color per vertex */
		void quadsSetColorFree( const Color& Color0, const Color& Color1, const Color& Color2, const Color& Color3 );

		/** This will set as the default batch rendering to PRIMITIVE_POINTS. And will reset the point color to ColorA(255,255,255,255). */
		void pointsBegin();

		/** Set the point color */
		void pointSetColor( const Color& Color );

		/** Set the texture sector to be rendered */
		void pointSetTexCoord(const Float & x, const Float & y);

		/** Add to the batch a point ( this will change your batch rendering method to PRIMITIVE_POINTS, so if you were using another one will Draw all the batched vertexs first ) */
		void batchPoint( const Float& x, const Float& y, const PrimitiveType& primitiveType = PRIMITIVE_POINTS );

		/** This will set as the default batch rendering to PRIMITIVE_LINES. And will reset the line color to ColorA(255,255,255,255). */
		void linesBegin();

		/** Set the line color */
		void linesSetColor( const Color& Color );

		/** Set the line color, per vertex */
		void linesSetColorFree( const Color& Color0, const Color& Color1 );

		/** Add to the batch a line ( this will change your batch rendering method to PRIMITIVE_LINES, so if you were using another one will Draw all the batched vertexs first ) */
		void batchLine( const Float& x0, const Float& y0, const Float& x1, const Float& y1 );

		/** This will set as the default batch rendering to GL_LINE_LOOP. And will reset the line color to ColorA(255,255,255,255). */
		void lineLoopBegin();

		/** Set the line color */
		void lineLoopSetColor( const Color& Color );

		/** Set the line color, per vertex */
		void lineLoopSetColorFree( const Color& Color0, const Color& Color1 );

		/** Add to the batch a line ( this will change your batch rendering method to PRIMITIVE_LINE_LOOP, so if you were using another one will Draw all the batched vertexs first ) */
		void batchLineLoop( const Float& x0, const Float& y0, const Float& x1, const Float& y1 );

		/** Add to the batch a point to the line loop batch ( this will change your batch rendering method to PRIMITIVE_LINE_LOOP, so if you were using another one will Draw all the batched vertexs first ) */
		void batchLineLoop( const Float& x0, const Float& y0 );

		/** Add to the batch a line ( this will change your batch rendering method to PRIMITIVE_LINE_LOOP, so if you were using another one will Draw all the batched vertexs first ) */
		void batchLineLoop( const Vector2f& vector1, const Vector2f& vector2 );

		/** Add to the batch a point to the line loop batch ( this will change your batch rendering method to PRIMITIVE_LINE_LOOP, so if you were using another one will Draw all the batched vertexs first ) */
		void batchLineLoop( const Vector2f& vector1 );

		/** This will set as the default batch rendering to PRIMITIVE_LINE_STRIP. And will reset the line color to ColorA(255,255,255,255). */
		void lineStripBegin();

		/** Set the line color */
		void lineStripSetColor( const Color& Color );

		/** Set the line color, per vertex */
		void lineStripSetColorFree( const Color& Color0, const Color& Color1 );

		/** Add to the batch a line ( this will change your batch rendering method to PRIMITIVE_LINE_STRIP, so if you were using another one will Draw all the batched vertexs first ) */
		void batchLineStrip( const Float& x0, const Float& y0, const Float& x1, const Float& y1 );

		/** Add to the batch a point to the line strip batch ( this will change your batch rendering method to PRIMITIVE_LINE_STRIP, so if you were using another one will Draw all the batched vertexs first ) */
		void batchLineStrip( const Float& x0, const Float& y0 );

		/** Add to the batch a line ( this will change your batch rendering method to PRIMITIVE_LINE_STRIP, so if you were using another one will Draw all the batched vertexs first ) */
		void batchLineStrip( const Vector2f& vector1, const Vector2f& vector2 );

		/** Add to the batch a point to the line strip batch ( this will change your batch rendering method to PRIMITIVE_LINE_STRIP, so if you were using another one will Draw all the batched vertexs first ) */
		void batchLineStrip( const Vector2f& vector1 );

		/** This will set as the default batch rendering to PRIMITIVE_TRIANGLE_FAN. And will reset the line color to ColorA(255,255,255,255). */
		void triangleFanBegin();

		/** Set the triangle fan color */
		void triangleFanSetColor( const Color& Color );

		/** Set the triangle fan color, per vertex */
		void triangleFanSetColorFree( const Color& Color0, const Color& Color1, const Color& Color2 );

		/** Set the texture sector to be rendered but freely seted */
		void triangleFanSetTexCoord( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2 );

		/** Add to the batch a triangle fan ( this will change your batch rendering method to PRIMITIVE_TRIANGLE_FAN, so if you were using another one will Draw all the batched vertexs first ) */
		void batchTriangleFan( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2 );

		/** Add to the batch a triangle fan ( this will change your batch rendering method to PRIMITIVE_TRIANGLE_FAN, so if you were using another one will Draw all the batched vertexs first ) */
		void batchTriangleFan( const Float& x0, const Float& y0 );

		/** This will set as the default batch rendering to PRIMITIVE_TRIANGLES. And will reset the line color to ColorA(255,255,255,255). */
		void trianglesBegin();

		/** Set the triangles color */
		void trianglesSetColor( const Color& Color );

		/** Set the triangles color, per vertex */
		void trianglesSetColorFree( const Color& Color0, const Color& Color1, const Color& Color2 );

		/** Set the texture sector to be rendered but freely seted */
		void trianglesSetTexCoord( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2 );

		/** Add to the batch a triangle ( this will change your batch rendering method to PRIMITIVE_TRIANGLES, so if you were using another one will Draw all the batched vertexs first ) */
		void batchTriangle( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2 );

		/** Set the polygon color */
		void polygonSetColor( const Color& Color );

		/** Add to the batch a polygon ( this will change your batch rendering method to PRIMITIVE_POLYGON, so if you were using another one will Draw all the batched vertexs first ) */
		void batchPolygon( const Polygon2f& Polygon );

		/** Set the line width */
		void setLineWidth( const Float& lineWidth );

		/** @return The current line width */
		Float getLineWidth();

		/** Set the point size */
		void setPointSize( const Float& pointSize );

		/** @return The current point size */
		Float getPointSize();

		/** Batch a poligon adding one by one vector */
		void batchPolygonByPoint( const Float& x, const Float& y );

		/** Batch a poligon adding one by one vector */
		void batchPolygonByPoint( const Vector2f& Vector );

		/** Foce the blending mode change, ignoring if it's the same that before ( so you can change the blend mode and restore it without problems ) */
		void setForceBlendModeChange( const bool& Force );

		/** @return If the blending mode switch is forced */
		const bool& getForceBlendModeChange() const;
	protected:
		eeVertex *			mVertex;
		unsigned int		mVertexSize;
		eeVertex *			mTVertex;
		unsigned int		mNumVertex;

		const Texture *		mTexture;
		BlendMode			mBlend;

		eeTexCoord			mTexCoord[4];
		Color				mVerColor[4];

		PrimitiveType		mCurrentMode;

		Float				mRotation;
		Vector2f			mScale;
		Vector2f			mPosition;
		Vector2f			mCenter;

		Texture::CoordinateType mCoordinateType;

		bool				mForceRendering;
		bool				mForceBlendMode;

		void flush();

		void init();

		void addVertexs( const unsigned int& num );

		void rotate( const Vector2f& center, Vector2f* point, const Float& angle );

		void setDrawMode( const PrimitiveType & Mode, const bool& Force );
};

}}

#endif
