#!/usr/bin/python3
# -*- coding: utf-8 -*-

import cgi
import subprocess
import html

print("Content-Type: text/html; charset=utf-8")
print()

form = cgi.FieldStorage()
action = form.getvalue("action")

print("""
<!DOCTYPE html>
<html>
<head>
    <title>Web Page Dashboard</title>
</head>
<body>
    <div>
        <h1>Web Page DashBoard</h1>
""")

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

# 結果メッセージとHTMLの終了部分
print(f'<p class="message">{message}</p>')
print("""
</div>
</body>
</html>
""")