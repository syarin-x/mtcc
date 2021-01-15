CFLAGS=-std=c11 -g -static

mtcc: mtcc.c

test: test
	./test/test.sh

clean:
	rm -f ./bin/*.o ./bin/*~ ./bin/tmp* ./mtcc

.PHONY: test clean