"""
ct_to_ida.py -- Cheat Engine table importer for IDA Pro 9.x

Run from IDA: File -> Script file -> ct_to_ida.py
The script prompts for a .CT file then imports all entries into the active IDB.

Phase 1 scope: label + comment per entry, module+offset resolution, static
address flagging, pointer chain annotation, conflict resolution dialog.
"""

from __future__ import annotations

import re
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from typing import Optional

try:
    import idaapi
    import ida_kernwin
    import ida_name
    import ida_bytes
    _INSIDE_IDA = True
except ImportError:
    _INSIDE_IDA = False


# ---------------------------------------------------------------------------
# Data model
# ---------------------------------------------------------------------------

@dataclass
class CheatEntry:
    """One row from a Cheat Engine .CT file."""

    entry_id: int
    description: str
    variable_type: str
    address_raw: str
    offsets_raw: list[str] = field(default_factory=list)

    @property
    def offsets(self) -> list[int]:
        """Offset list parsed as integers (0x-prefixed = hex, bare = decimal)."""
        return [int(raw, 0) for raw in self.offsets_raw]


# ---------------------------------------------------------------------------
# Address classification
# ---------------------------------------------------------------------------

_MODULE_OFFSET_RE = re.compile(
    r'^([\w][\w. -]*\.(?:exe|dll))\+((?:0x)?[0-9A-Fa-f]+)$',
    re.IGNORECASE,
)
_STATIC_ARITH_RE = re.compile(r'^([0-9A-Fa-f]+)([+-])((?:0x)?[0-9A-Fa-f]+)$', re.IGNORECASE)
_STATIC_PLAIN_RE = re.compile(r'^[0-9A-Fa-f]+$')

AddressClass = str  # 'module_offset' | 'static' | 'static_arith'


def classify_address(address_raw: str) -> tuple[AddressClass, str, int, int]:
    """
    Classify a raw CE address string into one of three classes.

    Returns (address_class, module_name, module_offset, static_ea).
      address_class:  'module_offset' | 'static' | 'static_arith'
      module_name:    e.g. 'ac_client.exe' (empty for static classes)
      module_offset:  hex offset within the module (0 for static classes)
      static_ea:      resolved integer EA (0 for module_offset class)

    Raises ValueError on unrecognised format.
    """
    if not address_raw:
        raise ValueError("empty address string")

    match = _MODULE_OFFSET_RE.match(address_raw)
    if match:
        return ('module_offset', match.group(1), _parse_hex(match.group(2)), 0)

    match = _STATIC_ARITH_RE.match(address_raw)
    if match:
        base = _parse_hex(match.group(1))
        operand = _parse_hex(match.group(3))
        ea = base + operand if match.group(2) == '+' else base - operand
        return ('static_arith', '', 0, ea)

    if _STATIC_PLAIN_RE.match(address_raw):
        return ('static', '', 0, _parse_hex(address_raw))

    raise ValueError(f"unrecognised address format: {address_raw!r}")


# ---------------------------------------------------------------------------
# Label and comment builders
# ---------------------------------------------------------------------------

def _fmt_offset(raw: str) -> str:
    """Format a raw CE offset string for use in a label suffix."""
    raw = raw.strip()
    return raw[2:].upper() if raw.lower().startswith('0x') else raw


def sanitize_label(
    description: str,
    address_class: AddressClass,
    offsets_raw: list[str] | None = None,
) -> str:
    """
    Convert a CE description string into a valid IDA label.

    Sanitization:
    - Non-alphanumeric/underscore chars become underscores
    - Consecutive underscores collapse to one
    - Leading/trailing underscores stripped
    - Labels starting with a digit get CE_ prefix
    - Static addresses get STATIC_ prefix
    - Pointer-chain entries append each offset as __OFFSET (e.g. __EC__8)
      so multiple chains anchored at the same base address get unique names
    - Result is capped at 128 characters
    """
    if not description:
        description = 'ENTRY'

    name = re.sub(r'[^A-Za-z0-9_]', '_', description)
    name = re.sub(r'_+', '_', name)
    name = name.strip('_')

    if not name:
        name = 'ENTRY'
    elif name[0].isdigit():
        name = 'CE_' + name

    if address_class in ('static', 'static_arith') and not name.startswith('STATIC_'):
        name = 'STATIC_' + name

    if offsets_raw:
        suffix = '__'.join(_fmt_offset(r) for r in offsets_raw)
        name = f'{name}__{suffix}'

    return name[:128]


def build_comment(entry: CheatEntry, address_class: AddressClass) -> str:
    """
    Build the repeatable IDA comment string for a CE entry.

    Direct (no offsets):
        "HEALTH" | 4 Bytes | module+offset

    Pointer chain:
        "BOT 2 Name" | String | chain: [ac_client.exe+0x18AC04] -> +205 -> +8

    Static:
        "AMMO REAL" | 4 Bytes | SESSION-SPECIFIC ADDRESS
    """
    desc = entry.description
    vtype = entry.variable_type
    addr = entry.address_raw

    if address_class == 'module_offset':
        if entry.offsets_raw:
            parts = [f'[{addr}]'] + [f'+{r}' for r in entry.offsets_raw]
            chain = ' -> '.join(parts)
            return f'"{desc}" | {vtype} | chain: {chain}'
        return f'"{desc}" | {vtype} | module+offset'

    return f'"{desc}" | {vtype} | SESSION-SPECIFIC ADDRESS'


# ---------------------------------------------------------------------------
# CT file parser
# ---------------------------------------------------------------------------

def load_ct_file(ct_path: str) -> list[CheatEntry]:
    """
    Parse a Cheat Engine .CT XML file.

    Returns a list of CheatEntry objects for every <CheatEntry> element found,
    including those inside groups (entries without an Address are skipped later
    during import, not here).

    Raises FileNotFoundError if the path does not exist.
    Raises ValueError if the XML is malformed or lacks a <CheatTable> root.
    """
    try:
        tree = ET.parse(ct_path)
    except ET.ParseError as exc:
        raise ValueError(f"malformed CT XML: {exc}") from exc

    root = tree.getroot()
    if root.tag != 'CheatTable':
        raise ValueError(f"expected <CheatTable> root, got <{root.tag}>")

    entries: list[CheatEntry] = []
    for elem in root.iter('CheatEntry'):
        entry_id = int(elem.findtext('ID') or '0')
        raw_desc = elem.findtext('Description') or ''
        description = raw_desc.strip('"')
        variable_type = elem.findtext('VariableType') or 'Unknown'
        address_raw = (elem.findtext('Address') or '').strip()

        offsets_raw: list[str] = []
        offsets_elem = elem.find('Offsets')
        if offsets_elem is not None:
            for off_elem in offsets_elem.findall('Offset'):
                raw = (off_elem.text or '0').strip()
                offsets_raw.append(raw)

        entries.append(CheatEntry(
            entry_id=entry_id,
            description=description,
            variable_type=variable_type,
            address_raw=address_raw,
            offsets_raw=offsets_raw,
        ))

    return entries


# ---------------------------------------------------------------------------
# IDA integration
# ---------------------------------------------------------------------------

def _parse_hex(value: str) -> int:
    """Parse a hex integer that may or may not carry a 0x prefix."""
    return int(value, 0) if value.lower().startswith('0x') else int(value, 16)


_AUTO_NAME_PREFIXES = (
    'unk_', 'sub_', 'loc_', 'byte_', 'word_', 'dword_',
    'qword_', 'off_', 'asc_', 'stru_', 'def_',
)


def _is_auto_name(name: str) -> bool:
    """Return True if name is an IDA auto-generated label, not a user name."""
    lower = name.lower()
    return any(lower.startswith(p) for p in _AUTO_NAME_PREFIXES)


def get_module_base(module_name: str) -> Optional[int]:
    """
    Return the IDA image base if module_name matches the loaded binary.

    Case-insensitive comparison against idaapi.get_root_filename().
    Returns None when the module is not the loaded file.
    """
    root_filename = idaapi.get_root_filename() or ''
    if root_filename.lower() == module_name.lower():
        return idaapi.get_imagebase()
    return None


def resolve_ea(
    address_class: AddressClass,
    module_name: str,
    module_offset: int,
    static_ea: int,
) -> Optional[int]:
    """
    Resolve classification data to an IDA effective address.

    Returns None when the module is not the loaded binary.
    """
    if address_class == 'module_offset':
        base = get_module_base(module_name)
        if base is None:
            return None
        return base + module_offset
    return static_ea


def apply_entry(
    ea: int,
    label: str,
    comment: str,
    conflict_policy: list[str],
) -> tuple[bool, str]:
    """
    Write label and repeatable comment to the IDA database at ea.

    conflict_policy is a single-element list so callers share state:
      'ask'         -- prompt user on each conflict (default)
      'replace_all' -- overwrite all conflicts without asking
      'skip_all'    -- skip all conflicts without asking

    Returns (success, status_message).
    The message is 'CANCEL' when the user aborts the entire import.
    """
    existing_name = ida_name.get_name(ea) or ''
    is_conflict = bool(existing_name) and not _is_auto_name(existing_name)
    force = False

    if is_conflict:
        policy = conflict_policy[0]

        if policy == 'skip_all':
            return (False, f'conflict skipped: {existing_name!r} at {ea:#010x}')

        if policy == 'replace_all':
            force = True

        elif policy == 'ask':
            choice = ida_kernwin.ask_buttons(
                'Replace', 'Skip', 'Cancel import', 1,
                f'Address {ea:#010x} already has a name:\n'
                f'  existing : {existing_name}\n'
                f'  CE label : {label}\n\n'
                f'Replace = apply to this entry\n'
                f'Skip    = keep existing name\n'
                f'Cancel  = abort the entire import',
            )
            if choice == -1:
                return (False, 'CANCEL')

            if choice == 0:
                apply_all = ida_kernwin.ask_yn(0, 'Skip all remaining conflicts?')
                if apply_all == 1:
                    conflict_policy[0] = 'skip_all'
                return (False, f'conflict skipped: {existing_name!r} at {ea:#010x}')

            force = True
            apply_all = ida_kernwin.ask_yn(0, 'Replace all remaining conflicts?')
            if apply_all == 1:
                conflict_policy[0] = 'replace_all'

    name_flags = ida_name.SN_CHECK | (ida_name.SN_FORCE if force else 0)
    name_ok = ida_name.set_name(ea, label, name_flags)
    ida_bytes.set_cmt(ea, comment, True)
    if not name_ok:
        return (False, f'set_name rejected {label!r} at {ea:#010x} (tail byte or outside segment)')
    return (True, f'{label} at {ea:#010x}')


# ---------------------------------------------------------------------------
# Orchestration
# ---------------------------------------------------------------------------

def run_import(ct_path: str) -> None:
    """
    Full pipeline: parse CT -> classify addresses -> resolve EAs -> apply to IDB.

    Prints a result summary to IDA's output window and writes a log file next to
    the CT file listing every successfully imported label and its address.
    """
    try:
        entries = load_ct_file(ct_path)
    except (ValueError, FileNotFoundError) as exc:
        print(f'[ct_to_ida] ERROR {exc}')
        return

    print(f'[ct_to_ida] loaded {len(entries)} entries from {ct_path}')

    conflict_policy: list[str] = ['ask']
    imported = skipped = failed = 0
    log_lines: list[str] = []

    for entry in entries:
        prefix = f'  entry {entry.entry_id:>3} "{entry.description}"'

        if not entry.address_raw:
            print(f'[ct_to_ida] SKIP {prefix}: no address field')
            skipped += 1
            continue

        try:
            address_class, module_name, module_offset, static_ea = classify_address(
                entry.address_raw
            )
        except ValueError as exc:
            print(f'[ct_to_ida] FAIL {prefix}: {exc}')
            failed += 1
            continue

        ea = resolve_ea(address_class, module_name, module_offset, static_ea)
        if ea is None:
            print(f'[ct_to_ida] FAIL {prefix}: module {module_name!r} not loaded')
            failed += 1
            continue

        label = sanitize_label(entry.description, address_class, entry.offsets_raw or None)
        comment = build_comment(entry, address_class)

        success, message = apply_entry(ea, label, comment, conflict_policy)

        if message == 'CANCEL':
            print(f'[ct_to_ida] import cancelled after {imported} entries written')
            break

        if success:
            imported += 1
            print(f'[ct_to_ida] OK   {prefix} -> {message}')
            log_lines.append(f'{ea:#010x}  {label}')
        else:
            skipped += 1
            print(f'[ct_to_ida] SKIP {prefix}: {message}')

    print(
        f'\n[ct_to_ida] imported {imported} | skipped {skipped} | failed {failed}'
    )

    if log_lines:
        import os
        log_path = os.path.splitext(ct_path)[0] + '_import_log.txt'
        try:
            with open(log_path, 'w', encoding='utf-8') as fh:
                fh.write(f'# ct_to_ida import log -- {ct_path}\n')
                fh.write(f'# {imported} labels imported\n\n')
                fh.write('address     label\n')
                fh.write('-' * 60 + '\n')
                for line in log_lines:
                    fh.write(line + '\n')
            print(f'[ct_to_ida] log written to {log_path}')
        except OSError as exc:
            print(f'[ct_to_ida] WARNING could not write log: {exc}')


def main() -> None:
    """Prompt for a .CT file then run the import."""
    ct_path = ida_kernwin.ask_file(False, '*.CT', 'Select Cheat Engine Table')
    if not ct_path:
        print('[ct_to_ida] no file selected, import cancelled')
        return
    run_import(ct_path)


if __name__ == '__main__':
    main()
