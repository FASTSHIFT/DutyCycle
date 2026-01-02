#!/bin/bash
set -e

# Check for root privileges
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root (sudo ./install.sh)"
  exit 1
fi

echo "Building and installing CH341 driver..."

cd driver
make clean
make

echo "Installing driver..."
make install

# Try to update initramfs if the command exists
if command -v update-initramfs >/dev/null 2>&1; then
    echo "Updating initramfs..."
    update-initramfs -u
elif command -v dracut >/dev/null 2>&1; then
    echo "Updating initramfs (dracut)..."
    dracut --force
fi

echo "Installation complete. The driver should now persist across reboots."
