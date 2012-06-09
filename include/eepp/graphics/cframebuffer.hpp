#ifndef EE_GRAPHICSCFRAMEBUFFER_HPP
#define EE_GRAPHICSCFRAMEBUFFER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/ctexture.hpp>
#include <eepp/window/cview.hpp>
#include <eepp/window/cwindow.hpp>

using namespace EE::Window;

namespace EE { namespace Graphics {

class EE_API cFrameBuffer {
	public:
		static cFrameBuffer * CreateNew( const Uint32& Width, const Uint32& Height, bool DepthBuffer = false, Window::cWindow * window = NULL );

		cFrameBuffer( Window::cWindow * window );

		virtual ~cFrameBuffer();

		virtual bool Create( const Uint32& Width, const Uint32& Height ) = 0;

		virtual void Bind() = 0;

		virtual void Unbind() = 0;

		void Clear();

		virtual void Reload() = 0;

		cTexture * GetTexture() const;

		void ClearColor( eeColorAf Color );

		eeColorAf ClearColor() const;

		const Int32& GetWidth() const;

		const Int32& GetHeight() const;

		const bool& HasDepthBuffer() const;
	protected:
		Window::cWindow *	mWindow;
		Int32		mWidth;
		Int32		mHeight;
		bool		mHasDepthBuffer;
		cTexture *	mTexture;
		eeColorAf	mClearColor;
		cView 		mPrevView;

		void		SetBufferView();
		void		RecoverView();
};

}}

#endif

