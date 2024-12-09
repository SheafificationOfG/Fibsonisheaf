DEFINES=
FLAGS=
OPTLEVEL=-O3
DFLAGS=$(DEFINES:%=-D%)
CFLAGS=-march=native $(OPTLEVEL) -fno-math-errno -Wall -Wextra $(FLAGS) $(DFLAGS)
ASMFLAGS=-fverbose-asm
CC=gcc -I.

IMPL_DIR=impl
OBJ_DIR=obj
BIN_DIR=bin
ASM_DIR=asm
DATA_DIR=data

EVAL=eval.c
HEX=hex.c

.PHONY: init
init:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(BIN_DIR)
	mkdir -p $(ASM_DIR)
	mkdir -p $(DATA_DIR)

.PHONY: clean clean-bin clean-asm clean-data clean-all
clean-all: clean clean-bin clean-asm clean-data
clean:
	rm -f $(OBJ_DIR)/*
clean-bin:
	rm -f $(BIN_DIR)/*
clean-asm:
	rm -f $(ASM_DIR)/*
clean-data:
	rm -f $(DATA_DIR)/*


###############################################################################
## Fibonacci implementations
IMPL = naive\
       linear

.PHONY: $(IMPL:%=run-%) all-data
all-data: $(IMPL:%=$(DATA_DIR)/%.dat)

$(IMPL:%=run-%): run-%: $(BIN_DIR)/%.out
	./$^

$(IMPL:%=$(DATA_DIR)/%.dat): $(DATA_DIR)/%.dat: $(BIN_DIR)/%.out
	./$^ > $@

.PHONY: all all-obj
all: $(IMPL:%=$(BIN_DIR)/%.out)
all-obj: $(IMPL:%=$(OBJ_DIR)/%.o)

$(IMPL:%=$(BIN_DIR)/%.out): $(BIN_DIR)/%.out: $(EVAL) $(OBJ_DIR)/%.o
	$(CC) $(CFLAGS) $^ -o $@

$(IMPL:%=$(BIN_DIR)/%.hex.out): $(BIN_DIR)/%.hex.out: $(HEX) $(OBJ_DIR)/%.o
	$(CC) $(CFLAGS) $^ -o $@

$(IMPL:%=$(OBJ_DIR)/%.o): $(OBJ_DIR)/%.o: $(IMPL_DIR)/%.c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: all-asm
all-asm: $(IMPL:%=$(ASM_DIR)/%.s)

$(IMPL:%=$(ASM_DIR)/%.s): $(ASM_DIR)/%.s: $(IMPL_DIR)/%.c
	$(CC) $(CFLAGS) $(ASMFLAGS) -c -S $^ -o $@

###############################################################################
## Checks

.PHONY: check-endian

check-endian: $(BIN_DIR)/check_endian.out
	@./$^

$(BIN_DIR)/check_endian.out: check_endian.c
	@$(CC) $(CFLAGS) $^ -o $@
