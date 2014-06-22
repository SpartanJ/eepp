#ifndef EE_GRAPHICSCFRAMEBUFFERFBO_HPP
#define EE_GRAPHICSCFRAMEBUFFERFBO_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/framebuffer.hpp>
#include <eepp/graphics/texture.hpp>

namespace EE { namespace Graphics {

class EE_API FrameBufferFBO : public FrameBuffer {
	public:
		FrameBufferFBO( EE::Window::Window * window = NULL );

		~FrameBufferFBO();

		FrameBufferFBO( const Uint32& Width, const Uint32& Height, bool DepthBuffer = false, EE::Window::Window * window = NULL );

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

