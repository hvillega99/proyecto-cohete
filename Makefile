.PHONY:
all: control monitor

control: control.c
	gcc -o control control.c -pthread

monitor: monitor.c
	gcc -o monitor monitor.c 

clean:
	rm -rf control monitor *.o