#include "ModelApp.h"

#pragma comment(lib, "vulkan-1.lib")

const int WindowWidth = 640, WindowHeight = 480;
const char* AppTile = "ClearScreen";

int _stdcall wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, 0);
	GLFWwindow* window = glfwCreateWindow(WindowWidth, WindowHeight, AppTile, nullptr, nullptr);

	ModelApp theApp;
	theApp.intialize(window, AppTile);

	while (glfwWindowShouldClose(window) == GLFW_FALSE)
	{
		glfwPollEvents();
		theApp.render();
	}

	theApp.terminate();
	glfwTerminate();
}

