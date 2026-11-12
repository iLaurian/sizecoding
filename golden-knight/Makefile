# golden-knight: everything we compile goes into build/
CC     = gcc
CFLAGS = -Wall -O2
BUILD  = build

$(BUILD)/knight: main.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< -lm

$(BUILD):
	mkdir -p $@

run: $(BUILD)/knight
	./$(BUILD)/knight

clean:
	rm -rf $(BUILD)

.PHONY: run clean
