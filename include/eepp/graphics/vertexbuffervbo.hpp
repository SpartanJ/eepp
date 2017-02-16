#ifndef EE_GRAPHICSCVERTEXBUFFERVBO_HPP
#define EE_GRAPHICSCVERTEXBUFFERVBO_HPP

#include <eepp/graphics/vertexbuffer.hpp>

namespace EE { namespace Graphics {

/** @brief The Vertex Buffer VBO class is the implementation of a Vertex Buffer using the OpenGL VBOs.
	@see VertexBuffer
	More information in http://en.wikipedia.org/wiki/Vertex_Buffer_Object
*/
class EE_API VertexBufferVBO : public VertexBuffer {
	public:
		VertexBufferVBO( const Uint32& VertexFlags = VERTEX_FLAGS_DEFAULT, EE_DRAW_MODE DrawType = DM_QUADS, const Int32& ReserveVertexSize = 0, const Int32& ReserveIndexSize = 0, EE_VBO_USAGE_TYPE UsageType = VBO_USAGE_TYPE_STATIC );

		virtual ~VertexBufferVBO();

		void bind();

		void draw();

		bool compile();

		void update( const Uint32& Types, bool Indices );

		void reload();

		void unbind();

		void clear();
	protected:
		bool mCompiled;
		bool mBuffersSet;
		bool mTextured;
		Uint32 mVAO;
		Uint32 mElementHandle;
		Uint32 mArrayHandle[ VERTEX_FLAGS_COUNT ];

		void setVertexStates();
};

}}

#endif
