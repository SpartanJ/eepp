#ifndef EE_GRAPHICSCFRAMEBUFFER_HPP
#define EE_GRAPHICSCFRAMEBUFFER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/window/view.hpp>

namespace EE { namespace Window { class Window; } }

using namespace EE::Window;

namespace EE { namespace Graphics {

/** @brief A frame buffer allows rendering to a off-sreen 2D texture  */
class EE_API FrameBuffer {
	public:
		/** @brief Creates a new instance of a frame buffer
		**	@param Width The frame buffer width
		**	@param Height The frame buffer height
		**	@param DepthBuffer Indicates if a depth buffer is used
		**	@param window In case that the application is using more than one window, the user can indicate which one to use ( by default uses the current active window )
		*/
		static FrameBuffer * New( const Uint32& Width, const Uint32& Height, bool DepthBuffer = false, EE::Window::Window * window = NULL );

		virtual ~FrameBuffer();

		/** @brief Enables the off-screen rendering.
		**	From this moment any rendered primitive will be rendered to the frame buffer.
		**	Anything rendered since the frame buffer is binded will use the fram buffer coordinates, so position 0,0 means 0,0 point in the frame buffer, not the screen. */
		virtual void bind() = 0;

		/** @brief Disables the off-screen rendering.
		**	Anything rendered after this will be rendered to the back-buffer. */
		virtual void unbind() = 0;

		/** @brief Clears the frame buffer pixels to the default frame buffer clear color. */
		void clear();

		/** @brief Recreates the frame buffer ( delete the current and creates a new one ).
		**	This is needed by the engine to recover any context lost. */
		virtual void reload() = 0;

		/** @return The frame buffer texture. Everything is rendered to this texture.
		** To render the frame buffer you just need to draw the texture as any other texture.
		** The frame buffer must be unbinded before any rendering is done outside the frame buffer.
		** For example MyFrameBufferPtr->getTexture()->Draw(0,0);
		*/
		Texture * getTexture() const;

		/** @brief Sets the frame buffer clear color. */
		void clearColor( ColorAf Color );

		/** @return The clear color used for the frame buffer. */
		ColorAf clearColor() const;

		/** @return The frame buffer width. */
		const Int32& getWidth() const;

		/** @return The frame buffer height. */
		const Int32& getHeight() const;

		/** @return True if the frame buffer has a depth buffer. */
		const bool& hasDepthBuffer() const;
	protected:
		EE::Window::Window *	mWindow;
		Int32		mWidth;
		Int32		mHeight;
		bool		mHasDepthBuffer;
		Texture *	mTexture;
		ColorAf	mClearColor;
		View 		mPrevView;
		float		mProjMat[16];

		FrameBuffer( EE::Window::Window * window );

		virtual bool create( const Uint32& Width, const Uint32& Height ) = 0;

		void		setBufferView();

		void		recoverView();
};

}}

#endif

