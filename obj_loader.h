#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp> 

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
};

struct Face
{
	std::vector<unsigned int> vertexIndices;
	std::vector<unsigned int> normalIndices;
};

struct EdgesList
{
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<Face> faces;
	bool hasNormals = false;
};



bool loadOBJ(const char* path, EdgesList& mesh) {
    std::cout << "Lade Object: " << path << "\n";

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Fehler: Konnte Datei nicht öffnen: " << path << "\n";
        return false;
    }

    std::string lineHeader;
    while (file >> lineHeader) {
        if (lineHeader == "v") {
            glm::vec3 pos;
            file >> pos.x >> pos.y >> pos.z;
            mesh.vertices.push_back(pos); // Direkt ins Mesh schreiben
        } 
        else if (lineHeader == "vn") {
            glm::vec3 norm;
            file >> norm.x >> norm.y >> norm.z;
            mesh.normals.push_back(norm);  // Direkt ins Mesh schreiben
        }
        else if (lineHeader == "f") {
            std::string lineRest;
            std::getline(file, lineRest);
            std::stringstream ss(lineRest);
            std::string token;
            
            Face currentFace;
            
            while (ss >> token) {
                size_t slash1 = token.find('/');
                unsigned int vIdx = 0;
                unsigned int nIdx = 0;
                
                if (slash1 == std::string::npos) {
                    vIdx = std::stoi(token);
                } else {
                    vIdx = std::stoi(token.substr(0, slash1));
                    size_t slash2 = token.find('/', slash1 + 1);
                    if (slash2 != std::string::npos) {
                        std::string vnPart = token.substr(slash2 + 1);
                        if (!vnPart.empty()) {
                            nIdx = std::stoi(vnPart);
                        }
                    }
                }
                
                currentFace.vertexIndices.push_back(vIdx - 1); // 1-basiert zu 0-basiert
                if (nIdx > 0) {
                    currentFace.normalIndices.push_back(nIdx - 1);
                    mesh.hasNormals = true; // Sobald eine Normale da ist, wird es true
                }
            }
            mesh.faces.push_back(currentFace);
        }
        else {
            // Zeilen wie vt, g, o, s oder Kommentare ignorieren
            std::string dummy;
            std::getline(file, dummy);
        }
    }

    file.close();
    std::cout << "Erfolgreich geladen! " 
              << mesh.vertices.size() << " Vertices, " 
              << mesh.faces.size() << " Faces.\n";
    return true;
}