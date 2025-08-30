#!/usr/bin/python3
# -*- coding: utf-8 -*-

import sys
import os
import json
import urllib.parse
import subprocess
import shlex

def set_wired_lan():
    response = {'status': 'error', 'message': '不明なエラーです。'}
    try:
        # POSTデータを読み取る
        content_length = int(os.environ.get("CONTENT_LENGTH", 0))
        post_data_raw = sys.stdin.read(content_length)
        params = urllib.parse.parse_qs(post_data_raw)
        
        method = params.get('method', [''])[0]

        # アクティブな有線LANの接続名を取得する
        result = subprocess.run(
            ['nmcli', '-t', '-f', 'NAME,TYPE', 'con', 'show', '--active'],
            capture_output=True, text=True, check=True
        )
        wired_connection_name = None
        for line in result.stdout.strip().split('\n'):
            if 'ethernet' in line:
                wired_connection_name = line.split(':')[0]
                break
        
        if not wired_connection_name:
            response['message'] = 'アクティブな有線LAN接続が見つかりません。'
            return response

        safe_conn_name = shlex.quote(wired_connection_name)
        
        # --- 設定変更コマンドを組み立てる ---
        if method == 'auto':
            cmd_mod = ["sudo", "/usr/bin/nmcli", "con", "mod", safe_conn_name, "ipv4.method", "auto"]
            # 以前の静的IP設定をクリア
            subprocess.run(["sudo", "/usr/bin/nmcli", "con", "mod", safe_conn_name, "ipv4.gateway", "''"], check=True)
            subprocess.run(["sudo", "/usr/bin/nmcli", "con", "mod", safe_conn_name, "ipv4.dns", "''"], check=True)

        elif method == 'manual':
            address = params.get('address', [''])[0]
            gateway = params.get('gateway', [''])[0]
            dns = params.get('dns', [''])[0]
            
            if not address or not gateway:
                response['message'] = '静的IP設定には、IPアドレスとゲートウェイが必須です。'
                return response
                
            cmd_mod = [
                "sudo", "/usr/bin/nmcli", "con", "mod", safe_conn_name,
                "ipv4.method", "manual",
                "ipv4.addresses", shlex.quote(address),
                "ipv4.gateway", shlex.quote(gateway),
                "ipv4.dns", shlex.quote(dns)
            ]
        else:
            response['message'] = '無効なメソッドです。'
            return response

        # --- コマンドを実行 ---
        mod_result = subprocess.run(cmd_mod, capture_output=True, text=True)
        if mod_result.returncode != 0:
            response['message'] = f'設定変更に失敗: {mod_result.stderr.strip()}'
            return response

        # --- 接続を再適用 ---
        up_result = subprocess.run(
            ["sudo", "/usr/bin/nmcli", "con", "up", safe_conn_name],
            capture_output=True, text=True
        )
        if up_result.returncode != 0:
            response['message'] = f'設定の適用に失敗: {up_result.stderr.strip()}'
            return response

        response['status'] = 'success'
        response['message'] = '設定を正常に適用しました。'

    except Exception as e:
        response['message'] = f'サーバー内部でエラーが発生しました: {str(e)}'
        
    return response

# --- メイン処理 ---
if __name__ == '__main__':
    print("Content-Type: application/json; charset=utf-8")
    print()
    
    result = set_wired_lan()
    print(json.dumps(result, indent=4))