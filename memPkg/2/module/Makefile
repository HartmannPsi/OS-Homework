obj-m := persist_mod.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules
	gcc -o persist_user persist_user.c

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f persist_user
