#!/bin/bash
echo ">>> Killing residual RAMfs..."
sudo pkill -f ramfs.py
sleep 1
echo ">>> Attempting lazy unmount..."
sudo umount -l /mnt/ramfs
sleep 1
echo ">>> Recreating mount directory..."
sudo rm -rf /mnt/ramfs
sudo mkdir -p /mnt/ramfs
sudo chown $USER:$USER /mnt/ramfs
