#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <string.h> 

#define SCREEN_SIZE (160 * 144)
#define PIXELS (SCREEN_SIZE * 2)
#define SCY_DATA_SIZE (PIXELS / 8)
#define PALETTE_SIZE (8 * 4)
#define DIFF_THRESHOLD 0x8000 // Todo: make this an argument?

unsigned compressed_size = 0;
unsigned uncompressed_size = 0;
static const double GB_FPS = (1024 * 1024 * 4) / (70224.0);

typedef struct {
    uint8_t b, g, r;
} color_t;
#define COLOR_INDEX(PAL, IND) ((PAL) * 4 + (IND))

#define LEFT_COMBINATIONS(LEFT, FIRST_RIGHT) \
LEFT, LEFT, LEFT, FIRST_RIGHT + 0, FIRST_RIGHT + 0 , FIRST_RIGHT + 0 , FIRST_RIGHT + 0 , FIRST_RIGHT + 0 , \
LEFT, LEFT, LEFT, FIRST_RIGHT + 1, FIRST_RIGHT + 1 , FIRST_RIGHT + 1 , FIRST_RIGHT + 1 , FIRST_RIGHT + 1 , \
LEFT, LEFT, LEFT, FIRST_RIGHT + 2, FIRST_RIGHT + 2 , FIRST_RIGHT + 2 , FIRST_RIGHT + 2 , FIRST_RIGHT + 2 , \
LEFT, LEFT, LEFT, FIRST_RIGHT + 3, FIRST_RIGHT + 3 , FIRST_RIGHT + 3 , FIRST_RIGHT + 3 , FIRST_RIGHT + 3 , \

#define LEFT_COMBINATIONS_FOR_PALETTE(PAL) \
LEFT_COMBINATIONS(COLOR_INDEX(PAL, 0), COLOR_INDEX(PAL, 0)) \
LEFT_COMBINATIONS(COLOR_INDEX(PAL, 1), COLOR_INDEX(PAL, 0)) \
LEFT_COMBINATIONS(COLOR_INDEX(PAL, 2), COLOR_INDEX(PAL, 0)) \
LEFT_COMBINATIONS(COLOR_INDEX(PAL, 3), COLOR_INDEX(PAL, 0)) \

#define RIGHT_COMBINATION(LEFT, RIGHT) \
LEFT, LEFT, \
(LEFT) == (RIGHT) ? (LEFT) ^ 1 : LEFT, (LEFT) == (RIGHT) ? (LEFT) ^ 1 : LEFT, \
(LEFT) == (RIGHT) ? (LEFT) ^ 2 : LEFT, (LEFT) == (RIGHT) ? (LEFT) ^ 2 : RIGHT, \
(LEFT) == (RIGHT) ? (LEFT) ^ 3 : RIGHT , (LEFT) == (RIGHT) ? (LEFT) ^ 3 : RIGHT, \

#define RIGHT_COMBINATIONS(LEFT, FIRST_RIGHT) \
RIGHT_COMBINATION(LEFT, FIRST_RIGHT + 0) \
RIGHT_COMBINATION(LEFT, FIRST_RIGHT + 1) \
RIGHT_COMBINATION(LEFT, FIRST_RIGHT + 2) \
RIGHT_COMBINATION(LEFT, FIRST_RIGHT + 3) \

#define RIGHT_COMBINATIONS_FOR_PALETTE(PAL) \
RIGHT_COMBINATIONS(COLOR_INDEX(PAL, 0), COLOR_INDEX(PAL, 0)) \
RIGHT_COMBINATIONS(COLOR_INDEX(PAL, 1), COLOR_INDEX(PAL, 0)) \
RIGHT_COMBINATIONS(COLOR_INDEX(PAL, 2), COLOR_INDEX(PAL, 0)) \
RIGHT_COMBINATIONS(COLOR_INDEX(PAL, 3), COLOR_INDEX(PAL, 0)) \
 
/*
 
 // Alternative encoding, can't be configured right now
 
#define LEFT_COMBINATIONS(LEFT, FIRST_RIGHT) \
LEFT, LEFT, FIRST_RIGHT + 0, FIRST_RIGHT + 0 , LEFT, LEFT, FIRST_RIGHT + 0 , FIRST_RIGHT + 0 , \
LEFT, LEFT, FIRST_RIGHT + 1, FIRST_RIGHT + 1 , LEFT, LEFT, FIRST_RIGHT + 1 , FIRST_RIGHT + 1 , \
LEFT, LEFT, FIRST_RIGHT + 2, FIRST_RIGHT + 2 , LEFT, LEFT, FIRST_RIGHT + 2 , FIRST_RIGHT + 2 , \
LEFT, LEFT, FIRST_RIGHT + 3, FIRST_RIGHT + 3 , LEFT, LEFT, FIRST_RIGHT + 3 , FIRST_RIGHT + 3 , \

#define LEFT_COMBINATIONS_FOR_PALETTE(PAL) \
LEFT_COMBINATIONS(COLOR_INDEX(PAL, 0), COLOR_INDEX(PAL, 0)) \
LEFT_COMBINATIONS(COLOR_INDEX(PAL, 1), COLOR_INDEX(PAL, 0)) \
LEFT_COMBINATIONS(COLOR_INDEX(PAL, 2), COLOR_INDEX(PAL, 0)) \
LEFT_COMBINATIONS(COLOR_INDEX(PAL, 3), COLOR_INDEX(PAL, 0)) \

#define RIGHT_COMBINATIONS(LEFT, FIRST_RIGHT) \
LEFT, LEFT, FIRST_RIGHT + 0, FIRST_RIGHT + 0 , LEFT ^ 3, LEFT ^ 3, (FIRST_RIGHT + 0) ^ 3 , (FIRST_RIGHT + 0) ^ 3 , \
LEFT, LEFT, FIRST_RIGHT + 1, FIRST_RIGHT + 1 , LEFT ^ 3, LEFT ^ 3, (FIRST_RIGHT + 1) ^ 3 , (FIRST_RIGHT + 1) ^ 3 , \
LEFT, LEFT, FIRST_RIGHT + 2, FIRST_RIGHT + 2 , LEFT ^ 3, LEFT ^ 3, (FIRST_RIGHT + 2) ^ 3 , (FIRST_RIGHT + 2) ^ 3 , \
LEFT, LEFT, FIRST_RIGHT + 3, FIRST_RIGHT + 3 , LEFT ^ 3, LEFT ^ 3, (FIRST_RIGHT + 3) ^ 3 , (FIRST_RIGHT + 3) ^ 3 , \

#define RIGHT_COMBINATIONS_FOR_PALETTE(PAL) \
RIGHT_COMBINATIONS(COLOR_INDEX(PAL, 0), COLOR_INDEX(PAL, 0)) \
RIGHT_COMBINATIONS(COLOR_INDEX(PAL, 1), COLOR_INDEX(PAL, 0)) \
RIGHT_COMBINATIONS(COLOR_INDEX(PAL, 2), COLOR_INDEX(PAL, 0)) \
RIGHT_COMBINATIONS(COLOR_INDEX(PAL, 3), COLOR_INDEX(PAL, 0)) \
 */

const static uint8_t combinations[256 * 8] =
{
    LEFT_COMBINATIONS_FOR_PALETTE(7)
    RIGHT_COMBINATIONS_FOR_PALETTE(7)
    LEFT_COMBINATIONS_FOR_PALETTE(6)
    RIGHT_COMBINATIONS_FOR_PALETTE(6)
    LEFT_COMBINATIONS_FOR_PALETTE(5)
    RIGHT_COMBINATIONS_FOR_PALETTE(5)
    LEFT_COMBINATIONS_FOR_PALETTE(4)
    RIGHT_COMBINATIONS_FOR_PALETTE(4)
    LEFT_COMBINATIONS_FOR_PALETTE(3)
    RIGHT_COMBINATIONS_FOR_PALETTE(3)
    LEFT_COMBINATIONS_FOR_PALETTE(2)
    RIGHT_COMBINATIONS_FOR_PALETTE(2)
    LEFT_COMBINATIONS_FOR_PALETTE(1)
    RIGHT_COMBINATIONS_FOR_PALETTE(1)
    LEFT_COMBINATIONS_FOR_PALETTE(0)
    RIGHT_COMBINATIONS_FOR_PALETTE(0)
};

/* RGB2 is rounded to 5bpc, RGB1 remains as is. */
static unsigned rounded_color_diff(signed r1, signed g1, signed b1, signed r2, signed g2, signed b2)
{
    r2 &= 0xF8;
    r2 |= r2 >> 5;
    
    g2 &= 0xF8;
    g2 |= g2 >> 5;
    
    b2 &= 0xF8;
    b2 |= b2 >> 5;
    
    return abs(r1 - r2) + abs(g1 - g2) + abs(b1 - b2);
}

static unsigned score_for_combination(const uint8_t *r, const uint8_t *g, const uint8_t *b, const color_t *palette, uint8_t combination)
{
    unsigned score = 0;
    for (unsigned x = 0; x < 8; x++) {
        unsigned color_index = combinations[combination * 8 + x];
        unsigned diff = rounded_color_diff(r[x], g[x], b[x],
                                           palette[color_index].r, palette[color_index].g, palette[color_index].b);
        score += diff;
    }
    
    return score;
}

static unsigned best_combination_for_pixels(const uint8_t *r, const uint8_t *g, const uint8_t *b, const color_t *palette, unsigned *score)
{
    unsigned best = -1;
    unsigned best_index = -1;
    for (unsigned combination = 0; combination < 256; combination++) {
        unsigned current_score = score_for_combination(r, g, b, palette, combination);
        if (current_score < best) {
            best = current_score;
            best_index = combination;
        }
    }
    
    if (score) *score = best;
    
    return best_index;
}

static bool optimize_palette_step(const uint8_t *r, const uint8_t *g, const uint8_t *b, color_t *palette, unsigned old_score, unsigned *new_score)
{
    unsigned r_sum[PALETTE_SIZE] = {0,};
    unsigned g_sum[PALETTE_SIZE] = {0,};
    unsigned b_sum[PALETTE_SIZE] = {0,};
    unsigned count[PALETTE_SIZE] = {0,};
    unsigned combination_score = 0;
    unsigned score = 0;
    color_t old_palette[PALETTE_SIZE];
    memcpy(&old_palette, palette, sizeof(old_palette));
    setbuf(stdout, NULL);
    for (unsigned i = SCY_DATA_SIZE; i--;) {
        const uint8_t *combination = combinations + (best_combination_for_pixels(r, g, b, palette, &combination_score)) * 8;
        score += combination_score;
        for (unsigned x = 8; x--;) {
            unsigned color_index = *(combination++);
            r_sum[color_index] += *(r++);
            g_sum[color_index] += *(g++);
            b_sum[color_index] += *(b++);
            count[color_index] ++;
        }
    }
    
    
    for (unsigned i = 0; i < PALETTE_SIZE; i++) {
        if (count[i]) {
            palette[i].r = r_sum[i] / count[i];
            palette[i].g = g_sum[i] / count[i];
            palette[i].b = b_sum[i] / count[i];
        }
        else {
            palette[i].r = rand();
            palette[i].g = rand();
            palette[i].b = rand();
        }
    }
    
    if (old_score <= score) {
        memcpy(palette, &old_palette, sizeof(old_palette));
        return false;
    }
    *new_score = score;
    return true;
}

__attribute__((unused)) static void dump_tga_paletted(const char *output, const uint8_t *r, const uint8_t *g, const uint8_t *b, color_t *palette)
{
    static const uint8_t header[] = {
        0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xA0, 0x00, 0x90, 0x00, 0x18, 0x20,
    };
    FILE *out = fopen(output, "wb");
    fwrite(header, sizeof(header), 1, out);
    for (unsigned i = SCY_DATA_SIZE; i--;) {
        const uint8_t *combination = combinations + (best_combination_for_pixels(r, g, b, palette, NULL)) * 8;
        
        for (unsigned x = 8; x--;) {
            fwrite(palette + *(combination++), 3, 1, out);
        }
        r += 8;
        g += 8;
        b += 8;
    }
    fclose(out);
}

void round_palette(color_t *palette)
{
    for (unsigned i = PALETTE_SIZE; i--;) {
        palette->r &= 0xF8;
        palette->r |= palette->r >> 5;
        
        palette->g &= 0xF8;
        palette->g |= palette->g >> 5;
        
        palette->b &= 0xF8;
        palette->b |= palette->b >> 5;
        
        palette++;
    }
}

static uint8_t output[1024 * 1024 * 8 + 0x4000];
size_t pos = 0x4001;
size_t frame_count_pos = 0;
/* Quality: 0 is lossless (as far as format allows), increasing value reduces quality */
static void encode_image(const uint8_t *r, const uint8_t *g, const uint8_t *b, const color_t *palette, unsigned quality, unsigned count)
{
    if (count > 255) {
        encode_image(r, g, b, palette, quality, count - 255);
        count = 255;
    }
    static uint8_t previous_line_buffer[20] = {0,};
    uint8_t line_buffer[20] = {};
    uint8_t lossy_line_buffer[20] = {};
    quality += 8;
    
    /* Frames are not allowed to start in bank 0xFF */
    if (pos / 0x4000 == 0xFF) {
        pos &= ~0x3fff;
        pos += 0x4000;
        printf("\n\033[1m\033[33mWarning: This video exceeds 256 banks, it might not be compatible with all flash carts\033[0m\n");
    }
    
    frame_count_pos = pos;
    output[pos++] = (uint8_t) count;
    
    if ((pos & 0x3fff) >= 0x4000 - 33) {
        output[pos] = 0xFF; // Needs bank switch
        pos &= ~0x3fff;
        pos += 0x4000;
    }
    for (unsigned i = 0; i < PALETTE_SIZE / 2; i++) {
        uint16_t color = (palette[i].r >> 3) | ((palette[i].g >> 3) << 5) | ((palette[i].b >> 3) << 10);
        output[pos++] = color >> 8;
        output[pos++] = color;
    }
    
    if ((pos & 0x3fff) >= 0x4000 - 33) {
        output[pos] = 0xFF; // Needs bank switch
        pos &= ~0x3fff;
        pos += 0x4000;
    }
    for (unsigned i = PALETTE_SIZE / 2; i < PALETTE_SIZE; i++) {
        uint16_t color = (palette[i].r >> 3) | ((palette[i].g >> 3) << 5) | ((palette[i].b >> 3) << 10);
        output[pos++] = color >> 8;
        output[pos++] = color;
    }
    
    for (unsigned y = 0; y < 288; y++) {
        unsigned ndiffs = 0;
        for (unsigned j = 0; j < 20; j++) {
            unsigned score;
            uint8_t combination = (best_combination_for_pixels(r, g, b, palette, &score));
            line_buffer[j] = combination;
            lossy_line_buffer[j] = combination;
            if (previous_line_buffer[j] != combination) {
                unsigned lossy_score = score_for_combination(r, g, b, palette, previous_line_buffer[j]);
                if (lossy_score > score * quality / 8) {
                    ndiffs++;
                }
                else {
                    lossy_line_buffer[j] = previous_line_buffer[j];
                }
            }

            r += 8;
            g += 8;
            b += 8;
        }
        
        if (y == 0) {
            /* Using compression for the first line in the first subframe has some unpleast artifacts if the palette
               greatly changed from the previous frame */
            ndiffs = 20;
        }
        
        static const unsigned max_diffs = 3;
        
        if ((pos & 0x3fff) < 0x4000 - 22) {
            /* Sadly, we only support up to 3 diffs, because 4 diffs and more take too much CPU cycles to copy */
            if (ndiffs > max_diffs) {
                output[pos++] = 0; // Uncompressed, no bank switch
            }
            else {
                output[pos++] = (((9 - ndiffs) * 3 + 1) << 1); // Compressed
            }
        }
        else {
            output[pos] = 1; // Uncompressed, Bank switch
            pos &= ~0x3fff;
            pos += 0x4000;
            ndiffs = 20;
        }
        
        if (ndiffs > max_diffs) {
            memcpy(previous_line_buffer, line_buffer, 20);
            memcpy(output + pos, line_buffer, 20);
            for (unsigned i = 20; i--;) {
                output[pos++] -= (y % 144);
            }
        }
        else {
            unsigned test = 0;
            for (unsigned i = 0; i < 20; i++) {
                if (previous_line_buffer[i] != lossy_line_buffer[i]) {
                    output[pos++] = lossy_line_buffer[i] - (y % 144) + 1;
                    output[pos++] = i + 0xc1;
                    test++;
                }
            }
            memcpy(previous_line_buffer, lossy_line_buffer, 20);
            if (test != ndiffs) {
                printf("WAT %d != %d\n", test, ndiffs);
                exit(1);
            }
        }
    }
    
    if (count != 1 && pos / 0x4000 == 0xFF) {
        /* The last frame in the first 256 banks must not repeat. Encode the frame once again in the next bank. */
        
        output[frame_count_pos] = 1;
        encode_image(r - 46080, g - 46080, b - 46080, palette, quality, count - 1);
    }
}

static uint8_t quantize(signed sample)
{
    if (sample < 18)  return 0;
    if (sample < 55)  return 1;
    if (sample < 91)  return 2;
    if (sample < 130) return 3;
    if (sample < 164) return 4;
    if (sample < 200) return 5;
    if (sample < 237) return 6;
    return 7;
}

int main(int argc, const char * argv[])
{
    
    if (argc != 5) {
        printf("Usage: %s frame_rate quality audio_file.pcm output.bin < frame_list.txt\n", argv[0]);
        return -1;
    }
    
    double source_fps = atof(argv[1]);
    unsigned quality = atoi(argv[2]);
    
    uint8_t r[PIXELS], g[PIXELS], b[PIXELS], rgb[SCREEN_SIZE * 3];
    color_t palette[PALETTE_SIZE] = {{0,},};
    color_t rounded_palette[PALETTE_SIZE] = {{0,},};
    bool palette_inited = false;
    unsigned id = 0;
    double frame_multiplier = GB_FPS / 2 / source_fps;
    double fps_tracking = 0;
 
    FILE *audiof = fopen(argv[3], "rb");
    if (!audiof) {
        perror("Failed to load PCM file");
        return 1;
    }
    
    memset(output, 0xFF, sizeof(output));
    
    bool done = false;
    while (!done) {
        for (unsigned i = 154; i--; ) {
            uint8_t left, right;
            if (fread(&left, 1, 1, audiof) != 1 || fread(&right, 1, 1, audiof) != 1) {
                left = right = 0x80;
                done = true;
            }
            
            output[pos++] = quantize(left) | (quantize(right) << 4);
        }
        if ((pos & 0x3FFF) + 154 > 0x4000) {
            pos &= ~0x3fff;
            pos += 0x4000;
        }
    }
    
    pos--;
    pos &= ~0x3fff;
    pos += 0x4000;
    output[0x4000] = pos / 0x4000;
    fclose(audiof);
    char filepath[1024];
    printf("GBVP2 Encoder! \n");
    unsigned long diff = 0;
    while (true) {
        bool truncate = false;
        if (pos / 0x4000 == 0x1FF) {
            printf("\n\033[1m\033[31mError: This video exceeds 512 banks. Try reducing the quality by setting 'QUALITY' to a higher value.\033[0m\n");
            truncate = true;
        }
        if (scanf("%1023s", filepath) != 1 || truncate) {
            /* Frames are not allowed to start in bank 0xFF */
            if (((pos / 0x4000) & 0xFF) == 0xFF) {
                pos &= ~0x3fff;
                pos += 0x4000;
                printf("\n\033[1m\033[33mWarning: This video exceeds 256 banks, it might not be compatible with all flash carts\033[0m\n");
            }
            /* Add a restart marker */
            output[pos++] = 0;
            FILE *f = fopen(argv[4], "wb");
            if (!f) {
                perror("Failed to write output file");
                return 1;
            }
            pos +=  0x3fff;
            pos &= ~0x3fff;
            fwrite(output + 0x4000, pos - 0x4000, 1, f);
            fclose(f);
            printf("\n");
            return truncate;
        }
        
        fps_tracking += frame_multiplier;
        unsigned frame_length = (unsigned) fps_tracking;
        if (frame_length == 0) {
            continue;
        }
        fps_tracking -= frame_length;
        FILE *in = fopen(filepath, "rb");
        fseek(in, 18, SEEK_SET); // Skip header
        /* Deinterleave the image (easier to process) while computing diff */
        fread(rgb, sizeof(rgb), 1, in);
        for (unsigned i = 0; i < SCREEN_SIZE; i++) {
            diff += abs(b[i] - rgb[i * 3]);
            diff += abs(g[i] - rgb[i * 3 + 1]);
            diff += abs(r[i] - rgb[i * 3 + 2]);
            b[i] = rgb[i * 3];
            g[i] = rgb[i * 3 + 1];
            r[i] = rgb[i * 3 + 2];
        }
        fclose(in);
        if (id != 0 && diff < DIFF_THRESHOLD && (unsigned)output[frame_count_pos] + frame_length < 0xFF &&
            (frame_count_pos / 0x4000 != 0xFE)) {
            output[frame_count_pos] += frame_length;
            id++;
            continue;
        }
        else {
            diff = 0;
        }

        /* Create shifted image for the blending effect */
        
        memcpy(r + SCREEN_SIZE + 3, r, SCREEN_SIZE - 3);
        memcpy(g + SCREEN_SIZE + 3, g, SCREEN_SIZE - 3);
        memcpy(b + SCREEN_SIZE + 3, b, SCREEN_SIZE - 3);
        
        for (unsigned y = 0; y < 144; y++) {
            r[y * 160 + SCREEN_SIZE] = r[y * 160 + SCREEN_SIZE + 1] = r[y * 160 + SCREEN_SIZE + 2] = r[y * 160 + SCREEN_SIZE + 3];
            g[y * 160 + SCREEN_SIZE] = g[y * 160 + SCREEN_SIZE + 1] = g[y * 160 + SCREEN_SIZE + 2] = g[y * 160 + SCREEN_SIZE + 3];
            b[y * 160 + SCREEN_SIZE] = b[y * 160 + SCREEN_SIZE + 1] = b[y * 160 + SCREEN_SIZE + 2] = b[y * 160 + SCREEN_SIZE + 3];
        }
        
        if (!palette_inited) {
            for (unsigned i = 0; i < PALETTE_SIZE; i++) {
                unsigned pixel = rand() % PIXELS;
                palette[i].r = r[pixel];
                palette[i].g = g[pixel];
                palette[i].b = b[pixel];
            }
            palette_inited = true;
        }
        
        unsigned i = 128;
        unsigned score = -1;
        for (; i--;) {
            if (!optimize_palette_step(r, g, b, palette, score, &score)) break;
        }
        
        
        memcpy(rounded_palette, palette, sizeof(palette));
        round_palette(rounded_palette);
        /*
         // For debugging
        sprintf(filepath, "debug/%04u.tga", id);
        dump_tga_paletted(filepath, r, g, b, palette);
        */
        printf("\rEncoding frame %d       ", id);
        encode_image(r, g, b, palette, quality, frame_length);
        id++;
    }
}
