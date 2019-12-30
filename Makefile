CFLAGS=-Wall

mcc : main.o gen.o parser.o scanner.o symbol.o misc.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f mcc *.o

main.o : mcc.h
gen.o : mcc.h
parser.o : mcc.h
scanner.o : mcc.h
symbol.o : mcc.h
misc.o : mcc.h
