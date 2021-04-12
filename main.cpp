#include <functional>

#include <emscripten.h>
#include <SDL.h>
#include <iostream>

#include <filesystem>
#include <limits>

#include <fstream>
#include <iterator>
#include <vector>

#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengles2.h>
#include "v2d.h"

uint16_t screen_width, screen_height;
float zoomPhysics[3], scale;
v2d panPhysics[3] = {{.2,.2}, {0,0}, {1,1}}, offset;
bool isMouseDown;
GLuint gridProgram, tilesProgram, tiles2Program;
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

// GLfloat vertices[12] = {-1.0f,  1.0f,
//                         -1.0f, -1.0f,
//                          1.0f, -1.0f,
                         
//                         -1.0f, -1.0f,
//                          1.0f,  1.0f,
//                          1.0f, -1.0f};

GLfloat vertices[12] = {-1.0f,  1.0f,
                        -1.0f, -1.0f,
                         1.0f, 1.0f,
                    
                         1.0f, 1.0f,
                         1.0f, -1.0f,
                         -1.0f, -1.0f};

GLfloat quadVertices[] = {
    // positions     // colors
    -0.05f,  0.05f,  1.0f, 0.0f, 0.0f,
     0.05f, -0.05f,  0.0f, 1.0f, 0.0f,
    -0.05f, -0.05f,  0.0f, 0.0f, 1.0f,

    -0.05f,  0.05f,  1.0f, 0.0f, 0.0f,
     0.05f, -0.05f,  0.0f, 1.0f, 0.0f,   
     0.05f,  0.05f,  0.0f, 1.0f, 1.0f		    		
};  


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

void handleEvents(float *zoomPhysics, v2d *panPhysics){
    SDL_Event event;
    while( SDL_PollEvent( &event ) ){
    std::cout << event.type << '\n';
        switch (event.type){
            case SDL_MOUSEWHEEL:
                // stg.scale += event.wheel.y > 0 ? (float)event.wheel.y / 2000.0f : (float)event.wheel.y / 2000.0f;
                zoomPhysics[0] += ((float)event.wheel.y / 16000.0f);
                break;
            case SDL_MULTIGESTURE:
                zoomPhysics[0] += ((float)event.mgesture.dDist / 10);
                break;
            case SDL_MOUSEBUTTONDOWN:
                isMouseDown = true;
                break;
            case SDL_MOUSEBUTTONUP:
                isMouseDown = false;
                break;
        }
    }
    if(isMouseDown){
        panPhysics[1] += v2d(-event.motion.xrel, event.motion.yrel)/1000;
    }
}
template <class vecT>
vecT processPhysics(vecT *physics, vecT drag, vecT clipStart, vecT clipEnd){
    physics[1] += physics[0];
    physics[0] /= drag;

    // Clip Position
    physics[2] = physics[2] > clipEnd ? clipEnd : physics[2];
    physics[2] = physics[2] < clipStart ? clipStart : physics[2];

    // pos += vel
    vecT adjVel = physics[1] * physics[2];
    physics[2] += adjVel;
    physics[1] /= drag;
    return physics[2];
}
// the function called by the javascript code
extern "C" void EMSCRIPTEN_KEEPALIVE toggle_background_color() { background_is_black = !background_is_black; }
void setCanvas(){
    getScreenSize();
    SDL_SetWindowSize(window, (int)screen_width, (int)screen_height);
    SDL_FlushEvent(SDL_WINDOWEVENT);
}

void drawGrid(){
        glUseProgram(gridProgram);
    GLint uniform_Resolution = glGetUniformLocation(gridProgram, "resolution");
        glUniform2f(uniform_Resolution, screen_width, screen_height);

    GLint uniform_Scale = glGetUniformLocation(gridProgram, "scale");
        glUniform1f(uniform_Scale, scale);

    GLint uniform_Offset = glGetUniformLocation(gridProgram, "offset");
        glUniform2f(uniform_Offset, offset.x, offset.y);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);

    // Draw a triangle fan for the quad

}

void drawTiles(){
    glUseProgram(tilesProgram);
    GLint uniform_ScaleTiles = glGetUniformLocation(tilesProgram, "scale");
        glUniform1f(uniform_ScaleTiles, (1/scale)*504.8);
        
    GLint uniform_OffsetTiles = glGetUniformLocation(tilesProgram, "offset");
        glUniform2f(uniform_OffsetTiles, offset.x, offset.y);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
}

void drawTiles2(){
    // float *pixels = new float[screen_width * screen_height];
    constexpr int tiles = 20;
    // std::vector<GLubyte> pixels(tiles * tiles, 255);
    // for (int i = 0; i < pixels.size() * 3 - 3; i+=3){
    //     //((float)i / (float)pixels.size()) * 255.0f
    //     pixels[i] = 128;
    //     pixels[i+1] = i/65;
    //     pixels[i+2] = 255;
    //     // pixels[i+3] = 255;
    // }
    GLubyte pixels[tiles][tiles][3];
       int i, j, c;

   for (i = 0; i < tiles; i++) {
      for (j = 0; j < tiles; j++) {
        //  c = ((((i&0x8)==0)^((j&0x8))==0))*255;
         pixels[i][j][0] = (GLubyte) i%255;
         pixels[i][j][1] = (GLubyte) i*j%255;
         pixels[i][j][2] = (GLubyte) i%255;
      }
   }
    auto *pixelData = pixels;
    std::cout << (int)pixels[2][2][2] << '\n';
    GLuint textureID;
    glGenTextures(1, &textureID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tiles, tiles, 0, GL_RGB, GL_UNSIGNED_BYTE, pixelData);
    // glBindTexture(GL_TEXTURE_2D, textureID);
    
    glUseProgram(tiles2Program);
    GLint uniform_Resolution = glGetUniformLocation(tiles2Program, "resolution");
        glUniform2f(uniform_Resolution, screen_width, screen_height);
    GLint uniform_ScaleTiles = glGetUniformLocation(tiles2Program, "scale");
        glUniform1f(uniform_ScaleTiles, scale);
        
    GLint uniform_OffsetTiles = glGetUniformLocation(tiles2Program, "offset");
        glUniform2f(uniform_OffsetTiles, offset.x, offset.y);

    GLint uniform_tileDims = glGetUniformLocation(tiles2Program, "tileDims");
        glUniform2f(uniform_tileDims, tiles, tiles);
    GLint uniform_texture = glGetUniformLocation(tiles2Program, "texture");
        glUniform1i(uniform_texture, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
}

void main_loop() { 
    handleEvents(zoomPhysics, panPhysics);

    // Calculate zoom and pan
    scale = processPhysics <float> (zoomPhysics, 1.1f, 7, 300);
    // offset = processPhysics <v2d> (panPhysics, {1.1,1.1}, {0,0}, {300,300});

    // Set canvas size and buffer the vertices for the quad
    setCanvas();
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Clear the screen
    glClearColor(0.086f, 0.086f, 0.086f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    drawTiles2();
    drawGrid();

    SDL_GL_SwapWindow(window);
}
GLuint compileShaders(std::string vertShader, std::string fragShader){
    GLchar *gridVertexSource = (char*)vertShader.c_str();
    GLchar *gridFragmentSource = (char*)fragShader.c_str();

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
    return shaderProgram;
}

void initGL(SDL_Window *window){
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    auto gridVertexSourceStr = read_shader_file("shaders/grid.vert");
    auto gridFragmentSourceStr = read_shader_file("shaders/grid.frag");
    gridProgram = compileShaders(gridVertexSourceStr, gridFragmentSourceStr);

    auto tilesVertexSourceStr = read_shader_file("shaders/tiles.vert");
    auto tilesFragmentSourceStr = read_shader_file("shaders/tiles.frag");
    tilesProgram = compileShaders(tilesVertexSourceStr, tilesFragmentSourceStr);
    
    auto tiles2VertexSourceStr = read_shader_file("shaders/texture.vert");
    auto tiles2FragmentSourceStr = read_shader_file("shaders/texture.frag");
    tiles2Program = compileShaders(tiles2VertexSourceStr, tiles2FragmentSourceStr);
    
    glUseProgram(gridProgram);
    // Specify the layout of the vertex data
    GLint posAttribGrid = glGetAttribLocation(gridProgram, "position");
    glEnableVertexAttribArray(posAttribGrid);
    glVertexAttribPointer(posAttribGrid, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GLint uniform_WindowSizeGrid = glGetUniformLocation(gridProgram, "resolution");
    glUniform2f(uniform_WindowSizeGrid, screen_width, screen_height);

    glUseProgram(tilesProgram);

    GLint posAttribTiles = glGetAttribLocation(tilesProgram, "position");
    glEnableVertexAttribArray(posAttribTiles);
    glVertexAttribPointer(posAttribTiles, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GLint uniform_WindowSizeTiles = glGetUniformLocation(tilesProgram, "resolution");
    glUniform2f(uniform_WindowSizeTiles, screen_width, screen_height);

    GLint uniform_ScaleTiles = glGetUniformLocation(tilesProgram, "scale");
    glUniform1f(uniform_ScaleTiles, scale);
    
}

int main()
{
    getScreenSize();
    SDL_CreateWindowAndRenderer(screen_width, screen_height, 0, &window, nullptr);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    
    initGL(window);

    emscripten_set_main_loop(main_loop, 0, true);

    return EXIT_SUCCESS;
}
