#include <SDL2/SDL.h>

#define WW_SIZE 16

#define WW_SCALE 32

enum {
    WW_CELL_NULL,
    WW_CELL_WIRE,
    WW_CELL_TAIL,
    WW_CELL_HEAD,
};

typedef Uint8 ww_cell_t;

typedef struct {
    ww_cell_t data[WW_SIZE][WW_SIZE];
} ww_world_t;

typedef struct {
    int x, y;
} ww_vector2_t;

static const SDL_Color
    WW_COLOR_BLACK   = {0x00, 0x00, 0x00, 0xFF},
    WW_COLOR_YELLOW  = {0xFF, 0xFF, 0x00, 0xFF},
    WW_COLOR_BLUE    = {0x00, 0x00, 0xFF, 0xFF},
    WW_COLOR_RED     = {0xFF, 0x00, 0x00, 0xFF},
    WW_COLOR_MAGENTA = {0xFF, 0x00, 0xFF, 0xFF};

SDL_Color ww_cell_color(ww_cell_t cell) {
    switch (cell) {
    case WW_CELL_NULL: return WW_COLOR_BLACK;
    case WW_CELL_WIRE: return WW_COLOR_YELLOW;
    case WW_CELL_HEAD: return WW_COLOR_BLUE;
    case WW_CELL_TAIL: return WW_COLOR_RED;
    }
    return WW_COLOR_MAGENTA;
}

void ww_cell_render(Uint8 cell, int x, int y, SDL_Renderer *renderer) {
    SDL_Color col = ww_cell_color(cell);

    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);

    SDL_Rect rect = {x, y, 32, 32};

    SDL_RenderFillRect(renderer, &rect);
}

int ww_world_coord(ww_world_t *world, ww_vector2_t vec) {
    return vec.x >= 0 && vec.y >= 0 && vec.x < WW_SIZE && vec.y < WW_SIZE;
}

ww_cell_t ww_world_cell(ww_world_t *world, ww_vector2_t vec) {
    return ww_world_coord(world, vec) ? world->data[vec.x][vec.y] : WW_CELL_NULL;
}

int ww_world_count8(ww_world_t *world, int i, int j, ww_cell_t cell) {
    ww_vector2_t ids[8] = {
        {i + 1, j},
        {i - 1, j},
        {i, j + 1},
        {i, j - 1},

        {i + 1, j + 1},
        {i - 1, j + 1},
        {i + 1, j - 1},
        {i - 1, j - 1},
    };

    int count = 0;

    for (int i = 0; i < 8; ++i) {
        if (ww_world_cell(world, ids[i]) == cell)
            ++count;
    }

    return count;
}

void ww_cell_update(ww_cell_t cell, int i, int j, ww_world_t *src, ww_world_t *dst) {
    ww_cell_t new_cell = cell;

    switch (cell) {
    case WW_CELL_WIRE: {
        int count = ww_world_count8(src, i, j, WW_CELL_HEAD);

        if (count > 0 && count != 3)
            new_cell = WW_CELL_HEAD;
    } break;

    case WW_CELL_HEAD: {
        new_cell = WW_CELL_TAIL;
    } break;
    
    case WW_CELL_TAIL: {
        new_cell = WW_CELL_WIRE;
    } break;
    }

    dst->data[i][j] = new_cell;
}

void ww_render_grid(SDL_Renderer *renderer) {
    int w = 0, h = 0;

    SDL_GetRendererOutputSize(renderer, &w, &h);
    
    SDL_SetRenderDrawColor(renderer, 0x10, 0x10, 0x10, 0xFF);

    for (int x = 0; x < 512; x += 32) {
        SDL_RenderDrawLine(renderer, x, 0, x, h);
    }

    for (int y = 0; y < 512; y += 32) {
        SDL_RenderDrawLine(renderer, 0, y, w, y);
    }
}

void ww_render_world(SDL_Renderer *renderer, ww_world_t *world) {
    for (int i = 0; i < WW_SIZE; ++i) {
        for (int j = 0; j < WW_SIZE; ++j) {
            const int
                x = i * WW_SCALE,
                y = j * WW_SCALE;
            
            ww_cell_render(world->data[i][j], x, y, renderer);
        }
    }
}

void ww_world_update(ww_world_t *world) {
    ww_world_t temp = {0};

    for (int i = 0; i < WW_SIZE; ++i) {
        for (int j = 0; j < WW_SIZE; ++j) {
            ww_cell_update(world->data[i][j], i, j, world, &temp);
        }
    }

    memcpy(world, &temp, sizeof(ww_world_t));
}

void ww_render_brush(SDL_Renderer *renderer, ww_cell_t brush) {
    int x, y;

    SDL_GetMouseState(&x, &y);

    const int
        i = x / WW_SCALE * WW_SCALE,
        j = y / WW_SCALE * WW_SCALE;

    SDL_Rect rect = {i, j, WW_SCALE, WW_SCALE};

    SDL_Color col = ww_cell_color(brush);

    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);

    SDL_RenderDrawRect(renderer, &rect);
}

typedef struct {
    int running;
    int delay;

    ww_world_t world;
} ww_context_t;

int ww_update_loop(ww_context_t *ctx) {
    while (ctx->running) {
        if (ctx->delay > 0)
            SDL_Delay(ctx->delay);

        if (ctx->delay < 0)
            continue;

        ww_world_update(&ctx->world);
    }
    return 0;
}

int main(int argc, char **argv) {
    int status = SDL_Init(SDL_INIT_EVERYTHING);

    SDL_assert(status == 0);

    ww_context_t ctx = {0};
    
    const int
        wx = SDL_WINDOWPOS_CENTERED,
        wy = SDL_WINDOWPOS_CENTERED,

        ww = WW_SIZE * WW_SCALE,
        wh = WW_SIZE * WW_SCALE;
    
    const Uint32
        rflags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    
    SDL_Window *window = (SDL_Window *)
        SDL_CreateWindow("WireWorld", wx, wy, ww, wh, rflags);
    
    SDL_assert(window != NULL);
    
    SDL_Renderer *renderer = (SDL_Renderer *)
        SDL_CreateRenderer(window, -1, rflags);
    
    SDL_assert(renderer != NULL);

    ww_cell_t brush = WW_CELL_HEAD;

    SDL_Thread *updateThread = (SDL_Thread *)
        SDL_CreateThread((SDL_ThreadFunction) ww_update_loop, "update", &ctx);

    ctx.running = 1;

    ctx.delay = 100;

    while (ctx.running) {        
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                ctx.running = 0;
                break;
            
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    ctx.running = 0;
                    break;
                
                case SDLK_SPACE:
                    ctx.delay = ctx.delay > 0 ? -1 : 100;
                    break;
                }
                break;
            
            case SDL_MOUSEBUTTONDOWN:
                switch (event.button.button) {
                case SDL_BUTTON_LEFT:
                    const int
                        i = event.button.x / WW_SCALE,
                        j = event.button.y / WW_SCALE;
                    
                    ctx.world.data[i][j] = brush;
                    break;
                }
                break;
            
            case SDL_MOUSEMOTION:
                switch (event.motion.state) {
                case SDL_BUTTON_LEFT:
                    const int
                        i = event.button.x / WW_SCALE,
                        j = event.button.y / WW_SCALE;
                    
                    ctx.world.data[i][j] = brush;
                    break;
                }
                break;
            
            case SDL_MOUSEWHEEL:
                if (event.wheel.y > 0 && brush < WW_CELL_HEAD) {
                    ++brush;
                }
                else if (event.wheel.y < 0 && brush > WW_CELL_NULL) {
                    --brush;
                }
                break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);

        SDL_RenderClear(renderer);

        ww_render_world(renderer, &ctx.world);

        ww_render_grid(renderer);

        ww_render_brush(renderer, brush);

        SDL_RenderPresent(renderer);
    }

    SDL_WaitThread(updateThread, NULL);

    SDL_DestroyRenderer(renderer);

    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}