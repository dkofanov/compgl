#include "graphics.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include "shader.hpp"

#include "stdlib.h"

GLFWwindow* WINDOW;

struct {
    size_t w_width_;
    size_t w_height_;
    size_t scale_;
    GLuint program_id_;
    GLuint vertex_buffer_;
    GLuint vao_;
    GLfloat *vertices_mapped_buf_;
    size_t vertices_mapped_buf_idx_;
    size_t active_pixels_n_;
    GLfloat pxl_width_;
    GLfloat pxl_height_;

    // FPS counter
    std::chrono::time_point<std::chrono::high_resolution_clock> prev_frame_timestamp_;
} GR_DATA;


void GR_Initialize(size_t w_width, size_t w_height, size_t scale)
{
    assert(w_width % scale == 0);
    assert(w_height % scale == 0);
    GR_DATA.w_height_ = w_height;
    GR_DATA.w_width_ = w_width;
    GR_DATA.pxl_height_ = 2.f / (w_height / scale);
    GR_DATA.pxl_width_ = 2.f / (w_width / scale);
    GR_DATA.scale_ = scale;
    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        getchar();
        return;
    }

    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        getchar();
        return;
    }

    glfwWindowHint(GLFW_DOUBLEBUFFER, 0);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    WINDOW = glfwCreateWindow(w_width, w_height, "grtest", NULL, NULL);
    if (WINDOW == NULL) {
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        getchar();
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(WINDOW);
    glfwSwapInterval(1);

    // Initialize GLEW
    glewExperimental = 1; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return;
    }
    glfwSetInputMode(WINDOW, GLFW_STICKY_KEYS, GL_TRUE);

    // Create VAO
    glGenVertexArrays(1, &GR_DATA.vao_);
    glBindVertexArray(GR_DATA.vao_);

    // Create and compile our GLSL program from the shaders
    GR_DATA.program_id_ = LoadShaders();

    // Generate 1 buffer, put the resulting identifier in vertexbuffer
    glGenBuffers(1, &GR_DATA.vertex_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, GR_DATA.vertex_buffer_);
    // Give our vertices to OpenGL.
    const size_t pxl_count = (w_height / scale) * (w_width / scale);
    const size_t max_triangles = pxl_count * 2;
    const size_t max_vertices = max_triangles * 3;
    glBufferData(GL_ARRAY_BUFFER, max_vertices * 3 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
    GR_DATA.vertices_mapped_buf_ = static_cast <GLfloat *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
    GR_DATA.vertices_mapped_buf_idx_ = 0;

    glClearColor(0.4f, 0.0f, 0.4f, 0.0f);
    GR_DATA.prev_frame_timestamp_ = std::chrono::high_resolution_clock::now();
}

static void GRstatic_DrawTriangle(GLfloat v[3][2])
{
    size_t idx = GR_DATA.vertices_mapped_buf_idx_;
    GR_DATA.vertices_mapped_buf_[idx + 0] = v[0][0];
    GR_DATA.vertices_mapped_buf_[idx + 1] = v[0][1];
    GR_DATA.vertices_mapped_buf_[idx + 2] = 0;
    GR_DATA.vertices_mapped_buf_[idx + 3] = v[1][0];
    GR_DATA.vertices_mapped_buf_[idx + 4] = v[1][1];
    GR_DATA.vertices_mapped_buf_[idx + 5] = 0;
    GR_DATA.vertices_mapped_buf_[idx + 6] = v[2][0];
    GR_DATA.vertices_mapped_buf_[idx + 7] = v[2][1];
    GR_DATA.vertices_mapped_buf_[idx + 8] = 0;
    GR_DATA.vertices_mapped_buf_idx_ += 9;
}

void GR_PutPixel(size_t x, size_t y)
{
    GR_DATA.active_pixels_n_++;
    assert(x < (GR_DATA.w_width_ / GR_DATA.scale_));
    assert(y < (GR_DATA.w_height_ / GR_DATA.scale_));
    GLfloat xmin = 0, xmax = 0, ymin = 0, ymax = 0;
    xmin = -1.f + x * GR_DATA.pxl_width_;
    xmax = xmin + GR_DATA.pxl_width_;
    ymax = 1.f - y * GR_DATA.pxl_height_;
    ymin = ymax - GR_DATA.pxl_height_;
    GLfloat lower_triag[3][2] = {
        {xmin, ymax},
        {xmin, ymin},
        {xmax, ymin}
    };
    GLfloat upper_triag[3][2] = {
        {xmin, ymax},
        {xmax, ymax},
        {xmax, ymin}
    };

    GRstatic_DrawTriangle(lower_triag);
    GRstatic_DrawTriangle(upper_triag);
}


void GR_Flush()
{
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = now - GR_DATA.prev_frame_timestamp_;
    double fps = 1. / diff.count();
    printf("FPS: %lf (%lf us)\n", fps, diff.count() * 1000000); 
    GR_DATA.prev_frame_timestamp_ = now;

    glClear( GL_COLOR_BUFFER_BIT );
    // Use shader:
    glUseProgram(GR_DATA.program_id_);
    
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, GR_DATA.vertex_buffer_);
    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*)0            // array buffer offset
    );
    glDrawArrays(GL_TRIANGLES, 0, GR_DATA.active_pixels_n_ * 2 * 3);
    glDisableVertexAttribArray(0);
    glfwSwapBuffers(WINDOW);
    //glfwPollEvents();

    GR_DATA.active_pixels_n_ = 0;
    GR_DATA.vertices_mapped_buf_idx_ = 0;
    return;
}

void GR_Destroy() {
    // Cleanup VBO
    glDeleteBuffers(1, &GR_DATA.vertex_buffer_);
    glDeleteVertexArrays(1, &GR_DATA.vao_);
    glDeleteProgram(GR_DATA.program_id_);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();
    return;
}

