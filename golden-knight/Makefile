# golden-knight: everything we compile goes into build/
CC     = gcc
CFLAGS = -Wall -m32 -Os -ffast-math -mfpmath=387 -fno-pic -fno-pie -no-pie -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-stack-protector -fno-ident -ffunction-sections -fdata-sections -fomit-frame-pointer
LDFLAGS = -Wl,--gc-sections -nostartfiles
BUILD  = build

$(BUILD)/knight: main.c sound.c song.h | $(BUILD)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -lm

$(BUILD):
	mkdir -p $@

$(BUILD)/sound: sound.c song.h | $(BUILD)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -lm

play: $(BUILD)/sound
	./$(BUILD)/sound | pacat --raw --format=s16le --rate=44100 --channels=1

run: $(BUILD)/knight
	./$(BUILD)/knight

size: $(BUILD)/knight
	strip -s -R .comment -o $(BUILD)/knight.s $(BUILD)/knight
	@echo "stripped:   $$(stat -c %s $(BUILD)/knight.s) bytes"
	@echo "compressed: $$(xz --format=lzma -9e -c $(BUILD)/knight.s | wc -c) bytes"
	@size $(BUILD)/knight | tail -1 | awk '{print "code:       " $$1 " bytes"}'

clean:
	rm -rf $(BUILD)

.PHONY: play run clean
