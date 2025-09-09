console.log("script.jsファイルが読み込まれました。"); // チェックポイント1

// TOC（目次）のハイライト機能
const list = document.querySelectorAll('.toc li');
function activeLink(){
    list.forEach((item) => item.classList.remove('active'));
    this.classList.add('active');
}
list.forEach((item) => item.addEventListener('click', activeLink));

// ページが読み込まれたら、システムの状態を取得するイベントリスナー
window.addEventListener('DOMContentLoaded', () => {
    console.log("DOMContentLoadedイベントが発生しました。fetchStatusを呼び出します。"); // チェックポイント2
    fetchStatus();
});

async function fetchStatus() {
    console.log("fetchStatus関数が開始されました。"); // チェックポイント3
    try {
        const response = await fetch('/cgi-bin/get_status.py');
        console.log("fetchリクエストを送信しました。レスポンス:", response); // チェックポイント4

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        const data = await response.json();
        console.log("JSONデータを取得しました:", data); // チェックポイント5

        updateNetworkUI(data.network);
        updateBluetoothUI(data.bluetooth);
        updateMediaUI(data.media);
         updateBatteryUI(data.battery);
    } catch (error) {
        console.error('fetchStatusの処理中にエラーが発生しました:', error);
    }
}

function updateNetworkUI(networkData) {
    console.log("updateNetworkUI関数が開始されました。"); // チェックポイント6
    const networkDiv = document.getElementById('network-status');
    
    // ▼▼▼ おそらく、ここが一番重要なチェックポイントです ▼▼▼
    if (!networkDiv) {
        console.error("エラー: ID 'network-status' の要素が見つかりません！ HTMLファイルを確認してください。"); // チェックポイント7
        return;
    }
    
    if (!networkData) {
        networkDiv.innerHTML = `<p>情報がありません。</p>`;
        return;
    }

    let html = '<h3>現在の接続</h3>';
    if (networkData.active_connection && networkData.active_connection.name) {
        html += `<p><strong>${escapeHTML(networkData.active_connection.name)}</strong> (${networkData.active_connection.type})</p>`;
        html += `<p>IPアドレス: ${networkData.active_connection.ip_address}</p>`;
    } else {
        html += `<p>接続されていません。</p>`;
    }

    html += '<h3>利用可能なWi-Fi</h3>';
    if (networkData.wifi_networks && networkData.wifi_networks.length > 0) {
        html += '<ul style="list-style: none; padding: 0;">';
        networkData.wifi_networks.forEach(wifi => {
             html += `<li style="display: flex; justify-content: space-between; align-items: center; padding: 0.5em; border-bottom: 1px solid #eee;">
                        <span><strong>${escapeHTML(wifi.ssid)}</strong> (強度: ${wifi.signal})</span>
                        <button class="connect-btn" data-ssid="${escapeHTML(wifi.ssid)}">接続</button>
                     </li>`;
                    });
        html += '</ul>';
    } else {
        html += '<p>利用可能なWi-Fiネットワークはありません。</p>';
         document.querySelectorAll('.connect-btn').forEach(button => {
        button.addEventListener('click', (event) => {
            const ssid = event.target.dataset.ssid;
            connectToWifi(ssid);
        });
    });
    }
    
    console.log("ネットワークUIを更新します。"); // チェックポイント8
    networkDiv.innerHTML = html;
}

async function connectToWifi(ssid) {
    const password = prompt(`「${ssid}」のパスワードを入力してください:`);
    if (password === null) { // キャンセルが押された場合
        return;
    }

    alert('接続を試みています...');

    try {
        const response = await fetch('/cgi-bin/connect_wifi.py', {
            method: 'POST', // パスワードを送るためPOSTメソッドを使用
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded',
            },
            body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}`
        });

        const result = await response.json();
        alert(result.message); // 結果をアラートで表示

        // 接続結果を反映するために、ネットワーク情報を再取得
        fetchStatus();

    } catch (error) {
        console.error('Connection error:', error);
        alert('接続処理中にエラーが発生しました。');
    }
}

function escapeHTML(str) {
    if (typeof str !== 'string') return '';
    return str.replace(/[&<>"']/g, function(match) {
        return {'&': '&amp;','<': '&lt;','>': '&gt;','"': '&quot;',"'": '&#39;'}[match];
    });
}
// IP設定フォームのラジオボタンのイベントリスナー
document.querySelectorAll('input[name="ip_method"]').forEach(radio => {
    radio.addEventListener('change', (event) => {
        const staticFields = document.getElementById('static-fields');
        if (event.target.value === 'manual') {
            staticFields.style.display = 'block';
        } else {
            staticFields.style.display = 'none';
        }
    });
});

// フォーム送信時のイベントリスナー
document.getElementById('wired-lan-form').addEventListener('submit', async (event) => {
    event.preventDefault(); // デフォルトのフォーム送信をキャンセル

    const resultDiv = document.getElementById('form-result');
    resultDiv.innerText = '設定を適用中...';
    
    const method = document.querySelector('input[name="ip_method"]:checked').value;
    const address = document.getElementById('ip_address').value;
    const gateway = document.getElementById('gateway').value;
    const dns = document.getElementById('dns').value;

    try {
        const response = await fetch('/cgi-bin/set_wired_lan.py', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded',
            },
            body: `method=${encodeURIComponent(method)}&address=${encodeURIComponent(address)}&gateway=${encodeURIComponent(gateway)}&dns=${encodeURIComponent(dns)}`
        });

        const result = await response.json();
        resultDiv.innerText = result.message;
        
        // 成功したら、ネットワーク情報を更新
        if (result.status === 'success') {
            setTimeout(fetchStatus, 2000); // 2秒待ってから情報を再取得
        }

    } catch (error) {
        console.error('IP Setting Error:', error);
        resultDiv.innerText = '設定の適用中にエラーが発生しました。';
    }
});


function updateBluetoothUI(btData) {
    const btDiv = document.getElementById('bluetooth-status');
    if (!btData || btData.error) {
        btDiv.innerHTML = `<p style="color: red;">Bluetooth情報の取得に失敗しました。</p><p>${btData ? btData.error : ''}</p>`;
        return;
    }

    let html = '<h3>コントローラー</h3>';
    if (btData.controller && btData.controller.mac) {
        html += `<p><strong>${btData.controller.name}</strong> (${btData.controller.mac})</p>`;
        // トグルスイッチ
        html += `
            <div style="display: flex; gap: 1em; margin-top: 1em;">
                <label>電源: <button class="bt-action-btn" data-action="${btData.controller.powered ? 'power_off' : 'power_on'}">${btData.controller.powered ? 'ON' : 'OFF'}</button></label>
                <label>ペアリング可: <button class="bt-action-btn" data-action="${btData.controller.pairable ? 'pairable_off' : 'pairable_on'}">${btData.controller.pairable ? 'ON' : 'OFF'}</button></label>
            </div>
        `;
    } else {
        html += '<p>コントローラーが見つかりません。</p>';
    }

    html += '<h3 style="margin-top: 1.5em;">デバイス</h3>';
    html += `<button class="bt-action-btn" data-action="scan">デバイスをスキャン</button>`;

    if (btData.devices && btData.devices.length > 0) {
        html += '<ul style="list-style: none; padding: 0; margin-top: 1em;">';
        btData.devices.forEach(dev => {
            html += `<li style="display: flex; justify-content: space-between; align-items: center; padding: 0.5em; border-bottom: 1px solid #eee;">
                        <span><strong>${escapeHTML(dev.name)}</strong><br><small>${dev.mac}</small></span>
                        <span style="display: flex; gap: 0.5em;">
                            ${!dev.paired ? `<button class="bt-action-btn" data-action="pair" data-mac="${dev.mac}">ペア</button>` : ''}
                            ${dev.paired && !dev.connected ? `<button class="bt-action-btn" data-action="connect" data-mac="${dev.mac}">接続</button>` : ''}
                            ${dev.connected ? `<button class="bt-action-btn" data-action="disconnect" data-mac="${dev.mac}">切断</button>` : ''}
                        </span>
                     </li>`;
        });
        html += '</ul>';
    } else {
        html += '<p style="margin-top: 1em;">デバイスが見つかりません。</p>';
    }

    btDiv.innerHTML = html;

    // 作成した全てのボタンにイベントリスナーを設定
    document.querySelectorAll('.bt-action-btn').forEach(button => {
        button.addEventListener('click', (event) => {
            const action = event.target.dataset.action;
            const mac = event.target.dataset.mac || ''; // MACアドレスがない場合もある
            setBluetoothAction(action, mac);
        });
    });
}

async function setBluetoothAction(action, mac) {
    if (action === 'scan') {
        alert('5秒間デバイスをスキャンします...');
    } else {
        alert(`操作を実行しています: ${action} ${mac}`);
    }

    try {
        const response = await fetch('/cgi-bin/set_bluetooth.py', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `action=${encodeURIComponent(action)}&mac=${encodeURIComponent(mac)}`
        });
        
        const result = await response.json();
        alert(result.message);

        // 状態を更新するために、情報を再取得
        fetchStatus();

    } catch (error) {
        console.error('Bluetooth Action Error:', error);
        alert('Bluetooth操作中にエラーが発生しました。');
    }
}

// --- ▼▼▼ script.jsの末尾にまるごと追記 ▼▼▼ ---

function updateMediaUI(mediaData) {
    const mediaDiv = document.getElementById('media-controls');
    if (!mediaData || mediaData.error) {
        mediaDiv.innerHTML = `<p style="color: red;">情報取得に失敗しました。</p>`;
        return;
    }

    const vol = mediaData.volume;
    const bright = mediaData.brightness;

    let html = `
        <div style="margin-bottom: 1.5em;">
            <h4>音量</h4>
            <div style="display: flex; align-items: center; gap: 1em;">
                <span>0</span>
                <input type="range" id="volume-slider" min="0" max="100" value="${vol.percent}" style="flex-grow: 1;">
                <span>100</span>
                <span id="volume-value">${vol.percent}%</span>
                <button id="mute-btn">${vol.muted ? 'ミュート解除' : 'ミュート'}</button>
            </div>
        </div>
        <div>
            <h4>画面の明るさ</h4>
            <div style="display: flex; align-items: center; gap: 1em;">
                <span>0</span>
                <input type="range" id="brightness-slider" min="0" max="100" value="${bright.percent}" style="flex-grow: 1;">
                <span>100</span>
                <span id="brightness-value">${bright.percent}%</span>
            </div>
        </div>
    `;
    mediaDiv.innerHTML = html;

    // --- イベントリスナーを設定 ---
    const volumeSlider = document.getElementById('volume-slider');
    const brightnessSlider = document.getElementById('brightness-slider');
    const muteBtn = document.getElementById('mute-btn');

    // スライダーを動かし終わった時にイベント発火
    volumeSlider.addEventListener('change', () => setMediaAction('set_volume', volumeSlider.value));
    brightnessSlider.addEventListener('change', () => setMediaAction('set_brightness', brightnessSlider.value));
    
    // スライダーを動かしている最中もパーセント表示を更新
    volumeSlider.addEventListener('input', () => { document.getElementById('volume-value').innerText = `${volumeSlider.value}%`; });
    brightnessSlider.addEventListener('input', () => { document.getElementById('brightness-value').innerText = `${brightnessSlider.value}%`; });
    
    muteBtn.addEventListener('click', () => setMediaAction('toggle_mute', ''));
}

async function setMediaAction(action, value) {
    try {
        const response = await fetch('/cgi-bin/set_media.py', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `action=${encodeURIComponent(action)}&value=${encodeURIComponent(value)}`
        });
        const result = await response.json();
        if (result.status === 'success') {
            // 成功したら、UIの状態を再取得して更新
            fetchStatus();
        } else {
            alert(`エラー: ${result.message}`);
        }
    } catch (error) {
        console.error('Media Action Error:', error);
        alert('操作中にエラーが発生しました。');
    }
}

function updateBatteryUI(batteryData) {
    const batteryDiv = document.getElementById('battery-status');
    if (!batteryData || batteryData.error) {
        batteryDiv.innerHTML = `<p>${batteryData ? batteryData.error : '情報なし'}</p>`;
        return;
    }

    // 充電状態に応じてアイコンとテキスト、色を決定
    let statusIcon = '🔋';
    let statusText = batteryData.status;
    let statusColor = '#333';

    if (batteryData.status === 'Charging') {
        statusIcon = '🔌';
        statusText = '充電中';
        statusColor = '#28a745'; // 緑色
    } else if (batteryData.status === 'Discharging') {
        statusText = '放電中';
        statusColor = '#0275d8'; // 青色
    } else if (batteryData.status === 'Full') {
        statusIcon = '🔌';
        statusText = '満充電';
    }

    let html = `
        <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 1em 2em; text-align: left;">
            <div>
                <h4>現在の残量</h4>
                <div style="display: flex; align-items: center; font-size: 1.5em; font-weight: bold;">
                    <span style="font-size: 2em; margin-right: 0.2em;">${statusIcon}</span>
                    <span>${batteryData.percent}%</span>
                </div>
                <progress value="${batteryData.percent}" max="100" style="width: 100%;"></progress>
                <p style="color: ${statusColor};">${statusText}</p>
            </div>
            <div>
                <h4>バッテリー消耗度</h4>
                <p style="font-size: 1.2em; font-weight: bold;">現在の最大容量: ${batteryData.health_percent}%</p>
                <progress value="${batteryData.health_percent}" max="100" style="width: 100%;"></progress>
                <p style="font-size: 0.8em; color: #666;">
                    設計容量: ${batteryData.design_capacity_mwh} MWh<br>
                    現在のフル充電容量: ${batteryData.last_full_capacity_mwh} MWh
                </p>
            </div>
        </div>
    `;

    batteryDiv.innerHTML = html;
}