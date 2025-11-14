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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


static char** map;
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

static inline int fexpect(FILE* f, const char* expected, int* first_last){
    int c = *first_last;
    for(; *expected && c == *expected; c = fgetc(f)) expected+=1;
    *first_last = c;
    return *expected == '\0';
}

static inline int fparse_uint(FILE* f, int* first_last){
    int output = 0;
    int c = *first_last;
    if(c < '0' || c > '9') return -1;
    for(; c >= '0' && c <= '9'; c = fgetc(f)){
        output = (output * 10) + (c - '0');
    }
    *first_last = c;
    return output;
}

int load_map(const char* path){

    if(!path){
        fprintf(stderr, "[ERROR] missing path, required for first load\n");
    }

    FILE* f = fopen(path, "r");

    int err = 0;

    if(!f){
        fprintf(stderr, "[ERROR] Could not open '%s'\n", path);
        return 1;
    }

    int c = fgetc(f);

    for(; c == ' ' || c == '\t' || c == '\n'; c = fgetc(f));

    if(0 == fexpect(f, "map:", &c)){
        fclose(f);
        int w;
        int h;
        int comp;
        stbi_uc* pixels = stbi_load(path, &w, &h, &comp, 0);
        if(!pixels){
            fprintf(stderr, "expected 'map:' identifier\n");
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
        for(int k = 0; k < layers; k+=1) map[k] = malloc(mapw * maph * sizeof(map[0][0]));

        if(map_path == path) return 0;
        int i = 0;
        for(; path[i]; i+=1);
        if(map_path) free(map_path);
        map_path = malloc(i + 1);
        for(map_path[i + 1] = '\0'; i > -1; i-=1) map_path[i] = path[i];
        return 0;
    }

    for(; c == ' ' || c == '\t' || c == '\n'; c = fgetc(f));
    
    if(0 == fexpect(f, "width:", &c)){
        err = 1;
        fprintf(stderr, "[ERROR] expected 'width:' identifier\n");
        goto defer;
    }
    for(; c == ' ' || c == '\t'; c = fgetc(f));

    const int width = fparse_uint(f, &c);
    if(width <= 0){
        fprintf(stderr, "[ERROR] invalid width\n");
        err = 1;
        goto defer;
    }
    for(; c == ' ' || c == '\t' || c == '\n'; c = fgetc(f));
    if(!fexpect(f, "height:", &c)){
        fprintf(f, "[ERROR] expected 'height:' identifier\n");
        err = 1;
        goto defer;
    }
    for(; c == ' ' || c == '\t'; c = fgetc(f));

    const int height = fparse_uint(f, &c);
    if(height <= 0){
        fprintf(stderr, "[ERROR] invalid height\n");
        err = 1;
        goto defer;
    }
    for(; c == ' ' || c == '\t' || c == '\n'; c = fgetc(f));

    if(!fexpect(f, "layers:", &c)){
        fprintf(f, "[ERROR] expected 'layers:' identifier\n");
        err = 1;
        goto defer;
    }
    for(; c == ' ' || c == '\t'; c = fgetc(f));

    const int lyr = fparse_uint(f, &c);
    if(lyr <= 0){
        fprintf(stderr, "[ERROR] invalid layers\n");
        err = 1;
        goto defer;
    }
    for(; c == ' ' || c == '\t' || c == '\n'; c = fgetc(f));

    const int _map_size = width * height;
    char** _map = malloc(lyr * sizeof(_map[0]));
    for(int k = 0; k < lyr; k +=1){
        _map[k] = malloc(_map_size);
        for(int i = 0; i < _map_size; i+=1){
            for(; c == ' ' || c == '\t' || c == '\n'; c = fgetc(f));
            const int tile = fparse_uint(f, &c);
            if(tile < 0){
                fprintf(stderr, "[ERROR] invalid/missing tile identifier for (%i, %i) layer %i\n", i % width, (int) (i / width), k);
                for(int n = 0; n < k; n+=1) free(_map[n]);
                free(_map);
                err = 1;
                goto defer;
            }
            if((char) (tile) != tile){
                fprintf(stderr, "[ERROR] tile value overflow at (%i, %i) layer %i\n", i % width, (int) (i / width), k);
                for(int n = 0; n < k; n+=1) free(_map[n]);
                free(_map);
                err = 1;
                goto defer;
            }
            _map[k][i] = (char) tile;
            for(; c == ' ' || c == '\t' || c == '\n'; c = fgetc(f));
            if(c != ','){
                fprintf(stderr, "[ERROR] expected ',' after tile at (%i, %i) layer %i\n", i % width, (int) (i / width), k);
                for(int n = 0; n < k; n+=1) free(_map[n]);
                free(_map);
                err = 1;
                goto defer;
            }
            c = fgetc(f);
        }
    }
    for(; c == ' ' || c == '\t' || c == '\n'; c = fgetc(f));
    if(c != EOF){
        fprintf(stderr, "[ERROR] Unexpected things at the end of file\n");
        fputc('\'', stderr);
        for(int i = 0; c != EOF && i < 10; i += 1){
            fputc(c, stderr);
            c = fgetc(f);
        }
        fprintf(stderr, "\'...\n");
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
        if(stbi_write_png(path, mapw, maph, layers, map, mapw * layers)){
            fprintf(stderr, "[ERROR] could not save map to '%s' as png\n", path);
            free(output);
            return 1;
        }
        free(output);

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

int place(int tile, int x, int y){
    const int xrange = (x + pencilw < mapw)? x + pencilw : mapw;
    const int yrange = (y + pencilh < maph)? y + pencilh : maph;
    const int x0 = (x < 0)? 0 : x;
    for(int i = (y < 0)? 0 : y; i < yrange; i+=1){
        for(int j = x0; j < xrange; j+=1){
            map[current_layer][i * mapw + j] = (char) tile;
        }
    }
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
    return 0;
}

#endif // =====================  END OF FILE MAP_DESIGNER_H ===========================