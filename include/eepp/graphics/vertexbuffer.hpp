#ifndef EE_GRAPHICSCVERTEXBUFFER_HPP
#define EE_GRAPHICSCVERTEXBUFFER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/primitivetype.hpp>
#include <eepp/graphics/vertexbufferhelper.hpp>

#include <eepp/system/color.hpp>
using namespace EE::System;

namespace EE { namespace Graphics {

/** @brief The vertex buffer class holds vertex data. The vertex position, colors, texture
 *coordinates and indexes. This is useful to accelerate and encapsulate data.
 */
class EE_API VertexBuffer {
  public:
	/** @brief Creates a new Vertex Buffer.
	 *	@param vertexFlags The vertex flags indicates which vertex data will be used. @see
	 *VertexFlags
	 *	@param drawType The type of the primitive to draw.
	 *	@param reserveVertexSize If the vertex size is known is possible to reserve the space in
	 *memory to avoid resizing the array.
	 *	@param reserveIndexSize If the vertex size is known is possible to reserve the space in
	 *memory for the indices to avoid resizing the array.
	 *	@param usageType This indicates the kind of usage VBO will have. It's only useful if VBO
	 *extensions are supported ( almost for sure that it's supported ). More information here:
	 *http://www.opengl.org/sdk/docs/man/xhtml/glBufferData.xml
	 */
	static VertexBuffer* New( const Uint32& vertexFlags = VERTEX_FLAGS_DEFAULT,
							  PrimitiveType drawType = PRIMITIVE_QUADS,
							  const Int32& reserveVertexSize = 0, const Int32& reserveIndexSize = 0,
							  VertexBufferUsageType usageType = VertexBufferUsageType::Static );

	/** Creates the simple vertex array implementation ( without VBOs or VAO ), which it's faster
	 * for many cases. */
	static VertexBuffer*
	NewVertexArray( const Uint32& vertexFlags = VERTEX_FLAGS_DEFAULT,
					PrimitiveType drawType = PRIMITIVE_QUADS, const Int32& reserveVertexSize = 0,
					const Int32& reserveIndexSize = 0,
					VertexBufferUsageType usageType = VertexBufferUsageType::Static );

	virtual ~VertexBuffer();

	/** @brief Adds a vertex of the type indicated to the buffer
	 *	@param type Can be the position or texture coordinates.
	 *	@param vertex The vexter data */
	void addVertex( const Uint32& type, const Vector2f& vertex );

	/** @brief Adds a vertex position to the buffer
	 *	@param vertex The vexter data */
	void addVertex( const Vector2f& vertex );

	/** @brief Adds a vertex texture coordinate.
	 *	@param vertexCoord The vertex texture coordinate.
	 *	@param textureLevel Indicates the texture level if it's using multitextures.
	 */
	void addTextureCoord( const Vector2f& vertexCoord, const Uint32& textureLevel = 0 );

	/** @brief Adds a color to the buffer.
	 *	@param color The color value.
	 */
	void addColor( const Color& color );

	/** @brief Adds an index to the buffer.
	 *	@param indexValue The index value.
	 */
	void addIndex( const Uint32& indexValue );

	/** @brief Set a vertex index of the type indicated to the buffer
	 *  @param index The array index of the vertex to set
	 *	@param type Can be the position or texture coordinates.
	 *	@param vertex The vexter data */
	void setVertex( const Uint32& index, const Uint32& type, const Vector2f& vertex );

	/** @brief Adds a vertex position to the buffer
	 *  @param index The array index of the vertex to set
	 *	@param vertex The vexter data */
	void setVertex( const Uint32& index, const Vector2f& vertex );

	/** @brief Adds a vertex texture coordinate.
	 *  @param index The array index of the vertex to set
	 *	@param vertexCoord The vertex texture coordinate.
	 *	@param textureLevel Indicates the texture level if it's using multitextures.
	 */
	void setTextureCoord( const Uint32& index, const Vector2f& vertexCoord,
						  const Uint32& textureLevel = 0 );

	/** @brief Adds a color to the buffer.
	 *  @param index The array index of the vertex to set
	 *	@param color The color value.
	 */
	void setColor( const Uint32& index, const Color& color );

	/** @brief Adds an index to the buffer.
	 *  @param index The array index of the vertex to set
	 *	@param indexValue The index value.
	 */
	void setIndex( const Uint32& index, const Uint32& indexValue );

	/** @brief Resizes the array of the type indicated.
	 *	@param type The type must be one of the vertex flags ( @see VertexFlags ).
	 *	@param size The size to be resized
	 */
	void resizeArray( const Uint32& type, const Uint32& size );

	/** @brief Resizes the indices array.
	 *	@param size The new size
	 */
	void resizeIndices( const Uint32& size );

	void addQuad( const Vector2f& pos, const Sizef& size, const Color& color );

	void setQuad( const Vector2u& gridPos, const Vector2f& pos, const Sizef& size,
				  const Color& color );

	void setQuadColor( const Vector2u& gridPos, const Color& color );

	void setQuadFree( const Vector2u& gridPos, const Vector2f& pos0, const Vector2f& pos1,
					  const Vector2f& pos2, const Vector2f& pos3, const Color& color );

	void setQuadTexCoords( const Vector2u& gridPos, const Rectf& coords,
						   const Uint32& textureLevel );

	void setGridSize( const Sizei& size );

	/** @return The position array */
	std::vector<Vector2f>& getPositionArray();

	/** @return The color array pointer. */
	std::vector<Color>& getColorArray();

	/** @return The indices array pointer. */
	std::vector<Uint32>& getIndices();

	/** @return The texture coord array from the texture level */
	std::vector<Vector2f>& getTextureCoordArray( const Uint32& textureLevel );

	/** @return The number of vertex added. */
	Uint32 getVertexCount();

	/** @return The number of indexes added. */
	Uint32 getIndexCount();

	/** @return The color at the buffer position.
	 *	@param index The position in the buffer.
	 */
	Color getColor( const Uint32& index );

	/** @return The index at the buffer position.
	 *	@param index The position in the buffer.
	 */
	Uint32 getIndex( const Uint32& index );

	/** @brief Sets the number of elements to draw. If not set, it will draw all the elements. */
	void setElementNum( Int32 num );

	/** @return The number of elements added. */
	const Int32& getElementNum() const;

	/** @brief Activates the vertex buffer. */
	virtual void bind() = 0;

	/** @brief Deactivates the vertex buffer. */
	virtual void unbind() = 0;

	/** @brief Draw the vertex buffer. */
	virtual void draw() = 0;

	/** @brief Compile the vertex buffer.
	 *	After adding all the vertex buffer data Compile() must be called to upload the data to the
	 *GPU.
	 */
	virtual bool compile() = 0;

	/** @brief Update is used in the case of some data is modified and need to be reuploaded to the
	 * GPU. */
	virtual void update( const Uint32& types, bool indices ) = 0;

	/** @brief Reupload all the data to the GPU. */
	virtual void reload() = 0;

	/** Clear the cached data and destroy the buffers */
	virtual void clear();

  protected:
	Uint32 mVertexFlags;
	PrimitiveType mDrawType;
	VertexBufferUsageType mUsageType;
	Int32 mElemDraw;
	std::vector<Vector2f> mPosArray;
	std::vector<Vector2f> mTexCoordArray[4];
	std::vector<Color> mColorArray;
	std::vector<Uint32> mIndexArray;
	Sizei mGridSize;

	VertexBuffer( const Uint32& VertexFlags = VERTEX_FLAGS_DEFAULT,
				  PrimitiveType DrawType = PRIMITIVE_QUADS, const Int32& ReserveVertexSize = 0,
				  const Int32& ReserveIndexSize = 0,
				  VertexBufferUsageType UsageType = VertexBufferUsageType::Static );

	virtual void setVertexStates() = 0;
};

}} // namespace EE::Graphics

#endif

/** @class EE::Graphics::VertexBuffer
*
*	Some example usage of this class:
*	@code
	// Creates a rounded rectangle.
	Polygon2f Poly = Polygon2f::createRoundedRectangle( 0, 0, 256, 50 );

	VertexBuffer * VBO = VertexBuffer::New( VERTEX_FLAGS_PRIMITIVE, PRIMITIVE_TRIANGLE_FAN );

	if ( NULL != VBO ) {
		// Upload the rounded rectangle data to the vertex buffer.
		for ( Uint32 i = 0; i < Poly.Size(); i++ ) {
			VBO->addVertex( Poly[i] );	// Adds the vertex position
			VBO->addColor( ColorA( 255, 255, 255, 255 ) ); // Adds the vertex color
		}

		// Compiles the buffered data
		VBO->compile();
	}

	// ... in the main loop draw the VBO
	while ( win->isRunning() )
	{
		...

		VBO->bind();	// First must be binded.
		VBO->draw();	// Then rendered.
		VBO->unbind();	// Then unbinded to allow render other things.

		...

		win->display()
	}
*	@endcode
*
**/
