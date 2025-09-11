#!/bin/bash

#dialog --title "インストール開始" --msgbox "Arch Linuxインストーラーへようこそ！\nこのインストーラーはUEFI/GPT環境向けです。" 8 40
dialog --title "Installation Start" --msgbox "Welcome to the Arch Linux Installer!\nThis installer is for UEFI/GPT environments." 8 40

export LANG=ja_JP.UTF-8
export LC_ALL=ja_JP.UTF-8
locale-gen

# ディスク選択
DISK=$(dialog --menu "インストール先ディスクを選択してください-Select the installation disk" 20 60 10 1 "/dev/sda" "ディスクDisk A" 2 "/dev/sdb" "ディスクDisk B" 2>/dev/tty)
if [ -z "$DISK" ]; then
    echo "No disk selected. Exiting.ディスクが選択されませんでした。終了します。"
    exit 1
fi

# パーティション自動作成
dialog --yesno "All data on the selected disk ($DISK) will be erased.\nDo you want to proceed with automatic partitioning?選択したディスク ($DISK) の全データを消去して、自動パーティショニングしますか？" 8 60
if [ $? -eq 0 ]; then
    echo "パーティショニングを開始します...tarting partitioning.."
    # 既存のパーティションを削除
    sgdisk --zap-all $DISK
    # EFIパーティションの作成 (512MB)
    sgdisk -n 1:0:+512MiB -t 1:ef00 -c 1:"EFI System" $DISK
    # ルートパーティションの作成 (残りすべて)
    sgdisk -n 2:0:0 -t 2:8300 -c 2:"Linux Root" $DISK
else
    dialog --msgbox "Please create partitions manually. Starting cfdisk.手動でパーティションを作成してください。cfdiskを起動します。" 8 40
    cfdisk $DISK
    # 手動パーティション後の処理は省略
fi

# フォーマット
dialog --msgbox "Formatting partitionsパーティションをフォーマットします。" 8 40
mkfs.fat -F32 "${DISK}1"
mkfs.ext4 "${DISK}2"

# マウント
dialog --msgbox "Mounting partitionsパーティションをマウントします。" 8 40
mount "${DISK}2" /mnt
mkdir -p /mnt/boot/efi
mount "${DISK}1" /mnt/boot/efi

# --- ここからユーザー入力部分 ---

# ユーザー名の設定
USERNAME=$(dialog --inputbox "enter ne user 新しいユーザー名を入力してください" 8 40 3>&1 1>&2 2>&3)
if [ -z "$USERNAME" ]; then
    echo "No usrname enetered so exitng ユーザー名が入力されませんでした。終了します。"
    exit 1
fi

# パスワード設定
PASSWORD=$(dialog --passwordbox "enetr password パスワードを入力してください" 8 40 3>&1 1>&2 2>&3)
if [ -z "$PASSWORD" ]; then
    echo "No password entered so exiting パスワードが入力されませんでした。終了します。"
    exit 1
fi
RE_PASSWORD=$(dialog --passwordbox "repite password パスワードを再入力してください" 8 40 3>&1 1>&2 2>&3)
if [ "$PASSWORD" != "$RE_PASSWORD" ]; then
    dialog --msgbox "passwd not match パスワードが一致しません。終了します。" 8 40
    exit 1
fi

# --- ここからインストール処理 ---
dialog --infobox "installing base sys ベースシステムをインストールしています..." 8 40
pacstrap /mnt base linux linux-firmware

dialog --infobox "generate fstab fstabを生成しています..." 8 40
genfstab -U /mnt >> /mnt/etc/fstab

dialog --infobox "performing install setup 初期設定をしています..." 8 40
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

dialog --msgbox "all done! reboot now?再起動しますか？ \nRebooting" 8 40
reboot