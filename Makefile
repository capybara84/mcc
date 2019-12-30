CFLAGS=-Wall

mcc : main.o gen.o parser.o scanner.o symbol.o misc.o
	$(CC) $(CFLAGS) -o $@ $^

test: scanner_test

test_scanner : test_scanner.o scanner.o symbol.o misc.o
	$(CC) $(CFLAGS) -o $@ $^

scanner_test : test_scanner
	./test_scanner

clean:
	rm -f mcc *.o

main.o : mcc.h
gen.o : mcc.h
parser.o : mcc.h
scanner.o : mcc.h
symbol.o : mcc.h
misc.o : mcc.h
