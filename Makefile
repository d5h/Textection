CC = g++
CFLAGS = -W -Wall -g3
COMPILE = $(CC) $(CFLAGS) -c
LINK = $(CC)

all: detect features train

detect: args.o classifier.o detect.o image.o objfind.o posfeatures.o
	$(LINK) -lcv -lcvaux $^ -o $@

features: features.o image.o objfind.o posfeatures.o sql.o
	$(LINK) -lcv -lcvaux -lsqlite3 $^ -o $@

train: args.o image.o objfind.o sql.o train.o
	$(LINK) -lcv -lcvaux -lsqlite3 $^ -o $@

classifier.h classifier.cc:
	./build_classifier

clean:
	rm -f *.o train

%.o: %.cc
	$(COMPILE) -o $@ $<
