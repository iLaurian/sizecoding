# golden-knight: everything we compile goes into build/
CC     = gcc
CFLAGS = -Wall -O2
BUILD  = build

$(BUILD)/knight: main.c sound.c song.h | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< -lm

$(BUILD):
	mkdir -p $@

$(BUILD)/sound: sound.c song.h | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< -lm

play: $(BUILD)/sound
	./$(BUILD)/sound | pacat --raw --format=s16le --rate=44100 --channels=1

run: $(BUILD)/knight
	./$(BUILD)/knight

clean:
	rm -rf $(BUILD)

.PHONY: play run clean
