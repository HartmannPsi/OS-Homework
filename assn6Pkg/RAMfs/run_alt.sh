mkdir /mnt/ramfs
sudo chown $(whoami) /mnt/ramfs
sudo chmod 777 /mnt/ramfs
python3 launcher.py /mnt/ramfs/ ramfs_persist/ -o allow_other