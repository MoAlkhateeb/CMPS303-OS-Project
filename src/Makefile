build:
	gcc process_generator.c -o process_generator.out
	gcc clk.c -o clk.out
	gcc process.c -o process.out
	gcc test_generator.c -o test_generator.out
	gcc process_table.c -c
	gcc process.c -o process.out
	gcc pri_queue.c -c
	gcc buddy.c -c
	gcc scheduler.c process_table.o pri_queue.o buddy.o -o scheduler.out -lm

clean:
	rm -f *.out *.o processes.txt

all: clean build

run:
	./process_generator.out
