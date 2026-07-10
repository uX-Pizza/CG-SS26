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

cg::GLSLProgram programSimple;
cg::GLSLProgram programFlat;
cg::GLSLProgram programPhong;

glm::mat4x4 view;
glm::mat4x4 projection;

float zNear = 0.1f;
float zFar  = 100.0f;

unsigned short n = 4;
static const int FPS = 60;
const float AXIS_LENGTH = 0.75f;
const float SMA = 1.75f;
const float MOON_SMA = 0.5f;
const float INCLINATION = 45.0f;
const float NORMALS_LENGTH = 2.0f;

float zoom = 4.0f;
const unsigned short MAX_ZOOM = 7;
const unsigned short MIN_ZOOM = 1;

float phaseAngle = 360.0f;
float phaseAngleStep = 1.0f;
bool paused = true;

bool wireframe = false;
bool shaded = false;
bool normalsInitialized = false;
bool showNormals = false;
bool showBox = false;
cg::GLSLProgram currentProgram;

unsigned  lightIndex = 0;
glm::vec4 lights[2] = {
	{ 0.0f, 1.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 4.0f, 1.0f }
};


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
      indexBuffer(0),
	  normalBuffer(0),
	  surfKa(0),
	  surfKd(0),
	  surfKs(0),
	  surfShininess(0)
  {}

  inline ~Object () { // GL context must exist on destruction
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &indexBuffer);
    glDeleteBuffers(1, &colorBuffer);
	glDeleteBuffers(1, &normalBuffer);
    glDeleteBuffers(1, &positionBuffer);
  }

  GLuint vao;        // vertex-array-object ID
  
  GLuint positionBuffer; // ID of vertex-buffer: position
  GLuint colorBuffer;    // ID of vertex-buffer: color
  GLuint normalBuffer;
  
  GLuint indexBuffer;    // ID of index-buffer

  glm::vec3 surfKa;
  glm::vec3 surfKd;
  glm::vec3 surfKs;
  float surfShininess;

  std::vector<glm::vec3> localVertices;

  int vertexCount;
  
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


std::vector<glm::vec3> getWorldVertices(const Object& object)
{
    std::vector<glm::vec3> worldVertices;
    worldVertices.reserve(object.localVertices.size());

    for (const glm::vec3& v : object.localVertices)
    {
        glm::vec4 world =
            object.model * glm::vec4(v, 1.0f);

        worldVertices.emplace_back(world.x, world.y, world.z);
    }

    return worldVertices;
}

void updateWorldBoundingBox(Object* object, Object* boxObject, glm::vec3 color)
{
    std::vector<glm::vec3> worldVertices = getWorldVertices(*object);

    if (worldVertices.empty())
        return;

    float minX = worldVertices[0].x;
    float maxX = worldVertices[0].x;
    float minY = worldVertices[0].y;
    float maxY = worldVertices[0].y;
    float minZ = worldVertices[0].z;
    float maxZ = worldVertices[0].z;

    for (const glm::vec3& v : worldVertices)
    {
        minX = glm::min(minX, v.x);
        maxX = glm::max(maxX, v.x);

        minY = glm::min(minY, v.y);
        maxY = glm::max(maxY, v.y);

        minZ = glm::min(minZ, v.z);
        maxZ = glm::max(maxZ, v.z);
    }

    glm::vec3 c0(minX, minY, minZ);
    glm::vec3 c1(maxX, minY, minZ);
    glm::vec3 c2(maxX, maxY, minZ);
    glm::vec3 c3(minX, maxY, minZ);

    glm::vec3 c4(minX, minY, maxZ);
    glm::vec3 c5(maxX, minY, maxZ);
    glm::vec3 c6(maxX, maxY, maxZ);
    glm::vec3 c7(minX, maxY, maxZ);


    std::vector<glm::vec3> vertices =
    {
        // untere Fläche
        c0,c1,
        c1,c5,
        c5,c4,
        c4,c0,

        // obere Fläche
        c3,c2,
        c2,c6,
        c6,c7,
        c7,c3,

        // Verbindungen
        c0,c3,
        c1,c2,
        c5,c6,
        c4,c7
    };


    // Position aktualisieren
    glBindBuffer(GL_ARRAY_BUFFER, boxObject->positionBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(glm::vec3),
        vertices.data(),
        GL_DYNAMIC_DRAW
    );

	std::vector<glm::vec3> colors(vertices.size(), color);

	// Farben aktualisieren
    glBindBuffer(GL_ARRAY_BUFFER, boxObject->colorBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        colors.size() * sizeof(glm::vec3),
        colors.data(),
        GL_DYNAMIC_DRAW
    );

    boxObject->vertexCount = vertices.size();
    boxObject->model = glm::mat4(1.0f);
}

void renderSphere(Object* object)
{
  glm::mat4 mv  = view * object->model;
  // Create mvp.
  glm::mat4 mvp = projection * mv;

  // Create normal matrix (nm) from model matrix.
  glm::mat3 nm = glm::inverseTranspose(glm::mat3(mv));

  // Bind the shader program and set uniform(s).
  currentProgram.use();
  currentProgram.setUniform("modelviewMatrix",  mv);
  currentProgram.setUniform("projectionMatrix", projection);
  currentProgram.setUniform("normalMatrix", nm);

  currentProgram.setUniform("surfKa", object->surfKa);
  currentProgram.setUniform("surfKd", object->surfKd);
  currentProgram.setUniform("surfKs", object->surfKs);
  currentProgram.setUniform("surfShininess", object->surfShininess);

  // Bind vertex array object
  glBindVertexArray(object->vao);
  glDrawElements(GL_TRIANGLES, 1500, GL_UNSIGNED_SHORT, 0);
  glBindVertexArray(0);
}

void renderLines(Object* object)
{
  // Create mvp.
  glm::mat4x4 mvp = projection * view * object->model;

  // Bind the shader program and set uniform(s).
  programSimple.use();
  programSimple.setUniform("mvp", mvp);

  // Bind vertex array object
  glBindVertexArray(object->vao);
  glDrawArrays(GL_LINES, 0, object->vertexCount);
  glBindVertexArray(0);
}

void renderBlenderModel(Object* object)
{
  glm::mat4 mv  = view * object->model;
  // Create mvp.
  glm::mat4 mvp = projection * mv;

  // Create normal matrix (nm) from model matrix.
  glm::mat3 nm = glm::inverseTranspose(glm::mat3(mv));

  // Bind the shader program and set uniform(s).
  currentProgram.use();
  currentProgram.setUniform("modelviewMatrix",  mv);
  currentProgram.setUniform("projectionMatrix", projection);
  currentProgram.setUniform("normalMatrix", nm);

  currentProgram.setUniform("light", lights[lightIndex]);
  currentProgram.setUniform("lightI", 1.0f);
  currentProgram.setUniform("surfKa", object->surfKa);
  currentProgram.setUniform("surfKd", object->surfKd);
  currentProgram.setUniform("surfKs", object->surfKs);
  currentProgram.setUniform("surfShininess", object->surfShininess);

  // Bind vertex array object
  glBindVertexArray(object->vao);
  glDrawArrays(GL_TRIANGLES, 0, object->vertexCount);
  glBindVertexArray(0);
}

void initTesselatedSphere(unsigned short n, float radius, glm::vec3 color, Object* object, glm::vec3 ka, glm::vec3 kd, glm::vec3 ks, float surfShininess)
{
  object->surfKa = ka;
  object->surfKd = kd;
  object->surfKs = ks;
  object->surfShininess = surfShininess;

  std::vector<glm::vec3> vertices = {};
  std::vector<glm::vec3> colors = {};
  std::vector<glm::vec3> normals = {};
  std::vector<GLushort> indices = {};
  
  // Create vertices
  for (int layer=0; layer<=2+n*2; layer++) {
    float y = cos((180 * ((float)layer/(2+(float)n*2))) * M_PI/180) * radius;
    float sin_remain = sin((180 * ((float)layer/(2+(float)n*2))) * M_PI/180) * radius;

    if (layer == 0 || layer == 2+n*2) { // if current layer is the first or last layer
      vertices.push_back(glm::vec3(0.0f, y, 0.0f)); // add vertex
	  normals.push_back(glm::normalize(glm::vec3(0.0f, y, 0.0f))); // add normal
      colors.push_back(color); // add matching color
    } else {
      if (layer <= (2+n*2) / 2) { // If layer is in the tp half
        for (int i=0; i<layer*4; i++) {
          float angle = 360 * (((float)i) + 1) / (((float)layer) * 4);
          float x = cos(angle * M_PI/180) * sin_remain;
          float z = sin(angle * M_PI/180) * sin_remain;
          vertices.push_back(glm::vec3(x, y, z)); // add vertex
		  normals.push_back(glm::normalize(glm::vec3(x, y, z))); // add normal
          colors.push_back(color); // add matching color
        }
      } else { // If layer is in the bottom half
        for (int i=0; i<((2+n*2) - layer)*4; i++) {
          float angle = 360 * (((float)i) + 1) / (((2+n*2) - layer)*4);
          float x = cos(angle * M_PI/180) * sin_remain;
          float z = sin(angle * M_PI/180) * sin_remain;
          vertices.push_back(glm::vec3(x, y, z)); // add vertex
          normals.push_back(glm::normalize(glm::vec3(x, y, z))); // add normal
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

      while (i <= currCount && j <= nextCount) {
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


  object->vertexCount = vertices.size();
  // Sphere object
  GLuint programId = currentProgram.getHandle();
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

  glGenBuffers(1, &object->normalBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, object->normalBuffer);
  glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);

  pos = glGetAttribLocation(programId, "normal");
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

  object->vertexCount = vertices.size();

  // Sphere object
  GLuint programId = programSimple.getHandle();
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

void initSpaceShipNormals(glm::vec3 color, Object* object, CornerList SpaceShipmesh) 
{
  if (!SpaceShipmesh.hasNormals) return;

  std::vector<glm::vec3> vertices;

  // Wir laufen durch alle n-Ecke und berechnen die Linien-Punkte im Local Space
  for (const auto& face : SpaceShipmesh.faces) {
    for (size_t i = 0; i < face.vertexIndices.size(); ++i) {
      glm::vec3 p_start = SpaceShipmesh.vertices[face.vertexIndices[i]];
      glm::vec3 n_dir = SpaceShipmesh.normals[face.normalIndices[i]];
      
      // Hinweis: 2.0f (deine Konstante) ist oft riesig für Normalen. 
      // Wenn die Linien zu lang sind, nimm hier lieber 0.1f oder 0.2f.
      glm::vec3 p_end = p_start + n_dir * NORMALS_LENGTH; 

      vertices.push_back(p_start);
      vertices.push_back(p_end);
    }
  }

  object->vertexCount = vertices.size();
  std::vector<glm::vec3> colors(vertices.size(), color);

  GLuint programId = programSimple.getHandle();
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

  normalsInitialized = true;
}

void initBlenderModel(const char* path, glm::vec3 modelColor, Object* object)
{
  std::vector<glm::vec3> vertices = {};
  std::vector<glm::vec3> normals = {};
  CornerList SpaceShipMesh;

  object->surfKa = glm::vec3(0.1f, 0.1f, 0.1f);
  object->surfKd = glm::vec3(0.5f, 0.5f, 0.5f);
  object->surfKs = glm::vec3(1.0f,1.0f,1.0f);
  object->surfShininess = 7.0f;
 
  if (!loadOBJ(path, SpaceShipMesh)) {
	return;
	}

  //Triangulierung der n-Ecke aus der Eckenliste
  for (const auto& face : SpaceShipMesh.faces) {
	size_t n = face.vertexIndices.size();
	if (n < 3) continue; //mindestens 3 Ecken

    // Fächer-Triangulierung:
    // Ein n-Eck wird in (n-2) Dreiecke zerlegt.
    for (size_t i = 1; i < n - 1; ++i) {
	  // Wir holen die echten 3D-Koordinaten aus den globalen Positions anhand der Indizes
      glm::vec3 p0 = SpaceShipMesh.vertices[face.vertexIndices[0]];
      glm::vec3 p1 = SpaceShipMesh.vertices[face.vertexIndices[i]];
	  glm::vec3 p2 = SpaceShipMesh.vertices[face.vertexIndices[i + 1]];

	  // Diese 3 Punkte bilden ein Dreieck zum zeichnen
      glm::vec3 n0 = SpaceShipMesh.normals[face.normalIndices[0]];
	  glm::vec3 n1 = SpaceShipMesh.normals[face.normalIndices[i]];
	  glm::vec3 n2 = SpaceShipMesh.normals[face.normalIndices[i + 1]];

	  vertices.push_back(p0);
	  vertices.push_back(p1);
	  vertices.push_back(p2);

	  normals.push_back(glm::normalize(n0));
	  normals.push_back(glm::normalize(n1));
	  normals.push_back(glm::normalize(n2));
    }
  }

//   for (const glm::vec3 normal : SpaceShipMesh.normals) {
// 	normals.push_back(glm::normalize(normal));
//   }

  // hasNormals = SpaceShipMesh.hasNormals;
  //Anzahl der Punkte zum zeichnen
  object->vertexCount = vertices.size();
  object->localVertices = vertices;

  //Jeder Punkte bekommt eine Farbe
  std::vector<glm::vec3> colors(vertices.size(), modelColor);
  
  GLuint programId = currentProgram.getHandle();
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

  glGenBuffers(1, &object->normalBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, object->normalBuffer);
  glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);

  pos = glGetAttribLocation(programId, "normal");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glBindVertexArray(0);
  object->model = glm::mat4(1.0f);

  if (SpaceShipMesh.hasNormals) {
    initSpaceShipNormals(glm::vec3(1.0f, 0.0f, 0.0f), &SpaceShipNormals, SpaceShipMesh);
  }
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
  if (!programSimple.compileShaderFromFile("shader/simple.vert", cg::GLSLShader::VERTEX)) {
    std::cerr << programSimple.log();
    return false;
  }
  
  if (!programSimple.compileShaderFromFile("shader/simple.frag", cg::GLSLShader::FRAGMENT)) {
    std::cerr << programSimple.log();
    return false;
  }

  if (!programSimple.link()) {
    std::cerr << programSimple.log();
    return false;
  }

  if (!programFlat.compileShaderFromFile("shader/shadedGouraud.vert", cg::GLSLShader::VERTEX)) {
    std::cerr << programFlat.log();
    return false;
  }
  
  if (!programFlat.compileShaderFromFile("shader/shadedGouraud.frag", cg::GLSLShader::FRAGMENT)) {
    std::cerr << programFlat.log();
    return false;
  }

  if (!programFlat.link()) {
    std::cerr << programFlat.log();
    return false;
  }

  if (!programPhong.compileShaderFromFile("shader/shadedPhong.vert", cg::GLSLShader::VERTEX)) {
    std::cerr << programPhong.log();
    return false;
  }

  if (!programPhong.compileShaderFromFile("shader/shadedPhong.frag", cg::GLSLShader::FRAGMENT)) {
    std::cerr << programPhong.log();
    return false;
  }

  if (!programPhong.link()) {
    std::cerr << programPhong.log();
    return false;
  }

  currentProgram = programFlat;


  programPhong.use();
  programPhong.setUniform("light",  lights[lightIndex]);
  programPhong.setUniform("lightI", float(1.0f));
  programPhong.setUniform("surfKa", glm::vec3(0.1f, 0.1f, 0.1f));
  programPhong.setUniform("surfKd", glm::vec3(0.8f, 0.1f, 0.1f));
  programPhong.setUniform("surfKs", glm::vec3(1, 1, 1));
  programPhong.setUniform("surfShininess", float(8.0f));

  programFlat.use();
  programFlat.setUniform("light", lights[lightIndex]);
  programFlat.setUniform("lightI", float(1.0f));
  programFlat.setUniform("surfKa", glm::vec3(0.1f,0.1f,0.1f));
  programFlat.setUniform("surfKd", glm::vec3(0.8f,0.8f,0.8f));
  programFlat.setUniform("surfKs", glm::vec3(1.0f,1.0f,1.0f));
  programFlat.setUniform("surfShininess", float(8.0f));

  // Create all objects.
  initTesselatedSphere(n, 0.3f, glm::vec3(1.0f, 1.0f, 0.0f), &sun , glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.3f,0.3f,0.3f), 8.0f);
  initLine(glm::vec3(0.0f, AXIS_LENGTH, 0.0f), glm::vec3(0.0f, -AXIS_LENGTH, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), &sunAxis);

  initTesselatedSphere(n, 0.15f, glm::vec3(0.0f, 0.0f, 1.0f), &inclinedPlanet, glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(0.1f, 0.1f, 0.8f), glm::vec3(1.0f,1.0f,1.0f), 8.0f);
  initLine(glm::vec3(0.0f, AXIS_LENGTH, 0.0f), glm::vec3(0.0f, -AXIS_LENGTH, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), &inclinedPlanetAxis);
  initTesselatedSphere(n, 0.07f, glm::vec3(0.5f, 0.5f, 0.5f), &inclinedMoon, glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(0.8f, 0.8f, 0.8f), glm::vec3(0.5f,0.5f,0.5f), 10.0f);

  initTesselatedSphere(n, 0.15f, glm::vec3(0.0f, 0.0f, 1.0f), &planet, glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(0.8f, 0.1f, 0.1f), glm::vec3(1.0f,1.0f,1.0f), 8.0f);
  initLine(glm::vec3(0.0f, AXIS_LENGTH, 0.0f), glm::vec3(0.0f, -AXIS_LENGTH, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), &planetAxis);
  initTesselatedSphere(n, 0.07f, glm::vec3(0.5f, 0.5f, 0.5f), &moon, glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(0.8f, 0.8f, 0.8f), glm::vec3(0.5f,0.5f,0.5f), 10.0f);

  initLine(glm::vec3(0),
         glm::vec3(0),
         glm::vec3(0,0,0),
         &SpaceShipBox);

  initBlenderModel("objects/smooth3-CG-Model.obj", glm::vec3(0.0f, 1.0f, 0.0f), &SpaceShip);
  //initBlenderModel("objects/CG-Model.obj", glm::vec3(0.0f, 1.0f, 0.0f), &SpaceShip);

  return true;
}

/*
 Rendering.
 */
void render()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (wireframe) {
	  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
	  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

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
  renderLines(&sunAxis);

  renderSphere(&inclinedPlanet);
  renderLines(&inclinedPlanetAxis);
  renderSphere(&inclinedMoon);

  renderSphere(&planet);
  renderLines(&planetAxis);
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

  if (showNormals && normalsInitialized){
    SpaceShipNormals.model = SpaceShip.model;
    renderLines(&SpaceShipNormals);
  }

  if (showBox == true){
    //SpaceShipBox.model = SpaceShip.model;
	updateWorldBoundingBox(&SpaceShip, &SpaceShipBox, glm::vec3(1.0f, 0.0f, 0.0f));
	renderLines(&SpaceShipBox);
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
  glm::vec3 eye(0.0f, 0.0f, zoom);
  glm::vec3 center(0.0f, 0.0f, 0.0f);
  glm::vec3 up(0.0f, 1.0f, 0.0f);

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
  case 'w':
	wireframe = !wireframe;
	break;
  case 's':
    shaded = !shaded;
	if (shaded) {
		currentProgram = programPhong;
	} else {
		currentProgram = programFlat;
	}
	break;
  case 'l':
  	lightIndex = 1 - lightIndex;

	programFlat.use();
	programFlat.setUniform("light", lights[lightIndex]);

	programPhong.use();
	programPhong.setUniform("light", lights[lightIndex]);
	break;
  case 'n':
	showNormals = !showNormals;
	break;
  case 'b':
    showBox = !showBox;
	break;
  case '+':
    if (zoom > MIN_ZOOM) {
      eye[2] -= 1;
      zoom -= 1;
      view = glm::lookAt(eye, center, up);
    }
    break;
  case '-':
    if (zoom < MAX_ZOOM) {
      eye[2] += 1;
      zoom += 1;
      view = glm::lookAt(eye, center, up);
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
  
  glutCreateWindow("Aufgabenblatt 04");
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
