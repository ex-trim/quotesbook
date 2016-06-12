TARGET = quotesbook
PREFIX = /usr/local/bin

.PHONY: all clean install uninstall

all: $(TARGET)

clean:
	rm -rf $(TARGET) *.o

quotesbook.o: quotesbook.c
	gcc -c -o quotesbook.o quotesbook.c -lsqlite3

$(TARGET):	quotesbook.o
	gcc -Og -o $(TARGET) quotesbook.o -lsqlite3

install:
	install $(TARGET) $(PREFIX)

uninstall:
	rm -rf $(PREFIX)/$(TARGET)
