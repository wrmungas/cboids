#include "wrm-common.h"
#include "wrm-render.h"
#include "wrm-memory.h"
#include "stb/stb_image.h"
#include "glad/glad.h"

/*
Internal type definitions
*/

// shader GL data, plus requirements of meshes rendered with it
typedef struct wrm_Shader {
    bool needs_col;
    bool needs_tex;
    GLuint vert;
    GLuint frag;
    GLuint program;
} wrm_Shader;

struct wrm_Texture {
    GLuint gl_tex;
    // mipmap settings?
    // filter settings?
    u32 w;
    u32 h;
};

struct wrm_Mesh {
    GLuint vao;
    GLuint pos_vbo;
    GLuint uv_vbo;
    GLuint col_vbo;
    GLuint ebo;
    size_t tri_cnt;
    bool cw;
};

// camera data
typedef struct wrm_Camera {
    float pitch;
    float yaw;
    float offset; // distance forward (positive) or backward (negative) along `facing` from `pos`: used for 3rd-person controls
    float fov;
    vec3 pos;
} wrm_Camera;

DEFINE_OPTION(GLuint, GLuint);

DEFINE_LIST(wrm_Model, Model);

/*
Constants
*/

// windowing constants

const u32 WRM_DEFAULT_WINDOW_HEIGHT = 680;
const u32 WRM_DEFAULT_WINDOW_WIDTH = 800;

// resource constants
typedef enum wrm_render_Resource_Type {
    WRM_RENDER_RESOURCE_MODEL, 
    WRM_RENDER_RESOURCE_MESH, 
    WRM_RENDER_RESOURCE_SHADER, 
    WRM_RENDER_RESOURCE_TEXTURE
} wrm_render_Resource_Type;

// general rendering constants
internal const float WRM_NEAR_CLIP_DISTANCE = 0.001f;
internal const float WRM_FAR_CLIP_DISTANCE = 1000.0f;
// shader constants

wrm_Shader_Defaults wrm_shader_defaults;

internal const u32 WRM_SHADER_ATTRIB_POS_LOC = 0;
internal const u32 WRM_SHADER_ATTRIB_COL_LOC = 1;
internal const u32 WRM_SHADER_ATTRIB_UV_LOC = 2;
// internal const u32 WRM_SHADER_ATTRIB_NORM_LOC = 3; // unused (yet)

internal const char *WRM_SHADER_DEFAULT_COL_V_TEXT = 
"#version 330 core\n"
"layout (location = 0) in vec3 v_pos;\n" // positions are location 0
"layout (location = 1) in vec4 v_col;\n" // colors are location 1
"uniform mat4 model;\n"
"uniform mat4 persp;\n"
"uniform mat4 view;\n"
"out vec4 col;\n" // specify a color output to the fragment shader
"void main()\n"
"{\n"
"    gl_Position = persp * view * model * vec4(v_pos, 1.0);\n"
"    col = v_col;\n"
"}\n";
internal const char *WRM_SHADER_DEFAULT_COL_F_TEXT = 
"#version 330 core\n"
"in vec4 col;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"    FragColor = col;\n"
"}\n";

// texture

internal const char *WRM_SHADER_DEFAULT_TEX_V_TEXT = 
"#version 330 core\n"
"layout (location = 0) in vec3 v_pos;\n" // positions are location 0
"layout (location = 2) in vec2 v_uv;\n"  // uvs are location 2
"uniform mat4 model;\n"
"uniform mat4 persp;\n"
"uniform mat4 view;\n"
"out vec2 uv;\n" // specify a uv for the fragment shader
"void main()\n"
"{\n"
"    gl_Position = persp * view * model * vec4(v_pos, 1.0);\n" 
"    uv = v_uv;\n"
"}\n";
internal const char *WRM_SHADER_DEFAULT_TEX_F_TEXT = 
"#version 330 core\n"
"in vec2 uv;\n"
"uniform sampler2D tex;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"    FragColor = texture(tex, uv);\n"
"}\n";

// both
internal const char *WRM_SHADER_DEFAULT_BOTH_V_TEXT = 
"#version 330 core\n"
"layout (location = 0) in vec3 v_pos;\n" // positions are location 0
"layout (location = 1) in vec4 v_col;\n"
"layout (location = 2) in vec2 v_uv;\n"  // uvs are location 2
"uniform mat4 model;\n"
"uniform mat4 persp;\n"
"uniform mat4 view;\n"
"out vec4 col;\n" // specify a color for the fragment shader
"out vec2 uv;\n" // specify a uv for the fragment shader\n"
"void main()\n"
"{\n"
"    gl_Position = persp * view * model * vec4(v_pos, 1.0);\n"
"    col = v_col;\n" 
"    uv = v_uv;\n"
"}\n";
internal const char *WRM_SHADER_DEFAULT_BOTH_F_TEXT = 
"#version 330 core\n"
"in vec4 col;\n"
"in vec2 uv;\n"
"uniform sampler2D tex;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"    FragColor = texture(tex, uv) * col;\n"
"}\n";

// default meshes
wrm_Mesh_Data default_color_mesh_data = {
    .positions = (float[]) {
        -0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
    },
    .colors = (float[]) {
        0.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
    },
    .uvs = NULL,
    .indices = (u32[]) {
        0, 1, 3, 0, 3, 2,
        1, 5, 7, 1, 7, 3,
        5, 4, 6, 5, 6, 7,
        4, 0, 2, 4, 2, 6,
        4, 5, 1, 4, 1, 0,
        2, 3, 7, 2, 7, 6,
    },
    .cw = true,
    .tri_cnt = 12,
    .vtx_cnt = 8
};

wrm_Mesh_Data default_texture_mesh_data = {
    .positions = (float[]) {
        
    },
    .colors = (float[]) {

    },
    .uvs = (float[]) {

    },
    .indices = (u32[]) {

    },
    .cw = true,
    .tri_cnt = 12,
    .vtx_cnt = 24,
};

// pool constants

internal const u32 WRM_RENDER_POOL_INITIAL_CAPACITY = 20;

// list constants

internal const u32 WRM_RENDER_LIST_INITIAL_CAPACITY = 10;
internal const u32 WRM_RENDER_LIST_SCALE_FACTOR = 2;


/*
Internal helper declarations
*/

// initializes the internal renderer resource pools
internal void wrm_render_initLists(void);
// compiles an individual shader program of the given GL type (GL_VERTEX_SHADER, GL_FRAGMENT_SHADER)
internal wrm_Option_GLuint wrm_render_compileShader(const char *shader_text, GLenum type);
// comparison function for sorting models: 
internal int wrm_render_compareModels(const void *model1, const void *model2);
// creates a default shader for meshes with per-vertex colors, per-vertex uv's, and both
internal void wrm_render_createDefaultShaders(void);
// creates a default pink-and-black error texture
internal void wrm_render_createErrorTexture(void);
// creates a default test triangle
internal void wrm_render_createTestModel(void);
// creates a list from the pool of models, sorted by GL state changes
internal inline void wrm_render_prepareModels(void);
// checks whether certain resources are in use
internal inline bool wrm_render_isInUse(wrm_Handle h, wrm_render_Resource_Type t, const char *caller);
// gets the view matrix from the current camera orientation
internal inline void wrm_render_getViewMatrix(mat4 view);
// sets the GL state before a draw call
internal inline void wrm_render_setGLState(wrm_Model *curr, wrm_Model *prev, mat4 model, mat4 view, mat4 persp, u32 *elements);


/*
Module global variables
*/

// Overall module status

internal bool wrm_render_is_initialized = false;
internal wrm_render_Settings wrm_render_settings;

// SDL data 

internal SDL_Window *wrm_window = NULL; // SDL window 
internal SDL_GLContext wrm_gl_context; // gl context obtained from SDL

// GL data

internal wrm_RGBAf wrm_bg_color; // background color
internal int wrm_window_height;
internal int wrm_window_width;
internal vec3 wrm_world_up = {0.0f, 1.0f, 0.0f};
internal wrm_Camera wrm_camera;

// resource pools

wrm_Pool wrm_shaders;
wrm_Pool wrm_meshes;
wrm_Pool wrm_textures;
wrm_Pool wrm_models;

/* a list of models to be drawn (used solely in render_draw() )*/
wrm_List_Model wrm_models_tbd;
/* a list of UI models (to be drawn with orthographic projection)*/
wrm_List_Model wrm_ui_tbd;

/*
Module function definitions
*/


// module

bool wrm_render_init(const wrm_render_Settings *s, const wrm_Window_Data *data)
{
    wrm_render_settings = *s;

    if(SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "ERROR: Render: failed to initialize SDL\n");
        return false;
    }
    if(wrm_render_settings.verbose) printf("Render: initialized SDL\n");

    // set gl attributes: version 3.3 core, with double-buffering
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    u32 sdl_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
    if(data->is_resizable) { sdl_flags |= SDL_WINDOW_RESIZABLE; }

    // create the window
    wrm_window = SDL_CreateWindow(
        data->name,
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        data->width_px, data->height_px,
        sdl_flags
    );
    SDL_GL_GetDrawableSize(wrm_window, &wrm_window_width, &wrm_window_height);
    if(!wrm_window) {
        fprintf(stderr, "ERROR: Render: Failed to create window\n");
        return false;
    }
    if(wrm_render_settings.verbose) printf("Render: created window\n");

    // get the gl context and load functions
    wrm_gl_context = SDL_GL_CreateContext(wrm_window);
    if(!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        fprintf(stderr, "ERROR: Render: Failed to initialize GL functions\n");
        return false;
    }
    if(wrm_render_settings.verbose) printf("Render: loaded GL functions\n");

    // setup resource lists
    wrm_render_initLists();
    if(wrm_render_settings.verbose) printf(
        "Render: created resource pools\n"
        "\tmodels[cap=%zu,size=%zu]\n"
        "\tmeshes[cap=%zu,size=%zu]\n"
        "\ttextures[cap=%zu,size=%zu]\n"
        "\tshaders[cap=%zu,size=%zu]\n",
        wrm_models.cap, wrm_models.used,
        wrm_meshes.cap, wrm_meshes.used,
        wrm_textures.cap, wrm_textures.used,
        wrm_shaders.cap, wrm_shaders.used
    );

    // add default resources to each list: the handle value 0 refers to these
    // setup default shaders
    wrm_render_createDefaultShaders();
    if(wrm_render_settings.verbose) printf("Render: created default shaders\n");
    // setup default texture
    wrm_render_createErrorTexture();
    // setup default mesh (creates default model as well)
    wrm_render_createTestModel();
    if(wrm_render_settings.verbose) printf(
        "Render: initialized resources\n"
        "\tmodels[cap=%zu,size=%zu]\n"
        "\tmeshes[cap=%zu,size=%zu]\n"
        "\ttextures[cap=%zu,size=%zu]\n"
        "\tshaders[cap=%zu,size=%zu]\n",
        wrm_models.cap, wrm_models.used,
        wrm_meshes.cap, wrm_meshes.used,
        wrm_textures.cap, wrm_textures.used,
        wrm_shaders.cap, wrm_shaders.used
    );

    // initialize GL data
    wrm_bg_color = data->background;
    glViewport(0, 0, wrm_window_width, wrm_window_height);

    // initialize camera
    wrm_camera = (wrm_Camera){
        .fov = 70.0f,
        .offset = 0.0f,
        .pitch = 0.0f,
        .yaw = 0.0f
    };

    wrm_render_is_initialized = true;
    return true;
}

void wrm_render_quit(void)
{
    if(!wrm_render_is_initialized) return;

    wrm_Pool_delete(&wrm_shaders);
    wrm_Pool_delete(&wrm_textures);
    wrm_Pool_delete(&wrm_meshes);
    wrm_Pool_delete(&wrm_models);

    free(wrm_models_tbd.data);

    SDL_GL_DeleteContext(wrm_gl_context);
    
    if(wrm_window) {
        SDL_DestroyWindow(wrm_window);
        wrm_window = NULL;
    }

    SDL_Quit();
}

void wrm_render_draw(float delta_time /*, bool show_ui*/) 
{
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    // clear the screen
    glClearColor(wrm_bg_color.r, wrm_bg_color.g, wrm_bg_color.b, wrm_bg_color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // handle camera and get view matrix
    mat4 view;
    wrm_render_getViewMatrix(view);

    // get the perspective projection matrix (account for changes in window dimensions and camera fov)
    mat4 persp;
    float aspect_ratio = (float) wrm_window_width / (float) wrm_window_height;
    glm_perspective(wrm_camera.fov, aspect_ratio, WRM_NEAR_CLIP_DISTANCE, WRM_FAR_CLIP_DISTANCE, persp);

    // prepare a list of models for rendering
    wrm_render_prepareModels();
    
    // initialize GL state and tracking of changes
    wrm_Model *prev = NULL;
    wrm_Model *curr = wrm_models_tbd.data;
    u32 elements = 0;

    mat4 model;
    

    // render all the models to backbuffer
    for(int i = 0; i < wrm_models_tbd.len; i++) {

        wrm_render_setGLState(curr, prev, model, view, persp, &elements);

        // render
        glDrawElements(GL_TRIANGLES, elements, GL_UNSIGNED_INT, NULL);

        prev = curr;
        curr++;
    }

    // space for future post-processing effects

    // space for UI rendering pass
    // use orthographic projection
    glm_ortho(0.0f, wrm_window_width, 0.0f, wrm_window_height, 0.0f, 1.0f, view);
    glDisable(GL_DEPTH_TEST);

    for(int i = 0; i < wrm_ui_tbd.len; i++) {

    }


    // swap the buffers to present the completed frame
    SDL_GL_SwapWindow(wrm_window);
}

SDL_Window *wrm_render_getWindow(void)
{
    return wrm_window;
}

// shader

wrm_Option_Handle wrm_render_createShader(const char *vert_text, const char *frag_text, bool needs_col, bool needs_tex)
{
    wrm_Option_Handle pool_result = wrm_Pool_getSlot(&wrm_shaders);

    if(!pool_result.exists) return pool_result;

    wrm_Shader s;
    s.needs_col = needs_col;
    s.needs_tex = needs_tex;

    // first compile the vertex and fragment shaders individually
    wrm_Option_GLuint result = wrm_render_compileShader(vert_text, GL_VERTEX_SHADER);
    if(!result.exists) {
        wrm_Pool_freeSlot(&wrm_shaders, pool_result.Handle_val);
        return (wrm_Option_Handle){.exists = false };
    }
    s.vert = result.GLuint_val;
    
    result = wrm_render_compileShader(frag_text, GL_FRAGMENT_SHADER);
    if(!result.exists) {
        wrm_Pool_freeSlot(&wrm_shaders, pool_result.Handle_val);
        glDeleteShader(s.vert);
        return (wrm_Option_Handle){.exists = false };
    }
    s.frag = result.GLuint_val;
    
    // then link them to form a program

    GLuint program = glCreateProgram();

    glAttachShader(program, s.vert);
    glAttachShader(program, s.frag);

    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if(success == GL_FALSE) {
        if(wrm_render_settings.errors) {
            GLint log_len = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);

            char *log_msg = malloc(log_len * sizeof(char));
            glGetProgramInfoLog(program, log_len, NULL, log_msg);
            fprintf(stderr, "ERROR: Render: failed to link shaders, GL error: %s\n", log_msg);
            free(log_msg);
        }

        glDeleteProgram(program);
        glDeleteShader(s.vert);
        glDeleteShader(s.frag);

        wrm_Pool_freeSlot(&wrm_shaders, pool_result.Handle_val);
        return (wrm_Option_Handle){ .exists = false };
    }

    glDetachShader(program, s.vert);
    glDetachShader(program, s.frag);

    s.program = program;

    if (s.needs_tex) {
    glUseProgram(program);
    GLint tex_uniform = glGetUniformLocation(program, "tex");
    if (tex_uniform != -1) {
        glUniform1i(tex_uniform, 0); // Assumes all your textured shaders use GL_TEXTURE0: can later extend to use multiple textures
    }
    glUseProgram(0); // Optional: Unbind the program
}

    ((wrm_Shader*)wrm_shaders.data)[pool_result.Handle_val] = s;
    return pool_result;
}

// texture

wrm_Option_Handle wrm_render_createTexture(const wrm_Texture_Data *data)
{
    wrm_Option_Handle result = wrm_Pool_getSlot(&wrm_textures);

    if(!result.exists) return result;

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(
        GL_TEXTURE_2D,  // texture target type
        0,              // detail level (for manually adding mipmaps; don't do this, generate them with glGenerateMipmap)
        GL_RGBA,        // format OpenGL should store the image with
        data->width,    // width in pixels
        data->height,   // height in pixels
        0,              // border (weird legacy argument - borders should be set explicitly with glTexParameterxx)
        GL_RGBA,        // format of the incoming image data
        GL_UNSIGNED_BYTE, // size of list entries
        data->pixels    // image data list
    );
    glGenerateMipmap(GL_TEXTURE_2D);

    ((wrm_Texture*)wrm_textures.data)[result.Handle_val] = (wrm_Texture){
        .gl_tex = texture,
        .w = data->width,
        .h = data->height
    };

    return result;
}

// mesh

wrm_Option_Handle wrm_render_createMesh(const wrm_Mesh_Data *data)
{
    wrm_Option_Handle result = wrm_Pool_getSlot(&wrm_meshes);
    GLuint vtx_attrib;

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint pos_vbo = 0;
    glGenBuffers(1, &pos_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, pos_vbo);
    glBufferData(GL_ARRAY_BUFFER, data->vtx_cnt * 3 * sizeof(float), data->positions, GL_STATIC_DRAW);

    vtx_attrib = WRM_SHADER_ATTRIB_POS_LOC;
    glVertexAttribPointer(vtx_attrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(vtx_attrib);

    GLuint col_vbo = 0;
    if(data->colors) {
        glGenBuffers(1, &col_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, col_vbo);
        glBufferData(GL_ARRAY_BUFFER, data->vtx_cnt * 4 * sizeof(float), data->colors, GL_STATIC_DRAW);

        vtx_attrib = WRM_SHADER_ATTRIB_COL_LOC;
        glVertexAttribPointer(vtx_attrib, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(vtx_attrib);
    }

    GLuint uv_vbo = 0;
    if(data->uvs) {
        glGenBuffers(1, &uv_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, uv_vbo);
        glBufferData(GL_ARRAY_BUFFER, data->vtx_cnt * 2 * sizeof(float), data->uvs, GL_STATIC_DRAW);

        vtx_attrib = WRM_SHADER_ATTRIB_UV_LOC;
        glVertexAttribPointer(vtx_attrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(vtx_attrib);
    }

    GLuint ebo = 0;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, data->tri_cnt * 3 * sizeof(u32), data->indices, GL_STATIC_DRAW);

    wrm_Mesh m = {
        .vao = vao,
        .pos_vbo = pos_vbo,
        .col_vbo = col_vbo,
        .uv_vbo = uv_vbo,
        .ebo = ebo,
        .tri_cnt = data->tri_cnt,
        .cw = data->cw,
    };

    ((wrm_Mesh*)wrm_meshes.data)[result.Handle_val] = m;
    return result;
}

wrm_Option_Handle wrm_render_cloneMesh(wrm_Handle mesh)
{
    return OPTION_NONE(Handle);
}

bool wrm_render_updateMesh(wrm_Handle mesh, const wrm_Mesh_Data *data)
{
    return true;
}

// model

wrm_Option_Handle wrm_render_createModel(const wrm_Model *data, bool use_default_shader)
{
    wrm_Option_Handle result = wrm_Pool_getSlot(&wrm_models);
    if(!result.exists) return result;

    // do some fun pointer arithmetic to get the allocated model
    wrm_Model* model = (wrm_Model*)wrm_models.data + result.Handle_val;
    *model = *data;


    wrm_Mesh m = ((wrm_Mesh*)wrm_meshes.data)[data->mesh];
    if(use_default_shader) {
        
        if(m.col_vbo && m.uv_vbo) {
            model->shader = wrm_shader_defaults.both;
        }
        else if(m.col_vbo) {
            model->shader = wrm_shader_defaults.color;
        }
        else if(m.uv_vbo) {
            model->shader = wrm_shader_defaults.texture;
        }
        else {
            if(wrm_render_settings.errors) fprintf(stderr, "ERROR: Render: createModel(): No suitable default shader for mesh [%u] found\n", data->mesh);
            wrm_Pool_freeSlot(&wrm_models, result.Handle_val);
            return OPTION_NONE(Handle);
        }
    }

    if(!wrm_render_isInUse(model->shader, WRM_RENDER_RESOURCE_SHADER, "createModel()")) {
        wrm_Pool_freeSlot(&wrm_models, result.Handle_val);
        return OPTION_NONE(Handle);
    }
    
    wrm_Shader s = ((wrm_Shader*)wrm_shaders.data)[model->shader];
    if( (s.needs_col && !m.col_vbo) || (s.needs_tex && !m.uv_vbo)) {
        wrm_Pool_freeSlot(&wrm_models, result.Handle_val);
        if(wrm_render_settings.errors) fprintf(stderr, "ERROR: Render: createModel(): Mesh [%u] does not meet shader [%u] data requirements\n", data->mesh, data->shader);
        return OPTION_NONE(Handle);
    }

    return result;
}

wrm_Option_Model wrm_render_getModel(wrm_Handle model)
{
    if(!wrm_render_isInUse(model, WRM_RENDER_RESOURCE_MODEL, "getModel()")) {
        return OPTION_NONE(Model);
    }
    return (wrm_Option_Model){.exists = true, .Model_val = ((wrm_Model*)wrm_models.data)[model]};
}

void wrm_render_updateModelTransform(wrm_Handle model, const vec3 pos, const vec3 rot, const vec3 scale)
{
    if(!wrm_render_isInUse(model, WRM_RENDER_RESOURCE_MODEL, "updateModelTransform()")) {
        return;
    }
    wrm_Model *data = (wrm_Model*)wrm_models.data;

    data[model].pos[0] = pos[0];
    data[model].pos[1] = pos[1];
    data[model].pos[2] = pos[2];

    data[model].rot[0] = rot[0];
    data[model].rot[1] = rot[1];
    data[model].rot[2] = rot[2];

    data[model].scale[0] = scale[0];
    data[model].scale[1] = scale[1];
    data[model].scale[2] = scale[2];
}

void wrm_render_updateModelMesh(wrm_Handle model, wrm_Handle mesh) 
{
    const char *caller = "updateModelMesh()";
    if(wrm_render_isInUse(model, WRM_RENDER_RESOURCE_MODEL, caller) && wrm_render_isInUse(mesh, WRM_RENDER_RESOURCE_MESH, caller)) {
        ((wrm_Model*)wrm_models.data)[model].mesh = mesh;
    }
}

void wrm_render_updateModelTexture(wrm_Handle model, wrm_Handle texture)
{
    const char *caller = "updateModelTexture()";
    if(wrm_render_isInUse(model, WRM_RENDER_RESOURCE_MODEL, caller) && wrm_render_isInUse(texture, WRM_RENDER_RESOURCE_TEXTURE, caller)) {
        ((wrm_Model*)wrm_models.data)[model].texture = texture;
    }
}

void wrm_render_updateModelShader(wrm_Handle model, wrm_Handle shader)
{
    const char *caller = "updateModelShader()";
    if(wrm_render_isInUse(model, WRM_RENDER_RESOURCE_MODEL, caller) && wrm_render_isInUse(shader, WRM_RENDER_RESOURCE_SHADER, caller)) {
        ((wrm_Model*)wrm_models.data)[model].shader = shader;
    }
}

void wrm_render_updateModel(wrm_Handle model, const wrm_Model *data)
{
    if(!wrm_render_isInUse(model, WRM_RENDER_RESOURCE_MODEL, "updateModel()")) {
        return;
    }

    wrm_render_updateModelTransform(model, data->pos, data->rot, data->scale);
    wrm_render_updateModelMesh(model, data->mesh);
    wrm_render_updateModelTexture(model, data->texture);
    wrm_render_updateModelShader(model, data->shader);
}

// camera

void wrm_render_updateCamera(float pitch, float yaw, float fov, float offset, const vec3 pos)
{
    wrm_camera.pitch = pitch;
    wrm_camera.yaw = yaw;
    wrm_camera.fov = fov;
    wrm_camera.offset = offset;
    
    wrm_camera.pos[0] = pos[0];
    wrm_camera.pos[1] = pos[1];
    wrm_camera.pos[2] = pos[2];
}

void wrm_render_getCameraData(float *pitch, float *yaw, float *fov, float *offset, vec3 position)
{
    if(pitch) *pitch = wrm_camera.pitch;
    if(yaw) *yaw = wrm_camera.yaw;
    if(fov) *fov = wrm_camera.fov;
    if(offset) *offset = wrm_camera.offset;

    if(position) {
        position[0] = wrm_camera.pos[0];
        position[1] = wrm_camera.pos[1];
        position[2] = wrm_camera.pos[2];
    }
}


// these are all defined in the header, but this ensures a compiler symbol is actually emitted for them

wrm_RGBAf wrm_RGBAf_fromRGBAi(wrm_RGBAi rgbai);

wrm_RGBAf wrm_RGBAf_fromRGBA(wrm_RGBA rgba);

wrm_RGBAi wrm_RGBAi_fromRGBAf(wrm_RGBAf rgbaf);

wrm_RGBAi wrm_RGBAi_fromRGBA(wrm_RGBA rgba);

wrm_RGBA wrm_RGBA_fromRGBAi(wrm_RGBAi rgbai);

wrm_RGBA wrm_RGBA_fromRGBAf(wrm_RGBAf rgbaf);

/*
Internal helper definitions
*/

internal void wrm_render_initLists(void)
{
    wrm_Pool_init(&wrm_shaders, WRM_RENDER_POOL_INITIAL_CAPACITY, sizeof(wrm_Shader));
    wrm_Pool_init(&wrm_textures, WRM_RENDER_POOL_INITIAL_CAPACITY, sizeof(wrm_Texture));
    wrm_Pool_init(&wrm_meshes, WRM_RENDER_POOL_INITIAL_CAPACITY, sizeof(wrm_Mesh));
    wrm_Pool_init(&wrm_models, WRM_RENDER_POOL_INITIAL_CAPACITY, sizeof(wrm_Model));

    wrm_models_tbd = (wrm_List_Model) {
        .cap = WRM_RENDER_LIST_INITIAL_CAPACITY, 
        .len = 0, 
        .data = (wrm_Model*)calloc(WRM_RENDER_LIST_INITIAL_CAPACITY, sizeof(wrm_Model))
    };
}


internal wrm_Option_GLuint wrm_render_compileShader(const char *shader_text, GLenum type) 
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shader_text, NULL);
    glCompileShader(shader);

    // error checking
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if(success == GL_FALSE) {
        if(wrm_render_settings.errors) {
            GLint log_len = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);

            char *log_msg = malloc(log_len * sizeof(char));
            glGetShaderInfoLog(shader, log_len, NULL, log_msg);

            fprintf(stderr, "ERROR: Render: failed to compile shader, GL error: %s\n Shader source: %s\n", log_msg, shader_text);
            free(log_msg);
        }

        glDeleteShader(shader);

        return (wrm_Option_GLuint){ .exists = false };
    }

    return (wrm_Option_GLuint){ .exists = true, .GLuint_val = shader };
}

internal int wrm_render_compareModels(const void *model1, const void *model2)
{
    const wrm_Model *m1 = (wrm_Model*)model1;
    const wrm_Model *m2 = (wrm_Model*)model2;

    if(m1->shader != m2->shader) {
        return (i64)(m1->shader) - (i64)(m2->shader);
    }

    if(m1->texture != m2->texture) {
        return (i64)(m1->texture) - (i64)(m2->texture);
    }

    if(m1->mesh != m2->mesh) {
        return (i64)(m1->mesh) - (i64)(m2->mesh);
    }

    return 0;
}

internal void wrm_render_createDefaultShaders(void)
{
    wrm_Option_Handle result; 
    result = wrm_render_createShader(WRM_SHADER_DEFAULT_COL_V_TEXT, WRM_SHADER_DEFAULT_COL_F_TEXT, true, false);
    if(!result.exists) {
        if(wrm_render_settings.errors) { fprintf(stderr, "ERROR: Render: failed to create default color shader\n"); }
    }
    wrm_shader_defaults.color = result.Handle_val;

    result = wrm_render_createShader(WRM_SHADER_DEFAULT_TEX_V_TEXT, WRM_SHADER_DEFAULT_TEX_F_TEXT, false, true);
    if(!result.exists) {
        if(wrm_render_settings.errors) { fprintf(stderr, "ERROR: Render: failed to create default texture shader\n"); }
    }
    wrm_shader_defaults.texture = result.Handle_val;

    result = wrm_render_createShader(WRM_SHADER_DEFAULT_BOTH_V_TEXT, WRM_SHADER_DEFAULT_BOTH_F_TEXT, true, true);
    if(!result.exists) {
        if(wrm_render_settings.errors) { fprintf(stderr, "ERROR: Render: failed to create default color + texture shader\n"); }
    }
    wrm_shader_defaults.both  = result.Handle_val;
}

internal void wrm_render_createErrorTexture(void)
{
    wrm_RGBAi p = wrm_RGBAi_fromRGBA(WRM_RGBA_PURPLE);
    wrm_RGBAi b = wrm_RGBAi_fromRGBA(WRM_RGBA_BLACK);

    u8 pixels[] = {
        p.r, p.g, p.b, p.a,
        b.r, b.g, b.b, b.a,
        b.r, b.g, b.b, b.a,
        p.r, p.g, p.b, p.a
    };

    wrm_Texture_Data t = (wrm_Texture_Data) {
        .pixels = pixels,
        .height = 2,
        .width = 2
    };

    wrm_Option_Handle texture = wrm_render_createTexture(&t);
    if(!texture.exists) {
        if(wrm_render_settings.errors) fprintf(stdout, "ERROR: Render: failed to create error texture\n");
        return;
    }

    if(wrm_render_settings.verbose) printf("Render: created error texture\n");
}

internal void wrm_render_createTestModel(void)
{
    wrm_Option_Handle mesh = wrm_render_createMesh(&default_color_mesh_data);
    if(!mesh.exists) {
        if(wrm_render_settings.errors) fprintf(stdout, "ERROR: Render: failed to create test triangle mesh\n");
        return;
    }
    if(wrm_render_settings.verbose) printf("Render: Created test triangle mesh (handle=%d)\n", mesh.Handle_val);

    wrm_Model model_data = {
        .pos = {1.0f, 0.0f, 0.0f},
        .rot = {0.0f, 0.0f, 0.0f},
        .scale = {1.0f, 1.0f, 1.0f},
        .mesh = mesh.Handle_val,
        .texture = 0,
        .shader = wrm_shader_defaults.color,
        .is_visible = wrm_render_settings.test,
        .is_ui = false
    };
    wrm_Option_Handle model = wrm_render_createModel(&model_data, false);
    if(!model.exists) {
        if(wrm_render_settings.errors) fprintf(stdout, "ERROR: Render: failed to create test triangle model\n");
        return;
    }
    if(wrm_render_settings.verbose) printf("Render: Created test triangle model (handle=%d)\n", model.Handle_val);
}

internal inline bool wrm_render_isInUse(wrm_Handle h, wrm_render_Resource_Type t, const char *caller)
{
    bool result;
    const char *type;
    switch(t) {
        case WRM_RENDER_RESOURCE_SHADER:
            type = "shader";
            result = h < wrm_shaders.cap && wrm_shaders.is_used[h];
            break;
        case WRM_RENDER_RESOURCE_TEXTURE:
            type = "texture";
            result = h < wrm_textures.cap && wrm_textures.is_used[h];
            break;
        case WRM_RENDER_RESOURCE_MESH:
            type = "mesh";
            result = h < wrm_meshes.cap && wrm_meshes.is_used[h];
            break;
        case WRM_RENDER_RESOURCE_MODEL:
            type = "model";
            result = h < wrm_models.cap && wrm_models.is_used[h];
            break;
        default:
            if(wrm_render_settings.errors) fprintf(stderr, "ERROR: Render: internal: isInUse(): invalid resource type [%d]\n", t);
            return false;
    }

    if(!result && wrm_render_settings.errors) fprintf(stderr, "ERROR: Render: %s: %s [%u] does not exist\n", caller, type, h);
    return result;
}

internal inline void wrm_render_prepareModels(void)
{
    // clear the list
    wrm_models_tbd.len = 0;

    wrm_Model *data = (wrm_Model*)wrm_models.data;

    for(u32 i = 0; i < wrm_models.cap; i++) {
        if(wrm_models.is_used[i] && data[i].is_visible /* && data[i].parent == 0 */) {
            /* recursively add models */
            if(wrm_models_tbd.len == wrm_models_tbd.cap) {
                if(!realloc(wrm_models_tbd.data, WRM_RENDER_LIST_SCALE_FACTOR * wrm_models_tbd.cap * sizeof(wrm_Model))) {
                    fprintf(stderr, "ERROR: Render: failed to allocate more memory for models to-be-drawn list\n");
                    return;
                }
            }
            wrm_models_tbd.data[wrm_models_tbd.len++] = data[i];
        }
    }

    if(wrm_models_tbd.len > 1) {
        qsort(wrm_models_tbd.data, wrm_models_tbd.len, sizeof(wrm_Model), wrm_render_compareModels);
    }
}

internal inline void wrm_render_getViewMatrix(mat4 view)
{
    // update camera facing direction
    vec3 facing = {
        cosf(glm_rad(wrm_camera.yaw)) * cosf(glm_rad(wrm_camera.pitch)),
        sinf(glm_rad(wrm_camera.pitch)),
        sinf(glm_rad(wrm_camera.yaw)) * cosf(glm_rad(wrm_camera.pitch))
    };
    glm_normalize(facing);

    vec3 pos = {
        wrm_camera.pos[0],
        wrm_camera.pos[1],
        wrm_camera.pos[2],
    };
    
    // include offset in camera eye position
    vec3 eye;
    vec3 offset_vec;
    // If offset > 0 (backward): Subtract direction vector * offset from player position
    glm_vec3_scale(facing, wrm_camera.offset, offset_vec);
    glm_vec3_sub(pos, offset_vec, eye); 

    // get target point (can't just pass facing to lookAt())
    vec3 target;
    glm_vec3_add(eye, facing, target); 

    // get the view projection matrix
    glm_lookat(eye, target, wrm_world_up, view);
}

internal inline void wrm_render_setGLState(wrm_Model *curr, wrm_Model *prev, mat4 model, mat4 view, mat4 persp, u32 *elements)
{
    if(!curr) return;
    wrm_Shader *s = (wrm_Shader*)wrm_shaders.data + curr->shader;
    
    
    if(!prev || curr->shader != prev->shader) {
        glUseProgram(s->program);
        GLint view_loc = glGetUniformLocation(s->program, "view");
        if(view_loc != -1) {
            glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float*)view);
        }

        GLint persp_loc = glGetUniformLocation(s->program, "persp");
        if(persp_loc != -1) {
            glUniformMatrix4fv(persp_loc, 1, GL_FALSE, (float*)persp);
        }
    }
    
    if(!prev || curr->texture != prev->texture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ((wrm_Texture*)wrm_textures.data)[curr->texture].gl_tex);
    }

    if(!prev || curr->mesh != prev->mesh) {
        wrm_Mesh *m = (wrm_Mesh*)wrm_meshes.data + curr->mesh;
        glBindVertexArray(m->vao);
        glFrontFace(m->cw ? GL_CW : GL_CCW);
        *elements = m->tri_cnt * 3;
    }
    
    glm_mat4_identity(model);
    glm_translate(model, curr->pos);
    GLint model_loc = glGetUniformLocation(s->program, "model");
    if(model_loc != -1) {
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, (float*)model);
    }

}