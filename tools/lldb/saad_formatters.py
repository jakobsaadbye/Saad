# LLDB formatters for Saad programs: they show the values like the program writes them.
#
#   (lldb) command script import tools/lldb/saad_formatters.py
#
#   string        "hello"                     instead of its data pointer and length
#   [..]T, []T    count = 3, with the elements [0], [1], [2] as children
#   ?T            5, or none
#   unions        .label "hi", with only the variant that is held as a child
#   any           int 7, with the value as a child
#
# The program must be built with debug information: saad build main.sd --llvm --debug

import lldb

MAX_ELEMENTS = 10000   # Children of a list or slice that are shown at most
MAX_STRING = 4096      # Bytes of a string that are read at most


def _read_string(process, address, length):
    if length <= 0:
        return '""'
    error = lldb.SBError()
    count = min(length, MAX_STRING)
    data = process.ReadMemory(address, count, error)
    if not error.Success():
        return '<unreadable string>'
    text = data.decode('utf-8', 'replace')
    text = text.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n').replace('\r', '\\r').replace('\t', '\\t')
    return '"' + text + ('..."' if length > count else '"')


def string_summary(value, internal_dict):
    value = value.GetNonSyntheticValue()
    data = value.GetChildMemberWithName('data').GetValueAsUnsigned()
    length = value.GetChildMemberWithName('len').GetValueAsSigned()
    return _read_string(value.GetProcess(), data, length)


def _short(value):
    """A value in one line: its summary, else its value."""
    summary = value.GetSummary()
    if summary:
        return summary
    text = value.GetValue()
    return text if text is not None else ''


#
# Lists ([..]T, List<T>) and slices ([]T): the elements from data, count many
#

class ListProvider:
    def __init__(self, value, internal_dict):
        self.value = value
        self.update()

    def update(self):
        self.data = self.value.GetChildMemberWithName('data')
        self.count = max(0, min(self.value.GetChildMemberWithName('count').GetValueAsSigned(), MAX_ELEMENTS))
        self.element = self.data.GetType().GetPointeeType()
        self.size = self.element.GetByteSize()
        return False

    def num_children(self):
        return self.count if self.size > 0 else 0

    def get_child_index(self, name):
        try:
            return int(name.lstrip('[').rstrip(']'))
        except ValueError:
            return -1

    def get_child_at_index(self, index):
        if index < 0 or index >= self.num_children():
            return None
        address = self.data.GetValueAsUnsigned() + index * self.size
        return self.value.CreateValueFromAddress('[%d]' % index, address, self.element)

    def has_children(self):
        return True


def list_summary(value, internal_dict):
    count = value.GetNonSyntheticValue().GetChildMemberWithName('count').GetValueAsSigned()
    return 'count = %d' % count


#
# Optionals (?T): the item when it is present
#

def optional_summary(value, internal_dict):
    value = value.GetNonSyntheticValue()
    if not value.GetChildMemberWithName('present').GetValueAsUnsigned():
        return 'none'
    return _short(value.GetChildMemberWithName('item'))


#
# Unions: a struct whose first member is the $tag, an enum of the names of the variants. Only the variant that is
# held is a child
#

def is_union(sbtype, internal_dict):
    sbtype = sbtype.GetCanonicalType()
    return sbtype.GetTypeClass() == lldb.eTypeClassStruct and sbtype.GetNumberOfFields() > 0 and \
        sbtype.GetFieldAtIndex(0).GetName() == '$tag'


class UnionProvider:
    def __init__(self, value, internal_dict):
        self.value = value
        self.update()

    def update(self):
        self.variant = self.value.GetChildMemberWithName('$tag').GetValue() or ''
        self.held = self.value.GetChildMemberWithName(self.variant) if self.variant else None
        if self.held is not None and not self.held.IsValid():
            self.held = None   # A variant without a value, e.g empty
        return False

    def num_children(self):
        return 1 if self.held is not None else 0

    def get_child_index(self, name):
        return 0 if self.held is not None and name == self.variant else -1

    def get_child_at_index(self, index):
        return self.held if index == 0 else None

    def has_children(self):
        return self.held is not None


def union_summary(value, internal_dict):
    raw = value.GetNonSyntheticValue()
    variant = raw.GetChildMemberWithName('$tag').GetValue() or '?'
    held = raw.GetChildMemberWithName(variant)
    if held is not None and held.IsValid():
        return '.%s %s' % (variant, _short(held))
    return '.%s' % variant


#
# any: the value it points to, with the type of its runtime Type, when the debug information has a type of that name
#

def _any_parts(value):
    value = value.GetNonSyntheticValue()
    data = value.GetChildMemberWithName('data').GetValueAsUnsigned()
    type_pointer = value.GetChildMemberWithName('type')
    if data == 0 or type_pointer.GetValueAsUnsigned() == 0:
        return None, None
    name = string_summary(type_pointer.Dereference().GetChildMemberWithName('name'), None).strip('"')
    sbtype = value.GetTarget().FindFirstType(name)
    if not sbtype.IsValid():
        return name, None
    return name, value.CreateValueFromAddress('value', data, sbtype)


class AnyProvider:
    def __init__(self, value, internal_dict):
        self.value = value
        self.update()

    def update(self):
        self.name, self.held = _any_parts(self.value)
        return False

    def num_children(self):
        return 1 if self.held is not None else 0

    def get_child_index(self, name):
        return 0 if name == 'value' and self.held is not None else -1

    def get_child_at_index(self, index):
        return self.held if index == 0 else None

    def has_children(self):
        return self.held is not None


def any_summary(value, internal_dict):
    name, held = _any_parts(value)
    if name is None:
        return 'null'
    return ('%s %s' % (name, _short(held))).strip() if held is not None else name


def __lldb_init_module(debugger, internal_dict):
    module = __name__
    run = debugger.HandleCommand
    category = '-w saad'
    run('type summary add %s -F %s.string_summary "string"' % (category, module))
    run('type summary add %s -e -F %s.list_summary -x "^(List<.+>|\\[\\].+|\\[\\.\\.\\].+)$"' % (category, module))
    run('type synthetic add %s -l %s.ListProvider -x "^(List<.+>|\\[\\].+|\\[\\.\\.\\].+)$"' % (category, module))
    run('type summary add %s -F %s.optional_summary -x "^\\?.+$"' % (category, module))
    run('type summary add %s -e -F %s.union_summary --recognizer-function %s.is_union' % (category, module, module))
    run('type synthetic add %s -l %s.UnionProvider --recognizer-function %s.is_union' % (category, module, module))
    run('type summary add %s -e -F %s.any_summary "any"' % (category, module))
    run('type synthetic add %s -l %s.AnyProvider "any"' % (category, module))
    run('type category enable saad')
