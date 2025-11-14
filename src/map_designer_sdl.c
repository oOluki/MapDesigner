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

#ifndef LE_MAPDESIGNER_C
#define LE_MAPDESIGNER_C

#include "sprites.h"

#include "map_designer.h"


#ifdef _WIN32

#define SDL_MAIN_HANDLED

#endif


#include <SDL2/SDL.h>

#include <stdarg.h>

#define FPS 30

#define render_present() SDL_RenderPresent(renderer)

#define EXPAND_RECT(RECT) (RECT).x, (RECT).y, (RECT).w, (RECT).h

enum Contexts{
    CONTEXT_NONE = 0,
    CONTEXT_HOME,
    CONTEXT_MAP,
    CONTEXT_TILESHEET,

    // for counting purposes
    CONTEXT_COUNT
};

enum States{
    STATE_NONE = 0,
};

enum TextInput{
    TEXT_INPUT_NONE = 0,
    TEXT_INPUT_MAP_PATH,
    TEXT_INPUT_HELD_TILE,
    TEXT_INPUT_TILEW,
    TEXT_INPUT_TILEH
};

enum Buttons{
    BUTTON_NONE = 0,
    BUTTON_GO_TO_HOME,
    BUTTON_GO_TO_MAP,
    BUTTON_GO_TO_TILESHEET,
    BUTTON_SAVE,

    // for counting purposes
    BUTTON_COUNT
};


static SDL_Window*      window;
static int              window_width;
static int              window_height;
static SDL_Renderer*    renderer;

static SDL_Texture*     sprite_canvas;

static SDL_Texture*     draw_canvas;
static int              draw_canvas_width;
static int              draw_canvas_height;

static SDL_Texture*     tilesheet;
static int              tilesheet_width;
static int              tilesheet_height;
static int              tilew;
static int              tileh;

static SDL_Event        events;

static SDL_Rect         map_canvas;

static int              zoom;

static int              buttons[10];
static int              button_count;

static int              mousex;
static int              mousey;
static int              rmouse_pressed;
static Uint64           rmousebuttonpress_time;
static int              rmouse_click;
static int              lmouse_pressed;
static Uint64           lmousebuttonpress_time;
static int              lmouse_click;

static int              max_tile  = 36;

static float            scrollx;
static float            scrolly;
static float            dt;

static char*            mousemsg = NULL;
static int              mousemsg_timer = 0;

static char             text[100];
static int              text_len;

static int              text_input = 0;
static int              state;
static int              context;

static inline int get_tile(int i, int j) {
    return (i >= 0 && i < maph && j >= 0 && j < mapw)? map[current_layer][i * mapw + j] : 0;
}

static inline int point_bound_rect(int x, int y, const SDL_Rect rect){
    return (x >= rect.x && x <= rect.x + rect.w) && (y >= rect.y && y <= rect.y + rect.h);
}

static inline void clear_display(SDL_Texture* display, Uint8 r, Uint8 g, Uint8 b, Uint8 a){
    SDL_SetRenderTarget(renderer, display);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_RenderClear(renderer);
}

static inline void draw_line(int x0, int y0, int x1, int y1, int thickness, Uint8 r, Uint8 g, Uint8 b, Uint8 a){

    float fdx = (float) (y1 - y0);
    float fdy = (float) (x0 - x1);
    const float fmod = SDL_sqrtf(fdx * fdx + fdy * fdy);

    const int32_t dx = (int32_t) ((fdx / fmod) * 256.0f);
    const int32_t dy = (int32_t) ((fdy / fmod) * 256.0f);

    x0 = (x0 << 8) - (dx * thickness) / 2;
    y0 = (y0 << 8) - (dy * thickness) / 2;
    x1 = (x1 << 8) - (dx * thickness) / 2;
    y1 = (y1 << 8) - (dy * thickness) / 2;

    SDL_SetRenderDrawColor(renderer, r, g, b, a);

    for(int i = 0; i < thickness; i+=1){
        SDL_RenderDrawLine(renderer, (int) (x0 >> 8), (int) (y0 >> 8), (int) (x1 >> 8), (int) (y1 >> 8));
        x0 += dx;
        y0 += dy;
        x1 += dx;
        y1 += dy;
    }
}

static inline void draw_rect(SDL_Rect rect, int thickness, Uint8 r, Uint8 g, Uint8 b, Uint8 a){
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    for(int i = 0; i < thickness; i+=1){
        SDL_RenderDrawRect(renderer, &rect);
        rect.x += 1;
        rect.y += 1;
        rect.w -= 2;
        rect.h -= 2;
    }
}

static inline void fill_rect(const SDL_Rect rect, Uint8 r, Uint8 g, Uint8 b, Uint8 a){
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_RenderFillRect(renderer, &rect);
}

void draw_tile(int tile, int x, int y, int w, int h){
    if(tile == 0) return ;
    if(tilesheet){
        const SDL_Rect src = (SDL_Rect){
            (tile % (tilesheet_width / tilew)) * tilew,
            (tile / (tilesheet_width / tilew)) * tileh,
            tilew, tileh
        };
        const SDL_Rect dest = (SDL_Rect){
            map_canvas.x + x, map_canvas.y + y, w, h
        };
        SDL_RenderCopy(renderer, tilesheet, &src, &dest);
        return;
    }
    const unsigned char* tile_sprite = sprite_sheet + (tile / SPRITES_PERROW) * SPRITE_DIM * SPRITE_SHEET_STRIDE + (tile % SPRITES_PERROW) * SPRITE_DIM;
    SDL_Texture* const old_target = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, sprite_canvas);
    for(int i = 0; i < SPRITE_DIM; i+=1){
        for(int j = 0; j < SPRITE_DIM; j+=1){
            const int pixel = (int) tile_sprite[i * SPRITE_SHEET_STRIDE + j];
            if(pixel){
                SDL_SetRenderDrawColor(renderer, 39, 79, 159, 255);
            }
            else{
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            }
            SDL_RenderDrawPoint(renderer, j, i);
        }
    }
    const SDL_Rect src = (SDL_Rect) {
        0, 0, SPRITE_DIM, SPRITE_DIM
    };
    const SDL_Rect dest = (SDL_Rect) {
        x + map_canvas.x, y + map_canvas.y, w, h
    };
    SDL_SetRenderTarget(renderer, old_target);
    SDL_RenderCopy(renderer, sprite_canvas, &src, &dest);
}

void draw_text(const char* txt, int x, int y, int charw, int charh, ...){

    if(!txt) return;

    va_list args;
    va_start(args, charh);

    SDL_Texture* const old_target = SDL_GetRenderTarget(renderer);

    for(int i = 0; txt[i]; i+=1){
        if(txt[i] == '\n'){
            y += charh;
            continue;
        }
        const int spriteid = get_sprite_from_char(txt[i]);
        if(spriteid < 0){
            fprintf(stderr, "[ERROR] can't draw text '%s', no support for character '%c'\n", txt, txt[i]);
            x += charw;
            continue;
        }
        if(txt[i] == '%'){
            char buff[20];
            if(txt[++i] == 'i'){
                int number = va_arg(args, int);
                const int is_neg = number < 0;
                if(is_neg){
                    number = -number;
                    buff[0] = '-';
                }
                int digits = 1 + is_neg;
                int _10n = 10;
                for(; number / _10n; _10n *= 10) digits += 1;
                if(digits > 20){
                    fprintf(stderr, "[ERROR] can't draw number %i, number is too big\n", number);
                    continue;
                }
                for(int j = is_neg; j < digits; j+=1){
                    _10n /= 10;
                    buff[j] = '0' + (number / _10n) - (number / (_10n * 10)) * 10;
                }
                buff[digits] = '\0';
                draw_text(buff, x, y, charw, charh);
                x += digits * charw;
                continue;
            }
            else{
                fprintf(stderr, "[ERROR] unsupported format '%c'\n", txt[i]);
            }
            continue;
        }
        const unsigned char* sprite = sprite_sheet + (spriteid / SPRITES_PERROW) * SPRITE_SHEET_STRIDE * SPRITE_DIM + (spriteid % SPRITES_PERROW) * SPRITE_DIM;
        SDL_SetRenderTarget(renderer, sprite_canvas);

        for(int k = 0; k < 8; k+=1){
            for(int j = 0; j < 8; j+=1){
                if(sprite[k * SPRITE_SHEET_STRIDE + j])
                    SDL_SetRenderDrawColor(renderer, 159, 159, 159, 255);
                else
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawPoint(renderer, j, k);
            }
        }

        const SDL_Rect src = (SDL_Rect) {
            0, 0, SPRITE_DIM, SPRITE_DIM
        };
        const SDL_Rect dest = (SDL_Rect) {
            x, y, charw, charh
        };
        SDL_SetRenderTarget(renderer, old_target);
        SDL_RenderCopy(renderer, sprite_canvas, &src, &dest);
        
        x += charw;
    }

    va_end(args);
}

int map_designer_init(){
    printf("Hello From Map Designer!\n");

    if(SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_TIMER)){
        fprintf(stderr, "[ERROR] Unable To Initiate Subsystem: %s\n", SDL_GetError());
        return 1;
    }

    SDL_DisplayMode dm;
    int window_width ;
    int window_height;
    if(SDL_GetDisplayMode(0, 0, &dm)){
        fprintf(stderr, "[WARNING] Unable To Get Display Information: %s\n", SDL_GetError());
        window_width  = 800;
        window_height = 600;
    } else{
        window_width  = dm.w;
        window_height = dm.h;
    }
    
    window = SDL_CreateWindow(
        "Map Designer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        dm.w, dm.h, SDL_WINDOW_RESIZABLE
    );
    if(!window){
        fprintf(stderr, "[ERROR] %s\n[ERROR] Unable To Create Window\n", SDL_GetError());
        return 1;
    }
    renderer = SDL_CreateRenderer(window, -1, 0);
    if(!renderer){
        fprintf(stderr, "[ERROR] %s\n[ERROR] Unable To Create Renderer\n", SDL_GetError());
        return 1;
    }

    draw_canvas = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, window_width, window_height);
    draw_canvas_width  = window_width;
    draw_canvas_height = window_height;
    if(!draw_canvas){
        fprintf(stderr, "[ERROR] Could not create draw canvas: %s\n", SDL_GetError());
        return 1;
    }

    sprite_canvas = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, SPRITE_DIM, SPRITE_DIM);
    if(!sprite_canvas){
        fprintf(stderr, "[ERROR] Could not create sprite canvas\n");
        return 1;
    }

    SDL_SetTextureBlendMode(sprite_canvas, SDL_BLENDMODE_BLEND);

    map_canvas = (SDL_Rect){
        window_width / 4, 0,
        (3 * window_width) / 4, (3 * window_height) / 4
    };

    camerax = 0;
    cameray = 0;
    cameraw = map_canvas.w;
    camerah = map_canvas.h;
    scrollx = 0;
    scrolly = 0;

    zoom  = 100;

    for(int i = 0; i < BUTTON_COUNT; i++){
        buttons[button_count++] = i;
    }

    context = CONTEXT_HOME;

    return 0;
}

void load_tilesheet(const char* path){
    int w;
    int h;
    int comp;
    stbi_uc* pixels = stbi_load(path, &w, &h, &comp, 0);
    if(!pixels){
        fprintf(stderr, "[ERROR] could not load tilesheet '%s'\n", path);
        return;
    }
    int format;
    switch (comp)
    {
    case 1:
        format = SDL_PIXELFORMAT_INDEX8;
        break;
    case 2:
        format = SDL_PIXELFORMAT_RGBA4444;
        break;
    case 3:
        format = SDL_PIXELFORMAT_RGB888;
        break;
    case 4:
        format = SDL_PIXELFORMAT_RGBA32;
        break;                
    default:
        fprintf(stderr, "[ERROR] invalid compression for tilesheet %i\n", comp);
        stbi_image_free(pixels);
        return;
    }
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormatFrom(pixels, w, h, 8, w * comp, format);
    if(!s){
        fprintf(stderr, "[ERROR] could not create tilesheet surface: %s\n", SDL_GetError());
        stbi_image_free(pixels);
        return;
    }
    SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
    if(!t){
        fprintf(stderr, "[ERROR] could not create tilesheet texture: %s\n", SDL_GetError());
        SDL_FreeSurface(s);
        stbi_image_free(pixels);
        return;
    }
    SDL_DestroyTexture(tilesheet);
    tilesheet = t;
    tilesheet_width  = w;
    tilesheet_height = h;
    max_tile = (tilesheet_height / tileh) * (tilesheet_width / tilew);
    SDL_FreeSurface(s);
    stbi_image_free(pixels);
}

void press_button(int button){
    switch (button)
    {
    case BUTTON_NONE:
        printf("pressed button_none\n");
        break;
    case BUTTON_GO_TO_HOME:
        printf("pressed button_go_to_home\n");
        state = STATE_NONE;
        context = CONTEXT_HOME;
        text_input = TEXT_INPUT_NONE;
        break;
    case BUTTON_GO_TO_MAP:
        printf("pressed button_go_to_map\n");
        state = STATE_NONE;
        context = CONTEXT_MAP;
        text_input = TEXT_INPUT_NONE;
        break;
    case BUTTON_GO_TO_TILESHEET:
        printf("pressed button_go_to_tilesheet\n");
        state = STATE_NONE;
        context = CONTEXT_TILESHEET;
        text_input = TEXT_INPUT_NONE;
        break;
    case BUTTON_SAVE:
        if(!map_path){
            const int chars_len = sizeof("abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ") / sizeof(char);
            const char* const chars = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            char path[10] = {'m', 'a', 'p', '_', '_', '.', 't', 'x', 't', '\0'};
            FILE* f = NULL;
            int i = 0;
            do {
                path[4] = chars[i];
                f = fopen(path, "r");
                if(f) fclose(f);
                i+=1;
            } while(f && i < chars_len);
            
            if(i >= chars_len){
                fprintf(stderr, "[ERROR] can't make a file to save to\n");
                mousemsg = "save failed, internal problem, give a valid save file before trying again";
                mousemsg_timer = FPS * 4;
            }
            else if(save_map(path)){
                mousemsg = "save failed, make sure you have a valid saving file";
                mousemsg_timer = FPS * 4;
            }
        }
        else if(save_map(map_path)){
            mousemsg = "save failed, make sure you have a valid saving file";
            mousemsg_timer = FPS * 3;
        }
        break;
    
    default:
        fprintf(stderr, "[ERROR] " __FILE__ ":%i:10: Invalid button id: %i\n", __LINE__, button);
        break;
    }
}

const unsigned char* get_button_sprite(int button){
    switch (button)
    {
    case BUTTON_NONE:
        return  "        "
                " #    # "
                "  #  #  "
                "   ##   "
                "   ##   "
                "  #  #  "
                " #    # "
                "        ";
    case BUTTON_GO_TO_HOME:
        return  "        "
                "   ##   "
                "  ####  "
                " ###### "
                "  ## #  "
                "  # ##  "
                "  # ##  "
                "        ";
    case BUTTON_GO_TO_MAP:
        return  "        "
                "  #  #  "
                " # ## # "
                " # ## # "
                " #    # "
                " #    # "
                " #    # "
                "        ";
    case BUTTON_GO_TO_TILESHEET:
        return  "        "
                "  ####  "
                " # ## # "
                "   ##   "
                "   ##   "
                "   ##   "
                "   ##   "
                "        ";
    case BUTTON_SAVE:
        return  "        "
                "   ##   "
                "   ##   "
                " # ## # "
                "  ####  "
                "   ##   "
                " ###### "
                "        ";
    
    default:
        fprintf(stderr, "[ERROR] " __FILE__ ":%i:10: Invalid button id: %i\n", __LINE__, button);
        return NULL;
    }
}

const char* get_button_name(int button){
    switch (button)
    {
    case BUTTON_GO_TO_HOME:
        return "HOME";
    case BUTTON_GO_TO_MAP:
        return "MAP";
    case BUTTON_GO_TO_TILESHEET:
        return "TILESHEET";
    case BUTTON_SAVE:
        return "SAVE";
    
    default:
        fprintf(stderr, "[ERROR] " __FILE__ ":%i:10: Invalid button id: %i\n", __LINE__, button);
    case BUTTON_NONE:
        return "BUTTON NONE";
    }
}

void handle_keydown(SDL_Keycode key){
    switch (key)
    {
    case SDLK_SPACE:
        printf(
            "camera: (%i, %i, %i, %i)\n",
            camerax, cameray, cameraw, camerah
        );
        break;
    case SDLK_w:
        scrolly -= 1;
        if(scrolly < 0) scrolly = 0;
        break;
    case SDLK_a:
        scrollx -= 1;
        if(scrollx < 0) scrollx = 0;
        break;
    case SDLK_s:
        scrolly += 1;
        if(scrolly + camerah >= maph * tileh){
            scrolly = (float) (maph * tileh - camerah);
        }
        break;
    case SDLK_d:
        scrollx += 1;
        if(scrollx + cameraw >= mapw * tilew){
            scrollx = (float) (mapw * tilew - cameraw);
        }
        break;

    case SDLK_UP:
        held_tile -= 10;
        if(held_tile < 0) held_tile = (max_tile + held_tile) % max_tile;
        break;
    case SDLK_LEFT:
        held_tile -= 1;
        if(held_tile < 0) held_tile = (max_tile + held_tile) % max_tile;
        break;
    case SDLK_DOWN:
        held_tile = (held_tile + 10) % max_tile;
        break;
    case SDLK_RIGHT:
        held_tile = (held_tile + 1) % max_tile;
        break;
    
    default:
        break;
    }
}

int handle_events(){

    rmouse_click = 0;
    lmouse_click = 0;

    while (SDL_PollEvent(&events))
    {
        switch (events.type)
        {
        case SDL_QUIT:
            context = CONTEXT_NONE;
            break;
        case SDL_KEYDOWN:
            if(text_input == TEXT_INPUT_NONE) handle_keydown(events.key.keysym.sym);
            else if(events.key.keysym.sym == SDLK_BACKSPACE){
                if(text_len > 0) text[--text_len] = '\0';
            }
            else if(events.key.keysym.sym == SDLK_KP_ENTER){
                switch (text_input)
                {
                case TEXT_INPUT_MAP_PATH:
                    if(load_map(text)){
                        mousemsg = "could not load map";
                        mousemsg_timer = 3 * FPS;
                    }
                    break;
                case TEXT_INPUT_HELD_TILE:{
                    const int i = parse_uint(text);
                    if(i < 0){
                        mousemsg = "invalid tile";
                        mousemsg_timer = 3 * FPS;
                    }
                    else held_tile = i;
                }
                    break;
                case TEXT_INPUT_TILEW:{
                    const int i = parse_uint(text);
                    if(i < 0){
                        mousemsg = "invalid tilew";
                        mousemsg_timer = 3 * FPS;
                    }
                    else{
                        tilew = i;
                        max_tile = (tilesheet_height / tileh) * (tilesheet_width / tilew);
                    }
                }
                    break;
                case TEXT_INPUT_TILEH:{
                    const int i = parse_uint(text);
                    if(i < 0){
                        mousemsg = "invalid tileh";
                        mousemsg_timer = 3 * FPS;
                    }
                    else{
                        tileh = i;
                        max_tile = (tilesheet_height / tileh) * (tilesheet_width / tilew);
                    }
                }
                    break;
                
                default:
                    fprintf(stderr, "[ERROR] " __FILE__ ":%i:10: invalid text input flag: %i\n", __LINE__, text_input);
                    break;
                }
                text_input = TEXT_INPUT_NONE;
                text_len = 0;
            }
            break;
        case SDL_WINDOWEVENT:{
            if(events.window.event != SDL_WINDOWEVENT_RESIZED) break;
            window_width  = events.window.data1;
            window_height = events.window.data2;
        }   break;
        case SDL_MOUSEMOTION:
            mousex = (events.motion.x * draw_canvas_width)  / window_width;
            mousey = (events.motion.y * draw_canvas_height) / window_height;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if(events.button.button == SDL_BUTTON_LEFT && mousex < map_canvas.x){
                const int pressed_button = (mousey * button_count) / draw_canvas_height;
                if(pressed_button < button_count){
                    press_button(pressed_button);
                }
            }
            if(events.button.button == SDL_BUTTON_LEFT){
                lmouse_pressed = 1;
                lmousebuttonpress_time = SDL_GetTicks64();
            }
            else if(events.button.button == SDL_BUTTON_RIGHT){
                rmouse_pressed = 1;
                rmousebuttonpress_time = SDL_GetTicks64();
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if(events.button.button == SDL_BUTTON_LEFT){
                lmouse_pressed = 0;
                if(SDL_GetTicks64() - lmousebuttonpress_time < 200) lmouse_click = 1;
            }
            else if(events.button.button == SDL_BUTTON_RIGHT){
                rmouse_pressed = 0;
                if(SDL_GetTicks64() - rmousebuttonpress_time < 200) rmouse_click = 1;
            }
            break;
        case SDL_MOUSEWHEEL:
            if(context != CONTEXT_MAP) break;
            if(events.wheel.y == 1 || events.wheel.y == -1){
                held_tile += events.wheel.y;
            }
            else{
                held_tile += events.wheel.y / 2;
            }
            if(held_tile < 0) held_tile = (max_tile + held_tile) % max_tile;
            else held_tile = held_tile % max_tile;
            break;
        case SDL_DROPFILE:
            switch (context)
            {
            case CONTEXT_MAP:
                load_map(events.drop.file);
                SDL_free(events.drop.file);
                break;
            case CONTEXT_TILESHEET:
                load_tilesheet(events.drop.file);
                SDL_free(events.drop.file);
                break;
            
            default:
                break;
            }
            break;
        case SDL_TEXTINPUT:
            for(int i = 0; events.text.text[i] && text_len < sizeof(text) - 1; i+=1){
                text[text_len++] = events.text.text[i];
            }
            break;
        
        default:
            break;
        }
    }
    
    return 0;
}

void map_designer_close(){
    if(sprite_canvas)   SDL_DestroyTexture(sprite_canvas);
    if(draw_canvas)     SDL_DestroyTexture(draw_canvas);
    if(tilesheet)       SDL_DestroyTexture(tilesheet);
    if(renderer)        SDL_DestroyRenderer(renderer);
    if(window)          SDL_DestroyWindow(window);
    SDL_Quit();
    context = CONTEXT_NONE;
}

int main(){

    if(map_designer_init()){
        fprintf(stderr, "[ERROR] Failed to initiate\n");
        map_designer_close();
        return 1;
    }
    if(layers <= 0) layers = 1;
    if(mapw <= 0)   mapw = 20;
    if(maph <= 0)   maph = 16;

    if(!map){
        map = malloc(layers * sizeof(map[0]));
        for(int k = 0; k < layers; k+=1){
            map[k] = malloc(mapw * maph * sizeof(map[0][0]));
            for(int i = 0; i < maph; i+=1)
                for(int j = 0; j < mapw; j+=1)
                    map[current_layer][i * mapw + j] = 0;
        }
    }
    if(tilew <= 0) tilew = SPRITE_DIM;
    if(tileh <= 0) tileh = SPRITE_DIM;

    Uint64 t0 = SDL_GetTicks64();

    while (context != CONTEXT_NONE){

        {   // clock tick
            const Uint64 _dt = SDL_GetTicks64() - t0;
            if(_dt < 1000 / FPS){
                SDL_Delay(1000 / FPS);
            }
            t0 = SDL_GetTicks64();
        }

        clear_display(NULL, 0, 0, 0, 255);{
            clear_display(draw_canvas, 32, 32, 32, 255);

            switch (context)
            {
            case CONTEXT_HOME:{
                const SDL_Rect map_path_text_rect = (SDL_Rect){
                    map_canvas.x + 8, map_canvas.y + 8, 16, 16
                };
                const SDL_Rect held_tile_text_rect = (SDL_Rect){
                    map_canvas.x + 8, map_canvas.y + 56, 16, 16
                };
                const SDL_Rect tilew_text_rect = (SDL_Rect){
                    map_canvas.x + 8, map_canvas.y + 80, 16, 16
                };
                const SDL_Rect tileh_text_rect = (SDL_Rect){
                    map_canvas.x + 8, map_canvas.y + 104, 16, 16
                };
                if(lmouse_click && point_bound_rect(mousex, mousey, map_path_text_rect)){
                    text_input = TEXT_INPUT_MAP_PATH;
                }
                if(lmouse_click && point_bound_rect(mousex, mousey, held_tile_text_rect)){
                    text_input = TEXT_INPUT_HELD_TILE;
                }
                if(lmouse_click && point_bound_rect(mousex, mousey, tilew_text_rect)){
                    text_input = TEXT_INPUT_TILEW;
                }
                if(lmouse_click && point_bound_rect(mousex, mousey, tileh_text_rect)){
                    text_input = TEXT_INPUT_TILEH;
                }
                draw_text(map_path, EXPAND_RECT(map_path_text_rect));
                //draw_text("camera: (%i, %i, %i, %i)", map_canvas.x + 8, map_canvas.y + 32, 16, 16, camerax, cameray, cameraw, camerah);
                draw_text("held tile: %i", EXPAND_RECT(held_tile_text_rect), held_tile);
                draw_text("tilew: %i", EXPAND_RECT(tilew_text_rect), tilew);
                draw_text("tileh: %i", EXPAND_RECT(tileh_text_rect), tileh);
                const int max_text_chars_per_line = ((map_canvas.w - 16) / 16);
                const int text_lines = text_len / max_text_chars_per_line + !!(text_len % max_text_chars_per_line);
                const int text_x = (text_len < max_text_chars_per_line)? map_canvas.x + (map_canvas.w - text_len * 16) / 2 : map_canvas.x + (map_canvas.w - max_text_chars_per_line * 16) / 2;
                const int text_y = map_canvas.y + (map_canvas.h - text_lines * 16 + 16) / 2;
                switch (text_input)
                {
                case TEXT_INPUT_MAP_PATH:
                    draw_text("enter valid map path", map_canvas.x + (map_canvas.w - sizeof("enter valid map pat") * 16) / 2,
                    map_canvas.y + (map_canvas.h / 2), 16, 16);
                    break;
                case TEXT_INPUT_HELD_TILE:
                    draw_text("enter valid tile", map_canvas.x + (map_canvas.w - sizeof("enter valid map pat") * 16) / 2,
                    map_canvas.y + (map_canvas.h / 2), 16, 16);
                    break;
                case TEXT_INPUT_TILEW:
                    draw_text("enter valid tile width", map_canvas.x + (map_canvas.w - sizeof("enter valid map pat") * 16) / 2,
                    map_canvas.y + (map_canvas.h / 2), 16, 16);
                    break;
                case TEXT_INPUT_TILEH:
                    draw_text("enter valid tile height", map_canvas.x + (map_canvas.w - sizeof("enter valid map pat") * 16) / 2,
                    map_canvas.y + (map_canvas.h / 2), 16, 16);
                    break;                
                default:
                    break;
                }
                if(text_input != TEXT_INPUT_NONE){
                    int text_pos = 0;
                    for(int i = 0; i < text_lines; i += 1){
                        if(text_pos + max_text_chars_per_line < text_len){
                            const char c = text[text_pos + max_text_chars_per_line];
                            text[text_pos + max_text_chars_per_line] = '\0';
                            draw_text(text + text_pos, text_x, text_y + i * 16, 16, 16);
                            text[text_pos + max_text_chars_per_line] = c;
                            text_pos += max_text_chars_per_line;
                        }
                        else{
                            draw_text(text + text_pos, text_x, text_y + i * 16, 16, 16);
                            break;
                        }
                    }
                    draw_rect(
                        (SDL_Rect){
                            text_x - 5, text_y - 5 - 16,
                            (text_len < max_text_chars_per_line)? text_len * 16 + 10 : max_text_chars_per_line * 16 + 10,
                            text_lines * 16 + 10
                        },
                        5, 159, 159, 159, 255
                    );
                }
            }
                break;
            case CONTEXT_MAP:{
                if(scrollx < 0) scrollx = 0.0f;
                if(scrollx + cameraw > mapw * tilew) scrollx = (float) (mapw * tilew - cameraw);
                if(scrolly < 0) scrolly = 0.0f;
                if(scrolly + camerah > maph * tileh) scrolly = (float) (maph * tileh - camerah);
                
                camerax = (int) scrollx;
                cameray = (int) scrolly;

                const int irange = ((cameray + camerah) / tileh < maph)? (cameray + camerah) / tileh : maph;
                const int jrange = ((camerax + cameraw) / tilew < mapw)? (camerax + cameraw) / tilew : mapw;
                const int i0 = SDL_max(cameray / tileh, 0);
                const int j0 = SDL_max(camerax / tilew, 0);
                const int zoommed_tilew = map_canvas.w / jrange;
                const int zoommed_tileh = map_canvas.h / irange;
                if(point_bound_rect(mousex, mousey, map_canvas)){
                    if(lmouse_pressed)
                        place(held_tile, j0 + (mousex - map_canvas.x) / zoommed_tilew, i0 + (mousey - map_canvas.y) / zoommed_tileh);
                    else if(rmouse_click)
                        place(0, j0 + (mousex - map_canvas.x) / zoommed_tilew, i0 + (mousey - map_canvas.y) / zoommed_tileh);
                }
                for(int i = i0; i < irange; i+=1){
                    for(int j = j0; j < jrange; j+=1){
                        draw_tile(map[current_layer][i * mapw + j], j * zoommed_tilew, i * zoommed_tileh, zoommed_tilew, zoommed_tileh);
                    }
                    draw_line(
                        map_canvas.x, map_canvas.y + (i - i0) * zoommed_tileh,
                        map_canvas.x + map_canvas.w, map_canvas.y + (i - i0) * zoommed_tileh,
                        3, 200, 200, 200, 200
                    );
                }
                for(int j = j0; j < jrange; j+=1){
                    draw_line(
                        map_canvas.x + (j - j0) * zoommed_tilew, map_canvas.y,
                        map_canvas.x + (j - j0) * zoommed_tilew, map_canvas.y + map_canvas.h,
                        3, 200, 200, 200, 200
                    );
                }
                if(point_bound_rect(mousex, mousey, map_canvas)) draw_tile(
                    held_tile,
                    ((mousex - map_canvas.x) / zoommed_tilew) * zoommed_tilew,
                    ((mousey - map_canvas.y) / zoommed_tileh) * zoommed_tileh,
                    zoommed_tilew, zoommed_tileh
                );
            }
                break;
            case CONTEXT_TILESHEET:{
                const int tile_ycount = tilesheet_height / tileh;
                const int tile_xcount = tilesheet_width  / tilew;
                const int zoommed_tileh = (tile_ycount)? map_canvas.h / tile_ycount : 0;
                const int zoommed_tilew = (tile_xcount)? map_canvas.w / tile_xcount : 0;
                for(int i = 0; i < tile_ycount; i+=1){
                    for(int j = 0; j < tile_xcount; j+=1){
                        const SDL_Rect src = (SDL_Rect){
                            j * tilew, i * tileh,
                            tilew, tileh
                        };
                        const SDL_Rect dst = (SDL_Rect){
                            map_canvas.x +  j * zoommed_tilew, map_canvas.y + i * zoommed_tileh,
                            zoommed_tilew, zoommed_tileh
                        };
                        SDL_RenderCopy(renderer, tilesheet, &src, &dst);
                    }

                }
                if(lmouse_click && point_bound_rect(mousex, mousey, map_canvas)){
                    const int i = (zoommed_tileh)? mousey / zoommed_tileh : 0;
                    const int j = (zoommed_tilew)? mousex / zoommed_tilew : 0;
                    held_tile = i * tile_xcount + j;
                }
            }
                break;
            
            default:
                break;
            }
            
            for(int i = 0; i < button_count; i+=1){
                const SDL_Rect dest = (SDL_Rect){
                    0, (i * draw_canvas_height) / button_count,
                    map_canvas.x, draw_canvas_height / button_count
                };
                const unsigned char* sprite = get_button_sprite(buttons[i]);
                if(sprite){
                    SDL_Texture* const old_target = SDL_GetRenderTarget(renderer);
                    SDL_SetRenderTarget(renderer, sprite_canvas);
                    for(int y = 0; y < 8; y+=1){
                        for(int x = 0; x < 8; x+=1){
                            if(sprite[y * 8 + x] != ' ')
                                SDL_SetRenderDrawColor(renderer, 159, 159, 159, 255);
                            else
                                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                            SDL_RenderDrawPoint(renderer, x, y);
                        }
                    }
                    SDL_SetRenderTarget(renderer, old_target);
                    SDL_RenderCopy(renderer, sprite_canvas, NULL, &dest);
                }
                draw_rect(dest, 5, 125, 25, 35, 200);
                if(point_bound_rect(mousex, mousey, dest)){
                    draw_text(get_button_name(i), mousex, mousey, 16, 16);
                }
            }

            if(mousemsg_timer > 0){
                mousemsg_timer -= 1;
                draw_text(mousemsg, mousex, mousey, 16, 16);
            }

            SDL_SetRenderTarget(renderer, NULL);
            SDL_RenderCopy(renderer, draw_canvas, NULL, NULL);
        } render_present();

        handle_events();
    }

    map_designer_close();

    return 0;
}

#endif // END OF FILE ============================================