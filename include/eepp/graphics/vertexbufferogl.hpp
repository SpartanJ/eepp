#ifndef EE_GRAPHICSCVERTEXBUFFEROGL_HPP
#define EE_GRAPHICSCVERTEXBUFFEROGL_HPP

#include <eepp/graphics/vertexbuffer.hpp>

namespace EE { namespace Graphics {

/** The Vertex Buffer OGL is the implementation of a Vertex Buffer without using the specific VBOs
   extensions from OpenGL.
	@see VertexBuffer
*/
class EE_API VertexBufferOGL : public VertexBuffer {
  public:
	VertexBufferOGL( const Uint32& VertexFlags = VERTEX_FLAGS_DEFAULT,
					 PrimitiveType DrawType = PRIMITIVE_QUADS, const Int32& ReserveVertexSize = 0,
					 const Int32& ReserveIndexSize = 0,
					 EE_VBO_USAGE_TYPE UsageType = VBO_USAGE_TYPE_STATIC );

	void bind();

	void draw();

	bool compile();

	void update( const Uint32& Types, bool Indices );

	void reload();

	void unbind();

  protected:
	void setVertexStates();
};

}} // namespace EE::Graphics

#endif
