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

unsigned short radius = 2;
const unsigned short MAX_RADIUS = 3;
const unsigned short MIN_RADIUS = 0;

float zoom = 4.0f;
const unsigned short MAX_ZOOM = 7;
const unsigned short MIN_ZOOM = 2;

const float ROTATION_RATE = 10.0f;

const float NORM_LENGTH_FACTOR = 0.5f;
bool show_norms = false;

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
Object normals;
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
  glDrawElements(GL_TRIANGLES, 1500, GL_UNSIGNED_SHORT, 0);
  glBindVertexArray(0);
}

void renderNormals()
{
  // Create mvp.
  glm::mat4x4 mvp = projection * view * normals.model;
  
  // Bind the shader program and set uniform(s).
  program.use();
  program.setUniform("mvp", mvp);
  
  // Bind vertex array object so we can render the 2 triangles.
  glBindVertexArray(normals.vao);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDrawElements(GL_LINES, 1500, GL_UNSIGNED_SHORT, 0);
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

void initTesselatedSphere(unsigned short n)
{
  float radius = 0.75f;

  std::vector<glm::vec3> vertices = {};
  std::vector<glm::vec3> colors = {};
  std::vector<GLushort> indices = {};

  std::vector<glm::vec3> norm_vertices = {};
  std::vector<glm::vec3> norm_colors = {};
  std::vector<GLushort> norm_indices = {};

  
  // Create vertices
  for (int layer=0; layer<=2+n*2; layer++) {
    float y = cos((180 * ((float)layer/(2+(float)n*2))) * M_PI/180) * radius;
    float sin_remain = sin((180 * ((float)layer/(2+(float)n*2))) * M_PI/180) * radius;

    if (layer == 0 || layer == 2+n*2) { // if current layer is the first or last layer
      vertices.push_back(glm::vec3(0.0f, y, 0.0f)); // add vertex
      colors.push_back(glm::vec3(1.0f, 1.0f, 0.0f)); // add matching color
    } else {
      if (layer <= (2+n*2) / 2) { // If layer is in the tp half
        for (int i=0; i<layer*4; i++) {
          float angle = 360 * (((float)i) + 1) / (((float)layer) * 4);
          float x = cos(angle * M_PI/180) * sin_remain;
          float z = sin(angle * M_PI/180) * sin_remain;
          vertices.push_back(glm::vec3(x, y, z)); // add vertex
          colors.push_back(glm::vec3(1.0f, 1.0f, 0.0f)); // add matching color
        }
      } else { // If layer is in the bottom half
        for (int i=0; i<((2+n*2) - layer)*4; i++) {
          float angle = 360 * (((float)i) + 1) / (((2+n*2) - layer)*4);
          float x = cos(angle * M_PI/180) * sin_remain;
          float z = sin(angle * M_PI/180) * sin_remain;
          vertices.push_back(glm::vec3(x, y, z)); // add vertex
          colors.push_back(glm::vec3(1.0f, 1.0f, 0.0f)); // add matching color
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
  
  sphere.model = glm::mat4(1.0f);


  // Normals object
  for (size_t i = 0; i < indices.size(); i += 3) {
    glm::vec3 a = vertices[indices[i]];
    glm::vec3 b = vertices[indices[i + 1]];
    glm::vec3 c = vertices[indices[i + 2]];

    glm::vec3 center = (a + b + c) / 3.0f;

    glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));

    norm_vertices.push_back(center);
    norm_colors.push_back(glm::vec3(0.0f, 1.0f, 1.0f));
    norm_vertices.push_back(center - normal * NORM_LENGTH_FACTOR);
    norm_colors.push_back(glm::vec3(0.0f, 1.0f, 1.0f));
  }

  for (int i=0; i<norm_vertices.size(); i+=2) {
    norm_indices.push_back(i);
    norm_indices.push_back(i+1);
  }

  // Step 0: Create vertex array object.
  glGenVertexArrays(1, &normals.vao);
  glBindVertexArray(normals.vao);
  
  // Step 1: Create vertex buffer object for position attribute and bind it to the associated "shader attribute".
  glGenBuffers(1, &normals.positionBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, normals.positionBuffer);
  glBufferData(GL_ARRAY_BUFFER, norm_vertices.size() * sizeof(glm::vec3), norm_vertices.data(), GL_STATIC_DRAW);
  
  // Bind it to position.
  pos = glGetAttribLocation(programId, "position");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 2: Create vertex buffer object for color attribute and bind it to...
  glGenBuffers(1, &normals.colorBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, normals.colorBuffer);
  glBufferData(GL_ARRAY_BUFFER, norm_colors.size() * sizeof(glm::vec3), norm_colors.data(), GL_STATIC_DRAW);
  
  // Bind it to color.
  pos = glGetAttribLocation(programId, "color");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 3: Create vertex buffer object for indices. No binding needed here.
  glGenBuffers(1, &normals.indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, normals.indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, norm_indices.size() * sizeof(GLushort), norm_indices.data(), GL_STATIC_DRAW);
  
  // Unbind vertex array object (back to default).
  glBindVertexArray(0);
  
  // float angle = 90 * (1/(2+n*2));
  // normals.model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(1.0f, 1.0f, 1.0f));
  normals.model = glm::mat4x4(1.0f);
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
  initCoordinateSystem();
  initTesselatedSphere(n);
  
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

  if (show_norms) {
    renderNormals();
  }
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
  glm::vec3 eye(0.0f, 0.0f, zoom);
  glm::vec3 center(0.0f, 0.0f, 0.0f);
  glm::vec3 up(0.0f, 1.0f, 0.0f);

  switch (keycode) {
  case 27: // ESC
    glutDestroyWindow ( glutID );
    return;

  case '+':
    if (n < 4) {
      n++;
      glm::mat4x4 old_sphere_model = sphere.model;
      glm::mat4x4 old_norms_model = normals.model;
      initTesselatedSphere(n);
      sphere.model = old_sphere_model;
      normals.model = old_norms_model;
    }
    break;
  case '-':
    if (n > 0) {
      n--;
      glm::mat4x4 old_sphere_model = sphere.model;
      glm::mat4x4 old_norms_model = normals.model;
      initTesselatedSphere(n);
      sphere.model = old_sphere_model;
      normals.model = old_norms_model;
    }
    break;
  case 's':
    if (zoom > MIN_ZOOM) {
      eye[2] -= 1;
      zoom -= 1;
      view = glm::lookAt(eye, center, up);
    }
    break;
  case 'a':
    if (zoom < MAX_ZOOM) {
      eye[2] += 1;
      zoom += 1;
      view = glm::lookAt(eye, center, up);
    }
    break;
  case 'x':
    sphere.model = glm::rotate(sphere.model, glm::radians(ROTATION_RATE), glm::vec3(1.0f, 0.0f, 0.0f));
    normals.model = glm::rotate(normals.model, glm::radians(ROTATION_RATE), glm::vec3(1.0f, 0.0f, 0.0f));
    coordinateSystem.model = glm::rotate(coordinateSystem.model, glm::radians(ROTATION_RATE), glm::vec3(1.0f, 0.0f, 0.0f));
    break;
  case 'y':
    sphere.model = glm::rotate(sphere.model, glm::radians(ROTATION_RATE), glm::vec3(0.0f, 1.0f, 0.0f));
    normals.model = glm::rotate(normals.model, glm::radians(ROTATION_RATE), glm::vec3(0.0f, 1.0f, 0.0f));
    coordinateSystem.model = glm::rotate(coordinateSystem.model, glm::radians(ROTATION_RATE), glm::vec3(0.0f, 1.0f, 0.0f));
    break;
  case 'z':
    sphere.model = glm::rotate(sphere.model, glm::radians(ROTATION_RATE), glm::vec3(0.0f, 0.0f, 1.0f));
    normals.model = glm::rotate(normals.model, glm::radians(ROTATION_RATE), glm::vec3(0.0f, 0.0f, 1.0f));
    coordinateSystem.model = glm::rotate(coordinateSystem.model, glm::radians(ROTATION_RATE), glm::vec3(0.0f, 0.0f, 1.0f));
    break;
  case 'n':
    sphere.model = glm::mat4x4(1.0f);
    normals.model = glm::mat4x4(1.0f);
    coordinateSystem.model = glm::mat4x4(1.0f);

    radius = 2;
    break;
  case 'r':
    if (radius > MIN_RADIUS) {
      radius--;
      sphere.model = glm::scale(sphere.model, glm::vec3(0.5f, 0.5f, 0.5f));
      normals.model = glm::scale(normals.model, glm::vec3(0.5f, 0.5f, 0.5f));
    }
    break;
  case 'R':
    if (radius < MAX_RADIUS) {
      radius++;
      sphere.model = glm::scale(sphere.model, glm::vec3(2.0f, 2.0f, 2.0f));
      normals.model = glm::scale(normals.model, glm::vec3(2.0f, 2.0f, 2.0f));
    }
    break;
  case 'v':
    if (show_norms == false) {
      show_norms = true;
    } else {
      show_norms = false;
    }
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
