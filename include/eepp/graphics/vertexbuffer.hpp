#ifndef EE_GRAPHICSCVERTEXBUFFER_HPP
#define EE_GRAPHICSCVERTEXBUFFER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/vbohelper.hpp>

namespace EE { namespace Graphics {

/** @brief The vertex buffer class holds vertex data. The vertex position, colors, texture coordinates and indexes.
*	This is useful to accelerate and encapsulate data. GPU VBOs are much faster because the data is in the GPU once is uploaded. eepp will try to use GPU VBOs if the GPU support them.
*/
class EE_API VertexBuffer {
	public:
		/** @brief Creates a new Vertex Buffer.
		*	@param VertexFlags The vertex flags indicates which vertex data will be used. @see EE_VERTEX_FLAGS
		*	@param DrawType The type of the primitive to draw.
		*	@param ReserveVertexSize If the vertex size is known is possible to reserve the space in memory to avoid resizeing the array.
		*	@param ReserveIndexSize If the vertex size is known is possible to reserve the space in memory for the indices to avoid resizeing the array.
		*	@param UsageType This indicates the kind of usage VBO will have. It's only useful if VBO extensions are supported ( almost for sure that it's supported ). More information here: http://www.opengl.org/sdk/docs/man/xhtml/glBufferData.xml
		*/
		static VertexBuffer * New( const Uint32& VertexFlags = VERTEX_FLAGS_DEFAULT, EE_DRAW_MODE DrawType = DM_QUADS, const Int32& ReserveVertexSize = 0, const Int32& ReserveIndexSize = 0, EE_VBO_USAGE_TYPE UsageType = VBO_USAGE_TYPE_STATIC );

		virtual ~VertexBuffer();

		/** @brief Adds a vertex of the type indicated to the buffer
		*	@param Type Can be the position or texture coordinates.
		*	@param Vertex The vexter data */
		void AddVertex( const Uint32& Type, const Vector2f& Vertex );

		/** @brief Adds a vertex position to the buffer
		*	@param Vertex The vexter data */
		void AddVertex( const Vector2f& Vertex );

		/** @brief Adds a vertex texture coordinate.
		*	@param VertexCoord The vertex texture coordinate.
		*	@param TextureLevel Indicates the texture level if it's using multitextures.
		*/
		void AddVertexCoord( const Vector2f& VertexCoord, const Uint32& TextureLevel = 0 );

		/** @brief Adds a color to the buffer.
		*	@param Color The color value.
		*/
		void AddColor( const ColorA& Color );

		/** @brief Adds an index to the buffer.
		*	@param Index The index value.
		*/
		void AddIndex( const Uint32& Index );

		/** @brief Resizes the array of the type indicated.
		*	@param Type The type must be one of the vertex flags ( EE_VERTEX_FLAGS ).
		*	@param Size The size to be resized
		*/
		void ResizeArray( const Uint32& Type, const Uint32& Size );

		/** @brief Resizes the indices array.
		*	@param Size The new size
		*/
		void ResizeIndices( const Uint32& Size );

		/** @return the pointer to the array of the type indicated.
		*	@param Type The type must be one of the vertex flags ( EE_VERTEX_FLAGS ). */
		Float * GetArray( const Uint32& Type );

		/** @return The color array pointer. */
		Uint8 * GetColorArray();

		/** @return The indices array pointer. */
		Uint32 * GetIndices();

		/** @return The number of vertex added. */
		Uint32 GetVertexCount();

		/** @return The number of indexes added. */
		Uint32 GetIndexCount();

		/** @return The vector data of the type indicated.
		*	@param Type Can be the position or texture coordinates.
		*	@param Index The position in the buffer.
		*/
		Vector2f GetVector2( const Uint32& Type, const Uint32& Index );

		/** @return The color at the buffer position.
		*	@param Index The position in the buffer.
		*/
		ColorA GetColor( const Uint32& Index );

		/** @return The index at the buffer position.
		*	@param Index The position in the buffer.
		*/
		Uint32 GetIndex( const Uint32& Index );

		/** @brief Sets the number of elements to draw. If not set, it will draw all the elements. */
		void SetElementNum( Int32 Num );

		/** @return The number of elements added. */
		const Int32& GetElementNum() const;

		/** @brief Activates the vertex buffer. */
		virtual void Bind() = 0;

		/** @brief Deactivates the vertex buffer. */
		virtual void Unbind() = 0;

		/** @brief Draw the vertex buffer. */
		virtual void Draw() = 0;

		/** @brief Compile the vertex buffer.
		*	After adding all the vertex buffer data Compile() must be called to upload the data to the GPU.
		*/
		virtual bool Compile() = 0;

		/** @brief Update is used in the case of some data is modified and need to be reuploaded to the GPU. */
		virtual void Update( const Uint32& Types, bool Indices ) = 0;

		/** @brief Reupload all the data to the GPU. */
		virtual void Reload() = 0;

		/** Clear the cached data and destroy the buffers */
		virtual void Clear();
	protected:
		Uint32 					mVertexFlags;
		EE_DRAW_MODE			mDrawType;
		EE_VBO_USAGE_TYPE		mUsageType;
		Int32					mElemDraw;
		std::vector<Float>	mVertexArray[ VERTEX_FLAGS_COUNT - 1 ];
		std::vector<Uint8>		mColorArray;
		std::vector<Uint32>		mIndexArray;

		VertexBuffer( const Uint32& VertexFlags = VERTEX_FLAGS_DEFAULT, EE_DRAW_MODE DrawType = DM_QUADS, const Int32& ReserveVertexSize = 0, const Int32& ReserveIndexSize = 0, EE_VBO_USAGE_TYPE UsageType = VBO_USAGE_TYPE_STATIC );

		virtual void SetVertexStates() = 0;
};

}}

#endif

/** @class EE::Graphics::VertexBuffer
*	@ingroup Graphics
*
*	Some example usage of this class:
*	@code
	// Creates a rounded rectangle.
	Polygon2f Poly = Polygon2f::CreateRoundedRectangle( 0, 0, 256, 50 );

	VertexBuffer * VBO = VertexBuffer::New( VERTEX_FLAGS_PRIMITIVE, DM_TRIANGLE_FAN );

	if ( NULL != VBO ) {
		// Upload the rounded rectangle data to the vertex buffer.
		for ( Uint32 i = 0; i < Poly.Size(); i++ ) {
			VBO->AddVertex( Poly[i] );	// Adds the vertex position
			VBO->AddColor( ColorA( 255, 255, 255, 255 ) ); // Adds the vertex color
		}

		// Compiles the buffered data
		VBO->Compile();
	}

	// ... in the main loop draw the VBO
	while ( win->Running() )
	{
		...

		VBO->Bind();	// First must be binded.
		VBO->Draw();	// Then rendered.
		VBO->Unbind();	// Then unbinded to allow render other things.

		...

		win->Display()
	}
*	@endcode
*
**/
