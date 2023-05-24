KDIR = /lib/modules/`uname -r`/build
obj-m        = mod_process_string.o

all: App mod_process_string.ko
	rm -f *.o

mod_process_string.ko: mod_process_string.c
	make -C $(KDIR) M=`pwd` modules

App: App.cpp
	g++ -o $@ App.cpp

clean:
	make -C $(KDIR) M=`pwd` clean
	rm -rf App
