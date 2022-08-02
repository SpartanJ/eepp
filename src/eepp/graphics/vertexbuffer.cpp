#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/vertexbuffer.hpp>
#include <eepp/graphics/vertexbuffermanager.hpp>
#include <eepp/graphics/vertexbufferogl.hpp>
#include <eepp/graphics/vertexbuffervbo.hpp>
using namespace EE::Graphics::Private;

namespace EE { namespace Graphics {

VertexBuffer* VertexBuffer::New( const Uint32& vertexFlags, PrimitiveType drawType,
								 const Int32& reserveVertexSize, const Int32& reserveIndexSize,
								 VertexBufferUsageType usageType ) {
	if ( GLi->isExtension( EEGL_ARB_vertex_buffer_object ) )
		return eeNew( VertexBufferVBO,
					  ( vertexFlags, drawType, reserveVertexSize, reserveIndexSize, usageType ) );

	return eeNew( VertexBufferOGL,
				  ( vertexFlags, drawType, reserveVertexSize, reserveIndexSize, usageType ) );
}

VertexBuffer* VertexBuffer::NewVertexArray( const Uint32& vertexFlags, PrimitiveType drawType,
											const Int32& reserveVertexSize,
											const Int32& reserveIndexSize,
											VertexBufferUsageType usageType ) {
	return eeNew( VertexBufferOGL,
				  ( vertexFlags, drawType, reserveVertexSize, reserveIndexSize, usageType ) );
}

VertexBuffer::VertexBuffer( const Uint32& vertexFlags, PrimitiveType drawType,
							const Int32& reserveVertexSize, const Int32& reserveIndexSize,
							VertexBufferUsageType usageType ) :
	mVertexFlags( vertexFlags ), mDrawType( drawType ), mUsageType( usageType ), mElemDraw( -1 ) {
	if ( reserveVertexSize > 0 ) {
		for ( Int32 i = 0; i < VERTEX_FLAGS_COUNT; i++ ) {
			if ( VERTEX_FLAG_QUERY( mVertexFlags, i ) ) {
				if ( i == VERTEX_FLAG_POSITION ) {
					mPosArray.reserve( reserveVertexSize );
				} else if ( i != VERTEX_FLAG_COLOR ) {
					mTexCoordArray[i - 1].reserve( reserveVertexSize );
				} else {
					mColorArray.reserve( reserveVertexSize );
				}
			}
		}
	}

	if ( reserveIndexSize > 0 ) {
		mIndexArray.reserve( reserveIndexSize );
	}

	VertexBufferManager::instance()->add( this );
}

VertexBuffer::~VertexBuffer() {
	VertexBufferManager::instance()->remove( this );
}

void VertexBuffer::addVertex( const Uint32& type, const Vector2f& vertex ) {
	eeASSERT( type < VERTEX_FLAGS_COUNT_ARR );
	switch ( type ) {
		case VERTEX_FLAG_POSITION:
			mPosArray.push_back( vertex );
			break;
		case VERTEX_FLAG_TEXTURE0:
		case VERTEX_FLAG_TEXTURE1:
		case VERTEX_FLAG_TEXTURE2:
		case VERTEX_FLAG_TEXTURE3:
			mTexCoordArray[type - 1].push_back( vertex );
			break;
	}
}

void VertexBuffer::addVertex( const Vector2f& vertex ) {
	mPosArray.push_back( vertex );
}

void VertexBuffer::addTextureCoord( const Vector2f& vertexCoord, const Uint32& textureLevel ) {
	eeASSERT( textureLevel < VERTEX_FLAG_TEXTURE3 );
	mTexCoordArray[textureLevel].push_back( vertexCoord );
}

void VertexBuffer::addColor( const Color& color ) {
	mColorArray.push_back( color );
}

void VertexBuffer::addIndex( const Uint32& indexValue ) {
	mIndexArray.push_back( indexValue );

	VERTEX_FLAG_SET( mVertexFlags, VERTEX_FLAG_USE_INDICES );
}

void VertexBuffer::setVertex( const Uint32& index, const Uint32& type, const Vector2f& vertex ) {
	switch ( type ) {
		case VERTEX_FLAG_POSITION:
			eeASSERT( index < mPosArray.size() );
			mPosArray[index] = vertex;
			break;
		case VERTEX_FLAG_TEXTURE0:
		case VERTEX_FLAG_TEXTURE1:
		case VERTEX_FLAG_TEXTURE2:
		case VERTEX_FLAG_TEXTURE3:
			eeASSERT( type < VERTEX_FLAG_TEXTURE3 );
			eeASSERT( index < mTexCoordArray[type - 1].size() );
			mTexCoordArray[type - 1][index] = vertex;
			break;
	}
}

void VertexBuffer::setVertex( const Uint32& index, const Vector2f& vertex ) {
	eeASSERT( index < mPosArray.size() );
	mPosArray[index] = vertex;
}

void VertexBuffer::setTextureCoord( const Uint32& index, const Vector2f& vertexCoord,
									const Uint32& textureLevel ) {
	eeASSERT( textureLevel < VERTEX_FLAG_TEXTURE3 );
	eeASSERT( index < mTexCoordArray[textureLevel].size() );
	mTexCoordArray[textureLevel][index] = vertexCoord;
}

void VertexBuffer::setColor( const Uint32& index, const Color& color ) {
	eeASSERT( index < mColorArray.size() );
	mColorArray[index] = color;
}

void VertexBuffer::setIndex( const Uint32& index, const Uint32& indexValue ) {
	eeASSERT( index < mIndexArray.size() );
	mIndexArray[index] = indexValue;
}

void VertexBuffer::resizeArray( const Uint32& type, const Uint32& size ) {
	switch ( type ) {
		case VERTEX_FLAG_POSITION:
			mPosArray.resize( size );
			break;
		case VERTEX_FLAG_TEXTURE0:
		case VERTEX_FLAG_TEXTURE1:
		case VERTEX_FLAG_TEXTURE2:
		case VERTEX_FLAG_TEXTURE3:
			mTexCoordArray[type - 1].resize( size );
			break;
		case VERTEX_FLAG_COLOR:
			mColorArray.resize( size );
			break;
	}
}

void VertexBuffer::resizeIndices( const Uint32& size ) {
	mIndexArray.resize( size );
}

std::vector<Vector2f>& VertexBuffer::getPositionArray() {
	return mPosArray;
}

std::vector<Color>& VertexBuffer::getColorArray() {
	return mColorArray;
}

std::vector<Uint32>& VertexBuffer::getIndices() {
	return mIndexArray;
}

std::vector<Vector2f>& VertexBuffer::getTextureCoordArray( const Uint32& textureLevel ) {
	eeASSERT( textureLevel < mTexCoordArray->size() );
	return mTexCoordArray[textureLevel];
}

Uint32 VertexBuffer::getVertexCount() {
	return mPosArray.size();
}

Uint32 VertexBuffer::getIndexCount() {
	return (Uint32)mIndexArray.size();
}

Color VertexBuffer::getColor( const Uint32& index ) {
	eeASSERT( !VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_COLOR ) );

	Int32 pos = index * VertexElementCount[VERTEX_FLAG_COLOR];

	return Color( mColorArray[pos] );
}

Uint32 VertexBuffer::getIndex( const Uint32& index ) {
	eeASSERT( index < mIndexArray.size() );
	return mIndexArray[index];
}

void VertexBuffer::setElementNum( Int32 num ) {
	mElemDraw = num;
}

const Int32& VertexBuffer::getElementNum() const {
	return mElemDraw;
}

void VertexBuffer::clear() {
	mPosArray.clear();
	for ( auto& texCoord : mTexCoordArray )
		texCoord.clear();
	mColorArray.clear();
	mIndexArray.clear();
}

}} // namespace EE::Graphics
