build:
	gcc process_generator.c -o process_generator.out
	gcc clk.c -o clk.out
	gcc scheduler.c -o scheduler.out
	gcc process.c -o process.out
	gcc test_generator.c -o test_generator.out
	gcc process_control_block.c -o process_control_block.out
	gcc linkedlist.c -o linkedlist.out
	gcc pri_queue.c -o pri_queue.out
  
clean:
	rm -f *.out  processes.txt

all: clean build

run:
	./process_generator.out
