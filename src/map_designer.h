/*
MIT License

Copyright (c) 2025 oOluki

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef MAP_DESIGNER_H
#define MAP_DESIGNER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#ifndef TILE
    #define TILE unsigned int
#endif

static TILE** map;
static int  mapw = 0;
static int  maph = 0;
static int  layers = 0;

static char* map_path = NULL;

static int camerax = 0;
static int cameray = 0;
static int cameraw = 32;
static int camerah = 24;

static int current_layer;
static int held_tile;

static int pencilw = 1;
static int pencilh = 1;

static int copyx = -1;
static int copyy = -1;


#define BUFF_CAP 1024

static char buff[BUFF_CAP];
static int  buffsize = 0;

static char DEFAULT_PALETTE[] = " 123456789abcdefghijklmnopqrstuvwxyz";

static char* palette = DEFAULT_PALETTE;

static int palette_len = (sizeof(DEFAULT_PALETTE) - sizeof(DEFAULT_PALETTE[0])) / sizeof(DEFAULT_PALETTE[0]);

#define DEFAULT_ASCII " .-~:;=+*!%xX$&#@"

static const char* ascii_map = DEFAULT_ASCII;
static int         ascii_len = sizeof(DEFAULT_ASCII) - sizeof(char);

#define ALIGN_BUFF_TO(N) if((N) > 1) buffsize += (N) - (buffsize % (N))

static const char* output_path;
static FILE* output;

static void (*display)(int);

static const char* tileset_path;
static stbi_uc* tileset;
static int tileset_tilew;
static int tileset_tileh;
static int tilesetx;
static int tilesety;
static int tileset_comp;

static uint32_t* pixels;
static int       pixelsw;
static int       pixelsh;

static int cursorx;
static int cursory;

static int active = 0;

static int changed_since_last_save = 0;

static TILE tile_mapping[32];

static uint32_t __is_tile_mapped = 0;

static inline int is_tile_mapped(TILE tile){
    return ((tile < sizeof(tile_mapping) / sizeof(tile_mapping[0])) && (__is_tile_mapped & (1 << tile)));
}

static inline TILE get_real_tile(TILE tile){
    return is_tile_mapped(tile)? tile_mapping[tile] : tile;
}

static inline int map_tile(TILE key, TILE value){
    if(key >= sizeof(tile_mapping) / sizeof(tile_mapping[0])){
        fprintf(stderr, "[ERROR] can not map tiles bigger than %i\n", (int) (sizeof(tile_mapping) / sizeof(tile_mapping[0])) - 1);
        return 1;
    }

    __is_tile_mapped |= (1 << key);

    tile_mapping[key] = value;

    return 0;
}

static inline void unmap_tile(TILE key){
    if(key < sizeof(tile_mapping) / sizeof(tile_mapping[0]))
        __is_tile_mapped &= ~(1 << key);
}

static inline int is_png_extension(const char* path){
    if(!path) return 0;
    int path_len = 0;
    for(; path[path_len]; path_len+=1);
    if(path_len > 3){
        return (path[path_len - 1] == 'g') && (path[path_len - 2] == 'n') && (path[path_len - 3] == 'p') && (path[path_len - 4] == '.');
    }
    return 0;
}

static inline int cmp_str(const char* str1, const char* str2){
    if(!str1 || !str2) return 0;
    for(; *str1 && *str1 == *str2; str1+=1) str2 += 1;
    return *str1 == *str2;
}

static int parse_uint(const char* str){
    int output = 0;
    while (*str >= '0' && *str <= '9')
    {
        output = (output * 10) + *str++ - '0';
    }
    return (*str == '\0')? output : -1;
}

static inline int Mstrlen(const char* str){
    int len = 0;
    for(; str[len]; len+=1);
    return len;
}

static inline int fexpect(FILE* f, const char* expected, int* first_last, int* column, int* row){
    int c = *first_last;

    // setting dummy values, if necessary
    if(!column) column = &c;
    if(!row)    row    = &c;

    for(; *expected && c == *expected; c = fgetc(f)){
        expected+=1;
        const int cond = c == '\n';
        *column = cond? 1 : *column + 1;
        *row    += cond;
    }
    *first_last = c;
    return *expected == '\0';
}

static inline int fparse_uint(FILE* f, int* first_last, int* column){
    int output = 0;
    int c = *first_last;

    // setting dummy values, if necessary
    if(!column) column = &c;

    if(c < '0' || c > '9') return -1;
    for(; c >= '0' && c <= '9'; c = fgetc(f)){
        output = (output * 10) + (c - '0');
        *column += 1;
    }
    *first_last = c;
    return output;
}

static inline void fskip(FILE* f, int* first_last, int* column, int* row){
    int c = *first_last;

    // setting dummy values, if necessary
    if(!column) column = &c;

    for(; c == ' ' || c == '\t' || c == '\n'; c = fgetc(f)){
        const int cond = c == '\n';
        *column = cond? 1 : *column + 1;
        *row    += cond;
    }
    *first_last = c;
}

int load_map(const char* path){
    #define __ERROR(MSG, ...) do { fprintf(stderr, "[ERROR] %s:%i:%i: " MSG "\n", path, row, column, __VA_ARGS__); } while(0)
    if(!path){
        fprintf(stderr, "[ERROR] missing path, required for first load\n");
    }

    FILE* f = fopen(path, "r");

    int column = 1;
    int row = 1;

    int err = 0;

    if(!f){
        fprintf(stderr, "[ERROR] Could not open '%s'\n", path);
        return 1;
    }

    int c = fgetc(f);

    fskip(f, &c, &column, &row);

    if(0 == fexpect(f, "map:", &c, &column, &row)){
        fclose(f);
        int w;
        int h;
        int comp;
        stbi_uc* pixels = stbi_load(path, &w, &h, &comp, 0);
        if(!pixels){
            __ERROR("expected 'map:' identifier%c", ' ');
            return 1;
        }
        if(map){
            for(int k = 0; k < layers; k+=1) free(map[k]);
            free(map);
        }
        mapw = w;
        maph = h;
        layers = comp;
        map = malloc(layers * sizeof(map[0]));
        for(int k = 0; k < layers; k+=1){
            map[k] = malloc(mapw * maph * sizeof(map[0][0]));
            for(int i = 0; i < maph; i+=1){
                for(int j = 0; j < mapw; j+=1){
                    map[k][i * mapw + j] = pixels[(i * w + j) * comp + k];
                }
            }
        }

        stbi_image_free(pixels);

        if(map_path == path) return 0;
        int i = 0;
        for(; path[i]; i+=1);
        if(map_path) free(map_path);
        map_path = malloc(i + 1);
        for(map_path[i + 1] = '\0'; i > -1; i-=1) map_path[i] = path[i];
        return 0;
    }

    fskip(f, &c, &column, &row);
    
    if(0 == fexpect(f, "width:", &c, &column, &row)){
        err = 1;
        __ERROR("expected 'width:' identifier%c", ' ');
        goto defer;
    }
    for(; c == ' ' || c == '\t'; c = fgetc(f)) column += 1;

    const int width = fparse_uint(f, &c, &column);
    if(width <= 0){
        __ERROR("invalid width%c", ' ');
        err = 1;
        goto defer;
    }
    fskip(f, &c, &column, &row);
    if(!fexpect(f, "height:", &c, &column, &row)){
        __ERROR("expected 'height:' identifier%c", ' ');
        err = 1;
        goto defer;
    }
    for(; c == ' ' || c == '\t'; c = fgetc(f)) column += 1;

    const int height = fparse_uint(f, &c, &column);
    if(height <= 0){
        __ERROR("invalid height%c", ' ');
        err = 1;
        goto defer;
    }
    fskip(f, &c, &column, &row);

    if(!fexpect(f, "layers:", &c, &column, &row)){
        __ERROR("expected 'layers:' identifier%c", ' ');
        err = 1;
        goto defer;
    }
    for(; c == ' ' || c == '\t'; c = fgetc(f)) column += 1;

    const int lyr = fparse_uint(f, &c, &column);
    if(lyr <= 0){
        __ERROR("invalid layer count%c", ' ');
        err = 1;
        goto defer;
    }
    fskip(f, &c, &column, &row);

    const int _map_size = width * height;
    TILE** _map = malloc(lyr * sizeof(_map[0]));
    for(int k = 0; k < lyr; k +=1){
        _map[k] = malloc(_map_size * sizeof(_map[0][0]));
        for(int i = 0; i < _map_size; i+=1){
            fskip(f, &c, &column, &row);
            const int tile = fparse_uint(f, &c, &column);
            if(tile < 0){
                __ERROR("invalid/missing tile identifier for (%i, %i) layer %i", i % width, (int) (i / width), k);
                for(int n = 0; n < k; n+=1) free(_map[n]);
                free(_map);
                err = 1;
                goto defer;
            }
            if((char) (tile) != tile){
                __ERROR("tile value overflow at (%i, %i) layer %i", i % width, (int) (i / width), k);
                for(int n = 0; n < k; n+=1) free(_map[n]);
                free(_map);
                err = 1;
                goto defer;
            }
            _map[k][i] = (char) tile;
            fskip(f, &c, &column, &row);
            if(c != ','){
                __ERROR("expected ',' after tile at (%i, %i) layer %i", i % width, (int) (i / width), k);
                for(int n = 0; n < k; n+=1) free(_map[n]);
                free(_map);
                err = 1;
                goto defer;
            }
            c = fgetc(f);
        }
    }
    fskip(f, &c, &column, &row);
    if(c != EOF){
        __ERROR("Unexpected things at the end of file%c", ' ');
        for(int n = 0; n < lyr; n+=1) free(_map[n]);
        free(_map);
        err = 1;
        goto defer;
    }

    if(map){
        for(int k = 0; k < layers; k+=1) free(map[k]);
        free(map);
    }
    map = _map;
    mapw = width;
    maph = height;
    layers = lyr;

    if(map_path == path) return 0;
    int i = 0;
    for(; path[i]; i+=1);
    if(map_path) free(map_path);
    map_path = malloc(i + 1);
    for(map_path[i + 1] = '\0'; i > -1; i-=1) map_path[i] = path[i];

    defer:
    if(f) fclose(f);
    return err;
    #undef __ERROR
}


int save_map(const char* path){

    if(!path){
        fprintf(stderr, "[ERROR] missing path, required for first save\n");
        return 1;
    }
    int save_as_png = is_png_extension(path);

    if(save_as_png && layers > 4){
        fprintf(stderr, "[ERROR] can't save map with more than 4 layers as png, got %i layers", layers);
        return 1;
    }
    if(save_as_png){
        stbi_uc* output = malloc(mapw * maph * 4);
        for(int i = 0; i < maph; i+=1){
            for(int j = 0; j < mapw; j += 1){
                for(int k = 0; k < layers; k+=1)
                    output[(i * mapw + j) * layers + k] = (stbi_uc) map[k][i * mapw + j];
                for(int k = layers; k < 4; k+=1)
                    output[(i * mapw + j) * layers + k] = 0;
            }
        }
        if(!stbi_write_png(path, mapw, maph, layers, output, mapw * layers)){
            fprintf(stderr, "[ERROR] could not save map to '%s' as png\n", path);
            free(output);
            return 1;
        }
        free(output);

        changed_since_last_save = 0;

        if(map_path == path) return 0;
        int i = 0;
        for(; path[i]; i+=1);
        if(map_path) free(map_path);
        map_path = malloc(i + 1);
        for(map_path[i + 1] = '\0'; i > -1; i-=1) map_path[i] = path[i];
        return 0;
    }

    FILE* f = fopen(path, "wb");

    if(!f){
        fprintf(stderr, "[ERROR] Could not open '%s'\n", path);
        return 1;
    }
    fprintf(f, "map:\n");
    fprintf(f, "width: %i\nheight: %i\nlayers: %i\n\n", mapw, maph, layers);

    for(int k = 0; k < layers; k+=1){
        for(int i = 0; i < maph; i+=1){
            fputc(' ', f);
            fputc(' ', f);
            fputc(' ', f);
            for(int j = 0; j < mapw; j+=1){
                    fprintf(f, " %3u,", (unsigned int) map[k][i * mapw + j]);
            }
            fputc('\n', f);
        }
        fputc('\n', f);
        fputc('\n', f);
    }
    fclose(f);

    changed_since_last_save = 0;

    if(map_path == path){
        return 0;
    }
    int i = 0;
    for(; path[i]; i+=1);
    if(map_path) free(map_path);
    map_path = malloc(i + 1);
    for(map_path[i + 1] = '\0'; i > -1; i-=1) map_path[i] = path[i];
    
    return 0;
}

int place(TILE tile, int x, int y){
    const int xrange = (x + pencilw < mapw)? x + pencilw : mapw;
    const int yrange = (y + pencilh < maph)? y + pencilh : maph;
    const int x0 = (x < 0)? 0 : x;

    tile = get_real_tile(tile);

    for(int i = (y < 0)? 0 : y; i < yrange; i+=1){
        for(int j = x0; j < xrange; j+=1){
            map[current_layer][i * mapw + j] = (TILE) tile;
        }
    }
    changed_since_last_save = 1;
    return 0;
}

int place_hollow(TILE tile, int x, int y){
    const int xrange = (x + pencilw < mapw)? x + pencilw : mapw;
    const int yrange = (y + pencilh < maph)? y + pencilh : maph;
    const int x0 = (x < 0)? 0 : x;
    const int y0 = (y < 0)? 0 : y;

    tile = get_real_tile(tile);

    if(x >= 0 && x < mapw) for(int i = y0; i < yrange; i+=1){
        map[current_layer][i * mapw + x]       = (TILE) tile;
    }
    if(xrange > 0 && x + pencilw <= mapw) for(int i = y0; i < yrange; i+=1){
        map[current_layer][i * mapw + xrange - 1]   = (TILE) tile;
    }
    if(y >= 0 && y < maph) for(int j = x0; j < xrange; j+=1){
        map[current_layer][y * mapw + j]       = (TILE) tile;
    }
    if(yrange > 0 && y + pencilh <= maph) for(int j = x0; j < xrange; j+=1){
        map[current_layer][(yrange - 1) * mapw + j]   = (TILE) tile;
    }
    changed_since_last_save = 1;

    return 0;
}

int paste(int destx, int desty){

    if(copyx < 0 || copyy < 0){
        fprintf(stderr, "No valid copied selection\n");
        return 1;
    }

    const int srcxrange = (copyx + pencilw < mapw)? pencilw : mapw;
    const int srcyrange = (copyy + pencilh < maph)? pencilh : maph;

    const int destxrange = (destx + pencilw < mapw)? pencilw : mapw;
    const int destyrange = (desty + pencilh < maph)? pencilh : maph;

    const int xrange = (srcxrange < destxrange)? srcxrange : destxrange;
    const int yrange = (srcyrange < destyrange)? srcyrange : destyrange;

    const int overlayx = (destx >= copyx && destx < copyx + srcxrange)? copyx + srcxrange - destx : 0;
    const int overlayy = (desty >= copyy && desty < copyy + srcyrange)? copyy + srcyrange - desty : 0;

    for(int i = overlayy; i < yrange; i+=1){
        for(int j = overlayx; j < xrange; j+=1){
            map[current_layer][(i + desty) * mapw + (j + destx)] = map[current_layer][(i + copyy) * mapw + (j + copyx)];
        }
    }
    for(int i = 0; i < overlayy; i+=1){
        for(int j = 0; j < overlayy; j+=1){
            map[current_layer][(i + desty) * mapw + (j + destx)] = map[current_layer][(i + copyy) * mapw + (j + copyx)];
        }
    }
    changed_since_last_save = 1;

    return 0;
}




static inline int cramp(int x, int max, int min) {
	if (max < min) {
		const int dummy = max;
		max = min;
		min = dummy;    
	}  
	if (x > max)
	  x = max;
	if (x < min)
		x = min;
	return x;  
}

static inline void consume_whole_line(){
    for(int c = getchar(); c && c != '\n' && c != EOF; c = getchar());
}


static inline void* align_buff_to(int n){
    if(n) buffsize += n - (buffsize % n);
    return (void*) &buff[buffsize];
}

static inline int getascii_alpha_index(uint8_t alpha){
    return (alpha * (ascii_len - 1)) / 255; 
}

static int getascii_color_index(uint32_t color){

    const uint32_t rw = 2126;
    const uint32_t gw = 7152;
    const uint32_t bw =  722;

    const uint32_t r = ((color >>  0) & 0xFF);
    const uint32_t g = ((color >>  8) & 0xFF);
    const uint32_t b = ((color >> 16) & 0xFF);
    const uint32_t a = ((color >> 24) & 0xFF);

    if(!a) return 0;

    const uint32_t brightness = (rw * r + gw * g + bw * b) / (rw + gw + bw);

    return (brightness <= 255)? (brightness * (ascii_len - 1)) / 255 : (ascii_len - 1);
}

static void print_map(int draw_interssections){

    if(output == stdout) printf("\x1B[2J\x1B[H\n");
    else{
        if(output) fclose(output);
        output = NULL;
        output = fopen(output_path, "w");
        if(!output){
            fprintf(stderr, "[ERROR] could not open output '%s', switching back to stdout\n", output_path);
            output = stdout;
        }
    }

    const int i0 = (cameray < 0)? 0 : cameray;
    const int j0 = (camerax < 0)? 0 : camerax;
    const int irange = (cameray + camerah < maph)? cameray + camerah : maph;
    const int jrange = (camerax + cameraw < mapw)? camerax + cameraw : mapw;

    int idigit_len = 1;
    for(int _10n = 10; (int) (irange / _10n); _10n *= 10) idigit_len+=1;

    int jdigit_len = 1;
    for(int _10n = 10; (int) (jrange / _10n); _10n *= 10) jdigit_len+=1;

    for(int i = 0; i < idigit_len + 3; i+=1)
        putc(' ', output);

    for(int j = j0; j < jrange; j+=1)
        fprintf(output, "%*i", jdigit_len + 1, j);
    putc('\n', output);
    for(int i = 0; i < idigit_len + 3; i+=1)
        putc(' ', output);
    for(int j = j0; j < jrange; j+=1){
        for(int i = (jdigit_len / 2) + 1; i; i-=1)
            putc(' ', output);
        putc('|', output);
    }
    putc('\n', output);
    for(int i = 0; i < idigit_len + 3; i+=1)
        putc(' ', output);
    for(int j = j0; j < jrange; j+=1)
        for(int i = 0; i < (jdigit_len + 1); i+=1)
            putc('_', output);

    putc('\n', output);

    if(draw_interssections){
        for(int i = i0; i < irange; i+=1){
            fprintf(output, "%*i- |", idigit_len, i);
            for(int j = j0; j < jrange; j+=1){
                for(int i = (jdigit_len / 2) + 1; i; i-=1)
                    putc(' ', output);
                int interssections = 0;
                for(int k = 0; k < layers; k+=1){
                    interssections += (map[k][i * mapw + j] != 0);
                }
                if(interssections > 9){
                    putc('!', output);
                }
                else if(interssections > 0){
                    putc('0' + interssections, output);
                }
                else{
                    putc(' ', output);
                }
            }
            putc('\n', output);
        }
    }
    else{
        for(int i = i0; i < irange; i+=1){
            fprintf(output, "%*i- |", idigit_len, i);
            for(int j = j0; j < jrange; j+=1){
                for(int i = (jdigit_len / 2) + 1; i; i-=1)
                    putc(' ', output);

                TILE actual_tile = map[current_layer][i * mapw + j];
                for(int i = 0; i < sizeof(tile_mapping) / sizeof(tile_mapping[0]); i+=1){
                    if(tile_mapping[i] == actual_tile){
                        if(is_tile_mapped(i)){
                            actual_tile = i;
                            break;
                        }
                    }
                }

                putc(
                    (actual_tile < palette_len && actual_tile >= 0)?
                        (int) palette[actual_tile] : '~', output
                );
            }
            putc('\n', output);
        }
    }

}


static inline uint32_t blend_color_channel(const uint32_t ct, const uint32_t at, const uint32_t cb){
    return (ct * at + (cb * (255 - at))) / 255;
}

static inline uint32_t blend_colors(const uint32_t ct, const uint32_t cb){
    const uint32_t rb = (cb >>  0) & 0xFF;
    const uint32_t gb = (cb >>  8) & 0xFF;
    const uint32_t bb = (cb >> 16) & 0xFF;
    const uint32_t ab = (cb >> 24) & 0xFF;
    const uint32_t rt = (ct >>  0) & 0xFF;
    const uint32_t gt = (ct >>  8) & 0xFF;
    const uint32_t bt = (ct >> 16) & 0xFF;
    const uint32_t at = (ct >> 24) & 0xFF;
    const uint32_t ro = blend_color_channel(rt, at, rb);
    const uint32_t go = blend_color_channel(gt, at, gb);
    const uint32_t bo = blend_color_channel(bt, at, bb);
    return (ro << 0) | (go << 8) | (bo << 16) | (ab << 24);
}

static inline const char* get_color_string(uint32_t foregroung_color, uint32_t background_color){
    static char buff[64];

    const uint8_t fr = (foregroung_color >>  0) & 0xFF;
    const uint8_t fg = (foregroung_color >>  8) & 0xFF;
    const uint8_t fb = (foregroung_color >> 16) & 0xFF;
    const uint8_t fa = (foregroung_color >> 24) & 0xFF;


    const uint8_t br = (background_color >>  0) & 0xFF;
    const uint8_t bg = (background_color >>  8) & 0xFF;
    const uint8_t bb = (background_color >> 16) & 0xFF;
    const uint8_t ba = (background_color >> 24) & 0xFF;

    sprintf(
        buff,
        "\x1b[38;2;%" PRIu8 ";%" PRIu8 ";%" PRIu8 "m"
        "\x1b[48;2;%" PRIu8 ";%" PRIu8 ";%" PRIu8 "m",
        fr, fg, fb,
        br, bg, bb
    );

    return buff;
}

static void put_color_char(uint32_t color){

    const uint8_t a = (color >> 24) & 0xFF;

    const char c = ascii_map[getascii_alpha_index(a)];

    printf("%s%c\x1b[0m", get_color_string(color, (a == 255)? color : 0x0), (a == 255)? ' ' : c);
}

static void render_tile_graphical(int tile, int x, int y, uint32_t* pixels, int pixelsw, int pixelsh, int pixels_stride){
    if(!pixels || !tileset) return;
    
    const int y0 = (y < 0)? 0 : y;
    const int x0 = (x < 0)? 0 : x;
    const int yrange = (y + tileset_tileh < pixelsh)? y + tileset_tileh : pixelsh;
    const int xrange = (x + tileset_tilew < pixelsw)? x + tileset_tilew : pixelsw;

    tile -= 1;

    if(tile >= tilesetx * tilesety || tile < 0){
        const uint32_t color = 0xFF0000FF;
        for(int i = y0; i < yrange; i+=1){
            for(int j = x0; j < xrange; j+=1){
                pixels[i * pixels_stride + j] = color;
            }
        }
        return;
    }
    const int tiley = (tile / (tilesetx / tileset_tilew));
    const int tilex = (tile % (tilesetx / tileset_tilew));
    const int tileoffset = tiley * tileset_tileh * tilesetx + tilex * tileset_tilew;

    y = y0;
    for(int i = 0; i < tileset_tileh && y < yrange; i+=1){
        x = x0;
        for(int j = 0; j < tileset_tilew && x < xrange; j+=1){
            uint32_t color = 0;
            for(int k = 0; k < tileset_comp; k+=1){
                color |= (tileset[(tileoffset + i * tilesetx + j) * tileset_comp + k]) << (k * 8);
            }
            pixels[y * pixels_stride + x] = blend_colors(color, pixels[y * pixels_stride + x]);
            x += 1;
        }
        y += 1;
    }
}

static void render_graphical(int draw_all_layers){

    if(!tileset){
        fprintf(stderr, "[ERROR] can't draw graphical representation of map, missing tileset, going back to standard\n");
        display = print_map;
        return;
    }

    const int pixelsw = cameraw * tileset_tilew;
    const int pixelsh = camerah * tileset_tileh;
    const int pixels_stride = pixelsw;

    uint32_t* pixels = malloc(pixelsw * pixelsh * sizeof(pixels[0]));

    if(!pixels){
        fprintf(stderr, "[ERRROR] can't draw graphical representation of map, could not create pixel buffer\n");
        return ;
    }

    for(int i = 0; i < cameraw * tileset_tilew * camerah * tileset_tileh; i+=1) pixels[i] = 0xFF000000;

    const int i0 = (cameray < 0)? 0 : cameray;
    const int j0 = (camerax < 0)? 0 : camerax;
    const int irange = (cameray + camerah < maph)? cameray + camerah : maph;
    const int jrange = (camerax + cameraw < mapw)? camerax + cameraw : mapw;

    if(draw_all_layers){
        for(int i = i0; i < irange; i+=1){
            for(int j = j0; j < jrange; j+=1){
                for(int k = layers - 1; k > -1; k-=1){
                    render_tile_graphical(
                        map[k][i * mapw + j],
                        (j - j0) * tileset_tilew, (i - i0) * tileset_tileh,
                        pixels, pixelsw, pixelsh, pixels_stride
                    );
                }
            }
        }
    }
    else{
        for(int i = i0; i < irange; i+=1){
            for(int j = j0; j < jrange; j+=1){
                render_tile_graphical(
                    map[current_layer][i * mapw + j],
                    (j - j0) * tileset_tilew, (i - i0) * tileset_tileh,
                    pixels, pixelsw, pixelsh, pixels_stride
                );
            }
        }
    }

    if(is_png_extension(output_path)){
        if(output) fclose(output);
        output = NULL;
        if(!stbi_write_png(output_path, pixelsw, pixelsh, (int) sizeof(pixels[0]), pixels, pixels_stride * (int) sizeof(pixels[0]))){
            fprintf(stderr, "[ERROR] could not render graphical representation to '%s'\n", output_path);
        }
    }
    else{
        if(output && output != stdout){
            fclose(output);
            output = NULL;
            output = fopen(output_path, "w");
        }
        if(!output){
            fprintf(stderr, "[ERROR] could not open output '%s', switching back to stdout\n", output_path);
            output = stdout;
            free(pixels);
            return ;
        }
        if(output == stdout){
            printf("\x1B[2J\x1B[H\n");
            for(int i = 0; i < (irange - i0) * tileset_tileh; i+=1){
                for(int j = 0; j < (jrange - j0) * tileset_tilew; j+=1){
                    put_color_char(pixels[i * pixels_stride + j]);
                }
                fprintf(output, "\n");
            }
        }
        else{
            for(int i = 0; i < (irange - i0) * tileset_tileh; i+=1){
                for(int j = 0; j < (jrange - j0) * tileset_tilew; j+=1){
                    fprintf(output, "%c", ascii_map[getascii_color_index(pixels[i * pixels_stride + j])]);
                }
                fprintf(output, "\n");
            }
        }
    }


    free(pixels);
}


static void render_terminal_and_graphics(int draw_all_layers){

    FILE* const saved_output = output;

    output = stdout;

    print_map(draw_all_layers);

    output = saved_output;

    if(output == stdout){
        return ;
    }
    render_graphical(draw_all_layers);
}

static void render_terminal_and_print_map(int draw_all_layers){

    FILE* const saved_output = output;

    output = stdout;

    print_map(draw_all_layers);

    output = saved_output;

    if(output == stdout){
        return ;
    }
    print_map(draw_all_layers);
}

#endif // =====================  END OF FILE MAP_DESIGNER_H ===========================