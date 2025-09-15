#! /usr/bin/env python3

import argparse
import binascii
import json
import subprocess
import sys
import requests
from requests_toolbelt import MultipartEncoder, MultipartEncoderMonitor
import os
from tqdm import tqdm
from tabulate import tabulate


def calc_stm32_checksum(filepath):
    """Calculate STM32-style checksum (simple sum method)"""
    with open(filepath, 'rb') as f:
        data = f.read()
        checksum = int.from_bytes(data[-4:], 'big')
        crc = 0xFFFFFFFF
        poly = 0x04C11DB7

        for i in range(0, len(data), 4):
            # Assemble 32-bit word in little-endian
            word_bytes = data[i:i+4]
            word = 0
            for j in range(len(word_bytes)):
                word |= word_bytes[j] << (8 * j)
            # XOR word into CRC
            crc ^= word
            # Process 32 bits
            for _ in range(32):
                if crc & 0x80000000:
                    crc = (crc << 1) ^ poly
                else:
                    crc <<= 1
                crc &= 0xFFFFFFFF  # ensure 32-bit
        return checksum, crc == 0

def main():
    parser = argparse.ArgumentParser(description="OTA update script for OpenFlap module")
    parser.add_argument("binfile", help="Path to binary file")
    parser.add_argument("--url", default="openflap.local", help="Controller URL (default: openflap.local)")
    args = parser.parse_args()

    # Verify CRC32.
    checksum, is_valid = calc_stm32_checksum(args.binfile)
    if not is_valid:
        print("Error: Invalid checksum!")
        sys.exit(1)

    print(f"Binary seems valid, checksum: {checksum:08X}")

    # Upload firmware
    firmware_url = f"http://{args.url}/api/module/firmware.bin"
    print("Uploading firmware to ", firmware_url)
    try:
        file_size = os.path.getsize(args.binfile)

        with tqdm(total=file_size, unit='B', unit_scale=True, desc="Uploading", ncols=100) as pbar:
            with open(args.binfile, 'rb') as f:
                def progress_callback(monitor):
                    pbar.update(monitor.bytes_read - pbar.n)
                
                # Create a simple file wrapper for progress tracking
                class FileWrapper:
                    def __init__(self, file_obj, callback):
                        self.file_obj = file_obj
                        self.callback = callback
                        self.bytes_read = 0
                    
                    def read(self, size=-1):
                        data = self.file_obj.read(size)
                        if data:
                            self.bytes_read += len(data)
                            self.callback(len(data))
                        return data
                    
                    def __getattr__(self, name):
                        return getattr(self.file_obj, name)
                
                def update_progress(bytes_read):
                    pbar.update(bytes_read)
                
                file_wrapper = FileWrapper(f, update_progress)
                
                resp = requests.put(
                    firmware_url,
                    data=file_wrapper,
                    headers={'Content-Type': 'application/octet-stream'},
                    timeout=60
                )
                
        if resp.status_code != 200:
            print(f"Firmware upload failed: {resp.status_code} {resp.text}")
            sys.exit(1)
        else:
            print("Firmware upload successful!")
            
    except Exception as e:
        print(f"Firmware upload failed: {e}")
        sys.exit(1)

    # Get module state
    print("Upload successful!")
    print("Validating module update success...")

    state_url = f"http://{args.url}/api/module"
    try:
        resp = requests.get(state_url)
        resp.raise_for_status()
        modules = resp.json()
        module_data = {}
        for mod in modules:
            module_data[mod.get("module")] = {
                "fw_crc": mod.get("firmware_version",{}).get("crc"),
                "column_end": mod.get("module_info", {}).get("column_end")
            }
        # Separate modules by column_end flag
        column_end_true = [k for k, v in module_data.items() if v["column_end"] is True]
        column_end_false = [k for k, v in module_data.items() if v["column_end"] is False]

        num_columns = len(column_end_true)
        num_non_columns = len(column_end_false)
        total_modules = len(module_data)

        if num_columns == 0 or total_modules % num_columns != 0:
            print("Module layout error: column_end count or total count mismatch")
            sys.exit(1)

        num_rows = total_modules // num_columns

        # Sort keys for deterministic order
        sorted_keys = sorted(module_data.keys())

        # Build table: rows x columns
        columns = []
        success_cnt = 0
        failure_cnt = 0
        for col in range(num_columns):
            col_entries = []
            for row in range(num_rows):
                idx = col * num_rows + row  # Changed from row * num_columns + col
                if idx < len(sorted_keys):
                    key = sorted_keys[idx]
                    fw_crc = module_data[key]['fw_crc']
                    # Compare CRCs and color accordingly
                    if str(fw_crc) != str(f"{checksum:08X}"):
                        # Red background for mismatch
                        entry = f"\033[41m {key:>3} \033[0m"
                        failure_cnt += 1
                    else:
                        # Green background for match
                        entry = f"\033[42m {key:>3} \033[0m"
                        success_cnt += 1
                    col_entries.append(entry)
                else:
                    col_entries.append("")
            columns.append(col_entries)

        # Transpose columns to rows for tabulate
        table = list(map(list, zip(*columns)))
        print(tabulate(table, tablefmt="heavy_grid"))
        print(f"Success: {success_cnt}, Failure: {failure_cnt}")
    except Exception as e:
        print(f"Failed to get module state: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()