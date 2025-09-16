#! /usr/bin/env python3

import argparse
import threading
import time
import sys
import websocket 
import socket


def main():
    parser = argparse.ArgumentParser(description="WebSocket logger for OpenFlap module")
    parser.add_argument("--url", default="openflap.local", help="Controller URL or ws URL (default: openflap.local)")

    args = parser.parse_args()

    # Accept full ws URL or build default
    ws_url = f"ws://{args.url}/log"

    print(socket.gethostbyname(args.url))

    stop_ws = threading.Event()
    ws_app = None

    def on_ws_open(_ws):
        print(f"[ws] connected: {ws_url}", file=sys.stderr)

    # Remove on_message and handle all frames in on_data
    def on_ws_data(_ws, data, data_type, cont):
        if data_type == websocket.ABNF.OPCODE_TEXT:
            # data can be str or bytes depending on websocket-client version
            if isinstance(data, (bytes, bytearray)):
                text = data.decode("utf-8", errors="replace")
            else:
                text = str(data)
            print(text, flush=True, end='')

    def on_ws_error(_ws, error):
        print(f"[ws] error: {error}", file=sys.stderr)

    def on_ws_close(_ws, *_args):
        print("[ws] closed", file=sys.stderr)

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
        while ws_thread.is_alive():
            time.sleep(0.5)
    except KeyboardInterrupt:
        pass
    finally:
        stop_ws.set()
        try:
            if ws_app is not None:
                ws_app.close()
        except Exception:
            pass
        ws_thread.join(timeout=2)


if __name__ == "__main__":
    main()