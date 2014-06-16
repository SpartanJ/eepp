#ifndef EE_GRAPHICSCFRAMEBUFFER_HPP
#define EE_GRAPHICSCFRAMEBUFFER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/ctexture.hpp>
#include <eepp/window/view.hpp>

namespace EE { namespace Window { class Window; } }

using namespace EE::Window;

namespace EE { namespace Graphics {

/** @brief A frame buffer allows rendering to a off-sreen 2D texture  */
class EE_API cFrameBuffer {
	public:
		/** @brief Creates a new instance of a frame buffer
		**	@param Width The frame buffer width
		**	@param Height The frame buffer height
		**	@param DepthBuffer Indicates if a depth buffer is used
		**	@param window In case that the application is using more than one window, the user can indicate which one to use ( by default uses the current active window )
		*/
		static cFrameBuffer * New( const Uint32& Width, const Uint32& Height, bool DepthBuffer = false, EE::Window::Window * window = NULL );

		virtual ~cFrameBuffer();

		/** @brief Enables the off-screen rendering.
		**	From this moment any rendered primitive will be rendered to the frame buffer.
		**	Anything rendered since the frame buffer is binded will use the fram buffer coordinates, so position 0,0 means 0,0 point in the frame buffer, not the screen. */
		virtual void Bind() = 0;

		/** @brief Disables the off-screen rendering.
		**	Anything rendered after this will be rendered to the back-buffer. */
		virtual void Unbind() = 0;

		/** @brief Clears the frame buffer pixels to the default frame buffer clear color. */
		void Clear();

		/** @brief Recreates the frame buffer ( delete the current and creates a new one ).
		**	This is needed by the engine to recover any context lost. */
		virtual void Reload() = 0;

		/** @return The frame buffer texture. Everything is rendered to this texture.
		** To render the frame buffer you just need to draw the texture as any other texture.
		** The frame buffer must be unbinded before any rendering is done outside the frame buffer.
		** For example MyFrameBufferPtr->GetTexture()->Draw(0,0);
		*/
		cTexture * GetTexture() const;

		/** @brief Sets the frame buffer clear color. */
		void ClearColor( ColorAf Color );

		/** @return The clear color used for the frame buffer. */
		ColorAf ClearColor() const;

		/** @return The frame buffer width. */
		const Int32& GetWidth() const;

		/** @return The frame buffer height. */
		const Int32& GetHeight() const;

		/** @return True if the frame buffer has a depth buffer. */
		const bool& HasDepthBuffer() const;
	protected:
		EE::Window::Window *	mWindow;
		Int32		mWidth;
		Int32		mHeight;
		bool		mHasDepthBuffer;
		cTexture *	mTexture;
		ColorAf	mClearColor;
		View 		mPrevView;
		float		mProjMat[16];

		cFrameBuffer( EE::Window::Window * window );

		virtual bool Create( const Uint32& Width, const Uint32& Height ) = 0;

		void		SetBufferView();

		void		RecoverView();
};

}}

#endif

