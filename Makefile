CFLAGS=-Wall -g

mcc : main.o gen.o node.o parser.o scanner.o symbol.o misc.o
	$(CC) $(CFLAGS) -o $@ $^

test: scanner_test parser_test

test_scanner : test_scanner.o gen.o scanner.o node.o symbol.o misc.o
	$(CC) $(CFLAGS) -o $@ $^

scanner_test : test_scanner
	-./test_scanner test_scanner1.c > test_scanner1.output
	-diff test_scanner1.result test_scanner1.output

test_parser : test_parser.o gen.o node.o parser.o scanner.o symbol.o misc.o
	$(CC) $(CFLAGS) -o $@ $^

parser_test : test_parser
	-./test_parser test_parser1.c > test_parser1.output
	-diff test_parser1.result test_parser1.output
	-./test_parser test_parser2.c > test_parser2.output
	-diff test_parser2.result test_parser2.output
	-./test_parser test_parser3.c > test_parser3.output
	-diff test_parser3.result test_parser3.output
	-./test_parser test_parser4.c > test_parser4.output
	-diff test_parser4.result test_parser4.output

clean:
	rm -f mcc *.o test_scanner test_parser *.output

main.o : mcc.h
gen.o : mcc.h
node.o : mcc.h
parser.o : mcc.h
scanner.o : mcc.h
symbol.o : mcc.h
misc.o : mcc.h

test_scanner.o : mcc.h
test_parser.o : mcc.h
