#ifndef EE_GRAPHICSCBATCHRENDERER_H
#define EE_GRAPHICSCBATCHRENDERER_H

#include <eepp/graphics/base.hpp>
#include <eepp/math/polygon2.hpp>

namespace EE { namespace Graphics {

struct eeTexCoord {
	eeFloat u;
	eeFloat v;
};

struct eeVertex {
	eeVector2f pos;
	eeTexCoord tex;
	eeColorA color;
};

class cTextureFactory;
class cTexture;

/** @brief A batch rendering class. */
class EE_API cBatchRenderer {
	public:
		cBatchRenderer();

		virtual ~cBatchRenderer();

		/** Construct with a defined number of vertexs preallocated */
		cBatchRenderer( const eeUint& Prealloc );

		/** Allocate space for vertexs */
		void AllocVertexs( const eeUint& size );

		/** Set the current texture to render on the batch ( if you change the texture and you have batched something, this will be renderer immediately ) */
		void SetTexture( const cTexture * Tex );

		/** Set the predefined blending function to use on the batch */
		void SetBlendMode( const EE_BLEND_MODE& Blend );

		/** Set if every batch call have to be immediately rendered */
		void BatchForceRendering( const bool& force ) { mForceRendering = force; }

		/** Get if the rendering is force on every batch call */
		bool BatchForceRendering() const { return mForceRendering; }

		/** Force the batch rendering */
		void Draw();

		/** Force the batch rendering only if BatchForceRendering is enable */
		void DrawOpt();

		/** Set the rotation of the rendered vertex. */
		void BatchRotation( const eeFloat& Rotation ) { mRotation = Rotation; }

		/** Get the rotation of the rendered vertex. */
		eeFloat BatchRotation() const { return mRotation; }

		/** Set the scale of the rendered vertex. */
		void BatchScale( const eeFloat& Scale ) { mScale = Scale; }

		/** Get the scale of the rendered vertex. */
		eeFloat BatchScale() const { return mScale; }

		/** The batch position */
		void BatchPosition( const eeVector2f Pos ) { mPosition = Pos; }

		/** @return The batch position */
		eeVector2f BatchPosition() const { return mPosition; }

		/** This will set a center position for rotating and scaling the batched vertex. */
		void BatchCenter( const eeVector2f Pos ) { mCenter = Pos; }

		/** @return The batch center position */
		eeVector2f BatchCenter() const { return mCenter; }

		/** Add to the batch a quad ( this will change your batch rendering method to DM_QUADS, so if you were using another one will Draw all the batched vertexs first ) */
		void BatchQuadEx( const eeFloat& x, const eeFloat& y, const eeFloat& width, const eeFloat& height, const eeFloat& angle = 0.0f, const eeFloat& scale = 1.0f, const bool& scalefromcenter = true );

		/** Add to the batch a quad ( this will change your batch rendering method to DM_QUADS, so if you were using another one will Draw all the batched vertexs first ) */
		void BatchQuad( const eeFloat& x, const eeFloat& y, const eeFloat& width, const eeFloat& height, const eeFloat& angle = 0.0f );

		/** Add to the batch a quad with the vertex freely seted ( this will change your batch rendering method to DM_QUADS, so if you were using another one will Draw all the batched vertexs first ) */
		void BatchQuadFree( const eeFloat& x0, const eeFloat& y0, const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2, const eeFloat& x3, const eeFloat& y3 );

		/** Add to the batch a quad with the vertex freely seted ( this will change your batch rendering method to DM_QUADS, so if you were using another one will Draw all the batched vertexs first ) */
		void BatchQuadFreeEx( const eeFloat& x0, const eeFloat& y0, const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2, const eeFloat& x3, const eeFloat& y3, const eeFloat& Angle = 0.0f, const eeFloat& Scale = 1.0f );

		/** This will set as the default batch rendering to GL_QUADS. WIll reset the texture subset rendering to the whole texture. Will reset the default color rendering to eeColorA(255,255,255,255). */
		void QuadsBegin();

		/** Set the texture sector to be rendered */
		void QuadsSetSubset( const eeFloat& tl_u, const eeFloat& tl_v, const eeFloat& br_u, const eeFloat& br_v );

		/** Set the texture sector to be rendered but freely seted */
		void QuadsSetSubsetFree( const eeFloat& x0, const eeFloat& y0, const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2, const eeFloat& x3, const eeFloat& y3 );

		/** Set the quad color */
		void QuadsSetColor( const eeColorA& Color );

		/** Set the quad color per vertex */
		void QuadsSetColorFree( const eeColorA& Color0, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3 );

		/** This will set as the default batch rendering to DM_POINTS. And will reset the point color to eeColorA(255,255,255,255). */
		void PointsBegin();

		/** Set the point color */
		void PointSetColor( const eeColorA& Color );

		/** Add to the batch a point ( this will change your batch rendering method to DM_POINTS, so if you were using another one will Draw all the batched vertexs first ) */
		void BatchPoint( const eeFloat& x, const eeFloat& y );

		/** This will set as the default batch rendering to DM_LINES. And will reset the line color to eeColorA(255,255,255,255). */
		void LinesBegin();

		/** Set the line color */
		void LinesSetColor( const eeColorA& Color );

		/** Set the line color, per vertex */
		void LinesSetColorFree( const eeColorA& Color0, const eeColorA& Color1 );

		/** Add to the batch a line ( this will change your batch rendering method to DM_LINES, so if you were using another one will Draw all the batched vertexs first ) */
		void BatchLine( const eeFloat& x0, const eeFloat& y0, const eeFloat& x1, const eeFloat& y1 );

		/** This will set as the default batch rendering to GL_LINE_LOOP. And will reset the line color to eeColorA(255,255,255,255). */
		void LineLoopBegin();

		/** Set the line color */
		void LineLoopSetColor( const eeColorA& Color );

		/** Set the line color, per vertex */
		void LineLoopSetColorFree( const eeColorA& Color0, const eeColorA& Color1 );

		/** Add to the batch a line ( this will change your batch rendering method to DM_LINE_LOOP, so if you were using another one will Draw all the batched vertexs first ) */
		void BatchLineLoop( const eeFloat& x0, const eeFloat& y0, const eeFloat& x1, const eeFloat& y1 );

		/** Add to the batch a point to the line loop batch ( this will change your batch rendering method to DM_LINE_LOOP, so if you were using another one will Draw all the batched vertexs first ) */
		void BatchLineLoop( const eeFloat& x0, const eeFloat& y0 );

		/** Add to the batch a line ( this will change your batch rendering method to DM_LINE_LOOP, so if you were using another one will Draw all the batched vertexs first ) */
		void BatchLineLoop( const eeVector2f& vector1, const eeVector2f& vector2 );

		/** Add to the batch a point to the line loop batch ( this will change your batch rendering method to DM_LINE_LOOP, so if you were using another one will Draw all the batched vertexs first ) */
		void BatchLineLoop( const eeVector2f& vector1 );

		/** This will set as the default batch rendering to DM_TRIANGLE_FAN. And will reset the line color to eeColorA(255,255,255,255). */
		void TriangleFanBegin();

		/** Set the triangle fan color */
		void TriangleFanSetColor( const eeColorA& Color );

		/** Set the triangle fan color, per vertex */
		void TriangleFanSetColorFree( const eeColorA& Color0, const eeColorA& Color1, const eeColorA& Color2 );

		/** Set the texture sector to be rendered but freely seted */
		void TriangleFanSetSubset( const eeFloat& x0, const eeFloat& y0, const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2 );

		/** Add to the batch a triangle fan ( this will change your batch rendering method to DM_TRIANGLE_FAN, so if you were using another one will Draw all the batched vertexs first ) */
		void BatchTriangleFan( const eeFloat& x0, const eeFloat& y0, const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2 );

		/** Add to the batch a triangle fan ( this will change your batch rendering method to DM_TRIANGLE_FAN, so if you were using another one will Draw all the batched vertexs first ) */
		void BatchTriangleFan( const eeFloat& x0, const eeFloat& y0 );

		/** This will set as the default batch rendering to DM_TRIANGLES. And will reset the line color to eeColorA(255,255,255,255). */
		void TrianglesBegin();

		/** Set the triangles color */
		void TrianglesSetColor( const eeColorA& Color );

		/** Set the triangles color, per vertex */
		void TrianglesSetColorFree( const eeColorA& Color0, const eeColorA& Color1, const eeColorA& Color2 );

		/** Set the texture sector to be rendered but freely seted */
		void TrianglesSetSubset( const eeFloat& x0, const eeFloat& y0, const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2 );

		/** Add to the batch a triangle ( this will change your batch rendering method to DM_TRIANGLES, so if you were using another one will Draw all the batched vertexs first ) */
		void BatchTriangle( const eeFloat& x0, const eeFloat& y0, const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2 );

		/** Set the polygon color */
		void PolygonSetColor( const eeColorA& Color );

		/** Add to the batch a polygon ( this will change your batch rendering method to DM_POLYGON, so if you were using another one will Draw all the batched vertexs first ) */
		void BatchPolygon( const eePolygon2f& Polygon );

		/** Set the line width */
		void SetLineWidth( const eeFloat& lineWidth );

		/** @return The current line width */
		eeFloat GetLineWidth();

		/** Set the point size */
		void SetPointSize( const eeFloat& pointSize );

		/** @return The current point size */
		eeFloat GetPointSize();

		/** Batch a poligon adding one by one vector */
		void BatchPolygonByPoint( const eeFloat& x, const eeFloat& y );

		/** Batch a poligon adding one by one vector */
		void BatchPolygonByPoint( const eeVector2f& Vector );

		/** Foce the blending mode change, ignoring if it's the same that before ( so you can change the blend mode and restore it without problems ) */
		void ForceBlendModeChange( const bool& Force );

		/** @return If the blending mode switch is forced */
		const bool& ForceBlendModeChange() const;
	protected:
		eeVertex *			mVertex;
		eeUint				mVertexSize;
		eeVertex *			mTVertex;
		eeUint				mNumVertex;

		const cTexture *	mTexture;
		cTextureFactory *	mTF;
		EE_BLEND_MODE	mBlend;

		eeTexCoord			mTexCoord[4];
		eeColorA			mVerColor[4];

		EE_DRAW_MODE		mCurrentMode;

		eeFloat				mRotation;
		eeFloat				mScale;
		eeVector2f			mPosition;
		eeVector2f			mCenter;

		bool				mForceRendering;
		bool				mForceBlendMode;

		void Flush();
		void Init();
		void AddVertexs( const eeUint& num );
		void Rotate( const eeVector2f& center, eeVector2f* point, const eeFloat& angle );
		void SetBlendMode( EE_DRAW_MODE Mode, const bool& Force );
};

}}

#endif
