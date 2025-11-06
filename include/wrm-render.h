#ifndef WRM_RENDER_H
#define WRM_RENDER_H

/*
File wrm-render.h

Created Oct 29, 2025 
by William R Mungas (wrm)

Version: 0.1.0 
(Last modified Nov 2, 2025)

DESCRIPTION:
Rendering framework that uses SDL to create a window, which is exposed externally 
for wrm-input. Currently uses cglm as a vector/matrix gl-optimized math library

In the future, I may abstract general window functionality to a separate header called
wrm-window or something similar, so that this and wrm-input are less tightly coupled. 
I may also provide a rendering backend based on Vulkan instead of OpenGL.

PROVIDES:
- rendering primitives: shader, texture, mesh, model
- access to a default camera

REQUIREMENTS:
- SDL2 must be installed for headers/linking
- cglm/ must also be in your project's includes, used as header-only lib
*/

#include "wrm-common.h"
#include "SDL2/SDL.h"
#include "cglm/cglm.h"


/*
Type Declarations
*/

// window creation arguments
typedef struct wrm_Window_Data wrm_Window_Data; 
typedef struct wrm_render_Settings wrm_render_Settings;

// shader-related
typedef struct wrm_Shader_Defaults wrm_Shader_Defaults;

// Color and texture related types

// single 32-bit integer rgba value
typedef u32 wrm_RGBA;
// collection of 4 bytes: r, g, b, a
typedef struct wrm_RGBAi wrm_RGBAi;
// collection of 4 floats: r, g, b, a: used for when OpenGL wants color channel values as floats in the range [0.0f, 1.0f]
typedef struct wrm_RGBAf wrm_RGBAf;
// represents a texture
typedef struct wrm_Texture wrm_Texture;
// data for creating a texture
typedef struct wrm_Texture_Data wrm_Texture_Data;

// Mesh types

// generic mesh type: may have either per-vertex uvs or per-vertex RGBA colors
typedef struct wrm_Mesh wrm_Mesh;
// Arguments for mesh creation
typedef struct wrm_Mesh_Data wrm_Mesh_Data;
// Enum of Mesh types
typedef enum wrm_Mesh_Type wrm_Mesh_Type;

// Represents a mesh plus a transform
typedef struct wrm_Model wrm_Model;

/*
Externally visible constants
*/

extern const u32 WRM_DEFAULT_WINDOW_WIDTH;
extern const u32 WRM_DEFAULT_WINDOW_HEIGHT;

// bit masks for colors 

#define WRM_RGBA_R_BITS 0xff000000u
#define WRM_RGBA_G_BITS 0x00ff0000u
#define WRM_RGBA_B_BITS 0x0000ff00u
#define WRM_RGBA_A_BITS 0x000000ffu

// a few simple predefined colors

#define WRM_RGBA_BLACK 0x000000ffu
#define WRM_RGBA_WHITE 0xffffffffu

#define WRM_RGBA_RED 0xff0000ffu
#define WRM_RGBA_GREEN 0x00ff00ffu
#define WRM_RGBA_BLUE 0x0000ffffu

#define WRM_RGBA_PURPLE 0xff00ffffu
#define WRM_RGBA_YELLOW 0xffff00ffu
#define WRM_RGBA_CYAN 0x00ffffffu

// Externally visible type definitions

struct wrm_Shader_Defaults {
    wrm_Handle color;
    wrm_Handle texture;
    wrm_Handle both;
};

struct wrm_RGBAi {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};

struct wrm_RGBAf {
    float r;
    float g;
    float b;
    float a;
};

struct wrm_Window_Data {
    const char *name;
    u32 height_px;
    u32 width_px;
    bool is_resizable;
    wrm_RGBAf background;
};

struct wrm_render_Settings {
    bool verbose;
    bool errors;
    bool test;
};

struct wrm_Texture_Data {
    u8 *pixels; // must be 4 * width * height long
    u32 width;
    u32 height;
};

struct wrm_Mesh_Data {
    float *positions;       // position for each vertex
    float *colors;          // RGBA color for each vertex
    float *uvs;             // uv for each vertex
    u32 *indices;           // vertex indices
    bool cw;                // clockwise winding order?
    size_t vtx_cnt;         // number of vertices for which we have data (independent of number of triangles)
    size_t tri_cnt;         // the number of triangles in the mesh
};

struct wrm_Model { 
    // these are relative to the parent model
    vec3 pos;
    vec3 rot;
    vec3 scale;

    wrm_Handle mesh;
    wrm_Handle texture; // only used when the model has a textured mesh; for now, meshes only use a single texture
    wrm_Handle shader;

    bool is_visible;
    bool is_ui; // whether to draw as part of the UI or not

    // likely gonna be unused for a while
    wrm_Handle parent;
    u32 child_count;
    wrm_Handle *children;
};

DEFINE_OPTION(wrm_Model, Model);

// Externally visible values

extern wrm_Shader_Defaults wrm_shader_defaults;

// Externally visible functions

/*
Initialize the renderer - opens a window with a GL context, sets up default shaders 
*/
bool wrm_render_init(const wrm_render_Settings *settings, const wrm_Window_Data *data);

/*
Shut down the renderer and clean up resources
*/
void wrm_render_quit(void);

/*
Cleans up any unused/freed resources and renders all currently visible objects
*/
void wrm_render_draw(float delta_time);


/*
Get the window created by the render
May be needed by other modules
*/
SDL_Window *wrm_render_getWindow(void);

/*
Creates a shader program using the given frag and vert
*/
wrm_Option_Handle wrm_render_createShader(const char *vert, const char *frag, bool needs_col, bool needs_tex);

/*
Create a texture
*/
wrm_Option_Handle wrm_render_createTexture(const wrm_Texture_Data *data);

/* Create a mesh */
wrm_Option_Handle wrm_render_createMesh(const wrm_Mesh_Data *data);
/* Clones an existing mesh (useful for changing data without affecting all instances of this mesh)*/
wrm_Option_Handle wrm_render_cloneMesh(wrm_Handle mesh);
/* Updates a mesh's data: IMPORTANT: will update ALL existing instances of this mesh */
bool wrm_render_updateMesh(wrm_Handle mesh, const wrm_Mesh_Data *data);

// model-related

/* Create a model - if use_default_shader is true, the renderer will attempt to select a default shader based on the mesh attributes */
wrm_Option_Handle wrm_render_createModel(const wrm_Model *data, bool use_default_shader);
/* Get a model*/
wrm_Option_Model wrm_render_getModel(wrm_Handle model);
/* Update a model's position data */
void wrm_render_updateModelTransform(wrm_Handle model, const vec3 position, const vec3 facing, const vec3 up);
/* Update a model's mesh */
void wrm_render_updateModelMesh(wrm_Handle model, wrm_Handle mesh);
/* Update a model's texture*/
void wrm_render_updateModelTexture(wrm_Handle model, wrm_Handle texture);
/* Update a model's shader */
void wrm_render_updateModelShader(wrm_Handle model, wrm_Handle shader);
/* Update a model as a whole data set*/
void wrm_render_updateModel(wrm_Handle model, const wrm_Model *data);

// camera-related

/* unusable at the moment; might be used later for projects where multiple cameras may be required */
// wrm_Option_Handle wrm_render_createCamera();
/* Updates the viewing camera */
void wrm_render_updateCamera(float pitch, float yaw, float fov, float offset, const vec3 position);
/* Gets the render's camera data, stores results in the provided pointers */
void wrm_render_getCameraData(float *pitch, float *yaw, float *fov, float *offset, vec3 position);

/* Self-explanatory RGBA functions: inlined because they are very small */

inline wrm_RGBA wrm_RGBA_fromRGBAi(wrm_RGBAi rgbai) 
{
    return (u32)rgbai.r << 24 | (u32)rgbai.g << 16 | (u32)rgbai.b << 8 | (u32)rgbai.a;
}

inline wrm_RGBA wrm_RGBA_fromRGBAf(wrm_RGBAf rgbaf)
{
    return (u32)(rgbaf.r * 255.0) << 24 | (u32)(rgbaf.g * 255.0) << 24 | (u32)(rgbaf.b * 255.0) << 24 | (u32)(rgbaf.a * 255.0);
}

inline wrm_RGBAi wrm_RGBAi_fromRGBA(wrm_RGBA rgba)
{
    return (wrm_RGBAi){
        .r = (u8)((rgba & WRM_RGBA_R_BITS) >> 24),
        .g = (u8)((rgba & WRM_RGBA_G_BITS) >> 16),
        .b = (u8)((rgba & WRM_RGBA_B_BITS) >> 8),
        .a = (u8)(rgba & WRM_RGBA_A_BITS)
    };
}

inline wrm_RGBAi wrm_RGBAi_fromRGBAf(wrm_RGBAf rgbaf)
{
    return wrm_RGBAi_fromRGBA(wrm_RGBA_fromRGBAf(rgbaf));
}

inline wrm_RGBAf wrm_RGBAf_fromRGBA(wrm_RGBA rgba)
{
    return (wrm_RGBAf) {
        .r = (float)((rgba & WRM_RGBA_R_BITS) >> 24) / 255.0f,
        .g = (float)((rgba & WRM_RGBA_G_BITS) >> 16) / 255.0f,
        .b = (float)((rgba & WRM_RGBA_B_BITS) >> 8) / 255.0f,
        .a = (float)(rgba & WRM_RGBA_A_BITS) / 255.0f
    };
}

inline wrm_RGBAf wrm_RGBAf_fromRGBAi(wrm_RGBAi rgbai)
{
    return wrm_RGBAf_fromRGBA(wrm_RGBA_fromRGBAi(rgbai));
}



#endif
