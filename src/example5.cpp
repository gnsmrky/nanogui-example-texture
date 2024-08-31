/*
    src/example4.cpp -- C++ version of an example application that shows
    how to use the OpenGL widget. For a Python implementation, see
    '../python/example4.py'.

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <nanogui/texture.h>
#include <nanogui/opengl.h>

#include <nanogui/screen.h>
#include <nanogui/layout.h>
#include <nanogui/window.h>
#include <nanogui/button.h>
#include <nanogui/canvas.h>
#include <nanogui/shader.h>
#include <nanogui/renderpass.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <limits.h> // for PATH_MAX
#include <unistd.h>
#include <nanogui/label.h>
#include <nanogui/button.h>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#if !defined(GL_RED)
    #define GL_RED 0x1903
#endif

#if !defined(GL_RG)
    #define GL_RG 0x8227
#endif

#if defined(_WIN32)
#  if defined(APIENTRY)
#    undef APIENTRY
#  endif
#  include <windows.h>
#endif

using nanogui::Vector3f;
using nanogui::Vector2i;
using nanogui::Shader;
using nanogui::Canvas;
using nanogui::ref;

constexpr float Pi = 3.14159f;

class MyCanvas : public Canvas {
public:
    MyCanvas(Widget *parent) : Canvas(parent, 1), m_rotation(0.f) {
        using namespace nanogui;

        m_shader = new Shader(
            render_pass(),

            // An identifying name
            "a_simple_shader",

#if defined(NANOGUI_USE_OPENGL)
            // Vertex shader
            R"(#version 330
            uniform mat4 mvp;
            in vec3 position;
            in vec3 color;
            out vec4 frag_color;
            void main() {
                frag_color = vec4(color, 1.0);
                gl_Position = mvp * vec4(position, 1.0);
            })",

            // Fragment shader
            R"(#version 330
            out vec4 color;
            in vec4 frag_color;
            void main() {
                color = frag_color;
            })"
#elif defined(NANOGUI_USE_GLES)
            // Vertex shader
            R"(precision highp float;
            uniform mat4 mvp;
            attribute vec3 position;
            attribute vec3 color;
            varying vec4 frag_color;
            void main() {
                frag_color = vec4(color, 1.0);
                gl_Position = mvp * vec4(position, 1.0);
            })",

            // Fragment shader
            R"(precision highp float;
            varying vec4 frag_color;
            void main() {
                gl_FragColor = frag_color;
            })"
#elif defined(NANOGUI_USE_METAL)
            // Vertex shader
            R"(using namespace metal;

            struct VertexOut {
                float4 position [[position]];
                float4 color;
            };

            vertex VertexOut vertex_main(const device packed_float3 *position,
                                         const device packed_float3 *color,
                                         constant float4x4 &mvp,
                                         uint id [[vertex_id]]) {
                VertexOut vert;
                vert.position = mvp * float4(position[id], 1.f);
                vert.color = float4(color[id], 1.f);
                return vert;
            })",

            /* Fragment shader */
            R"(using namespace metal;

            struct VertexOut {
                float4 position [[position]];
                float4 color;
            };

            fragment float4 fragment_main(VertexOut vert [[stage_in]]) {
                return vert.color;
            })"
#endif
        );

        uint32_t indices[3*12] = {
            3, 2, 6, 6, 7, 3,
            4, 5, 1, 1, 0, 4,
            4, 0, 3, 3, 7, 4,
            1, 5, 6, 6, 2, 1,
            0, 1, 2, 2, 3, 0,
            7, 6, 5, 5, 4, 7
        };

        float positions[3*8] = {
            -1.f, 1.f, 1.f, -1.f, -1.f, 1.f,
            1.f, -1.f, 1.f, 1.f, 1.f, 1.f,
            -1.f, 1.f, -1.f, -1.f, -1.f, -1.f,
            1.f, -1.f, -1.f, 1.f, 1.f, -1.f
        };

        float colors[3*8] = {
            0, 1, 1, 0, 0, 1,
            1, 0, 1, 1, 1, 1,
            0, 1, 0, 0, 0, 0,
            1, 0, 0, 1, 1, 0
        };

        m_shader->set_buffer("indices", VariableType::UInt32, {3*12}, indices);
        m_shader->set_buffer("position", VariableType::Float32, {8, 3}, positions);
        m_shader->set_buffer("color", VariableType::Float32, {8, 3}, colors);

    }

    void set_rotation(float rotation) {
        m_rotation = rotation;
    }

    virtual void draw_contents() override {
        using namespace nanogui;

        Matrix4f view = Matrix4f::look_at(
            Vector3f(0, -2, -10),
            Vector3f(0, 0, 0),
            Vector3f(0, 1, 0)
        );

        Matrix4f model = Matrix4f::rotate(
            Vector3f(0, 1, 0),
            (float) glfwGetTime()
        );

        Matrix4f model2 = Matrix4f::rotate(
            Vector3f(1, 0, 0),
            m_rotation
        );

        Matrix4f proj = Matrix4f::perspective(
            float(25 * Pi / 180),
            0.1f,
            20.f,
            m_size.x() / (float) m_size.y()
        );

        Matrix4f mvp = proj * view * model * model2;

        m_shader->set_uniform("mvp", mvp);

        // Draw 12 triangles starting at index 0
        m_shader->begin();
        m_shader->draw_array(Shader::PrimitiveType::Triangle, 0, 12*3, true);
        m_shader->end();
    }

private:
    ref<Shader> m_shader;
    float m_rotation;
};



const char vert_shader_source_310es[] = "#version 310 es              \n"
                                    "precision highp float;     \n"

                                    "uniform mat4 mvp;          \n"
                                    "layout (location = 0) in vec3 position;          \n"
                                    "layout (location = 1) in vec3 color;             \n"
                                    //"in vec2 texcoord;          \n"

                                    "out vec4 frag_color;       \n"
                                    //"out vec2 uv;               \n"

                                    "void main() {                                   \n"
                                    "   gl_Position = vec4(position, 1.0);              \n"
                                    "   frag_color = vec4(color, 1.0);              \n"
                                    //"    gl_Position = mvp * vec4(position, 1.0);    \n"
                                    //"    uv = texcoord;                              \n"
                                    "}";

const char frag_shader_source_310es[] = "#version 310 es              \n"
                                    "precision highp float;     \n"
                                    
                                    "uniform sampler2D image;   \n"
                                    "in vec4 frag_color;        \n"
                                    //"in vec2 uv;                \n"
                                    "out vec4 c;                \n"

                                    "void main() {                                      \n"
                                    "    vec4 value = texture(image, frag_color.xy);    \n"
                                    //"    c = value;                                     \n"
                                    "    c = vec4(value.xyz, 1.0);                   \n"
                                    //"    c = vec4(value.x, 0.0, 0.0, 1.0);                   \n"
                                    //"    c = frag_color;                   \n"
                                    "}";


#define POSITION 0
#define COLOR    1

static GLuint buildShader(const char* shader_source, GLenum type)
{
    GLuint shader;
    GLint status;

    shader = glCreateShader(type);
    if (shader == 0) {
        return 0;
    }

    glShaderSource(shader, 1, &shader_source, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        int length;
        char* log;

        fprintf(stderr, "failed to compile shader\n");
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        if (length > 1) {
            log = (char*)calloc(length, sizeof(char));
            glGetShaderInfoLog(shader, length, &length, log);
            fprintf(stderr, "%s\n", log);
            free(log);
        }
        return false;
    }

    return shader;
}

static GLuint createAndLinkProgram(GLuint v_shader, GLuint f_shader)
{
    GLuint program;
    GLint linked;

    program = glCreateProgram();
    if (program == 0) {
        fprintf(stderr, "failed to create program\n");
        return 0;
    }

    glAttachShader(program, v_shader);
    glAttachShader(program, f_shader);

    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &linked);

    if (linked != GL_TRUE) {
        int length;
        char* log;

        fprintf(stderr, "failed to link program\n");
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        if (length > 1) {
            log = (char*)calloc(length, sizeof(char));
            glGetProgramInfoLog(program, length, &length, log);
            fprintf(stderr, "%s\n", log);
            free(log);
        }
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

void drawTriangle(GLuint program, GLuint vbo, GLuint color, GLuint texture)
{
    glUseProgram(program);

    glClear(GL_COLOR_BUFFER_BIT);

    glEnableVertexAttribArray(POSITION);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(POSITION, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(COLOR);
    glBindBuffer(GL_ARRAY_BUFFER, color);
    glVertexAttribPointer(COLOR, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindTexture(GL_TEXTURE_2D, texture);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(POSITION);
    glDisableVertexAttribArray(COLOR);

    //glutSwapBuffers();
}

class MyTextureCanvas : public Canvas {
public:
    MyTextureCanvas(Widget *parent) : Canvas(parent, 1), m_rotation(0.f) {
        using namespace nanogui;

        m_shader = new Shader(
            render_pass(),

            // An identifying name
            "a_simple_texture_shader",

#if defined(NANOGUI_USE_OPENGL)
            // Vertex shader
            R"(#version 330
            uniform mat4 mvp;
            in vec3 position;
            in vec3 color;
            out vec4 frag_color;
            void main() {
                frag_color = vec4(color, 1.0);
                gl_Position = mvp * vec4(position, 1.0);
            })",

            // Fragment shader
            R"(#version 330
            out vec4 color;
            in vec4 frag_color;
            void main() {
                color = frag_color;
            })"
#elif defined(NANOGUI_USE_GLES)
            // Vertex shader
            R"(#version 310 es      
            precision highp float;

            uniform mat4 mvp;
            layout (location = 0) in vec3 position;
            layout (location = 1) in vec3 color;
            in vec2 texcoord;

            out vec4 frag_color;
            out vec2 uv;

            void main() {
                frag_color = vec4(color, 1.0);
                gl_Position = mvp * vec4(position, 1.0);
                uv = texcoord;
            })",

            // Fragment shader
            R"(#version 310 es
            precision highp float;
            
            uniform sampler2D image;
            in vec4 frag_color;
            in vec2 uv;
            out vec4 c;

            void main() {
                vec4 value = texture(image, frag_color.xy);
                c = value;

                //NOTE: <-- by un-comment this line, it makes GLES 3 compiler generates incorrect shader (missing 'color')
                //          GLESv3 is implemented by mesa's GLESv2.  But the this library does not compile version 310 es shader correctly...
                //          GLESv3 is not part of mesa environment.
                //c = vec4(1.0, 0.0, 0.0, 1.0);

                //NOTE: Seems like OpenGL ES compiler would strip out unused attributes during compilation.  The above fixed color value 
                //          'c' variable renders 'color' in vertex shader has no dependencies.  Thus 'color' would be removed during 
                //          shader compilation.
                //          However, this following line makes use of 'frag_color', which depends on 'color' in vertex shader.  So 'color'
                //          attribute is kept.
                //c = vec4(value.x, 0.0, 0.0, 1.0);
            })"
#elif defined(NANOGUI_USE_METAL)
            // Vertex shader
            R"(using namespace metal;

            struct VertexOut {
                float4 position [[position]];
                float4 color;
            };

            vertex VertexOut vertex_main(const device packed_float3 *position,
                                         const device packed_float3 *color,
                                         constant float4x4 &mvp,
                                         uint id [[vertex_id]]) {
                VertexOut vert;
                vert.position = mvp * float4(position[id], 1.f);
                vert.color = float4(color[id], 1.f);
                return vert;
            })",

            /* Fragment shader */
            R"(using namespace metal;

            struct VertexOut {
                float4 position [[position]];
                float4 color;
            };

            fragment float4 fragment_main(VertexOut vert [[stage_in]]) {
                return vert.color;
            })"
#endif
        );

        uint32_t indices[3*12] = {
            3, 2, 6, 6, 7, 3,
            4, 5, 1, 1, 0, 4,
            4, 0, 3, 3, 7, 4,
            1, 5, 6, 6, 2, 1,
            0, 1, 2, 2, 3, 0,
            7, 6, 5, 5, 4, 7
        };

        float positions[3*8] = {
            -1.f, 1.f, 1.f, -1.f, -1.f, 1.f,
            1.f, -1.f, 1.f, 1.f, 1.f, 1.f,
            -1.f, 1.f, -1.f, -1.f, -1.f, -1.f,
            1.f, -1.f, -1.f, 1.f, 1.f, -1.f
        };

        float colors[3*8] = {
            0.0, 1.0, 1.0, 0.0, 0.0, 1.0,
            1.0, 0, 1.0, 1.0, 1.0, 1.0,
            0.0, 1.0, 0.0, 0.0, 0.0, 0.5,
            1.0, 0.0, 0.0, 1.0, 1.0, 0.5
        };

        m_shader->set_buffer("indices", VariableType::UInt32, {3*12}, indices);
        m_shader->set_buffer("position", VariableType::Float32, {8, 3}, positions);
        m_shader->set_buffer("color", VariableType::Float32, {8, 3}, colors);
        
        // load texture file
        char cwd[PATH_MAX] = "";
        getcwd(cwd, sizeof(cwd));

        std::string texture_file_path = std::string(cwd) + "/icons/icon1.png";
        int texture_file_exists = access (texture_file_path.c_str(), F_OK);
        if (texture_file_exists == 0) {
            std::cout << "\r" << std::string(96, ' ');
            std::cout << "\rLoading " << texture_file_path;

            //int width, height, channels;
            int channels;
            uint8_t* image_data = stbi_load(texture_file_path.c_str(), &m_texture_cx, &m_texture_cy, &channels, 0);
            if (image_data != nullptr) {
                int pixel_format;
                switch (channels)
                {
                    case 1: pixel_format = GL_RED; break;
                    case 2: pixel_format = GL_RG; break;
                    case 3: pixel_format = GL_RGB; break;
                    case 4: pixel_format = GL_RGBA; break;
                    default:
                        stbi_image_free(image_data);
                        goto errExit;
                }
                
                glGenTextures(1, &m_texture);
                glBindTexture(GL_TEXTURE_2D, m_texture);
                glTexImage2D(GL_TEXTURE_2D, 0, pixel_format, m_texture_cx, m_texture_cy, 0, pixel_format, GL_UNSIGNED_BYTE, image_data);
                glGenerateMipmap(GL_TEXTURE_2D);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                
                errExit:
                stbi_image_free(image_data);

            }
        }

        // create quad for drawing texture to frame buffer
        GLfloat vVertices[] = {
            -1.0f, -1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            
            -1.0f,  1.0f, 0.0f,
            1.0f,  1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
        };

        
        float vColors[] = { // used as texture st coordinates
            0.0f,  0.0f, 0.0f,
            0.0f, -1.0f, 0.0f,
            1.0f,  0.0f, 0.0f,
            
            0.0f, -1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            1.0f,  0.0f, 0.0f,
        };
        
        glClearColor(0, 0, 0, 1);
        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW);

        glGenBuffers(1, &m_color);
        glBindBuffer(GL_ARRAY_BUFFER, m_color);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vColors), vColors, GL_STATIC_DRAW);

        // create frame buffer
        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        glGenTextures(1, &m_fbo_texture);
        glBindTexture(GL_TEXTURE_2D, m_fbo_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_texture_cx, m_texture_cy, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fbo_texture, 0);

        // 310es shaders
        GLuint v_shader = buildShader(vert_shader_source_310es, GL_VERTEX_SHADER);
        if (v_shader == 0) {
            fprintf(stderr, "failed to build vertex shader\n");
            return;
        }

        GLuint f_shader = buildShader(frag_shader_source_310es, GL_FRAGMENT_SHADER);
        if (f_shader == 0) {
            fprintf(stderr, "failed to build fragment shader\n");
            glDeleteShader(v_shader);
            return;
        }

        glReleaseShaderCompiler(); // should release resources allocated for the compiler

        m_program = createAndLinkProgram(v_shader, f_shader);
        if (m_program == 0) {
            fprintf(stderr, "failed to create and link program\n");
            glDeleteShader(v_shader);
            glDeleteShader(f_shader);
            return;
        }

        //glUseProgram(m_program);

        // this won't actually delete the shaders until the program is closed but it's a good practice
        glDeleteShader(v_shader);
        glDeleteShader(f_shader);

        // bind fbo
        glGetIntegerv(GL_VIEWPORT, m_prev_viewport);
        glViewport(0, 0, m_texture_cx, m_texture_cy);

        glGetIntegerv(GL_SCISSOR_BOX, m_prev_viewport);
        glScissor(0, 0, m_texture_cx, m_texture_cy);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);

        // draw texture to frame buffer
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        drawTriangle (m_program, m_vbo, m_color, m_texture);

        // unbind fbo
        glViewport(m_prev_viewport[0], m_prev_viewport[1], m_prev_viewport[2], m_prev_viewport[3]);
        glScissor(m_prev_scissor[0], m_prev_scissor[1], m_prev_scissor[2], m_prev_scissor[3]);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    virtual ~MyTextureCanvas() {
        glDeleteTextures(1, &m_texture);
    }

    void set_rotation(float rotation) {
        m_rotation = rotation;
    }

    virtual void draw_contents() override {
        using namespace nanogui;

        Matrix4f view = Matrix4f::look_at(
            Vector3f(0, -2, -10),
            Vector3f(0, 0, 0),
            Vector3f(0, 1, 0)
        );

        Matrix4f model = Matrix4f::rotate(
            Vector3f(0, 1, 0),
            (float) glfwGetTime()
        );

        Matrix4f model2 = Matrix4f::rotate(
            Vector3f(1, 0, 0),
            m_rotation
        );

        Matrix4f proj = Matrix4f::perspective(
            float(25 * Pi / 180),
            0.1f,
            20.f,
            m_size.x() / (float) m_size.y()
        );

        Matrix4f mvp = proj * view * model * model2;

        m_shader->set_uniform("mvp", mvp);

        // Draw 12 triangles starting at index 0
        m_shader->begin();
        
        //glActiveTexture(GL_TEXTURE0 + m_texture);
        //glBindTexture(GL_TEXTURE_2D, m_texture);
        glBindTexture(GL_TEXTURE_2D, m_fbo_texture);

        m_shader->draw_array(Shader::PrimitiveType::Triangle, 0, 12*3, true);
        m_shader->end();
    }

private:
    ref<Shader> m_shader;
    float m_rotation;
    
    GLuint m_texture;
    int    m_texture_cx;
    int    m_texture_cy;

    GLuint m_fbo;
    GLuint m_fbo_texture;

    GLuint m_program;

    GLuint m_vbo, m_color;
    
    int m_prev_viewport[4] = { 0 };
    int m_prev_scissor[4] = { 0 };
};


class ExampleApplication : public nanogui::Screen {
public:
    ExampleApplication() : nanogui::Screen(Vector2i(1000, 800), "NanoGUI Test (example 5 - Textured Cube)", false) {
        using namespace nanogui;

#if 1
        Window *window = new Window(this, "Canvas widget demo");
        window->set_position(Vector2i(15, 15));
        window->set_layout(new GroupLayout());

        m_canvas = new MyCanvas(window);
        m_canvas->set_background_color({100, 100, 100, 255});
        m_canvas->set_fixed_size({400, 400});

        Widget *tools = new Widget(window);
        tools->set_layout(new BoxLayout(Orientation::Horizontal,
                                       Alignment::Middle, 0, 5));

        Button *b0 = new Button(tools, "Random Background");
        b0->set_callback([this]() {
            m_canvas->set_background_color(
                Vector4i(rand() % 256, rand() % 256, rand() % 256, 255));
        });

        Button *b1 = new Button(tools, "Random Rotation");
        b1->set_callback([this]() {
            m_canvas->set_rotation((float) Pi * rand() / (float) RAND_MAX);
        });
#endif

        //----- new window for GLES 3.1 Canvas -----
        Window *texture_window = new Window(this, "Textured Cube");
        texture_window->set_position(nanogui::Vector2i(500, 15));
        texture_window->set_layout(new nanogui::GroupLayout());
        
        // texture canvas
        m_textureCanvas = new MyTextureCanvas(texture_window);
        m_textureCanvas->set_background_color(Color(32, 32, 128, 255));
        m_textureCanvas->set_fixed_size({400, 400});

        Widget *texture_tools = new Widget(texture_window);
        texture_tools->set_layout(new BoxLayout(Orientation::Horizontal,
                                       Alignment::Middle, 0, 5));
        Button *t_b0 = new Button(texture_tools, "Random Background");
        t_b0->set_callback([this]() {
            m_textureCanvas->set_background_color(
                Vector4i(rand() % 256, rand() % 256, rand() % 256, 255));
        });

        Button *t_b1 = new Button(texture_tools, "Random Rotation");
        t_b1->set_callback([this]() {
            m_textureCanvas->set_rotation((float) Pi * rand() / (float) RAND_MAX);
        });


        perform_layout();
    }

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) {
        if (Screen::keyboard_event(key, scancode, action, modifiers))
            return true;
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            set_visible(false);
            return true;
        }
        return false;
    }

    virtual void draw(NVGcontext *ctx) {
        // Draw the user interface
        Screen::draw(ctx);
    }
private:
    MyCanvas *m_canvas;
    MyTextureCanvas *m_textureCanvas;
};

int main(int /* argc */, char ** /* argv */) {
    try {
        nanogui::init();

        /* scoped variables */ {
            nanogui::ref<ExampleApplication> app = new ExampleApplication();
            app->draw_all();
            app->set_visible(true);
            nanogui::mainloop(1 / 60.f * 1000);
        }

        nanogui::shutdown();
    } catch (const std::runtime_error &e) {
        std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
        #if defined(_WIN32)
            MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
        #else
            std::cerr << error_msg << std::endl;
        #endif
        return -1;
    }

    return 0;
}
