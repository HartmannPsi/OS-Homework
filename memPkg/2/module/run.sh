gcc -o test1 test1.c persist_user.c
gcc -o test2 test2.c persist_user.c -lpthread
gcc -o test3 test3.c persist_user.c

sudo ./test1
sudo ./test2
sudo ./test3
