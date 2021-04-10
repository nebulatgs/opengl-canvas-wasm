#include <functional>

#include <emscripten.h>
#include <SDL.h>
#include <iostream>

#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengles2.h>

uint16_t screen_width, screen_height;
float physics[3], scale;
GLuint program;
SDL_Window *window;

void setScreenSize(){
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

GLfloat vertices[12] = {-1.0f, 1.0f,
                          -1.0f, -1.0f,
                           1.0f, -1.0f,
                           
                           -1.0f, -1.0f,
                           1.0f, 1.0f,
                           1.0f, -1.0f};

// Shader sources
const GLchar* gridVertexSource =
    "attribute vec4 position;                     \n"
    "void main()                                  \n"
    "{                                            \n"
    "  gl_Position = vec4(position.xyz, 1.0);     \n"
    "}                                            \n";
// const GLchar* gridFragmentSource =
//     "precision mediump float;\n"
//     "void main()                                  \n"
//     "{                                            \n"
//     "  gl_FragColor[0] = gl_FragCoord.x/640.0;    \n"
//     "  gl_FragColor[1] = gl_FragCoord.y/480.0;    \n"
//     "  gl_FragColor[2] = 0.5;                     \n"
//     "}                                            \n";

const GLchar* gridFragmentSource = R"END(
        #extension GL_OES_standard_derivatives : enable
        #define PI 3.141592
        precision lowp float;

        // size of a square in pixel
        uniform float N;

        uniform vec2 resolution;
        //out vec4 fragColor;

        float aastep(float threshold, float value) {
         float afwidth = 0.7 * length(vec2(dFdx(value), dFdy(value)));
         return smoothstep(threshold-afwidth, threshold+afwidth, value);
        }

        void main() {
            vec2 uv = gl_FragCoord.xy/resolution.xy;
            float dotResolution = resolution.y / N/2.0;
            float dotRadius = 0.3;
            // Normalized pixel coordinates (from 0 to 1)
            
            uv.x *= dotResolution * (resolution.x / resolution.y);
            uv.y *= dotResolution;
            uv.y += 1.5;
            uv.x += 1.5;
            vec2 uv2 = 2.0 * fract(uv) - 1.0;
            //uv2.x *= u_resolution.x / u_resolution.y;
            float distance = length(uv2);
            
            vec4 white = vec4(1.0, 1.0, 1.0, 1.0);
            vec4 black = vec4(0.282, 0.282, 0.282, 0);
            
            vec4 Coord = cos(PI/N*gl_FragCoord);
            vec4 gridColor = vec4(0.082)+0.2*aastep(0.9,max(Coord.x,Coord.y));
            gridColor += vec4(0,0,0,1);
            
            vec4 color = mix(black, gridColor, aastep(dotRadius, distance));

            gl_FragColor = color;
        })END";

// an example of something we will control from the javascript side
bool background_is_black = true;

void handleEvents(float *physics){
    SDL_Event event;
    while( SDL_PollEvent( &event ) ){
    std::cout << event.type << '\n';
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

std::function<void()> loop;
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
    SDL_SetWindowSize(window, screen_width, screen_height);
    SDL_FlushEvent(SDL_WINDOWEVENT);
    loop();
}

GLuint initGL(SDL_Window *window){
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
    setScreenSize();
    SDL_CreateWindowAndRenderer(screen_width, screen_height, 0, &window, nullptr);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    program = initGL(window);
    
    loop = [&]
        {   
            setScreenSize();
            SDL_SetWindowSize(window, (int)screen_width, (int)screen_height);
            GLint uniform_WindowSize = glGetUniformLocation(program, "resolution");
            glUniform2f(uniform_WindowSize, screen_width, screen_height);

            GLint uniform_SquareSize = glGetUniformLocation(program, "N");
            glUniform1f(uniform_SquareSize, scale);
            // move a vertex
            const uint32_t milliseconds_since_start = SDL_GetTicks();
            const uint32_t milliseconds_per_loop = 3000;
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
        };



    emscripten_set_main_loop(main_loop, 0, true);

    return EXIT_SUCCESS;
}
