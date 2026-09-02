# ==============================================================
# Makefile for z88dk + Cygwin
# Target: MZ-1500
# Output: ROGUE_OBJ.mzt, ROGUE_MESG.mzt, ROGUE.mzt
# ==============================================================

CC      = zcc
PYTHON ?= /usr/bin/python3
CFLAGS  = +mz -c -O3
LDFLAGS = +mz -create-app -m -s -startup=2 -pragma-define:STACKPTR=0xc000

ifdef DEBUG
CFLAGS += -DDEBUG
endif

SOURCE = main.c init.c play.c level.c room.c move.c hit.c display.c message.c invent.c use.c trap.c score.c \
	random.c object.c pack.c monster.c spechit.c throw.c zap.c ring.c \
	mz_display.c mz_curses.c mz_system.c mz_io.c
OBJ    = $(SOURCE:.c=.o)
PROGRAM = ROGUE
PROGRAM_MZT = ROGUE_OBJ.mzt
MESSAGE_MZT = ROGUE_MESG.mzt
PACKAGE = ROGUE.mzt
QD_IMAGE_MAX = 65536
MESSAGE_SOURCE = mz_mesg
MESSAGE_TOOL   = tools/mkmessage.py

INCLUDE = mz_common.h rogue.h main.h init.h level.h room.h move.h hit.h display.h message.h invent.h use.h trap.h score.h \
	random.h object.h pack.h monster.h spechit.h throw.h zap.h ring.h \
	play.h mz_curses.h mz_display.h mz_system.h mz_io.h

.PHONY: all messages clean

all: $(PACKAGE)

messages: $(MESSAGE_MZT)

$(MESSAGE_MZT): $(MESSAGE_SOURCE) $(MESSAGE_TOOL) mz_display.h
	$(PYTHON) $(MESSAGE_TOOL) --display mz_display.h --input $(MESSAGE_SOURCE) --output $@ --mzt-name MESG --load-address 0xd000 --exec-address 0xd000 --ids 2,4,10,12,18,19,21,22,23,24,26,27,28,29,30,31,32,35,36,49,50,51,53,55,69,71,72,73,74,75,76,77,85,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,106,107,158,159,160,161,162,163,164,165,166,167,170,176,180,182,183,184,185,207,210,211,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,231,232,233,234,235,236,237,238,245,246,247,248,249,250,251,252,253,254,255,256,257,259,262,263,264,266,267,268,269,278,279,280,281,282,308,309,310,311,314,317,321,323,324,325,326,327,332,333,334,335,336,337,338,374,375,376,377,378,379,380,381,382,383,384,385,386,387,388,389,395,396,397,398,403,410,411,412,413,414,415,416,417,418,419,420,421,422,423,424,428

$(PACKAGE): $(PROGRAM_MZT) $(MESSAGE_MZT)
	@size=$$(($$(wc -c < $(PROGRAM_MZT)) + $$(wc -c < $(MESSAGE_MZT)))); \
	if [ $$size -gt $(QD_IMAGE_MAX) ]; then \
		echo "error: combined QuickDisk image is $$size bytes (maximum $(QD_IMAGE_MAX))"; \
		exit 1; \
	fi
	cat $(PROGRAM_MZT) $(MESSAGE_MZT) > $@

$(PROGRAM_MZT): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $(PROGRAM)
	mv $(PROGRAM).mzt $@

%.o: %.c $(INCLUDE)
	$(CC) $(CFLAGS) $<

clean:
	$(RM) $(OBJ) $(PROGRAM) $(PROGRAM).bin $(PROGRAM_MZT) $(PROGRAM).map $(PROGRAM).sym \
		$(MESSAGE_MZT) $(PACKAGE) MESG.mzt
