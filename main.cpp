#include <iostream>
#include <vector>

#include <GL/glew.h>
//#include <GL/gl.h> // OpenGL header not necessary, included by GLEW
#include <GL/freeglut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "GLSLProgram.h"
#include "GLTools.h"

// Standard window width
const int WINDOW_WIDTH  = 640;
// Standard window height
const int WINDOW_HEIGHT = 480;
// GLUT window id/handle
int glutID = 0;

cg::GLSLProgram program;

glm::mat4x4 view;
glm::mat4x4 projection;

float zNear = 0.1f;
float zFar  = 100.0f;

float radius = 1.0f;
int n = 0;
const float rotationRate = 10.0f;
glm::vec3 cumulativeRotations = glm::vec3(0.0f, 0.0f, 0.0f);

/*
Struct to hold data for object rendering.
*/
class Object
{
public:
  inline Object ()
    : vao(0),
      positionBuffer(0),
      colorBuffer(0),
      indexBuffer(0)
  {}

  inline ~Object () { // GL context must exist on destruction
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &indexBuffer);
    glDeleteBuffers(1, &colorBuffer);
    glDeleteBuffers(1, &positionBuffer);
  }

  GLuint vao;        // vertex-array-object ID
  
  GLuint positionBuffer; // ID of vertex-buffer: position
  GLuint colorBuffer;    // ID of vertex-buffer: color
  
  GLuint indexBuffer;    // ID of index-buffer
  
  glm::mat4x4 model; // model matrix
};

Object sphere;
Object coordinateSystem;

void renderSphere()
{
  // Create mvp.
  glm::mat4x4 mvp = projection * view * sphere.model;
  
  // Bind the shader program and set uniform(s).
  program.use();
  program.setUniform("mvp", mvp);
  
  // Bind vertex array object so we can render the 2 triangles.
  glBindVertexArray(sphere.vao);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDrawElements(GL_TRIANGLES, 21, GL_UNSIGNED_SHORT, 0);
  glBindVertexArray(0);
}

void renderCoordinateSystem()
{
    // Create mvp.
  glm::mat4x4 mvp = projection * view * coordinateSystem.model;
  
  // Bind the shader program and set uniform(s).
  program.use();
  program.setUniform("mvp", mvp);
  
  // Bind vertex array object so we can render the 2 triangles.
  glBindVertexArray(coordinateSystem.vao);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDrawElements(GL_LINES, 6, GL_UNSIGNED_SHORT, 0);
  glBindVertexArray(0);
}

void initSphere(float radius)
{
  // Construct triangle. These vectors can go out of scope after we have send all data to the graphics card.
  const std::vector<glm::vec3> vertices = { glm::vec3(0.0f, radius, 0.0f),
                                            glm::vec3(radius, 0.0f, 0.0f),
                                            glm::vec3(0.0f, 0.0f, radius),
                                            glm::vec3(-radius, 0.0f, 0.0f),
                                            glm::vec3(0.0f, 0.0f, -radius),
                                            glm::vec3(0.0f, -radius, 0.0f) };
  const std::vector<glm::vec3> colors   = { glm::vec3(1.0f, 1.0f, 1.0f),
                                            glm::vec3(1.0f, 1.0f, 1.0f),
                                            glm::vec3(1.0f, 1.0f, 1.0f),
                                            glm::vec3(1.0f, 1.0f, 1.0f),
                                            glm::vec3(1.0f, 1.0f, 1.0f),
                                            glm::vec3(1.0f, 1.0f, 1.0f) };
  const std::vector<GLushort>  indices  = { 0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 1, 5, 1, 2, 5, 2, 3, 5, 3, 4};

  GLuint programId = program.getHandle();
  GLuint pos;

  // Step 0: Create vertex array object.
  glGenVertexArrays(1, &sphere.vao);
  glBindVertexArray(sphere.vao);
  
  // Step 1: Create vertex buffer object for position attribute and bind it to the associated "shader attribute".
  glGenBuffers(1, &sphere.positionBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, sphere.positionBuffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
  
  // Bind it to position.
  pos = glGetAttribLocation(programId, "position");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 2: Create vertex buffer object for color attribute and bind it to...
  glGenBuffers(1, &sphere.colorBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, sphere.colorBuffer);
  glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);
  
  // Bind it to color.
  pos = glGetAttribLocation(programId, "color");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 3: Create vertex buffer object for indices. No binding needed here.
  glGenBuffers(1, &sphere.indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere.indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);
  
  // Unbind vertex array object (back to default).
  glBindVertexArray(0);
  
  // Modify model matrix.
  // sphere.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
  sphere.model = glm::mat4(1.0f);
}

void initCoordinateSystem()
{
  // Construct triangle. These vectors can go out of scope after we have send all data to the graphics card.
  const std::vector<glm::vec3> vertices = { glm::vec3(0.0f, 0.0f, 0.0f),
                                            glm::vec3(2.0f, 0.0f, 0.0f),
                                            glm::vec3(0.0f, 0.0f, 0.0f),
                                            glm::vec3(0.0f, 2.0f, 0.0f),
                                            glm::vec3(0.0f, 0.0f, 0.0f),
                                            glm::vec3(0.0f, 0.0f, 2.0f) };
  const std::vector<glm::vec3> colors   = { glm::vec3(1.0f, 0.0f, 0.0f),
                                            glm::vec3(1.0f, 0.0f, 0.0f),
                                            glm::vec3(0.0f, 1.0f, 0.0f),
                                            glm::vec3(0.0f, 1.0f, 0.0f),
                                            glm::vec3(0.0f, 0.0f, 1.0f),
                                            glm::vec3(0.0f, 0.0f, 1.0f) };
  const std::vector<GLushort>  indices  = { 0, 1, 2, 3, 4, 5 };

  GLuint programId = program.getHandle();
  GLuint pos;

  // Step 0: Create vertex array object.
  glGenVertexArrays(1, &coordinateSystem.vao);
  glBindVertexArray(coordinateSystem.vao);
  
  // Step 1: Create vertex buffer object for position attribute and bind it to the associated "shader attribute".
  glGenBuffers(1, &coordinateSystem.positionBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, coordinateSystem.positionBuffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
  
  // Bind it to position.
  pos = glGetAttribLocation(programId, "position");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 2: Create vertex buffer object for color attribute and bind it to...
  glGenBuffers(1, &coordinateSystem.colorBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, coordinateSystem.colorBuffer);
  glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);
  
  // Bind it to color.
  pos = glGetAttribLocation(programId, "color");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 3: Create vertex buffer object for indices. No binding needed here.
  glGenBuffers(1, &coordinateSystem.indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coordinateSystem.indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);
  
  // Unbind vertex array object (back to default).
  glBindVertexArray(0);
  
  // Modify model matrix.
  // coordinateSystem.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
  coordinateSystem.model = glm::mat4(1.0f);
}

/*
 Initialization. Should return true if everything is ok and false if something went wrong.
 */
bool init()
{
  // OpenGL: Set "background" color and enable depth testing.
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  
  // Construct view matrix.
  glm::vec3 eye(0.0f, 0.0f, 4.0f);
  glm::vec3 center(0.0f, 0.0f, 0.0f);
  glm::vec3 up(0.0f, 1.0f, 0.0f);
  
  view = glm::lookAt(eye, center, up);
  
  // Create a shader program and set light direction.
  if (!program.compileShaderFromFile("shader/simple.vert", cg::GLSLShader::VERTEX)) {
    std::cerr << program.log();
    return false;
  }
  
  if (!program.compileShaderFromFile("shader/simple.frag", cg::GLSLShader::FRAGMENT)) {
    std::cerr << program.log();
    return false;
  }
  
  if (!program.link()) {
    std::cerr << program.log();
    return false;
  }

  // Create all objects.
  initSphere(1.0f);
  initCoordinateSystem();
  
  return true;
}

/*
 Rendering.
 */
void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Render all objects.
  renderSphere();
  renderCoordinateSystem();
}

void glutDisplay ()
{
   render();
   glutSwapBuffers();
}

/*
 Resize callback.
 */
void glutResize (int width, int height)
{
  // Division by zero is bad...
  height = height < 1 ? 1 : height;
  glViewport(0, 0, width, height);
  
  // Construct projection matrix.
  projection = glm::perspective(45.0f, (float) width / height, zNear, zFar);
}

/*
 Callback for char input.
 */
void glutKeyboard (unsigned char keycode, int x, int y)
{
  switch (keycode) {
  case 27: // ESC
    glutDestroyWindow ( glutID );
    return;
    
  case '+':
    break;
  case '-':
    // do something
    break;
  case 'x':
    sphere.model = glm::rotate(sphere.model, glm::radians(rotationRate), glm::vec3(1.0f, 0.0f, 0.0f));
    coordinateSystem.model = glm::rotate(coordinateSystem.model, glm::radians(rotationRate), glm::vec3(1.0f, 0.0f, 0.0f));
    cumulativeRotations[0] += rotationRate;
    break;
  case 'y':
    sphere.model = glm::rotate(sphere.model, glm::radians(rotationRate), glm::vec3(0.0f, 1.0f, 0.0f));
    coordinateSystem.model = glm::rotate(coordinateSystem.model, glm::radians(rotationRate), glm::vec3(0.0f, 1.0f, 0.0f));
    cumulativeRotations[1] += rotationRate;
    break;
  case 'z':
    sphere.model = glm::rotate(sphere.model, glm::radians(rotationRate), glm::vec3(0.0f, 0.0f, 1.0f));
    coordinateSystem.model = glm::rotate(coordinateSystem.model, glm::radians(rotationRate), glm::vec3(0.0f, 0.0f, 1.0f));
    cumulativeRotations[2] += rotationRate;
    break;
  case 'n':
    sphere.model = glm::mat4x4(1.0f);
    coordinateSystem.model = glm::mat4x4(1.0f);
  }
  glutPostRedisplay();
}

int main(int argc, char** argv)
{
  // GLUT: Initialize freeglut library (window toolkit).
  glutInitWindowSize    (WINDOW_WIDTH, WINDOW_HEIGHT);
  glutInitWindowPosition(40,40);
  glutInit(&argc, argv);
  
  // GLUT: Create a window and opengl context (version 4.1 core profile).
  glutInitContextVersion(4, 1);
  glutInitContextProfile(GLUT_CORE_PROFILE);
  glutInitContextFlags  (GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);
  glutInitDisplayMode   (GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
  
  glutCreateWindow("Aufgabenblatt 02");
  glutID = glutGetWindow();
  
  // GLEW: Load opengl extensions
  //glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    return -1;
  }
#if _DEBUG
  if (glDebugMessageCallback) {
    std::cout << "Register OpenGL debug callback " << std::endl;
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(cg::glErrorVerboseCallback, nullptr);
    glDebugMessageControl(GL_DONT_CARE,
			  GL_DONT_CARE,
			  GL_DONT_CARE,
			  0,
			  nullptr,
			  true); // get all debug messages
  } else {
    std::cout << "glDebugMessageCallback not available" << std::endl;
  }
#endif

  // GLUT: Set callbacks for events.
  glutReshapeFunc(glutResize);
  glutDisplayFunc(glutDisplay);
  //glutIdleFunc   (glutDisplay); // redisplay when idle
  
  glutKeyboardFunc(glutKeyboard);
  
  // init vertex-array-objects.
  bool result = init();
  if (!result) {
    return -2;
  }

  // GLUT: Loop until the user closes the window
  // rendering & event handling
  glutMainLoop ();
  
  // Cleanup in destructors:
  // Objects will be released in ~Object
  // Shader program will be released in ~GLSLProgram
  
  return 0;
}
