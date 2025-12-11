
//#include "RendererGL.h"
//
//#include <iostream>
//
//RendererGL::RendererGL() {}
//
//RendererGL::~RendererGL()
//{
//   if (m_window) glfwDestroyWindow(m_window);
//   glfwTerminate();
//}
//
//bool RendererGL::Init(const RenderParams& params)
//{
//   // Initialise GLFW
//   if (!glfwInit()) {
//      std::cerr << "Failed to init GLFW\n";
//      return false;
//   }
//
//   // Request an OpenGL 3.3 Core context (adjust as needed)
//   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//
//   m_window = glfwCreateWindow(params.width, params.height,
//      "OpenGL Renderer", nullptr, nullptr);
//   if (!m_window) {
//      std::cerr << "Failed to create window\n";
//      return false;
//   }
//   glfwMakeContextCurrent(m_window);
//
//   // Load OpenGL functions
//   if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
//      std::cerr << "Failed to load GLAD\n";
//      return false;
//   }
//
//   // Simple full?screen quad VAO/VBO
//   float vertices[] = {
//       -1.0f,  1.0f, 0.f,
//       -1.0f, -1.0f, 0.f,
//        1.0f, -1.0f, 0.f,
//        1.0f,  1.0f, 0.f
//   };
//   glGenVertexArrays(1, &m_vao);
//   glBindVertexArray(m_vao);
//
//   glGenBuffers(1, &m_vbo);
//   glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
//   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
//   glEnableVertexAttribArray(0);           // location 0 in the shader
//   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
//      3 * sizeof(float), (void*)0);
//
//   return true;
//}
//
//void RendererGL::Resize(int width, int height)
//{
//   glfwSetWindowSize(m_window, width, height);
//   glViewport(0, 0, width, height);
//}
//
//void RendererGL::BeginFrame()
//{
//   glClearColor(0.1f, 0.2f, 0.3f, 1.f);
//   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//}
//
//void RendererGL::EndFrame() {}
//
//void RendererGL::Present()
//{
//   glfwSwapBuffers(m_window);
//}
