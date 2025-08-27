#!/usr/bin/python3
# -*- coding: utf-8 -*-

import os
import urllib.parse
import subprocess
import html

# CGIスクリプトとして動作するための準備
print("Content-Type: text/html; charset=utf-8")
print() # ヘッダーと本体を区切るための空行

# --- ▼▼▼ cgiモジュールの代替部分 ▼▼▼ ---
action = None
# 環境変数からURLのクエリ文字列を取得
query_string = os.environ.get('QUERY_STRING', '')
if query_string:
    # クエリ文字列を解析
    params = urllib.parse.parse_qs(query_string)
    # 'action'パラメータの値を取得 (例: {'action': ['shutdown']})
    action_values = params.get('action')
    if action_values:
        action = action_values[0]
# --- ▲▲▲ ここまでが変更箇所 ▲▲▲ ---


# HTMLの開始部分 (変更なし)
print("""
<!DOCTYPE html>
<html>
<head>
    <title>システムコントロール</title>
    <style>
        body { font-family: sans-serif; background-color: #f0f0f0; text-align: center; padding-top: 50px; }
        .container { background-color: white; padding: 2em; margin: auto; max-width: 400px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }
        h1 { color: #333; }
        p { color: #555; }
        .message { font-weight: bold; color: #d9534f; }
    </style>
</head>
<body>
<div class="container">
    <h1>システムコントロール</h1>
""")

# actionパラメータの値によって処理を分岐 (変更なし)
message = ""
if action == "shutdown":
    try:
        # シャットダウンコマンドを実行
        subprocess.run(["sudo", "/sbin/shutdown", "-h", "now"], check=True)
        message = "システムをシャットダウンします..."
    except Exception as e:
        message = f"エラーが発生しました: {html.escape(str(e))}"

elif action == "reboot":
    try:
        # 再起動コマンドを実行
        subprocess.run(["sudo", "/sbin/reboot"], check=True)
        message = "システムを再起動します..."
    except Exception as e:
        message = f"エラーが発生しました: {html.escape(str(e))}"

else:
    message = "操作を選択してください。"

# 結果メッセージとHTMLの終了部分 (変更なし)
print(f'<p class="message">{message}</p>')
print("""
</div>
</body>
</html>
""")