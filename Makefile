export DEVKITPRO		?=	/opt/devkitpro/
export EMULATOR 		?= 	mgba-qt
export PATH					:=	$(DEVKITPRO)/devkitARM/bin/:$(DEVKITPRO)/tools/bin/:$(PATH)

CFILES		=		$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CC				=		arm-none-eabi-gcc
CFLAGS		=		-O2 -Iinclude/ -I$(DEVKITPRO)/libtonc/include/
LDFLAGS		=		-specs=gba.specs -L$(DEVKITPRO)/libtonc/lib -ltonc

# CFLAGS		=		-mthumb-interwork -mthumb -O2 -Iinclude/ -I$(DEVKITPRO)/libtonc/include/
# LDFLAGS		=		-mthumb-interwork -mthumb -specs=gba.specs -L$(DEVKITPRO)/libtonc/lib -ltonc


EXEC			=		main
SRC				=		$(wildcard src/*.c)
OBJ				=		$(SRC:.c=.o)

all: $(EXEC)

main: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)
	arm-none-eabi-objcopy -v -O binary $(EXEC) $(EXEC).gba
	gbafix $(EXEC).gba
	rm $(EXEC)

%.o: %.c
	@$(CC) -o $@ -c $< $(CFLAGS)

run: main
	$(EMULATOR) $(EXEC).gba

clean:
	@rm -rf $(OBJ)
	@rm -rf $(EXEC).gba
