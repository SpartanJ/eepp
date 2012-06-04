#ifndef EE_GRAPHICSCFRAMEBUFFERFBO_HPP
#define EE_GRAPHICSCFRAMEBUFFERFBO_HPP

#include "base.hpp"
#include "cframebuffer.hpp"
#include "ctexture.hpp"

namespace EE { namespace Graphics {

class EE_API cFrameBufferFBO : public cFrameBuffer {
	public:
		cFrameBufferFBO( Window::cWindow * window = NULL );

		~cFrameBufferFBO();

		cFrameBufferFBO( const Uint32& Width, const Uint32& Height, bool DepthBuffer = false, Window::cWindow * window = NULL );

		bool Create( const Uint32& Width, const Uint32& Height );

		bool Create( const Uint32& Width, const Uint32& Height, bool DepthBuffer );

		void Bind();

		void Unbind();

		void Reload();

		static bool IsSupported();
	protected:
		Int32 		mFrameBuffer;
		Uint32 		mDepthBuffer;
		Int32		mLastFB;
		Int32		mLastRB;

		void		BindFrameBuffer();
};

}}

#endif

