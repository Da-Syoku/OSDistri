#!/usr/bin/python3
# -*- coding: utf-8 -*-

import sys
import json
import urllib.parse
import subprocess
import shlex

def connect_wifi():
    """POSTデータからSSIDとパスワードを受け取り、Wi-Fiに接続する"""
    response = {'status': 'error', 'message': '不明なエラーです。'}
    try:
        # POSTデータ（例: ssid=MyWifi&password=pass）を読み取る
        content_length = int(os.environ.get("CONTENT_LENGTH", 0))
        post_data_raw = sys.stdin.read(content_length)
        # データを辞書にパース
        params = urllib.parse.parse_qs(post_data_raw)
        
        ssid = params.get('ssid', [''])[0]
        password = params.get('password', [''])[0]

        if not ssid:
            response['message'] = 'SSIDが指定されていません。'
            return response

        # ★★★ セキュリティ対策 ★★★
        # 悪意のあるコマンドを実行させないために、引数を安全にクォートする
        safe_ssid = shlex.quote(ssid)
        safe_password = shlex.quote(password)

        # nmcliコマンドを実行
        cmd = [
            "sudo", "/usr/bin/nmcli", "device", "wifi", "connect",
            safe_ssid, "password", safe_password
        ]
        result = subprocess.run(cmd, capture_output=True, text=True)

        if result.returncode == 0:
            response['status'] = 'success'
            response['message'] = f'"{ssid}"への接続に成功しました！'
        else:
            # nmcliからのエラーメッセージを返す
            response['message'] = f'接続に失敗しました: {result.stderr.strip()}'

    except Exception as e:
        response['message'] = f'サーバー内部でエラーが発生しました: {str(e)}'
        
    return response

# --- メイン処理 ---
if __name__ == '__main__':
    # HTTPヘッダーとJSONデータを出力
    print("Content-Type: application/json; charset=utf-8")
    print()
    
    # Python 3.8以降ではosモジュールが必要
    import os
    
    connection_result = connect_wifi()
    print(json.dumps(connection_result, indent=4))