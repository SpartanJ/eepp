#ifndef EE_GRAPHICSCVERTEXBUFFEROGL_HPP
#define EE_GRAPHICSCVERTEXBUFFEROGL_HPP

#include "cvertexbuffer.hpp"

namespace EE { namespace Graphics {

class cVertexBufferOGL : public cVertexBuffer {
	public:
		cVertexBufferOGL( const Uint32& VertexFlags = VERTEX_FLAGS_DEFAULT, EE_DRAW_MODE DrawType = DM_QUADS, const Int32& ReserveVertexSize = 0, const Int32& ReserveIndexSize = 0, EE_VBO_USAGE_TYPE UsageType = VBO_USAGE_TYPE_STATIC );

		virtual ~cVertexBufferOGL();

		void Bind();

		void Draw();

		bool Compile();

		void Update( const Uint32& Types, bool Indices );
	protected:
		void SetVertexStates();

};

}}

#endif

