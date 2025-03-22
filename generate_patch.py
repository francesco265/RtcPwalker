import re
import os
import struct
import subprocess
from sys import argv
from collections import defaultdict

version = ''
if len(argv) > 1:
    version = f' {argv[1]}'

ARM9_CONTROLS_BASE_ADDR = 0x23C0200

asm_exe_path = '/opt/devkitpro/devkitARM/bin/arm-none-eabi-as'
objcopy_exe_path = '/opt/devkitpro/devkitARM/bin/arm-none-eabi-objcopy'
objdump_exe_path = '/opt/devkitpro/devkitARM/bin/arm-none-eabi-objdump'
ld_exe_path = '/opt/devkitpro/devkitARM/bin/arm-none-eabi-ld'
make_exe_path = 'make'

tmp_folder_name = 'tmp'
action_replay_folder_name = 'action_replay_codes'
arm7_update_rtcom_function_name = 'Update_RTCom'

# This is the original opcode used for branching to the ir recv function,
# we use it just to check if the patch was already applied
addr_opcode = {
    'Italy': 0xE9AAF6F8,
    'USA': 0xE97AF6F8,
    'Spain': 0xE966F6F8,
    'Germany': 0xE98AF6F8,
    'France': 0xE96AF6F8,
    'Japan': 0xEB16F6F8,
}

arm9_addresses = [
    # ir recv call 2; ir recv call 1; ir send call; ir init call; ir end branch
    0x21E5B3C, 0x21E5918, 0x21E59A6, 0x21E5906, 0x21E78F0
]

# Games from different regions have variations in the addresses of the IR functions, so we need to adjust them
arm9_addresses_offsets = {
    'Italy': 0,
    'USA': 0x60,
    'Spain': 0x80,
    'Germany': 0x40,
    'France': 0x80,
    'Japan': -0xA60,
}

arm7_addresses = [
    # rtcom block start addr; VBlank handler end addr
    0x380C000, 0x37F87AC
]

# HeartGold and SoulSilver game IDs for each region
rom_info = {
    'Italy': ['IPKI-73F49A89', 'IPGI-0DAA88EF'],
    'USA': ['IPKE-4DFFBF91', 'IPGE-2D5118CA'],
    'Spain': ['IPKS-F88ADC52', 'IPGS-1D38C3BC'],
    'Germany': ['IPKD-1CAB3FD9', 'IPGD-E30FD415'],
    'France': ['IPKF-F16A1F7B', 'IPGF-E1041645'],
    'Japan': ['IPKJ-A587D7CD', 'IPGJ-7387AC7F']
}

def find_function_offset_in_asm_listing(asm_code, func_name):
    asm_func_regexp = r'([a-f0-9]{8}) .+' + func_name + '[^-+].+'
    regexp_func = re.search(asm_func_regexp, asm_code, re.IGNORECASE)
    if regexp_func:
        addr = int(regexp_func.group(1), 16)
    else:
        raise Exception(f"Can't find the function '{func_name}' in the object file")
    return addr

def assemble_arm7_rtcom_patch(rtc_code_block_start_addr):
    arm7_patch_dir = 'arm7_rtcom_patch'
    try:
        subprocess.check_output(
            [make_exe_path, '--directory', arm7_patch_dir],
            stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        print(e.output.decode('utf-8'))
        exit(-1)

    try:
        subprocess.check_output(
            [ld_exe_path, 'rtcom.o', f'{arm7_patch_dir}.uc11.o', '--output', 'arm7_patch.o', '--section-start',
             f'.text={hex(rtc_code_block_start_addr)}'],
            stderr=subprocess.DEVNULL, cwd=f'{arm7_patch_dir}/arm7/build/')
    except subprocess.CalledProcessError as e:
        print(e.output.decode('utf-8'))
        exit(-1)

    subprocess.check_output(
        [objcopy_exe_path, '-O', 'binary', '--only-section=.rodata', '--only-section=.text', 'arm7_patch.o',
         'arm7_patch.bin'],
        cwd=f'{arm7_patch_dir}/arm7/build/')
    arm7_patch_bytes = open(f'{arm7_patch_dir}/arm7/build/arm7_patch.bin', 'rb').read()

    rtcom_asm_code = subprocess.check_output([objdump_exe_path, '-d', 'rtcom.o'],
                                             cwd=f'{arm7_patch_dir}/arm7/build/', text=True)
    update_rtcom_func_offset = find_function_offset_in_asm_listing(rtcom_asm_code, arm7_update_rtcom_function_name)

    return update_rtcom_func_offset, arm7_patch_bytes

def assemble_arm9_controls_hook_patch(asm_symbols_params):
    patch_asm_filename = 'arm9_ir_controls.s'
    assembled_patch_filename = f'{tmp_folder_name}/ARM9_CONTROLS_HOOK_PATCH.bin'

    subprocess.check_output([asm_exe_path, patch_asm_filename, '-o',
                             f'{tmp_folder_name}/arm9_controls_hook.o', *asm_symbols_params])
    subprocess.check_output([objcopy_exe_path, '-Obinary',
                            f'{tmp_folder_name}/arm9_controls_hook.o', assembled_patch_filename])

    asm_code = subprocess.check_output([objdump_exe_path, '-d', f'{tmp_folder_name}/arm9_controls_hook.o'], text=True)

    return bytearray(open(assembled_patch_filename, 'rb').read()), asm_code

def generate_action_replay_code(region):
    def ar_code__bulk_write(bin: bytearray, address: int):
        if len(bin) % 8 != 0:  # make the size a multiple of 8
            bin += b'\x00' * (8 - len(bin) % 8)
        ar_code = f"E{address:07X} {len(bin):08X}\n"
        for words in struct.iter_unpack("<II", bin):
            ar_code += f"{words[0]:08X} {words[1]:08X}\n"
        return ar_code

    def instr__arm_b(from_addr, target, link=False, exchange=False):
        offset = target - (from_addr + 8)
        if exchange:  # blx
            instr_type = 0xFA000000 if (offset & 0x2) == 0 else 0xFB000000
            return instr_type | ((offset >> 2) & 0xFFFFFF)
        else:
            instr_type = 0xEB000000 if link else 0xEA000000
            return instr_type | ((offset >> 2) & 0xFFFFFF)

    def instr__thumb_blx(from_addr, target, exchange=True):
        offset = (target - (from_addr + 4)) >> 1
        hi = (0b11110 << 11) | (offset & 0x3FF800) >> 11
        lo = ((0b11101 if exchange else 0b11111) << 11) | offset & 0x7FF
        lo = lo + 1 if from_addr & 0x3 != 0 else lo # ???
        return (lo << 16) | hi

    action_replay_code = ""
    ####################################################################################
    # Arm7 Patch

    rtcom_code_addr, vblank_handler_end_addr = arm7_addresses

    update_rtcom_offset, arm7_patch_bytes = assemble_arm7_rtcom_patch(rtcom_code_addr)
    branch_to_rtcom_update = instr__arm_b(vblank_handler_end_addr, rtcom_code_addr + update_rtcom_offset)

    action_replay_code += f"""
        5{vblank_handler_end_addr:07X} E12FFF1E # if the patch wasn't uploaded yet
            {ar_code__bulk_write(arm7_patch_bytes, rtcom_code_addr)} # write the Arm7 + Arm11 code
            0{vblank_handler_end_addr:07X} {branch_to_rtcom_update:08X} # Hook the VBlank IRQ Handler
        D2000000 00000000
    """


    ####################################################################################
    # Arm9 Patch

    arm9_patch_code, arm9_patch_asm = assemble_arm9_controls_hook_patch([])

    recv1_addr, recv2_addr, send_addr, init_addr, end_addr = [addr + arm9_addresses_offsets[region] for addr in arm9_addresses]
    ir_recv_offset = find_function_offset_in_asm_listing(arm9_patch_asm, "IrRecvData")
    ir_send_offset = find_function_offset_in_asm_listing(arm9_patch_asm, "IrSendData")
    ir_init_offset = find_function_offset_in_asm_listing(arm9_patch_asm, "IrInit")
    ir_end_offset  = find_function_offset_in_asm_listing(arm9_patch_asm, "IrEnd")
    recv1_branch_instr = instr__thumb_blx(recv1_addr, ARM9_CONTROLS_BASE_ADDR + ir_recv_offset)
    recv2_branch_instr = instr__thumb_blx(recv2_addr, ARM9_CONTROLS_BASE_ADDR + ir_recv_offset)
    send_branch_instr = instr__thumb_blx(send_addr, ARM9_CONTROLS_BASE_ADDR + ir_send_offset)
    init_branch_instr = instr__thumb_blx(init_addr, ARM9_CONTROLS_BASE_ADDR + ir_init_offset)
    end_branch_instr = instr__thumb_blx(end_addr, ARM9_CONTROLS_BASE_ADDR + ir_end_offset, exchange=False)

    # Send branch is not 4 bytes aligned :(
    # 0x21E59A4 = 1C29
    # 0x21E59AA = BD38
    #
    # Neither is the init branch
    # 0x21E5904 = B508
    # 0x21E590A = 2032
    action_replay_code += f"""
        5{recv1_addr:07X} {addr_opcode[region]:08X} # if the patch wasn't uploaded yet
            {ar_code__bulk_write(arm9_patch_code, ARM9_CONTROLS_BASE_ADDR)}
            
            # insert an instruction to branch into the patch code
            0{recv1_addr:07X} {recv1_branch_instr:08X}
            0{recv2_addr:07X} {recv2_branch_instr:08X}
            0{send_addr-2:07X} {send_branch_instr & 0xFFFF:04X}1C29
            0{send_addr+2:07X} BD38{(send_branch_instr & 0xFFFF0000) >> 16:04X}
            0{init_addr-2:07X} {init_branch_instr & 0xFFFF:04X}B508
            0{init_addr+2:07X} 2032{(init_branch_instr & 0xFFFF0000) >> 16:04X}
            0{end_addr:07X} {end_branch_instr:08X}
        D2000000 00000000
    """

    # remove comments, indentation, and empty lines
    formatted_cheatcode = ""
    for line in action_replay_code.splitlines():
        mb_nonempty_line = line.split('#')[0].strip()
        if len(mb_nonempty_line) > 0:
            formatted_cheatcode += mb_nonempty_line + '\n'

    return formatted_cheatcode

def generate_usrcheat_dat_file_with_ar_codes(patches: dict):
    def make_ar_file_header():
        main_header = bytearray(b'\0' * 0x100)
        main_header[0:0xC] = b'R4 CheatCode'
        main_header[0xD] = 1
        db_name = b"Pokemon HGSS Pokewalker 3DS IR"
        struct.pack_into(f'{len(db_name)}s', main_header, 0x10, db_name)
        struct.pack_into('<I', main_header, 0x4C, 0x594153d5)
        main_header[0x50] = 1
        return main_header

    def make_ar_game_header(game_name, n_code):
        game_name = game_name.encode()
        game_header = game_name + b'\0' * (4 - len(game_name) % 4)
        game_attribute_header = bytearray(0x24)
        game_attribute_header[0] = n_code
        game_attribute_header[8] = 1
        game_header += game_attribute_header
        return game_header

    def get_ar_code_values_from_text(codes):
        ar_code_values = []
        for line in codes.splitlines():
            columns = line.split()
            if len(columns) != 2:
                continue
            ar_code_values.append(int(columns[0], 16))
            ar_code_values.append(int(columns[1], 16))
        return ar_code_values

    def make_ar_code_record(code_name, code_description, ar_code_values):
        # Code Header
        ar_code_name = code_name.encode()
        ar_code_description = code_description.encode()
        ar_code_header = ar_code_name + b'\0'
        ar_code_header += ar_code_description
        ar_code_header += b'\0' * (4 - len(ar_code_header) % 4)

        # AR Code Record
        ar_code = struct.pack(f"<{len(ar_code_values)}I", *ar_code_values)
        ar_code_size = len(ar_code)
        ar_code_header += struct.pack("<I", ar_code_size // 4)
        ar_code_size_with_header = ar_code_size + len(ar_code_header)
        ar_code_header = struct.pack("<I", ar_code_size_with_header // 4) + ar_code_header

        ar_code_record = ar_code_header + ar_code
        return ar_code_record

    n_games = len(patches)

    # Game Position Table
    game_pos_table = bytearray(0x10 * n_games)

    game_codes = defaultdict(list)
    for i, (rom_id, roms_info) in enumerate(patches.items()):
        gamecode, crc32_str = rom_id.split('-')
        struct.pack_into('<4sI', game_pos_table, 0x10 * i, gamecode.encode(), int(crc32_str, 16))

        for rom_code in roms_info:
            ar_code_record = make_ar_code_record(rom_code['name'], "",
                                                 get_ar_code_values_from_text(rom_code['code']))
            game_codes[rom_id].append(ar_code_record)

    # Make it Whole
    usrcheat_dat_file = bytearray()
    usrcheat_dat_file += make_ar_file_header()
    usrcheat_dat_file += game_pos_table + bytes(0x10)

    for i, (rom_id, rom_codes) in enumerate(game_codes.items()):
        struct.pack_into('<I', usrcheat_dat_file, 0x100 + i * 0x10 + 8, len(usrcheat_dat_file))
        usrcheat_dat_file += make_ar_game_header(rom_id, n_code=len(rom_codes))
        for rom_code in rom_codes:
            usrcheat_dat_file += rom_code

    return usrcheat_dat_file


def main():
    for folder in [tmp_folder_name, action_replay_folder_name]:
        if not os.path.exists(folder):
            os.makedirs(folder)

    cheat_codes = defaultdict(list)

    for region, rom_ids in rom_info.items():
        # The patch is the same for both HG and SS
        codes = generate_action_replay_code(region)
        for i, rom_id in enumerate(rom_ids):
            gametype = 'HG' if i == 0 else 'SS'
            with open(f'{action_replay_folder_name}/{rom_id}({gametype}-{region}).txt', 'w') as f:
                f.write(codes)
            cheat_codes[rom_id].append({'code': codes, 'name': f'RtcPwalker{version}'})

    usrcheat_file = generate_usrcheat_dat_file_with_ar_codes(cheat_codes)
    with open(f'{action_replay_folder_name}/usrcheat.dat', 'wb') as f:
        f.write(usrcheat_file)

if __name__ == '__main__':
    main()
