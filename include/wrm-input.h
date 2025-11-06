#ifndef WRM_INPUT_H
#define WRM_INPUT_H

/*
File wrm-input.h

Created  Nov 3, 2025
by William R Mungas (wrm)

Version: 0.1.0 
(Last modified Nov 3, 2025)

DESCRIPTION:
Basic input checking, essentially a wrapper over SDL functions.
Stores the current state of the keyboard and mouse, as well as
whether or not the user has selected an OS event to quit the window

PROVIDES:
- 

REQUIREMENTS:


This is necessary to define the functions declared in this file properly
*/

#include "wrm-common.h"
#include "SDL2/SDL.h"


/*
Type declarations
*/

typedef struct wrm_Key wrm_Key;

typedef struct wrm_Mouse wrm_Mouse;

typedef struct wrm_input_Settings wrm_input_Settings;


/*
Externally visible type definitions
*/

struct wrm_Key {
    bool down; // is the key down?
    u32 counter; // how long the key has been in the current state
};

struct wrm_Mouse {
    i32 x_abs; // current x coordinate
    i32 y_abs; // current y coordinate
    i32 delta_x; // change in x coordinate this frame
    i32 delta_y; // change in y coordinate this frame
    i32 delta_scroll; // change in scroll wheel rotation this frame
    u32 left_counter;
    u32 right_counter;
    u32 middle_counter;
    bool left_button;
    bool right_button;
    bool middle_button;
    bool relative;
};

struct wrm_input_Settings {
    bool errors;
};


/*
Externally visible globals
*/
extern bool wrm_input_should_quit;


/*
Externally visible functions
*/

/* Initialize reading user input */
bool wrm_input_init(wrm_input_Settings *s, SDL_Window *w);

void wrm_input_update(void);

wrm_Key wrm_input_getKey(SDL_Scancode key);

void wrm_input_getMouseState(wrm_Mouse *dest);

void wrm_input_setMouseState(bool relative);

void wrm_input_quit(void);

#endif