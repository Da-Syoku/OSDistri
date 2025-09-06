#!/usr/bin/python3
# -*- coding: utf-8 -*-

import subprocess
import json
import re

def get_network_status():
    """nmcliコマンドを使ってネットワーク情報を取得し、辞書として返す"""
    status = {}
    try:
        # 接続状態の概要を取得 (IPアドレスなど)
        result = subprocess.run(['nmcli', 'con', 'show', '--active'], capture_output=True, text=True, check=True)
        active_connection = {}
        # 最初の接続情報を解析（複数接続対応は省略）
        lines = result.stdout.strip().split('\n')
        if len(lines) > 1:
            headers = lines[0].split()
            values = lines[1].split()
            # 簡易的にIP4アドレスを取得
            ip_res = subprocess.run(['ip', 'addr', 'show', values[2]], capture_output=True, text=True)
            ip_match = re.search(r'inet (\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})', ip_res.stdout)
            if ip_match:
                active_connection['ip_address'] = ip_match.group(1)
            else:
                 active_connection['ip_address'] = 'N/A'
            active_connection['name'] = values[0]
            active_connection['device'] = values[2]
            active_connection['type'] = values[1]
        status['active_connection'] = active_connection

        # 利用可能なWi-Fiリストを取得
        wifi_result = subprocess.run(['nmcli', '-f', 'SSID,SIGNAL,SECURITY', 'dev', 'wifi', 'list', '--rescan', 'yes'], capture_output=True, text=True, check=True)
        wifi_list = []
        for line in wifi_result.stdout.strip().split('\n')[1:]:
            parts = re.split(r'\s{2,}', line.strip())
            if len(parts) >= 2:
                wifi_list.append({'ssid': parts[0], 'signal': parts[1], 'security': parts[2] if len(parts) > 2 else 'None'})
        status['wifi_networks'] = wifi_list

    except Exception as e:
        status['error'] = str(e)
        
    return status


def get_bluetooth_status():
    """bluetoothctlコマンドを使ってBluetoothの情報を取得し、辞書として返す"""
    status = {'controller': {}, 'devices': []}
    try:
        # コントローラーの情報を取得
        # bluetoothctl show の出力から情報を抜き出す
        show_result = subprocess.run(['bluetoothctl', 'show'], capture_output=True, text=True, check=True)
        controller_info = {}
        for line in show_result.stdout.strip().split('\n'):
            if 'Controller' in line:
                parts = line.split()
                controller_info['mac'] = parts[1]
                controller_info['name'] = parts[2].strip('()')
            elif 'Powered' in line:
                controller_info['powered'] = (line.split(': ')[1] == 'yes')
            elif 'Pairable' in line:
                controller_info['pairable'] = (line.split(': ')[1] == 'yes')
        status['controller'] = controller_info

        # ペアリング済み・接続済みデバイスの情報を取得
        devices_result = subprocess.run(['bluetoothctl', 'devices'], capture_output=True, text=True, check=True)
        paired_devices_result = subprocess.run(['bluetoothctl', 'devices', 'Paired'], capture_output=True, text=True, check=True)
        connected_devices_result = subprocess.run(['bluetoothctl', 'devices', 'Connected'], capture_output=True, text=True, check=True)

        paired_macs = {line.split()[1] for line in paired_devices_result.stdout.strip().split('\n')}
        connected_macs = {line.split()[1] for line in connected_devices_result.stdout.strip().split('\n')}

        for line in devices_result.stdout.strip().split('\n'):
            parts = line.split(' ', 2)
            mac = parts[1]
            name = parts[2]
            status['devices'].append({
                'mac': mac,
                'name': name,
                'paired': mac in paired_macs,
                'connected': mac in connected_macs
            })

    except Exception as e:
        status['error'] = str(e)

    return status


# --- メイン処理 ---
if __name__ == '__main__':
    # HTTPヘッダーとJSONデータを出力
    print("Content-Type: application/json; charset=utf-8")
    print() # ヘッダーと本体の区切り
    
    system_status = {}
    system_status['network'] = get_network_status()
    system_status['bluetooth'] = get_bluetooth_status() # Bluetooth情報を追加

    print(json.dumps(system_status, indent=4))
