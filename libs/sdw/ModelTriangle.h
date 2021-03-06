#pragma once

#include <glm/glm.hpp>
#include <string>
#include <array>
#include "Colour.h"
#include "TexturePoint.h"

struct ModelTriangle {
	std::array<glm::vec3, 3> vertices{};
	std::array<TexturePoint, 3> texturePoints{};
	float Ns;
	glm::vec3 Ka;
	Colour colour{};
	float Ni;
	float d;
	std::string map_Kd;
	glm::vec3 normal{};
	glm::vec3 v0Normal{};
	glm::vec3 v1Normal{};
	glm::vec3 v2Normal{};

	ModelTriangle();
	ModelTriangle(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, Colour trigColour);
	friend std::ostream &operator<<(std::ostream &os, const ModelTriangle &triangle);
};
