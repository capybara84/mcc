CFLAGS=-Wall

mcc : main.o symbol.o misc.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f mcc *.o

main.o : mcc.h
symbol.o : mcc.h
misc.o : mcc.h
