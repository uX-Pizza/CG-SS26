#pragma once

#include <glm/glm.hpp>

namespace cg
{
	class AbstractCube
	{
	public:
		AbstractCube()
			: renderNormals(false)
		{

		}

		virtual ~AbstractCube()
		{

		}

		virtual void render(const glm::mat4& view, const glm::mat4& projection) = 0;
		virtual void setLightVector(const glm::vec4& v) = 0;

		glm::mat4& getModel()
		{
			return model;
		}

		void setRenderNormals(bool renderNormals)
		{
			this->renderNormals = renderNormals;
		}

		void toggleRenderNormals()
		{
			renderNormals = !renderNormals;
		}

		bool isRenderNormals() const
		{
			return renderNormals;
		}
	protected:
		glm::mat4 model;
		bool renderNormals;
	};
}
