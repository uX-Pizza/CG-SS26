#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

namespace cg
{
	struct Object
	{
		GLuint vao;
		GLuint positionBuffer;
		GLuint colorBuffer;
		GLuint normalBuffer;
		GLuint indexBuffer;

		GLuint indexCount;

		~Object()
		{
			glDeleteVertexArrays(1, &vao);
			glDeleteBuffers(1, &indexBuffer);
			glDeleteBuffers(1, &normalBuffer);
			glDeleteBuffers(1, &colorBuffer);
			glDeleteBuffers(1, &positionBuffer);
		}
	};
}
