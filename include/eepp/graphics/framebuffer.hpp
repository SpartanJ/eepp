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
		**	@param StencilBuffer Indicates if a stencil buffer is used
		**	@param DepthBuffer Indicates if a depth buffer is used
		**	@param window In case that the application is using more than one window, the user can indicate which one to use ( by default uses the current active window )
		*/
		static FrameBuffer * New( const Uint32& Width, const Uint32& Height, bool StencilBuffer = true, bool DepthBuffer = false, const Uint32& channels = 4, EE::Window::Window * window = NULL );

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

		/** @brief Resizes the current Frame Buffer */
		virtual void resize( const Uint32& Width, const Uint32& Height ) = 0;

		/** @return The allocated Frame Buffer internal Id ( OpenGL FBO id ) */
		virtual const Int32& getFrameBufferId() const = 0;

		/** @return The frame buffer texture. Everything is rendered to this texture.
		** To render the frame buffer you just need to draw the texture as any other texture.
		** The frame buffer must be unbinded before any rendering is done outside the frame buffer.
		** For example MyFrameBufferPtr->getTexture()->Draw(0,0);
		*/
		Texture * getTexture() const;

		/** @brief Sets the frame buffer clear color. */
		void setClearColor( ColorAf Color );

		/** @return The clear color used for the frame buffer. */
		ColorAf getClearColor() const;

		/** @return The frame buffer width. */
		const Int32& getWidth() const;

		/** @return The frame buffer height. */
		const Int32& getHeight() const;

		/** @return The frame buffer size. */
		const Sizei& getSize() const;

		/** @return The frame buffer size ( float ). */
		const Sizef getSizef();

		/** @return True if the frame buffer has a depth buffer. */
		const bool& hasDepthBuffer() const;

		/** @return True if the frame buffer has a stencil buffer */
		const bool& hasStencilBuffer() const;

		/** @return The frame buffer name. */
		const std::string& getName() const;

		/** Set a name to the frame buffer */
		void setName( const std::string& name );

		/** @return The hash id from its name */
		const Uint32& getId() const;
	protected:
		EE::Window::Window *	mWindow;
		Sizei		mSize;
		Uint32		mChannels;
		std::string mName;
		Uint32		mId;
		bool		mHasDepthBuffer;
		bool		mHasStencilBuffer;
		Texture *	mTexture;
		ColorAf	mClearColor;
		View		mView;
		View 		mPrevView;
		float		mProjMat[16];
		float		mModelViewMat[16];

		FrameBuffer( EE::Window::Window * window );

		virtual bool create( const Uint32& Width, const Uint32& Height ) = 0;

		void		setBufferView();

		void		recoverView();
};

}}

#endif

