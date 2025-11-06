#include "wrm-common.h"
#include "wrm-input.h"


/*
Internal type definitions
*/

DEFINE_LIST(wrm_Key, Key);


/*
Internal helper declarations
*/

/* Update the keys state */
internal inline void wrm_input_updateKeys(void);
/* Update mouse state */
internal inline void wrm_input_updateMouse();


/*
Globals
*/

bool wrm_input_should_quit;

SDL_Window *wrm_window;
internal int wrm_window_width;
internal int wrm_window_height;

internal wrm_List_Key wrm_keys;
internal wrm_Mouse wrm_mouse;
internal wrm_input_Settings wrm_input_settings;


/*
Module function definitions
*/

bool wrm_input_init(wrm_input_Settings *s, SDL_Window *w)
{
    wrm_input_settings = *s;
    wrm_input_should_quit = false;
    wrm_window = w;

    SDL_GetWindowSize(w, &wrm_window_width, &wrm_window_height);

    int num_keys;
    const u8 *sdl_keys = SDL_GetKeyboardState(&num_keys);

    if(num_keys <= 0) {
        if(wrm_input_settings.errors) { fprintf(stderr, "ERROR: Input: init(): invalid SDL keyboard size of [%d]\n", num_keys); }
        return false;
    }

    wrm_keys.cap = (u32)num_keys;
    wrm_keys.data = malloc(wrm_keys.cap * sizeof(wrm_Key));

    if(!wrm_keys.data) {
        if(wrm_input_settings.errors) { fprintf(stderr, "ERROR: Input: init(): unable to allocate keyboard data buffer\n"); }
        return false;
    }

    for(u32 i = 0; i < wrm_keys.cap; i++) {
        wrm_keys.data[i] = (wrm_Key){
            .down = sdl_keys[i],
            .counter = 0
        };
        wrm_keys.len++; // make sure len and cap are the same
    }

    wrm_mouse = (wrm_Mouse){
        .x_abs = 0,
        .y_abs = 0,
        .delta_x = 0,
        .delta_y = 0,
        .delta_scroll = 0,
        .left_counter = 0,
        .right_counter = 0,
        .middle_counter = 0,
        .left_button = false,
        .middle_button = false,
        .right_button = false,
        .relative = false,
    };
    return true;
}

void wrm_input_update(void)
{
    wrm_mouse.delta_scroll = 0.0f;
    SDL_Event e;

    wrm_input_updateMouse();

	while(SDL_PollEvent(&e)) {
        switch(e.type) {
            case SDL_QUIT:
                wrm_input_should_quit = true;
                break;
            case SDL_MOUSEWHEEL:
                wrm_mouse.delta_scroll = e.wheel.y;
                break;
            default:
                continue;
                // do nothing, flush the event
        }
	}

    wrm_input_updateKeys();
}

wrm_Key wrm_input_getKey(SDL_Scancode key)
{
    return wrm_keys.data[key];
}

void wrm_input_getMouseState(wrm_Mouse *dest)
{
    if(dest) {
        *dest = wrm_mouse;
    }
}

void wrm_input_setMouseState(bool relative)
{
    i32 window_center_x = wrm_window_width / 2;
    i32 window_center_y = wrm_window_height / 2;

    wrm_mouse.relative = relative;

    if(relative) {
        SDL_SetRelativeMouseMode(SDL_TRUE);
        wrm_mouse.x_abs = window_center_x;
        wrm_mouse.y_abs = window_center_y;
    }
    else {
        SDL_WarpMouseInWindow(wrm_window, window_center_x, window_center_y);
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_ShowCursor(SDL_ENABLE);
        wrm_mouse.x_abs = window_center_x;
        wrm_mouse.y_abs = window_center_y;
    }
}

void wrm_input_quit(void)
{
    free(wrm_keys.data);
}


/*
Internal helper definitions
*/

internal inline void wrm_input_updateMouse()
{
    u32 button_bits = SDL_GetRelativeMouseState(&wrm_mouse.delta_x, &wrm_mouse.delta_y);

    if(!wrm_mouse.relative) {
        wrm_mouse.x_abs += wrm_mouse.delta_x;
        wrm_mouse.y_abs += wrm_mouse.delta_y;
    }

    bool left = SDL_BUTTON(1) & button_bits;
    bool middle = SDL_BUTTON(2) & button_bits;
    bool right = SDL_BUTTON(3) & button_bits;

    wrm_mouse.left_counter++;
    wrm_mouse.right_counter++;
    wrm_mouse.middle_counter++;

    if(wrm_mouse.left_button != left) wrm_mouse.left_counter = 0;
    if(wrm_mouse.middle_button != middle) wrm_mouse.middle_counter = 0;
    if(wrm_mouse.right_button != right) wrm_mouse.right_counter = 0;

    wrm_mouse.left_button = left;
    wrm_mouse.middle_button = middle;
    wrm_mouse.right_button = right;

}


internal inline void wrm_input_updateKeys(void)
{
    // update keyboard
    const u8 *sdl_keys = SDL_GetKeyboardState(NULL);
    for(u32 i = 0; i < wrm_keys.len; i++) {
        wrm_Key *key = wrm_keys.data + i;
        bool is_pressed = sdl_keys[i];
        bool was_pressed = key->down;

        key->down = is_pressed;
        key->counter++;
        if(was_pressed != is_pressed) {
            key->counter = 0;
        }
    }
}