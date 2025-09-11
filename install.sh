#!/bin/bash

# --- Start installation ---
dialog --title "Arch Linux Installer" --msgbox "Welcome to the Arch Linux Installer!\n\nThis script will guide you through the installation process." 8 50

# --- Network connection ---
dialog --title "Network Connection" --msgbox "Attempting to connect to a Wi-Fi network.\n\nMake sure your Wi-Fi device is powered on." 8 50

# Get Wi-Fi device
WIFI_DEV=$(iwctl device list | grep 'station' | awk '{print $1}')
if [ -z "$WIFI_DEV" ]; then
    dialog --title "Error" --msgbox "No Wi-Fi device found.\nPlease try a wired connection." 8 50
    exit 1
fi

# Enable Wi-Fi device
iwctl device "$WIFI_DEV" set-property Powered on

# Scan and select SSID
iwctl station "$WIFI_DEV" scan
SSID_LIST=$(iwctl station "$WIFI_DEV" get-networks | sed '1d' | awk '{print $2}' | sort -u)

SSID=$(dialog --title "SSID Selection" --menu "Select a network to connect to:" 20 60 10 $SSID_LIST 3>&1 1>&2 2>&3)
if [ -z "$SSID" ]; then
    dialog --title "Skipped" --msgbox "Network connection skipped. Please check your connection manually." 8 50
else
    # Enter password and connect
    PASSWORD=$(dialog --title "Password Entry" --passwordbox "Enter the password for '$SSID':" 8 50 3>&1 1>&2 2>&3)
    dialog --infobox "Connecting to '$SSID'..." 6 50
    
    if ! iwctl --passphrase "$PASSWORD" station "$WIFI_DEV" connect "$SSID"; then
        dialog --title "Error" --msgbox "Failed to connect. Please check the password or try again." 8 50
    else
        dhcpcd
        dialog --title "Success" --msgbox "Successfully connected to the network!" 6 50
    fi
fi

# --- Disk Partitioning and Formatting ---
dialog --title "Disk Preparation" --msgbox "Next, we will prepare the disk for installation.\n\nAll data on the selected disk will be erased." 8 50

# Select disk
DISK=$(dialog --menu "Select the installation disk:" 20 60 10 1 "/dev/sda" "Disk A" 2 "/dev/sdb" "Disk B" 2>/dev/tty)
if [ -z "$DISK" ]; then
    dialog --title "Error" --msgbox "No disk selected. Exiting." 8 50
    exit 1
fi

dialog --yesno "Proceed with automatic partitioning on $DISK?\n(UEFI/GPT - 512MiB EFI, rest Root)" 8 60
if [ $? -eq 0 ]; then
    # Automatic partitioning
    sgdisk --zap-all "$DISK"
    sgdisk -n 1:0:+512MiB -t 1:ef00 -c 1:"EFI System" "$DISK"
    sgdisk -n 2:0:0 -t 2:8300 -c 2:"Linux Root" "$DISK"
else
    dialog --msgbox "Please partition the disk manually using cfdisk." 8 50
    cfdisk "$DISK"
fi

# Format partitions
dialog --msgbox "Formatting partitions..." 8 50
mkfs.fat -F32 "${DISK}1"
mkfs.ext4 "${DISK}2"

# Mount partitions
mount "${DISK}2" /mnt
mkdir -p /mnt/boot/efi
mount "${DISK}1" /mnt/boot/efi

# --- User and Hostname Setup ---
dialog --title "User Setup" --msgbox "Now, let's create a user and set a password." 8 50

USERNAME=$(dialog --inputbox "Enter a new username:" 8 50 3>&1 1>&2 2>&3)
if [ -z "$USERNAME" ]; then
    dialog --title "Error" --msgbox "Username cannot be empty. Exiting." 8 50
    exit 1
fi

PASSWORD=$(dialog --passwordbox "Enter password for '$USERNAME':" 8 50 3>&1 1>&2 2>&3)
RE_PASSWORD=$(dialog --passwordbox "Re-enter password:" 8 50 3>&1 1>&2 2>&3)
if [ "$PASSWORD" != "$RE_PASSWORD" ]; then
    dialog --title "Error" --msgbox "Passwords do not match. Exiting." 8 50
    exit 1
fi

HOSTNAME=$(dialog --inputbox "Enter a hostname for this computer:" 8 50 "my-arch-pc" 3>&1 1>&2 2>&3)
if [ -z "$HOSTNAME" ]; then
    dialog --title "Error" --msgbox "Hostname cannot be empty. Exiting." 8 50
    exit 1
fi

# --- Installation Process ---
dialog --infobox "Installing base system and applications...\n\nThis may take a while." 8 50
# NOTE: Add all desired packages to this pacstrap command
pacstrap /mnt base linux linux-firmware dialog rsync dhcpcd networkmanager sudo vim
dialog --infobox "Copying custom files from the live environment..." 8 50
# Copy all files from airootfs to the installed system
rsync -aAXv --exclude={"/dev/*","/proc/*","/sys/*","/tmp/*","/run/*","/mnt/*","/media/*","/lost+found"} / /mnt

dialog --infobox "Generating fstab..." 6 50
genfstab -U /mnt >> /mnt/etc/fstab

# --- Chroot and Post-installation Setup ---
dialog --infobox "Performing final system configurations..." 8 50
arch-chroot /mnt /bin/bash <<EOF
    # Timezone
    ln -sf /usr/share/zoneinfo/Asia/Tokyo /etc/localtime
    hwclock --systohc

    # Locale
    echo "ja_JP.UTF-8 UTF-8" >> /etc/locale.gen
    locale-gen
    echo "LANG=ja_JP.UTF-8" > /etc/locale.conf

    # Hostname
    echo "$HOSTNAME" > /etc/hostname

    # Root password
    echo "root:$(openssl passwd -1 -salt xyz)" | chpasswd

    # User creation and sudo privileges
    useradd -m -G wheel,video,audio,storage $USERNAME
    echo "$USERNAME:$PASSWORD" | chpasswd
    echo "%wheel ALL=(ALL:ALL) ALL" >> /etc/sudoers

    # Enable essential services
    systemctl enable NetworkManager
    systemctl enable dhcpcd

EOF

# --- Finalization ---
dialog --msgbox "Installation complete!\n\nThe system will now reboot." 8 50
reboot