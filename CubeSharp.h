#pragma once

#include <iostream>
#include <vector>
#include <algorithm>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "GLSLProgram.h"
#include "GLTools.h"
#include "Object.h"
#include "AbstractCube.h"

namespace cg
{
	class CubeSharp : public AbstractCube
	{
	public:
		CubeSharp(const glm::vec3& color = glm::vec3(0.8f))
		{
			initShader();
			initObject(color);
		}

		void initShader()
		{
			CubeSharp::initShader(programSimple, "shader/simple.vert", "shader/simple.frag");
			//CubeSharp::initShader(programShaded, "shader/shadedGouraud.vert", "shader/shadedGouraud.frag");
			CubeSharp::initShader(programShaded, "shader/shadedPhong.vert", "shader/shadedPhong.frag");

			programShaded.use();
			programShaded.setUniform("light",  glm::vec3(0, 0, 0));
			programShaded.setUniform("lightI", float(1.0f));
			programShaded.setUniform("surfKa", glm::vec3(0.1f, 0.1f, 0.1f));
			programShaded.setUniform("surfKd", glm::vec3(0.7f, 0.1f, 0.1f));
			programShaded.setUniform("surfKs", glm::vec3(1, 1, 1));
			programShaded.setUniform("surfShininess", float(8.0f));
		}

		void initObject(const glm::vec3& color)
		{
			std::vector<glm::vec3> vertices = { { -1.0f, 1.0f, -1.0f, }, { 1.0f, 1.0f, -1.0f }, { 1.0f, -1.0f, -1.0f }, { -1.0f, -1.0f, -1.0f }, { -1.0f, 1.0f, 1.0f, }, { 1.0f, 1.0f, 1.0f }, { 1.0f, -1.0f, 1.0f }, { -1.0f, -1.0f, 1.0f } };
			
			// Build the cube.
			std::vector<glm::vec3> positions;
			std::vector<glm::vec3> colors;
			std::vector<glm::vec3> normals;
			std::vector<GLuint> indices;

			CubeSharp::addQuad(positions, normals, colors, color, vertices[0], vertices[1], vertices[2], vertices[3]);
			CubeSharp::addQuad(positions, normals, colors, color, vertices[4], vertices[0], vertices[3], vertices[7]);
			CubeSharp::addQuad(positions, normals, colors, color, vertices[1], vertices[5], vertices[6], vertices[2]);
			CubeSharp::addQuad(positions, normals, colors, color, vertices[4], vertices[5], vertices[1], vertices[0]);
			CubeSharp::addQuad(positions, normals, colors, color, vertices[3], vertices[2], vertices[6], vertices[7]);
			CubeSharp::addQuad(positions, normals, colors, color, vertices[7], vertices[6], vertices[5], vertices[4]);
			
			for (GLushort i = 0; i < positions.size(); i++)
				indices.push_back(i);

			GLuint programId = programShaded.getHandle();
			GLuint pos;

			// Vertex array object.
			glGenVertexArrays(1, &objCube.vao);
			glBindVertexArray(objCube.vao);

			// Position buffer.
			glGenBuffers(1, &objCube.positionBuffer);
			glBindBuffer(GL_ARRAY_BUFFER, objCube.positionBuffer);
			glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3), positions.data(), GL_STATIC_DRAW);

			pos = glGetAttribLocation(programId, "position");
			glEnableVertexAttribArray(pos);
			glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

			// Color buffer.
			// glGenBuffers(1, &objCube.colorBuffer);
			// glBindBuffer(GL_ARRAY_BUFFER, objCube.colorBuffer);
			// glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);
			
			// pos = glGetAttribLocation(programId, "color");
			// std::cout << "color location: " << pos << std::endl;
			// glEnableVertexAttribArray(pos);
			// glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
			
			// Normal buffer.
			glGenBuffers(1, &objCube.normalBuffer);
			glBindBuffer(GL_ARRAY_BUFFER, objCube.normalBuffer);
			glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);

			pos = glGetAttribLocation(programId, "normal");
			glEnableVertexAttribArray(pos);
			glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

			// Index buffer.
			objCube.indexCount = (GLuint)indices.size();

			glGenBuffers(1, &objCube.indexBuffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objCube.indexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, objCube.indexCount * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

			// Build the normals.
			programId = programSimple.getHandle();

			std::vector<glm::vec3> positions2;
			std::vector<glm::vec3> colors2;
			std::vector<GLuint> indices2;

			const glm::vec3 colorNormal(1.0f, 1.0f, 1.0f);
			
			// create vertex-array-object for normal lines
			// Front
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[0], normals[0]);
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[8], normals[8]);
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[22], normals[22]);

			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[2], normals[2]);
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[12], normals[12]);
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[23], normals[23]);

			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[1], normals[1]);
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[16], normals[16]);
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[26], normals[26]);

			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[4], normals[4]);
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[7], normals[7]);
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[24], normals[24]);

			// Back
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[34], normals[34]);
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[6], normals[6]);
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[18], normals[18]);

			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[31], normals[31]);
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[20], normals[20]);
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[14], normals[14]);
			
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[32], normals[32]);
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[13], normals[13]);
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[25], normals[25]);
			
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[30], normals[30]);
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[10], normals[10]);
			CubeSharp::addNormal(positions2, colors2, colorNormal, positions[28], normals[28]);

			for (GLushort i = 0; i < positions2.size(); i++)
				indices2.push_back(i);
		
			// Vertex array object.
			glGenVertexArrays(1, &objNormals.vao);
			glBindVertexArray(objNormals.vao);

			// Position buffer.
			glGenBuffers(1, &objNormals.positionBuffer);
			glBindBuffer(GL_ARRAY_BUFFER, objNormals.positionBuffer);
			glBufferData(GL_ARRAY_BUFFER, positions2.size() * sizeof(glm::vec3), positions2.data(), GL_STATIC_DRAW);

			pos = glGetAttribLocation(programId, "position");
			glEnableVertexAttribArray(pos);
			glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

			// Color buffer.
			glGenBuffers(1, &objNormals.colorBuffer);
			glBindBuffer(GL_ARRAY_BUFFER, objNormals.colorBuffer);
			glBufferData(GL_ARRAY_BUFFER, colors2.size() * sizeof(glm::vec3), colors2.data(), GL_STATIC_DRAW);

			pos = glGetAttribLocation(programId, "color");
			glEnableVertexAttribArray(pos);
			glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
			
			// Index buffer.
			objNormals.indexCount = (GLuint)indices2.size();

			glGenBuffers(1, &objNormals.indexBuffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objNormals.indexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, objNormals.indexCount * sizeof(GLuint), indices2.data(), GL_STATIC_DRAW);
		}

		~CubeSharp()
		{
			// GLSL Programs and objects will be automatically released upon object destruction.
		}

		void render(const glm::mat4& view, const glm::mat4& projection)
		{
 		        glm::mat4 mv  = view * model;
			// Create mvp.
			glm::mat4 mvp = projection * mv;

			// Create normal matrix (nm) from model matrix.
			glm::mat3 nm = glm::inverseTranspose(glm::mat3(model));

			// Bind the shader program and set uniform(s).
			programShaded.use();
			programShaded.setUniform("modelviewMatrix",  mv);
			programShaded.setUniform("projectionMatrix", projection);
			programShaded.setUniform("normalMatrix", nm);
			
			// Bind vertex array object so we can render the triangle.
			glBindVertexArray(objCube.vao);
			glDrawElements(GL_TRIANGLES, objCube.indexCount, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);

			if (renderNormals)
			{
				// Bind the shader program and set uniform(s).
				programSimple.use();
				programSimple.setUniform("mvp", mvp);

				// Bind vertex array object so we can render the triangle.
				glBindVertexArray(objNormals.vao);
				glDrawElements(GL_LINES, objNormals.indexCount, GL_UNSIGNED_INT, 0);
				glBindVertexArray(0);
			}
		}
		
		void setLightVector (const glm::vec4& v)
		{
			programShaded.use();
			programShaded.setUniform("light", v);
		}
	private:
		static void addNormal(std::vector<glm::vec3>& positions, std::vector<glm::vec3>& colors, const glm::vec3& color, const glm::vec3& vertex, const glm::vec3& normal, float scale = 0.5f)
		{
			positions.push_back(vertex);
			positions.push_back(vertex + normal * scale);
			
			colors.push_back(color);
			colors.push_back(color);
		}

		static void addQuad(std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::vector<glm::vec3>& colors, const glm::vec3& color, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d)
		{
			addTriangle(positions, normals, colors, color, a, c, b);
			addTriangle(positions, normals, colors, color, a, d, c);
		}

		static void addTriangle(std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::vector<glm::vec3>& colors,
								const glm::vec3& color, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
		{
			// Calculate normal for the triangle.
			auto u = b - a;
			auto v = c - a;

			auto normal = glm::normalize(glm::cross(v, u));

			// Add vertices, normals and colors to the buffers.
			positions.push_back(a);
			positions.push_back(b);
			positions.push_back(c);

			normals.push_back(normal);
			normals.push_back(normal);
			normals.push_back(normal);

			colors.push_back(color);
			colors.push_back(color);
			colors.push_back(color);
		}

		static void initShader(GLSLProgram& program, const std::string& vert, const std::string& frag)
		{
			if (!program.compileShaderFromFile(vert.c_str(), cg::GLSLShader::VERTEX))
			{
				throw std::runtime_error("COMPILE VERTEX: " + program.log());
			}

			if (!program.compileShaderFromFile(frag.c_str(), cg::GLSLShader::FRAGMENT))
			{
				throw std::runtime_error("COMPILE FRAGMENT: " + program.log());
			}

			if (!program.link())
			{
				throw std::runtime_error("LINK: " + program.log());
			}
		}
	private:
		GLSLProgram programShaded;
		GLSLProgram programSimple;
		
		Object objCube;
		Object objNormals;
	};
}
