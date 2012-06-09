#ifndef EE_GRAPHICSCVERTEXBUFFER_HPP
#define EE_GRAPHICSCVERTEXBUFFER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/vbohelper.hpp>

namespace EE { namespace Graphics {

class EE_API cVertexBuffer {
	public:
		static cVertexBuffer * Create( const Uint32& VertexFlags = VERTEX_FLAGS_DEFAULT, EE_DRAW_MODE DrawType = DM_QUADS, const Int32& ReserveVertexSize = 0, const Int32& ReserveIndexSize = 0, EE_VBO_USAGE_TYPE UsageType = VBO_USAGE_TYPE_STATIC );

		cVertexBuffer( const Uint32& VertexFlags = VERTEX_FLAGS_DEFAULT, EE_DRAW_MODE DrawType = DM_QUADS, const Int32& ReserveVertexSize = 0, const Int32& ReserveIndexSize = 0, EE_VBO_USAGE_TYPE UsageType = VBO_USAGE_TYPE_STATIC );

		virtual ~cVertexBuffer();

		void AddVertex( const Uint32& Type, const eeVector2f& Vertex );

		void AddVertex( const eeVector2f& Vertex );

		void AddVertexCoord( const eeVector2f& VertexCoord, const Uint32& TextureLevel = 0 );

		void AddColor( const eeColorA& Color );

		void AddIndex( const Uint32& Index );

		void ResizeArray( const Uint32& Type, const Uint32& Size );

		void ResizeIndices( const Uint32& Size );

		eeFloat * GetArray( const Uint32& Type );

		Uint8 * GetColorArray();

		Uint32 * GetIndices();

		Uint32 GetVertexCount();

		Uint32 GetIndexCount();

		eeVector2f GetVector2( const Uint32& Type, const Uint32& Index );

		eeColorA GetColor( const Uint32& Index );

		Uint32 GetIndex( const Uint32& Index );

		void SetElementNum( Int32 Num );

		const Int32& GetElementNum() const;

		virtual void Bind() = 0;

		virtual void Unbind() = 0;

		virtual void Draw() = 0;

		virtual bool Compile() = 0;

		virtual void Update( const Uint32& Types, bool Indices ) = 0;

		virtual void Reload() = 0;
	protected:
		Uint32 					mVertexFlags;
		EE_DRAW_MODE			mDrawType;
		EE_VBO_USAGE_TYPE		mUsageType;
		Int32					mElemDraw;
		std::vector<eeFloat>	mVertexArray[ VERTEX_FLAGS_COUNT - 1 ];
		std::vector<Uint8>		mColorArray;
		std::vector<Uint32>		mIndexArray;

		virtual void SetVertexStates() = 0;
};

}}

#endif
