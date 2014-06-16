#ifndef EE_GRAPHICSCFRAMEBUFFERFBO_HPP
#define EE_GRAPHICSCFRAMEBUFFERFBO_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/cframebuffer.hpp>
#include <eepp/graphics/ctexture.hpp>

namespace EE { namespace Graphics {

class EE_API cFrameBufferFBO : public cFrameBuffer {
	public:
		cFrameBufferFBO( EE::Window::Window * window = NULL );

		~cFrameBufferFBO();

		cFrameBufferFBO( const Uint32& Width, const Uint32& Height, bool DepthBuffer = false, EE::Window::Window * window = NULL );

		void Bind();

		void Unbind();

		void Reload();

		static bool IsSupported();
	protected:
		Int32 		mFrameBuffer;
		Uint32 		mDepthBuffer;
		Int32		mLastFB;
		Int32		mLastRB;

		bool Create( const Uint32& Width, const Uint32& Height );

		bool Create( const Uint32& Width, const Uint32& Height, bool DepthBuffer );

		void BindFrameBuffer();

		void BindRenderBuffer();
};

}}

#endif

