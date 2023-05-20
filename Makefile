CC = g++
CFLAGS = -Wall -g

OBJS =
cpm_OBJS = $(OBJS)
cpm_gen_OBJS = $(OBJS)


all: cpm	cpm_gen

lang:	lang.o $(lang_OBJS)
	$(CC) -o ./bin/lang $^ -lm -g

findlang:	findlang.o $(findlang_OBJS)
	$(CC) -o ./bin/findlang $^ -lm -g

locatelang:	locatelang.o $(locatelang_OBJS)
	$(CC) -o ./bin/locatelang $^ -lm -g

clean:
	rm -rf *.o