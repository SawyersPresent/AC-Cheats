"""
test_ct_to_ida.py -- pytest tests for ct_to_ida.py

Run outside IDA: pytest test_ct_to_ida.py -v
conftest.py installs idc/idaapi stubs before import.
"""

from __future__ import annotations

import os
import sys
import textwrap
from pathlib import Path
from unittest.mock import MagicMock, patch, call

import pytest

# conftest.py runs first so idc/idaapi stubs are in sys.modules
from ct_to_ida import (
    CheatEntry,
    classify_address,
    sanitize_label,
    build_comment,
    load_ct_file,
    _is_auto_name,
    apply_entry,
    get_module_base,
    resolve_ea,
)

SAMPLE_CT = Path(__file__).parent / 'sample.CT'


# ---------------------------------------------------------------------------
# classify_address
# ---------------------------------------------------------------------------

class TestClassifyAddress:
    def test_module_offset(self):
        klass, module, offset, static_ea = classify_address('ac_client.exe+18AC04')
        assert klass == 'module_offset'
        assert module == 'ac_client.exe'
        assert offset == 0x18AC04
        assert static_ea == 0

    def test_module_offset_case_insensitive(self):
        klass, module, offset, _ = classify_address('AC_CLIENT.EXE+ABCD')
        assert klass == 'module_offset'
        assert offset == 0xABCD

    def test_module_offset_dll(self):
        klass, module, offset, _ = classify_address('some.dll+1234')
        assert klass == 'module_offset'
        assert module == 'some.dll'

    def test_static_plain(self):
        klass, module, offset, ea = classify_address('256386DC')
        assert klass == 'static'
        assert module == ''
        assert offset == 0
        assert ea == 0x256386DC

    def test_static_addition(self):
        klass, _, _, ea = classify_address('00939F2C+298')
        assert klass == 'static_arith'
        assert ea == 0x00939F2C + 0x298

    def test_static_subtraction(self):
        klass, _, _, ea = classify_address('00939F30-4')
        assert klass == 'static_arith'
        assert ea == 0x00939F30 - 4

    def test_empty_string_raises(self):
        with pytest.raises(ValueError, match='empty address string'):
            classify_address('')

    def test_garbage_raises(self):
        with pytest.raises(ValueError, match='unrecognised'):
            classify_address('not_an_address!')

    def test_zero_offset(self):
        klass, _, offset, _ = classify_address('ac_client.exe+0')
        assert klass == 'module_offset'
        assert offset == 0


# ---------------------------------------------------------------------------
# sanitize_label
# ---------------------------------------------------------------------------

class TestSanitizeLabel:
    def test_spaces_become_underscores(self):
        assert sanitize_label('Player Health', 'module_offset') == 'Player_Health'

    def test_special_chars_removed(self):
        label = sanitize_label('"AMMO REAL"', 'module_offset')
        assert '"' not in label
        assert ' ' not in label

    def test_static_prefix(self):
        label = sanitize_label('AMMO', 'static')
        assert label.startswith('STATIC_')

    def test_static_arith_prefix(self):
        label = sanitize_label('HEALTH', 'static_arith')
        assert label.startswith('STATIC_')

    def test_leading_digit_prefix(self):
        label = sanitize_label('1st_player', 'module_offset')
        assert label.startswith('CE_')

    def test_empty_description_fallback(self):
        label = sanitize_label('', 'module_offset')
        assert label == 'ENTRY'

    def test_truncation_at_128(self):
        long_name = 'A' * 200
        label = sanitize_label(long_name, 'module_offset')
        assert len(label) <= 128

    def test_consecutive_underscores_collapsed(self):
        label = sanitize_label('A  B  C', 'module_offset')
        assert '__' not in label

    def test_chain_single_offset_suffix(self):
        label = sanitize_label('Player health', 'module_offset', ['0xEC'])
        assert label == 'Player_health__EC'

    def test_chain_two_offset_suffix(self):
        label = sanitize_label('BOT 2 Name', 'module_offset', ['205', '8'])
        assert label == 'BOT_2_Name__205__8'

    def test_chain_hex_prefix_stripped_in_suffix(self):
        label = sanitize_label('RECOIL', 'module_offset', ['0x60', 'C', '0x364'])
        assert label == 'RECOIL__60__C__364'

    def test_no_offsets_no_suffix(self):
        label = sanitize_label('address 1', 'module_offset', [])
        assert '__' not in label

    def test_chain_suffix_within_128_chars(self):
        long_desc = 'A' * 120
        label = sanitize_label(long_desc, 'module_offset', ['0xEC', '8'])
        assert len(label) <= 128

    def test_no_double_static_prefix(self):
        label = sanitize_label('STATIC_SOMETHING', 'static')
        assert label.count('STATIC_') == 1

    def test_all_special_chars_fallback(self):
        label = sanitize_label('!@#$%', 'module_offset')
        assert label == 'ENTRY'


# ---------------------------------------------------------------------------
# build_comment
# ---------------------------------------------------------------------------

def _entry(**kwargs) -> CheatEntry:
    defaults = dict(
        entry_id=1,
        description='test',
        variable_type='4 Bytes',
        address_raw='ac_client.exe+1234',
        offsets_raw=[],
    )
    defaults.update(kwargs)
    return CheatEntry(**defaults)


class TestBuildComment:
    def test_direct_module_offset(self):
        entry = _entry(description='HEALTH', variable_type='4 Bytes')
        comment = build_comment(entry, 'module_offset')
        assert 'HEALTH' in comment
        assert '4 Bytes' in comment
        assert 'module+offset' in comment
        assert 'chain' not in comment

    def test_pointer_chain_two_levels(self):
        entry = _entry(
            description='BOT 2 Name',
            variable_type='String',
            offsets_raw=['205', '8'],
        )
        comment = build_comment(entry, 'module_offset')
        assert 'chain' in comment
        assert '+205' in comment
        assert '+8' in comment

    def test_pointer_chain_five_levels(self):
        entry = _entry(offsets_raw=['0x10', '0x20', '0x30', '0x40', '0x50'])
        comment = build_comment(entry, 'module_offset')
        assert comment.count('->') == 5

    def test_static_address(self):
        entry = _entry(description='AMMO REAL', address_raw='256386DC')
        comment = build_comment(entry, 'static')
        assert 'SESSION-SPECIFIC' in comment
        assert 'chain' not in comment

    def test_static_arith_address(self):
        entry = _entry(address_raw='00939F30-4')
        comment = build_comment(entry, 'static_arith')
        assert 'SESSION-SPECIFIC' in comment

    def test_chain_shows_raw_address(self):
        entry = _entry(
            address_raw='ac_client.exe+18AC04',
            offsets_raw=['0'],
        )
        comment = build_comment(entry, 'module_offset')
        assert 'ac_client.exe+18AC04' in comment


# ---------------------------------------------------------------------------
# load_ct_file
# ---------------------------------------------------------------------------

class TestLoadCtFile:
    def test_loads_sample(self):
        entries = load_ct_file(str(SAMPLE_CT))
        assert len(entries) > 0

    def test_module_offset_entry_parsed(self):
        entries = load_ct_file(str(SAMPLE_CT))
        health = next(e for e in entries if e.description == 'Player Health')
        assert health.address_raw == 'ac_client.exe+0x18EC'
        assert health.offsets_raw == []

    def test_pointer_chain_entry_parsed(self):
        entries = load_ct_file(str(SAMPLE_CT))
        bot = next(e for e in entries if e.description == 'BOT 2 Name')
        assert bot.offsets_raw == ['205', '8']

    def test_entry_without_address_included(self):
        entries = load_ct_file(str(SAMPLE_CT))
        group = next(e for e in entries if e.description == 'Group: Weapons')
        assert group.address_raw == ''

    def test_missing_file_raises(self):
        with pytest.raises(FileNotFoundError):
            load_ct_file('nonexistent.CT')

    def test_malformed_xml_raises(self):
        import tempfile
        with tempfile.NamedTemporaryFile(suffix='.CT', mode='w', delete=False) as f:
            f.write('<not valid xml <<<<')
            tmp = f.name
        try:
            with pytest.raises(ValueError, match='malformed'):
                load_ct_file(tmp)
        finally:
            os.unlink(tmp)

    def test_wrong_root_tag_raises(self):
        import tempfile
        with tempfile.NamedTemporaryFile(suffix='.CT', mode='w', delete=False) as f:
            f.write('<?xml version="1.0"?><WrongRoot></WrongRoot>')
            tmp = f.name
        try:
            with pytest.raises(ValueError, match='CheatTable'):
                load_ct_file(tmp)
        finally:
            os.unlink(tmp)

    def test_description_strips_quotes(self):
        entries = load_ct_file(str(SAMPLE_CT))
        for entry in entries:
            assert not entry.description.startswith('"')
            assert not entry.description.endswith('"')

    def test_five_level_chain_parsed(self):
        entries = load_ct_file(str(SAMPLE_CT))
        deep = next(e for e in entries if e.description == 'Deep chain test')
        assert len(deep.offsets_raw) == 5


# ---------------------------------------------------------------------------
# _is_auto_name
# ---------------------------------------------------------------------------

class TestIsAutoName:
    @pytest.mark.parametrize('name', [
        'unk_83A4EC', 'sub_401000', 'loc_4012AB',
        'byte_1234', 'dword_5678', 'off_ABCD',
    ])
    def test_auto_names_detected(self, name: str):
        assert _is_auto_name(name) is True

    @pytest.mark.parametrize('name', [
        'HEALTH', 'player_base', 'MyLabel', 'STATIC_AMMO',
    ])
    def test_user_names_not_auto(self, name: str):
        assert _is_auto_name(name) is False

    def test_empty_string_is_not_auto(self):
        assert _is_auto_name('') is False


# ---------------------------------------------------------------------------
# apply_entry
# ---------------------------------------------------------------------------

class TestApplyEntry:
    def _setup_mocks(self, existing_name: str = ''):
        ida_name = sys.modules['ida_name']
        ida_name.get_name = MagicMock(return_value=existing_name)
        ida_name.set_name = MagicMock(return_value=True)

        ida_bytes = sys.modules['ida_bytes']
        ida_bytes.set_cmt = MagicMock(return_value=True)

        ida_kernwin = sys.modules['ida_kernwin']
        ida_kernwin.ask_buttons = MagicMock(return_value=1)
        ida_kernwin.ask_yn = MagicMock(return_value=0)

        return ida_name, ida_bytes, ida_kernwin

    def test_fresh_address_writes_label_and_comment(self):
        ida_name, ida_bytes, _ = self._setup_mocks('')
        success, msg = apply_entry(0x1234, 'HEALTH', 'comment text', ['ask'])
        assert success is True
        # No conflict -> SN_CHECK only, no SN_FORCE
        ida_name.set_name.assert_called_once_with(0x1234, 'HEALTH', ida_name.SN_CHECK)
        ida_bytes.set_cmt.assert_called_once_with(0x1234, 'comment text', True)

    def test_replace_uses_sn_force(self):
        ida_name, _, ida_kernwin = self._setup_mocks('my_existing_name')
        ida_kernwin.ask_buttons = MagicMock(return_value=1)   # Replace
        ida_kernwin.ask_yn = MagicMock(return_value=0)
        apply_entry(0x1234, 'HEALTH', 'comment', ['ask'])
        flags_used = ida_name.set_name.call_args.args[2]
        assert flags_used & ida_name.SN_FORCE

    def test_replace_all_policy_uses_sn_force(self):
        ida_name, _, _ = self._setup_mocks('my_existing_name')
        apply_entry(0x1234, 'HEALTH', 'comment', ['replace_all'])
        flags_used = ida_name.set_name.call_args.args[2]
        assert flags_used & ida_name.SN_FORCE

    def test_auto_name_not_treated_as_conflict(self):
        _, _, ida_kernwin = self._setup_mocks('unk_1234')
        success, msg = apply_entry(0x1234, 'HEALTH', 'comment', ['ask'])
        assert success is True
        ida_kernwin.ask_buttons.assert_not_called()

    def test_real_conflict_prompts_user(self):
        _, _, ida_kernwin = self._setup_mocks('my_existing_name')
        ida_kernwin.ask_buttons = MagicMock(return_value=1)
        ida_kernwin.ask_yn = MagicMock(return_value=0)
        success, _ = apply_entry(0x1234, 'HEALTH', 'comment', ['ask'])
        assert success is True
        ida_kernwin.ask_buttons.assert_called_once()

    def test_conflict_skip_returns_false(self):
        ida_name, _, ida_kernwin = self._setup_mocks('my_existing_name')
        ida_kernwin.ask_buttons = MagicMock(return_value=0)
        ida_kernwin.ask_yn = MagicMock(return_value=0)
        success, msg = apply_entry(0x1234, 'HEALTH', 'comment', ['ask'])
        assert success is False
        ida_name.set_name.assert_not_called()

    def test_conflict_cancel_returns_cancel_message(self):
        _, _, ida_kernwin = self._setup_mocks('my_existing_name')
        ida_kernwin.ask_buttons = MagicMock(return_value=-1)
        _, msg = apply_entry(0x1234, 'HEALTH', 'comment', ['ask'])
        assert msg == 'CANCEL'

    def test_replace_all_policy_skips_dialog(self):
        _, _, ida_kernwin = self._setup_mocks('my_existing_name')
        success, _ = apply_entry(0x1234, 'HEALTH', 'comment', ['replace_all'])
        assert success is True
        ida_kernwin.ask_buttons.assert_not_called()

    def test_skip_all_policy_skips_without_writing(self):
        ida_name, _, ida_kernwin = self._setup_mocks('my_existing_name')
        success, msg = apply_entry(0x1234, 'HEALTH', 'comment', ['skip_all'])
        assert success is False
        ida_name.set_name.assert_not_called()
        ida_kernwin.ask_buttons.assert_not_called()

    def test_replace_all_propagates_to_policy(self):
        _, _, ida_kernwin = self._setup_mocks('my_existing_name')
        ida_kernwin.ask_buttons = MagicMock(return_value=1)
        ida_kernwin.ask_yn = MagicMock(return_value=1)
        policy = ['ask']
        apply_entry(0x1234, 'HEALTH', 'comment', policy)
        assert policy[0] == 'replace_all'

    def test_skip_all_propagates_to_policy(self):
        _, _, ida_kernwin = self._setup_mocks('my_existing_name')
        ida_kernwin.ask_buttons = MagicMock(return_value=0)
        ida_kernwin.ask_yn = MagicMock(return_value=1)
        policy = ['ask']
        apply_entry(0x1234, 'HEALTH', 'comment', policy)
        assert policy[0] == 'skip_all'


# ---------------------------------------------------------------------------
# Integration: full pipeline with stub IDA
# ---------------------------------------------------------------------------

class TestIntegration:
    def setup_method(self):
        ida_name = sys.modules['ida_name']
        ida_name.get_name = MagicMock(return_value='')
        ida_name.set_name = MagicMock(return_value=True)

        ida_bytes = sys.modules['ida_bytes']
        ida_bytes.set_cmt = MagicMock(return_value=True)

        ida_kernwin = sys.modules['ida_kernwin']
        ida_kernwin.ask_buttons = MagicMock(return_value=1)
        ida_kernwin.ask_yn = MagicMock(return_value=0)

        idaapi = sys.modules['idaapi']
        idaapi.get_root_filename = MagicMock(return_value='ac_client.exe')
        idaapi.get_imagebase = MagicMock(return_value=0x400000)

    def test_module_offset_resolves_correctly(self):
        from ct_to_ida import run_import
        run_import(str(SAMPLE_CT))
        ida_name = sys.modules['ida_name']
        # Player Health: ac_client.exe+0x18EC -> 0x400000 + 0x18EC = 0x4018EC
        calls = [c.args[0] for c in ida_name.set_name.call_args_list]
        assert 0x4018EC in calls

    def test_static_entries_imported_with_static_prefix(self):
        from ct_to_ida import run_import
        run_import(str(SAMPLE_CT))
        ida_name = sys.modules['ida_name']
        label_calls = {c.args[1] for c in ida_name.set_name.call_args_list}
        assert any(lbl.startswith('STATIC_') for lbl in label_calls)

    def test_group_entry_without_address_is_skipped(self):
        from ct_to_ida import run_import
        run_import(str(SAMPLE_CT))
        ida_name = sys.modules['ida_name']
        calls = [c.args[1] for c in ida_name.set_name.call_args_list]
        assert 'Group__Weapons' not in calls
