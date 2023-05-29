all: lang	findlang locatelang generatemodels

lang:	./src/lang.o
	g++ -o ./bin/lang $^ -lm -g
	rm $^

findlang:	./src/findlang.o
	g++ -o ./bin/findlang $^ -lm -g
	rm $^

locatelang:	./src/locatelang.o
	g++ -o ./bin/locatelang $^ -lm -g
	rm $^

generatemodels:	./src/generatemodels.o
	g++ -o ./bin/generatemodels $^ -lm -g
	rm $^