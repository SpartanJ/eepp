#include "cvertexbuffer.hpp"
#include "cvertexbufferogl.hpp"
#include "cvertexbuffervbo.hpp"
#include "glhelper.hpp"

using namespace EE::Graphics::Private;

namespace EE { namespace Graphics {

cVertexBuffer * cVertexBuffer::Create( const Uint32& VertexFlags, EE_DRAW_MODE DrawType, const Int32& ReserveVertexSize, const Int32& ReserveIndexSize, EE_VBO_USAGE_TYPE UsageType ) {
	if ( cGL::instance()->IsExtension( GLEW_ARB_vertex_buffer_object ) )
		return eeNew( cVertexBufferVBO, ( VertexFlags, DrawType, ReserveVertexSize, ReserveIndexSize, UsageType ) );

	return eeNew( cVertexBufferOGL, ( VertexFlags, DrawType, ReserveVertexSize, ReserveIndexSize, UsageType ) );
}

cVertexBuffer::cVertexBuffer( const Uint32& VertexFlags, EE_DRAW_MODE DrawType, const Int32& ReserveVertexSize, const Int32& ReserveIndexSize, EE_VBO_USAGE_TYPE UsageType ) :
	mVertexFlags( VertexFlags ),
	mDrawType( DrawType ),
	mUsageType( UsageType ),
	mElemDraw(-1)
{
		if( ReserveVertexSize > 0 ) {
			for( Int32 i = 0; i < VERTEX_FLAGS_COUNT; i++ ) {
				if( VERTEX_FLAG_QUERY( mVertexFlags, i ) ) {
					if ( i != VERTEX_FLAG_COLOR )
						mVertexArray[ i ].reserve( ReserveVertexSize * eeVertexElements[ i ] );
					else
						mColorArray.reserve( ReserveVertexSize * eeVertexElements[ i ] );
				}
			}
		}
}

cVertexBuffer::~cVertexBuffer() {
		for( Int32 i = 0; i < VERTEX_FLAGS_COUNT_ARR; i++ )
			mVertexArray[ i ].clear();

		mColorArray.clear();
		mIndexArray.clear();
}

void cVertexBuffer::AddVertex( const Uint32& Type, const eeVector2f& Vertex ) {
	if ( Type < VERTEX_FLAGS_COUNT_ARR ) {
		mVertexArray[ Type ].push_back( Vertex.x );
		mVertexArray[ Type ].push_back( Vertex.y );
	}
}

void cVertexBuffer::AddVertex( const eeVector2f& Vertex ) {
	AddVertex( VERTEX_FLAG_POSITION, Vertex );
}

void cVertexBuffer::AddVertexCoord( const eeVector2f& VertexCoord, const Uint32& TextureLevel ) {
	AddVertex( VERTEX_FLAG_TEXTURE0 + TextureLevel, VertexCoord );
}

void cVertexBuffer::AddColor( const eeColorA& Color ) {
	mColorArray.push_back( Color.R() );
	mColorArray.push_back( Color.G() );
	mColorArray.push_back( Color.B() );
	mColorArray.push_back( Color.A() );
}

void cVertexBuffer::AddIndex( const Uint32& Index ) {
	mIndexArray.push_back( Index );

	VERTEX_FLAG_SET( mVertexFlags, VERTEX_FLAG_USE_INDICES );
}

void cVertexBuffer::ResizeArray( const Uint32& Type, const Uint32& Size ) {
	if ( Type != VERTEX_FLAG_COLOR )
		mVertexArray[ Type ].resize( Size );
	else
		mColorArray.resize( Size );
}

void cVertexBuffer::ResizeIndices( const Uint32& Size ) {
	mIndexArray.resize( Size );
}

eeFloat * cVertexBuffer::GetArray( const Uint32& Type ) {
	if ( Type && Type < VERTEX_FLAGS_COUNT_ARR && mVertexArray[ Type ].size() )
		return &mVertexArray[ Type - 1 ][0];

	return NULL;
}

Uint8 * cVertexBuffer::GetColorArray() {
	if ( mColorArray.size() )
		return &mColorArray[0];

	return NULL;
}

Uint32 * cVertexBuffer::GetIndices() {
	if ( mIndexArray.size() )
		return &mIndexArray[0];

	return NULL;
}

Uint32 cVertexBuffer::GetVertexCount() {
	return (Uint32)mVertexArray[ VERTEX_FLAG_POSITION ].size() / eeVertexElements[ VERTEX_FLAG_POSITION ];
}

Uint32 cVertexBuffer::GetIndexCount() {
	return (Uint32)mIndexArray.size();
}

eeVector2f cVertexBuffer::GetVector2( const Uint32& Type, const Uint32& Index ) {
	/// assert
	if( Type < VERTEX_FLAGS_COUNT_ARR && !VERTEX_FLAG_QUERY( mVertexFlags, Type ) )
		return eeVector2f(0.f,0.f);

	Int32 pos = Index * eeVertexElements[ Type ];

	return eeVector2f( mVertexArray[ Type ][ pos ], mVertexArray[ Type ][ pos + 1 ] );
}

eeColorA cVertexBuffer::GetColor( const Uint32& Index ) {
	/// assert
	if( !VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_COLOR ) )
		return eeColorA();

	Int32 pos = Index * eeVertexElements[ VERTEX_FLAG_COLOR ];

	return eeColorA( mColorArray[ pos ], mColorArray[ pos + 1 ], mColorArray[ pos + 2 ], mColorArray[ pos + 3 ] );
}

Uint32 cVertexBuffer::GetIndex( const Uint32& Index ) {
	/// assert
	return mIndexArray[ Index ];
}

void cVertexBuffer::SetElementNum( Int32 Num ) {
	mElemDraw = Num;
}

const Int32& cVertexBuffer::GetElementNum() const {
	return mElemDraw;
}

void cVertexBuffer::Unbind() {
	if( !VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_POSITION ) ) {
		//glEnableClientState( GL_VERTEX_ARRAY );
	}

	if( !VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_COLOR ) ) {
		//glEnableClientState( GL_COLOR_ARRAY );
	}

	if( !VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_TEXTURE0 ) ) {
		//glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glEnable( GL_TEXTURE_2D );
	}
}

}}
