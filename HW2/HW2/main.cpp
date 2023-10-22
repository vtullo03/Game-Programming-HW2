/**
* Author: Vitoria Tullo
* Assignment: Pong Clone
* Date due: 2023-10-21, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

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

// Background color -- blood red
const float BG_RED = 0.812f,
BG_BLUE = 0.369f,
BG_GREEN = 0.004f,
BG_OPACITY = 0.208f;

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
g_model_matrix_tumbleweed,
g_model_matrix_p1_win,
g_model_matrix_p2_win,
g_projection_matrix; // camera characteristics

// Texture filepaths
const char LEFT_COWBOY_SPRITE[] = "Cowboy1.png",
RIGHT_COWBOY_SPRITE[] = "Cowboy2.png",
TUMBLEWEED_SPRITE[] = "Tumbleweed.png",
P1_WIN_SPRITE[] = "p1win.png",
P2_WIN_SPRITE[] = "p2win.png";

GLuint left_cowboy_texture_id,
right_cowboy_texture_id,
tumbleweed_texture_id,
p1_win_texture_id,
p2_win_texture_id;

const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL = 0,
			TEXTURE_BORDER = 0;

// position and movement matrixes
// offset x position to get them in starting points
float offset = 4.3f;
glm::vec3 left_cowboy_position = glm::vec3(0, 0, 0),
left_cowboy_movement = glm::vec3(0, 0, 0),
right_cowboy_position = glm::vec3(0, 0, 0),
right_cowboy_movement = glm::vec3(0, 0, 0),
tumbleweed_position = glm::vec3(0, 0, 0),
tumbleweed_movement = glm::vec3(0, 0, 0);

// scale matrixes
const glm::vec3 cowboy_scale = glm::vec3(1.0f, 1.0f, 0.0f),
tumbleweed_scale = glm::vec3(0.5f, 0.5f, 0.0f);
glm::vec3 p1_win_scale = glm::vec3(0.0f, 0.0f, 0.0f),
p2_win_scale = glm::vec3(0.0f, 0.0f, 0.0f);

const float COWBOY_MOVEMENT_SPEED = 3.0f;
const float TUMBLEWEED_SPEED = 4.0f;
const float WALL_BORDER = 3.0f;

bool game_ended = false;
bool singleplayer = false;

// helpers
GLuint load_texture(const char* filepath);
void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id);
void normalize(glm::vec3& movement);
void limit_to_border(glm::vec3& position, glm::vec3& movement);
void cowboy_check(glm::vec3& tumbleweed_pos, const glm::vec3& tumbleweed_size,
    glm::vec3& obstacle_pos, const glm::vec3& obstacle_size);
std::pair<bool, int> wall_check(glm::vec3& tumbleweed_pos, glm::vec3& tumbleweed_move);
void check_for_game_end();
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

void check_for_game_end()
{
    if (tumbleweed_position.x > 11.0f)
    {
        g_model_matrix_p1_win = glm::mat4(1.0f);
        p1_win_scale = glm::vec3(10.0f, 8.0f, 8.0f);
        g_model_matrix_p1_win = glm::scale(g_model_matrix_p1_win, p1_win_scale);
        game_ended = true;
    }
    if (tumbleweed_position.x < -11.0f)
    {
        g_model_matrix_p2_win = glm::mat4(1.0f);
        p2_win_scale = glm::vec3(10.0f, 8.0f, 8.0f);
        g_model_matrix_p2_win = glm::scale(g_model_matrix_p2_win, p2_win_scale);
        game_ended = true;
    }
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
    g_model_matrix_tumbleweed = glm::mat4(1.0f);
    g_model_matrix_p1_win = glm::mat4(1.0f);
    g_model_matrix_p2_win = glm::mat4(1.0f);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    // initialize scale and position
    g_model_matrix_left_cowboy = glm::scale(g_model_matrix_left_cowboy, cowboy_scale);
    g_model_matrix_right_cowboy = glm::scale(g_model_matrix_right_cowboy, cowboy_scale);
    g_model_matrix_tumbleweed = glm::scale(g_model_matrix_tumbleweed, tumbleweed_scale);
    g_model_matrix_p1_win = glm::scale(g_model_matrix_p1_win, p1_win_scale);
    g_model_matrix_p2_win = glm::scale(g_model_matrix_p2_win, p2_win_scale);
    left_cowboy_position.x = -offset;
    right_cowboy_position.x = offset;

    // initialize movement
    tumbleweed_movement.x = 1.0f;
    tumbleweed_movement.y = 0.5f;

    // load the textures with the images
    left_cowboy_texture_id = load_texture(LEFT_COWBOY_SPRITE);
    right_cowboy_texture_id = load_texture(RIGHT_COWBOY_SPRITE);
    tumbleweed_texture_id = load_texture(TUMBLEWEED_SPRITE);
    p1_win_texture_id = load_texture(P1_WIN_SPRITE);
    p2_win_texture_id = load_texture(P2_WIN_SPRITE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
 }

std::pair<bool, int> wall_check(glm::vec3& tumbleweed_pos, glm::vec3& tumbleweed_move, 
    float offset)
{
    float wall = WALL_BORDER + offset;
    if (tumbleweed_pos.y > wall) return std::make_pair(true, 1);
    if (tumbleweed_pos.y < -wall) return std::make_pair(true, -1);
    return std::make_pair(false, 0);
}

void limit_to_border(glm::vec3& position, glm::vec3& movement)
{
    std::pair<bool, int> wall_check_outcome = wall_check(position, movement, 0.0f);
    if (wall_check_outcome.first)
    {
        position.y = WALL_BORDER * wall_check_outcome.second;
        movement.y = 0;
    }
}

void cowboy_check(glm::vec3& tumbleweed_pos, const glm::vec3& tumbleweed_size,
    glm::vec3& obstacle_pos, const glm::vec3& obstacle_size, bool is_left)
{
    float collision_factor = 0.02f;

    float x_distance = fabs(tumbleweed_pos.x - (obstacle_pos.x + (is_left ? -offset : offset))) -
        ((tumbleweed_size.x + obstacle_size.x * 2.0f) / 2.0f);
    float y_distance = fabs(tumbleweed_pos.y - obstacle_pos.y) -
        ((tumbleweed_size.y * 2.5f + obstacle_size.y * 2.5f) / 2.0f);

    if (x_distance < 0.0f && y_distance < 0.0f)
    {
        tumbleweed_movement.x *= -1.0f;
    }
}

void wall_bounce(glm::vec3& tumbleweed_pos, glm::vec3& tumbleweed_move)
{
    std::pair<bool, int> wall_check_outcome = 
        wall_check(tumbleweed_pos, tumbleweed_move, 4.0f);
    if (wall_check_outcome.first)
    {
        tumbleweed_move.y *= -1.0f;
    }
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
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                case SDLK_t:
                    singleplayer = !singleplayer;
                }
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL); // if keyboard is not null

    // left cowboy controls
    if (key_state[SDL_SCANCODE_W])
    {
        left_cowboy_movement.y = 1.0f;
    }
    if (key_state[SDL_SCANCODE_S])
    {
        left_cowboy_movement.y = -1.0f;
    }

    if (!singleplayer)
    {
        // right cowboy controls
        if (key_state[SDL_SCANCODE_UP])
        {
            right_cowboy_movement.y = 1.0f;
        }
        if (key_state[SDL_SCANCODE_DOWN])
        {
            right_cowboy_movement.y = -1.0f;
        }
    }
}

void update()
{
    if (!game_ended)
    {
        // calculate time
        float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
        float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
        g_previous_ticks = ticks;

        // multiple movement by speed and time for both
        left_cowboy_position += left_cowboy_movement * COWBOY_MOVEMENT_SPEED * delta_time;
        right_cowboy_position += right_cowboy_movement * COWBOY_MOVEMENT_SPEED * delta_time;
        tumbleweed_position += tumbleweed_movement * TUMBLEWEED_SPEED * delta_time;

        // reset
        g_model_matrix_left_cowboy = glm::mat4(1.0f);
        g_model_matrix_right_cowboy = glm::mat4(1.0f);
        g_model_matrix_tumbleweed = glm::mat4(1.0f);
        g_model_matrix_left_cowboy = glm::scale(g_model_matrix_left_cowboy, cowboy_scale);
        g_model_matrix_right_cowboy = glm::scale(g_model_matrix_right_cowboy, cowboy_scale);
        g_model_matrix_tumbleweed = glm::scale(g_model_matrix_tumbleweed, tumbleweed_scale);

        // limit the cowboys' vertical movement to within the screen
        limit_to_border(left_cowboy_position, left_cowboy_movement);
        limit_to_border(right_cowboy_position, right_cowboy_movement);

        // move the objects
        g_model_matrix_left_cowboy = glm::translate(g_model_matrix_left_cowboy, left_cowboy_position);
        g_model_matrix_right_cowboy = glm::translate(g_model_matrix_right_cowboy, right_cowboy_position);
        g_model_matrix_tumbleweed = glm::translate(g_model_matrix_tumbleweed, tumbleweed_position);

        // bounce off surfaces
        cowboy_check(tumbleweed_position, tumbleweed_scale,
            left_cowboy_position, cowboy_scale, true);
        cowboy_check(tumbleweed_position, tumbleweed_scale,
            right_cowboy_position, cowboy_scale, false);
        wall_bounce(tumbleweed_position, tumbleweed_movement);

        check_for_game_end();
    }
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
    draw_object(g_model_matrix_tumbleweed, tumbleweed_texture_id);
    draw_object(g_model_matrix_p1_win, p1_win_texture_id);
    draw_object(g_model_matrix_p2_win, p2_win_texture_id);

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