#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp> 

// ACHTUNG: Die Parameter für UVs müssen vec2 sein, nicht vec3!
// Und ich habe es loadOBJ (großes J) genannt, damit dein Aufruf später funktioniert.
bool loadOBJ(
	const char* path,
	std::vector<glm::vec3>& out_vertices,
	std::vector<glm::vec2>& out_uvs,      // FEHLER BEHOBEN: Hier muss vec2 stehen
	std::vector<glm::vec3>& out_normals
) {
	std::cout << "Lade Object: " << path << "\n";

	// FEHLER BEHOBEN: Tippfehler korrigiert
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;      // FEHLER BEHOBEN: Hier muss vec2 stehen
	std::vector<glm::vec3> temp_normals;

	// FEHLER BEHOBEN: Tippfehler korrigiert
	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;

	// FEHLER BEHOBEN: Das Wichtigste! Wir müssen die Datei auch öffnen, 
	// sonst existiert 'file' gar nicht.
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cerr << "Fehler: Konnte Datei nicht öffnen: " << path << "\n";
		return false;
	}

	std::string lineHeader;
	while (file >> lineHeader)
	{
		if (lineHeader == "v") {
			glm::vec3 vertex;
			file >> vertex.x >> vertex.y >> vertex.z;
			temp_vertices.push_back(vertex);
		}
		else if (lineHeader == "vt") {
			glm::vec2 uvs;
			// FEHLER BEHOBEN: Hier stand uv.y statt uvs.y
			file >> uvs.x >> uvs.y;
			temp_uvs.push_back(uvs);
		}
		else if (lineHeader == "vn") {
			// FEHLER BEHOBEN: Normalen sind vec3 (3D), nicht vec2!
			glm::vec3 normals;
			file >> normals.x >> normals.y >> normals.z;
			// FEHLER BEHOBEN: Hier stand temp_uvs statt temp_normals
			temp_normals.push_back(normals);
		}
		else if (lineHeader == "f") {
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];

			char slash;
			for (int i = 0; i < 3; i++) {
				file >> vertexIndex[i] >> slash >> uvIndex[i] >> slash >> normalIndex[i];

				vertexIndices.push_back(vertexIndex[i]);
				uvIndices.push_back(uvIndex[i]);
				normalIndices.push_back(normalIndex[i]);
			}
		}
		else {
			std::string stupidBuffer;
			std::getline(file, stupidBuffer);
		}
	}

	for (unsigned int i = 0; i < vertexIndices.size(); i++) {
		unsigned int vertexIndex = vertexIndices[i];
		unsigned int uvIndex = uvIndices[i];
		unsigned int normalIndex = normalIndices[i];

		glm::vec3 vertex = temp_vertices[vertexIndex - 1];
		glm::vec2 uv = temp_uvs[uvIndex - 1];
		glm::vec3 normal = temp_normals[normalIndex - 1];

		out_vertices.push_back(vertex);
		out_uvs.push_back(uv);
		out_normals.push_back(normal);
	}

	file.close();
	std::cout << "Erfolgreich geladen!" << std::endl;
	return true;
}