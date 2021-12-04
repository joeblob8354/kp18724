#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>
#include <CanvasPoint.h>
#include <Colour.h>
#include <CanvasTriangle.h>
#include <TextureMap.h>
#include <TexturePoint.h>
#include <ModelTriangle.h>
#include <RayTriangleIntersection.h>
#include <algorithm>

#define WIDTH 320
#define HEIGHT 240
#define pi 3.14159265358979323846 
std::string renderMethod = "raster";
std::string fileName = "cornell+sphere";
bool texture = false;
std::string textureFile = "checkerboardfloor.ppm";
std::string textureName = "Checkerboard";
glm::vec3 cameraPosition(0.0, 0.0, 10.0);
float scale = 75;
float focalLength = 4;
float depthBuffer[HEIGHT][WIDTH];
glm::mat3 rotationMatrix(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
glm::mat3 cameraOrientation(glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
glm::vec3 lightSource(0, 0.45, 0);

std::pair <std::vector<Colour>, std::vector<TextureMap>> mtlReader() {
	std::vector<std::string> colourNames;
	std::vector<Colour> palette;
	std::vector<TextureMap> texturePalette;

	std::ifstream file(fileName + ".mtl");
	std::string line;
	for (line; std::getline(file, line);) {
		std::vector<std::string> tokens = split(line, ' ');
		if (tokens[0] == "newmtl") {
			colourNames.push_back(tokens[1]);
		}
		else if (tokens[0] == "Kd") {
			float red, green, blue;
			red = stof(tokens[1]);
			green = stof(tokens[2]);
			blue = stof(tokens[3]);
			Colour myColour;
			myColour.red = round(red * 255);
			myColour.green = round(green * 255);
			myColour.blue = round(blue * 255);
			palette.push_back(myColour);
		}
		else if (tokens[0] == "map_Kd") {
			std::string filename = tokens[1];
			TextureMap texture(filename);
			texturePalette.push_back(texture);
		}
	}

	for (int i = 0; i < palette.size(); i++) {
		palette[i].name = colourNames[i];
	}
	for (int i = 0; i < texturePalette.size(); i++) {
		texturePalette[i].name = colourNames[i];
	}
	std::pair<std::vector<Colour>, std::vector<TextureMap>> res;
	res.first = palette;
	res.second = texturePalette;
	return res;
}

std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>> vertexTranslator(std::vector<unsigned int> faceVertices, std::vector<glm::vec3> vertices, std::vector<unsigned int> faceVertexNormals, std::vector<glm::vec3> vertexNormals) {
	std::vector<glm::vec3> outputVertices;
	std::vector<glm::vec3> outputVertexNormals;
	//loop through each vertex of each triangle (f lines)
	if (faceVertices.size() != 0) {
		for (unsigned int i = 0; i < faceVertices.size(); i++) {
			//get position of the vertex since obj indexes from 1
			glm::vec3 vertex = vertices[faceVertices[i] - 1];
			if (vertexNormals.size() > 0) {
				glm::vec3 vertexNormal = vertexNormals[faceVertexNormals[i] - 1];
				outputVertexNormals.push_back(vertexNormal);
			}
			outputVertices.push_back(vertex);
		}
	}
	std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>> res = { outputVertices, outputVertexNormals };
	return res;
}

std::vector<TexturePoint> textureTranslator(std::vector<unsigned int> texturePointIndexes, std::vector<TexturePoint> texturePoints) {
	std::vector<TexturePoint> outputTexturePoints;
	//loop through each vertex of each triangle (f lines)
	for (unsigned int i = 0; i < texturePointIndexes.size(); i++) {
		//get position of the vertex since obj indexes from 1
		TexturePoint texturePoint = texturePoints[texturePointIndexes[i] - 1];
		outputTexturePoints.push_back(texturePoint);
	}
	return outputTexturePoints;
}

std::vector<ModelTriangle> modelTriangleMaker(std::vector<glm::vec3> vertices, std::string colour, std::vector<Colour> palette, std::vector<TextureMap> texturePalette, std::vector<TexturePoint> texturePoints, std::vector<glm::vec3> vertexNormals) {
	std::vector<ModelTriangle> modelTriangles;
	Colour red;
	red.red = 255;
	red.green = 0;
	red.blue = 0;
	if (vertices.size() != 0) {
		for (int i = 0; i < vertices.size(); i += 3) {
			int i1 = i + 1;
			int i2 = i + 2;
			ModelTriangle myTriangle;
			myTriangle.vertices[0] = vertices[i];
			myTriangle.vertices[1] = vertices[i1];
			myTriangle.vertices[2] = vertices[i2];
			for (int y = 0; y < palette.size(); y++) {
				if (palette[y].name == colour) {
					myTriangle.colour = palette[y];
				}
			}
			if (texturePoints.size() > 0) {
				myTriangle.texturePoints[0] = texturePoints[i];
				myTriangle.texturePoints[1] = texturePoints[i1];
				myTriangle.texturePoints[2] = texturePoints[i2];
			}
			for (int y = 0; y < texturePalette.size(); y++) {
				if (texturePalette[y].name == colour) {
					TextureMap textureMap = texturePalette[y];
					myTriangle.colour.name = colour;
				}
			}
			if (vertexNormals.size() > 0) {
				myTriangle.v0Normal = vertexNormals[i];
				myTriangle.v1Normal = vertexNormals[i1];
				myTriangle.v2Normal = vertexNormals[i2];
			}
			if (palette.size() == 0) {
				myTriangle.colour = red;
			}
			modelTriangles.push_back(myTriangle);
		}
	}
	return modelTriangles;
}

std::vector<ModelTriangle> objReader() {
	std::string objectName;
	std::string objectColour;
	std::pair <std::vector<Colour>, std::vector<TextureMap>> mtlRes = mtlReader();
	std::vector<Colour> palette = mtlRes.first;
	std::vector<TextureMap> texturePalette = mtlRes.second;
	std::vector<glm::vec3> vertices;
	std::vector<TexturePoint> texturePoints;
	std::vector<glm::vec3> vertexNormals;
	std::vector<unsigned int> faceVertices;
	std::vector<unsigned int> texturePointIndexes;
	std::vector<unsigned int> faceVerticeNormals;
	std::vector<ModelTriangle> finalTriangles;

	std::ifstream file(fileName + ".obj");
	std::string line;
	for (line; std::getline(file, line);) {
		std::vector<std::string> tokens = split(line, ' ');
		if (tokens[0] == "o") {
			std::vector<glm::vec3> objectVertices;
			std::vector<glm::vec3> objectVertexNormals;
			std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>> res = vertexTranslator(faceVertices, vertices, faceVerticeNormals, vertexNormals);
			objectVertices = res.first;
			objectVertexNormals = res.second;
			std::vector<TexturePoint> objectTexturePoints = textureTranslator(texturePointIndexes, texturePoints);
			std::vector<ModelTriangle> objectTriangles = modelTriangleMaker(objectVertices, objectColour, palette, texturePalette, objectTexturePoints, objectVertexNormals);
			for (int i = 0; i < objectTriangles.size(); i++) {
				finalTriangles.push_back(objectTriangles[i]);
			}
			faceVertices.clear();
			texturePointIndexes.clear();
			faceVerticeNormals.clear();
			objectName = tokens[1];
		}
		else if (tokens[0] == "usemtl") {
			objectColour = tokens[1];
		}
		else if (tokens[0] == "v") {
			glm::vec3 vertex;
			for (int i = 1; i < tokens.size(); i++) {
				vertex[i - 1] = stof(tokens[i]);
			}
			vertices.push_back(vertex);
		}
		else if (tokens[0] == "vt") {
			TexturePoint texturePoint;
			texturePoint.x = stof(tokens[1]);
			texturePoint.y = stof(tokens[2]);
			texturePoints.push_back(texturePoint);
		}
		else if (tokens[0] == "vn") {
			glm::vec3 vertexNormal;
			for (int i = 1; i < tokens.size(); i++) {
				vertexNormal[i - 1] = stof(tokens[i]);
			}
			vertexNormals.push_back(vertexNormal);
		}
		else if (tokens[0] == "f") {
			glm::vec3 vertices;
			float vertex;
			float texturePointIndex;
			glm::vec3 vertexNormals;
			float vertexNormal;
			for (int i = 1; i < tokens.size(); i++) {
				std::vector<std::string> tokenVertices = split(tokens[i], '/');
				vertex = stof(tokenVertices[0]);
				vertices[i - 1] = vertex;
				faceVertices.push_back(vertices[i - 1]);
				if (tokenVertices[1] != "") {
					texturePointIndex = stof(tokenVertices[1]);
					texturePointIndexes.push_back(texturePointIndex);
				}
				if (tokenVertices.size() >= 3) {
					vertexNormal = stof(tokenVertices[2]);
					vertexNormals[i - 1] = vertexNormal;
					faceVerticeNormals.push_back(vertexNormals[i - 1]);
				}
			}
		}
	}
	std::vector<glm::vec3> objectVertices;
	std::vector<glm::vec3> objectVertexNormals;
	std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>> res = vertexTranslator(faceVertices, vertices, faceVerticeNormals, vertexNormals);
	objectVertices = res.first;
	objectVertexNormals = res.second;
	std::vector<TexturePoint> objectTexturePoints = textureTranslator(texturePointIndexes, texturePoints);
	std::vector<ModelTriangle> objectTriangles = modelTriangleMaker(objectVertices, objectColour, palette, texturePalette, objectTexturePoints, objectVertexNormals);
	for (int i = 0; i < objectTriangles.size(); i++) {
		finalTriangles.push_back(objectTriangles[i]);
	}
	for (int i = 0; i < finalTriangles.size(); i++) {
		finalTriangles[i].normal = glm::cross((finalTriangles[i].vertices[1] - finalTriangles[i].vertices[0]), (finalTriangles[i].vertices[2] - finalTriangles[i].vertices[0]));
	}
	return finalTriangles;
}

void drawLine(DrawingWindow& window, CanvasPoint from, CanvasPoint to, uint32_t colour) {
	float xDiff = from.x - to.x;
	float yDiff = from.y - to.y;
	float depthDiff = 1 / from.depth - 1 / to.depth;
	float numberOfSteps1 = std::max(abs(xDiff), abs(yDiff));
	float numberOfSteps = std::max(abs(numberOfSteps1), abs(depthDiff));
	float xStepSize = xDiff / numberOfSteps;
	float yStepSize = yDiff / numberOfSteps;
	float depthStepSize = depthDiff / numberOfSteps;

	for (float i = 0; i < numberOfSteps; i++) {
		float x = to.x + (xStepSize * i);
		float y = to.y + (yStepSize * i);
		float newDepth = abs(1 / (1 / to.depth + (depthStepSize * i)));
		int xi = round(x);
		int yi = round(y);
		if (xi < WIDTH && xi >= 0 && yi < HEIGHT && yi >= 0) {
			float oldDepth = depthBuffer[yi][xi];
			if (oldDepth < 1 / newDepth) {
				window.setPixelColour(xi, yi, colour);
				depthBuffer[yi][xi] = 1 / newDepth;
			}
		}
	}
}

CanvasPoint getCanvasIntersectionPoint(glm::vec3 vertexPosition) {
	CanvasPoint projectedVertex;
	glm::vec3 newVertex = vertexPosition - cameraPosition;
	newVertex = newVertex * cameraOrientation;
	projectedVertex.x = -(focalLength * newVertex.x / newVertex.z) * scale;
	projectedVertex.y = (focalLength * newVertex.y / newVertex.z) * scale;
	projectedVertex.x = projectedVertex.x + (WIDTH / 2);
	projectedVertex.y = projectedVertex.y + (HEIGHT / 2);
	projectedVertex.depth = newVertex.z;
	return projectedVertex;
}

glm::vec3 getRayIntersection(glm::vec3 cameraPosition, ModelTriangle triangle, glm::vec3 rayDirection) {
	glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
	glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
	glm::vec3 SPVector = cameraPosition - triangle.vertices[0];
	glm::mat3 DEMatrix(-rayDirection, e0, e1);
	glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;
	return possibleSolution;
}

RayTriangleIntersection getClosestIntersection(glm::vec3 cameraPosition, std::vector<ModelTriangle> modelTriangles, glm::vec3 ray, float triangleIndex) {
	ray = glm::normalize(ray);
	std::vector<RayTriangleIntersection> rayTriangleIntersections;
	for (int i = 0; i < modelTriangles.size(); i++) {
		glm::vec3 e0 = modelTriangles[i].vertices[1] - modelTriangles[i].vertices[0];
		glm::vec3 e1 = modelTriangles[i].vertices[2] - modelTriangles[i].vertices[0];
		glm::vec3 SPVector = cameraPosition - modelTriangles[i].vertices[0];
		glm::mat3 DEMatrix(-ray, e0, e1);
		glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;
		float u = possibleSolution[1];
		float v = possibleSolution[2];
		float w = u + v;
		if (u >= 0.0 && u <= 1.0 && v >= 0.0 && v <= 1.0 && w <= 1.0 && possibleSolution[0] >= 0) {
			RayTriangleIntersection rayTriangleIntersection;
			rayTriangleIntersection.distanceFromCamera = possibleSolution[0];
			rayTriangleIntersection.intersectedTriangle = modelTriangles[i];
			rayTriangleIntersection.intersectionPoint = modelTriangles[i].vertices[0] + u * e0 + v * e1;
			rayTriangleIntersection.triangleIndex = i;
			if (i != triangleIndex) {
				rayTriangleIntersections.push_back(rayTriangleIntersection);
			}
		}
	}
	RayTriangleIntersection closestTriangle;
	closestTriangle.distanceFromCamera = INFINITY;
	for (int i = 0; i < rayTriangleIntersections.size(); i++) {
		if (abs(rayTriangleIntersections[i].distanceFromCamera) < abs(closestTriangle.distanceFromCamera)) {
			closestTriangle = rayTriangleIntersections[i];
		}
	}
	return closestTriangle;
}

glm::vec3 getRayDirection(CanvasPoint projectedVertex) {
	glm::vec3 locationInSpace;
	locationInSpace[0] = (projectedVertex.x - (WIDTH / 2)) / scale;
	locationInSpace[1] = -(projectedVertex.y - (HEIGHT / 2)) / scale;
	locationInSpace[2] = -focalLength;
	locationInSpace = locationInSpace * glm::inverse(cameraOrientation);
	return locationInSpace;
}

bool sortByY(CanvasPoint& lhs, CanvasPoint& rhs) {
	return lhs.y < rhs.y;
}

void drawFilledTriangle(DrawingWindow& window, CanvasTriangle triangle, uint32_t colour) {
	//sort vertices by their y coords
	std::sort(triangle.vertices.begin(), triangle.vertices.end(), sortByY);

	CanvasPoint topVertex = triangle.vertices[0];
	CanvasPoint middleVertex = triangle.vertices[1];
	CanvasPoint bottomVertex = triangle.vertices[2];

	CanvasPoint extraPoint;
	extraPoint.y = middleVertex.y;

	//finding x coord of the 'extra point'
	float yDiff = bottomVertex.y - topVertex.y;
	float xDiff = bottomVertex.x - topVertex.x;
	float extraPointYDiff = extraPoint.y - topVertex.y;
	float ratio = extraPointYDiff / yDiff;
	extraPoint.x = topVertex.x + ratio * xDiff;
	
	//find the depth of the extra point
	float depthDiff = 1 / bottomVertex.depth - 1 / topVertex.depth;
	extraPoint.depth = 1 / (1 / topVertex.depth + ratio * depthDiff);

	//use interpolation to fill top triangle section, line by line
	float line1XDiff = extraPoint.x - topVertex.x;
	float line1YDiff = extraPoint.y - topVertex.y;
	float numberOfSteps1 = std::max(abs(line1XDiff), abs(line1YDiff));

	float line2XDiff = middleVertex.x - topVertex.x;
	float line2YDiff = middleVertex.y - topVertex.y;
	float numberOfSteps2 = std::max(abs(line2XDiff), abs(line2YDiff));

	float numberOfSteps3 = std::max(abs(numberOfSteps1), abs(numberOfSteps2));

	float line1DepthDiff = 1 / extraPoint.depth - 1 / topVertex.depth;
	float line2DepthDiff = 1 / middleVertex.depth - 1 / topVertex.depth;
	float numberOfSteps4 = std::max(abs(line1DepthDiff), abs(line2DepthDiff));

	float numberOfSteps = std::max(abs(numberOfSteps3), abs(numberOfSteps4));

	float line1XStepSize = line1XDiff / numberOfSteps;
	float line1YStepSize = line1YDiff / numberOfSteps;
	float line2XStepSize = line2XDiff / numberOfSteps;
	float line2YStepSize = line2YDiff / numberOfSteps;
	float line1DepthStepSize = line1DepthDiff / numberOfSteps;
	float line2DepthStepSize = line2DepthDiff / numberOfSteps;

	CanvasPoint point1;
	CanvasPoint point2;
	for (float i = 0; i < numberOfSteps + 1; i++) {
		float x1 = topVertex.x + (line1XStepSize * i);
		float y1 = topVertex.y + (line1YStepSize * i);
		point1.x = x1;
		point1.y = y1;
		point1.depth = 1 / (1 / topVertex.depth + (line1DepthStepSize * i));

		float x2 = topVertex.x + (line2XStepSize * i);
		float y2 = topVertex.y + (line2YStepSize * i);
		point2.x = x2;
		point2.y = y2;
		point2.depth = 1 / (1 / topVertex.depth + (line2DepthStepSize * i));

		drawLine(window, point1, point2, colour);
	}

	//use interpolation to fill bottom triangle section, line by line
	line1XDiff = extraPoint.x - bottomVertex.x;
	line1YDiff = extraPoint.y - bottomVertex.y;
	numberOfSteps1 = std::max(abs(line1XDiff), abs(line1YDiff));

	line2XDiff = middleVertex.x - bottomVertex.x;
	line2YDiff = middleVertex.y - bottomVertex.y;
	numberOfSteps2 = std::max(abs(line2XDiff), abs(line2YDiff));

	numberOfSteps3 = std::max(abs(numberOfSteps1), abs(numberOfSteps2));

	line1DepthDiff = 1 / extraPoint.depth - 1 / bottomVertex.depth;
	line2DepthDiff = 1 / middleVertex.depth - 1 / bottomVertex.depth;
	numberOfSteps4 = std::max(abs(line1DepthDiff), abs(line2DepthDiff));

	numberOfSteps = std::max(abs(numberOfSteps3), abs(numberOfSteps4));

	line1XStepSize = line1XDiff / numberOfSteps;
	line1YStepSize = line1YDiff / numberOfSteps;
	line2XStepSize = line2XDiff / numberOfSteps;
	line2YStepSize = line2YDiff / numberOfSteps;
	line1DepthStepSize = line1DepthDiff / numberOfSteps;
	line2DepthStepSize = line2DepthDiff / numberOfSteps;

	for (float i = 0; i < numberOfSteps; i++) {
		float x1 = bottomVertex.x + (line1XStepSize * i);
		float y1 = bottomVertex.y + (line1YStepSize * i);
		point1.x = x1;
		point1.y = y1;
		point1.depth = 1 / (1 / bottomVertex.depth + (line1DepthStepSize * i));

		float x2 = bottomVertex.x + (line2XStepSize * i);
		float y2 = bottomVertex.y + (line2YStepSize * i);
		point2.x = x2;
		point2.y = y2;
		point2.depth = 1 / (1 / bottomVertex.depth + (line2DepthStepSize * i));

		drawLine(window, point1, point2, colour);
	}
}

void drawStrokedTriangle(DrawingWindow& window, CanvasTriangle triangle, uint32_t colour) {
	//get RGB colour for triangle
	drawLine(window, triangle.vertices[0], triangle.vertices[1], colour);
	drawLine(window, triangle.vertices[1], triangle.vertices[2], colour);
	drawLine(window, triangle.vertices[2], triangle.vertices[0], colour);
}

void lookAt(glm::vec3 pointToLookAt) {
	cameraOrientation[2] = cameraPosition - pointToLookAt;
	glm::vec3 vertical(0, 1, 0);
	cameraOrientation[0] = glm::cross(vertical, cameraOrientation[2]);
	cameraOrientation[1] = glm::cross(cameraOrientation[2], cameraOrientation[0]);
	cameraOrientation[0] = glm::normalize(cameraOrientation[0]);
	cameraOrientation[1] = glm::normalize(cameraOrientation[1]);
	cameraOrientation[2] = glm::normalize(cameraOrientation[2]);
}

float clamp(float intensity) {
	if (intensity > 1) {
		intensity = 1;
	}
	if (intensity < 0) {
		intensity = 0;
	}
	return intensity;
}

Colour getColour(uint32_t RGBcolour) {
	uint32_t alphaOffset{ 0x20 };
	uint32_t redOffset{ 0x10 };
	uint32_t greenOffset{ 0x08 };
	uint32_t blueOffset{ 0x00 };
	
	uint32_t byteMask{ 0xFF };
	uint32_t alphaMask{ byteMask << alphaOffset };
	uint32_t redMask{ byteMask << redOffset };
	uint32_t greenMask{ byteMask << greenOffset };
	uint32_t blueMask{ byteMask << blueOffset };

	Colour colour;
	colour.red = (RGBcolour & redMask >> redOffset) & byteMask;
	colour.green = (RGBcolour & greenMask >> greenOffset) & byteMask;
	colour.blue = (RGBcolour & blueMask >> blueOffset) & byteMask;

	return colour;
}

Colour getTextureColour(ModelTriangle triangle, float u, float v, float w, TextureMap textureMap) {
	TexturePoint v0Tp = triangle.texturePoints[0];
	TexturePoint v1Tp = triangle.texturePoints[1];
	TexturePoint v2Tp = triangle.texturePoints[2];
	TexturePoint pointTp;
	pointTp.x = w * v0Tp.x + u * v1Tp.x + v * v2Tp.x;
	pointTp.y = w * v0Tp.y + u * v1Tp.y + v * v2Tp.y;
	float pixelOnTextureX = round(pointTp.x * textureMap.width);
	float pixelOnTextureY = round(pointTp.y * textureMap.height);
	int index = pixelOnTextureY * textureMap.width + pixelOnTextureX;
	uint32_t RGBcolour = textureMap.pixels[index];
	Colour colour = getColour(RGBcolour);
	return colour;
}

void draw(DrawingWindow &window) {
	window.clearPixels();
	std::vector<ModelTriangle> modelTriangles = objReader();

	//render wireframe
	if (renderMethod == "wire") {
		//clear depth buffer
		for (int y = 0; y < HEIGHT; y++) {
			for (int x = 0; x < WIDTH; x++) {
				depthBuffer[y][x] = 0;
			}
		}

		for (int i = 0; i < modelTriangles.size(); i++) {
			CanvasTriangle triangle;
			CanvasPoint vertex0 = getCanvasIntersectionPoint(modelTriangles[i].vertices[0]);
			CanvasPoint vertex1 = getCanvasIntersectionPoint(modelTriangles[i].vertices[1]);
			CanvasPoint vertex2 = getCanvasIntersectionPoint(modelTriangles[i].vertices[2]);
			uint32_t white = (255 << 24) + (255 << 16) + (255 << 8) + 255;
			triangle.vertices[0] = vertex0;
			triangle.vertices[1] = vertex1;
			triangle.vertices[2] = vertex2;
			drawStrokedTriangle(window, triangle, white);
		}
	}

	//render using rasterisation
	if (renderMethod == "raster") {
		//clear depth buffer
		for (int y = 0; y < HEIGHT; y++) {
			for (int x = 0; x < WIDTH; x++) {
				depthBuffer[y][x] = 0;
			}
		}

		for (int i = 0; i < modelTriangles.size(); i++) {
			ModelTriangle triangle = modelTriangles[i];
			CanvasTriangle canvasTriangle;
			canvasTriangle.vertices[0] = getCanvasIntersectionPoint(triangle.vertices[0]);
			canvasTriangle.vertices[1] = getCanvasIntersectionPoint(triangle.vertices[1]);
			canvasTriangle.vertices[2] = getCanvasIntersectionPoint(triangle.vertices[2]);

			Colour colour = triangle.colour;
			uint32_t RGBcolour = (255 << 24) + (colour.red << 16) + (colour.green << 8) + colour.blue;

			drawFilledTriangle(window, canvasTriangle, RGBcolour);
			drawStrokedTriangle(window, canvasTriangle, RGBcolour);
		}
		CanvasPoint light;
		light = getCanvasIntersectionPoint(lightSource);
		uint32_t lightColour = (255 << 24) + (255 << 16) + (255 << 8) + 255;
		window.setPixelColour(light.x, light.y, lightColour);
	}
	
	//render using ray tracing
	if (renderMethod == "ray") {
		TextureMap textureMap(textureFile);
		for (int y = 0; y < HEIGHT; y++) {
			for (int x = 0; x < WIDTH; x++) {

				CanvasPoint point;
				point.x = x;
				point.y = y;
				glm::vec3 ray = getRayDirection(point);
				RayTriangleIntersection rayIntersection = getClosestIntersection(cameraPosition, modelTriangles, ray, -1);
				ModelTriangle triangle = rayIntersection.intersectedTriangle;

				//calculating barycentric coords
				float triangleArea = glm::length(glm::cross((triangle.vertices[1] - triangle.vertices[0]), (triangle.vertices[2] - triangle.vertices[0]))) / 2;
				float u = (glm::length(glm::cross((triangle.vertices[0] - triangle.vertices[2]), (rayIntersection.intersectionPoint - triangle.vertices[2]))) / 2) / triangleArea;
				float v = (glm::length(glm::cross((triangle.vertices[1] - triangle.vertices[0]), (rayIntersection.intersectionPoint - triangle.vertices[0]))) / 2) / triangleArea;
				float w = 1 - u - v;

				//proximity lighting
				float distanceToLight = glm::length(lightSource - rayIntersection.intersectionPoint);
				float lightIntensity = 100 / (4.0 * pi * distanceToLight * distanceToLight);
				lightIntensity = clamp(lightIntensity);
				
				//interpolating normals
				glm::vec3 v0normal = triangle.v0Normal;
				glm::vec3 v1normal = triangle.v1Normal;
				glm::vec3 v2normal = triangle.v2Normal;
				glm::vec3 pointNormal = glm::normalize(w * v0normal + u * v1normal + v * v2normal);

				//Interpolated normals AOI lighting
				glm::vec3 lightDirection = glm::normalize(lightSource - rayIntersection.intersectionPoint);
				float AOI = clamp(glm::dot(lightDirection, pointNormal));

				//Interpolated normals specular lighting
				glm::vec3 Ri = glm::normalize(rayIntersection.intersectionPoint - lightSource);
				glm::vec3 view = glm::normalize(cameraPosition - rayIntersection.intersectionPoint);
				glm::vec3 Rr = glm::normalize(Ri - (pointNormal * 2.0f) * glm::dot(Ri, pointNormal));
				float specular = clamp(pow(glm::dot(Rr, view), 256));

				float brightnessModifier = lightIntensity * 0.6 + AOI * 0.2 + specular * 0.2;

				//gouraud AOI
				/*glm::vec3 lightDirection = lightSource - rayIntersection.intersectionPoint;
				lightDirection = glm::normalize(lightDirection);
				glm::vec3 v0normal = glm::normalize(triangle.v0Normal);
				glm::vec3 v1normal = glm::normalize(triangle.v1Normal);
				glm::vec3 v2normal = glm::normalize(triangle.v2Normal);
				float v0AOI = glm::dot(lightDirection, v0normal);
				float v1AOI = glm::dot(lightDirection, v1normal);
				float v2AOI = glm::dot(lightDirection, v2normal);
				float AOI = clamp(w * v0AOI + u * v1AOI + v * v2AOI);*/

				//gouraud specular
				/*
				glm::vec3 Ri = rayIntersection.intersectionPoint - lightSource;
				Ri = glm::normalize(Ri);
				glm::vec3 view = cameraPosition - rayIntersection.intersectionPoint;
				view = glm::normalize(view);
				glm::vec3 v0Rr = glm::normalize(Ri - (triangle.v0Normal * 2.0f) * glm::dot(Ri, triangle.v0Normal));
				glm::vec3 v1Rr = glm::normalize(Ri - (triangle.v1Normal * 2.0f) * glm::dot(Ri, triangle.v1Normal));
				glm::vec3 v2Rr = glm::normalize(Ri - (triangle.v2Normal * 2.0f) * glm::dot(Ri, triangle.v2Normal));
				float v0Specular = pow(glm::dot(v0Rr, view), 256);
				float v1Specular = pow(glm::dot(v1Rr, view), 256);
				float v2Specular = pow(glm::dot(v2Rr, view), 256);
				float specular = clamp(w * v0Specular + u * v1Specular + v * v2Specular);*/

				//AOI
				/*
				glm::vec3 normal = glm::normalize(triangle.normal);
				glm::vec3 lightDirection = lightSource - rayIntersection.intersectionPoint;
				lightDirection = glm::normalize(lightDirection);
				
				float AOI = glm::dot(lightDirection, normal);
				AOI = clamp(AOI);*/
				
				//specular
				/*
				glm::vec3 Ri = rayIntersection.intersectionPoint - lightSource;
				Ri = glm::normalize(Ri);
				glm::vec3 view = cameraPosition - rayIntersection.intersectionPoint;
				view = glm::normalize(view);
				glm::vec3 Rr = Ri - (normal * 2.0f) * glm::dot(Ri, normal);
				Rr = glm::normalize(Rr);
				float specular = pow(glm::dot(Rr, view), 256.0);
				specular = clamp(specular);*/

				//ambient
				if (brightnessModifier < 0.3) {
					brightnessModifier = 0.3;
				}

				//shadows
				ray = lightSource - rayIntersection.intersectionPoint;
				RayTriangleIntersection closestIntersect = getClosestIntersection(rayIntersection.intersectionPoint, modelTriangles, ray, rayIntersection.triangleIndex);
				if (closestIntersect.distanceFromCamera < distanceToLight) {
					brightnessModifier = 0.2;
				}

				//texture mapping
				Colour colour;
				if (triangle.colour.name == textureName && texture) {
					colour = getTextureColour(triangle, u, v, w, textureMap);
				}
				else
				{
					colour = triangle.colour;
				}

				//mirror reflections
				if (triangle.colour.name == "Mirror") {
					glm::vec3 triangleNormal = glm::normalize(triangle.normal);
					Ri = glm::normalize(rayIntersection.intersectionPoint - cameraPosition);
					Rr = glm::normalize(Ri - 2.0f * (glm::dot(Ri, triangleNormal) * triangleNormal));
					RayTriangleIntersection reflectionRay = getClosestIntersection(rayIntersection.intersectionPoint, modelTriangles, Rr, rayIntersection.triangleIndex);
					Colour reflectionColour;
					reflectionColour = reflectionRay.intersectedTriangle.colour;
					if (reflectionColour.name == textureName && texture) {
						colour = getTextureColour(reflectionRay.intersectedTriangle, u, v, w, textureMap);
					}
					else
					{
						colour = reflectionColour;
					}
				}
				colour.red *= brightnessModifier;
				colour.green *= brightnessModifier;
				colour.blue *= brightnessModifier;
				uint32_t RGBcolour = (255 << 24) + (colour.red << 16) + (colour.green << 8) + colour.blue;
				window.setPixelColour(x, y, RGBcolour);
			}
		}
		//light position in scene
		CanvasPoint light;
		light = getCanvasIntersectionPoint(lightSource);
		uint32_t lightColour = (255 << 24) + (255 << 16) + (255 << 8) + 255;
		window.setPixelColour(light.x, light.y, lightColour);
	}
}

void handleEvent(SDL_Event event, DrawingWindow &window) {
	if (event.type == SDL_KEYDOWN) {
		//move light left
		if (event.key.keysym.sym == SDLK_LEFT) {
			lightSource = { lightSource[0] - 0.1, lightSource[1], lightSource[2] };
		}
		//move light right
		else if (event.key.keysym.sym == SDLK_RIGHT) {
			lightSource = { lightSource[0] + 0.1, lightSource[1], lightSource[2] };
		}
		//move light up
		else if (event.key.keysym.sym == SDLK_UP) {
			lightSource = { lightSource[0], lightSource[1] + 0.1, lightSource[2] };
		}
		//move light down
		else if (event.key.keysym.sym == SDLK_DOWN) {
			lightSource = { lightSource[0], lightSource[1] - 0.1, lightSource[2] };
		}
		//move light out
		else if (event.key.keysym.sym == SDLK_SPACE) {
			lightSource = { lightSource[0], lightSource[1], lightSource[2] + 0.1 };
		}
		//move light in
		else if (event.key.keysym.sym == SDLK_LSHIFT) {
			lightSource = { lightSource[0], lightSource[1], lightSource[2] - 0.1 };
		}
		//move camera up
		else if (event.key.keysym.sym == SDLK_w) {
			cameraPosition = { cameraPosition[0] , cameraPosition[1] + 0.1, cameraPosition[2] };
		}
		//move camera down
		else if (event.key.keysym.sym == SDLK_s) {
			cameraPosition = { cameraPosition[0], cameraPosition[1] - 0.1, cameraPosition[2] };
		}
		//move camera left
		else if (event.key.keysym.sym == SDLK_a) {
			cameraPosition = { cameraPosition[0] - 0.1 , cameraPosition[1], cameraPosition[2] };
		}
		//move camera right
		else if (event.key.keysym.sym == SDLK_d) {
			cameraPosition = { cameraPosition[0] + 0.1, cameraPosition[1], cameraPosition[2] };
		}
		//move camera in
		else if (event.key.keysym.sym == SDLK_PERIOD) {
			cameraPosition[2] = cameraPosition[2] - 1;
		}
		//move camera out
		else if (event.key.keysym.sym == SDLK_COMMA) {
			cameraPosition[2] = cameraPosition[2] + 1;
		}
		//rotate + about x-axis
		else if (event.key.keysym.sym == SDLK_RIGHTBRACKET) {
			rotationMatrix = glm::mat3(glm::vec3(1, 0, 0), glm::vec3(0, cos(pi / 64), sin(pi / 64)), glm::vec3(0, -sin(pi / 64), cos(pi / 64)));
			cameraPosition = rotationMatrix * cameraPosition;
			lookAt(glm::vec3(0, 0, 0));
		}
		//rotate - about x-axis
		else if (event.key.keysym.sym == SDLK_LEFTBRACKET) {
			rotationMatrix = glm::mat3(glm::vec3(1, 0, 0), glm::vec3(0, cos(-pi / 64), sin(-pi / 64)), glm::vec3(0, -sin(-pi / 64), cos(-pi / 64)));
			cameraPosition = rotationMatrix * cameraPosition;
			lookAt(glm::vec3(0, 0, 0));
		}
		//rotate - about y-axis
		else if (event.key.keysym.sym == SDLK_o) {
			rotationMatrix = glm::mat3(glm::vec3(cos(pi / 64), 0, -sin(pi / 64)), glm::vec3(0, 1, 0), glm::vec3(sin(pi / 64), 0, cos(pi / 64)));
			cameraPosition = rotationMatrix * cameraPosition;
			lookAt(glm::vec3(0, 0, 0));
		}
		//rotate + about y-axis
		else if (event.key.keysym.sym == SDLK_p) {
			rotationMatrix = glm::mat3(glm::vec3(cos(-pi / 64), 0, -sin(-pi / 64)), glm::vec3(0, 1, 0), glm::vec3(sin(-pi / 64), 0, cos(-pi / 64)));
			cameraPosition = rotationMatrix * cameraPosition;
			lookAt(glm::vec3(0, 0, 0));
		}
		//render using rasterising
		else if (event.key.keysym.sym == SDLK_r) {
			renderMethod = "raster";
			std::cout << "Render mode: Rasterised" << std::endl;
		}
		//render using ray tracing
		else if (event.key.keysym.sym == SDLK_t) {
			renderMethod = "ray";
			std::cout << "Render mode: Ray-traced" << std::endl;
		}
		//render wireframe
		else if (event.key.keysym.sym == SDLK_e) {
			renderMethod = "wire";
			std::cout << "Render mode: Wireframe" << std::endl;
		}
		//]textures on/off
		else if (event.key.keysym.sym == SDLK_y) {
			texture = !texture;
			if (texture) {
				std::cout << "Texturing: On" << std::endl;
			}
			else
			{
				std::cout << "Texturing: Off" << std::endl;
			}
		}
	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		draw(window);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
