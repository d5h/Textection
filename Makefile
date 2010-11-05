CC = g++
CFLAGS = -W -Wall -g3
COMPILE = $(CC) $(CFLAGS) -c
LINK = $(CC)

all: train

train: train.o objfind.o
	$(LINK) -lcv -lcvaux -lsqlite3 $^ -o $@

clean:
	rm -f *.o train

%.o: %.cc
	$(COMPILE) -o $@ $<
