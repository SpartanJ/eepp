import lldb


class EESmallVectorSyntheticProvider:
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        self.update()

    def update(self):
        self.size = 0
        self.capacity = 0
        self.is_direct = True
        self.data_addr = lldb.LLDB_INVALID_ADDRESS

        # 1. Get the type of T (e.g., Client*)
        self.type_t = self.valobj.GetType().GetTemplateArgumentType(0)

        # 2. Find the m_data array
        self.m_data = self.valobj.GetChildMemberWithName("m_data")
        if not self.m_data.IsValid():
            return

        process = self.valobj.GetProcess()
        addr = self.m_data.GetLoadAddress()
        error = lldb.SBError()

        if addr == lldb.LLDB_INVALID_ADDRESS or not process.IsValid():
            return

        # 3. Read the first byte (the discriminator & size)
        first_byte = process.ReadUnsignedIntegerFromMemory(addr, 1, error)
        self.is_direct = (first_byte & 1) != 0

        ptr_size = process.GetAddressByteSize()

        if self.is_direct:
            self.size = first_byte >> 1
            self.capacity = 0  # Implied by N, but not explicitly stored

            # Data starts at offset equal to the alignment of T
            align_t = self.type_t.GetByteAlign()
            if align_t == 0:
                align_t = self.type_t.GetByteSize()  # Fallback
            if align_t == 0:
                align_t = ptr_size

            self.data_addr = addr + align_t
        else:
            # 4. Indirect Mode: read the pointer to the heap storage
            void_ptr = process.ReadPointerFromMemory(addr, error)

            # The storage header contains: size_t m_size, size_t m_capacity
            self.size = process.ReadUnsignedIntegerFromMemory(void_ptr, ptr_size, error)
            self.capacity = process.ReadUnsignedIntegerFromMemory(
                void_ptr + ptr_size, ptr_size, error
            )

            # Calculate offset_to_data: round_up(sizeof(header), alignment_of_t)
            header_size = 2 * ptr_size
            align_t = self.type_t.GetByteAlign()
            if align_t == 0:
                align_t = self.type_t.GetByteSize()
            if align_t == 0:
                align_t = ptr_size

            offset = ((header_size + (align_t - 1)) // align_t) * align_t
            self.data_addr = void_ptr + offset

    def num_children(self):
        return self.size

    def get_child_index(self, name):
        try:
            return int(name.lstrip("[").rstrip("]"))
        except:
            return -1

    def get_child_at_index(self, index):
        if index < 0 or index >= self.size:
            return None
        item_addr = self.data_addr + index * self.type_t.GetByteSize()
        return self.valobj.CreateValueFromAddress(f"[{index}]", item_addr, self.type_t)

    def has_children(self):
        return self.size > 0


def EESmallVectorSummaryProvider(valobj, internal_dict):
    provider = EESmallVectorSyntheticProvider(valobj, internal_dict)
    if provider.is_direct:
        return f"[Direct] size={provider.size}"
    else:
        return f"[Indirect] size={provider.size}, capacity={provider.capacity}"


def __lldb_init_module(debugger, internal_dict):
    # Register the Summary (the text next to the variable)
    debugger.HandleCommand(
        'type summary add -x "^EE::SmallVector<.+>$" -F eepp_lldb.EESmallVectorSummaryProvider'
    )

    # Register the Synthetic Children (the expandable array elements)
    debugger.HandleCommand(
        'type synthetic add -x "^EE::SmallVector<.+>$" -l eepp_lldb.EESmallVectorSyntheticProvider'
    )
