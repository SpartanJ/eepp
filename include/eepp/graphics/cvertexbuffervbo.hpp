#ifndef EE_GRAPHICSCVERTEXBUFFERVBO_HPP
#define EE_GRAPHICSCVERTEXBUFFERVBO_HPP

#include <eepp/graphics/cvertexbuffer.hpp>

namespace EE { namespace Graphics {

/** @brief The Vertex Buffer VBO class is the implementation of a Vertex Buffer using the OpenGL VBOs.
	@see cVertexBuffer
	More information in http://en.wikipedia.org/wiki/Vertex_Buffer_Object
*/
class EE_API cVertexBufferVBO : public cVertexBuffer {
	public:
		cVertexBufferVBO( const Uint32& VertexFlags = VERTEX_FLAGS_DEFAULT, EE_DRAW_MODE DrawType = DM_QUADS, const Int32& ReserveVertexSize = 0, const Int32& ReserveIndexSize = 0, EE_VBO_USAGE_TYPE UsageType = VBO_USAGE_TYPE_STATIC );

		virtual ~cVertexBufferVBO();

		void Bind();

		void Draw();

		bool Compile();

		void Update( const Uint32& Types, bool Indices );

		void Reload();

		void Unbind();

		void Clear();
	protected:
		bool mCompiled;
		bool mBuffersSet;
		bool mTextured;
		Uint32 mVAO;
		Uint32 mElementHandle;
		Uint32 mArrayHandle[ VERTEX_FLAGS_COUNT ];

		void SetVertexStates();
};

}}

#endif
