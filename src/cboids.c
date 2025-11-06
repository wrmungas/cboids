#define WRM_COMMON_IMPLEMENTATION
#include "wrm-common.h"
#include "wrm-render.h"
#include "wrm-input.h"
#include "boids-world.h"


static const char *BOIDS_APP_NAME = "cboids - boids in C!";

static const u8 REQUIRED_ARGS = 2;
static const u8 ARG_STRLEN = 2;

u64 sdl_frequency;
u64 sdl_counter;

void boids_processFlags(int argc, char **argv, bool *verbose, bool *super_verbose);
bool boids_init(bool verbose, bool super_verbose);
bool boids_update(void);
void boids_quit(void);

/*
Cboids - An interactive 3d flocking simulation made in C
*/
int main(int argc, char **argv)
{
	bool verbose, super_verbose;
	boids_processFlags(argc, argv, &verbose, &super_verbose);

	if(!boids_init(verbose, super_verbose)) {
		wrm_fail(1, "Failed to start cboids - see output for errors\n");
	}

	while(boids_update()) {
		// ... empty, we just want to call update() as long as we can
	}

	boids_quit();
	return 0;
}


// high-level helper implementations


void boids_processFlags(int argc, char **argv, bool *verbose, bool *super_verbose)
{
	if(argc != REQUIRED_ARGS) wrm_fail(1, "Usage: cboids <arg>, use -h for further info\n");
	
	// try my match syntax
	const char* options[] = {"-s", "-V", "-v", "-h"};
	u8 n = sizeof(options) / sizeof(const char*);

	switch(wrm_cstrn_match(ARG_STRLEN, argv[1], options, n)) {
		case 0:
			wrm_fail(1, "Invalid argument\n");
		case 1:
			*super_verbose = false;
			*verbose = false;
			break;
		case 2:
			*super_verbose = true;
			*verbose = true;
			break;
		case 3:
			*super_verbose = false;
			*verbose = true;
			break;
		case 4:
			printf(
			"Command-line options:\n%s%s%s",
			" -v: verbose, print high-level application status during startup and exit\n",
			" -V: super verbose, print high-level and submodule application status at startup and exit\n",
			" -s: silent, do neither of the above\n"
			);
			// valid program end point
			exit(EXIT_SUCCESS);
	}
}


bool boids_init(bool verbose, bool super_verbose)
{
	wrm_Window_Data args = {
		.name = BOIDS_APP_NAME,
		.height_px = WRM_DEFAULT_WINDOW_HEIGHT,
		.width_px = WRM_DEFAULT_WINDOW_WIDTH,
		.is_resizable = false,
		.background = wrm_RGBAf_fromRGBA(0x6488eaffU) // a nice soft blue
	};

	wrm_render_Settings r_settings = {
		.verbose = super_verbose,
		.errors = true,
		.test = true
	};

	if(!wrm_render_init(&r_settings, &args)) return false;
	if(verbose) printf("Renderer initialized!\n");

	wrm_input_Settings i_settings = {
		.errors = true
	};
	if(!wrm_input_init(&i_settings, wrm_render_getWindow())) {
		wrm_render_quit();
		return false;
	}
	if(verbose) printf("Input initialized!\n");


	if(!boids_world_init()) {

	}

	sdl_counter = SDL_GetPerformanceCounter();
	
	return true;
}


bool boids_update(void)
{
	u64 new_counter = SDL_GetPerformanceCounter();
	u64 new_frequency = SDL_GetPerformanceFrequency();
	if(new_frequency != sdl_frequency) sdl_frequency = new_frequency;

	float delta_time = (float)(new_counter - sdl_counter) / sdl_frequency;
	sdl_counter = new_counter;

	// first process events and input
	wrm_input_update();
	if(wrm_input_should_quit) {
		return false;
	}

	// then update the UI
	// boids_ui_update(input);
	
	// then update the simulation world
	boids_world_update(delta_time);

	// then render the updates
	wrm_render_draw(delta_time);
	return true;
}

void boids_quit(void)
{
	// destroy subsystems in REVERSE order of creation
	wrm_input_quit();
	wrm_render_quit();
}