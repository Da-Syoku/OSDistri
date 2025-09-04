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

document.getElementById('bluetooth-form').addEventListener('submit', async (event) => {
    event.preventDefault();

    const resultDiv = document.getElementById('bluetooth-status');
    resultDiv.innerHTML = '設定を適用中...';

    const state = document.querySelector('input[name="bluetooth_state"]:checked').value;

    try {
        const response = await fetch('/cgi-bin/set_bluetooth.py', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded',
            },
            body: `state=${encodeURIComponent(state)}`
        });

        const result = await response.json();
        resultDiv.innerHTML = result.message;

    } catch (error) {
        console.error('Bluetooth Setting Error:', error);
        resultDiv.innerHTML = '設定の適用中にエラーが発生しました。';
    }
});