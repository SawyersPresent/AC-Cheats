# ct_to_ida

Imports a Cheat Engine `.CT` table into an IDA Pro 9.x database. Each entry
becomes a named label and a repeatable comment at the resolved address.

## Overview

A `.CT` file contains two kinds of addresses:

- **Module+offset** (`ac_client.exe+18AC04`) -- stable across runs because
  the offset is relative to the loaded module base. These are the entries
  worth importing.
- **Static absolute** (`256386DC`, `00939F30-4`) -- session-specific heap
  addresses that change every run. These are imported with a `STATIC_` prefix
  and a `SESSION-SPECIFIC ADDRESS` warning in the comment so you know they
  are ephemeral.

## Usage

1. Open your target binary in IDA Pro.
2. `File` -> `Script file` -> select `ct_to_ida.py`.
3. A file dialog opens -- select your `.CT` file.
4. Results are printed to the IDA output window.

If an address already has a user-defined name (not an IDA auto-name like
`unk_XXXX`), a dialog will ask whether to replace or skip. You can apply
your choice to all remaining conflicts in one step.

## Address Format Reference

| Format | Example | Class | IDA action |
|---|---|---|---|
| `MODULE.exe+HEXOFF` | `ac_client.exe+18AC04` | module_offset | Resolve against image base, write label |
| `HEXADDR` | `256386DC` | static | Import with `STATIC_` prefix and session warning |
| `HEXADDR+HEXOFF` | `00939F2C+298` | static_arith | Evaluate arithmetic, import as static |
| `HEXADDR-HEXOFF` | `00939F30-4` | static_arith | Evaluate arithmetic, import as static |

## What appears in IDA

For each imported entry you get:

**Label** (searchable in the Names window):

    HEALTH
    STATIC_AMMO_REAL
    BOT_2_Name

**Repeatable comment** (visible in disassembly next to the address):

    "HEALTH" | 4 Bytes | module+offset
    "BOT 2 Name" | String | chain: [ac_client.exe+18AC04] -> +205 -> +8
    "AMMO REAL" | 4 Bytes | SESSION-SPECIFIC ADDRESS

## Pointer chains

Entries with `<Offsets>` blocks are pointer chains (multi-level dereferences).
Phase 1 imports the base address only and records the chain in the comment.
Live resolution (following pointers in a running process) is planned for a
later phase.

Offsets without a `0x` prefix are parsed as decimal integers. Offsets with a
`0x` prefix are parsed as hexadecimal. If you enter an offset as bare hex in
Cheat Engine without a prefix (for example `205` meaning `0x205`), prefix it
with `0x` in the CE offset field before saving the table, or the importer
will parse it as decimal 205 instead of hex 517.

## Output labels

CE description strings are sanitized into valid IDA labels:

- Non-alphanumeric characters become underscores
- Consecutive underscores collapse to one
- Labels starting with a digit get a `CE_` prefix
- Static addresses get a `STATIC_` prefix
- Result is capped at 128 characters

## Running the tests

No IDA installation required. Tests use stubs for the IDA API.

    pip install pytest
    cd ct_to_ida
    pytest test_ct_to_ida.py -v

## Known limits

- Only the primary loaded module is resolved. Entries that reference a DLL
  other than the main executable are skipped with a warning.
- Pointer chains are annotated but not dereferenced. The resolved address is
  the pointer base, not the final target.
- CE variable types (`Float`, `String`, etc.) are recorded in the comment
  only. IDA data types are not modified.
- Lua scripts and encrypted entries inside `.CT` files are ignored.
