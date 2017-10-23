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

		FrameBufferFBO( const Uint32& Width, const Uint32& Height, bool StencilBuffer = true, bool DepthBuffer = false, const Uint32& channels = 4, EE::Window::Window * window = NULL );

		void bind();

		void unbind();

		void reload();

		void resize( const Uint32& Width, const Uint32& Height );

		const Int32& getFrameBufferId() const;

		static bool isSupported();
	protected:
		Int32 		mFrameBuffer;
		Uint32 		mDepthBuffer;
		Uint32		mStencilBuffer;
		Int32		mLastFB;
		Int32		mLastDB;
		Int32		mLastSB;

		bool create( const Uint32& Width, const Uint32& Height );

		bool create( const Uint32& Width, const Uint32& Height, bool StencilBuffer, bool DepthBuffer, const Uint32& channels );

		void bindFrameBuffer();

		void bindDepthBuffer();

		void bindStencilBuffer();
};

}}

#endif

