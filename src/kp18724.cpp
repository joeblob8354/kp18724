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
glm::vec3 cameraPosition(0.0, 0.0, 10.0);
float scale = 45;
float focalPoint = 4;
float depthBuffer[HEIGHT][WIDTH];
glm::mat3 rotationMatrix(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
glm::mat3 cameraOrientation(glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));

std::vector<Colour> mtlReader() {
	std::vector<std::string> colourNames;
	std::vector<Colour> palette;

	std::ifstream file("cornell-box.mtl");
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
	}

	for (int i = 0; i < palette.size(); i++) {
		palette[i].name = colourNames[i];
	}
	return palette;
}

std::vector<glm::vec3> vertexTranslator(std::vector<unsigned int> faceVertices, std::vector<glm::vec3> vertices) {
	std::vector<glm::vec3> outputVertices;
	//loop through each vertex of each triangle (f lines)
	if (faceVertices.size() != 0) {
		for (unsigned int i = 0; i < faceVertices.size(); i++) {
			//get position of the vertex since obj indexes from 1
			glm::vec3 vertex = vertices[faceVertices[i] - 1];
			outputVertices.push_back(vertex);
		}
	}
	return outputVertices;
}

std::vector<ModelTriangle> modelTriangleMaker(std::vector<glm::vec3> vertices, std::string colour, std::vector<Colour> palette) {
	std::vector<ModelTriangle> modelTriangles;
	Colour white;
	white.red = 255;
	white.green = 255;
	white.blue = 255;
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
			if (myTriangle.colour.name != colour) {
				myTriangle.colour = white;
			}
			modelTriangles.push_back(myTriangle);
		}
	}
	return modelTriangles;
}

std::vector<ModelTriangle> objReader() {
	std::string objectName;
	std::string objectColour;
	std::vector<Colour> palette = mtlReader();
	std::vector<glm::vec3> vertices;
	std::vector<unsigned int> faceVertices;
	std::vector<ModelTriangle> finalTriangles;

	std::ifstream file("cornell-box.obj");
	std::string line;
	for (line; std::getline(file, line);) {
		std::vector<std::string> tokens = split(line, ' ');
		if (tokens[0] == "o") {
			std::vector<glm::vec3> objectVertices = vertexTranslator(faceVertices, vertices);
			std::vector<ModelTriangle> objectTriangles = modelTriangleMaker(objectVertices, objectColour, palette);
			for (int i = 0; i < objectTriangles.size(); i++) {
				finalTriangles.push_back(objectTriangles[i]);
			}
			faceVertices.clear();
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
		else if (tokens[0] == "f") {
			glm::vec3 vertices;
			float vertex;
			for (int i = 1; i < tokens.size(); i++) {
				std::vector<std::string> tokenVertices = split(tokens[i], '/');
				vertex = stof(tokenVertices[0]);
				vertices[i - 1] = vertex;
				faceVertices.push_back(vertices[i - 1]);
			}
		}
	}
	std::vector<glm::vec3> objectVertices = vertexTranslator(faceVertices, vertices);
	std::vector<ModelTriangle> objectTriangles = modelTriangleMaker(objectVertices, objectColour, palette);
	for (int i = 0; i < objectTriangles.size(); i++) {
		finalTriangles.push_back(objectTriangles[i]);
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
	projectedVertex.x = -(focalPoint * newVertex.x / newVertex.z) * scale;
	projectedVertex.y = (focalPoint * newVertex.y / newVertex.z) * scale;
	projectedVertex.x = projectedVertex.x + (WIDTH / 2);
	projectedVertex.y = projectedVertex.y + (HEIGHT / 2);
	projectedVertex.depth = newVertex.z;
	return projectedVertex;
}

bool sortByY(CanvasPoint& lhs, CanvasPoint& rhs) {
	return lhs.y < rhs.y;
}

void drawFilledTriangle(DrawingWindow& window, CanvasTriangle triangle, Colour colour) {
	//get RGB colour for triangle
	uint32_t finalColour = (255 << 24) + (colour.red << 16) + (colour.green << 8) + colour.blue;

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

		drawLine(window, point1, point2, finalColour);
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

		drawLine(window, point1, point2, finalColour);
	}
}

void drawStrokedTriangle(DrawingWindow& window, CanvasTriangle triangle, Colour colour) {
	//get RGB colour for triangle
	uint32_t finalColour = (255 << 24) + (colour.red << 16) + (colour.green << 8) + colour.blue;
	drawLine(window, triangle.vertices[0], triangle.vertices[1], finalColour);
	drawLine(window, triangle.vertices[1], triangle.vertices[2], finalColour);
	drawLine(window, triangle.vertices[2], triangle.vertices[0], finalColour);
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

void draw(DrawingWindow &window) {
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			depthBuffer[y][x] = 0;
		}
	}
	window.clearPixels();
	std::vector<ModelTriangle> modelTriangles = objReader();
	for (int i = 0; i < modelTriangles.size(); i++) {
		CanvasTriangle triangle;
		CanvasPoint vertex0 = getCanvasIntersectionPoint(modelTriangles[i].vertices[0]);
		CanvasPoint vertex1 = getCanvasIntersectionPoint(modelTriangles[i].vertices[1]);
		CanvasPoint vertex2 = getCanvasIntersectionPoint(modelTriangles[i].vertices[2]);
		Colour colour = modelTriangles[i].colour;
		if (colour.name == "") {
			colour.red = 255; //rand() % 256;
			colour.green = 255; //rand() % 256;
			colour.blue = 255; //rand() % 256;
		}
		triangle.vertices[0] = vertex0;
		triangle.vertices[1] = vertex1;
		triangle.vertices[2] = vertex2;
		//drawStrokedTriangle(window, triangle, colour);
		drawFilledTriangle(window, triangle, colour);
		drawStrokedTriangle(window, triangle, colour);
	}
}

void handleEvent(SDL_Event event, DrawingWindow &window) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) std::cout << "LEFT" << std::endl;
		else if (event.key.keysym.sym == SDLK_RIGHT) std::cout << "RIGHT" << std::endl;
		else if (event.key.keysym.sym == SDLK_UP) std::cout << "UP" << std::endl;
		else if (event.key.keysym.sym == SDLK_DOWN) std::cout << "DOWN" << std::endl;
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
