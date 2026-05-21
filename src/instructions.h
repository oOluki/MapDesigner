#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "map_designer.h"

enum Instructions {
  INST_NONE = 0,
  INST_EXIT,
  INST_DISPLAY,  
  INST_HOLD,
  INST_HOLDS,
  INST_MAPTILE,
  INST_PLACE,
  INST_PLACEHOLLOW,
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
  INST_MERGELAYERS,
  INST_LAYER,
  INST_SHOW,
  INST_PATTERN,
  INST_SAVE,
  INST_LOAD,
  INST_QUERY,
  INST_HELP,

  // for counting purposes
  INST_COUNT
};

static char single_char_cmd[INST_COUNT] = {
    [INST_NONE] = '\'',
    [INST_EXIT] = 'q',
    [INST_DISPLAY] = ' ',
    [INST_HOLD] = 'g',
    [INST_HOLDS] = 'h',
    [INST_MAPTILE] = '$',
    [INST_PLACE] = 'p',
    [INST_PLACEHOLLOW] = 'P',
    [INST_PENCIL] = '@',
    [INST_MOVE] = 'm',
    [INST_ZOOM] = 'z',
    [INST_SYMSWAP] = ';',
    [INST_COPY] = 'c',
    [INST_PASTE] = '=',
    [INST_CHECK] = '.',
    [INST_NEW] = 'n',
    [INST_NEWLAYER] = '+',
    [INST_DELLAYER] = '-',
    [INST_SWAPLAYERS] = '/',
    [INST_MERGELAYERS] = '\"',
    [INST_LAYER] = 'l',
    [INST_SHOW] = '!',
    [INST_PATTERN] = ':',
    [INST_SAVE] = 'v',
    [INST_LOAD] = '^',
    [INST_QUERY] = '%',
    [INST_HELP] = '?'
};



int get_instruction(const char *what) {

    if (!what) return INST_NONE;  
  
    if(cmp_str(what, "exit") || cmp_str(what, "quit") || cmp_str(what, "q") || cmp_str(what, "e"))
      return INST_EXIT;

    if (!what[1]) {
		switch (what[0]) {
		case 'w':
			if (cameray > 0)
				cameray -= 1;
			return INST_DISPLAY;
		case 'a':
			if (camerax > 0)
				camerax -= 1;
			return INST_DISPLAY;
		case 's':
			if (cameray + camerah < maph)
				cameray += 1;
			return INST_DISPLAY;
		case 'd':
			if (camerax + cameraw < mapw)
				camerax += 1;
			return INST_DISPLAY;                

		default:
			for (int i = 0; i < sizeof(single_char_cmd) / sizeof(single_char_cmd[0]); i += 1) {
				if (single_char_cmd[i] == what[0])
					return i;            
			}
			return INST_NONE;        
        }
	}

    if(cmp_str(what, "display"))                        return INST_DISPLAY    ;
    if(cmp_str(what, "hold"))                           return INST_HOLD       ;
    if(cmp_str(what, "holds"))                          return INST_HOLDS      ;
    if(cmp_str(what, "maptile"))                        return INST_MAPTILE    ;
    if(cmp_str(what, "place"))                          return INST_PLACE      ;
    if(cmp_str(what, "hollow"))                         return INST_PLACEHOLLOW;
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
    if(cmp_str(what, "merge"))                          return INST_MERGELAYERS;
    if(cmp_str(what, "layer"))                          return INST_LAYER      ;
    if(cmp_str(what, "show"))                           return INST_SHOW       ;
    if(cmp_str(what, "pattern"))                        return INST_PATTERN    ;
    if(cmp_str(what, "save"))                           return INST_SAVE       ;
    if(cmp_str(what, "load"))                           return INST_LOAD       ;
    if(cmp_str(what, "query"))                          return INST_QUERY    ;
    if(cmp_str(what, "help"))                           return INST_HELP       ;

    return INST_NONE;
}


static int get_first_char_in_line(){
    int c = 0;
    for(c = fgetc(stdin); c == ' ' || c == '\t'; c = fgetc(stdin));
    const int answer = c;
    for(c = fgetc(stdin); c != EOF && c != '\n'; c = fgetc(stdin));
    return answer;
}

static inline int get_yes_or_no_answer(){
    const int c = get_first_char_in_line();
    return c == 'y' || c == 'Y';
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
        for(TILE i = 0; i < sizeof(tile_mapping) / sizeof(tile_mapping[0]); i+=1){
            if(is_tile_mapped(i)){
                printf("tile %i maps to %i\n", i, get_real_tile(i));
            }
        }
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
                    draw_tile = get_yes_or_no_answer();
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
            draw_tileset = get_yes_or_no_answer();
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
    if(cmp_str(what, "tile")){
        printf("held tile %i -> %c\n", held_tile, (held_tile >= 0 && held_tile < palette_len)? palette[held_tile] : '~');
        if(is_tile_mapped(held_tile)){
            printf("tile %i maps to %i\n", held_tile, get_real_tile(held_tile));
        }
        if(!skip_questions && tileset != NULL){
            printf("do you wish to draw the tile graphical representation[y/n]?\n");
            const int response = get_first_char_in_line();
            if(response != 'y' && response != 'Y')
                return 0;
            
            uint32_t* pixels = (uint32_t*) malloc(tileset_tilew * tileset_tileh * sizeof(pixels[0]));

            render_tile_graphical(held_tile, 0, 0, pixels, tileset_tilew, tileset_tileh, tileset_tilew);

            for(int i = 0; i < tileset_tileh; i+=1){
                printf("  ");
                for(int j = 0; j < tileset_tilew; j+=1){
                    put_color_char(pixels[i * tileset_tilew + j]);
                }
                putchar('\n');
            }

            free(pixels);
        }
        return 0;
    }
    if(cmp_str(what, "tileset")){

        int draw_tileset = 0;
        if(!skip_questions){
            printf("do you wish to draw the tileset[y/n]?\n");
            draw_tileset = get_first_char_in_line();
            draw_tileset = draw_tileset == 'y' || draw_tileset == 'Y';
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
    if(cmp_str(what, "tile_mapping")){
        for(TILE i = 0; i < sizeof(tile_mapping) / sizeof(tile_mapping[0]); i+=1){
            printf("tile %i maps to %i\n", i, get_real_tile(i));
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

// \returns the number of replacements performed, if only searched then the number of matched tiles
static int query(int old, int _new, int query){

    int count = 0;

    if(_new < 0)
        _new = old;

    changed_since_last_save = 1;

    if(!query){

        for(int l = 0; l < layers; l+=1){
            for(int i = 0; i < maph; i+=1){
                for(int j = 0; j < mapw; j+=1){
                    if(map[l][i * mapw + j] == old){
                        map[l][i * mapw + j] = _new;
                        count += 1;
                    }
                }
            }
        }

        return count;

    }

    printf(
        "enter l to only search current layer, d to only search inside the display view,\n"
        "D to search inside the display view through all layers, s to only search the current copy selection or\n"
        "S search the current copy selection through all layers. Otherwise a global search will be performed.\n"
    );
    
    const int __response = get_first_char_in_line();

    int l0 = 0;
    int i0 = 0;
    int j0 = 0;
    int lr = layers;
    int ir = maph;
    int jr = mapw;

    switch (__response)
    {
    case 'd':
        l0 = current_layer;
        lr = current_layer + 1;
    case 'D':
        i0 = cameray;
        ir = cameray + camerah;
        j0 = camerax;
        jr = camerax + cameraw;
        break;
    case 'l':
        l0 = current_layer;
        lr = current_layer + 1;
        break;
    case 's':
        l0 = current_layer;
        lr = current_layer + 1;
    case 'S':
        if(copyy < 0 || copyx < 0){
            fprintf(stderr, "[ERROR] no active selection\n");
            return -1;
        }
        i0 = copyy;
        ir = i0 + pencilh;
        j0 = copyx;
        jr = j0 + pencilw;
        break;
    default:
        break;
    }

    for(int l = l0; l < lr && l < layers; l+=1){
        for(int i = i0; i < ir && i < maph; i+=1){
            for(int j = j0; j < jr && j < mapw; j+=1){
                if(map[l][i * mapw + j] == old){
                    if(!query){
                        map[l][i * mapw + j] = _new;
                        count += 1;
                        continue;
                    }
                    current_layer = l;
                    camerax = cramp(j - cameraw / 2, mapw, 0);
                    cameray = cramp(i - camerah / 2, maph, 0);
                    display(0);
                    if(_new != old) printf(
                        "replace tile %i at layer %i (%i, %i) for %i?\n"
                        "(n or s to skip, q to quit query, a to replace all from here, otherwise replace it)\n",
                        map[l][i * mapw + j], l, i, j, _new
                    );
                    else printf(
                        "enter q to quit query, a to query all from here, otherwise continue query\n"
                        "found queried tile %i at layer %i (%i, %i)\n",
                        old, l, i, j
                    );
                    const int response = get_first_char_in_line();
                    switch (response)
                    {
                    case 'q':
                        return count;
                    case 'a':
                        query = 0;
                    default:
                        map[l][i * mapw + j] = _new;
                        count += 1;
                    case 'n':
                    case 's':
                        break;
                    }
                }
            }
        }
    }

    return count;
}


#endif // =====================  END OF FILE INSTRUCTIONS_H ===========================