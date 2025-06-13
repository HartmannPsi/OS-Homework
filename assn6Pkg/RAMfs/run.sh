mkdir /mnt/ramfs
sudo chown $(whoami) /mnt/ramfs
sudo chmod 777 /mnt/ramfs
sudo PYTHONPATH=$(python -c "import site; print(site.getsitepackages()[0])") python3 launcher.py /mnt/ramfs