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
import threading
import re
import time
import websocket

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

    # Upload firmware with progress driven by WebSocket logs
    firmware_url = f"http://{args.url}/api/module/firmware.bin"
    ws_url = f"ws://{args.url}/log"
    print("Uploading firmware to", firmware_url)

    # Regular PUT will stream the file; progress will be tracked from device log via WS
    ws_thread = None
    ws_app = None
    stop_ws = threading.Event()

    # Regex to capture "... writing <chunk> <written>/<total> bytes"
    # Example: "MODULE_FIRMWARE_ENDPOINTS: writing 128 26368/28672 bytes"
    progress_re = re.compile(r'writing\s+\d+\s+(\d+)/(\d+)\s+bytes', re.IGNORECASE)

    # Prepare progress bar (total may be unknown initially; will adjust when first WS msg arrives)
    with tqdm(total=0, unit='B', unit_scale=True, desc="Flashing", ncols=100) as pbar:
        ws_last_written = {'value': 0}  # mutable container for closure
        line_buf = {'s': ""}  # accumulate partial text frames

        def _process_line(line: str):
            m = progress_re.search(line)
            if not m:
                return
            try:
                written = int(m.group(1))
                total = int(m.group(2))
            except Exception:
                return
            if pbar.total != total:
                pbar.total = total
                pbar.refresh()
            delta = written - ws_last_written['value']
            if delta > 0:
                pbar.update(delta)
                ws_last_written['value'] = written

        # Replace on_message with on_data to handle text frames consistently
        def on_ws_data(_ws, data, data_type, cont):
            if data_type == websocket.ABNF.OPCODE_TEXT:
                text = data.decode("utf-8", errors="replace") if isinstance(data, (bytes, bytearray)) else str(data)
                line_buf['s'] += text
                while True:
                    idx = line_buf['s'].find('\n')
                    if idx == -1:
                        break
                    line, rest = line_buf['s'][:idx], line_buf['s'][idx+1:]
                    line_buf['s'] = rest
                    _process_line(line.rstrip('\r'))

        def on_ws_error(_ws, error):
            # Keep quiet; progress will just not update from WS
            pass

        def on_ws_close(_ws, *_args):
            # No-op
            pass

        def ws_runner():
            nonlocal ws_app
            ws_app = websocket.WebSocketApp(
                ws_url,
                on_data=on_ws_data,
                on_error=on_ws_error,
                on_close=on_ws_close,
            )
            # Run until stop flag set; run_forever blocks, so we close it when stopping
            while not stop_ws.is_set():
                try:
                    ws_app.run_forever()
                except Exception:
                    # Backoff a little on reconnect attempt
                    time.sleep(0.2)
                if stop_ws.is_set():
                    break
                time.sleep(0.1)

        ws_thread = threading.Thread(target=ws_runner, daemon=True)
        ws_thread.start()

        try:
            with open(args.binfile, 'rb') as f:
                # Stream upload; server generates WS progress logs while writing
                resp = requests.put(
                    firmware_url,
                    data=f,
                    headers={'Content-Type': 'application/octet-stream'},
                    timeout=120
                )
        finally:
            # Stop WS listener
            stop_ws.set()
            if ws_app is not None:
                try:
                    ws_app.close()
                except Exception:
                    pass
            if ws_thread is not None:
                ws_thread.join(timeout=2.0)

    # Evaluate upload response
    if resp.status_code != 200:
        print(f"Firmware upload failed: {resp.status_code} {resp.text}")
        sys.exit(1)
    else:
        print("Firmware upload successful!")

    # Get module state
    print("Upload successful!")
    print("Validating module update success...")

    state_url = f"http://{args.url}/api/module"
    try:
        resp = requests.get(state_url, timeout=10)
        resp.raise_for_status()
        modules = resp.json()
        module_data = {}
        for mod in modules:
            module_data[mod.get("module")] = {
                "fw_crc": mod.get("firmware_version", {}).get("crc"),
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
                idx = col * num_rows + row
                if idx < len(sorted_keys):
                    key = sorted_keys[idx]
                    fw_crc = module_data[key]['fw_crc']
                    if str(fw_crc) != str(f"{checksum:08X}"):
                        entry = f"\033[41m {key:>3} \033[0m"
                        failure_cnt += 1
                    else:
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