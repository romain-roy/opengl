#pragma once

#include <glm/vec3.hpp>

#include <vector>

struct Triangle
{
	glm::vec3 p0;
	glm::vec3 n0;
	glm::vec3 p1;
	glm::vec3 n1;
	glm::vec3 p2;
	glm::vec3 n2;
};

std::vector<Triangle> ReadStl(const char* filename);
