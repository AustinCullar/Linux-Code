TOP = ..
SOURCES = $(wildcard *.c)
TARGET = agent
SILENT=@

CC = gcc
CFLAGS=-I$(TOP)/include -lkmod

all:
	$(SILENT)$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

clean:
	$(SILENT)rm -f $(TARGET)

.PHONY: all clean
