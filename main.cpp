#include <functional>

#include <emscripten.h>
#include <SDL.h>
#include <iostream>

#include <filesystem>
#include <limits>

#include <fstream>
#include <iterator>

#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengles2.h>

uint16_t screen_width, screen_height;
float physics[3], scale;
GLuint program;
SDL_Window *window;

void getScreenSize(){
    screen_width = (uint16_t)EM_ASM_INT({
        var width = window.innerWidth
        || document.documentElement.clientWidth
        || document.body.clientWidth;
        return width;
    });
    screen_height = (uint16_t)EM_ASM_INT({
        var height = window.innerHeight
        || document.documentElement.clientHeight
        || document.body.clientHeight;
        return height;
    });
    // screen_width = screen_width > 1920 ? 1920  : screen_width;
    // screen_height = screen_height > 1080 ? 1080 : screen_height;
}

GLfloat vertices[12] = {-1.0f,  1.0f,
                        -1.0f, -1.0f,
                         1.0f, -1.0f,
                         
                        -1.0f, -1.0f,
                         1.0f,  1.0f,
                         1.0f, -1.0f};


// Shader sources
static std::string read_shader_file
(
    const std::__fs::filesystem::path::value_type *shader_file)
{
    std::ifstream ifs;

    auto ex = ifs.exceptions();
    ex |= std::ios_base::badbit | std::ios_base::failbit;
    ifs.exceptions(ex);

    ifs.open(shader_file);
    ifs.ignore(std::numeric_limits<std::streamsize>::max());
    auto size = ifs.gcount();

    // if (size > 0x10000) // 64KiB sanity check:
    //     return false;

    ifs.clear();
    ifs.seekg(0, std::ios_base::beg);

    return std::string {std::istreambuf_iterator<char> {ifs}, {}};
}



// an example of something we will control from the javascript side
bool background_is_black = true;

void handleEvents(float *physics){
    SDL_Event event;
    while( SDL_PollEvent( &event ) ){
    // std::cout << event.type << '\n';
        switch (event.type){
            case SDL_MOUSEWHEEL:
                // stg.scale += event.wheel.y > 0 ? (float)event.wheel.y / 2000.0f : (float)event.wheel.y / 2000.0f;
                physics[0] += ((float)event.wheel.y / 16000.0f);
                break;
            case SDL_MULTIGESTURE:
                physics[0] += ((float)event.mgesture.dDist / 10);
            }
    }
}
// the function called by the javascript code
extern "C" void EMSCRIPTEN_KEEPALIVE toggle_background_color() { background_is_black = !background_is_black; }

void main_loop() { 
    handleEvents(physics);
    // vel += acc
    physics[1] += physics[0];
    physics[0] /= 1.1f;

    // Clip Position
    physics[2] = physics[2] > 300 ? 300 : physics[2];
    physics[2] = physics[2] < 7 ? 7 : physics[2];

    // pos += vel
    float adjVel = physics[1] * physics[2];
    physics[2] += adjVel;
    physics[1] /= 1.1f;
    // std::cout << physics[2] << '\n';

    scale = physics[2];
    // SDL_SetWindowSize(window, screen_width, screen_height);


    getScreenSize();
    SDL_SetWindowSize(window, (int)screen_width, (int)screen_height);
    SDL_FlushEvent(SDL_WINDOWEVENT);

    GLint uniform_Resolution = glGetUniformLocation(program, "resolution");
    glUniform2f(uniform_Resolution, screen_width, screen_height);

    GLint uniform_Scale = glGetUniformLocation(program, "scale");
    glUniform1f(uniform_Scale, scale);

    // move a vertex
    // const uint32_t milliseconds_since_start = SDL_GetTicks();
    // const uint32_t milliseconds_per_loop = 3000;
    // vertices[0] = ( milliseconds_since_start % milliseconds_per_loop ) / float(milliseconds_per_loop) - 0.5f;
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Clear the screen
    if( background_is_black )
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    else
        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw a triangle fan for the quad
    glDrawArrays(GL_TRIANGLE_FAN, 0, 6);
    SDL_GL_SwapWindow(window);
}

GLuint initGL(SDL_Window *window){
    auto gridVertexSourceStr = read_shader_file("shaders/grid.vert");
    auto gridFragmentSourceStr = read_shader_file("shaders/grid.frag");

    GLchar *gridVertexSource = (char*)gridVertexSourceStr.c_str();
    GLchar *gridFragmentSource = (char*)gridFragmentSourceStr.c_str();

    // std::cout << gridVertexSource << "\n";
    // Create a Vertex Buffer Object and copy the vertex data to it
    GLuint vbo;
    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Create and compile the vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &gridVertexSource, nullptr);
    glCompileShader(vertexShader);

    // Create and compile the fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &gridFragmentSource, nullptr);
    glCompileShader(fragmentShader);

    // Link the vertex and fragment shader into a shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);
    // Specify the layout of the vertex data
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GLint uniform_WindowSize = glGetUniformLocation(shaderProgram, "resolution");
    glUniform2f(uniform_WindowSize, screen_width, screen_height);
    return shaderProgram;
    
}

int main()
{
    getScreenSize();
    SDL_CreateWindowAndRenderer(screen_width, screen_height, 0, &window, nullptr);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    program = initGL(window);

    emscripten_set_main_loop(main_loop, 0, true);

    return EXIT_SUCCESS;
}
