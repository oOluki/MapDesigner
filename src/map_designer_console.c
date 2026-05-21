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
#include "instructions.h"
#include <stdio.h>



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

	printf("\n-");
    switch (what)
    {
    case INST_EXIT:
        printf("exit: quits the aplication, quit, e and q are equivalent to this\n");
        break;
    case INST_DISPLAY:
		printf("display: displays map\n");
		break;
    case INST_HOLD:
        printf("hold <tile>: holds given tile\n");
        break;
    case INST_HOLDS:
        printf("holds <symbol>: holds the tile with given symbol\n");
        break;
    case INST_MAPTILE:
        printf(
            "maptile <tile key> <tile value>: maps <tile key> to <tile value>, can only map tiles up to %i key. "
            "If no <tile value> is provided then the <tile key> will be unmapped\n",
            (int) (sizeof(tile_mapping) / sizeof(tile_mapping[0]) - 1)
        );
        break;
    case INST_PLACE:
        printf("place <x> <y>: fills a square of held tiles at (x, y) with pencil's width and height (check pencil instruction)\n");
        break;
    case INST_PLACEHOLLOW:
        printf("hollow <x> <y>: places a hollow square of held tiles at (x, y) with pencil's width and height (check pencil instruction)\n");
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
    case INST_MERGELAYERS:
        printf("merge <layer 1> <layer 2>: merge layer 1 to layer 2, preserving layer 1\n");
        break;
    case INST_LAYER:
        printf("layer <layer>: selects given layer, expects 1 uint argument, the layer to go to\n");
        break;
    case INST_SHOW:
        printf(
            "show <optional: what>... -<optional: flags>...: shows parameters, map, tiles and configurations\n"
            "    use <what>:\n"
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
            "    tile_mapping: to show the mapped tiles configuration\n"
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
            "        n<uint_number>: sets pencil's height\n"
            "        m<uint_number>: sets pencil's width\n"
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
    case INST_QUERY:
        printf(
            "query <queried tile> <optional: new tile number> <optional: -q>: searches for <queried tile>, if <new tile number> is provided <queried tile> will be replaced by it\n"
            "\tthe -q flag sets query mode, where the program will stop at each query hit, it is also usefull for searches\n"
        );
        break;
    case INST_HELP:
        printf("help <optional: what>: displays this help message or, if provided, a help message about <what>\n");
        break;
    
    default:
        fprintf(stderr, "[ERROR] " __FILE__ ":%i:0: no help for instruction with id %i\n", __LINE__, what);
        break;
    }
	if(what > 0 && what < sizeof(single_char_cmd) / sizeof(single_char_cmd[0]))    
		printf("    this command is equivalent to %c\n", single_char_cmd[what]);
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


int handle_prompt(int argc, const char** argv){

    if(argc < 1){
        if(output == stdout || display == render_terminal_and_graphics || display == render_terminal_and_print_map)
            display(0);
        else
            printf("\x1B[2J\x1B[H\n");
        return 0;
    }
    const int inst = get_instruction(argv[0]);
    switch (inst)
    {
    case INST_EXIT:
        if(changed_since_last_save){
            printf("there are unsaved changes, enter q to quit, s to save and quit or c to cancel\n");
            const int response = get_first_char_in_line();
            if(response == 'q'){
                active = 0;
                return 0;
            }
            if(response == 's'){
                if(save_map(map_path))
                    return 1;
                active = 0;
                return 0;
            }
            return 0;
        }
		active = 0;
        return 0;
    case INST_DISPLAY:
		display(0);
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
    case INST_MAPTILE:{
        if(argc < 2){
            fprintf(stderr, "[ERROR] %s expects at least one argument, tile key\n", argv[0]);
            return 1;
        }
        if(argc > 3){
            fprintf(stderr, "[ERROR] %s expects at most two argument, tile key and tile value, got %i instead\n", argv[0], argc - 1);
            return 1;
        }

        GET_UINT(x, argv, 1);
        if(x >= sizeof(tile_mapping) / sizeof(tile_mapping[0])){
            fprintf(stderr, "[ERROR] can only map tiles up to %i\n", (int) (sizeof(tile_mapping) / sizeof(tile_mapping[0])));
            return 1;
        }

        if(argc == 2){
            unmap_tile(x);
            return 0;
        }
        GET_UINT(y, argv, 2);

        return map_tile(x, y);
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
    case INST_PLACEHOLLOW:{
        PROMPT_EXPECT_ARGC(2);
        GET_UINT(x, argv, 1);
        GET_UINT(y, argv, 2);
        if(place_hollow(held_tile, x, y)){
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
                printf(
                    "\ttile %i with symbol %c at layer %i",
                    tile, (tile >= 0 && tile < palette_len)? palette[tile] : '\0', k
                );
                for(TILE i = 0; i < sizeof(tile_mapping) / sizeof(tile_mapping[0]); i+=1){
                    if(tile_mapping[i] == tile && is_tile_mapped(i)){
                        printf(
                            ", tile %i is mapped by %i",
                            tile, i
                        );
                        break;
                    }
                }
                printf("\n");
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
        TILE** nmap = malloc(layers * sizeof(nmap[0]));
        const int wmin = (w < mapw)? w : mapw;
        const int hmin = (h < maph)? h : maph;

        for(int k = 0; k < layers; k += 1){
            nmap[k] = malloc(w * h * sizeof(nmap[0][0]));
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
        TILE** nmap = malloc((layers + 1) * sizeof(nmap[0]));
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
        TILE* const first_placeholder = map[first];
        map[first] = map[second];
        map[second] = first_placeholder;
        display(0);
    }
        return 0;
    case INST_MERGELAYERS:{
        PROMPT_EXPECT_ARGC(2);
        GET_UINT(first, argv, 1);
        GET_UINT(second, argv, 2);
        if(first >= layers || second >= layers){
            fprintf(stderr, "[ERROR] layers only go up to %i, got %i and %i\n", layers - 1, first, second);
            return 1;
        }

        for(int i = 0; i < maph; i+=1){
            for(int j = 0; j < mapw; j+=1){
                const int f = map[first][i * mapw + j];
                const int s = map[second][i * mapw + j];
                map[second][i * mapw + j] = (f < s)? s : f;
            }
        }
        current_layer = second;
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
    
    case INST_QUERY:{
        if(argc < 2){
            fprintf(stderr, "[ERROR] expected at least one argument, queried tile, got %i instead\n", argc - 1);
            return 1;
        }
        int _query = 0;
        int old = -1;
        int _new = -1;
        int targc = 0;
        for(int i = 1; i < argc; i++){
            if(cmp_str(argv[i], "-q")){
                _query = 1;
                continue;;
            }
            GET_UINT(tile_number, argv, i);
            if(old >= 0){
                _new = tile_number;
            }
            else{
                old = tile_number;
            }
            targc += 1;
        }
        if(targc > 2){
            fprintf(stderr, "[ERROR] expected at most two nonflag arguments, old and new tile, got %i instead\n", targc);
            return 1;
        }
        if(old < 0){
            fprintf(stderr, "[ERROR] expected valid queried tile\n");
            return 1;
        }
        if(is_tile_mapped(old)){
            old = get_real_tile(old);
        }
        if(_new > -1 && is_tile_mapped(_new))
            _new = get_real_tile(_new);
        const int count = query(old, _new, _query);
        if(count < 0){
            fprintf(stderr, "[ERROR] query failed\n");
            return 1;
        }
        if(count == 0)
            return 0;
        display(0);
        printf("query hit %i times\n", count);
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
                "\tO: does the same as -o, but also displays map with tiles represented by single characters to terminal\n"
                "\tw <map width>: sets the map width\n"
                "\th <map_height>: sets the map height\n"
                "\tl <layers>: sets the number of layers\n"
                "\tsingle_character_graphics: displays map with tiles represented by single characters\n"
                "\tcommon_graphics: display map with tiles represented by graphical form (tilesheet required)\n"
                "\ttw: sets the tileset's tile width\n"
                "\tth: sets the tileset's tile height\n"
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
            if(i + 1 >= argc){
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
        else if(cmp_str(argv[i], "-O")){
            if(i + 1 >= argc){
                fprintf(stderr, "[ERROR] expected output path after '-O'\n");
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
                display = render_terminal_and_graphics;
                fclose(output);
                output = NULL;
            }
            else{
                display = render_terminal_and_print_map;
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
            if(display == render_terminal_and_graphics || display == render_terminal_and_print_map){
                fprintf(stderr, "[ERROR] cannot use %s after -O flag, possible incompatible draw modes\n", argv[i]);
                MAIN_RETURN_STATUS(1);
            }
            display = render_graphical;
        }
        else if(cmp_str(argv[i], "-single_character_graphics")){
            if(display == render_terminal_and_graphics || display == render_terminal_and_print_map){
                fprintf(stderr, "[ERROR] cannot use %s after -O flag, possible incompatible draw modes\n", argv[i]);
                MAIN_RETURN_STATUS(1);
            }
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


