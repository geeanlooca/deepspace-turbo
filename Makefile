CC = gcc
CFLAGS = -g -std=gnu99
OBJDIR = ./build
BINDIR = ./bin
LIBS = -lm -fopenmp

default: convolutionalBER

# Build binaries
convolutionalBER: $(OBJDIR)/main.o $(OBJDIR)/utilities.o $(OBJDIR)/libconvcodes.o $(OBJDIR)/libturbocodes.o
	$(CC) $(CFLAGS) -o $(BINDIR)/convolutionalBER $(OBJDIR)/main.o $(OBJDIR)/utilities.o $(OBJDIR)/libconvcodes.o  $(OBJDIR)/libturbocodes.o $(LIBS)

# Build objects
$(OBJDIR)/main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o $(OBJDIR)/main.o $(LIBS)

$(OBJDIR)/libconvcodes.o: libconvcodes.c libconvcodes.h
	$(CC) $(CFLAGS) -c libconvcodes.c -o $(OBJDIR)/libconvcodes.o $(LIBS)

$(OBJDIR)/utilities.o: utilities.c utilities.h
	$(CC) $(CFLAGS) -c utilities.c -o $(OBJDIR)/utilities.o $(LIBS)

$(OBJDIR)/libturbocodes.o: libturbocodes.c libturbocodes.h
	$(CC) $(CFLAGS) -c libturbocodes.c -o $(OBJDIR)/libturbocodes.o $(LIBS)

clean: 
	rm -f $(OBJDIR)/*
	rm -f $(BINDIR)/*
