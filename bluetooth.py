#!/usr/bin/python3
# -*- coding: utf-8 -*-

import os
import sys
import json

def set_bluetooth_state(state):
    # Bluetoothの状態を設定する関数
    if state not in ["on", "off"]:
        print("無効な状態です。'on'または'off'を指定してください。")
        return

    # Bluetoothの状態を設定するコマンドを実行
    command = f"bluetoothctl power {state}"
    os.system(command)

    # Bluetoothの状態を取得するコマンドを実行
    command = "bluetoothctl show"
    output = os.popen(command).read()
    print("Bluetoothの状態:")
    print(output)

    # Bluetoothの状態をJSON形式で返す
    status = {}
    for line in output.splitlines():
        if line.startswith("  "):
            key, value = line.split(":", 1)
            status[key.strip()] = value.strip()
    return json.dumps(status, ensure_ascii=False)

# --- メイン処理 ---
if __name__ == '__main__':
    print("Content-Type: application/json; charset=utf-8")
    print()

    # コマンドライン引数からBluetoothの状態を取得
    if len(sys.argv) > 1:
        state = sys.argv[1].lower()
    else:
        state = "on"  # デフォルトは'on'

    result = set_bluetooth_state(state)
    print(result)

    # 結果をJSON形式で返す
    print("Content-Type: application/json; charset=utf-8")
    print()
    print(result)

