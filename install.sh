#!/bin/bash

# 日本語表示のための設定 (ダイアログ表示用)
export LC_ALL=C
#dialog --title "インストール開始" --msgbox "Arch Linuxインストーラーへようこそ！\nこのインストーラーはUEFI/GPT環境向けです。" 8 40
dialog --title "Installation Start" --msgbox "Welcome to the Arch Linux Installer!\nThis installer is for UEFI/GPT environments." 8 40
export LANG=ja_JP.UTF-8
export LC_ALL=ja_JP.UTF-8
# locale.genにja_JP.UTF-8を追記
if ! grep -q "ja_JP.UTF-8" /etc/locale.gen; then
    echo "ja_JP.UTF-8 UTF-8" >> /etc/locale.gen
fi
locale-gen

# ディスク選択
DISK=$(dialog --menu "インストール先ディスクを選択してください" 20 60 10 1 "/dev/sda" "ディスク A" 2 "/dev/sdb" "ディスク B" 2>/dev/tty)
if [ -z "$DISK" ]; then
    echo "ディスクが選択されませんでした。終了します。"
    exit 1
fi

# パーティション自動作成
dialog --yesno "選択したディスク ($DISK) の全データを消去して、自動パーティショニングしますか？" 8 60
if [ $? -eq 0 ]; then
    echo "パーティショニングを開始します..."
    # 既存のパーティションを削除
    sgdisk --zap-all $DISK
    # EFIパーティションの作成 (512MB)
    sgdisk -n 1:0:+512MiB -t 1:ef00 -c 1:"EFI System" $DISK
    # ルートパーティションの作成 (残りすべて)
    sgdisk -n 2:0:0 -t 2:8300 -c 2:"Linux Root" $DISK
else
    dialog --msgbox "手動でパーティションを作成してください。cfdiskを起動します。" 8 40
    cfdisk $DISK
    # 手動パーティション後の処理は省略
fi

# フォーマット
dialog --msgbox "パーティションをフォーマットします。" 8 40
mkfs.fat -F32 "${DISK}1"
mkfs.ext4 "${DISK}2"

# マウント
dialog --msgbox "パーティションをマウントします。" 8 40
mount "${DISK}2" /mnt
mkdir -p /mnt/boot/efi
mount "${DISK}1" /mnt/boot/efi

# --- ここからユーザー入力部分 ---

# ユーザー名の設定
USERNAME=$(dialog --inputbox "新しいユーザー名を入力してください" 8 40 3>&1 1>&2 2>&3)
if [ -z "$USERNAME" ]; then
    echo "ユーザー名が入力されませんでした。終了します。"
    exit 1
fi

# パスワード設定
PASSWORD=$(dialog --passwordbox "パスワードを入力してください" 8 40 3>&1 1>&2 2>&3)
if [ -z "$PASSWORD" ]; then
    echo "パスワードが入力されませんでした。終了します。"
    exit 1
fi
RE_PASSWORD=$(dialog --passwordbox "パスワードを再入力してください" 8 40 3>&1 1>&2 2>&3)
if [ "$PASSWORD" != "$RE_PASSWORD" ]; then
    dialog --msgbox "パスワードが一致しません。終了します。" 8 40
    exit 1
fi

# --- ここからインストール処理 ---
dialog --infobox "ベースシステムをインストールしています..." 8 40
pacstrap /mnt base linux linux-firmware

dialog --infobox "fstabを生成しています..." 8 40
genfstab -U /mnt >> /mnt/etc/fstab

dialog --infobox "初期設定をしています..." 8 40
arch-chroot /mnt /bin/bash <<EOF
    # タイムゾーンの設定
    ln -sf /usr/share/zoneinfo/Asia/Tokyo /etc/localtime
    hwclock --systohc

    # ロケール設定
    #echo "ja_JP.UTF-8 UTF-8" >> /etc/locale.gen
    #locale-gen
    #echo "LANG=ja_JP.UTF-8" > /etc/locale.conf

    # ホスト名の設定
    echo "my-arch-pc" > /etc/hostname

    # rootパスワードの設定
    echo "root:$(openssl passwd -1 -salt xyz)" | chpasswd

    # 新しいユーザーの作成とパスワード設定
    useradd -m -G wheel,video,audio,storage $USERNAME
    echo "$USERNAME:$PASSWORD" | chpasswd
    
    # sudo権限を付与
    echo "%wheel ALL=(ALL:ALL) ALL" >> /etc/sudoers

    # NetworkManagerを有効化
    systemctl enable NetworkManager

    # Airootfsからカスタムファイルをコピー
    # ライブ環境からインストール先へファイルをコピー
    # cp -r /srv/http/* /srv/http/   <- カスタムファイルのコピー

EOF

dialog --msgbox "インストールが完了しました！\n再起動してください。" 8 40
reboot