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

#include "obj_loader.h"

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

const float normalLength = 2;

unsigned short n = 4;
static const int FPS = 60;
const float AXIS_LENGTH = 0.75f;
const float SMA = 1.75f;
const float MOON_SMA = 0.5f;
const float INCLINATION = 45.0f;

float phaseAngle = 360.0f;
float phaseAngleStep = 1.0f;
bool paused = true;

bool hasNormals = false;
bool showNormals = false; // Steuert, ob die Normalen gezeichnet werden sollen
bool boxVisibile = true;


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

  int vertexCount = 0; //Vertex Count für das einlesen des Models
  
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

Object SpaceShip;
Object SpaceShipNormals;
Object SpaceShipBox;

EdgesList SpaceShipMesh;

void renderSphere(Object* object)
{
  // Create mvp.
  glm::mat4x4 mvp = projection * view * object->model;

  // Bind the shader program and set uniform(s).
  program.use();
  program.setUniform("mvp", mvp);

  // Bind vertex array object
  glBindVertexArray(object->vao);
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

void initSpaceShipNormals(glm::vec3 color, Object* object, EdgesList SpaceShipmesh) 
{
  std::vector<glm::vec3> vertices;

// Wir laufen durch alle n-Ecke und berechnen die Linien-Punkte im Local Space
  for (const auto& face : SpaceShipmesh.faces) {
    for (size_t i = 0; i < face.vertexIndices.size(); ++i) {
      glm::vec3 p_start = SpaceShipmesh.vertices[face.vertexIndices[i]];
      glm::vec3 n_dir = SpaceShipmesh.normals[face.normalIndices[i]];
      
      // Hinweis: 2.0f (deine Konstante) ist oft riesig für Normalen. 
      // Wenn die Linien zu lang sind, nimm hier lieber 0.1f oder 0.2f.
      glm::vec3 p_end = p_start + n_dir * normalLength; 

      vertices.push_back(p_start);
      vertices.push_back(p_end);
    }
  }

  object->vertexCount = vertices.size();
  std::vector<glm::vec3> colors(vertices.size(), color);

  GLuint programId = program.getHandle();
  GLuint pos;

  glGenVertexArrays(1, &object->vao);
  glBindVertexArray(object->vao);
  
  // Positionen in den VBO laden
  glGenBuffers(1, &object->positionBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, object->positionBuffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
  pos = glGetAttribLocation(programId, "position");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Farben in den VBO laden
  glGenBuffers(1, &object->colorBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, object->colorBuffer);
  glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);
  pos = glGetAttribLocation(programId, "color");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  glBindVertexArray(0);
  object->model = glm::mat4(1.0f);
}

void initBlenderModel(const char* path, glm::vec3 modelColor, Object* object)
{
    std::vector<glm::vec3> vertices; 
 
    if (!loadOBJ(path, SpaceShipMesh)) {
        return;
    }

    //Triangulierung der n-Ecke aus der Eckenliste
    for (const auto& face : SpaceShipMesh.faces) {
        size_t n = face.vertexIndices.size();
        if (n < 3) continue; //mindestens 3 Ecken

        // Fächer-Triangulierung (Fan Triangulation):
        // Ein n-Eck wird in (n-2) Dreiecke zerlegt.
        for (size_t i = 1; i < n - 1; ++i) {
            // Wir holen die echten 3D-Koordinaten aus den globalen Positions anhand der Indizes
            glm::vec3 p0 = SpaceShipMesh.vertices[face.vertexIndices[0]];
            glm::vec3 p1 = SpaceShipMesh.vertices[face.vertexIndices[i]];
            glm::vec3 p2 = SpaceShipMesh.vertices[face.vertexIndices[i + 1]];

            // Diese 3 Punkte bilden ein Dreieck zum zeichnen
            vertices.push_back(p0);
            vertices.push_back(p1);
            vertices.push_back(p2);
        }
    }

    hasNormals = SpaceShipMesh.hasNormals;
    //Anzahl der Punkte zum zeichnen
    object->vertexCount = vertices.size();

    //Jeder Punkte bekommt eine Farbe
    std::vector<glm::vec3> colors(vertices.size(), modelColor);

    GLuint programId = program.getHandle();
    GLuint pos;

    glGenVertexArrays(1, &object->vao);
    glBindVertexArray(object->vao);

    // Positionen an die Grafikkarte schicken
    glGenBuffers(1, &object->positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, object->positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    pos = glGetAttribLocation(programId, "position");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Farben an die Grafikkarte schicken
    glGenBuffers(1, &object->colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, object->colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);
    pos = glGetAttribLocation(programId, "color");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindVertexArray(0);
    object->model = glm::mat4(1.0f);
    
    //BoundingBox
    if (!SpaceShipMesh.vertices.empty()) {
        // Starte mit den Werten des allerersten Punktes
        float minX = SpaceShipMesh.vertices[0].x, maxX = SpaceShipMesh.vertices[0].x;
        float minY = SpaceShipMesh.vertices[0].y, maxY = SpaceShipMesh.vertices[0].y;
        float minZ = SpaceShipMesh.vertices[0].z, maxZ = SpaceShipMesh.vertices[0].z;

        // Durchlaufe alle Vertices, um die absoluten Minima und Maxima zu finden
        for (const auto& v : SpaceShipMesh.vertices) {
            if (v.x < minX) minX = v.x; if (v.x > maxX) maxX = v.x;
            if (v.y < minY) minY = v.y; if (v.y > maxY) maxY = v.y;
            if (v.z < minZ) minZ = v.z; if (v.z > maxZ) maxZ = v.z;
        }

        // Aus den 6 Werten bauen wir die 8 Eckpunkte der Box
        glm::vec3 c0(minX, minY, minZ);
        glm::vec3 c1(maxX, minY, minZ);
        glm::vec3 c2(maxX, maxY, minZ);
        glm::vec3 c3(minX, maxY, minZ);
        glm::vec3 c4(minX, minY, maxZ);
        glm::vec3 c5(maxX, minY, maxZ);
        glm::vec3 c6(maxX, maxY, maxZ);
        glm::vec3 c7(minX, maxY, maxZ);

        // Jetzt definieren wir die 12 Kanten (Linien) der Box.
        // Für GL_LINES brauchen wir immer Paare: Startpunkt, Endpunkt.
        std::vector<glm::vec3> boxVertices = {
            // Unterer Ring
            c0, c1,  c1, c5,  c5, c4,  c4, c0,
            // Oberer Ring
            c3, c2,  c2, c6,  c6, c7,  c7, c3,
            // Vertikale Säulen, die oben und unten verbinden
            c0, c3,  c1, c2,  c5, c6,  c4, c7
        };

        // Weise dem Box-Objekt die Anzahl der Punkte zu (12 Linien * 2 Punkte = 24)
        SpaceShipBox.vertexCount = boxVertices.size();

        // Farbe für die Bounding Box (z.B. ein auffälliges Weiß oder Rot)
        std::vector<glm::vec3> boxColors(boxVertices.size(), glm::vec3(1.0f, 1.0f, 1.0f));

        // OpenGL-Buffer für die Box erstellen
        glGenVertexArrays(1, &SpaceShipBox.vao);
        glBindVertexArray(SpaceShipBox.vao);

        // Positions-Buffer
        glGenBuffers(1, &SpaceShipBox.positionBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, SpaceShipBox.positionBuffer);
        glBufferData(GL_ARRAY_BUFFER, boxVertices.size() * sizeof(glm::vec3), boxVertices.data(), GL_STATIC_DRAW);
        pos = glGetAttribLocation(programId, "position");
        glEnableVertexAttribArray(pos);
        glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

        // Farben-Buffer
        glGenBuffers(1, &SpaceShipBox.colorBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, SpaceShipBox.colorBuffer);
        glBufferData(GL_ARRAY_BUFFER, boxColors.size() * sizeof(glm::vec3), boxColors.data(), GL_STATIC_DRAW);
        pos = glGetAttribLocation(programId, "color");
        glEnableVertexAttribArray(pos);
        glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glBindVertexArray(0);
        SpaceShipBox.model = glm::mat4(1.0f);
    }

}

void renderBlenderModel(Object* object)
{
    glm::mat4x4 mvp = projection * view * object->model;
    program.use();
    program.setUniform("mvp", mvp);

    glBindVertexArray(object->vao);

    // Wir nutzen hier glDrawArrays weil gespeicherte vertexCount
    glDrawArrays(GL_TRIANGLES, 0, object->vertexCount);

    glBindVertexArray(0);
}

void renderNormalLines(Object* object)
{
  glm::mat4x4 mvp = projection * view * object->model;
  program.use();
  program.setUniform("mvp", mvp);

  glBindVertexArray(object->vao);
  glDrawArrays(GL_LINES, 0, object->vertexCount);
  glBindVertexArray(0);
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

  initBlenderModel("3dModel/CG-Model.obj", glm::vec3(0.0f, 1.0f, 0.0f), &SpaceShip);
  
  if(hasNormals){
    initSpaceShipNormals(glm::vec3(1.0f, 0.0f, 0.0f), &SpaceShipNormals, SpaceShipMesh);
  }

  return true;
}

/*
 Rendering.
 */
void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  glm::vec3 inclinedPlanetPosition = glm::vec3(glm::cos(glm::radians(phaseAngle)) * SMA, 0.0f, glm::sin(glm::radians(phaseAngle)) * SMA);
  inclinedPlanet.model = glm::translate(glm::mat4x4(1.0f), inclinedPlanetPosition);
  inclinedPlanet.model = glm::rotate(inclinedPlanet.model, glm::radians(INCLINATION), glm::vec3(0.0f, 0.0f, 1.0f));
  inclinedPlanet.model = glm::rotate(inclinedPlanet.model, glm::radians(phaseAngle*2), glm::vec3(0.0f, 1.0f, 0.0f));
  
  inclinedPlanetAxis.model = glm::translate(glm::mat4x4(1.0f), inclinedPlanetPosition);
  inclinedPlanetAxis.model = glm::rotate(inclinedPlanetAxis.model, glm::radians(INCLINATION), glm::vec3(0.0f, 0.0f, 1.0f));
  
  inclinedMoon.model = glm::translate(glm::mat4x4(1.0f), inclinedPlanetPosition);
  inclinedMoon.model = glm::rotate(inclinedMoon.model, glm::radians(INCLINATION), glm::vec3(0.0f, 0.0f, 1.0f));
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


  //SpaceShip
  float shipOrbitRadius = 2.0f; //Entfernung zur Sonne
  float shipSpeed = phaseAngle + 90; //Position


  glm::vec3 shipPosition = glm::vec3(
      glm::cos(glm::radians(shipSpeed)) * shipOrbitRadius,
      0.0f, // Y Position
      glm::sin(glm::radians(shipSpeed)) * shipOrbitRadius
  );

  //Matrix zurücksetzen
  SpaceShip.model = glm::mat4x4(1.0f);

  //Schiff neu positionieren
  SpaceShip.model = glm::translate(SpaceShip.model, shipPosition);

  //Schiff auf der Bahn drehen
  SpaceShip.model = glm::rotate(SpaceShip.model, glm::radians(shipSpeed * -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

  //Schiff um sich selbst drehen
  SpaceShip.model = glm::rotate(SpaceShip.model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

  //Schiff kleiner machen
  SpaceShip.model = glm::scale(SpaceShip.model, glm::vec3(0.05f, 0.05f, 0.05f));

  // 4. Zeichnen
  renderBlenderModel(&SpaceShip);

  if (showNormals && hasNormals){
    SpaceShipNormals.model = SpaceShip.model;
    renderNormalLines(&SpaceShipNormals);
  }
  if (boxVisibile == true){
    SpaceShipBox.model = SpaceShip.model;
    renderNormalLines(&SpaceShipBox);
  }

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
  case 'n':
    if(hasNormals == false){
      std::cout << "hasNormals: " << hasNormals << std::endl;
      break;
    }
    showNormals = !showNormals;
    break;
  case 'b':
    boxVisibile = !boxVisibile;
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
