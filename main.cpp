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

unsigned short n = 0;
static const int FPS = 60;
const float AXIS_LENGTH = 0.75f;
const float SMA = 1.75f;
const float MOON_SMA = 0.5f;
const unsigned int INCLINATION = 45;

float phaseAngle = 360.0f;
float phaseAngleStep = 1.0f;
bool paused = true;


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

Object sun;
Object sunAxis;

Object planet;
Object planetAxis;
Object moon;

Object inclinedPlanet;
Object inclinedPlanetAxis;
Object inclinedMoon;

void renderSphere(Object* object)
{
  // Create mvp.
  glm::mat4x4 mvp = projection * view * object->model;

  // Bind the shader program and set uniform(s).
  program.use();
  program.setUniform("mvp", mvp);

  // Bind vertex array object
  glBindVertexArray(object->vao);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDrawElements(GL_TRIANGLES, 1500, GL_UNSIGNED_SHORT, 0);
  glBindVertexArray(0);
}

void renderLine(Object* object)
{
  // Create mvp.
  glm::mat4x4 mvp = projection * view * object->model;

  // Bind the shader program and set uniform(s).
  program.use();
  program.setUniform("mvp", mvp);

  // Bind vertex array object
  glBindVertexArray(object->vao);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, 0);
  glBindVertexArray(0);
}

void initTesselatedSphere(unsigned short n, float radius, glm::vec3 color, Object* object)
{
  std::vector<glm::vec3> vertices = {};
  std::vector<glm::vec3> colors = {};
  std::vector<GLushort> indices = {};
  
  // Create vertices
  for (int layer=0; layer<=2+n*2; layer++) {
    float y = cos((180 * ((float)layer/(2+(float)n*2))) * M_PI/180) * radius;
    float sin_remain = sin((180 * ((float)layer/(2+(float)n*2))) * M_PI/180) * radius;

    if (layer == 0 || layer == 2+n*2) { // if current layer is the first or last layer
      vertices.push_back(glm::vec3(0.0f, y, 0.0f)); // add vertex
      colors.push_back(color); // add matching color
    } else {
      if (layer <= (2+n*2) / 2) { // If layer is in the tp half
        for (int i=0; i<layer*4; i++) {
          float angle = 360 * (((float)i) + 1) / (((float)layer) * 4);
          float x = cos(angle * M_PI/180) * sin_remain;
          float z = sin(angle * M_PI/180) * sin_remain;
          vertices.push_back(glm::vec3(x, y, z)); // add vertex
          colors.push_back(color); // add matching color
        }
      } else { // If layer is in the bottom half
        for (int i=0; i<((2+n*2) - layer)*4; i++) {
          float angle = 360 * (((float)i) + 1) / (((2+n*2) - layer)*4);
          float x = cos(angle * M_PI/180) * sin_remain;
          float z = sin(angle * M_PI/180) * sin_remain;
          vertices.push_back(glm::vec3(x, y, z)); // add vertex
          colors.push_back(color); // add matching color
        }
      }
    }
  }


  // Create indices
  std::vector<int> layerStart; // Indices of the first index per layer
  int currentIndex = 0;

  for (int layer = 0; layer <= 2 + n * 2; layer++) {
    layerStart.push_back(currentIndex);

    if (layer == 0 || layer == 2 + n * 2) // If layer is top or bottom
      currentIndex += 1;
    else if (layer <= (2 + n * 2) / 2) // If next layer is on the top half
      currentIndex += layer * 4;
    else // If layer is on the bottom half
      currentIndex += ((2 + n * 2) - layer) * 4;
  }

  int maxLayer = 2 + n * 2; // Total number of layers

  for (int layer = 0; layer < maxLayer; layer++) {

    int currCount; // number of vertices in current layer
    int nextCount; // number of vertices in next layer

    if (layer == 0) {
      currCount = 1;
    }
    else if (layer <= maxLayer / 2) {
      currCount = layer * 4;
    }
    else {
      currCount = (maxLayer - layer) * 4;
    }

    if (layer + 1 == maxLayer) {
      nextCount = 1;
    }
    else if ((layer + 1) <= maxLayer / 2) {
      nextCount = (layer + 1) * 4;
    }
    else {
      nextCount = (maxLayer - (layer + 1)) * 4;
    }

    int currStart = layerStart[layer];
    int nextStart = layerStart[layer + 1];

    if (currCount == 1) {
      // Top cap
      for (int i = 0; i < nextCount; i++) {
        indices.push_back(currStart);
        indices.push_back(nextStart + i);
        indices.push_back(nextStart + ((i + 1) % nextCount));
      }
    }
    else if (nextCount == 1) {
      // Bottom cap
      for (int i = 0; i < currCount; i++) {
        indices.push_back(currStart + i);
        indices.push_back(nextStart);
        indices.push_back(currStart + ((i + 1) % currCount));
      }
    }
    else {
      int i = 0;
      int j = 0;

      while (i < currCount && j < nextCount) {
        int currA = currStart + i % currCount;
        int currB = currStart + (i + 1) % currCount;

        int nextA = nextStart + j % nextCount;
        int nextB = nextStart + (j + 1) % nextCount;

        float currRatio = (float)(i + 1) / currCount;
        float nextRatio = (float)(j + 1) / nextCount;

        if ((currCount < nextCount && currRatio < nextRatio) ||
            (currCount > nextCount && currRatio <= nextRatio)) {
          indices.push_back(currA);
          indices.push_back(nextA);
          indices.push_back(currB);
          i++;
        }
        else {
          indices.push_back(currA);
          indices.push_back(nextA);
          indices.push_back(nextB);
          j++;
        }
      }
    }
  }


  // Sphere object
  GLuint programId = program.getHandle();
  GLuint pos;

  // Step 0: Create vertex array object.
  glGenVertexArrays(1, &object->vao);
  glBindVertexArray(object->vao);
  
  // Step 1: Create vertex buffer object for position attribute and bind it to the associated "shader attribute".
  glGenBuffers(1, &object->positionBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, object->positionBuffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
  
  // Bind it to position.
  pos = glGetAttribLocation(programId, "position");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 2: Create vertex buffer object for color attribute and bind it to...
  glGenBuffers(1, &object->colorBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, object->colorBuffer);
  glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);
  
  // Bind it to color.
  pos = glGetAttribLocation(programId, "color");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 3: Create vertex buffer object for indices. No binding needed here.
  glGenBuffers(1, &object->indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object->indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);
  
  // Unbind vertex array object (back to default).
  glBindVertexArray(0);
  
  object->model = glm::mat4(1.0f);
}

void initLine(glm::vec3 p1, glm::vec3 p2, glm::vec3 color, Object* object) 
{
  std::vector<glm::vec3> vertices = {p1, p2};
  std::vector<glm::vec3> colors = {color, color};
  std::vector<GLushort> indices = {0, 1};

  // Sphere object
  GLuint programId = program.getHandle();
  GLuint pos;

  // Step 0: Create vertex array object.
  glGenVertexArrays(1, &object->vao);
  glBindVertexArray(object->vao);
  
  // Step 1: Create vertex buffer object for position attribute and bind it to the associated "shader attribute".
  glGenBuffers(1, &object->positionBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, object->positionBuffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
  
  // Bind it to position.
  pos = glGetAttribLocation(programId, "position");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 2: Create vertex buffer object for color attribute and bind it to...
  glGenBuffers(1, &object->colorBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, object->colorBuffer);
  glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);
  
  // Bind it to color.
  pos = glGetAttribLocation(programId, "color");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 3: Create vertex buffer object for indices. No binding needed here.
  glGenBuffers(1, &object->indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object->indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);
  
  // Unbind vertex array object (back to default).
  glBindVertexArray(0);
  
  object->model = glm::mat4(1.0f);
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
  initTesselatedSphere(n, 0.3f, glm::vec3(1.0f, 1.0f, 0.0f), &sun);
  initLine(glm::vec3(0.0f, AXIS_LENGTH, 0.0f), glm::vec3(0.0f, -AXIS_LENGTH, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), &sunAxis);

  initTesselatedSphere(n, 0.15f, glm::vec3(0.0f, 0.0f, 1.0f), &inclinedPlanet);
  initLine(glm::vec3(0.0f, AXIS_LENGTH, 0.0f), glm::vec3(0.0f, -AXIS_LENGTH, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), &inclinedPlanetAxis);
  initTesselatedSphere(n, 0.07f, glm::vec3(0.5f, 0.5f, 0.5f), &inclinedMoon);

  initTesselatedSphere(n, 0.15f, glm::vec3(0.0f, 0.0f, 1.0f), &planet);
  initLine(glm::vec3(0.0f, AXIS_LENGTH, 0.0f), glm::vec3(0.0f, -AXIS_LENGTH, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), &planetAxis);
  initTesselatedSphere(n, 0.07f, glm::vec3(0.5f, 0.5f, 0.5f), &moon);

  return true;
}

/*
 Rendering.
 */
void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glm::vec3 inclinedPlanetPosition = glm::vec3(glm::cos(glm::radians(phaseAngle)) * SMA, 0.0f, glm::sin(glm::radians(phaseAngle)) * SMA);
  inclinedPlanet.model = glm::translate(glm::mat4x4(1.0f), inclinedPlanetPosition);
  inclinedPlanet.model = glm::rotate(inclinedPlanet.model, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  inclinedPlanet.model = glm::rotate(inclinedPlanet.model, glm::radians(phaseAngle*2), glm::vec3(0.0f, 1.0f, 0.0f));
  
  inclinedPlanetAxis.model = glm::translate(glm::mat4x4(1.0f), inclinedPlanetPosition);
  inclinedPlanetAxis.model = glm::rotate(inclinedPlanetAxis.model, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  
  inclinedMoon.model = glm::translate(glm::mat4x4(1.0f), inclinedPlanetPosition);
  inclinedMoon.model = glm::rotate(inclinedMoon.model, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  inclinedMoon.model = glm::translate(inclinedMoon.model, glm::vec3(glm::cos(glm::radians(phaseAngle*2)) * MOON_SMA, 0.0f, glm::sin(glm::radians(phaseAngle*2)) * MOON_SMA));
  
  
  glm::vec3 planetPosition = glm::vec3(glm::cos(glm::radians(phaseAngle)) * -SMA, 0.0f, glm::sin(glm::radians(phaseAngle)) * -SMA);
  planet.model = glm::translate(glm::mat4x4(1.0f), planetPosition);
  planet.model = glm::rotate(planet.model, glm::radians(phaseAngle*2), glm::vec3(0.0f, 1.0f, 0.0f));
  planetAxis.model = glm::translate(glm::mat4x4(1.0f), glm::vec3(glm::cos(glm::radians(phaseAngle)) * -SMA, 0.0f, glm::sin(glm::radians(phaseAngle)) * -SMA));
  
  moon.model = glm::translate(glm::mat4x4(1.0f), planetPosition);
  moon.model = glm::translate(moon.model, glm::vec3(glm::cos(glm::radians(phaseAngle*2)) * -MOON_SMA, 0.0f, glm::sin(glm::radians(phaseAngle*2)) * -MOON_SMA));


  // Render all objects
	renderSphere(&sun);
  renderLine(&sunAxis);

	renderSphere(&inclinedPlanet);
  renderLine(&inclinedPlanetAxis);
  renderSphere(&inclinedMoon);

  renderSphere(&planet);
  renderLine(&planetAxis);
  renderSphere(&moon);
}

void glutDisplay ()
{
   render();
   glutSwapBuffers();
}

void timer(int v) {
  if (!paused) {
    phaseAngle -= phaseAngleStep;
    if (phaseAngle <= 0) {
      phaseAngle += 360.0;
    }
    glutPostRedisplay();
  }
  glutTimerFunc(1000/FPS, timer, v);
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
    
  case 'd':
    // langsamer
    if (phaseAngleStep > 0.4f) {
      phaseAngleStep -= 0.2f;
    }
    break;
  case 'f':
    // schneller
    if (phaseAngleStep < 2.0f) {
      phaseAngleStep += 0.2f;
    }
    break;
  case 'g':
    // pause#
    if (!paused) {
      paused = true;
    } else {
      paused = false;
    }
    break;
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
  
  glutCreateWindow("Aufgabenblatt 03");
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
  
  glutKeyboardFunc(glutKeyboard);
  
  // init vertex-array-objects.
  bool result = init();
  if (!result) {
    return -2;
  }
  
  glutTimerFunc(100, timer, 0); // periodic redisplay
  // GLUT: Loop until the user closes the window
  // rendering & event handling
  glutMainLoop ();
  
  // Cleanup in destructors:
  // Objects will be released in ~Object
  // Shader program will be released in ~GLSLProgram
  
  return 0;
}
