CC = g++
CFLAGS = -W -Wall -g3
COMPILE = $(CC) $(CFLAGS) -c
LINK = $(CC)

PROGS = detect features train

all: $(PROGS)

detect: args.o classifier.o detect.o image.o objfind.o posfeatures.o
	$(LINK) -lcv -lcvaux $^ -o $@

features: features.o image.o objfind.o posfeatures.o sql.o
	$(LINK) -lcv -lcvaux -lsqlite3 $^ -o $@

train: args.o image.o objfind.o sql.o train.o
	$(LINK) -lcv -lcvaux -lsqlite3 $^ -o $@

classifier.h classifier.cc: features
	./build_classifier

clean:
	rm -f *.o $(PROGS) classifier.cc classifier.h

%.o: %.cc
	$(COMPILE) -o $@ $<
