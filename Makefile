# Quality: Lower is better. 0 is lossless as far as the format allows.
QUALITY ?= 4

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(SOURCE),)
$(error Missing video source. Use "make SOURCE=...")
endif
endif

OUT := output/$(basename $(notdir $(SOURCE)))
$(shell mkdir -p $(OUT))
CC ?= clang
FFMPEG := ffmpeg -loglevel warning -stats -hide_banner

TITLE = "\033[1m\033[36m"
TITLE_END = "\033[0m"

$(OUT)/$(basename $(notdir $(SOURCE))).gbc: output/video.gbc $(OUT)/video.bin
	@echo $(TITLE)Creating ROM...$(TITLE_END)
	cat $^ > $@
	rgbfix -Cv -t "GBVP2" -m 25 $@
	cp output/video.sym $(basename $@).sym
	
output/video.gbc: video.asm
	@echo $(TITLE)Compiling player...$(TITLE_END)
	rgbasm -o $@.o $<
	rgblink -o $@ -m output/video.map -n output/video.sym $@.o

$(OUT)/video.bin: output/encoder $(OUT)/frames $(OUT)/audio.raw
	@echo $(TITLE)Encoding video...$(TITLE_END)
	find $(OUT)/frames | sort | \
	output/encoder $(shell ffmpeg -i $(SOURCE) 2>&1 | sed -n "s/.*, \(.*\) fp.*/\1/p") \
	               $(QUALITY) \
	               $(OUT)/audio.raw \
	               $@ \

output/encoder: encoder.c
	@echo $(TITLE)Compiling encoder...$(TITLE_END)
	$(CC) -g -Ofast -std=c11 -Werror -Wall -o $@ $^

$(OUT)/audio.raw: $(SOURCE)
	@echo $(TITLE)Converting audio...$(TITLE_END)
	$(eval GAIN := 0$(shell ffmpeg -i $^ -filter:a volumedetect -f null /dev/null 2>&1 | sed -n "s/.*max_volume: -\(.*\) dB/\1/p"))
	$(FFMPEG) -i $^ -f u8 -acodec pcm_u8 -ar 9198 -filter:a "volume=$(GAIN)dB" $@ 

$(OUT)/frames: $(OUT)/video.mp4
	@echo $(TITLE)Extracting frames...$(TITLE_END)
	-@rm -rf $@
	mkdir -p $@
	$(FFMPEG) -i $^ -coder "raw" $@/%05d.tga

$(OUT)/video.mp4: $(SOURCE)
	@echo $(TITLE)Resizing video...$(TITLE_END)
	$(FFMPEG) -i $^ -c:v rawvideo  -vf scale=-2:144 $@.tmp.mp4
	$(FFMPEG) -i $@.tmp.mp4 -c:v rawvideo  -filter:v "crop=160:144" $@
	rm $@.tmp.mp4
	
clean:
	rm -rf output