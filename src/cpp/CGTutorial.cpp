#include <iostream>
#include <functional>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "shader.hpp"
#include "objects.hpp"
// kuemmert sich um die Pfade zu den Shadern und Texturen
#include "asset.hpp"
#include "objloader.hpp"
#include "texture.hpp"

glm::vec3 angle{};
glm::vec4 robot_modules{};
glm::vec3 pos{};
uint module{3};
void error_callback(int error, const char *description)
{
	std::cerr << error << '\n';
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	switch (key)
	{
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(window, GL_TRUE);
		break;
	case GLFW_KEY_UP:
		pos.y += 0.1f;
		break;
	case GLFW_KEY_DOWN:
		pos.y -= 0.1f;
		break;
	case GLFW_KEY_LEFT:
		pos.x += 0.1f;
		break;
	case GLFW_KEY_RIGHT:
		pos.x -= 0.1f;
		break;
	case GLFW_KEY_0:
		pos.z += 0.1f;
		break;
	case GLFW_KEY_9:
		pos.z -= 0.1f;
		break;
	case GLFW_KEY_SPACE:
		robot_modules.w += 0.05f;
		break;
	case GLFW_KEY_1:
		angle.x += 0.1f;
		break;
	case GLFW_KEY_2:
		angle.y += 0.1f;
		break;
	case GLFW_KEY_3:
		angle.z += 0.1f;
		break;
	case GLFW_KEY_A:
		module = 1;
		std::cout << "Modul: " << module << '\n';
		break;
	case GLFW_KEY_S:
		module = 2;
		std::cout << "Modul: " << module << '\n';
		break;
	case GLFW_KEY_D:
		module = 3;
		std::cout << "Modul: " << module << '\n';
		break;
	case GLFW_KEY_J:
		if (module == 1)
			robot_modules.x += 0.05f;
		else if (module == 2)
			robot_modules.y += 0.05f;
		else if (module == 3)
			robot_modules.z += 0.05f;
		break;
	default:
		break;
	}
}

glm::mat4 Projection{};
glm::mat4 View{};
glm::mat4 Model{};
GLuint programID{};

void sendMVP()
{
	glm::mat4 MVP{Projection * View * Model};
	glUniformMatrix4fv(glGetUniformLocation(programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(programID, "M"), 1, GL_FALSE, &Model[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(programID, "V"), 1, GL_FALSE, &View[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(programID, "P"), 1, GL_FALSE, &Projection[0][0]);
}

void save_and_restore(const std::function<void()>& callback)
{
	glm::mat4 Save{Model};
	callback();
	Model = Save;
}

void draw_coordinate_system()
{
	auto send_and_draw{[]() -> void {
		sendMVP();
		drawCube();
	}};
	auto draw_axis{[&send_and_draw](const glm::vec3& scale_vector) -> void {
		save_and_restore([&send_and_draw, &scale_vector]() -> void {
			Model = glm::scale(Model, scale_vector);
			send_and_draw();
		});
	}};
	draw_axis(glm::vec3(10.0f, 0.005f, 0.005f));
	draw_axis(glm::vec3(0.005f, 10.0f, 0.005f));
	draw_axis(glm::vec3(0.005f, 0.005f, 10.0f));
}

void draw_robot(float height)
{
	auto draw_module{[height]() -> void {
		save_and_restore([height]() -> void {
			Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, height));
			Model = glm::scale(Model, glm::vec3(0.2f, 0.2f, height));
			sendMVP();
			drawSphere(10, 10);
		});
	}};
	save_and_restore([&draw_module, height]() -> void {
		Model = glm::rotate(Model, robot_modules.z, glm::vec3(1.0f, 0.0f, 0.0f));
		draw_module();
		Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 2 * height));
		Model = glm::rotate(Model, robot_modules.y, glm::vec3(1.0f, 0.0f, 0.0f));
		draw_module();
		Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 2 * height));
		Model = glm::rotate(Model, robot_modules.x, glm::vec3(1.0f, 0.0f, 0.0f));
		draw_module();
		glm::vec4 lightPos{Model * glm::vec4(0.0f, 0.0f, 2 * height, 1.0f)};
		glUniform3f(glGetUniformLocation(programID, "LightPosition_worldspace"), lightPos.x, lightPos.y, lightPos.z);
	});
}

int main(void)
{
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		exit(EXIT_FAILURE);
	}
	glfwSetErrorCallback(error_callback);
	GLFWwindow *window = glfwCreateWindow(1024, 768, "CGTutorial", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);
	glewExperimental = true;
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Failed to initialize GLEW\n";
		return -1;
	}
	glfwSetKeyCallback(window, key_callback);
	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
	programID = LoadShaders(SHADER_DIR "/StandardShading.vertexshader", SHADER_DIR "/StandardShading.fragmentshader");
	glUseProgram(programID);
	GLuint normalbuffer{};
	GLuint vertexbuffer{};
	GLuint uvbuffer{};
	std::vector<glm::vec3> vertices{};
	std::vector<glm::vec2> uvs{};
	std::vector<glm::vec3> normals{};
	loadOBJ(RESOURCES_DIR "/teapot.obj", vertices, uvs, normals);
	GLuint VertexArrayIDTeapot;
	glGenVertexArrays(1, &VertexArrayIDTeapot);
	glBindVertexArray(VertexArrayIDTeapot);
	GLuint Texture = loadBMP_custom(RESOURCES_DIR "/mandrill.bmp");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Texture);
	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
	glUniform1i(glGetUniformLocation(programID, "myTextureSampler"), 0);
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		Projection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
		View = glm::lookAt(glm::vec3(0, 0, -5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		Model = glm::mat4(1.0f);
		Model = glm::translate(Model, glm::vec3(pos.x, 0.0f, 0.0f));
		Model = glm::translate(Model, glm::vec3(0.0f, pos.y, 0.0f));
		Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, pos.z));
		Model = glm::rotate(Model, angle.x, glm::vec3(1.0f, 0.0f, 0.0f));
		Model = glm::rotate(Model, angle.y, glm::vec3(0.0f, 1.0f, 0.0f));
		Model = glm::rotate(Model, angle.z, glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 Save{Model};
		Model = glm::translate(Model, glm::vec3(1.5, 0.0, 0.0));
		Model = glm::scale(Model, glm::vec3(1.0 / 1000.0, 1.0 / 1000.0, 1.0 / 1000.0));
		sendMVP();
		glBindVertexArray(VertexArrayIDTeapot);
		glDrawArrays(GL_TRIANGLES, 0, vertices.size());
		Model = glm::scale(Model, glm::vec3(0.5, 0.5, 0.5));
		sendMVP();
		Model = Save;
		draw_coordinate_system();
		Save = Model;
		Model = glm::rotate(Model, robot_modules.w, glm::vec3(0.0f, 0.0f, 1.0f));
		draw_robot(0.5f);
		Model = Save;
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glDeleteProgram(programID);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glfwTerminate();
	return 0;
}