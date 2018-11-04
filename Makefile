OBJS1	= main.o
OBJS2	= structs.o
OUT		= project
CC	= gcc
FLAGS	= -g -c -Wall

all: $(OBJS1) $(OBJS2)
	$(CC) $(OBJS1) $(OBJS2) -o $(OUT) -lm
structs.o: structs.c
	$(CC) $(FLAGS) structs.c
main.o: main.c
	$(CC) $(FLAGS) main.c

clean:
	rm -f $(OBJS1) $(OBJS2) $(OUT) output.txt

