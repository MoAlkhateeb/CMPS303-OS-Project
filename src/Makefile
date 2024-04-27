build:
	gcc process_generator.c -o process_generator.out
	gcc clk.c -o clk.out
	gcc process.c -o process.out
	gcc test_generator.c -o test_generator.out
	gcc linkedlist.c -c
	gcc process.c -o process.out
	gcc pri_queue.c -c
	gcc scheduler.c linkedlist.o pri_queue.o -o scheduler.out

clean:
	rm -f *.out *.o processes.txt

all: clean build

run:
	./process_generator.out
