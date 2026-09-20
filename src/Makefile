# ==============================================================
# Makefile for z88dk + Cygwin
# Target: MZ-700/1500
# Output: ROGUE_OBJ.mzt, ROGUE_MESG.mzt, ROGUE.mzt
# ==============================================================

CC      = zcc
PYTHON ?= /usr/bin/python3
CFLAGS  = +mz -compiler=sdcc -c -SO3 --opt-code-size --max-allocs-per-node500000 --peep-return --callee-saves-bc
LDFLAGS = +mz -compiler=sdcc -create-app -m -s -startup=2 -pragma-define:STACKPTR=0xc000 \
	-pragma-define:CRT_ENABLE_STDIO=0 \
	-pragma-define:CLIB_EXIT_STACK_SIZE=0

ifeq ($(DEBUG),1)
CFLAGS += -DDEBUG
endif

SOURCE = main.c init.c play.c level.c room.c move.c hit.c display.c message.c invent.c use.c trap.c score.c \
	random.c object.c pack.c monster.c spechit.c throw.c zap.c ring.c machdep.c \
	mz_display.c mz_curses.c mz_system.c mz_io.c
OBJ    = $(SOURCE:.c=.o)
PROGRAM = ROGUE
PROGRAM_MZT = ROGUE_OBJ.mzt
MESSAGE_MZT = ROGUE_MESG.mzt
PACKAGE = ROGUE.mzt
QD_IMAGE_MAX = 65536
MESSAGE_SOURCE = mz_mesg
MONSTER_SOURCE = mz_monster
MESSAGE_TOOL   = tools/mkmessage.py
ZX0_TOOL = ZX0-main/src/zx0.exe

INCLUDE = mz_common.h rogue.h main.h init.h level.h room.h move.h hit.h display.h message.h invent.h use.h trap.h score.h \
	random.h object.h pack.h monster.h spechit.h throw.h zap.h ring.h \
	play.h machdep.h mz_curses.h mz_display.h mz_system.h mz_io.h

.PHONY: all messages clean

all: $(PACKAGE)

messages: $(MESSAGE_MZT)

$(MESSAGE_MZT): $(MESSAGE_SOURCE) $(MONSTER_SOURCE) $(MESSAGE_TOOL) mz_display.h $(ZX0_TOOL)
	$(PYTHON) $(MESSAGE_TOOL) --display mz_display.h --input $(MESSAGE_SOURCE) \
		--monster $(MONSTER_SOURCE) --zx0 $(ZX0_TOOL) --output $@ \
		--mzt-name MESG --load-address 0xc000 --exec-address 0xc000 \
		--max-compressed-size 0x1000 \
		--exclude-ids 98,102,162,211,232,246,263,279,282 \
		--data-address 0xd000 --monster-address 0xe800 \
		--level-points-address 0xeda0 \
		--id-wands-address 0xee00 --id-rings-address 0xee50 \
		--wand-materials-address 0xeea8 --gems-address 0xeee4 \
		--data-limit-address 0xef00 \
	--ids 2,3,4,5,8,10,11,12,18,19,21,22,23,24,26,27,28,29,30,31,32,33,35,36,48,49,50,51,53,55,63,64,65,66,67,68,69,71,72,73,74,75,76,77,78,79,85,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,104,105,106,107,158,159,160,161,162,163,164,165,166,167,168,169,170,171,176,182,183,184,185,200,201,202,203,204,205,206,207,208,209,210,211,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,256,257,259,260,262,263,264,266,267,268,269,272,273,274,277,278,279,280,281,282,307,308,309,310,311,312,313,314,315,316,317,318,319,320,321,322,323,324,325,326,327,328,329,330,331,332,333,334,335,336,337,338,339,340,341,342,343,344,345,346,347,348,349,350,351,352,353,354,355,356,357,358,359,360,361,362,363,364,365,366,367,368,369,370,371,372,373,374,375,376,377,378,379,380,381,382,383,384,385,386,387,388,389,390,391,392,393,394,395,396,397,398,399,400,401,402,403,404,405,406,407,408,409,410,411,412,413,414,415,416,417,418,419,420,421,422,423,424,425,426,427,428,429,430,431,432,433,434,435,436,437,438,439,440,441,442,443,444,445,446,447,448,449,450,451,452,453,454,455,456,457,458,459,460,461,462,463,464,465,466,467,468,469,470,471,472,473,474,475,476,477,478,479,480,481,482,483,484,485,486,487,488,489,490,491,492,493,500,501,502,503,504,505,506,507,508,509,510,511,512,513,517,518,520,521,522,523,524,525,526,527,528

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
