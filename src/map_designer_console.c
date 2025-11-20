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

#include "map_designer.h"


#define BUFF_CAP 1024

static char buff[BUFF_CAP];
static int  buffsize = 0;

static char DEFAULT_PALETTE[] = " 123456789abcdefghijklmnopqrstuvwxyz";

static char* palette = DEFAULT_PALETTE;

static int palette_len = (sizeof(DEFAULT_PALETTE) - sizeof(DEFAULT_PALETTE[0])) / sizeof(DEFAULT_PALETTE[0]);

#define DEFAULT_ASCII "@#Xx%*+=~-:. "

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

enum Instructions{
    INST_NONE = 0,
    INST_EXIT,
    INST_HOLD,
    INST_HOLDS,
    INST_PLACE,
    INST_PENCIL,
    INST_MOVE,
    INST_ZOOM,
    INST_SYMSWAP,
    INST_COPY,
    INST_PASTE,
    INST_CHECK,
    INST_NEW,
    INST_NEWLAYER,
    INST_DELLAYER,
    INST_SWAPLAYERS,
    INST_LAYER,
    INST_SHOW,
    INST_PATTERN,
    INST_SAVE,
    INST_LOAD,
    INST_HELP,

    // for counting purposes
    INST_COUNT
};


int get_instruction(const char* what){
    if(cmp_str(what, "exit") || cmp_str(what, "quit") || cmp_str(what, "q") || cmp_str(what, "e"))
        return INST_EXIT;
    if(cmp_str(what, "hold"))                           return INST_HOLD       ;
    if(cmp_str(what, "holds"))                          return INST_HOLDS      ;
    if(cmp_str(what, "place"))                          return INST_PLACE      ;
    if(cmp_str(what, "pencil"))                         return INST_PENCIL     ;
    if(cmp_str(what, "move") || cmp_str(what, "mv"))    return INST_MOVE       ;
    if(cmp_str(what, "zoom"))                           return INST_ZOOM       ;
    if(cmp_str(what, "symswap"))                        return INST_SYMSWAP    ;
    if(cmp_str(what, "copy") || cmp_str(what, "cp"))    return INST_COPY       ;
    if(cmp_str(what, "paste"))                          return INST_PASTE      ;
    if(cmp_str(what, "check"))                          return INST_CHECK      ;
    if(cmp_str(what, "new"))                            return INST_NEW        ;
    if(cmp_str(what, "newlayer"))                       return INST_NEWLAYER   ;
    if(cmp_str(what, "dellayer"))                       return INST_DELLAYER   ;
    if(cmp_str(what, "swaplayers"))                     return INST_SWAPLAYERS ;
    if(cmp_str(what, "layer"))                          return INST_LAYER      ;
    if(cmp_str(what, "show"))                           return INST_SHOW       ;
    if(cmp_str(what, "pattern"))                        return INST_PATTERN    ;
    if(cmp_str(what, "save"))                           return INST_SAVE       ;
    if(cmp_str(what, "load"))                           return INST_LOAD       ;
    if(cmp_str(what, "help"))                           return INST_HELP       ;

    else return INST_NONE;
}


static inline void* align_buff_to(int n){
    if(n) buffsize += n - (buffsize % n);
    return (void*) &buff[buffsize];
}

int getascii_color_index(uint32_t color){

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
                putc(
                    (map[current_layer][i * mapw + j] < palette_len && map[current_layer][i * mapw + j] >= 0)?
                        (int) palette[map[current_layer][i * mapw + j]] : '~', output
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

static void render_tile_graphical(int tile, int x, int y, uint32_t* pixels, int pixelsw, int pixelsh, int pixels_stride){
    if(!pixels || !tileset || tile == 0) return;
    
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

    // if cameray < 0
    for(int i = cameray; i < 0; i+=1){
        for(int j = camerax; j < jrange; j+=1){
            render_tile_graphical(0, j * tileset_tilew, i * tileset_tileh, pixels, pixelsw, pixelsh, pixels_stride);
        }
    }
    // if camerax < 0
    for(int j = camerax; j < 0; j+=1){
        for(int i = cameray; i < irange; i+=1){
            render_tile_graphical(0, j * tileset_tilew, i * tileset_tileh, pixels, pixelsw, pixelsh, pixels_stride);
        }
    }
    if(draw_all_layers){
        for(int i = i0; i < irange; i+=1){
            for(int j = j0; j < jrange; j+=1){
                for(int k = 0; k < layers; k+=1){
                    render_tile_graphical(
                        map[k][i * mapw + j],
                        j * tileset_tilew, i * tileset_tileh,
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
                    j * tileset_tilew, i * tileset_tileh,
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
        printf("\x1B[2J\x1B[H\n");
        for(int i = 0; i < irange * tileset_tileh; i+=1){
            for(int j = 0; j < jrange * tileset_tilew; j+=1){
                fprintf(output, "%c", ascii_map[getascii_color_index(pixels[i * pixels_stride + j])]);
            }
            fprintf(output, "\n");
        }
    }


    free(pixels);
}

// \returns the size of the word
static int get_next_word(const char** output, const char* str){
    char c = *str;
    for(; c == ' ' || c == '\t' || c == '\n'; c = *(++str));

    if(*str == '\0'){
        return 0;
    }

    if(output) *output = str;
    
    int word_size = 0;
    for(c = str[++word_size]; c != '\0' && c != ' ' && c != '\t' && c != '\n' && c != '\\'; c = str[++word_size]);

    return word_size;
}


void help(int what){

    switch (what)
    {
    case INST_EXIT:
        printf("exit: quits the aplication, quit, e and q are equivalent to this\n");
        break;
    case INST_HOLD:
        printf("hold <tile>: holds given tile\n");
        break;
    case INST_HOLDS:
        printf("holds <symbol>: holds the tile with given symbol\n");
        break;
    case INST_PLACE:
        printf("place <x> <y>: places a square of held tiles at (x, y) with pencil's width and height (check pencil instruction)\n");
        break;
    case INST_PENCIL:
        printf("pencil <w> <h>: sets the width and height of the square for tile placement/copying\n");
        break;
    case INST_MOVE:
        printf("move <x> <y>: moves camera to (x, y), mv is equivalent to this\n");
        break;
    case INST_ZOOM:
        printf("zoom <w> <h>: sets the camera's width and height\n");
        break;
    case INST_SYMSWAP:
        printf("symswap <tile> <new>: sets the new symbol for displaing a tile, expects an uint number as the tile and a character as its new palette symbol\n");
        break;
    case INST_COPY:
        printf("copy <x> <y>: sets the position of the rect used to take a copy of a selection, width and height are given by pencil, cp is equivalent to this\n");
        break;
    case INST_PASTE:
        printf("paste <x> <x>: pastes a copied selection to (x, y), also check copy\n");
        break;
    case INST_CHECK:
        printf("check <x> <y>: does a complete check for tile at (x, y), expects 2 uint numbers, x and y coordinates to check\n");
        break;
    case INST_NEW:
        printf("new <w> <h>: creates a new map with (w, h) dimensions\n");
        break;
    case INST_NEWLAYER:
        printf("newlayer : creates a new layer\n");
        break;
    case INST_DELLAYER:
        printf("dellayer <optional: layers>...: deletes given layers, or current layer if none are passed, expects n uint arguments, the layers to delete, or nothing to delete current layer\n");
        break;
    case INST_SWAPLAYERS:
        printf("swaplayers <layer1> <layer2> : swaps layer1 and layer2, expects 2 uint arguments, the layers to swap\n");
        break;
    case INST_LAYER:
        printf("layer <layer>: selects given layer, expects 1 uint argument, the layer to go to\n");
        break;
    case INST_SHOW:
        printf(
            "show <optional: what>... -<optional: flags>...: shows parameters, map, tiles and configurations\n"
            "    use <what>:"
            "    map: to show the map's configuration and display map in single character mode\n"
            "    mapf: to display map with all layers, in the single character mode the number of interssections will be displayed per each tile\n"
            "    layer: to show layer's configuration\n"
            "    camera: to show the camera's configuration\n"
            "    pencil: to show the pencil's configuration\n"
            "    copy: to show the copy selection\n"
            "    tile: to show the held tile\n"
            "    tilesheet: to check the tile sheet configuration\n"
            "    tileset: to show all tile symbols available\n"
            "    mode: to show the draw mode, ascii sequence and output file path\n"
            "    -flags can be:\n"
            "        t <tile number>: displays tile with tile number\n"
            "        s <tile symbol>: displays tile with tile symbol\n"
            "        skip_question: skip questions about wether you want to draw things, etc\n"
            "        dont_skip_question: dont skip questions about wether you want to draw things, etc\n"
        );
        break;
    case INST_PATTERN:
        printf(
            "pattern <patterns>...: each pattern is a space sepparated word where each character or character sequence performs a function\n"
            "    -characters ands sequences are:\n"
            "        x<uint_number>: sets cursor x position\n"
            "        y<uint_number>: sets cursor y position\n"
            "        n<uint_number>: sets pencil's height position\n"
            "        m<uint_number>: sets pencil's width position\n"
            "        h<uint_number>: hold tile with given number\n"
            "        w: moves up\n"
            "        a: moves left\n"
            "        s: moves down\n"
            "        d: moves right\n"
            "        b: goes to previous layer (as in goes (b)ack)\n"
            "        f: goes to next layer (as in goes (f)orward)\n"
            "        p: place tiles\n"
            "        i: places tiles and moves up\n"
            "        j: places tiles and moves left\n"
            "        k: places tiles and moves down\n"
            "        l: places tiles and moves right\n"
            "    -number preffixes can be used to tell how many times that pattern should repeat\n"
            "        example: h12 10il wd x12 y13 n1 m1 3bpddsfn2m2h3p\n"

        );
        break;
    case INST_SAVE:
        printf(
            "save <optional: path>: saves the map\n"
            "\tyou can pass the path to the output save file, otherwise the map will be saved to the last loaded map or map.bin if no map was loaded\n"
            "\tyou can choose the .png extension to save the map as a png file, but for that the map needs to have less than 5 layers\n"
        );
        break;
    case INST_LOAD:
        printf("load <optional: path>: loads a map, you can pass the path to the input file, otherwise the map will be reloaded to the last loaded map\n");
        break;
    case INST_HELP:
        printf("help <optional: what>: displays this help message or, if provided, a help message about <what>\n");
        break;
    
    default:
        fprintf(stderr, "[ERROR] " __FILE__ ":%i:0: no help for instruction with id %i\n", __LINE__, what);
        break;
    }
}

#define PROMPT_EXPECT_ARGC(expected) if(((argc) - 1) != (expected)){\
            fprintf(stderr, "[ERROR] %s expects %i arguments, got %i instead\n", argv[0], (expected), (argc));\
            return 1;\
        }
#define GET_UINT(output, argv, index)\
        int output = parse_uint((argv)[(index)]);\
        if((output) < 0){\
            fprintf(stderr, "[ERROR] %s %ith argument '" #output "' expects uint, got '%s' instead\n", argv[0], (int) (index), (argv)[(index)]);\
            return 1;\
        }

int get_yes_or_no_asnwer(){
    int c = 0;
    for(c = fgetc(stdin); c == ' ' || c == '\t'; c = fgetc(stdin));
    const int answer = (c == 'y' || c == 'Y');
    for(c = fgetc(stdin); c != EOF && c != '\n'; c = fgetc(stdin));
    return answer;
}

int show(const char* what, int iwhat, int skip_questions){

    if(!what){
        printf("\x1B[2J\x1B[H\n");
        printf(
            "map: %s\n"
            "map size: (%i, %i)\n"
            "layer: %i / %i\n"
            "camera: (%i, %i, %i, %i)\n"
            "pencil: (%i, %i)\n"
            "copy selection: (%i, %i, %i, %i)\n"
            "held tile: %i -> %c\n"
            "cursor: (%i, %i)\n"
            "output: %s\n"
            "tilesheet = %s\n"
            "tilew = %i, tileh = %i\n"
            "ascii: %s\n"
            "draw mode: %s\n",
            map_path,
            mapw, maph,
            current_layer, layers - 1,
            camerax, cameray, cameraw, camerah,
            pencilw, pencilh,
            copyx, copyy, pencilw, pencilh,
            held_tile, (held_tile >= 0 && held_tile < palette_len)? palette[held_tile] : '~',
            cursorx, cursory,
            (output == stdout)? "stdout" : output_path,
            tileset_path,
            tileset_tilew, tileset_tileh,
            ascii_map,
            (display == render_graphical)? "graphics" : "character"
        );
        return 0;
    }

    if(what[0] == '-'){
        if(what[1] == '\0') return 1;
        if(what[2] != '\0') return 1;
        switch (what[1])
        {
        case 's':{
            int symfound = 0;
            for(int i = 0; i < palette_len; i+=1){
                if(palette[i] == iwhat){
                    iwhat = i;
                    symfound = 1;
                    break;
                }
            }
            if(!symfound){
                printf("no tile with symbol '%c'\n", (char) iwhat);
                return 0;
            }
        }
        case 't':
            printf(
                "tile %i:\n"
                "\tsymbol: %s%c\n",
                iwhat,
                (iwhat >= 0 && iwhat < palette_len)? "" : "no symbol",
                (iwhat >= 0 && iwhat < palette_len)? palette[iwhat] : ' '
            );
            if(tileset){
                if(iwhat <= 0) return 0;
                const int tilex = ((iwhat - 1) % (tilesetx / tileset_tilew));
                const int tiley = ((iwhat - 1) / (tilesetx / tileset_tilew));
                if(tilex >= (tilesetx / tileset_tilew) || tiley >= (tilesety / tileset_tileh)){
                    printf("\tthe tile is not in the tilesheet\n");
                    return 0;
                }
                printf("\tposition in tilesheet (y, x) = (%i, %i)\n", tiley, tilex);
                int draw_tile = 1;
                if(!skip_questions){
                    printf("do you wish to draw the tile[y/n]?\n");
                    draw_tile = get_yes_or_no_asnwer();
                }
                if(draw_tile){
                    const int toffset = tiley * tileset_tileh * tilesetx + tilex * tileset_tilew;
                    for(int i = 0; i < tileset_tileh; i+=1){
                        for(int j = 0; j < tileset_tilew; j+=1){
                            uint32_t color = 0;
                            for(int channel = 0; channel < tileset_comp; channel+=1){
                                color |= (tileset[(toffset + i * tilesetx + j) * tileset_comp + channel]) << (channel * 8);
                            }
                            printf("%c", ascii_map[getascii_color_index(color)]);
                        }
                        printf("\n");
                    }
                }
            }
            return 0;
        
        default:
            return 1;
        }
    }

    if(cmp_str(what, "map")){
        if(output != stdout) display(0);
        FILE* const output_temp = output;
        output = stdout;
        print_map(0);
        output = output_temp;
        printf("map: %s    mapw = %i    maph = %i\n", map_path, mapw, maph);
        return 0;
    }
    if(cmp_str(what, "mapf")){
        if(output != stdout) display(1);
        FILE* const output_temp = output;
        output = stdout;
        print_map(1);
        output = output_temp;
        printf("map: %s    mapw = %i    maph = %i\n", map_path, mapw, maph);
        return 0;
    }
    if(cmp_str(what, "layer")){
        printf("layer: %i / %i\n", current_layer, layers - 1);
        return 0;
    }
    if(cmp_str(what, "camera")){
        printf("camerax = %i   cameray = %i\ncameraw = %i   camerah = %i\n", camerax, cameray, cameraw, camerah);
        return 0;
    }
    if(cmp_str(what, "pencil")){
        printf("pencilw = %i   pencilh = %i\n", pencilw, pencilh);
        return 0;
    }
    if(cmp_str(what, "copy")){
        const int camerax_temp = camerax;
        const int cameray_temp = cameray;
        const int cameraw_temp = cameraw;
        const int camerah_temp = camerah;
        camerax = copyx;
        cameray = copyy;
        cameraw = pencilw;
        camerah = pencilh;
        print_map(0);

        printf("copyx = %i   copyy = %i\ncopyw = %i   copyh = %i\n", copyx, copyy, pencilw, pencilh);

        camerax = camerax_temp;
        cameray = cameray_temp;
        cameraw = cameraw_temp;
        camerah = camerah_temp;
        return 0;
    }
    if(cmp_str(what, "cursor")){
        printf("cursorx = %i   cursory = %i\n", cursorx, cursory);
        return 0;
    }
    if(cmp_str(what, "mode") || cmp_str(what, "output") || cmp_str(what, "ascii")){
        printf(
            "output: %s\n"
            "ascii: %s\n"
            "draw mode: %s\n",
            output_path? output_path : "stdout",
            ascii_map,
            (display == render_graphical)? "graphical" : "character"
        );
        return 0;
    }
    if(cmp_str(what, "tilesheet")){
        printf(
            "tileset: %s\n"
            "    tilesetw = %i, tileseth =%i\n"
            "    tilew = %i, tileh = %i\n"
            "    tile count x: %i, tile count y: %i\n",
            tileset_path,
            tilesetx, tilesety,
            tileset_tilew, tileset_tileh,
            tilesetx / tileset_tilew, tilesetx / tileset_tileh
        );
        int draw_tileset = 0;
        if(!skip_questions &&  tileset != NULL){
            printf("do you wish to draw the tilesheet[y/n]?\n");
            draw_tileset = get_yes_or_no_asnwer();
        }
        if(draw_tileset){
            const int tileset_tilexcount = tilesetx / tileset_tilew;
            const int tileset_tileycount = tilesety / tileset_tileh;
            for(int tiley = 0; tiley < tileset_tileycount; tiley+=1){
                for(int i = 0; i < tileset_tileh; i+=1){
                    for(int tilex = 0; tilex < tileset_tilexcount; tilex+=1){
                        const int toffset = tiley * tileset_tileh * tilesetx + tilex * tileset_tilew;
                        for(int j = 0; j < tileset_tilew; j+=1){
                            uint32_t color = 0;
                            for(int channel = 0; channel < tileset_comp; channel+=1){
                                color = (color << 8) | (tileset[(toffset + i * tilesetx + j) * tileset_comp + channel]);
                            }
                            putchar((int) ascii_map[getascii_color_index(color)]);
                        }
                    }
                    putchar(' ');
                }
                putchar('\n');
            }
            putchar('\n');
        }
        return 0;
    }
    if(cmp_str(what, "tile") || cmp_str(what, "tileset")){
        printf("held tile %i -> %c\n", held_tile, (held_tile >= 0 && held_tile < palette_len)? palette[held_tile] : ' ');
        int draw_tileset = 0;
        if(!skip_questions){
            printf("do you wish to draw the tileset[y/n]?\n");
            draw_tileset = get_yes_or_no_asnwer();
        }
        if(draw_tileset){
            printf("  ");
            for(int j = 0; j < 10; j += 1){
                printf(" %i ", j);
            }
            printf("\n  ");
            for(int j = 0; j < 10; j += 1){
                printf(" | ");
            }
            putchar('\n');
            for(int i = 0; i < palette_len; i+=1){
                putchar('\n');
                printf("%i-", i / 10);
                for(int j = 0; j < 10 && i < palette_len; j += 1){
                    printf(" %c ", palette[i++]);
                }
                putchar('\n');
            }
        }
        return 0;
    }

    return 1;
}

int parse_rep_in_pattern_word(const char* pattern, int* len){
    if(pattern[0] < '0' || pattern[0] > '9'){
        if(len) *len = 0;
        return -1;
    }
    int output = 0;
    int i = 0;
    for(; pattern[i] >= '0' && pattern[i] <= '9'; i+=1){
        output = (output * 10) + (pattern[i] - '0');
    }
    if(len) *len = i;
    return output;
}

int perform_pattern_word(const char* pattern_word){

    int ilen = 0;
    int rep = parse_rep_in_pattern_word(pattern_word, &ilen);
    if(rep < 0) rep = 1;

    int number = -1;

    while(rep-- > 0){

        int nlen = 0;

        for(int i = ilen; pattern_word[i]; i += nlen + 1){

            nlen = 0;

            switch (pattern_word[i])
            {
            case 'x':
                number = parse_rep_in_pattern_word(pattern_word + i + 1, &nlen);
                if(number < 0){
                    fprintf(
                        stderr,
                        "[ERROR] in pattern word '%s' at position %i: expected valid number after 'x', got '%c' instead\n",
                        pattern_word, i, pattern_word[i + 1]
                    );
                    return 1;
                }
                cursorx = number;
                break;
            case 'y':
                number = parse_rep_in_pattern_word(pattern_word + i + 1, &nlen);
                if(number < 0){
                    fprintf(
                        stderr,
                        "[ERROR] in pattern word '%s' at position %i: expected valid number after 'y', got '%c' instead\n",
                        pattern_word, i, pattern_word[i + 1]
                    );
                    return 1;
                }
                cursory = number;
                break;
            case 'n':
                number = parse_rep_in_pattern_word(pattern_word + i + 1, &nlen);
                if(number < 0){
                    fprintf(
                        stderr,
                        "[ERROR] in pattern word '%s' at position %i: expected valid number after 'n', got '%c' instead\n",
                        pattern_word, i, pattern_word[i + 1]
                    );
                    return 1;
                }
                pencilh = number;
                break;
            case 'm':
                number = parse_rep_in_pattern_word(pattern_word + i + 1, &nlen);
                if(number < 0){
                    fprintf(
                        stderr,
                        "[ERROR] in pattern word '%s' at position %i: expected valid number after 'm', got '%c' instead\n",
                        pattern_word, i, pattern_word[i + 1]
                    );
                    return 1;
                }
                pencilw = number;
                break;
            case 'h':
                number = parse_rep_in_pattern_word(pattern_word + i + 1, &nlen);
                if(number < 0){
                    fprintf(
                        stderr,
                        "[ERROR] in pattern word '%s' at position %i: expected valid number after 'h', got '%c' instead\n",
                        pattern_word, i, pattern_word[i + 1]
                    );
                    return 1;
                }
                held_tile = number;
                break;
            case 'i':
                place(held_tile, cursorx, cursory);
            case 'w':
                cursory -= 1;
                break;
            case 'j':
                place(held_tile, cursorx, cursory);
            case 'a':
                cursorx -= 1;
                break;
            case 'k':
                place(held_tile, cursorx, cursory);
            case 's':
                cursory += 1;
                break;
            case 'l':
                place(held_tile, cursorx, cursory);
            case 'd':
                cursorx += 1;
                break;
            case 'b':
                if(current_layer > 0) current_layer -= 1;
                else current_layer = layers - 1;
                break;
            case 'f':
                current_layer = (current_layer + 1) % layers;
                break;
            case 'p':
                place(held_tile, cursorx, cursory);
                break;
            
            default:
                fprintf(stderr, "[ERROR] in pattern word '%s': unknown character for pattern '%c'\n", pattern_word, pattern_word[i]);
                return 1;
            }
        }

    }

    return 0;
}

int handle_prompt(int argc, const char** argv){

    if(argc < 1){
        if(output == stdout)
            display(0);
        else
            printf("\x1B[2J\x1B[H\n");
        return 0;
    }
    const int inst = get_instruction(argv[0]);
    switch (inst)
    {
    case INST_EXIT:
        active = 0;
        return 0;
    case INST_HOLD:{
        PROMPT_EXPECT_ARGC(1);
        GET_UINT(tile, argv, 1);
        held_tile = tile;
    }
        return 0;
    case INST_HOLDS:{
        PROMPT_EXPECT_ARGC(1);
        int slen = 0;
        for(; argv[1][slen] && slen < 2; slen+=1);
        if(slen > 1){
            printf("holds expects single character symbol, got '%s' instead\n", argv[1]);
            return 1;
        }
        int tile = -1;
        for(int i = 0; i < palette_len; i += 1){
            if(argv[1][0] == palette[i]){
                tile = i;
                break;
            }
        }
        if(tile < 0){
            fprintf(stderr, "[ERROR] no tile has symbol %c\n", argv[1][0]);
            return 1;
        }
        held_tile = tile;
    }
        return 0;
    case INST_PLACE:{
        PROMPT_EXPECT_ARGC(2);
        GET_UINT(x, argv, 1);
        GET_UINT(y, argv, 2);
        if(place(held_tile, x, y)){
            fprintf(stderr, "[ERROR] failed to place tile %i to (%i, %i)\n", held_tile, x, y);
            return 1;
        }
        display(0);
    }
        return 0;
    case INST_PENCIL:{
        PROMPT_EXPECT_ARGC(2);
        GET_UINT(w   , argv, 1);
        GET_UINT(h   , argv, 2);
        if(w==0){
            fprintf(stderr, "[ERROR] w==0\n");
            return 1;
        }
        if(h==0){
            fprintf(stderr, "[ERROR] h==0\n");
            return 1;
        }
        pencilw = w;
        pencilh = h;
    }
        return 0;
    case INST_MOVE:{
        PROMPT_EXPECT_ARGC(2);
        GET_UINT(x, argv, 1);
        GET_UINT(y, argv, 2);
        if(x >= mapw){
            fprintf(stderr, "[ERROR] x out of bounds\n");
            return 1;
        }
        if(y >= maph){
            fprintf(stderr, "[ERROR] y out of bound\n");
            return 1;
        }
        camerax = x - cameraw / 2;
        cameray = y - camerah / 2;
        display(0);
    }
        return 0;
    case INST_ZOOM:{
        PROMPT_EXPECT_ARGC(2);
        GET_UINT(w, argv, 1);
        GET_UINT(h, argv, 2);
        if(w==0){
            fprintf(stderr, "[ERROR] w==0\n");
            return 1;
        }
        if(h==0){
            fprintf(stderr, "[ERROR] h==0\n");
            return 1;
        }
        cameraw = w;
        camerah = h;
        display(0);
    }
        return 0;
    case INST_SYMSWAP:{
        PROMPT_EXPECT_ARGC(2);
        GET_UINT(tile_number, argv, 1);
        if(tile_number < 0 || tile_number >= palette_len){
            printf("no tile for %i\n", tile_number);
            return 1;
        }
        if(Mstrlen(argv[2]) != 1){
            fprintf(stderr, "[ERROR] palette symbol should be one character long\n");
            return 1;
        }
        const char nc = argv[2][0];
        for(int i = 0; i < palette_len; i += 1){
            if(palette[i] == nc && i != tile_number){
                fprintf(stderr, "[ERROR] '%c' is already used as palette symbol for tile %i\n", nc, i);
                return 1;
            }
        }
        palette[tile_number] = nc;
        display(0);
    }
        return 0;
    case INST_COPY:{
        PROMPT_EXPECT_ARGC(2);
        GET_UINT(x, argv, 1);
        GET_UINT(y, argv, 2);
        copyx = x;
        copyy = y;
    }
        return 0;
    case INST_PASTE:{
        PROMPT_EXPECT_ARGC(2);
        GET_UINT(destx, argv, 1);
        GET_UINT(desty, argv, 2);
        if(paste(destx, desty)){
            fprintf(stderr, "[ERROR] failed to paste to (%i, %i)\n", destx, desty);
            return 1;
        }
        display(0);
    }
        return 0;
    case INST_CHECK:{
        PROMPT_EXPECT_ARGC(2);
        GET_UINT(x, argv, 1);
        GET_UINT(y, argv, 2);
        if(x >= mapw || y >= maph){
            fprintf(stderr, "[ERROR] (%i, %i) point is out of bounds (%i, %i)\n", x, y, mapw, maph);
            return 1;
        }
        printf("at (%i, %i):\n", x, y);
        int tile_count = 0;
        for(int k = 0; k < layers; k+=1){
            const int tile = (int) map[k][y * mapw + x];
            if(tile){
                tile_count += 1;
                printf("\ttile %i with symbol %c at layer %i\n", tile, (tile >= 0 && tile < palette_len)? palette[tile] : '\0', k);
            }
        }
        printf("\t%i tiles at (%i, %i)\n", tile_count, x, y);
    }
        return 0;
    case INST_NEW:{
        PROMPT_EXPECT_ARGC(2);
        GET_UINT(w, argv, 1);
        GET_UINT(h, argv, 2);
        if(w == 0 || h == 0){
            fprintf(stderr, "[ERROR] can't have zeroed dimension(s)\n");
            return 1;
        }
        char** nmap = malloc(layers * sizeof(nmap[0]));
        const int wmin = (w < mapw)? w : mapw;
        const int hmin = (h < maph)? h : maph;

        for(int k = 0; k < layers; k += 1){
            nmap[k] = malloc(w * h);
            for(int i = 0; i < hmin; i+=1){
                for(int j = 0; j < wmin; j += 1){
                    nmap[k][i * w + j] = map[k][i * mapw + j];
                }
            }
        }
        for(int i = hmin; i < h; i+=1){
            for(int j = wmin; j < w; j += 1){
                for(int k = 0; k < layers; k += 1)
                    nmap[k][i * w + j] = 0;
            }
        }

        if(map){
            for(int k = 0; k < layers; k+=1) free(map[k]);
            free(map);
        }
        map = nmap;
        mapw = w;
        maph = h;
        display(0);
    }
        return 0;
    case INST_NEWLAYER:{
        char** nmap = malloc((layers + 1) * sizeof(nmap[0]));
        if(!nmap){
            fprintf(stderr, "[ERROR] buy more RAM...\n");
            return 1;
        }
        for(int i = 0; i < current_layer; i+=1) nmap[i] = map[i];
        nmap[current_layer] = malloc(mapw * maph * sizeof(map[0][0]));
        for(int i = 0; i < mapw * maph; i+=1) nmap[current_layer][i] = 0;
        for(int i = current_layer; i < layers; i+=1) nmap[i + 1] = map[i];
        layers += 1;
        free(map);
        map = nmap;
        display(0);
    }
        return 0;
    case INST_DELLAYER:{
        if(layers == 0){
            printf("no layers to delete\n");
            return 0;
        }
        int err = 0;
        if(argc == 1){
            for(int i = current_layer; i < layers - 1; i+=1){
                map[i] = map[i + 1];
            }
            layers -= 1;
        }
        else{
            for(int i = 1; i < argc; i+=1){
                const int layer = parse_uint(argv[i]);
                if(layer < 0){
                    err = 1;
                    fprintf(stderr, "[ERROR] invalid layer '%s'\n", argv[i]);
                    continue;
                }
                if(layer >= layers){
                    err = 1;
                    fprintf(stderr, "[ERROR] will not delete layer %i as it does not exist, layers go up to %i\n", layer, layers);
                    continue;
                }
                free(map[layer]);
                map[layer] = NULL;
            }
            for(int k = 0; k < layers; k+=1){
                if(map[k] == NULL){
                    for(int i = k; i < layers - 1; i+=1){
                        map[i] = map[i + 1];
                    }
                    layers -= 1;
                }
            }
        }
        if(layers <= 0){
            layers = 1;
            map[0] = malloc(mapw * maph * sizeof(map[0][0]));
            for(int i = 0; i < mapw * maph; i+=1) map[0][i] = 0;
        }
        if(current_layer >= layers) current_layer = layers - 1;
        if(err == 0) display(0);
    }
        return 0;
    case INST_SWAPLAYERS:{
        PROMPT_EXPECT_ARGC(2);
        GET_UINT(first, argv, 1);
        GET_UINT(second, argv, 2);
        if(first >= layers || second >= layers){
            fprintf(stderr, "[ERROR] layers only go up to %i, got %i and %i\n", layers - 1, first, second);
            return 1;
        }
        char* const first_placeholder = map[first];
        map[first] = map[second];
        map[second] = first_placeholder;
        display(0);
    }
        return 0;
    case INST_LAYER:{
        PROMPT_EXPECT_ARGC(1);
        GET_UINT(layer, argv, 1);
        if(layer >= layers){
            fprintf(stderr, "[ERROR] layers only go up to %i, got %i\n", layers - 1, layer);
            return 1;
        }
        current_layer = layer;
        display(0);
    }
        return 0;
    case INST_SHOW:{
        if(argc == 1) show(NULL, -1, 1);
        int skip_questions = 0;
        for(int i = 1; i < argc; i+=1){
            if(argv[i][0] == '-'){
                if(cmp_str(argv[i], "-skip_questions")){
                    skip_questions = 1;
                    continue;
                }
                if(cmp_str(argv[i], "-dont_skip_questions")){
                    skip_questions = 0;
                    continue;
                }
                if(i + 1 >= argc){
                    fprintf(stderr, "[ERROR] expected uint number after '%s'\n", argv[i]);
                    return 1;
                }
                const int iwhat = parse_uint(argv[++i]);
                if(iwhat < 0){
                    fprintf(stderr, "[ERROR] invalid uint number '%s' after %s\n", argv[i], argv[i - 1]);
                    return 1;
                }
                if(show(argv[i - 1], iwhat, skip_questions)){
                    fprintf(stderr, "[ERROR] %s failed\n", argv[0]);
                    return 1;
                }
                continue;
            }
            if(show(argv[i], 0, skip_questions)){
                fprintf(stderr, "[ERROR] %s has no support for %s\n", argv[0], argv[i]);
                return 1;
            }
        }
    }
        return 0;
    case INST_PATTERN:{
        if(argc == 1){
            fprintf(stderr, "[ERROR] missing pattern\n");
            return 1;
        }
        for(int i = 1; i < argc; i+=1){
            if(perform_pattern_word(argv[i])){
                fprintf(stderr, "[ERROR] error while performing word %i of pattern: '%s'\n", i, argv[i]);
                return 1;
            }
        }
        display(0);
    }
        return 0;
    case INST_SAVE:{
        if(argc > 2){
            fprintf(stderr, "[ERROR] save expects up to 1 argument (output path), got %i instead\n", argc - 1);
            return 1;
        }
        if(save_map((argc > 1)? argv[1] : map_path)){
            fprintf(stderr, "[ERROR] Could not save to '%s'\n", (argc > 1)? argv[1] : map_path);
            return 1;
        }
    }
        return 0;
    case INST_LOAD:{
        if(argc > 2){
            fprintf(stderr, "[ERROR] load expects up to 1 argument (input path), got %i instead\n", argc - 1);
            return 1;
        }
        if(load_map((argc == 2)? argv[1] : map_path)){
            fprintf(stderr, "[ERROR] Could not load '%s'\n", (argc == 2)? argv[1] : map_path);
            return 1;
        }
        display(0);
    }
        return 0;
    case INST_HELP:
        printf("\x1B[2J\x1B[H\n");
        if(argc > 1){
            for(int i = 1; i < argc; i+=1){
                const int what = get_instruction(argv[i]);
                if(what == INST_NONE){
                    fprintf(stderr, "[ERROR] No instruction '%s'\n", argv[i]);
                    return 1;
                }
                help(what);
            }
        }
        else{
            for(int i = 0; i < INST_NONE; i+=1) help(i);
            for(int i = INST_NONE + 1; i < INST_COUNT; i+=1) help(i);
        }
        return 0;
    default:
        fprintf(stderr, "[ERROR] '%s", argv[0]);
        fputc('\'', stderr);
        fputc(' ', stderr);
        fprintf(stderr, "Unknown instruction '%s'\n", argv[0]);
        return 1;
    }

    return 0;
}

static int get_user_prompt(const char*** _argv){
    char* const inbuff = &buff[buffsize];
    const int inbuff_start = buffsize;
    int c = fgetc(stdin);

    for(; c && c != '\n' && c != EOF; c = fgetc(stdin)){
        buff[buffsize++] = c;
    }
    buff[buffsize++] = '\0';
    buff[buffsize++] = '\0';

    int argc = 0;
    char** argv = align_buff_to(sizeof(*argv));

    int word_size = get_next_word((const char**) &argv[argc], &buff[inbuff_start]);

    for(int i = inbuff_start + word_size + 1; word_size; i += word_size + 1){
        argv[argc][word_size] = '\0';
        buffsize += sizeof(*argv);
        word_size = get_next_word((const char**) &argv[++argc], &buff[i]);
    }

    if(_argv) *_argv = (const char**) argv;

    return (c == EOF)? -1 : argc;
}

int main(int argc, char** argv){

    #define MAIN_RETURN_STATUS(STATUS) do {err = STATUS; goto defer;} while(0)
    
    int err = 0;

    int map_pathi = 0;

    for(int i = 1; i < argc; i += 1){
        if(cmp_str(argv[i], "--help")){
            printf(
                "a handy console map designer for creating and editing maps in console\n"
                "usage: %s <optional: map_to_load> -<flags> --<kwargs>\n"
                "flags are:\n"
                "\to <output>: displays into output, if output has .png extension a graphical display will be forced and as such a tileset will be required\n"
                "\tw <map width>: sets the map width\n"
                "\th <map_height>: sets the map height\n"
                "\tl <layers>: sets the number of layers\n"
                "\tsingle_character_graphics: displays map with tiles represented by single characters"
                "\tcommon_graphics: display map with tiles represented by graphical form (tilesheet required)\n"
                "\ttw: sets the tileset's tile width\n"
                "\tth: sets the tileset's tile height"
                "keyword arguments are:\n"
                "\tpalette <symbol sequence>: sets the tile's symbol palette for display\n"
                "\ttilesheet <tilesheet_path>: loads the tilesheet that'll get used to graphically draw the map, provide the tilesheet's tile width and height before using this\n"
                "\tascii <character_sequence>: sets the character sequence to use as ascii colors, form drakest to brightest\n"
                "\tcamera <x> <y> <w> <h>: positions the camera to (x, y) with with=w and height=h\n"
                "\thelp: displays this help message\n",
                argv[0]
            );
            MAIN_RETURN_STATUS(0);
        }
        else if(cmp_str(argv[i], "--camera")){
            const char* const kwarg_name = argv[i];
            if(i + 4 >= argc){
                fprintf(stderr, "[ERROR] %s expects 4 arguments, x, y, w and h\n", kwarg_name);
                MAIN_RETURN_STATUS(1);
            }
            camerax = parse_uint(argv[++i]);
            if(camerax < 0){
                fprintf(stderr, "[ERROR] in argument %i, expected valid x position for %s, got '%s' instead\n", i, kwarg_name, argv[i]);
                MAIN_RETURN_STATUS(1);
            }
            cameray = parse_uint(argv[++i]);
            if(cameray < 0){
                fprintf(stderr, "[ERROR] in argument %i, expected valid y position for %s, got '%s' instead\n", i, kwarg_name, argv[i]);
                MAIN_RETURN_STATUS(1);
            }
            cameraw = parse_uint(argv[++i]);
            if(cameraw <= 0){
                fprintf(stderr, "[ERROR] in argument %i, expected valid width for %s, got '%s' instead\n", i, kwarg_name, argv[i]);
                MAIN_RETURN_STATUS(1);
            }
            camerah = parse_uint(argv[++i]);
            if(camerah <= 0){
                fprintf(stderr, "[ERROR] in argument %i, expected valid height for %s, got '%s' instead\n", i, kwarg_name, argv[i]);
                MAIN_RETURN_STATUS(1);
            }
        }
        else if(cmp_str(argv[i], "--ascii")){
            if(++i >= argc){
                fprintf(stderr, "[ERROR] expected character_sequence path after '--ascii'\n");
                MAIN_RETURN_STATUS(1);
            }
            for(ascii_len = 0; argv[i][ascii_len]; ascii_len += 1);
            if(ascii_len < 2){
                fprintf(stderr, "[ERROR] ascii map must containg at least 2 character\n");
                MAIN_RETURN_STATUS(1);
            }
            ascii_map = argv[i];
        }
        else if(cmp_str(argv[i], "--tilesheet")){
            if(i >= argc){
                fprintf(stderr, "[ERROR] expected tileset path after '%s'\n", argv[i]);
                MAIN_RETURN_STATUS(1);
            }
            if(tileset_tilew <= 0 || tileset_tileh <= 0){
                fprintf(stderr, "[ERROR] provide both the tilesheet's tile width and height before using %s\n", argv[i]);
                MAIN_RETURN_STATUS(1);
            }
            tileset = stbi_load(argv[++i], &tilesetx, &tilesety, &tileset_comp, 0);
            if(!tileset){
                fprintf(stderr, "[ERROR] could not load tileset '%s'\n", argv[i]);
                MAIN_RETURN_STATUS(1);
            }
            tileset_path = argv[i];
        }
        else if(cmp_str(argv[i], "--palette")){
            if(++i >= argc){
                fprintf(stderr, "[ERROR] expected palette after '--palette'\n");
                MAIN_RETURN_STATUS(1);
            }
            for(palette_len = 0; argv[i][palette_len]; palette_len += 1);
            palette = argv[i];
        }
        else if(cmp_str(argv[i], "-o")){
            if(i + 1 >= argc){
                fprintf(stderr, "[ERROR] expected output path after '-o'\n");
                MAIN_RETURN_STATUS(1);
            }
            output_path = argv[++i];
            output = fopen(output_path, "w");
            if(!output){
                fprintf(stderr, "[ERROR] could not open output '%s'\n", output_path);
                MAIN_RETURN_STATUS(1);
            }
            if(is_png_extension(output_path)){
                if(!tileset){
                    fprintf(stderr, "[ERROR] provide a tileset to output to .png file\n");
                    MAIN_RETURN_STATUS(1);
                }
                display = render_graphical;
                fclose(output);
                output = NULL;
            }
        }
        else if(cmp_str(argv[i], "-w")){
            if(i + 1 >= argc){
                fprintf(stderr, "[ERROR] expected map width after '-w'\n");
                MAIN_RETURN_STATUS(1);
            }
            mapw = parse_uint(argv[++i]);
            if(mapw <= 0){
                fprintf(stderr, "[ERROR] invalid map width '%s'\n", argv[i]);
                MAIN_RETURN_STATUS(1);
            }
        }
        else if(cmp_str(argv[i], "-h")){
            if(i + 1 >= argc){
                fprintf(stderr, "[ERROR] expected map height after '-h'\n");
                MAIN_RETURN_STATUS(1);
            }
            maph = parse_uint(argv[++i]);
            if(maph <= 0){
                fprintf(stderr, "[ERROR] invalid map height '%s'\n", argv[i]);
                MAIN_RETURN_STATUS(1);
            }
        }
        else if(cmp_str(argv[i], "-l")){
            if(i + 1 >= argc){
                fprintf(stderr, "[ERROR] expected layer count after '-l'\n");
                MAIN_RETURN_STATUS(1);
            }
            layers = parse_uint(argv[++i]);
            if(layers <= 0){
                fprintf(stderr, "[ERROR] invalid layer count '%s'\n", argv[i]);
                MAIN_RETURN_STATUS(1);
            }
        }
        else if(cmp_str(argv[i], "-common_graphics")){
            display = render_graphical;
        }
        else if(cmp_str(argv[i], "-single_character_graphics")){
            display = print_map;
        }
        else if(cmp_str(argv[i], "-tw")){
            if(i + 1 >= argc){
                fprintf(stderr, "[ERROR] expected tile width after '-tw'\n");
                MAIN_RETURN_STATUS(1);
            }
            tileset_tilew = parse_uint(argv[++i]);
            if(tileset_tilew <= 0){
                fprintf(stderr, "[ERROR] invalid tile width '%s'\n", argv[i]);
            }
        }
        else if(cmp_str(argv[i], "-th")){
            if(i + 1 >= argc){
                fprintf(stderr, "[ERROR] expected tile height after '-th'\n");
                MAIN_RETURN_STATUS(1);
            }
            tileset_tileh = parse_uint(argv[++i]);
            if(tileset_tileh <= 0){
                fprintf(stderr, "[ERROR] invalid tile height '%s'\n", argv[i]);
            }
        }
        else{
            if(map_pathi){
                fprintf(
                    stderr,
                    "[ERROR] multiple input files given, '%s' and '%s', in arguments %i and %i\n",
                    argv[map_pathi], argv[i],
                    map_pathi, i
                );
                MAIN_RETURN_STATUS(1);
            }
            map_pathi = i;
        }
    }

    if(layers <= 0) layers = 1;
    if(mapw <= 0) mapw = (maph > 0)? maph : 10;
    if(maph <= 0) maph = (mapw > 0)? mapw : 10;

    if(map_pathi > 0){
        FILE* const dummy = fopen(argv[map_pathi], "r");
        if(dummy) fclose(dummy);
        if(dummy == NULL){
            if(!map){
                map = malloc(layers * sizeof(map[0]));
                for(int k = 0; k < layers; k+=1){
                    map[k] = malloc(mapw * maph * sizeof(map[0][0]));
                    for(int i = 0; i < maph; i+=1){
                        for(int j = 0; j < mapw; j+=1){
                            map[k][i * mapw + j] = 0;
                        }
                    }
                }
            }
            if(save_map(argv[map_pathi])){
                fprintf(stderr, "[ERROR] Could not make map '%s'\n", argv[map_pathi]);
                MAIN_RETURN_STATUS(1);
            }
        }
        else if(load_map(argv[map_pathi])){
            fprintf(stderr, "[ERROR] Could not load map '%s'\n", argv[map_pathi]);
            MAIN_RETURN_STATUS(1);
        }
    }
    
    if(!map){
        map = malloc(layers * sizeof(map[0]));
        for(int k = 0; k < layers; k+=1){
            map[k] = malloc(mapw * maph * sizeof(map[0][0]));
            for(int i = 0; i < maph; i+=1){
                for(int j = 0; j < mapw; j+=1){
                    map[k][i * mapw + j] = 0;
                }
            }
        }
    }

    if(output == NULL && output_path == NULL) output = stdout;
    if(display == NULL){
        if(tileset){
            display = render_graphical;
        }
        else{
            display = print_map;
        }
    }

    active = 1;

    display(0);

    while (active)
    {
        const int    buff_scope = buffsize;
        const char** prompt_argv = NULL;
        printf(">>> ");
        const int prompt_argc = get_user_prompt(&prompt_argv);
        if(prompt_argc < 0) break;
        if(handle_prompt(prompt_argc, prompt_argv)){
            fprintf(stderr, "for a help message enter help <optional: command_name>\n");
        }
        buffsize = buff_scope;
    }
    

    putchar('\n');

    defer:
    if(map){
        for(int k = 0; k < layers; k+=1) free(map[k]);
        free(map);
    }
    if(map_path){
        free(map_path);
    }
    if(tileset){
        free(tileset);
    }
    if(output && output != stdout) fclose(output);

    return err;
}


