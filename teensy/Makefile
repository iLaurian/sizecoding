# teensy: everything we build goes into build/
BUILD = build

all: $(BUILD)/tiny $(BUILD)/tiny-c

# the hand-written 64-byte ELF
$(BUILD)/tiny: tiny.asm | $(BUILD)
	nasm -f bin -o $@ $<
	chmod +x $@

# the plain C version, for comparison
$(BUILD)/tiny-c: tiny.c | $(BUILD)
	gcc -Wall -Os -s -o $@ $<

$(BUILD):
	mkdir -p $@

run: $(BUILD)/tiny
	./$(BUILD)/tiny; echo "exit=$$?"

size: all
	wc -c $(BUILD)/tiny $(BUILD)/tiny-c

clean:
	rm -rf $(BUILD)

.PHONY: all run size clean
