// ┌─┐┬  ┬┌─┐┌─┐┌┬┐┌─┐┬─┐   Compact SVO optimized vector C++17 or higher
// └─┐└┐┌┘├┤ │   │ │ │├┬┘   Version 1.4.0
// └─┘ └┘ └─┘└─┘ ┴ └─┘┴└─   https://github.com/martinus/svector
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2023 Martin Leitner-Ankerl <martin.ankerl@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ANKERL_SVECTOR_H
#define ANKERL_SVECTOR_H

// see https://semver.org/spec/v2.0.0.html
#define ANKERL_SVECTOR_VERSION_MAJOR 1 // incompatible API changes
#define ANKERL_SVECTOR_VERSION_MINOR 4 // add functionality in a backwards compatible manner
#define ANKERL_SVECTOR_VERSION_PATCH 0 // backwards compatible bug fixes

// API versioning with inline namespace, see https://www.foonathan.net/2018/11/inline-namespaces/
#define ANKERL_SVECTOR_VERSION_CONCAT1(major, minor, patch) v##major##_##minor##_##patch
#define ANKERL_SVECTOR_VERSION_CONCAT(major, minor, patch) ANKERL_SVECTOR_VERSION_CONCAT1(major, minor, patch)
#define ANKERL_SVECTOR_NAMESPACE \
    ANKERL_SVECTOR_VERSION_CONCAT(ANKERL_SVECTOR_VERSION_MAJOR, ANKERL_SVECTOR_VERSION_MINOR, ANKERL_SVECTOR_VERSION_PATCH)

/**
 * @brief Keeps a function out of line even where a compiler would rather inline it.
 *
 * Only used for commit_grow(), where being shared between callers is the whole point, see
 * insert_n(). Silently nothing on a compiler that has no such spelling: then the inlining is back
 * and so is the code size, but nothing is wrong.
 */
#if defined(__GNUC__) || defined(__clang__)
#    define ANKERL_SVECTOR_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#    define ANKERL_SVECTOR_NOINLINE __declspec(noinline)
#else
#    define ANKERL_SVECTOR_NOINLINE
#endif

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

#if defined(__has_include)
#    if __has_include(<version>)
#        include <version>
#    endif
#endif

// The C++23 range members. Guarded on the library rather than on the language, because what they
// need is std::from_range_t and the range concepts, and a C++17 build has neither. Everything else
// in this header stays exactly as it was for such a build.
#if defined(__cpp_lib_containers_ranges) && __cpp_lib_containers_ranges >= 202202L
#    define ANKERL_SVECTOR_HAS_RANGES 1
#    include <concepts>
#    include <ranges>
#else
#    define ANKERL_SVECTOR_HAS_RANGES 0
#endif

namespace ankerl {
inline namespace ANKERL_SVECTOR_NAMESPACE {
namespace detail {

template <typename Condition, typename T = void>
using enable_if_t = std::enable_if_t<Condition::value, T>;

template <typename It>
using is_input_iterator = std::is_base_of<std::input_iterator_tag, typename std::iterator_traits<It>::iterator_category>;

// A C++20 iterator need not publish an iterator_category, only an iterator_concept, and then
// is_input_iterator above cannot even be formed. Used to decide whether a range can be handed to
// the iterator pair members as it is.
template <typename It, typename = void>
struct has_iterator_category : std::false_type {};

template <typename It>
struct has_iterator_category<It, std::void_t<typename std::iterator_traits<It>::iterator_category>> : std::true_type {};

#if ANKERL_SVECTOR_HAS_RANGES
// The standard calls this container-compatible-range<T> and uses it for exactly these members.
template <typename R, typename T>
concept container_compatible_range = std::ranges::input_range<R> && std::convertible_to<std::ranges::range_reference_t<R>, T>;
#endif

constexpr auto round_up(size_t n, size_t multiple) -> size_t {
    return ((n + (multiple - 1)) / multiple) * multiple;
}

template <typename T>
constexpr auto cx_min(T a, T b) -> T {
    return a < b ? a : b;
}

template <typename T>
constexpr auto cx_max(T a, T b) -> T {
    return a > b ? a : b;
}

template <typename T>
constexpr auto alignment_of_svector() -> size_t {
    return cx_max(sizeof(void*), std::alignment_of_v<T>);
}

/**
 * @brief Calculates sizeof(svector<T, N>) for a given type and inline capacity
 */
template <typename T>
constexpr auto size_of_svector(size_t min_inline_capacity) -> size_t {
    // + 1 for one byte size in direct mode
    return round_up(sizeof(T) * min_inline_capacity + 1, alignment_of_svector<T>());
}

/**
 * @brief Calculates how many T we can actually store inside of an svector without increasing its sizeof().
 *
 * E.g. svector<char, 1> could store 7 bytes even though 1 is specified. This makes sure we don't waste any
 * of the padding.
 */
template <typename T>
constexpr auto automatic_capacity(size_t min_inline_capacity) -> size_t {
    return cx_min((size_of_svector<T>(min_inline_capacity) - 1U) / sizeof(T), size_t{127});
}

/**
 * @brief Whether the allocator has anything to say about building and destroying an element.
 *
 * std::allocator_traits<A>::construct() calls a.construct(p, args...) when that compiles and does a
 * placement new otherwise, and destroy() is the same story with p->~T(). So when none of them
 * compile, going through the traits and doing it directly are the same code, and a whole range can
 * go through the algorithms in <memory> instead, which know how to turn the relocation of a
 * trivially copyable T into a memcpy. std::allocator lands here, and so does the usual arena or
 * pool allocator that only spells out allocate() and deallocate(), which is what keeps both of them
 * on exactly the code there was before there was an allocator at all.
 * std::pmr::polymorphic_allocator, which passes its resource on to the elements it builds, does
 * not, and takes the loops below.
 *
 * See builds_directly() for which signatures are asked about and why the order matters.
 */
template <typename Void, typename A, typename... Args>
struct has_construct_impl : std::false_type {};

template <typename A, typename... Args>
struct has_construct_impl<std::void_t<decltype(std::declval<A&>().construct(std::declval<Args>()...))>, A, Args...>
    : std::true_type {};

template <typename A, typename... Args>
using has_construct = has_construct_impl<void, A, Args...>;

template <typename A, typename T, typename = void>
struct has_destroy : std::false_type {};

template <typename A, typename T>
struct has_destroy<A, T, std::void_t<decltype(std::declval<A&>().destroy(std::declval<T*>()))>> : std::true_type {};

/**
 * @brief std::allocator, which has to be named rather than probed.
 *
 * Until C++20 it carries a construct() and a destroy() of its own, deprecated in C++17 and gone
 * after it, and both do exactly the placement new and the ~T() the traits would have done without
 * them. So it answers the probes above with a yes that means no, and letting them decide would take
 * the default allocator down the generic path -- and with it every svector written before there was
 * an allocator to name.
 */
template <typename A>
struct is_std_allocator : std::false_type {};

template <typename T>
struct is_std_allocator<std::allocator<T>> : std::true_type {};

/**
 * @brief The probes and the std::allocator exemption, put together so that none is asked that does
 *        not have to be.
 *
 * The probed signatures are the argument shapes the bulk operations use: nothing, an lvalue to copy,
 * an rvalue to move. Inserting from a range of some other type builds a T from whatever the iterator
 * yields, which is not probed, so an allocator with a construct() for that and for none of these
 * three would be missed. std::allocator_traits asks the same question one expression at a time; this
 * asks it once for all of them, because the answer picks a whole algorithm.
 *
 * std::disjunction and std::conjunction stop instantiating at the first argument that decides the
 * answer, and that is load bearing rather than tidy: std::pmr::polymorphic_allocator::destroy() is
 * deprecated in C++20, and merely naming it in an unevaluated expression is enough for a build with
 * -Werror to fail. Its construct() is not deprecated and answers first, so destroy() is never asked
 * about. std::allocator answers before any of them.
 */
template <typename A, typename T>
struct builds_directly : std::disjunction<is_std_allocator<A>,
                                          std::conjunction<std::negation<has_construct<A, T*>>,
                                                           std::negation<has_construct<A, T*, T const&>>,
                                                           std::negation<has_construct<A, T*, T&&>>,
                                                           std::negation<has_destroy<A, T>>>> {};

template <typename A, typename T>
inline constexpr bool builds_elements_directly = builds_directly<A, T>::value;

/**
 * @brief Whether the allocator's allocate() and deallocate() are ::operator new and ::operator
 *        delete, so storage below can call those and skip the trip through allocator_traits.
 *
 * Only std::allocator, and only because the standard says that is what it does. Deliberately not
 * the same name as the question above even though it has the same answer today: one is about how
 * elements are built and the other about where bytes come from, and an allocator that changed its
 * mind about one of them should not silently change the other.
 */
template <typename A>
struct allocates_with_operator_new : is_std_allocator<A> {};

/**
 * @brief Holds the allocator, and holds nothing at all when it has no state.
 *
 * An svector is meant to be as small as its inline capacity and no larger, and for std::allocator
 * and every other stateless allocator it stays that way: the empty base takes no space of its own.
 * A stateful allocator is stored, and then the object grows by what it costs, which is the price of
 * asking for one.
 */
template <typename A, bool = std::is_empty_v<A> && !std::is_final_v<A>>
class allocator_holder : private A {
public:
    allocator_holder() = default;

    explicit allocator_holder(A a)
        : A(std::move(a)) {}

    [[nodiscard]] auto allocator() -> A& {
        return *this;
    }

    [[nodiscard]] auto allocator() const -> A const& {
        return *this;
    }
};

template <typename A>
class allocator_holder<A, false> {
    A m_allocator;

public:
    allocator_holder() = default;

    explicit allocator_holder(A a)
        : m_allocator(std::move(a)) {}

    [[nodiscard]] auto allocator() -> A& {
        return m_allocator;
    }

    [[nodiscard]] auto allocator() const -> A const& {
        return m_allocator;
    }
};

/**
 * @brief std::destroy() through the allocator.
 *
 * The static_cast<void> here and in the five functions like it below says that this branch has no
 * use for the allocator, which is a thing gcc and clang work out for themselves -- neither warns
 * about a parameter that only the discarded branch of an if constexpr reads. MSVC at /W4 is the one
 * this cannot be checked against from here, and the Windows job builds with werror, so the cast
 * stays: it costs a line and nothing else, and removing it can only be worth a red build.
 */
template <typename A, typename T>
void alloc_destroy(A& alloc, T* first, T* last) {
    if constexpr (builds_elements_directly<A, T>) {
        static_cast<void>(alloc);
        std::destroy(first, last);
    } else {
        for (; first != last; ++first) {
            std::allocator_traits<A>::destroy(alloc, first);
        }
    }
}

template <typename A, typename T>
void alloc_destroy_n(A& alloc, T* first, size_t n) {
    alloc_destroy(alloc, first, first + n);
}

/**
 * @brief Destroys [first, last) on the way out, unless it has been released.
 *
 * Half built storage can normally say what it holds with a size, and then a destructor is all the
 * cleanup anyone needs. This is for the places where the built part is not a prefix of a container,
 * so no size can express it. Deleting the copy is what keeps it from being handed around and
 * destroying twice.
 *
 * The allocator is held the way an svector holds it rather than by pointer, so a stateless one adds
 * nothing to the guard at all. A pointer would have been a word of its own on a path that was tuned
 * to the instruction, and for std::allocator it would have pointed at an empty base that is never
 * read. A copy is as good as the original: copying an allocator may not throw, and the copy
 * compares equal, which is all destroying through it needs.
 */
template <typename A>
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions,hicpp-special-member-functions)
class destroy_guard : private allocator_holder<A> {
    using T = typename std::allocator_traits<A>::value_type;
    using holder = allocator_holder<A>;

    T* m_first;
    T* m_last;

public:
    destroy_guard(A const& alloc, T* first, T* last)
        : holder(alloc)
        , m_first(first)
        , m_last(last) {}

    destroy_guard(destroy_guard const&) = delete;
    auto operator=(destroy_guard const&) -> destroy_guard& = delete;

    ~destroy_guard() {
        alloc_destroy(holder::allocator(), m_first, m_last);
    }

    // The guarded range only ever grows, and stays contiguous while it does.
    void extend_front(T* first) {
        m_first = first;
    }

    void extend_back(T* last) {
        m_last = last;
    }

    void release() {
        m_last = m_first;
    }
};

/**
 * @brief Builds n elements at dst, one call to build() each, and destroys whatever it managed to
 *        build if one of them throws.
 *
 * Only for an allocator that builds elements its own way: everything else goes through <memory>,
 * see builds_elements_directly.
 */
template <typename A, typename T, typename Build>
void construct_each(A& alloc, T* dst, size_t n, Build build) {
    auto guard = destroy_guard<A>(alloc, dst, dst);
    for (size_t i = 0; i != n; ++i) {
        build(dst + i);
        guard.extend_back(dst + i + 1);
    }
    guard.release();
}

template <typename A, typename T, typename It>
void alloc_uninitialized_copy_n(A& alloc, It first, size_t n, T* dst) {
    if constexpr (builds_elements_directly<A, T>) {
        static_cast<void>(alloc);
        std::uninitialized_copy_n(first, n, dst);
    } else {
        construct_each(alloc, dst, n, [&](T* p) {
            std::allocator_traits<A>::construct(alloc, p, *first);
            ++first;
        });
    }
}

template <typename A, typename T>
void alloc_uninitialized_move(A& alloc, T* first, T* last, T* dst) {
    if constexpr (builds_elements_directly<A, T>) {
        static_cast<void>(alloc);
        std::uninitialized_move(first, last, dst);
    } else {
        construct_each(alloc, dst, static_cast<size_t>(last - first), [&](T* p) {
            std::allocator_traits<A>::construct(alloc, p, std::move(*first));
            ++first;
        });
    }
}

template <typename A, typename T>
void alloc_uninitialized_fill_n(A& alloc, T* dst, size_t n, T const& value) {
    if constexpr (builds_elements_directly<A, T>) {
        static_cast<void>(alloc);
        std::uninitialized_fill_n(dst, n, value);
    } else {
        construct_each(alloc, dst, n, [&](T* p) {
            std::allocator_traits<A>::construct(alloc, p, value);
        });
    }
}

template <typename A, typename T>
void alloc_uninitialized_value_construct_n(A& alloc, T* dst, size_t n) {
    if constexpr (builds_elements_directly<A, T>) {
        static_cast<void>(alloc);
        std::uninitialized_value_construct_n(dst, n);
    } else {
        construct_each(alloc, dst, n, [&](T* p) {
            std::allocator_traits<A>::construct(alloc, p);
        });
    }
}

/**
 * Holds size & capacity, a glorified struct.
 */
class header {
    size_t m_size{};
    size_t const m_capacity;

public:
    inline explicit header(size_t capacity)
        : m_capacity{capacity} {}

    [[nodiscard]] inline auto size() const -> size_t {
        return m_size;
    }

    [[nodiscard]] inline auto capacity() const -> size_t {
        return m_capacity;
    }

    inline void size(size_t s) {
        m_size = s;
    }
};

/**
 * @brief What the allocator is actually asked for: one alignment's worth of bytes.
 *
 * An allocator hands back memory aligned for its own value_type, and what an indirect svector needs
 * is a header followed by an array of T, which is neither. So the rebind target has to state the
 * alignment itself. Every T with the same alignment shares this type, and with it one instantiation
 * of the rebound allocator.
 */
template <size_t Align>
struct alignas(Align) chunk {
    std::byte raw[Align];
};

/**
 * @brief Holds header (size+capacity) plus an arbitrary number of T.
 *
 * To make storage compact, we don't actually store a pointer to T. We don't have to
 * because we know exactly at which location it begins.
 */
template <typename T>
struct storage : public header {
    static constexpr auto alignment_of_t = std::alignment_of_v<T>;
    static constexpr auto max_alignment = (std::max)(std::alignment_of_v<header>, std::alignment_of_v<T>);
    static constexpr auto offset_to_data = detail::round_up(sizeof(header), alignment_of_t);
    static_assert(max_alignment <= __STDCPP_DEFAULT_NEW_ALIGNMENT__);

    using chunk_type = chunk<max_alignment>;

    template <typename A>
    using rebound = typename std::allocator_traits<A>::template rebind_alloc<chunk_type>;

    explicit storage(size_t capacity)
        : header(capacity) {}

    auto data() -> T* {
        auto ptr_to_data = reinterpret_cast<std::byte*>(this) + offset_to_data;
        return std::launder(reinterpret_cast<T*>(ptr_to_data));
    }

    /**
     * @brief How many chunks hold a header plus capacity*T. allocator() and dealloc() have to agree on
     *        this, and all dealloc() has left to work from is the capacity in the header.
     */
    [[nodiscard]] static constexpr auto num_chunks(size_t capacity) -> size_t {
        return round_up(offset_to_data + sizeof(T) * capacity, max_alignment) / max_alignment;
    }

    /**
     * @brief Allocates space for storage plus capacity*T objects.
     *
     * Checks to make sure that allocation won't overflow.
     *
     * @param a Allocator to take the memory from, rebound to chunk_type.
     * @param capacity Number of T to allocate.
     * @return storage<T>*
     */
    template <typename A>
    static auto alloc(A& a, size_t capacity) -> storage<T>* {
        static_assert(std::is_same_v<typename std::allocator_traits<rebound<A>>::pointer, chunk_type*>,
                      "sorry, an allocator whose rebound pointer is not a raw pointer is not supported");

        // make sure we don't overflow!
        auto mem = sizeof(T) * capacity;
        if (mem < capacity) {
            throw std::bad_alloc();
        }
        if (offset_to_data + mem < mem) {
            throw std::bad_alloc();
        }
        mem += offset_to_data;
        if (static_cast<uint64_t>(mem) > static_cast<uint64_t>((std::numeric_limits<std::ptrdiff_t>::max)())) {
            throw std::bad_alloc();
        }

        void* ptr = nullptr;
        if constexpr (allocates_with_operator_new<A>::value) {
            // std::allocator is ::operator new, so this is where it was going anyway. Taking it
            // directly is not a shortcut for its own sake: it keeps the byte count already computed
            // above instead of dividing it into chunks for allocate() to multiply back, which is
            // the only thing an allocator costs a container that just uses the default one.
            static_cast<void>(a);
            ptr = ::operator new(mem);
        } else {
            auto chunk_alloc = rebound<A>(a);
            ptr = std::allocator_traits<rebound<A>>::allocate(chunk_alloc, num_chunks(capacity));
        }
        if (nullptr == ptr) {
            throw std::bad_alloc(); // LCOV_EXCL_LINE an allocator is supposed to throw rather than return nothing
        }
        // use void* to ensure we don't use an overload for T*
        return new (ptr) storage<T>(capacity);
    }

    /**
     * @brief Counterpart to allocator(). Does not touch the T's, they have to be destroyed already.
     */
    template <typename A>
    static void dealloc(A& a, storage<T>* ptr) {
        if constexpr (allocates_with_operator_new<A>::value) {
            static_cast<void>(a);
            std::destroy_at(ptr);
            ::operator delete(ptr);
        } else {
            // read before the header is gone: an allocator wants to be told the size it handed out
            auto const n = num_chunks(ptr->capacity());
            std::destroy_at(ptr);

            auto chunk_alloc = rebound<A>(a);
            std::allocator_traits<rebound<A>>::deallocate(chunk_alloc, reinterpret_cast<chunk_type*>(ptr), n);
        }
    }
};

/**
 * @brief Deallocates storage on the way out, unless it has been released.
 *
 * For the window in which an allocation exists but no svector holds it yet, which is where an
 * insert that has to grow runs the caller's constructors. Deleting the copy is what keeps it from
 * being handed around and freeing twice.
 *
 * Holds the allocator by value for the same reason destroy_guard does, see there.
 */
template <typename A>
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions,hicpp-special-member-functions)
class storage_guard : private allocator_holder<A> {
    using T = typename std::allocator_traits<A>::value_type;
    using holder = allocator_holder<A>;

    storage<T>* m_storage;

public:
    storage_guard(A const& alloc, storage<T>* s)
        : holder(alloc)
        , m_storage(s) {}

    storage_guard(storage_guard const&) = delete;
    auto operator=(storage_guard const&) -> storage_guard& = delete;

    ~storage_guard() {
        if (m_storage != nullptr) {
            storage<T>::dealloc(holder::allocator(), m_storage);
        }
    }

    [[nodiscard]] auto get() const -> storage<T>* {
        return m_storage;
    }

    auto release() -> storage<T>* {
        auto* s = m_storage;
        m_storage = nullptr;
        return s;
    }
};

} // namespace detail

template <typename T, size_t MinInlineCapacity, typename Allocator = std::allocator<T>>
class svector : private detail::allocator_holder<Allocator> {
    static_assert(MinInlineCapacity <= 127, "sorry, can't have more than 127 direct elements");

    using alloc_traits = std::allocator_traits<Allocator>;
    using holder = detail::allocator_holder<Allocator>;

    static_assert(std::is_same_v<T, typename alloc_traits::value_type>,
                  "the allocator has to hand out the element type, same as std::vector's does");
    static_assert(std::is_same_v<typename alloc_traits::pointer, T*>,
                  "sorry, an allocator with a fancy pointer is not supported: svector's iterator is a plain T*");

    static constexpr auto N = detail::automatic_capacity<T>(MinInlineCapacity);

    enum class direction { direct, indirect };

    using holder::allocator;

    /**
     * A buffer to hold the data of the svector Depending on direct/indirect mode, the content it holds is like so:
     *
     * direct:
     *    m_data[0] & 1: lowest bit is 1 for direct mode.
     *    m_data[0] >> 1: size for direct mode
     *    Then 0-X bytes unused (padding), and then the actual inline T data.
     * indirect:
     *    m_data[0] & 1: lowest bit is 0 for indirect mode
     *    m_data[0..sizeof(void*)-1]: stores an uintptr_t, which points to the indirect data.
     *
     * The mode flag and the pointer share m_data[0], which only holds on a little endian target.
     * There the pointer's least significant byte lands in m_data[0] and alignment guarantees its
     * low bit is 0, so the flag has a byte to live in. On a big endian target m_data[0] would be
     * the pointer's most significant byte instead and the flag would read something unrelated.
     */
    alignas(detail::alignment_of_svector<T>()) std::array<uint8_t, detail::size_of_svector<T>(MinInlineCapacity)> m_data;

    // Nothing in CI can catch this: every runner is little endian. MSVC does not define
    // __BYTE_ORDER__ but targets nothing big endian either, so check where a check is possible.
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
    static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__,
                  "svector packs its direct/indirect flag into the low byte of the indirect pointer, "
                  "which requires a little endian target");
#endif

    /**
     * @brief Whether the whole inline buffer can stand in for the elements it holds.
     *
     * Only for a trivially copyable T, and only while the buffer is small: such a copy always
     * covers the full inline capacity rather than the part in use. do_move_assign() has the
     * measurements behind the size limit; swap() rides on the same trade.
     *
     * An allocator that builds its elements its own way is left out: relocating one bytewise runs
     * neither its construct() nor its destroy(), and only the allocator knows whether that is the
     * same thing. Every stateless allocator that does not customize those is still in.
     */
    static constexpr bool relocate_by_memcpy =
        std::is_trivially_copyable_v<T> && detail::builds_elements_directly<Allocator, T>;

    static constexpr bool relocate_by_copying_m_data =
        relocate_by_memcpy && detail::size_of_svector<T>(MinInlineCapacity) <= 128U;

    /**
     * @brief Whether destroying an element does anything at all, and so whether it is worth asking.
     *
     * A trivial destructor is not enough on its own any more: an allocator with a destroy() of its
     * own has to see every element go, however little ~T() would have done.
     */
    static constexpr bool destroy_is_observable =
        !std::is_trivially_destructible_v<T> || !detail::builds_elements_directly<Allocator, T>;

    // direct mode ///////////////////////////////////////////////////////////

    [[nodiscard]] auto is_direct() const -> bool {
        return (m_data[0] & 1U) != 0U;
    }

    [[nodiscard]] auto direct_size() const -> size_t {
        return m_data[0] >> 1U;
    }

    // sets size of direct mode and mode to direct too.
    constexpr void set_direct_and_size(size_t s) {
        m_data[0] = static_cast<uint8_t>((s << 1U) | 1U);
    }

    [[nodiscard]] auto direct_data() -> T* {
        return std::launder(reinterpret_cast<T*>(m_data.data() + std::alignment_of_v<T>));
    }

    // indirect mode /////////////////////////////////////////////////////////

    [[nodiscard]] auto indirect() -> detail::storage<T>* {
        detail::storage<T>* ptr; // NOLINT(cppcoreguidelines-init-variables)
        // NOLINTNEXTLINE(bugprone-sizeof-expression,bugprone-multi-level-implicit-pointer-conversion)
        std::memcpy(&ptr, m_data.data(), sizeof(ptr));
        return ptr;
    }

    [[nodiscard]] auto indirect() const -> detail::storage<T> const* {
        return const_cast<svector*>(this)->indirect(); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }

    /**
     * @brief set_indirect() without the low bit check, for a pointer already known to be one.
     *
     * set_indirect() re-reads the byte it just wrote to make sure the mode came out right. Where
     * the pointer was just taken off a live indirect svector that is answering a question we have
     * already answered, and it is 4 of the 29 instructions of an indirect swap.
     */
    void set_indirect_unchecked(detail::storage<T>* ptr) {
        // NOLINTNEXTLINE(bugprone-sizeof-expression,bugprone-multi-level-implicit-pointer-conversion)
        std::memcpy(m_data.data(), &ptr, sizeof(ptr));
    }

    void set_indirect(detail::storage<T>* ptr) {
        // NOLINTNEXTLINE(bugprone-sizeof-expression,bugprone-multi-level-implicit-pointer-conversion)
        std::memcpy(m_data.data(), &ptr, sizeof(ptr));

        // safety check to guarantee the lowest bit is 0
        if (is_direct()) {
            throw std::bad_alloc(); // LCOV_EXCL_LINE
        }
    }

    // helpers ///////////////////////////////////////////////////////////////

    /**
     * @brief Moves size objects from source_ptr to target_ptr, and destroys what remains in source_ptr.
     *
     * Assumes data is not overlapping
     */
    void uninitialized_move_and_destroy(T* source_ptr, T* target_ptr, size_t size) {
        // the memcpy runs neither a constructor nor a destructor, so it is only the same thing when
        // the allocator would not have run anything of its own either
        if constexpr (relocate_by_memcpy) {
            std::memcpy(target_ptr, source_ptr, size * sizeof(T));
        } else {
            detail::alloc_uninitialized_move(allocator(), source_ptr, source_ptr + size, target_ptr);
            detail::alloc_destroy_n(allocator(), source_ptr, size);
        }
    }

    /**
     * @brief True when value is one of our own elements, e.g. from v.push_back(v[0]).
     *
     * Growing or shifting moves those elements around and assigns over them, so a caller taking a
     * T const& has to get it out of the way before that starts. There are two ways to do that and
     * only one of them needs this: either copy value to the stack first, which is what the callers
     * below do, or build the new element before touching anything, which is what emplace() does.
     * insert(pos, value) takes the second route, which is why it does not ask.
     *
     * Compared as uintptr_t because relational operators on pointers into different objects are
     * not specified.
     */
    [[nodiscard]] auto is_reference_into_self(T const& value) const -> bool {
        auto const p = reinterpret_cast<uintptr_t>(std::addressof(value));
        auto const first = reinterpret_cast<uintptr_t>(data());
        return p >= first && p < first + (sizeof(T) * size());
    }

    /**
     * @brief Moves all current elements into storage, frees the old one and adopts storage.
     *
     * Takes ownership of storage even when it fails: T's move constructor is allowed to throw,
     * and then nothing here has happened yet, so storage has to be freed again on the way out or
     * it leaks. num_constructed says how many elements the caller has already built at the end of
     * storage, which then have to be destroyed too. std::uninitialized_move_n already cleans up
     * whatever it managed to move itself.
     *
     * Not a template, so every emplace_back instantiation shares this instead of inlining its
     * own copy of the direct/indirect fork.
     */
    void take_over_storage(detail::storage<T>* storage, size_t new_size, size_t num_constructed = 0) {
        try {
            if (is_direct()) {
                uninitialized_move_and_destroy(data<direction::direct>(), storage->data(), size<direction::direct>());
            } else {
                uninitialized_move_and_destroy(data<direction::indirect>(), storage->data(), size<direction::indirect>());
                detail::storage<T>::dealloc(allocator(), indirect());
            }
        } catch (...) {
            detail::alloc_destroy_n(allocator(), storage->data() + new_size - num_constructed, num_constructed);
            detail::storage<T>::dealloc(allocator(), storage);
            throw;
        }
        storage->size(new_size);
        set_indirect(storage);
    }

    /**
     * @brief Grows and appends in a single step. Precondition: size() == capacity().
     *
     * args may reference one of our own elements, as in v.push_back(v[0]). Reallocating first
     * would move that element into the new storage and destroy the original, leaving args
     * dangling, so the new element is constructed into the fresh storage while the old elements
     * are still untouched. Only afterwards are they moved over.
     *
     * emplace(cend(), ...) relies on this ordering too, don't turn it back into
     * reallocate-then-construct.
     */
    template <class... Args>
    auto emplace_back_grow(size_t s, Args&&... args) -> T& {
        // s + 1 > capacity() >= N, so the new storage is always indirect
        auto fresh = detail::storage_guard<Allocator>(
            allocator(), detail::storage<T>::alloc(allocator(), calculate_new_capacity(s + 1, s)));

        auto* const element = fresh.get()->data() + s;
        alloc_traits::construct(allocator(), element, std::forward<Args>(args)...);

        // args has been consumed, the old elements can be moved now. take_over_storage() owns the
        // allocation from here whether it succeeds or not, so the guard lets go of it first; if the
        // move throws, element is the one thing already built in there, hence the 1.
        take_over_storage(fresh.release(), s + 1, 1);
        return *element;
    }

    /**
     * @brief Reallocates all data when capacity changes.
     *
     * if new_capacity <= N chooses direct memory, otherwise indirect.
     *
     * Invalidates every reference into the container, so any T const& argument that might be
     * one of our own elements has to be copied out of the way before calling this. See
     * is_reference_into_self().
     */
    void realloc(size_t new_capacity) {
        if (new_capacity <= N) {
            // put everything into direct storage
            if (is_direct()) {
                // direct -> direct: nothing to do!
                return;
            }

            // indirect -> direct
            auto* storage = indirect();
            uninitialized_move_and_destroy(storage->data(), direct_data(), storage->size());
            set_direct_and_size(storage->size());
            detail::storage<T>::dealloc(allocator(), storage);
        } else {
            // put everything into indirect storage
            take_over_storage(detail::storage<T>::alloc(allocator(), new_capacity), size());
        }
    }

    /**
     * @brief Doubles starting_capacity until it is >= size_to_fit.
     */
    [[nodiscard]] static auto calculate_new_capacity(size_t size_to_fit, size_t starting_capacity) -> size_t {
        if (size_to_fit > max_size()) {
            // Asking for more elements than can exist is a size error, not a failure to find
            // memory, and std::vector spells it the same way. bad_alloc stays for the case that
            // really is one: a size that is legal but that the allocator cannot satisfy.
            throw std::length_error("svector: requested size exceeds max_size()");
        }

        if (size_to_fit == 0) {
            // special handling for 0 so N==0 works
            return starting_capacity;
        }
        // start with at least 1, so N==0 works
        auto new_capacity = std::max<size_t>(1, starting_capacity);

        // double capacity until its large enough, but make sure we don't overflow
        while (new_capacity < size_to_fit && new_capacity * 2 > new_capacity) {
            new_capacity *= 2;
        }
        if (new_capacity < size_to_fit) {
            // got an overflow, set capacity to max
            new_capacity = max_size();
        }
        return (std::min)(new_capacity, max_size());
    }

    template <direction D>
    [[nodiscard]] auto capacity() const -> size_t {
        if constexpr (D == direction::direct) {
            return N;
        } else {
            return indirect()->capacity();
        }
    }

    template <direction D>
    [[nodiscard]] auto size() const -> size_t {
        if constexpr (D == direction::direct) {
            return direct_size();
        } else {
            return indirect()->size();
        }
    }

    template <direction D>
    void set_size(size_t s) {
        if constexpr (D == direction::direct) {
            set_direct_and_size(s);
        } else {
            indirect()->size(s);
        }
    }

    void set_size(size_t s) {
        if (is_direct()) {
            set_size<direction::direct>(s);
        } else {
            set_size<direction::indirect>(s);
        }
    }

    template <direction D>
    [[nodiscard]] auto data() -> T* {
        if constexpr (D == direction::direct) {
            return direct_data();
        } else {
            return indirect()->data();
        }
    }

    template <direction D>
    [[nodiscard]] auto data() const -> T const* {
        return const_cast<svector*>(this)->data<D>(); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }

    template <direction D>
    void pop_back() {
        if constexpr (std::is_trivially_destructible_v<T>) {
            set_size<D>(size<D>() - 1);
        } else {
            auto s = size<D>() - 1;
            (data<D>() + s)->~T();
            set_size<D>(s);
        }
    }

    /**
     * @brief resize(count) for a vector that has just been constructed, so there is nothing to
     *        shrink and no capacity to keep.
     *
     * Not resize(count): that one asks whether it has to grow, which here it always does, and gcc
     * makes the whole constructor 15 bytes larger for the question.
     */
    void resize_to_value_constructed(size_t count) {
        reserve(count);
        if (is_direct()) {
            resize_after_reserve<direction::direct>(count);
        } else {
            resize_after_reserve<direction::indirect>(count);
        }
    }

    /**
     * @brief We need variadic arguments so we can either use copy ctor or default ctor
     */
    template <direction D, class... Args>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    void resize_after_reserve(size_t count, Args&&... args) {
        auto current_size = size<D>();
        if (current_size > count) {
            if constexpr (destroy_is_observable) {
                auto* d = data<D>();
                detail::alloc_destroy(allocator(), d + count, d + current_size);
            }
        } else {
            // The hand written loop this replaces left everything it had already built behind when
            // a constructor threw: the size still said current_size, so nothing ever destroyed
            // them. These two clean up after themselves, which is the same reason insert() builds
            // through them rather than looping. See issue #70.
            //
            // Value construction, not default construction: resize(n) on an svector<int> has to
            // zero the new elements the way T() does.
            auto* const first_new = data<D>() + current_size;
            if constexpr (sizeof...(Args) == 0) {
                detail::alloc_uninitialized_value_construct_n(allocator(), first_new, count - current_size);
            } else {
                detail::alloc_uninitialized_fill_n(allocator(), first_new, count - current_size, args...);
            }
        }
        set_size<D>(count);
    }

    // Makes sure that to is not past the end iterator, and does nothing when that leaves an empty
    // range to erase
    template <direction D>
    auto erase_checked_end(T const* cfrom, T const* to) -> T* {
        auto* const erase_begin = const_cast<T*>(cfrom); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        auto* const container_end = data<D>() + size<D>();
        auto* const erase_end = (std::min)(const_cast<T*>(to), container_end); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        auto const num_erased = std::distance(erase_begin, erase_end);

        if (num_erased == 0) {
            // Not just a shortcut: std::move below would be handed a destination equal to its
            // source begin, which it does not allow, and it would self-move-assign every element
            // from here to the end. See issue #66.
            return erase_begin;
        }

        std::move(erase_end, container_end, erase_begin);
        detail::alloc_destroy(allocator(), container_end - num_erased, container_end);
        set_size<D>(size<D>() - num_erased);
        return erase_begin;
    }

    template <typename It>
    void assign(It first, It last, std::input_iterator_tag /*unused*/) {
        clear();

        // TODO this can be made faster, e.g. by setting size only when finished.
        while (first != last) {
            push_back(*first);
            ++first;
        }
    }

    template <typename It>
    void assign(It first, It last, std::forward_iterator_tag /*unused*/) {
        clear();

        auto s = std::distance(first, last);
        reserve(s);
        detail::alloc_uninitialized_copy_n(allocator(), first, static_cast<size_t>(s), data());
        set_size(s);
    }

    /**
     * @brief Whether other's allocation, if it has one, could be freed through our allocator.
     *
     * An allocation only ever goes back to an allocator that compares equal to the one it came
     * from, so this is what decides between stealing other's pointer and moving its elements over
     * one at a time. Almost always a compile time yes, and then no allocator is even looked at.
     */
    [[nodiscard]] auto can_take_over(svector const& other) const -> bool {
        if constexpr (alloc_traits::is_always_equal::value) {
            static_cast<void>(other);
            return true;
        } else {
            return allocator() == other.allocator();
        }
    }

    /**
     * @brief Takes over other's elements, and its allocation if it has one.
     *
     * Precondition: we hold nothing, and our allocator can free what other's allocated -- either
     * because they compare equal, or because ours has just been replaced by other's. Every caller
     * checks; the one that cannot moves the elements one at a time instead.
     */
    // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
    void do_move_assign(svector&& other) {
        /**
         * Everything that makes up an svector lives inside m_data: in indirect mode just the
         * pointer, in direct mode the size byte and the elements themselves. So copying the array
         * copies the whole vector, with no branch on the mode and no loop over the elements. See
         * issue #54.
         *
         * Two things have to hold. The elements must survive being relocated bytewise, which is
         * what trivially copyable buys: byte-relocating e.g. a libstdc++ std::string in its small
         * string mode leaves its data pointer aimed at other's inline buffer, which dangles as
         * soon as other is gone.
         *
         * And the object has to be small, because the copy always covers the full inline capacity
         * and not just the used part. Sorting a std::vector of svector<int> holding two elements
         * each: gcc 16 -O3 gains 31% at 24 bytes, 21% at 72 bytes, but loses 72% at 264; clang 22
         * gains 38% / 29% with the same crossover. Filling a std::vector by moving svectors into
         * it is allocation bound and gains 15% on clang while costing up to 3% on gcc.
         */
        if constexpr (relocate_by_copying_m_data) {
            m_data = other.m_data;
        } else if (!other.is_direct()) {
            // take other's memory, even when empty
            set_indirect(other.indirect());
        } else {
            auto* other_ptr = other.data<direction::direct>();
            auto s = other.size<direction::direct>();
            auto* other_end = other_ptr + s;

            detail::alloc_uninitialized_move(allocator(), other_ptr, other_end, data<direction::direct>());
            detail::alloc_destroy(allocator(), other_ptr, other_end);
            set_size(s);
        }
        other.set_direct_and_size(0);
    }

    /**
     * @brief Where insert_n() gets its new elements from: source is a range of count of them.
     *
     * construct() builds n of them, starting at the offset'th, into raw storage. assign() puts the
     * first n onto elements that are already there. An insert needs both, see insert_n().
     */
    template <typename It>
    struct place_range {
        It source;

        void construct(Allocator& alloc, T* dst, size_t offset, size_t n) const {
            detail::alloc_uninitialized_copy_n(alloc, std::next(source, static_cast<ptrdiff_t>(offset)), n, dst);
        }

        void assign(T* dst, size_t n) const {
            std::copy_n(source, n, dst);
        }
    };

    /**
     * @brief Same, for count copies of one value.
     */
    struct place_copies {
        T const& value;

        void construct(Allocator& alloc, T* dst, size_t /*offset*/, size_t n) const {
            detail::alloc_uninitialized_fill_n(alloc, dst, n, value);
        }

        void assign(T* dst, size_t n) const {
            std::fill_n(dst, n, value);
        }
    };

    static auto place_moved(T& source) -> place_range<std::move_iterator<T*>> {
        return {std::make_move_iterator(std::addressof(source))};
    }

    /**
     * @brief Inserts count elements at pos, taken from place. Returns the first one.
     *
     * Invalidates every reference into the container: the elements from pos on are either shifted
     * right or moved into a fresh allocation. A T const& argument that might be one of our own
     * elements has to be dealt with first, see is_reference_into_self().
     *
     * Only the shift below is per (direction, placer) pair; growing goes through commit_grow(),
     * which knows nothing about the placer and is one function for all of them. The whole body used
     * to be per pair, and four near identical copies of the growth path is what made each additional
     * insert form cost about 8 KB where it had been 2.8 KB. See issue #79.
     *
     * The shift is deliberately not shared as well, and that is the trade the issue is about: as a
     * function of its own it is small enough that gcc inlines it straight back, and clang, which
     * does not, pays for it. Sharing it costs a compile time count on the paths where there is one
     * -- 100 x 1000 emplace(begin()) on svector<int> spends 12.7% more instructions when a one
     * element relocation the compiler was unrolling becomes a call to memmove with a runtime length.
     * The growth path has no such constant to lose, which is why the seam sits where it does.
     *
     * There is deliberately no point in here where size() counts memory that holds no element. What
     * lands past the old end is constructed, what lands on an element that is still there is
     * assigned over it, and size() only ever grows by a step that has already happened. Opening a
     * hole and constructing into it afterwards was simpler, but it left the container undestroyable
     * for as long as the hole lasted, so every constructor that could throw needed a rollback to
     * close it again -- and getting that rollback right is what issues #68 and #74 were about.
     *
     * What it costs is the strong exception guarantee for an insert that has to assign over live
     * elements: failing part way leaves the container with the right number of elements but no
     * promise about which, and only the part before pos is certain to be untouched. That is what
     * the standard asks of insert, and what std::vector does. The paths that assign over nothing
     * are unaffected: growing builds the result in an allocation of its own, and a single element
     * goes through emplace(), which builds it before anything is touched. Growing is all or nothing
     * only as far as relocating a T is: a move constructor that throws part way through leaves our
     * own elements moved from, and then all that is left is that nothing leaks.
     */
    template <direction D, typename Place>
    auto insert_n(T const* pos, size_t count, Place const& place) -> T* {
        auto* const p = const_cast<T*>(pos); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        auto const s = size<D>();

        if (count == 0) {
            // Not just a shortcut: the std::move_backward below would be handed a destination equal
            // to its source end, which it does not allow, and it would self-move-assign every
            // element it covers. See issue #65.
            return p;
        }

        // Both written as subtractions so neither can wrap: s <= max_size() and s <= capacity()
        // always hold. It used to say s + count > capacity(), which overflowed for a huge count,
        // and the wrapped sum then looked small enough to fit, so the in place shift below ran
        // straight past the end of the buffer. See issue #69.
        if (count > max_size() - s) {
            throw std::length_error("svector: requested size exceeds max_size()");
        }

        if (count > capacity<D>() - s) {
            return insert_n_new<D>(p, count, place);
        }

        auto* const old_end = data<D>() + s;
        auto const tail = static_cast<size_t>(old_end - p);

        if (tail > count) {
            // The tail is long enough that shifting it right stays within the old elements plus
            // the count raw slots behind them, so all the new elements land on live ones.
            detail::alloc_uninitialized_move(allocator(), old_end - count, old_end, old_end);
            set_size<D>(s + count);
            std::move_backward(p, old_end - count, old_end);
            place.assign(p, count);
        } else {
            // The tail clears the ground it stood on, so the new elements behind the old end have
            // nothing under them and are built instead.
            place.construct(allocator(), old_end, tail, count - tail);
            set_size<D>(s + count - tail);
            detail::alloc_uninitialized_move(allocator(), p, old_end, p + count);
            set_size<D>(s + count);
            place.assign(p, tail);
        }
        return p;
    }

    /**
     * @brief The other half of insert_n_new(): relocates our elements around the new ones and
     *        adopts the storage they were built in. Returns the first of them.
     *
     * Deliberately out of line. This is the bulky part of an insert and the cold one, and having it
     * exist once instead of once per (direction, placer) pair is most of what splitting insert_n()
     * up buys -- gcc inlines it back into every caller otherwise. See issue #79.
     *
     * What that costs is one call per reallocation, about 28 instructions, and it is the only thing
     * about the split that costs anything: without the attribute a growth-only workload measures
     * within 0.05% of the code this replaces, with it svector<int> pays 1.8%. Only a T that
     * relocates by memcpy is fast enough for a fixed 28 to show up at all; for std::string the same
     * workload is unchanged. That is the trade for ~2 KB per insert form.
     */
    template <direction D>
    ANKERL_SVECTOR_NOINLINE auto commit_grow(T* p, size_t count, detail::storage_guard<Allocator>& fresh) -> T* {
        auto const s = size<D>();
        auto* const old_data = data<D>();
        auto* const dst = fresh.get()->data();
        auto* const gap = dst + (p - old_data);

        // What is built now is the gap but nothing in front of it, and no size can say that, so for
        // as long as the hole lasts the cleanup is spelled out here. Leaving it to the destructor of
        // a container that sees a size of zero is what leaked the relocated elements in issue #74.
        // Both moves below destroy whatever they managed to build themselves.
        auto guard = detail::destroy_guard<Allocator>(allocator(), gap, gap + count);
        detail::alloc_uninitialized_move(allocator(), old_data, p, dst);
        guard.extend_front(dst);
        detail::alloc_uninitialized_move(allocator(), p, old_data + s, gap + count);
        guard.release();

        // Nothing below throws, so this is where the new storage stops being the guard's.
        auto* const storage = fresh.release();
        storage->size(s + count);
        destroy(); // our elements are all moved from now, and the old allocation can go
        set_indirect(storage);
        return gap;
    }

    /**
     * @brief insert_n() for when the elements no longer fit. Builds the result in fresh storage.
     *
     * The allocation is the guard's until commit_grow() takes it: place.construct() is where the
     * caller's constructors run, nothing of ours has moved yet at that point, so an exception there
     * means the insert simply has not happened and the storage has to go back.
     */
    template <direction D, typename Place>
    auto insert_n_new(T* p, size_t count, Place const& place) -> T* {
        auto const s = size<D>();

        // the capacity reserve(s + count) on an empty svector would pick, which is where this used
        // to start before it built the result itself
        auto fresh = detail::storage_guard<Allocator>(
            allocator(), detail::storage<T>::alloc(allocator(), calculate_new_capacity(s + count, N)));
        auto* const gap = fresh.get()->data() + (p - data<D>());

        // The new elements before ours: nothing of ours has moved yet if this throws, so the insert
        // simply has not happened, and place can still read the elements we hold.
        place.construct(allocator(), gap, 0, count);
        return commit_grow<D>(p, count, fresh);
    }

    template <typename Place>
    auto insert_n(T const* pos, size_t count, Place const& place) -> T* {
        if (is_direct()) {
            return insert_n<direction::direct>(pos, count, place);
        }
        return insert_n<direction::indirect>(pos, count, place);
    }

    /**
     * @brief swap() for when both hold their elements inline.
     *
     * Exchanges as far as both of them reach, then hands the remainder of the longer one over.
     */
    // uninitialized_move_and_destroy() is the obvious helper for the tail below and is the wrong
    // choice: its memcpy specialisation for a trivially copyable T becomes an out of line call,
    // which loses to the loop the compiler inlines here. Measured on svector<uint64_t, 40>, which
    // is trivially copyable and too big for the whole buffer path, so it lands right here: the
    // helper is 70 instructions against 157, and 8.93 ns against 5.69.
    void swap_direct(svector& other) {
        auto const s = direct_size();
        auto const other_s = other.direct_size();
        auto* const mine = direct_data();
        auto* const theirs = other.direct_data();
        auto const common = (std::min)(s, other_s);

        std::swap_ranges(mine, mine + common, theirs);
        if (s > other_s) {
            detail::alloc_uninitialized_move(allocator(), mine + common, mine + s, theirs + common);
            detail::alloc_destroy(allocator(), mine + common, mine + s);
        } else {
            detail::alloc_uninitialized_move(allocator(), theirs + common, theirs + other_s, mine + common);
            detail::alloc_destroy(allocator(), theirs + common, theirs + other_s);
        }

        set_direct_and_size(other_s);
        other.set_direct_and_size(s);
    }

    /**
     * @brief swap() for when dir holds its elements inline and ind holds a pointer.
     *
     * The pointer has to be read out before dir's elements are moved on top of it: when T's
     * alignment is below sizeof(void*) the inline storage starts inside the bytes the pointer
     * occupies.
     */
    static void swap_direct_with_indirect(svector& dir, svector& ind) {
        auto* const storage = ind.indirect();
        auto const s = dir.direct_size();
        auto* const from = dir.direct_data();

        detail::alloc_uninitialized_move(dir.allocator(), from, from + s, ind.direct_data());
        detail::alloc_destroy(dir.allocator(), from, from + s);

        ind.set_direct_and_size(s);
        dir.set_indirect(storage);
    }

    void destroy() {
        auto const is_dir = is_direct();
        if constexpr (destroy_is_observable) {
            T* ptr = nullptr;
            size_t s = 0;
            if (is_dir) {
                ptr = data<direction::direct>();
                s = size<direction::direct>();
            } else {
                ptr = data<direction::indirect>();
                s = size<direction::indirect>();
            }
            detail::alloc_destroy_n(allocator(), ptr, s);
        }
        if (!is_dir) {
            detail::storage<T>::dealloc(allocator(), indirect());
        }
        set_direct_and_size(0);
    }

    // performs a const_cast so we don't need this implementation twice
    template <direction D>
    auto at(size_t idx) -> T& {
        if (idx >= size<D>()) {
            throw std::out_of_range{"svector: idx out of range"};
        }
        auto* ptr = const_cast<T*>(data<D>() + idx); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        return *ptr;
    } // LCOV_EXCL_LINE why is this single } marked as not covered? gcov bug?

public:
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = value_type const&;
    using pointer = T*;
    using const_pointer = T const*;
    using iterator = T*;
    using const_iterator = T const*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    svector() noexcept(std::is_nothrow_default_constructible_v<Allocator>) {
        set_direct_and_size(0);
    }

    explicit svector(Allocator const& alloc) noexcept
        : holder(alloc) {
        set_direct_and_size(0);
    }

    /**
     * Every constructor that takes an allocator is spelled twice rather than once with an
     * `Allocator const& = Allocator()` default argument. A default argument of class type is a
     * temporary the caller has to materialize and pass the address of, and it does that even for an
     * allocator with nothing in it, so `svector<int, 7> v(n)` grew a stack slot, a lea and a third
     * argument register over what it cost before there was an allocator at all -- 39 bytes to 47 on
     * clang -Os. The second overload is cheaper than the default argument it replaces.
     */
    explicit svector(size_t count)
        : svector() {
        resize_to_value_constructed(count);
    }

    svector(size_t count, Allocator const& alloc)
        : svector(alloc) {
        resize_to_value_constructed(count);
    }

    svector(size_t count, T const& value)
        : svector() {
        resize(count, value);
    }

    svector(size_t count, T const& value, Allocator const& alloc)
        : svector(alloc) {
        resize(count, value);
    }

    template <typename InputIt, typename = detail::enable_if_t<detail::is_input_iterator<InputIt>>>
    svector(InputIt first, InputIt last)
        : svector() {
        assign(first, last);
    }

    template <typename InputIt, typename = detail::enable_if_t<detail::is_input_iterator<InputIt>>>
    svector(InputIt first, InputIt last, Allocator const& alloc)
        : svector(alloc) {
        assign(first, last);
    }

#if ANKERL_SVECTOR_HAS_RANGES
    template <detail::container_compatible_range<T> R>
    svector(std::from_range_t /*unused*/, R&& rg)
        : svector() {
        append_range(std::forward<R>(rg));
    }

    template <detail::container_compatible_range<T> R>
    svector(std::from_range_t /*unused*/, R&& rg, Allocator const& alloc)
        : svector(alloc) {
        append_range(std::forward<R>(rg));
    }
#endif

    /**
     * @brief Copying asks the allocator which one the copy should use.
     *
     * That is select_on_container_copy_construction(), and for most allocators it hands back the
     * same one. An allocator that owns an arena is where it does not: it can say that a copy starts
     * from a default constructed one instead of sharing the arena.
     *
     * It builds the holder from that answer rather than handing it to the constructor below, which
     * would be the same materialized temporary the note above is about, on every copy of every
     * svector.
     */
    svector(svector const& other)
        : holder(alloc_traits::select_on_container_copy_construction(other.allocator())) {
        set_direct_and_size(0);
        auto s = other.size();
        reserve(s);
        detail::alloc_uninitialized_copy_n(allocator(), other.begin(), s, begin());
        set_size(s);
    }

    svector(svector const& other, Allocator const& alloc)
        : svector(alloc) {
        auto s = other.size();
        reserve(s);
        detail::alloc_uninitialized_copy_n(allocator(), other.begin(), s, begin());
        set_size(s);
    }

    /**
     * @brief Moving is only noexcept when moving a T is.
     *
     * std::vector can promise this unconditionally because its move only steals a pointer. In
     * direct mode we have to relocate the inline elements instead, which calls T's move
     * constructor, so the promise is only ours to make when that one is noexcept. Claiming it
     * anyway turns a throwing move into std::terminate, and makes std::move_if_noexcept pick us
     * up for a move where it should have fallen back to a copy. See issue #63.
     *
     * The allocator comes along, so whatever other holds can be taken as it is. Copying one is not
     * allowed to throw, so it adds no condition here.
     */
    svector(svector&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
        : holder(other.allocator()) {
        set_direct_and_size(0);
        do_move_assign(std::move(other));
    }

    /**
     * @brief Moving into a named allocator, which may not be the one other's memory came from.
     *
     * Then there is nothing to take over: an allocation can only go back to an allocator that
     * compares equal to the one that handed it out, so the elements are moved one at a time.
     */
    svector(svector&& other, Allocator const& alloc)
        : svector(alloc) {
        if (can_take_over(other)) {
            do_move_assign(std::move(other));
        } else {
            assign(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
        }
    }

    svector(std::initializer_list<T> init)
        : svector(init.begin(), init.end()) {}

    svector(std::initializer_list<T> init, Allocator const& alloc)
        : svector(init.begin(), init.end(), alloc) {}

    ~svector() {
        destroy();
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    void assign(size_t count, T const& value) {
        if (is_reference_into_self(value)) {
            // clear() destroys every element, including the one value refers to.
            // Copy it to the stack first, then it's an ordinary assign. v.assign(1000, v[0])
            auto const tmp = value; // NOLINT(performance-unnecessary-copy-initialization)
            assign(count, tmp);
            return;
        }
        clear();
        resize(count, value);
    }

    template <typename InputIt, typename = detail::enable_if_t<detail::is_input_iterator<InputIt>>>
    void assign(InputIt first, InputIt last) {
        assign(first, last, typename std::iterator_traits<InputIt>::iterator_category());
    }

    void assign(std::initializer_list<T> l) {
        assign(l.begin(), l.end());
    }

    /**
     * @brief Copies other's elements, and other's allocator when the allocator asks for it.
     *
     * propagate_on_container_copy_assignment is that ask. What we hold came from the allocator
     * that is about to be replaced, and only that one can take it back, so it goes first.
     */
    auto operator=(svector const& other) -> svector& {
        if (&other == this) {
            return *this;
        }

        if constexpr (alloc_traits::propagate_on_container_copy_assignment::value) {
            if (!can_take_over(other)) {
                destroy();
            }
            allocator() = other.allocator();
        }

        assign(other.begin(), other.end());
        return *this;
    }

    /**
     * @brief Conditional for the same reason as the move constructor, plus one of its own.
     *
     * Without propagate_on_container_move_assignment an allocator that does not compare equal to
     * other's leaves nothing to steal, and moving the elements across can throw, so the promise is
     * only there when it cannot come to that.
     */
    auto operator=(svector&& other) noexcept(std::is_nothrow_move_constructible_v<T> &&
                                             (alloc_traits::propagate_on_container_move_assignment::value ||
                                              alloc_traits::is_always_equal::value)) -> svector& {
        if (&other == this) {
            // It doesn't seem to be required to do self-check, but let's do it anyways to be safe
            return *this;
        }

        if constexpr (alloc_traits::propagate_on_container_move_assignment::value) {
            destroy(); // still ours to free, the allocator that gave it to us is on the next line
            allocator() = other.allocator();
        } else if (!can_take_over(other)) {
            // Two allocators that do not know about each other's memory: all that is left is to
            // move the elements themselves, and other keeps its allocation and its moved from
            // elements. The standard asks no more of it than to be usable afterwards.
            assign(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
            return *this;
        } else {
            destroy();
        }

        do_move_assign(std::move(other));
        return *this;
    }

    auto operator=(std::initializer_list<T> l) -> svector& {
        assign(l.begin(), l.end());
        return *this;
    }

    void resize(size_t count) {
        if (count > capacity()) {
            reserve(count);
        }
        if (is_direct()) {
            resize_after_reserve<direction::direct>(count);
        } else {
            resize_after_reserve<direction::indirect>(count);
        }
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    void resize(size_t count, T const& value) {
        if (is_reference_into_self(value)) {
            // reserve() below moves the elements into new storage and destroys the originals,
            // so value has to be copied out of the way first. v.resize(1000, v[0])
            // Deliberately not also testing count > capacity(): duplicating the condition
            // below would silently stop matching if that one ever changes.
            auto const tmp = value; // NOLINT(performance-unnecessary-copy-initialization)
            resize(count, tmp);
            return;
        }
        if (count > capacity()) {
            reserve(count);
        }
        if (is_direct()) {
            resize_after_reserve<direction::direct>(count, value);
        } else {
            resize_after_reserve<direction::indirect>(count, value);
        }
    }

    void reserve(size_t s) {
        auto old_capacity = capacity();
        auto new_capacity = calculate_new_capacity(s, old_capacity);
        if (new_capacity > old_capacity) {
            realloc(new_capacity);
        }
    }

    [[nodiscard]] auto capacity() const noexcept -> size_t {
        if (is_direct()) {
            return capacity<direction::direct>();
        }
        return capacity<direction::indirect>();
    }

    [[nodiscard]] auto size() const noexcept -> size_t {
        if (is_direct()) {
            return size<direction::direct>();
        }
        return size<direction::indirect>();
    }

    [[nodiscard]] auto data() noexcept -> T* {
        if (is_direct()) {
            return direct_data();
        }
        return indirect()->data();
    }

    [[nodiscard]] auto data() const noexcept -> T const* {
        return const_cast<svector*>(this)->data(); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }

    /**
     * @brief Appends one element, growing first if there is no room.
     *
     * Written as two whole paths rather than one path that asks which mode it is in three times.
     * Constructing the element writes through a T*, which the compiler cannot prove does not alias
     * our own bytes, so every is_direct() after it is a fresh load and every indirect() after it is
     * a fresh pointer chase. Deciding once and carrying the answer in a register is worth about a
     * third of the instructions of an append.
     */
    template <class... Args>
    auto emplace_back(Args&&... args) -> T& {
        if (is_direct()) {
            auto const s = direct_size();
            if (s != N) {
                // construct before recording the size, so a throwing constructor does not leave
                // the vector claiming an element that was never built
                auto* const element = direct_data() + s;
                alloc_traits::construct(allocator(), element, std::forward<Args>(args)...);
                set_direct_and_size(s + 1);
                return *element;
            }
            return emplace_back_grow(s, std::forward<Args>(args)...);
        }

        auto* const storage = indirect();
        auto const s = storage->size();
        if (s != storage->capacity()) {
            auto* const element = storage->data() + s;
            alloc_traits::construct(allocator(), element, std::forward<Args>(args)...);
            storage->size(s + 1);
            return *element;
        }
        return emplace_back_grow(s, std::forward<Args>(args)...);
    }

    void push_back(T const& value) {
        emplace_back(value);
    }

    void push_back(T&& value) {
        emplace_back(std::move(value));
    }

    [[nodiscard]] auto operator[](size_t idx) const noexcept -> T const& {
        return *(data() + idx);
    }

    [[nodiscard]] auto operator[](size_t idx) noexcept -> T& {
        return *(data() + idx);
    }

    auto at(size_t idx) -> T& {
        if (is_direct()) {
            return at<direction::direct>(idx);
        }
        return at<direction::indirect>(idx);
    }

    auto at(size_t idx) const -> T const& {
        return const_cast<svector*>(this)->at(idx); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }

    [[nodiscard]] auto begin() const noexcept -> T const* {
        return data();
    }

    [[nodiscard]] auto cbegin() const noexcept -> T const* {
        return begin();
    }

    [[nodiscard]] auto begin() noexcept -> T* {
        return data();
    }

    [[nodiscard]] auto end() noexcept -> T* {
        if (is_direct()) {
            return data<direction::direct>() + size<direction::direct>();
        }
        return data<direction::indirect>() + size<direction::indirect>();
    }

    [[nodiscard]] auto end() const noexcept -> T const* {
        return const_cast<svector*>(this)->end(); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }

    [[nodiscard]] auto cend() const noexcept -> T const* {
        return end();
    }

    [[nodiscard]] auto rbegin() noexcept -> reverse_iterator {
        return reverse_iterator{end()};
    }

    [[nodiscard]] auto rbegin() const noexcept -> const_reverse_iterator {
        return crbegin();
    }

    [[nodiscard]] auto crbegin() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator{end()};
    }

    [[nodiscard]] auto rend() noexcept -> reverse_iterator {
        return reverse_iterator{begin()};
    }

    [[nodiscard]] auto rend() const noexcept -> const_reverse_iterator {
        return crend();
    }

    [[nodiscard]] auto crend() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator{begin()};
    }

    [[nodiscard]] auto front() const noexcept -> T const& {
        return *data();
    }

    [[nodiscard]] auto front() noexcept -> T& {
        return *data();
    }

    [[nodiscard]] auto back() noexcept -> T& {
        if (is_direct()) {
            return *(data<direction::direct>() + size<direction::direct>() - 1);
        }
        return *(data<direction::indirect>() + size<direction::indirect>() - 1);
    }

    [[nodiscard]] auto back() const noexcept -> T const& {
        return const_cast<svector*>(this)->back(); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }

    void clear() noexcept {
        if constexpr (destroy_is_observable) {
            detail::alloc_destroy(allocator(), begin(), end());
        }

        if (is_direct()) {
            set_size<direction::direct>(0);
        } else {
            set_size<direction::indirect>(0);
        }
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return 0U == size();
    }

    void pop_back() noexcept {
        if (is_direct()) {
            pop_back<direction::direct>();
        } else {
            pop_back<direction::indirect>();
        }
    }

    /**
     * @brief Deliberately not min()'d with the allocator's own max_size().
     *
     * An allocator with a smaller limit of its own still holds: asking it for more than it has
     * throws out of allocate(), which is where a container that had min()'d would have ended up
     * anyway. What that limit cannot do is be read without an allocator to read it from, and this
     * function has been public and static since before there was one -- svector<T, N>::max_size()
     * is spelled that way in test/unit/insert.cpp. Making it a non-static member is a breaking
     * change and belongs to a major version, not to adding an allocator.
     *
     * Divided by sizeof(T) because a count is not a byte count. It used to answer PTRDIFF_MAX for
     * every T, which claimed a size no svector could reach: alloc() refuses anything whose bytes
     * pass PTRDIFF_MAX, so the real ceiling has always been this. std::vector answers the same.
     */
    [[nodiscard]] static auto max_size() noexcept -> size_t {
        return static_cast<size_t>((std::numeric_limits<std::ptrdiff_t>::max)()) / sizeof(T);
    }

    [[nodiscard]] auto get_allocator() const noexcept -> Allocator {
        return allocator();
    }

    /**
     * @brief Exchanges the contents with other.
     *
     * std::swap(*this, other) would do it in three whole container moves, and in direct mode a
     * container move is every element moved and the original destroyed. Two indirect svectors need
     * not touch an element at all, and a mixed pair only has to relocate the inline side.
     *
     * Measured against the three move version, gcc 16 -O3, inline capacity 7, ns per swap:
     * two heap std::string 22.9 -> 15.5, unequal sizes 23.0 -> 16.6, uint64_t 2.9 -> 2.3,
     * indirect 1.7 -> 1.3, mixed 15.0 -> 11.3. One case is worse: two equal length runs of short
     * strings, 28.5 -> 34.8, because std::swap_ranges finds std::string::swap, and for a string
     * small enough to live inside itself that is three copies of the internal buffer where a
     * relocation would have been one. That is libstdc++'s trade, not one this can pick around, and
     * it buys the other six.
     *
     * The condition is what the work below actually needs: relocating between the two inline
     * buffers is a move construction, and exchanging elements in place is a swap.
     *
     * The allocators are exchanged too when propagate_on_container_swap says so. When it does not
     * and they do not compare equal the standard says the behaviour is undefined, and this makes no
     * attempt to be nice about it: the two allocations would simply change hands.
     */
    void swap(svector& other) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T> &&
                                       (alloc_traits::propagate_on_container_swap::value ||
                                        alloc_traits::is_always_equal::value)) {
        if (this == &other) {
            return;
        }

        if constexpr (alloc_traits::propagate_on_container_swap::value) {
            using std::swap;
            swap(allocator(), other.allocator());
        }

        auto const is_dir = is_direct();
        auto const other_is_dir = other.is_direct();

        if (!is_dir && !other_is_dir) {
            // both on the heap, so nothing but the two pointers moves
            auto* const mine = indirect();
            set_indirect_unchecked(other.indirect());
            other.set_indirect_unchecked(mine);
        } else if constexpr (relocate_by_copying_m_data) {
            // m_data is the whole of an svector whichever mode it is in, so for a T that can be
            // relocated by copying bytes this exchanges everything at once, and vectorizes
            std::swap(m_data, other.m_data);
        } else if (is_dir && other_is_dir) {
            swap_direct(other);
        } else if (is_dir) {
            swap_direct_with_indirect(*this, other);
        } else {
            swap_direct_with_indirect(other, *this);
        }
    }

    void shrink_to_fit() {
        // per the standard we wouldn't need to do anything here. But since we are so nice,
        // let's do the shrink.
        auto const c = capacity();
        auto const s = size();
        if (s >= c) {
            return;
        }

        auto new_capacity = calculate_new_capacity(s, N);
        if (new_capacity == c) {
            // nothing change!
            return;
        }

        realloc(new_capacity);
    }

    template <class... Args>
    auto emplace(const_iterator pos, Args&&... args) -> iterator {
        if (pos == cend()) {
            // no elements have to move out of the way, and emplace_back already builds the
            // new element before it grows, so args referencing us is fine there
            return std::addressof(emplace_back(std::forward<Args>(args)...));
        }

        // args may reference one of our own elements, and making space either shifts those
        // elements right or moves them into a new allocation, either of which leaves args
        // dangling. Build the element first. Inserting in the middle already moves every
        // element after pos, so one extra move does not change the cost.
        //
        // tmp is a plain local and not built through the allocator, which only shows for an
        // allocator that hands its elements something, e.g. a std::pmr::string built here holds the
        // default resource until it is moved into place. What ends up in the container is
        // constructed from it through the allocator, so it holds the right one; what it costs is
        // that the move is a copy when the two resources differ.
        auto tmp = T(std::forward<Args>(args)...);
        return insert_n(pos, 1, place_moved(tmp));
    }

    // Both of these want the element built before anything is shifted, which is what emplace()
    // does, so they go there rather than say it again. See insert_n() for what that buys.
    auto insert(const_iterator pos, T const& value) -> iterator {
        return emplace(pos, value);
    }

    auto insert(const_iterator pos, T&& value) -> iterator {
        return emplace(pos, std::move(value));
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    auto insert(const_iterator pos, size_t count, T const& value) -> iterator {
        if (is_reference_into_self(value)) {
            // the shift moves our elements around and assigns over them, so value has to be
            // copied out of the way first. v.insert(v.begin(), 1000, v[0])
            auto const tmp = value; // NOLINT(performance-unnecessary-copy-initialization)
            return insert(pos, count, tmp);
        }
        return insert_n(pos, count, place_copies{value});
    }

    template <typename It>
    auto insert(const_iterator pos, It first, It last, std::input_iterator_tag /*unused*/) {
        if (!(first != last)) {
            return const_cast<T*>(pos); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        }

        // just input_iterator_tag makes this very slow. Let's do the same as the STL.
        if (pos == end()) {
            auto s = size();
            while (first != last) {
                emplace_back(*first);
                ++first;
            }
            return begin() + s;
        }

        auto tmp = svector(first, last, allocator());
        return insert(pos, std::make_move_iterator(tmp.begin()), std::make_move_iterator(tmp.end()));
    }

    template <typename It>
    auto insert(const_iterator pos, It first, It last, std::forward_iterator_tag /*unused*/) -> iterator {
        auto const count = static_cast<size_t>(std::distance(first, last));
        return insert_n(pos, count, place_range<It>{first});
    }

    template <typename InputIt, typename = detail::enable_if_t<detail::is_input_iterator<InputIt>>>
    auto insert(const_iterator pos, InputIt first, InputIt last) -> iterator {
        return insert(pos, first, last, typename std::iterator_traits<InputIt>::iterator_category());
    }

#if ANKERL_SVECTOR_HAS_RANGES
    /**
     * @brief The C++23 range members, which std::vector has and this did not.
     *
     * All of them go through the iterator pair members, so a range gets the same growth, the same
     * exception guarantees and the same self referencing checks an iterator pair already got,
     * rather than a second implementation of all three.
     *
     * A range cannot always be handed over as a pair: its sentinel need not be its iterator, and
     * its iterator need not publish an iterator_category. One that cannot is built into a
     * temporary first. That costs an allocation for exactly the ranges that could not have been
     * sized anyway.
     */
    template <typename R>
    static constexpr bool is_iterator_pair_range =
        std::ranges::common_range<R> && detail::has_iterator_category<std::ranges::iterator_t<R>>::value;

    template <detail::container_compatible_range<T> R>
    auto insert_range(const_iterator pos, R&& rg) -> iterator {
        if constexpr (is_iterator_pair_range<R>) {
            return insert(pos, std::ranges::begin(rg), std::ranges::end(rg));
        } else {
            auto tmp = svector(allocator());
            for (auto&& element : rg) {
                tmp.emplace_back(std::forward<decltype(element)>(element));
            }
            return insert(pos, std::make_move_iterator(tmp.begin()), std::make_move_iterator(tmp.end()));
        }
    }

    template <detail::container_compatible_range<T> R>
    void append_range(R&& rg) {
        static_cast<void>(insert_range(cend(), std::forward<R>(rg)));
    }

    template <detail::container_compatible_range<T> R>
    void assign_range(R&& rg) {
        if constexpr (is_iterator_pair_range<R>) {
            assign(std::ranges::begin(rg), std::ranges::end(rg));
        } else {
            clear();
            append_range(std::forward<R>(rg));
        }
    }
#endif

    auto insert(const_iterator pos, std::initializer_list<T> l) -> iterator {
        return insert(pos, l.begin(), l.end());
    }

    /**
     * @brief Resizes to count elements, letting op initialize the new ones in place.
     *
     * Same contract as std::string::resize_and_overwrite, see
     * https://en.cppreference.com/w/cpp/string/basic_string/resize_and_overwrite
     *
     * op is called as op(p, count) with p == data(), and returns the actual new size:
     *  * p[0, min(count, size())) are the existing elements, readable and assignable.
     *  * p[min(count, size()), count) is raw uninitialized storage. op has to construct
     *    every element it wants to keep, e.g. with placement new.
     *  * op returns r, which must be in [0, count]. Afterwards size() == r, so p[0, r)
     *    must all be constructed objects when op returns.
     *
     * This skips the value-initialization that resize() would do, which is what makes it
     * faster: for e.g. reading into an svector<char> the zero fill is pure overhead.
     */
    template <class Operation>
    void resize_and_overwrite(size_t count, Operation op) {
        // step 1: make room. This preserves the existing elements and may switch to indirect mode.
        reserve(count);

        auto const old_size = size();
        if (count < old_size) {
            // Shrinking: the tail is gone. Commit the smaller size *before* running op, so that
            // if op throws, the destructor sees exactly the elements that are still alive.
            detail::alloc_destroy_n(allocator(), data() + count, old_size - count);
            set_size(count);
        }

        // step 2: op initializes [min(count, old_size), count) and tells us how much it kept.
        // The stored size is still min(count, old_size) here, so an exception escaping op
        // destroys the untouched prefix and leaks only what op itself constructed.
        auto const new_size = std::move(op)(data(), count);

        // step 3: commit. new_size <= count <= capacity() is a precondition, so in direct mode
        // this can never overflow the 7 bit size field.
        set_size(new_size);
    }

    auto erase(const_iterator pos) -> iterator {
        return erase(pos, pos + 1);
    }

    auto erase(const_iterator first, const_iterator last) -> iterator {
        if (is_direct()) {
            return erase_checked_end<direction::direct>(first, last);
        }
        return erase_checked_end<direction::indirect>(first, last);
    }
};

template <typename T, size_t NA, typename AA, size_t NB, typename AB>
[[nodiscard]] auto operator==(svector<T, NA, AA> const& a, svector<T, NB, AB> const& b) -> bool {
    return std::equal(a.begin(), a.end(), b.begin(), b.end());
}

template <typename T, size_t NA, typename AA, size_t NB, typename AB>
[[nodiscard]] auto operator!=(svector<T, NA, AA> const& a, svector<T, NB, AB> const& b) -> bool {
    return !(a == b);
}

template <typename T, size_t NA, typename AA, size_t NB, typename AB>
[[nodiscard]] auto operator<(svector<T, NA, AA> const& a, svector<T, NB, AB> const& b) -> bool {
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}

template <typename T, size_t NA, typename AA, size_t NB, typename AB>
[[nodiscard]] auto operator>=(svector<T, NA, AA> const& a, svector<T, NB, AB> const& b) -> bool {
    return !(a < b);
}

template <typename T, size_t NA, typename AA, size_t NB, typename AB>
[[nodiscard]] auto operator>(svector<T, NA, AA> const& a, svector<T, NB, AB> const& b) -> bool {
    return std::lexicographical_compare(b.begin(), b.end(), a.begin(), a.end());
}

template <typename T, size_t NA, typename AA, size_t NB, typename AB>
[[nodiscard]] auto operator<=(svector<T, NA, AA> const& a, svector<T, NB, AB> const& b) -> bool {
    return !(a > b);
}

/**
 * @brief Found by argument dependent lookup, so std::swap and everything built on it get the
 * member swap rather than the generic three move version.
 *
 * Only for two svectors of the same inline capacity; the generic one cannot exchange those either.
 */
template <typename T, size_t N, typename A>
void swap(svector<T, N, A>& a, svector<T, N, A>& b) noexcept(noexcept(a.swap(b))) {
    a.swap(b);
}

} // namespace ANKERL_SVECTOR_NAMESPACE
} // namespace ankerl

namespace std {
// NOLINTNEXTLINE(cert-dcl58-cpp)
inline namespace ANKERL_SVECTOR_NAMESPACE {

template <class T, size_t N, class A, class U>
constexpr auto erase(ankerl::svector<T, N, A>& sv, U const& value) -> typename ankerl::svector<T, N, A>::size_type {
    auto* removed_begin = std::remove(sv.begin(), sv.end(), value);
    auto num_removed = std::distance(removed_begin, sv.end());
    sv.erase(removed_begin, sv.end());
    return num_removed;
}

template <class T, size_t N, class A, class Pred>
constexpr auto erase_if(ankerl::svector<T, N, A>& sv, Pred pred) -> typename ankerl::svector<T, N, A>::size_type {
    auto* removed_begin = std::remove_if(sv.begin(), sv.end(), pred);
    auto num_removed = std::distance(removed_begin, sv.end());
    sv.erase(removed_begin, sv.end());
    return num_removed;
}

} // namespace ANKERL_SVECTOR_NAMESPACE
} // namespace std

#endif
