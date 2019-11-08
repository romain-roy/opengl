#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Timer.h"
#include "objloader.hpp"

#include <vector>
#include <iostream>
#include <random>
#include <sstream>
#include <fstream>
#include <string>

#define TINYPLY_IMPLEMENTATION
#include <tinyply.h>

glm::vec3 position = glm::vec3(0.f, 0.f, 5.f);
float pitch = 0.f;
float yaw = 0.f;
float speed = 5.f;
float mouseSensitivity = 0.005;

glm::vec2 lmp;

static void processCameraInput(GLFWwindow* window, float deltaTime) {
	glm::mat4 pitchRotation = glm::rotate(-pitch, glm::vec3(1.f, 0.f, 0.f));
	glm::mat4 yawRotation = glm::rotate(-yaw, glm::vec3(0.f, 1.f, 0.f));

	glm::vec3 forward = yawRotation * pitchRotation * glm::vec4(0.f, 0.f, -1.f, 0.f);
	glm::vec3 right = yawRotation * glm::vec4(1.f, 0.f, 0.f, 0.f);
	glm::vec3 up = glm::vec3(0, 1, 0);
	
	glm::vec3 velocity = glm::vec3();
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) velocity += forward;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) velocity -= forward;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) velocity -= right;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) velocity += right;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) velocity -= up;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) velocity += up;

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // lock cursor in window & hides it
	}

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // unlocks cursor
	}

	double x, y;
	glfwGetCursorPos(window, &x, &y);
	glm::vec2 mp = glm::vec2(x, y);
	if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
		glm::vec2 dmp = mp - lmp;
		pitch += dmp.y * mouseSensitivity;
		yaw += dmp.x * mouseSensitivity;
	}
	lmp = mp;

	//std::cout << "Camera Settings (Pitch : " << pitch << ", Yaw : " << yaw << std::endl;

	if (velocity.x || velocity.y || velocity.z)
		position += glm::normalize(velocity) * speed * deltaTime;
}

static void error_callback(int /*error*/, const char* description)
{
	std::cerr << "Error: " << description << std::endl;
}

static void key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

/* PARTICULES */
struct Particule {
	glm::vec3 position;
	glm::vec3 color;
	glm::vec3 speed;
};

std::vector<Particule> MakeParticules(const int n)
{
	std::default_random_engine generator;
	std::uniform_real_distribution<float> distribution01(0, 1);
	std::uniform_real_distribution<float> distributionWorld(-1, 1);

	std::vector<Particule> p;
	p.reserve(n);

	for (int i = 0; i < n; i++)
	{
		p.push_back(Particule{
				{
				distributionWorld(generator),
				distributionWorld(generator),
				distributionWorld(generator)
				},
				{
				distribution01(generator),
				distribution01(generator),
				distribution01(generator)
				},
				{0.f, 0.f, 0.f}
			});
	}

	return p;
}

GLuint MakeShader(GLuint t, std::string path)
{
	std::cout << path << std::endl;
	std::ifstream file(path.c_str(), std::ios::in);
	std::ostringstream contents;
	contents << file.rdbuf();
	file.close();

	const auto content = contents.str();
	std::cout << content << std::endl;

	const auto s = glCreateShader(t);

	GLint sizes[] = { (GLint)content.size() };
	const auto data = content.data();

	glShaderSource(s, 1, &data, sizes);
	glCompileShader(s);

	GLint success;
	glGetShaderiv(s, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		GLsizei l;
		glGetShaderInfoLog(s, 512, &l, infoLog);

		std::cout << infoLog << std::endl;
	}

	return s;
}

GLuint AttachAndLink(std::vector<GLuint> shaders)
{
	const auto prg = glCreateProgram();
	for (const auto s : shaders)
	{
		glAttachShader(prg, s);
	}

	glLinkProgram(prg);

	GLint success;
	glGetProgramiv(prg, GL_LINK_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		GLsizei l;
		glGetProgramInfoLog(prg, 512, &l, infoLog);

		std::cout << infoLog << std::endl;
	}

	return prg;
}

void APIENTRY opengl_error_callback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	std::cout << message << std::endl;
}

int main(void)
{
	if (!glfwInit())
		exit(EXIT_FAILURE);

	GLFWwindow* window;
	glfwSetErrorCallback(error_callback);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

	window = glfwCreateWindow(640, 480, "OpenGL Gamagora", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window, key_callback);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	// NOTE: OpenGL error checks have been omitted for brevity

	if (!gladLoadGL()) {
		std::cerr << "Something went wrong!" << std::endl;
		exit(-1);
	}

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	loadOBJ("cube.obj", vertices, uvs, normals);

	// Callbacks

	glDebugMessageCallback(opengl_error_callback, nullptr);

	// Shader

	const auto vertex = MakeShader(GL_VERTEX_SHADER, "shader.vert");
	const auto fragment = MakeShader(GL_FRAGMENT_SHADER, "shader.frag");

	const auto program = AttachAndLink({ vertex, fragment });

	glUseProgram(program);

	// Vertex Arrays

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Buffers

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	if (!vertices.empty())
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	if (!uvs.empty())
		glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);


	GLuint normalbuffer;
	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	if (!normals.empty())
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec2), &normals[0], GL_STATIC_DRAW);

	// Bindings

	{
		int location = glGetAttribLocation(program, "position");
		glEnableVertexAttribArray(location);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			location,           // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			sizeof(glm::vec3),  // stride
			(void*)0            // array buffer offset
		);
	}

	{
		int location = glGetAttribLocation(program, "uv");
		glEnableVertexAttribArray(location);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
			location,                         // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			sizeof(glm::vec2),			      // stride
			(void*)0                          // array buffer offset
		);
	}

	{

		int location = glGetAttribLocation(program, "normal");
		glEnableVertexAttribArray(location);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer(
			location,           // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			sizeof(glm::vec3),  // stride
			(void*)0            // array buffer offset
		);
	}
	glm::mat4 scalingMatrix, translateMatrix, rotateMatrix, model, view, projection;
	glm::vec3 cameraTarget;
	GLuint MatrixID;

	float rotateXY = 0.f;
	Timer dt, age;
	while (!glfwWindowShouldClose(window))
	{
		float deltaTime = dt.elapsed(); // in seconds
		dt.reset();
		processCameraInput(window, deltaTime);
		
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		glViewport(0, 0, width, height);

//		rotateXY += 0.01f;
		if (rotateXY >= 360.f) rotateXY -= 360.f;

		scalingMatrix = glm::scale(glm::vec3(1.f, 1.f, 1.f));
		translateMatrix = glm::translate(glm::vec3(0.f, 0.f, 0.f));
		rotateMatrix = glm::rotate(rotateXY, glm::vec3(1.f, 1.f, 0.f));

	
		view = glm::rotate(pitch, glm::vec3(1.f, 0.f, 0.f)) * glm::rotate(yaw, glm::vec3(0.f, 1.f, 0.f)) * glm::translate(-position);

		projection = glm::perspective(glm::radians(45.f), (float)width / (float)height, 0.1f, 100.f);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		for (int i = -1; i <= 1; i++)
		{
			translateMatrix = glm::translate(glm::vec3(3 * i, 0, 0));
			model = translateMatrix * rotateMatrix * scalingMatrix;

			glProgramUniformMatrix4fv(program, glGetUniformLocation(program, "projection"), 1, GL_FALSE, &projection[0][0]);
			glProgramUniformMatrix4fv(program, glGetUniformLocation(program, "view"), 1, GL_FALSE, &view[0][0]);
			glProgramUniformMatrix4fv(program, glGetUniformLocation(program, "model"), 1, GL_FALSE, &model[0][0]);

			//Light on camera
			glProgramUniform3f(program, glGetUniformLocation(program, "light.position"), 5 * std::cos(age.elapsed()), 5, 3 * std::sin(age.elapsed()));
			glProgramUniform3f(program, glGetUniformLocation(program, "light.color"), std::abs(std::sin(age.elapsed())), 0, std::abs(std::cos(age.elapsed())));


			glDrawArrays(GL_TRIANGLES, 0, vertices.size() * 3);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
