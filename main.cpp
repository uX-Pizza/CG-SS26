#include <iostream>
#include <vector>
#include <math.h>

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

Object triangle;
Object quad;

void renderTriangle()
{
  // Create mvp.
  glm::mat4x4 mvp = projection * view * triangle.model;
  
  // Bind the shader program and set uniform(s).
  program.use();
  program.setUniform("mvp", mvp);
  
  // Bind vertex array object so we can render the 1 triangle.
  glBindVertexArray(triangle.vao);
  glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, 0);
  glBindVertexArray(0);
}

void renderQuad()
{
  // Create mvp.
  glm::mat4x4 mvp = projection * view * quad.model;
  
  // Bind the shader program and set uniform(s).
  program.use();
  program.setUniform("mvp", mvp);
  
  // Bind vertex array object so we can render the 2 triangles.
  glBindVertexArray(quad.vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
  glBindVertexArray(0);
}

void initTriangle()
{
  // Construct triangle. These vectors can go out of scope after we have send all data to the graphics card.
  const std::vector<glm::vec3> vertices = { glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f) };
  const std::vector<glm::vec3> colors   = { glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };
  const std::vector<GLushort>  indices  = { 0, 1, 2 };

  GLuint programId = program.getHandle();
  GLuint pos;

  // Step 0: Create vertex array object.
  glGenVertexArrays(1, &triangle.vao);
  glBindVertexArray(triangle.vao);
  
  // Step 1: Create vertex buffer object for position attribute and bind it to the associated "shader attribute".
  glGenBuffers(1, &triangle.positionBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, triangle.positionBuffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
  
  // Bind it to position.
  pos = glGetAttribLocation(programId, "position");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 2: Create vertex buffer object for color attribute and bind it to...
  glGenBuffers(1, &triangle.colorBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, triangle.colorBuffer);
  glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);
  
  // Bind it to color.
  pos = glGetAttribLocation(programId, "color");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 3: Create vertex buffer object for indices. No binding needed here.
  glGenBuffers(1, &triangle.indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangle.indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);
  
  // Unbind vertex array object (back to default).
  glBindVertexArray(0);
  
  // Modify model matrix.
  triangle.model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.25f, 0.0f, 0.0f));
}

void initQuad(float* color)
{
  // Construct triangle. These vectors can go out of scope after we have send all data to the graphics card.
  const std::vector<glm::vec3> vertices = { { -1.0f, 1.0f, 0.0f }, { -1.0, -1.0, 0.0 }, { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f } };
  const std::vector<glm::vec3> colors   = { { color[0], color[1], color[2] }, { color[0], color[1], color[2] }, { color[0], color[1], color[2] }, { color[0], color[1], color[2] } };
  const std::vector<GLushort>  indices  = { 0, 1, 2, 0, 2, 3 };

  GLuint programId = program.getHandle();
  GLuint pos;
  
  // Step 0: Create vertex array object.
  glGenVertexArrays(1, &quad.vao);
  glBindVertexArray(quad.vao);
  
  // Step 1: Create vertex buffer object for position attribute and bind it to the associated "shader attribute".
  glGenBuffers(1, &quad.positionBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, quad.positionBuffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
  
  // Bind it to position.
  pos = glGetAttribLocation(programId, "position");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 2: Create vertex buffer object for color attribute and bind it to...
  glGenBuffers(1, &quad.colorBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, quad.colorBuffer);
  glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);
  
  // Bind it to color.
  pos = glGetAttribLocation(programId, "color");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 3: Create vertex buffer object for indices. No binding needed here.
  glGenBuffers(1, &quad.indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad.indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);
  
  // Unbind vertex array object (back to default).
  glBindVertexArray(0);
  
  // Modify model matrix.
  quad.model = glm::translate(glm::mat4(1.0f), glm::vec3(1.25f, 0.0f, 0.0f));
}

/*
 Initialization. Should return true if everything is ok and false if something went wrong.
 */
bool init(float* squareColor)
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
  initTriangle();
  initQuad(squareColor);
  
  return true;
}

/*
 Rendering.
 */
void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderTriangle();
	renderQuad();
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
    // do something
    break;
  case '-':
    // do something
    break;
  case 'x':
    // do something
    break;
  case 'y':
    // do something
    break;
  case 'z':
    // do something
    break;
  }
  glutPostRedisplay();
}

void rgbToCmy(float rgbValues[3], float* out)
{
  out[0] = 1-rgbValues[0];
  out[1] = 1-rgbValues[1];
  out[2] = 1-rgbValues[2];
}

void rgbToHsv(float rgbValues[3], float* out)
{
  float h, s, v, max, min;
  max = std::max(rgbValues[0], std::max(rgbValues[1], rgbValues[2]));
  min = std::min(rgbValues[0], std::min(rgbValues[1], rgbValues[2]));
  v = max;
  
  if (v > 0) {
    s = (max - min) / max;
  } else {
    s = 0;
  }

  if ((max - min) == 0) {
    h = 0; // greyscale
  } else if (max == rgbValues[0]) {
    h = 60 * fmod((rgbValues[1] - rgbValues[2]) / (max - min), 6.0f);
  } else if (max == rgbValues[1]) {
    h = 60 * (((rgbValues[2] - rgbValues[0]) / (max - min)) + 2);
  } else if (max == rgbValues[2]) {
    h = 60 * (((rgbValues[0] - rgbValues[1]) / (max - min)) + 4);
  }

  if (h < 0) {
    h = h + 360;
  }

  out[0] = h;
  out[1] = s;
  out[2] = v;
}

void cmyToRgb(float cmyValues[3], float* out)
{
  out[0] = 1-cmyValues[0];
  out[1] = 1-cmyValues[1];
  out[2] = 1-cmyValues[2];
}

void cmyToHsv(float cmyValues[3], float* out)
{
  float rgbValues[3];
  cmyToRgb(cmyValues, rgbValues);
  rgbToHsv(rgbValues, out);
}

void hsvToRgb(float hsvValues[3], float* out)
{
  float r, g, b;
  float c = hsvValues[2] * hsvValues[1]; // v * s
  float x = c * (1 - std::abs(fmod(hsvValues[0]/60, 2.0f) - 1));
  float m = hsvValues[2] - c; // v - c

  if (0 <= hsvValues[0] && hsvValues[0] < 60) {
    r = c;
    g = x;
    b = 0;
  } else if (60 <= hsvValues[0] && hsvValues[0] < 120) {
    r = x;
    g = c;
    b = 0;
  } else if (120 <= hsvValues[0] && hsvValues[0] < 180) {
    r = 0;
    g = c;
    b = x;
  } else if (180 <= hsvValues[0] && hsvValues[0] < 240) {
    r = 0;
    g = x;
    b = c;
  } else if (240 <= hsvValues[0] && hsvValues[0] < 300) {
    r = x;
    g = 0;
    b = c;
  } else {
    r = c;
    g = 0;
    b = x;
  }

  out[0] = r + m;
  out[1] = g + m;
  out[2] = b + m;
}

void hsvToCmy(float hsvValues[3], float* out)
{
  float rgbValues[3];
  hsvToRgb(hsvValues, rgbValues);
  rgbToCmy(rgbValues, out);
}

void colorConversion()
{
  std::cout << "Select source model:" << std::endl
            << "CMY: 1" << std::endl
            << "HSV: 2" << std::endl
            << "SKIP: 3" << std::endl;

  int option;
  std::cin >> option;

  if (option == 1) {
    float c, m, y;
    std::cout << "----------------------------" << std::endl
              << "input C: ";
    std::cin >> c;
    std::cout << "input M: ";
    std::cin >> m;
    std::cout << "input Y: ";
    std::cin >> y;
    float cmyValues[3] = {c, m, y};
    float rgbValues[3], hsvValues[3];
    cmyToRgb(cmyValues, rgbValues);
    cmyToHsv(cmyValues, hsvValues);
    
    std::cout << "RGB values:" << std::endl
              << rgbValues[0]
              << ", "
              << rgbValues[1]
              << ", "
              << rgbValues[2] << std::endl;
              std::cout << "HSV values:" << std::endl
              << hsvValues[0]
              << ", "
              << hsvValues[1]
              << ", "
              << hsvValues[2] << std::endl;
  } else if (option == 2) {
    float h, s, v;
    std::cout << "----------------------------" << std::endl
              << "input H: ";
    std::cin >> h;
    std::cout << "input S: ";
    std::cin >> s;
    std::cout << "input V: ";
    std::cin >> v;
    float hsvValues[3] = {h, s, v};
    float rgbValues[3], cmyValues[3];
    hsvToRgb(hsvValues, rgbValues);
    hsvToCmy(hsvValues, cmyValues);

    std::cout << "RGB values:" << std::endl
              << rgbValues[0]
              << ", "
              << rgbValues[1]
              << ", "
              << rgbValues[2] << std::endl;
    std::cout << "CMY values:" << std::endl
              << cmyValues[0]
              << ", "
              << cmyValues[1]
              << ", "
              << cmyValues[2] << std::endl;
  } else if (option == 3) {
    std::cout << "Skipped" << std::endl;
  } else {
    std::cout << "Invalid option" << std::endl;
  }
}

void setSquareColor(float* out)
{
  std::cout << "Select source model:" << std::endl
            << "RGB: 1" << std::endl
            << "CMY: 2" << std::endl
            << "HSV: 3" << std::endl;

  int option;
  std::cin >> option;

  if (option == 1) {
    std::cout << "----------------------------" << std::endl
              << "input R: ";
    std::cin >> out[0];
    std::cout << "input G: ";
    std::cin >> out[1];
    std::cout << "input B: ";
    std::cin >> out[2];
  } else if (option == 2) {
    float cmyValues[3];
    std::cout << "----------------------------" << std::endl
              << "input C: ";
    std::cin >> cmyValues[0];
    std::cout << "input M: ";
    std::cin >> cmyValues[1];
    std::cout << "input Y: ";
    std::cin >> cmyValues[2];

    cmyToRgb(cmyValues, out);
  } else if (option == 3) {
    float hsvValues[3];
    std::cout << "----------------------------" << std::endl
              << "input H: ";
    std::cin >> hsvValues[0];
    std::cout << "input S: ";
    std::cin >> hsvValues[1];
    std::cout << "input V: ";
    std::cin >> hsvValues[2];

    hsvToRgb(hsvValues, out);
  } else {
    std::cout << "Invalid option" << std::endl;
  }
}

int main(int argc, char** argv)
{
  colorConversion();
  std::cout << "----------------------------" << std::endl;
  float squareColor[3];
  setSquareColor(squareColor);

  // GLUT: Initialize freeglut library (window toolkit).
  glutInit(&argc, argv);
  glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
  glutInitWindowPosition(40,40);
  
  // GLUT: Create a window and opengl context (version 4.1 core profile).
  glutInitContextVersion(4, 1);
  glutInitContextProfile(GLUT_CORE_PROFILE);
  glutInitContextFlags  (GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);
  glutInitDisplayMode   (GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
  
  glutCreateWindow("Aufgabenblatt 01");
  glutID = glutGetWindow();
  
  // GLEW: Load opengl extensions
  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    return -1;
  }
#if _DEBUG
  if (glDebugMessageCallback) {
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
  bool result = init(squareColor);
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
