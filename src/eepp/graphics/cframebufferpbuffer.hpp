#ifndef EE_GRAPHICSCFRAMEBUFFERPBUFFER_HPP
#define EE_GRAPHICSCFRAMEBUFFERPBUFFER_HPP

/** Part of this code is based on the implementation of PBuffers from:
*
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2009 Laurent Gomila (laurent.gom@gmail.com)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
*
* NOTE by Martin Lucas Golini: This is not the original version, so differs from SFML implementation.
*/

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/cframebuffer.hpp>

namespace EE { namespace Graphics {

class EE_API cFrameBufferPBuffer : public cFrameBuffer {
	public:
		cFrameBufferPBuffer( Window::cWindow * window = NULL );

		~cFrameBufferPBuffer();

		cFrameBufferPBuffer( const Uint32& Width, const Uint32& Height, bool DepthBuffer = false, Window::cWindow * window = NULL );

		void Bind();

		void Unbind();

		void Reload();

		static bool IsSupported();
	protected:
		#ifdef EE_GLEW_AVAILABLE

		#if EE_PLATFORM == EE_PLATFORM_WIN
		void *			mDeviceContext;
		void *			mPBuffer;
		void *			mContext;
		#elif defined( EE_X11_PLATFORM )
		void *			mDisplay;
		unsigned long	mPBuffer;
		void *			mContext;
		#endif

		#endif

		bool Create( const Uint32& Width, const Uint32& Height );

		bool Create( const Uint32& Width, const Uint32& Height, bool DepthBuffer );
};

}}

#endif
