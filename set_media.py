#!/usr/bin/python3
# -*- coding: utf-8 -*-

import sys
import os
import json
import urllib.parse
import subprocess
import shlex

def set_media_action():
    response = {'status': 'error', 'message': '不明なエラーです。'}
    try:
        content_length = int(os.environ.get("CONTENT_LENGTH", 0))
        post_data_raw = sys.stdin.read(content_length)
        params = urllib.parse.parse_qs(post_data_raw)
        
        action = params.get('action', [''])[0]
        value = params.get('value', [''])[0]

        cmd = []
        if action == 'set_volume':
            cmd = ["sudo", "/usr/bin/amixer", "set", "Master", f"{shlex.quote(value)}%"]
        elif action == 'toggle_mute':
            cmd = ["sudo", "/usr/bin/amixer", "set", "Master", "toggle"]
        elif action == 'set_brightness':
            cmd = ["sudo", "/usr/bin/brightnessctl", "set", f"{shlex.quote(value)}%"]
        else:
            response['message'] = '無効なアクションです。'
            return response
            
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode == 0:
            response['status'] = 'success'
            response['message'] = '設定を変更しました。'
        else:
            response['message'] = f'コマンド実行エラー: {result.stderr.strip()}'

    except Exception as e:
        response['message'] = f'サーバー内部でエラーが発生しました: {str(e)}'
        
    return response

# --- メイン処理 ---
if __name__ == '__main__':
    print("Content-Type: application/json; charset=utf-8")
    print()
    
    result = set_media_action()
    print(json.dumps(result, indent=4))