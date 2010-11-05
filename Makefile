CC = g++
CFLAGS = -W -Wall -g3
COMPILE = $(CC) $(CFLAGS) -c
LINK = $(CC)

all: train

train: image.o objfind.o sql.o train.o
	$(LINK) -lcv -lcvaux -lsqlite3 $^ -o $@

clean:
	rm -f *.o train

%.o: %.cc
	$(COMPILE) -o $@ $<
