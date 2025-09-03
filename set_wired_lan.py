#!/usr/bin/python3
# -*- coding: utf-8 -*-

import sys
import os
import json
import urllib.parse

def debug_post_data():
    """ブラウザから送られてきたPOSTデータや関連情報を調査して返す"""
    response = {}
    try:
        # Apacheから渡される環境変数を記録
        response['REQUEST_METHOD'] = os.environ.get("REQUEST_METHOD")
        response['CONTENT_LENGTH'] = os.environ.get("CONTENT_LENGTH")
        response['CONTENT_TYPE'] = os.environ.get("CONTENT_TYPE")
        
        # POSTデータを読み取る
        content_length = int(response.get('CONTENT_LENGTH') or 0)
        post_data_raw = sys.stdin.read(content_length)
        response['RAW_POST_DATA'] = post_data_raw
        
        # 読み取ったデータをパースしてみる
        params = urllib.parse.parse_qs(post_data_raw)
        response['PARSED_PARAMS'] = params
        
        response['message'] = "デバッグ情報を取得しました。"
        response['status'] = "debug"

    except Exception as e:
        response['status'] = "error"
        response['message'] = f'デバッグ中にエラーが発生しました: {str(e)}'
        
    return response

# --- メイン処理 ---
if __name__ == '__main__':
    print("Content-Type: application/json; charset=utf-8")
    print()
    
    debug_info = debug_post_data()
    print(json.dumps(debug_info, indent=4))