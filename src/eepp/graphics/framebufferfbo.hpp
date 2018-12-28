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

		FrameBufferFBO( const Uint32& Width, const Uint32& Height, bool StencilBuffer = true, bool DepthBuffer = false, bool useColorBuffer = false, const Uint32& channels = 4, EE::Window::Window * window = NULL );

		void bind();

		void unbind();

		void reload();

		void resize( const Uint32& Width, const Uint32& Height );

		void draw( const Vector2f& position, const Sizef& size );

		void draw( Rect src, Rect dst );

		bool created() const;

		const Int32& getFrameBufferId() const;

		static bool isSupported();
	protected:
		Int32 		mFrameBuffer;
		Uint32		mColorBuffer;
		Uint32 		mDepthBuffer;
		Uint32		mStencilBuffer;
		Int32		mLastFB;
		Int32		mLastCB;
		Int32		mLastDB;
		Int32		mLastSB;

		bool create( const Uint32& Width, const Uint32& Height );

		bool create( const Uint32& Width, const Uint32& Height, bool StencilBuffer, bool useColorBuffer, bool DepthBuffer, const Uint32& channels );

		void bindFrameBuffer();

		void bindDepthBuffer();

		void bindStencilBuffer();

		void bindColorBuffer();
};

}}

#endif

