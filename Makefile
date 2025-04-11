CC=clang-18 # or: gcc-14
PY=python3.13

DEFINES=
FLAGS=
OPTLEVEL=-O3
DFLAGS=$(DEFINES:%=-D%)
CFLAGS=-march=native -fno-math-errno
WFLAGS=-Wall -Wextra -Wpedantic
ASMFLAGS=-fverbose-asm
IFLAGS=-I.
OBJFLAGS=-std=c23

BUILD_CMD=$(CC) $(IFLAGS) $(CFLAGS) $(FLAGS) $(DFLAGS) $(OPTLEVEL) $(WFLAGS)

IMPL_DIR=impl
OBJ_DIR=obj
BIN_DIR=bin
ASM_DIR=asm
DATA_DIR=data
HEADER_DIR=autoheader
AUTOHEADER=scripts.autoheader
AUTOHEADER_FLAGS=

EVAL=eval.c
HEX=hex.c

.PHONY: init
init:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(BIN_DIR)
	mkdir -p $(ASM_DIR)
	mkdir -p $(DATA_DIR)

.PHONY: clean clean-bin clean-asm clean-data clean-header clean-all
clean-all: clean clean-bin clean-asm clean-data clean-header
clean: clean-header
	rm -f $(OBJ_DIR)/*
clean-bin:
	rm -f $(BIN_DIR)/*
clean-asm:
	rm -f $(ASM_DIR)/*
clean-data:
	rm -f $(DATA_DIR)/*
clean-header:
	rm -f $(HEADER_DIR)/*


###############################################################################
## Fibonacci implementations
GMPL = gmp\
       gmp_explicit

IMPL = naive\
       linear\
       fastexp\
       fastexp2d\
       fastsquaring\
       ntt\
       nttt\
       $(GMPL)


.PHONY: $(IMPL:%=run-%) all-data
all-data: $(IMPL:%=$(DATA_DIR)/%.dat)

$(IMPL:%=run-%): run-%: $(BIN_DIR)/%.out
	./$^

$(IMPL:%=$(DATA_DIR)/%.dat): $(DATA_DIR)/%.dat: $(BIN_DIR)/%.out
	./$^ > $@

.PHONY: all all-hex all-obj
all: $(IMPL:%=$(BIN_DIR)/%.out)
all-hex: $(IMPL:%=$(BIN_DIR)/%.hex.out)
all-obj: $(IMPL:%=$(OBJ_DIR)/%.o)
all-header: $(IMPL:%=$(HEADER_DIR)/%.h)

$(patsubst %,$(BIN_DIR)/%.out,$(filter-out $(GMPL),$(IMPL))): $(BIN_DIR)/%.out: $(EVAL) $(OBJ_DIR)/%.o
	$(BUILD_CMD) $^ -o $@

$(patsubst %,$(BIN_DIR)/%.hex.out,$(filter-out $(GMPL),$(IMPL))): $(BIN_DIR)/%.hex.out: $(HEX) $(OBJ_DIR)/%.o
	$(BUILD_CMD) $^ -o $@

$(GMPL:%=$(BIN_DIR)/%.out): $(BIN_DIR)/%.out: $(EVAL) $(OBJ_DIR)/%.o
	$(BUILD_CMD) $^ -o $@ -lgmp

$(GMPL:%=$(BIN_DIR)/%.hex.out): $(BIN_DIR)/%.hex.out: $(HEX) $(OBJ_DIR)/%.o
	$(BUILD_CMD) $^ -o $@ -lgmp

$(IMPL:%=$(OBJ_DIR)/%.o): $(OBJ_DIR)/%.o: $(IMPL_DIR)/%.c $(HEADER_DIR)/%.h
	$(BUILD_CMD) $(OBJFLAGS) -DAUTOHEADER="\"$(word 2,$^)\"" -c $< -o $@

$(IMPL:%=$(HEADER_DIR)/%.h): %.h:
	$(PY) -m $(AUTOHEADER) --folder=$(HEADER_DIR) $(@:$(HEADER_DIR)/%.h=%) $(AUTOHEADER_FLAGS) $(DFLAGS)

.PHONY: all-asm
all-asm: $(IMPL:%=$(ASM_DIR)/%.s)

$(IMPL:%=$(ASM_DIR)/%.s): $(ASM_DIR)/%.s: $(IMPL_DIR)/%.c
	$(BUILD_CMD) $(OBJFLAGS) $(ASMFLAGS) -c -S $^ -o $@
