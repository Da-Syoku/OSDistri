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
            html += `<li style="text-align: left; padding: 0.5em; border-bottom: 1px solid #eee;"><strong>${escapeHTML(wifi.ssid)}</strong> (強度: ${wifi.signal})</li>`;
        });
        html += '</ul>';
    } else {
        html += '<p>利用可能なWi-Fiネットワークはありません。</p>';
    }
    
    console.log("ネットワークUIを更新します。"); // チェックポイント8
    networkDiv.innerHTML = html;
}

function escapeHTML(str) {
    if (typeof str !== 'string') return '';
    return str.replace(/[&<>"']/g, function(match) {
        return {'&': '&amp;','<': '&lt;','>': '&gt;','"': '&quot;',"'": '&#39;'}[match];
    });
}