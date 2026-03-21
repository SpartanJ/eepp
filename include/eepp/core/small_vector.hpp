#pragma once

#include <eepp/thirdparty/svector.h>

namespace EE {

template <typename T, size_t MinInlineCapacity = 16>
class SmallVector : public ankerl::svector<T, MinInlineCapacity> {
  public:
	using ankerl::svector<T, MinInlineCapacity>::svector; // Inherit constructors

	/**
     * @return The total number of bytes this object occupies on the stack.
     */
    static constexpr size_t stack_size() {
        return sizeof(SmallVector<T, MinInlineCapacity>);
    }

    /**
     * @return True if the vector is currently using its inline stack buffer.
     */
    bool is_small() const {
        // Based on the svector.h logic: bit 0 of m_data[0] is 1 for direct mode
        // We use the public capacity() check for safety
        return this->capacity() <= MinInlineCapacity;
    }
};

} // namespace EE
