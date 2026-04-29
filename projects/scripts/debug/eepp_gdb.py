import gdb


class EESmallVectorPrinter:
    """Pretty Printer for EE::SmallVector (ankerl::svector)"""

    def __init__(self, val):
        self.val = val
        # Get the underlying uint8_t array
        self.m_data = self.val["m_data"]["_M_elems"]
        self.type_t = self.val.type.template_argument(0)

    def is_direct(self):
        # bit 0 of the first byte is the discriminator
        return (int(self.m_data[0]) & 1) != 0

    def to_string(self):
        if self.is_direct():
            size = int(self.m_data[0]) >> 1
            return f"EE::SmallVector<{self.type_t}> [Direct] (size={size})"
        else:
            # For indirect, we have to fetch the pointer from m_data
            void_ptr = self.m_data.address.cast(
                gdb.lookup_type("void").pointer().pointer()
            ).dereference()
            storage_ptr = void_ptr.cast(
                gdb.lookup_type(f"ankerl::v1_0_3::detail::storage<{self.type_t}>").pointer()
            )
            size = int(storage_ptr["m_size"])
            cap = int(storage_ptr["m_capacity"])
            return f"EE::SmallVector<{self.type_t}> [Indirect] (size={size}, capacity={cap})"

    def children(self):
        # We need a char pointer type to do proper byte-level pointer math
        char_ptr_type = gdb.lookup_type("char").pointer()
        align_t = self.type_t.alignof

        if self.is_direct():
            size = int(self.m_data[0]) >> 1
            base_addr = self.m_data.address.cast(char_ptr_type)
            data_ptr = (base_addr + align_t).cast(self.type_t.pointer())
        else:
            void_ptr = self.m_data.address.cast(
                gdb.lookup_type("void").pointer().pointer()
            ).dereference()

            storage_ptr = void_ptr.cast(
                gdb.lookup_type(f"ankerl::v1_0_3::detail::storage<{self.type_t}>").pointer()
            )
            size = int(storage_ptr["m_size"])

            # C++ logic: offset_to_data = detail::round_up(sizeof(header), alignment_of_t)
            # header contains size_t m_size and size_t m_capacity, so sizeof(header) is usually 16.
            # Get the exact size of size_t for the current architecture
            size_t_type = gdb.lookup_type("size_t")
            header_size = size_t_type.sizeof * 2  # header has two size_t fields

            # Translate svector's detail::round_up(sizeof(header), alignment_of_t)
            offset_to_data = ((header_size + align_t - 1) // align_t) * align_t

            data_ptr = (storage_ptr.cast(char_ptr_type) + offset_to_data).cast(
                self.type_t.pointer()
            )

        for i in range(size):
            yield f"[{i}]", (data_ptr + i).dereference()


def register_ee_printers(obj):
    if obj is None:
        obj = gdb
    obj.pretty_printers.append(
        lambda val: EESmallVectorPrinter(val) if "svector" in str(val.type) else None
    )


register_ee_printers(gdb.current_objfile())
