#ifndef EE_GRAPHICSCVERTEXBUFFERVBO_HPP
#define EE_GRAPHICSCVERTEXBUFFERVBO_HPP

#include <eepp/graphics/vertexbuffer.hpp>

namespace EE { namespace Graphics {

/** @brief The Vertex Buffer VBO class is the implementation of a Vertex Buffer using the OpenGL
   VBOs.
	@see VertexBuffer
	More information in http://en.wikipedia.org/wiki/Vertex_Buffer_Object
*/
class EE_API VertexBufferVBO : public VertexBuffer {
  public:
	VertexBufferVBO( const Uint32& vertexFlags = VERTEX_FLAGS_DEFAULT,
					 PrimitiveType drawType = PRIMITIVE_QUADS, const Int32& reserveVertexSize = 0,
					 const Int32& reserveIndexSize = 0,
					 VertexBufferUsageType usageType = VertexBufferUsageType::Static );

	virtual ~VertexBufferVBO();

	void bind();

	void draw();

	bool compile();

	void update( const Uint32& types, bool indices );

	void reload();

	void unbind();

	void clear();

  protected:
	bool mCompiled;
	bool mBuffersSet;
	bool mTextured;
	Uint32 mVAO;
	Uint32 mElementHandle;
	Uint32 mArrayHandle[VERTEX_FLAGS_COUNT];

	void setVertexStates();
};

}} // namespace EE::Graphics

#endif
