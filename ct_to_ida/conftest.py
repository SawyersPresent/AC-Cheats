"""
conftest.py -- pytest configuration for ct_to_ida tests.

Installs stub modules for idc and idaapi into sys.modules before any test
file imports ct_to_ida, so the try/except ImportError block in ct_to_ida.py
resolves them as if we were inside IDA.
"""

import sys
from unittest.mock import MagicMock

idaapi_stub = MagicMock()

ida_kernwin_stub = MagicMock()
ida_kernwin_stub.ASKBTN_YES = 1
ida_kernwin_stub.ASKBTN_NO = 0
ida_kernwin_stub.ASKBTN_CANCEL = -1

ida_name_stub = MagicMock()
ida_name_stub.SN_CHECK = 1
ida_name_stub.SN_FORCE = 0x800
ida_name_stub.SN_NOCHECK = 0

ida_bytes_stub = MagicMock()

sys.modules.setdefault('idaapi', idaapi_stub)
sys.modules.setdefault('ida_kernwin', ida_kernwin_stub)
sys.modules.setdefault('ida_name', ida_name_stub)
sys.modules.setdefault('ida_bytes', ida_bytes_stub)
sys.modules.setdefault('idautils', MagicMock())
