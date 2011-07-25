#ifndef EE_GRAPHICSCVERTEXBUFFERVBO_HPP
#define EE_GRAPHICSCVERTEXBUFFERVBO_HPP

#include "cvertexbuffer.hpp"

namespace EE { namespace Graphics {

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
	protected:
		void SetVertexStates();

		bool mCompiled;

		bool mBuffersSet;

		bool mTextured;

		GLuint mVAO;

		Uint32 mElementHandle;

		Uint32 mArrayHandle[ VERTEX_FLAGS_COUNT ];
};

}}

#endif
