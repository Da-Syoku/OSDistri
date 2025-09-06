#!/usr/bin/python3
# -*- coding: utf-8 -*-

import sys
import os
import json
import urllib.parse
import subprocess
import time

def run_bt_command(command_string):
    """対話的なbluetoothctlにコマンドをパイプで送り込み、実行する"""
    # echo -e '...' は改行を含む文字列をbluetoothctlの標準入力に渡すためのテクニック
    return subprocess.run(
        f"echo -e '{command_string}' | bluetoothctl",
        shell=True, capture_output=True, text=True
    )

def set_bluetooth_action():
    response = {'status': 'error', 'message': '不明なエラーです。'}
    try:
        content_length = int(os.environ.get("CONTENT_LENGTH", 0))
        post_data_raw = sys.stdin.read(content_length)
        params = urllib.parse.parse_qs(post_data_raw)
        
        action = params.get('action', [''])[0]
        mac = params.get('mac', [''])[0]

        cmd = ""
        if action == 'power_on':
            cmd = 'power on'
        elif action == 'power_off':
            cmd = 'power off'
        elif action == 'pairable_on':
            cmd = 'pairable on'
        elif action == 'pairable_off':
            cmd = 'pairable off'
        elif action == 'scan':
            # スキャンは少し特殊。5秒間スキャンして停止する。
            run_bt_command('scan on')
            time.sleep(5)
            run_bt_command('scan off')
            response['status'] = 'success'
            response['message'] = '5秒間のスキャンが完了しました。'
            return response
        elif action == 'pair':
            cmd = f'pair {mac}'
        elif action == 'connect':
            cmd = f'connect {mac}'
        elif action == 'disconnect':
            cmd = f'disconnect {mac}'
        else:
            response['message'] = '無効なアクションです。'
            return response

        result = run_bt_command(cmd)

        if "failed" in result.stdout.lower() or "error" in result.stdout.lower():
            response['message'] = f'操作に失敗しました: {result.stdout.strip()}'
        else:
            response['status'] = 'success'
            response['message'] = f'操作が完了しました: {result.stdout.strip()}'

    except Exception as e:
        response['message'] = f'サーバー内部でエラーが発生しました: {str(e)}'
        
    return response

# --- メイン処理 ---
if __name__ == '__main__':
    print("Content-Type: application/json; charset=utf-8")
    print()
    
    result = set_bluetooth_action()
    print(json.dumps(result, indent=4))