#!/usr/bin/python3
# -*- coding: utf-8 -*-

import os
import urllib.parse
import subprocess
import html

# (ヘッダー出力とパラメータ取得部分は変更なし)
print("Content-Type: text/html; charset=utf-8")
print()

action = None
query_string = os.environ.get('QUERY_STRING', '')
if query_string:
    params = urllib.parse.parse_qs(query_string)
    action_values = params.get('action')
    if action_values:
        action = action_values[0]

# (HTML開始部分とスタイルは変更なし)
print("""
<!DOCTYPE html>
<html>
<head>
    <title>システムコントロール</title>
    <meta charset="UTF-8">
    <style>
        body { font-family: sans-serif; background-color: #f0f0f0; text-align: center; padding-top: 50px; }
        .container { background-color: white; padding: 2em; margin: auto; max-width: 400px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }
        h1 { color: #333; }
        p { color: #555; }
        .message { font-weight: bold; }
        a { color: #0275d8; text-decoration: none; }
    </style>
</head>
<body>
<div class="container">
    <h1>システムコントロール</h1>
""")

message = ""
# --- ▼▼▼ ここから処理分岐 ▼▼▼ ---
if action == "shutdown":
    try:
        subprocess.run(["sudo", "/sbin/shutdown", "-h", "now"], check=True)
        message = "システムをシャットダウンします..."
    except Exception as e:
        message = f"エラーが発生しました: {html.escape(str(e))}"

elif action == "reboot":
    try:
        subprocess.run(["sudo", "/sbin/reboot"], check=True)
        message = "システムを再起動します..."
    except Exception as e:
        message = f"エラーが発生しました: {html.escape(str(e))}"

# --- ▼▼▼ スリープ処理を追加 ▼▼▼ ---
elif action == "suspend":
    try:
        subprocess.run(["sudo", "/usr/bin/systemctl", "suspend"], check=True)
        message = "システムをスリープさせます..."
    except Exception as e:
        message = f"エラーが発生しました: {html.escape(str(e))}"

else:
    # actionが指定されていない場合は、パネルへのリンクを表示
    message = '操作パネルから操作を選択してください。<br><br><a href="/">操作パネルに戻る</a>'


print(f'<p class="message">{message}</p>')
print("""
</div>
</body>
</html>
""")