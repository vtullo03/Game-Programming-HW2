#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"                
#include "glm/gtc/matrix_transform.hpp"  
#include "ShaderProgram.h"               
#include "stb_image.h"

#define LOG(argument) std::cout << argument << '\n'

// window dimensions + viewport
const int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

const int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

// for time.deltaTime
float g_previous_ticks = 0.0f;
const float MILLISECONDS_IN_SECOND = 1000.0f;

// !!!! CHANGE COLOR LATER !!!!
// 
// Background color -- blood red
const float BG_RED = 0.404f,
BG_BLUE = 0.016f,
BG_GREEN = 0.016f,
BG_OPACITY = 1.0f;

// shaders
const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

ShaderProgram g_shader_program;

// to display game and check if running
bool g_game_is_running = true;
SDL_Window* g_display_window;


glm::mat4 g_view_matrix, // position of the camera
g_model_matrix_left_cowboy, // transforms of objects
g_model_matrix_right_cowboy,
g_projection_matrix; // camera characteristics

// Texture filepaths
const char LEFT_COWBOY_SPRITE[] = "Cowboy1.png",
RIGHT_COWBOY_SPRITE[] = "Cowboy2.png";

GLuint left_cowboy_texture_id,
	   right_cowboy_texture_id;

const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL = 0,
			TEXTURE_BORDER = 0;

// position and movement matrixes
// offset x position to get them in starting points
float offset = 4.2f;
glm::vec3 left_cowboy_position = glm::vec3(-offset, 0, 0),
          left_cowboy_movement = glm::vec3(0, 0, 0),
          right_cowboy_position = glm::vec3(offset, 0, 0),
          right_cowboy_movement = glm::vec3(0, 0, 0);

const float COWBOY_MOVEMENT_SPEED = 0.2f;

// helpers
GLuint load_texture(const char* filepath);
void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id);
void normalize(glm::vec3& movement);
// for game program
void initialise();
void process_input();
void update();
void render();
void shutdown();


int main(int argc, char* argv[])
{
    initialise(); // initailize all game objects and code -- runs ONCE

    while (g_game_is_running)
    {
        process_input(); // get input from player
        update(); // update the game state, run every frame
        render(); // show the game state (after update to show changes in game state)
    }

    shutdown(); // close game safely
    return 0;
}

// loads a texture to be used by OpenGL
GLuint load_texture(const char* filepath)
{
    // Load image file from filepath
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    // Throw error if no image found at filepath
    if (image == NULL)
    {
        LOG(" Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // Generate and bind texture ID to image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // Setting up texture filter parameters
    // NEAREST better for pixel art
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Release from memory and return texture id
    stbi_image_free(image);

    return textureID;
}

// initialises all game objects and code
// RUNS ONLY ONCE AT THE START
void initialise()
{
    // create window
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("HW 2!!!!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    // for windows machines
#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    // initialize all model matrixes
    g_model_matrix_left_cowboy = glm::mat4(1.0f);
    g_model_matrix_right_cowboy = glm::mat4(1.0f);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // load the textures with the images
    left_cowboy_texture_id = load_texture(LEFT_COWBOY_SPRITE);
    right_cowboy_texture_id = load_texture(RIGHT_COWBOY_SPRITE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 }

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                // check if game is quit
                g_game_is_running = false;
                break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL); // if keyboard is not null

    // left cowboy controls
    if (key_state[SDL_SCANCODE_W])
    {
        left_cowboy_movement.y += 1.0f;
    }
    if (key_state[SDL_SCANCODE_S])
    {
        left_cowboy_movement.y -= 1.0f;
    }

    // right cowboy controls
    if (key_state[SDL_SCANCODE_UP])
    {
        right_cowboy_movement.y += 1.0f;
    }
    if (key_state[SDL_SCANCODE_DOWN])
    {
        right_cowboy_movement.y -= 1.0f;
    }
}

void update()
{
    // calculate time
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
    g_previous_ticks = ticks;
    
    // multiple movement by speed and time for both
    left_cowboy_position += left_cowboy_movement * COWBOY_MOVEMENT_SPEED * delta_time;
    right_cowboy_position += right_cowboy_movement * COWBOY_MOVEMENT_SPEED * delta_time;

    // reset
    g_model_matrix_left_cowboy = glm::mat4(1.0f);
    g_model_matrix_right_cowboy = glm::mat4(1.0f);
    // move the objects
    g_model_matrix_left_cowboy = glm::translate(g_model_matrix_left_cowboy, left_cowboy_position);
    g_model_matrix_right_cowboy = glm::translate(g_model_matrix_right_cowboy, right_cowboy_position);
}

void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // for the two halves of an image texture
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind textures
    draw_object(g_model_matrix_left_cowboy, left_cowboy_texture_id);
    draw_object(g_model_matrix_right_cowboy, right_cowboy_texture_id);

    // Disable
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

// shutdown safely
void shutdown()
{
    SDL_Quit();
}