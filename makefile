.SUFFIXES:
CC     = gcc
LEX    = flex
YACC   = bison

CFLAGS = -std=c99 -Wall -DLEXDEBUG -g -pedantic
YFLAGS = -d -v
LFLAGS = -t

YFILES = minako-syntax.y
LFILES = minako-lexic.l
CFILES = symtab.c stack.c syntree.c dict.c minako.c
HFILES = symtab.h stack.h syntree.h dict.h

SOURCE = $(YFILES) $(LFILES) $(HFILES) $(CFILES)
TARGET = $(YFILES:%.y=%.tab.o) $(LFILES:%.l=%.o) $(CFILES:%.c=%.o)

# Compiling
%.tab.c %.tab.h: %.y
	$(YACC) $(YFLAGS) $<
%.c: %.l
	$(LEX) $(LFLAGS) $< > $@
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Targets
all: minako

minako: $(TARGET)
	$(CC) $(CFLAGS) $^ -o $@

run: minako
	./minako simple.c1

clean:
	$(RM) $(RMFILES) minako core *.o *.tab.* *.output
