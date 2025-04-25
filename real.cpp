#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int CURVE_RESOLUTION = 100;
const int CIRCLE_SEGMENTS = 32; // Increased segments for smoother circles
float M_PI = 3.14;

std::vector<GLfloat> controlPoints;
std::vector<GLfloat> curvePoints;

bool dragging = false;
int draggedIndex = -1;

GLFWwindow* window;
GLuint vao[3], vbo[3];
GLuint shaderProgram;

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 position;
void main() {
gl_Position = vec4(position, 0.0, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
uniform vec3 uColor;
out vec4 FragColor;
void main() {
FragColor = vec4(uColor, 1.0);
}
)";

// Compile shader
GLuint compileShader(GLenum type, const char* src) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	// Check for shader compile errors
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cerr << "Shader compilation error: " << infoLog << std::endl;
	}

	return shader;
}

// De Casteljau's algorithm
std::vector<GLfloat> computeBezierCurve(const std::vector<GLfloat>& points) {
	std::vector<GLfloat> curve;
	int n = points.size() / 2 - 1;
	if (n < 1) return curve;

	for (int i = 0; i <= CURVE_RESOLUTION; ++i) {
		float t = i / (float)CURVE_RESOLUTION;
		std::vector<float> temp(points);
		for (int r = 1; r <= n; ++r)
			for (int j = 0; j <= n - r; ++j) {
				temp[j * 2] = (1 - t) * temp[j * 2] + t * temp[(j + 1) * 2];
				temp[j * 2 + 1] = (1 - t) * temp[j * 2 + 1] + t * temp[(j + 1) * 2 + 1];
			}
		curve.push_back(temp[0]);
		curve.push_back(temp[1]);
	}
	return curve;
}

// Coordinate conversion
void screenToGLCoords(double x, double y, float& outX, float& outY) {
	outX = 2.0f * (float)x / WINDOW_WIDTH - 1.0f;
	outY = 1.0f - 2.0f * (float)y / WINDOW_HEIGHT;
}

// Update buffers
void updateBuffers() {
	curvePoints = computeBezierCurve(controlPoints);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * sizeof(float), controlPoints.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * sizeof(float), controlPoints.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, curvePoints.size() * sizeof(float), curvePoints.data(), GL_DYNAMIC_DRAW);
}

// Generate vertices for a perfect circle
std::vector<GLfloat> generateCircleVertices(float centerX, float centerY, float radius) {
	std::vector<GLfloat> vertices;
	float aspect = (float)WINDOW_WIDTH / WINDOW_HEIGHT;

	vertices.reserve(2 * (CIRCLE_SEGMENTS + 2));
	vertices.push_back(centerX);
	vertices.push_back(centerY);

	for (int i = 0; i <= CIRCLE_SEGMENTS; i++) {
		float angle = 2.0f * M_PI * i / CIRCLE_SEGMENTS;
		float x = centerX + radius * cosf(angle) / aspect;
		float y = centerY + radius * sinf(angle);
		vertices.push_back(x);
		vertices.push_back(y);
	}

	return vertices;
}


// Draw a perfect circle at the specified position
void drawCircle(float centerX, float centerY, float radius) {
	std::vector<GLfloat> circleVertices = generateCircleVertices(centerX, centerY, radius);

	GLuint tempVAO, tempVBO;
	glGenVertexArrays(1, &tempVAO);
	glGenBuffers(1, &tempVBO);

	glBindVertexArray(tempVAO);
	glBindBuffer(GL_ARRAY_BUFFER, tempVBO);
	glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float),
		circleVertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);

	glDrawArrays(GL_TRIANGLE_FAN, 0, circleVertices.size() / 2);

	// Cleanup
	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glDeleteBuffers(1, &tempVBO);
	glDeleteVertexArrays(1, &tempVAO);
}



// Get distance between a point and position
float getPointDistance(int pointIndex, float x, float y) {
	float dx = controlPoints[pointIndex * 2] - x;
	float dy = controlPoints[pointIndex * 2 + 1] - y;
	return dx * dx + dy * dy;
}

// Find point under cursor
int findPointUnderCursor(float x, float y, float threshold = 0.03f) {
	for (int i = 0; i < controlPoints.size() / 2; ++i) {
		if (getPointDistance(i, x, y) < threshold * threshold) {
			return i;
		}
	}
	return -1; // No point found
}

// Mouse handling
void mouse_button_callback(GLFWwindow*, int button, int action, int) {
	if (action == GLFW_PRESS) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		float mx, my;
		screenToGLCoords(xpos, ypos, mx, my);

		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			// Check if clicking on an existing point for dragging
			int pointIndex = findPointUnderCursor(mx, my);
			if (pointIndex != -1) {
				dragging = true;
				draggedIndex = pointIndex;
				return;
			}

			// Otherwise add a new point
			controlPoints.push_back(mx);
			controlPoints.push_back(my);
			updateBuffers();
		}
		else if (button == GLFW_MOUSE_BUTTON_RIGHT && !controlPoints.empty()) {
			// Find if we're clicking on a specific point to delete
			int pointIndex = findPointUnderCursor(mx, my);
			if (pointIndex != -1) {
				// Only delete if we still have enough control points (at least 2)
				if (controlPoints.size() > 4) { // Keep at least 2 points (4 floats)
					controlPoints.erase(controlPoints.begin() + pointIndex * 2,
						controlPoints.begin() + pointIndex * 2 + 2);
					updateBuffers();
				}
			}
		}
	}
	else if (action == GLFW_RELEASE) {
		dragging = false;
		draggedIndex = -1;
	}
}

void cursor_position_callback(GLFWwindow*, double xpos, double ypos) {
	if (dragging && draggedIndex != -1) {
		float mx, my;
		screenToGLCoords(xpos, ypos, mx, my);
		controlPoints[draggedIndex * 2] = mx;
		controlPoints[draggedIndex * 2 + 1] = my;
		updateBuffers();
	}
}

int main() {
	// Initialize GLFW
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	// Create window
	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Bezier Curve Editor", nullptr, nullptr);
	if (!window) {
		std::cerr << "Failed to create window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	// Initialize GLEW
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
		return -1;
	}

	// Set callbacks
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);

	// Compile shaders
	GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
	GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

	// Create and link shader program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vs);
	glAttachShader(shaderProgram, fs);
	glLinkProgram(shaderProgram);

	// Check for linking errors
	GLint success;
	GLchar infoLog[512];
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cerr << "Shader program linking error: " << infoLog << std::endl;
	}

	// Delete shaders as they're linked into our program
	glDeleteShader(vs);
	glDeleteShader(fs);

	// Create VAOs and VBOs
	glGenVertexArrays(3, vao);
	glGenBuffers(3, vbo);

	for (int i = 0; i < 3; ++i) {
		glBindVertexArray(vao[i]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[i]);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);
	}

	// Initial control points
	controlPoints = {
		-0.8f, -0.8f,
		-0.4f,  0.9f,
		0.4f, -0.9f,
		0.8f,  0.8f
	};
	updateBuffers();

	// Create a VAO for circle rendering
	GLuint circleVAO;
	glGenVertexArrays(1, &circleVAO);

	// Enable blending for nicer looking circles
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Main loop
	while (!glfwWindowShouldClose(window)) {
		// Clear the screen
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Use shader program
		glUseProgram(shaderProgram);

		// Draw blue lines for control polygon
		glUniform3f(glGetUniformLocation(shaderProgram, "uColor"), 0.0f, 0.0f, 1.0f);
		glBindVertexArray(vao[1]);
		glLineWidth(1.5f);
		glDrawArrays(GL_LINE_STRIP, 0, controlPoints.size() / 2);

		// Draw green curve
		glUniform3f(glGetUniformLocation(shaderProgram, "uColor"), 0.0f, 1.0f, 0.0f);
		glBindVertexArray(vao[2]);
		glLineWidth(2.0f);
		glDrawArrays(GL_LINE_STRIP, 0, curvePoints.size() / 2);

		// Draw red control points as perfect circles
		glUniform3f(glGetUniformLocation(shaderProgram, "uColor"), 1.0f, 0.0f, 0.0f);
		glBindVertexArray(circleVAO);

		for (int i = 0; i < controlPoints.size() / 2; ++i) {
			float x = controlPoints[i * 2];
			float y = controlPoints[i * 2 + 1];
			drawCircle(x, y, 0.015f);
		}

		// Swap buffers and poll events
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Cleanup
	glDeleteVertexArrays(3, vao);
	glDeleteBuffers(3, vbo);
	glDeleteVertexArrays(1, &circleVAO);
	glDeleteProgram(shaderProgram);

	glfwTerminate();
	return 0;
}
