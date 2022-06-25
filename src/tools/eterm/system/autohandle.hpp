// The MIT License (MIT)

// Copyright (c) 2020 Fredrik A. Kristiansen

//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
#pragma once

namespace EE {

// This class is designed to automatically free a resource handle when destructed
class AutoHandle final {
  public:
#ifdef _WIN32
	// On Windows we deal with the Windows API, which means HANDLE is used as the handle to files
	// and pipes
	using type = void*;
	static constexpr type invalid_value();
#else
	// On non-Windows OS'es we deal with file descriptors
	using type = int;
	static constexpr type invalid_value();
#endif
	explicit AutoHandle( type handle );

	AutoHandle();

	AutoHandle( AutoHandle&& other );

	~AutoHandle();

	AutoHandle& operator=( AutoHandle&& other );

	AutoHandle( const AutoHandle& ) = delete;

	AutoHandle& operator=( const AutoHandle& ) = delete;

	explicit operator type() const noexcept;

	operator bool() const noexcept;

	type* get();

	const type* get() const;

	type handle() const {
		return m_hHandle;
	}

	void release() const;

  private:
	mutable type m_hHandle;
};

} // namespace EE
