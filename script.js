
        const list = document.querySelectorAll('.toc li');
        function activeLink(){
            list.forEach((item) =>
            item.classList.remove('active'));
            this.classList.add('active');
            window.addEventListener('DOMContentLoaded', () => {
                 fetchStatus();
            });
        }

            async function fetchStatus() {
               try {
                   const response = await fetch('/cgi-bin/get_status.py');
                   if (!response.ok) {
                   throw new Error(`HTTP error! status: ${response.status}`);
                  }
                   const data = await response.json();

        // ネットワーク情報を表示エリアに反映させる
                   updateNetworkUI(data.network);

                } catch (error) {
                   console.error('Fetch error:', error);
                  const networkDiv = document.getElementById('network-status');
                  networkDiv.innerHTML = `<p style="color: red;">情報の取得に失敗しました。</p>`;
                }
            }

            function updateNetworkUI(networkData) {
             const networkDiv = document.getElementById('network-status');
              if (!networkData) {
                  networkDiv.innerHTML = `<p>情報がありません。</p>`;
                   return;
              }

             let html = '<h3>現在の接続</h3>';
             if (networkData.active_connection && networkData.active_connection.name) {
                  html += `<p><strong>${networkData.active_connection.name}</strong> (${networkData.active_connection.type})</p>`;
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

            networkDiv.innerHTML = html;
        }

        // HTMLエスケープ用の補助関数
            function escapeHTML(str) {
                 return str.replace(/[&<>"']/g, function(match) {
                   return {
                    '&': '&amp;',
                    '<': '&lt;',
                    '>': '&gt;',
                     '"': '&quot;',
                      "'": '&#39;'
                      }[match];
                 });
            }
        list.forEach((item) =>(
            item.addEventListener('click', activeLink)
        ))
        // JavaScript for interactive elements can be added here