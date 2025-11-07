#include "wrm-common.h"
#include "wrm-input.h"
#include "wrm-render.h"
#include "wrm-memory.h"
#include "stb/stb_image.h"
#include "boids-world.h"

/*
Internal type definitions
*/

DEFINE_LIST(wrm_Handle, Handle);
DEFINE_LIST(wrm_List_Handle, List_Handle);

/*
Constants
*/

// default control settings: TODO: make this readable from a file instead

internal SDL_Scancode BOIDS_FORWARD = SDL_SCANCODE_W;
internal SDL_Scancode BOIDS_LEFT = SDL_SCANCODE_A;
internal SDL_Scancode BOIDS_RIGHT = SDL_SCANCODE_D;
internal SDL_Scancode BOIDS_BACKWARD = SDL_SCANCODE_S;
internal SDL_Scancode BOIDS_UP = SDL_SCANCODE_SPACE;
internal SDL_Scancode BOIDS_DOWN = SDL_SCANCODE_LCTRL;

internal float BOIDS_SENSITIVITY_X = 0.3f; 
internal float BOIDS_SENSITIVITY_Y = 0.3f;
internal bool INVERTED = false;

/*
Globals
*/

// player-related

vec3 player_vel; // the player's velocity
vec3 player_pos; // the player's (camera's) position
float player_accel; // determines how fast inputs add to player velocity
float player_max_speed; // cap on player speed
float player_min_speed; // speed below which player is clamped to zero
float player_decel; // determines how fast the player's velocity decreases without input

float player_pitch; // the player's (camera's) pitch
float player_yaw; // the player's (camera's) yaw
float player_fov; // the player's (camera's) fov
bool player_inverted;

// screen controls-related
bool has_mouse;

// boids-related

wrm_Pool the_boids; // this sounds ominous as hell lmao
wrm_Pool the_obstacles;

wrm_List_List_Handle; // this is some goofy shit

/*
Internal helper declarations
*/

/* Loads an image to the renderer */
internal wrm_Option_Handle boids_loadImage(const char *path);
/* Loads text from a file; returned pointer must be freed by user */
internal char *boids_loadText(const char *path, u32 *length);
/* Handles user input */
internal void boids_handlePlayerControls(float delta_time);

/*
Module functions
*/

bool boids_world_init(void)
{
    // for image loading
    stbi_set_flip_vertically_on_load(true);

    // load resources, set up lists/pools, etc

    glm_vec3_zero(player_pos);
    glm_vec3_zero(player_vel);
    
    player_accel = 0.3f;
    player_decel = -0.8f;
    player_max_speed = 0.5f;
    player_min_speed = 0.001f;

    player_pitch = 0.0f;
    player_yaw = 0.0f;
    player_fov = 70.0f;

    player_inverted = true;
    
    has_mouse = true;
    wrm_input_setMouseState(has_mouse);

    return true;
}

void boids_world_update(float delta_time)
{
    // handle 'player' controls    
    boids_handlePlayerControls(delta_time);
        
    
    glm_vec3_add(player_pos, player_vel, player_pos);

    wrm_Key esc = wrm_input_getKey(SDL_SCANCODE_ESCAPE);
    if(esc.down && !esc.counter) {
        has_mouse = false;
        wrm_input_setMouseState(has_mouse);
    }


    // could apply a cool fov effect if moving, based on the player's acceleration value

    wrm_Mouse m;
    wrm_input_getMouseState(&m);
    if(m.left_button && !m.left_counter) {
        if(!has_mouse) {
            has_mouse = true;
            wrm_input_setMouseState(has_mouse);
        }
        // spawn boids around cursor
    }
    if(m.right_button && !m.right_counter) {
        // remove boids around cursor
    }
    
    
    // chunk the total area

    // update all boids based on the boids in their vicinity

    // update render data
    wrm_render_updateCamera(player_pitch, player_yaw, player_fov, 0.0f, player_pos);
}

void boids_world_quit(void)
{

}


/*
Internal helper definitions
*/

wrm_Option_Handle boids_loadImage(const char *path)
{
    int width, height, nrChannels;
    u8 *pixels = stbi_load("./resources/container.jpg", &width, &height, &nrChannels, 4); 

    if(!pixels) {
        fprintf(stdout, "ERROR: Render: failed to load test image\n");
        return OPTION_NONE(Handle);
    }

    wrm_Option_Handle result = wrm_render_createTexture(&(wrm_Texture_Data) {.pixels = pixels, .height = height, .width = width});
    stbi_image_free(pixels);
    return result;
}

internal char *boids_loadText(const char *path, u32 *length)
{
    FILE *fp = fopen(path, "r");

    fseek(fp, 0L, SEEK_END);
    i64 size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // don't forget the NULL
    char *text = malloc(size + 1);
    size_t n = fread(text, sizeof(char), size, fp);
    if(n < size) {
        *length = 0;
        free(text);
        fclose(fp);
        return NULL;
    }
    *length = size;
    text[size] = '\0';

    fclose(fp);
    return text;
}

internal void boids_handlePlayerControls(float delta_time)
{
    // only grab changes if the mouse is captured
    if(has_mouse) {
        wrm_Mouse m;
        wrm_input_getMouseState(&m);

        if(player_inverted) {
            player_pitch -= (float)m.delta_y * BOIDS_SENSITIVITY_Y;
        }
        else {
            player_pitch += (float)m.delta_y * BOIDS_SENSITIVITY_Y;
        }
        // bound the pitch so we don't flip the camera
        if(player_pitch > 89.5f) { player_pitch = 89.5f; }
        if(player_pitch < -89.5f) { player_pitch = -89.5f; }

        player_yaw += (float)m.delta_x * BOIDS_SENSITIVITY_X;

        if(m.delta_scroll > 0) {
            player_accel *= 0.8f;
        }
        if(m.delta_scroll < 0) {
            player_accel /= 0.8f;
        }
    }
    

    vec3 forward = { cosf(glm_rad(player_yaw)), 0.0f, sinf(glm_rad(player_yaw)) };
    vec3 up = {0.0f, 1.0f, 0.0f};
    vec3 right;

    glm_normalize(forward);
    glm_cross(forward, up, right);
    glm_normalize(up);

    vec3 accel;
    glm_vec3_zero(accel);

    if(has_mouse) {
        if(wrm_input_getKey(BOIDS_FORWARD).down) {
            glm_vec3_add(accel, forward, accel);    
        }
        if(wrm_input_getKey(BOIDS_RIGHT).down) {
            glm_vec3_add(accel, right, accel);
        }
        if(wrm_input_getKey(BOIDS_UP).down) {
            glm_vec3_add(accel, up, accel);
        }
        if(wrm_input_getKey(BOIDS_BACKWARD).down) {
            glm_vec3_negate(forward);
            glm_vec3_add(accel, forward, accel);
        }
        if(wrm_input_getKey(BOIDS_LEFT).down) {
            glm_vec3_negate(right);
            glm_vec3_add(accel, right, accel);
        }
        if(wrm_input_getKey(BOIDS_DOWN).down) {
            glm_vec3_negate(up);
            glm_vec3_add(accel, up, accel);
        }
    }

    // get acceleration or deceleration
    float scale = delta_time;
    
    if(glm_vec3_norm(accel) < 0.9f) {
        scale *= ((glm_vec3_norm(player_vel) / player_max_speed) + 0.2f) * player_decel ;
        glm_vec3_copy(player_vel, accel);
    }
    else {
        scale *= player_accel;
    }

    glm_vec3_normalize(accel);
    glm_vec3_scale(accel, scale, accel);
    glm_vec3_add(accel, player_vel, player_vel);
    
    // cap the speed
    float magnitude = glm_vec3_norm(player_vel);
    if(magnitude > player_max_speed) {
        glm_vec3_scale(player_vel, player_max_speed / magnitude, player_vel);
    }
    if(magnitude < player_min_speed) {
        glm_vec3_zero(player_vel);
    }
}



