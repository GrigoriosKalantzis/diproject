OBJS1	= main.o
OBJS2	= structs.o
OBJS3	= harness.o
OUT1	= project
OUT2	= harness
CC1	= gcc
CC2 = g++
FLAGS	= -g -c -Wall

all: $(OBJS1) $(OBJS2) $(OBJS3)
	$(CC1) -Wall $(OBJS1) $(OBJS2) -o $(OUT1) -lm
	$(CC2) -Wall $(OBJS3) -o $(OUT2)
structs.o: structs.c
	$(CC1) $(FLAGS) structs.c
main.o: main.c
	$(CC1) $(FLAGS) main.c
harness.o: harness.cpp
	$(CC2) $(FLAGS) -std=gnu++11  harness.cpp

clean:
	rm -f $(OBJS1) $(OBJS2) $(OBJS3) $(OUT1) $(OUT2)

