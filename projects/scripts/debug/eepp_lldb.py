import lldb


class EESmallVectorSyntheticProvider:
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        self.update()

    def _get_m_data(self):
        # 1. Try direct access
        m_data = self.valobj.GetChildMemberWithName("m_data")
        if m_data and m_data.IsValid():
            return m_data

        # 2. Try looking inside the base class (ankerl::svector)
        for i in range(self.valobj.GetNumChildren()):
            child = self.valobj.GetChildAtIndex(i)
            if child.GetName() and "svector" in child.GetName():
                m_data = child.GetChildMemberWithName("m_data")
                if m_data and m_data.IsValid():
                    return m_data
        return None

    def update(self):
        self.size = 0
        self.capacity = 0
        self.is_direct = True
        self.data_addr = lldb.LLDB_INVALID_ADDRESS

        self.type_t = self.valobj.GetType().GetTemplateArgumentType(0)
        self.m_data = self._get_m_data()

        if not self.m_data or not self.m_data.IsValid():
            return

        # Use SBData instead of direct memory reading to support variables in registers
        sb_data = self.m_data.GetData()
        error = lldb.SBError()
        if sb_data.GetByteSize() == 0:
            return

        first_byte = sb_data.GetUnsignedInt8(error, 0)
        if error.Fail():
            return

        self.is_direct = (first_byte & 1) != 0

        target = self.valobj.GetTarget()
        ptr_size = target.GetAddressByteSize()

        # Safely get alignment
        align_t = 0
        if hasattr(self.type_t, "GetByteAlign"):
            align_t = self.type_t.GetByteAlign()
        if align_t == 0:
            align_t = self.type_t.GetByteSize()
        if align_t == 0:
            align_t = ptr_size

        if self.is_direct:
            self.size = first_byte >> 1
            self.capacity = 0

            # Data starts at offset equal to the alignment of T
            addr = self.m_data.GetLoadAddress()
            if addr != lldb.LLDB_INVALID_ADDRESS:
                self.data_addr = addr + align_t
        else:
            # Indirect Mode: read the pointer to the heap storage
            void_ptr = sb_data.GetAddress(error, 0)
            process = self.valobj.GetProcess()

            if error.Fail() or void_ptr == 0 or not process.IsValid():
                return

            self.size = process.ReadUnsignedIntegerFromMemory(void_ptr, ptr_size, error)
            self.capacity = process.ReadUnsignedIntegerFromMemory(
                void_ptr + ptr_size, ptr_size, error
            )

            # Calculate offset_to_data: round_up(sizeof(header), alignment_of_t)
            header_size = 2 * ptr_size
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
    """
    Standalone summary provider. It's an LLDB anti-pattern to initialize the
    SyntheticProvider class inside here, so we duplicate the tiny bit of logic
    needed to read the state efficiently.
    """
    # Find m_data, handling base classes
    m_data = valobj.GetChildMemberWithName("m_data")
    if not m_data or not m_data.IsValid():
        for i in range(valobj.GetNumChildren()):
            child = valobj.GetChildAtIndex(i)
            if child.GetName() and "svector" in child.GetName():
                m_data = child.GetChildMemberWithName("m_data")
                break

    if not m_data or not m_data.IsValid():
        return f"size={valobj.GetNumChildren()}"

    sb_data = m_data.GetData()
    error = lldb.SBError()
    first_byte = sb_data.GetUnsignedInt8(error, 0)

    if error.Fail():
        return f"size={valobj.GetNumChildren()}"

    is_direct = (first_byte & 1) != 0

    if is_direct:
        size = first_byte >> 1
        return f"[Direct] size={size}"
    else:
        target = valobj.GetTarget()
        ptr_size = target.GetAddressByteSize()
        void_ptr = sb_data.GetAddress(error, 0)
        process = valobj.GetProcess()

        if process.IsValid() and void_ptr != 0:
            size = process.ReadUnsignedIntegerFromMemory(void_ptr, ptr_size, error)
            capacity = process.ReadUnsignedIntegerFromMemory(void_ptr + ptr_size, ptr_size, error)
            return f"[Indirect] size={size}, capacity={capacity}"

        return "[Indirect]"


def __lldb_init_module(debugger, internal_dict):
    debugger.HandleCommand(
        'type summary add -x "^EE::SmallVector<.+>$" -F eepp_lldb.EESmallVectorSummaryProvider'
    )
    debugger.HandleCommand(
        'type synthetic add -x "^EE::SmallVector<.+>$" -l eepp_lldb.EESmallVectorSyntheticProvider'
    )
