#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <assert.h>
#include <functional>
#include <unordered_map>

using namespace std;

class Window
{
public:
	Window(unsigned int width, unsigned int height, const char* title);
	~Window();
	bool shouldClose();
	void pollEvents();
	GLFWwindow& getWindow();
	void setKeyCallback(function<void(Window&, int, int, int, int)> callback);
	void setMouseButtonCallback(function<void(Window&, int, int, int)> callback);
	void setMouseMoveCallback(function<void(Window&, double, double)> callback);

	static void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void onMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void onMouseMoveCallback(GLFWwindow* window, double xpos, double ypos);

private:

	GLFWwindow* m_Window = nullptr;

	struct GLFWCallbacks
	{
		function<void(Window&, int, int, int, int)> m_KeyCallback = nullptr;
		function<void(Window&, int, int, int)>  m_MouseButtonCallback = nullptr;
		function<void(Window&, double, double)> m_MouseMoveCallback = nullptr;
		Window* m_Instance = nullptr;
	};

	static unordered_map< GLFWwindow*, GLFWCallbacks> m_Callbacks;
};


