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
	Window(unsigned int width, unsigned int height, const char* title)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	}

	~Window()
	{
		m_Callbacks.erase(m_Window);
		glfwDestroyWindow(m_Window);
		glfwTerminate();
		m_Window = nullptr;
	}

	bool shouldClose()
	{
		return glfwWindowShouldClose(m_Window);
	}

	void pollEvents()
	{
		glfwPollEvents();
	}

	GLFWwindow& getWindow()
	{
		assert(m_Window != nullptr);
		return *m_Window;
	}

	void setKeyCallback( function<void(Window&, int, int, int, int)> callback)
	{
		m_Callbacks[m_Window].m_Instance = this;

		m_Callbacks[m_Window].m_KeyCallback = callback;
		glfwSetKeyCallback(m_Window, onKeyCallback);
	}

	void setMouseButtonCallback(function<void(Window&, int, int, int)> callback)
	{
		m_Callbacks[m_Window].m_Instance = this;

		m_Callbacks[m_Window].m_MouseButtonCallback = callback;
		glfwSetMouseButtonCallback(m_Window, onMouseButtonCallback);
	}

	void setMouseMoveCallback(function<void(Window&, double, double)> callback)
	{
		m_Callbacks[m_Window].m_Instance = this;

		m_Callbacks[m_Window].m_MouseMoveCallback = callback;
		glfwSetCursorPosCallback(m_Window, onMouseMoveCallback);
	}

	static void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		auto it = m_Callbacks.find(window);
		if (it != end(m_Callbacks) && it->second.m_KeyCallback!=nullptr)
		{
			it->second.m_KeyCallback(*it->second.m_Instance, key, scancode, action, mods);
		}
	}
	
	static void onMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		auto it = m_Callbacks.find(window);
		if (it != end(m_Callbacks) && it->second.m_MouseButtonCallback != nullptr)
		{
			it->second.m_MouseButtonCallback(*it->second.m_Instance, button, action, mods);
		}
	}

	static void onMouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
	{
		auto it = m_Callbacks.find(window);
		if (it != end(m_Callbacks) && it->second.m_MouseMoveCallback != nullptr)
		{
			it->second.m_MouseMoveCallback(*it->second.m_Instance,  xpos, ypos);
		}
	}

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

unordered_map< GLFWwindow*, Window::GLFWCallbacks> Window::m_Callbacks;

