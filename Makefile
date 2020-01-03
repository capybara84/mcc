CFLAGS=-Wall

mcc : main.o gen.o node.o parser.o scanner.o symbol.o misc.o
	$(CC) $(CFLAGS) -o $@ $^

test: scanner_test parser_test

test_scanner : test_scanner.o scanner.o symbol.o misc.o
	$(CC) $(CFLAGS) -o $@ $^

scanner_test : test_scanner
	-./test_scanner

test_parser : test_parser.o gen.o node.o parser.o scanner.o symbol.o misc.o
	$(CC) $(CFLAGS) -o $@ $^

parser_test : test_parser
	-./test_parser

clean:
	rm -f mcc *.o

main.o : mcc.h
gen.o : mcc.h
node.o : mcc.h
parser.o : mcc.h
scanner.o : mcc.h
symbol.o : mcc.h
misc.o : mcc.h

test_scanner.o : mcc.h
test_parser.o : mcc.h
