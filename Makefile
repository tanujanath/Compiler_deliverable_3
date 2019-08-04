BISON = bison -t -d --verbose
FLEX = flex -d
GCC = gcc

TARGET = uc
GEN_SOURCES = parser.tab.c lex.yy.c
GEN_HEADERS = parser.tab.h

RMFILES = $(TARGET).exe $(GEN_SOURCES) $(GEN_HEADERS) parser.output

$(TARGET): parser.tab.c lex.yy.c parser.tab.h
	$(GCC) -o $(TARGET) $(GEN_SOURCES) library.c

parser.tab.c: parser.y
	$(BISON) $<

lex.yy.c: scanner.l
	$(FLEX) $<

clean:
	del $(RMFILES)