#ifndef VERTEX_FORMATS_H
#define VERTEX_FORMATS_H

#include <glm/glm.hpp>

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
};

#endif