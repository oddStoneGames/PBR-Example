#pragma region Includes

#include <iostream>
#include <random>

#include "../../vendor/glad/include/glad.h"
#include "../../vendor/glfw/include/GLFW/glfw3.h"
#include "../../vendor/glm/glm.hpp"
#include "../../vendor/glm/gtc/matrix_transform.hpp"
#include "../../vendor/glm/gtc/type_ptr.hpp"

#include "../../vendor/imgui/imgui.h"
#include "../../vendor/imgui/imgui_impl_glfw.h"
#include "../../vendor/imgui/imgui_impl_opengl3.h"

#include <stb_image.h>

#include "Model.h"
#include "Shader.h"
#include "Camera.h"

using namespace std;
using namespace glm;

#pragma endregion

#pragma region Variables

///<summary>Screen Width in Screen Coordinates.</summary>
unsigned const int SCR_WIDTH = 1280;
///<summary>Screen Height in Screen Coordinates.</summary>
unsigned const int SCR_HEIGHT = 720;

///<summary>Shadow Width in Screen Coordinates.</summary>
unsigned const int SHADOW_WIDTH  = 1024;
///<summary>Shadow Height in Screen Coordinates.</summary>
unsigned const int SHADOW_HEIGHT = 1024;

///<summary>Number of Samples For Multisampling.</summary>
unsigned const int SAMPLES = 4;

///<summary>Camera Near Plane Distance.</summary>
const float CAM_NEAR_DIST = 0.01f;
///<summary>Camera Far Plane Distance.</summary>
const float CAM_FAR_DIST = 100.0f;

///<summary>Buffer width of the window incase the Screen Width is not in Screen Coordinates.</summary>
int bufferWidth;
///<summary>Buffer height of the window incase the Screen Height is not in Screen Coordinates.</summary>
int bufferHeight;

///<summary>Tells Us If We Are in Full Screen Mode Or Not.</summary>
bool fullScreen = false;
///<summary>True If Full Screen State Changed.</summary>
bool fullScreenDirty = false;
///<summary>Time To Wait Before Switching Full Screen To Windowed To Avoid Spamming.</summary>
float fullScreenSwitchWaitTime = 0.5f;
///<summary>Time Passed Since Switched.</summary>
float timeSinceFullScreenStateSwitch = 0.0f;

///<summary>Primary Monitor.</summary>
GLFWmonitor* primaryMonitor;
///<summary>Primary Monitor's Width, Height, RGB Channel Depth & Refresh Rate.</summary>
const GLFWvidmode* videoMode;

///<summary>Fly Camera</summary>
Camera camera(vec3(0.0f, 1.0f, 2.0f));

///<summary>The Time In Seconds Since We Held An Camera Movement Input Key.</summary>
float timeSinceCameraMovementInput = 0.0f;
///<summary>Last Mouse X Position.</summary>
float lastX = SCR_WIDTH / 2.0f;
///<summary>Last Mouse Y Position.</summary>
float lastY = SCR_HEIGHT / 2.0f;
///<summary>Is This Our First Mouse Input?</summary>
bool firstMouse = true;
/// <summary>True if Camera Zoom Changed This Frame.</summary>
bool camZoomDirty = false;

///<summary>The Time Elapsed Since Last Frame Was Rendered.</summary>
float deltaTime = 0.0f;
///<summary>The Time At Which Last Frame Was Rendered.</summary>
float lastFrame = 0.0f;

//RenderQuad() VAO & VBO.
unsigned int quadVAO = 0;
unsigned int quadVBO;
//RenderCube() VAO & VBO.
unsigned int cubeVAO = 0;
unsigned int cubeVBO;

//HDR Render Buffer
unsigned int renderFBO;
unsigned int finalColorBufferTexture[2];
unsigned int finalRBO;

//Geometry Buffer
unsigned int gBuffer;
unsigned int gPosition, gNormal, gAlbedo, gEmission, gMetallicRoughness;
unsigned int gDepth;

//PBR Image Based Lighting
bool pbrInitialized = false;	//True if PBR has been Initialized atleast once.
unsigned int envCubemap;		//Enivornment Cubemap Generated From Equirectangular Map(HDR Map).
unsigned int irradianceMap;		//Irradiance Cubemap
unsigned int prefilterMap;		//Prefilter Cubemap
unsigned int brdfLUTTexture;	//2D LUT Generated from the BRDF equations.
unsigned int captureFBO;		//PBR FBO
unsigned int captureRBO;		//PBR RBO

//Screen Space Ambient Occlusion Buffer.
unsigned int ssaoFBO, ssaoBlurFBO;
unsigned int ssaoGrayscaleBuffer, ssaoGrayscaleBlurBuffer;

//Bloom Ping-Pong Buffers
unsigned int bloomFBO[2];
unsigned int bloomTexture[2];

#pragma endregion

#pragma region Prototypes

unsigned int LoadTexture(char const* path, bool sRGB = false);
unsigned int LoadHDRTexture(char const* path);
unsigned int LoadCubemap(vector<string> path);
void Window_Resize_Callback(GLFWwindow* window, int width, int height);
void ProcessMouseInput(GLFWwindow* window, double xpos, double ypos);
void ProcessScrollInput(GLFWwindow* window, double xoffset, double yoffset);
void ProcessInput(GLFWwindow* window);
void UpdateAllFramebuffersSize(int bufferWidth, int bufferHeight);
void RenderQuad();
void RenderCube();
void SetupPBR(unsigned int hdrTexture);

#pragma endregion

#pragma region Functions

#pragma region Main

int main()
{

	#pragma region Initialization

	//Initialize GLFW
	glfwInit();

	//Set GLFW Parameters
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//Cache Primary Monitor
	primaryMonitor = glfwGetPrimaryMonitor();

	//Store Monitor Info In Video Mode.
	videoMode = glfwGetVideoMode(primaryMonitor);

	//Set Bit Depths & Refresh Rate From Out Current Monitor's Info.
	glfwWindowHint(GLFW_RED_BITS, videoMode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, videoMode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, videoMode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, videoMode->refreshRate);

	//Create OpenGL Window
	GLFWwindow* window = glfwCreateWindow(fullScreen ? videoMode->width : SCR_WIDTH,
		fullScreen ? videoMode->height : SCR_HEIGHT,
		"PBR-Example", fullScreen ? primaryMonitor : NULL, NULL);

	// Window NullCheck
	if (!window)
	{
		cout << "Failed To Create OpenGL Window!";
		glfwTerminate();
		return -1;
	}

	//Make This Window The Current Context Of OpenGL
	glfwMakeContextCurrent(window);

	//Initialize GLAD
	//Because We Can Call gl Functions Only After GLAD is Initialized!
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		// GLAD Failed Initialization.
		cout << "Failed To Initialize GLAD!";
		glfwTerminate();
		return -1;
	}

	// Enable Depth Testing & Face Culling.
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// Enable seamless cubemap sampling for lower mip levels in the pre-filter map.
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	//Set Clear Color For Background Color.
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	//Get The FrameBufferSize
	glfwGetFramebufferSize(window, &bufferWidth, &bufferHeight);

	//Set The FrameBufferResizeCallback
	glfwSetFramebufferSizeCallback(window, Window_Resize_Callback);
	glfwSetCursorPosCallback(window, ProcessMouseInput);
	glfwSetScrollCallback(window, ProcessScrollInput);

	//Call glViewport To Set Viewport Transform Or The General Area where OpenGL will Render!
	glViewport(0, 0, bufferWidth, bufferHeight);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	const char* glsl_version = "#version 420";
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	#pragma region Vertices Data

	#pragma region Cube Vertices Data with manual tangent and bitangent calculation

	vector<float> cubeVerticesData;

	#pragma region Back Face

	vec3 pos1(0.5f, -0.5f, -0.5f);
	vec3 pos2(-0.5f, -0.5f, -0.5f);
	vec3 pos3(-0.5f, 0.5f, -0.5f);
	vec3 pos4(0.5f, 0.5f, -0.5f);

	vec2 uv1(0.0f, 0.0f);
	vec2 uv2(1.0f, 0.0f);
	vec2 uv3(1.0f, 1.0f);
	vec2 uv4(0.0f, 1.0f);

	vec3 nm(0.0f, 0.0f, -1.0f);	//Normal Vector

	// calculate tangent/bitangent vectors of both triangles
	glm::vec3 tangent1, bitangent1;
	glm::vec3 tangent2, bitangent2;
	// triangle 1
	// ----------
	glm::vec3 edge1 = pos2 - pos1;
	glm::vec3 edge2 = pos3 - pos1;
	glm::vec2 deltaUV1 = uv2 - uv1;
	glm::vec2 deltaUV2 = uv3 - uv1;

	float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	tangent1 = glm::normalize(tangent1);

	bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	bitangent1 = glm::normalize(bitangent1);

	// triangle 2
	// ----------
	edge1 = pos4 - pos3;
	edge2 = pos1 - pos3;
	deltaUV1 = uv4 - uv3;
	deltaUV2 = uv1 - uv3;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	tangent2 = glm::normalize(tangent2);

	bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	bitangent2 = glm::normalize(bitangent2);

	//Append it to cubeVerticesData Vector.
	cubeVerticesData.push_back(pos1.x); cubeVerticesData.push_back(pos1.y); cubeVerticesData.push_back(pos1.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv1.x); cubeVerticesData.push_back(uv1.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos2.x); cubeVerticesData.push_back(pos2.y); cubeVerticesData.push_back(pos2.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv2.x); cubeVerticesData.push_back(uv2.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos3.x); cubeVerticesData.push_back(pos3.y); cubeVerticesData.push_back(pos3.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv3.x); cubeVerticesData.push_back(uv3.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos3.x); cubeVerticesData.push_back(pos3.y); cubeVerticesData.push_back(pos3.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv3.x); cubeVerticesData.push_back(uv3.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos4.x); cubeVerticesData.push_back(pos4.y); cubeVerticesData.push_back(pos4.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv4.x); cubeVerticesData.push_back(uv4.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos1.x); cubeVerticesData.push_back(pos1.y); cubeVerticesData.push_back(pos1.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv1.x); cubeVerticesData.push_back(uv1.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	#pragma endregion

	#pragma region Front Face

	pos1 = vec3(-0.5f, -0.5f, 0.5f);
	pos2 = vec3(0.5f, -0.5f, 0.5f);
	pos3 = vec3(0.5f, 0.5f, 0.5f);
	pos4 = vec3(-0.5f, 0.5f, 0.5f);

	uv1 = vec2(0.0f, 0.0f);
	uv2 = vec2(1.0f, 0.0f);
	uv3 = vec2(1.0f, 1.0f);
	uv4 = vec2(0.0f, 1.0f);

	nm = vec3(0.0f, 0.0f, 1.0f);	//Normal Vector

	// calculate tangent/bitangent vectors of both triangles
	// triangle 1
	// ----------
	edge1 = pos2 - pos1;
	edge2 = pos3 - pos1;
	deltaUV1 = uv2 - uv1;
	deltaUV2 = uv3 - uv1;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	tangent1 = glm::normalize(tangent1);

	bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	bitangent1 = glm::normalize(bitangent1);

	// triangle 2
	// ----------
	edge1 = pos4 - pos3;
	edge2 = pos1 - pos3;
	deltaUV1 = uv4 - uv3;
	deltaUV2 = uv1 - uv3;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	tangent2 = glm::normalize(tangent2);

	bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	bitangent2 = glm::normalize(bitangent2);

	//Append it to cubeVerticesData Vector.
	cubeVerticesData.push_back(pos1.x); cubeVerticesData.push_back(pos1.y); cubeVerticesData.push_back(pos1.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv1.x); cubeVerticesData.push_back(uv1.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos2.x); cubeVerticesData.push_back(pos2.y); cubeVerticesData.push_back(pos2.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv2.x); cubeVerticesData.push_back(uv2.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos3.x); cubeVerticesData.push_back(pos3.y); cubeVerticesData.push_back(pos3.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv3.x); cubeVerticesData.push_back(uv3.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos3.x); cubeVerticesData.push_back(pos3.y); cubeVerticesData.push_back(pos3.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv3.x); cubeVerticesData.push_back(uv3.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos4.x); cubeVerticesData.push_back(pos4.y); cubeVerticesData.push_back(pos4.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv4.x); cubeVerticesData.push_back(uv4.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos1.x); cubeVerticesData.push_back(pos1.y); cubeVerticesData.push_back(pos1.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv1.x); cubeVerticesData.push_back(uv1.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	#pragma endregion
	
	#pragma region Left Face

	pos1 = vec3(-0.5f, -0.5f, -0.5f);
	pos2 = vec3(-0.5f, -0.5f, 0.5f);
	pos3 = vec3(-0.5f, 0.5f, 0.5f);
	pos4 = vec3(-0.5f, 0.5f, -0.5f);

	uv1 = vec2(0.0f, 0.0f);
	uv2 = vec2(1.0f, 0.0f);
	uv3 = vec2(1.0f, 1.0f);
	uv4 = vec2(0.0f, 1.0f);

	nm = vec3(-1.0f, 0.0f, 0.0f);	//Normal Vector

	// calculate tangent/bitangent vectors of both triangles
	// triangle 1
	// ----------
	edge1 = pos2 - pos1;
	edge2 = pos3 - pos1;
	deltaUV1 = uv2 - uv1;
	deltaUV2 = uv3 - uv1;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	tangent1 = glm::normalize(tangent1);

	bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	bitangent1 = glm::normalize(bitangent1);

	// triangle 2
	// ----------
	edge1 = pos4 - pos3;
	edge2 = pos1 - pos3;
	deltaUV1 = uv4 - uv3;
	deltaUV2 = uv1 - uv3;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	tangent2 = glm::normalize(tangent2);

	bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	bitangent2 = glm::normalize(bitangent2);

	//Append it to cubeVerticesData Vector.
	cubeVerticesData.push_back(pos1.x); cubeVerticesData.push_back(pos1.y); cubeVerticesData.push_back(pos1.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv1.x); cubeVerticesData.push_back(uv1.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos2.x); cubeVerticesData.push_back(pos2.y); cubeVerticesData.push_back(pos2.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv2.x); cubeVerticesData.push_back(uv2.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos3.x); cubeVerticesData.push_back(pos3.y); cubeVerticesData.push_back(pos3.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv3.x); cubeVerticesData.push_back(uv3.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos3.x); cubeVerticesData.push_back(pos3.y); cubeVerticesData.push_back(pos3.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv3.x); cubeVerticesData.push_back(uv3.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos4.x); cubeVerticesData.push_back(pos4.y); cubeVerticesData.push_back(pos4.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv4.x); cubeVerticesData.push_back(uv4.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos1.x); cubeVerticesData.push_back(pos1.y); cubeVerticesData.push_back(pos1.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv1.x); cubeVerticesData.push_back(uv1.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	#pragma endregion

	#pragma region Right Face

	pos1 = vec3(0.5f, -0.5f, 0.5f);
	pos2 = vec3(0.5f, -0.5f, -0.5f);
	pos3 = vec3(0.5f, 0.5f, -0.5f);
	pos4 = vec3(0.5f, 0.5f, 0.5f);

	uv1 = vec2(0.0f, 0.0f);
	uv2 = vec2(1.0f, 0.0f);
	uv3 = vec2(1.0f, 1.0f);
	uv4 = vec2(0.0f, 1.0f);

	nm = vec3(1.0f, 0.0f, 0.0f);	//Normal Vector

	// calculate tangent/bitangent vectors of both triangles
	// triangle 1
	// ----------
	edge1 = pos2 - pos1;
	edge2 = pos3 - pos1;
	deltaUV1 = uv2 - uv1;
	deltaUV2 = uv3 - uv1;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	tangent1 = glm::normalize(tangent1);

	bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	bitangent1 = glm::normalize(bitangent1);

	// triangle 2
	// ----------
	edge1 = pos4 - pos3;
	edge2 = pos1 - pos3;
	deltaUV1 = uv4 - uv3;
	deltaUV2 = uv1 - uv3;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	tangent2 = glm::normalize(tangent2);

	bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	bitangent2 = glm::normalize(bitangent2);

	//Append it to cubeVerticesData Vector.
	cubeVerticesData.push_back(pos1.x); cubeVerticesData.push_back(pos1.y); cubeVerticesData.push_back(pos1.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv1.x); cubeVerticesData.push_back(uv1.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos2.x); cubeVerticesData.push_back(pos2.y); cubeVerticesData.push_back(pos2.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv2.x); cubeVerticesData.push_back(uv2.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos3.x); cubeVerticesData.push_back(pos3.y); cubeVerticesData.push_back(pos3.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv3.x); cubeVerticesData.push_back(uv3.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos3.x); cubeVerticesData.push_back(pos3.y); cubeVerticesData.push_back(pos3.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv3.x); cubeVerticesData.push_back(uv3.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos4.x); cubeVerticesData.push_back(pos4.y); cubeVerticesData.push_back(pos4.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv4.x); cubeVerticesData.push_back(uv4.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos1.x); cubeVerticesData.push_back(pos1.y); cubeVerticesData.push_back(pos1.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv1.x); cubeVerticesData.push_back(uv1.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	#pragma endregion

	#pragma region Bottom Face

	pos1 = vec3(-0.5f, -0.5f, -0.5f);
	pos2 = vec3(0.5f, -0.5f, -0.5f);
	pos3 = vec3(0.5f, -0.5f, 0.5f);
	pos4 = vec3(-0.5f, -0.5f, 0.5f);

	uv1 = vec2(0.0f, 0.0f);
	uv2 = vec2(1.0f, 0.0f);
	uv3 = vec2(1.0f, 1.0f);
	uv4 = vec2(0.0f, 1.0f);

	nm = vec3(0.0f, -1.0f, 0.0f);	//Normal Vector

	// calculate tangent/bitangent vectors of both triangles
	// triangle 1
	// ----------
	edge1 = pos2 - pos1;
	edge2 = pos3 - pos1;
	deltaUV1 = uv2 - uv1;
	deltaUV2 = uv3 - uv1;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	tangent1 = glm::normalize(tangent1);

	bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	bitangent1 = glm::normalize(bitangent1);

	// triangle 2
	// ----------
	edge1 = pos4 - pos3;
	edge2 = pos1 - pos3;
	deltaUV1 = uv4 - uv3;
	deltaUV2 = uv1 - uv3;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	tangent2 = glm::normalize(tangent2);

	bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	bitangent2 = glm::normalize(bitangent2);

	//Append it to cubeVerticesData Vector.
	cubeVerticesData.push_back(pos1.x); cubeVerticesData.push_back(pos1.y); cubeVerticesData.push_back(pos1.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv1.x); cubeVerticesData.push_back(uv1.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos2.x); cubeVerticesData.push_back(pos2.y); cubeVerticesData.push_back(pos2.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv2.x); cubeVerticesData.push_back(uv2.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos3.x); cubeVerticesData.push_back(pos3.y); cubeVerticesData.push_back(pos3.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv3.x); cubeVerticesData.push_back(uv3.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos3.x); cubeVerticesData.push_back(pos3.y); cubeVerticesData.push_back(pos3.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv3.x); cubeVerticesData.push_back(uv3.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos4.x); cubeVerticesData.push_back(pos4.y); cubeVerticesData.push_back(pos4.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv4.x); cubeVerticesData.push_back(uv4.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos1.x); cubeVerticesData.push_back(pos1.y); cubeVerticesData.push_back(pos1.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv1.x); cubeVerticesData.push_back(uv1.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	#pragma endregion

	#pragma region Top Face

	pos1 = vec3(-0.5f, 0.5f, 0.5f);
	pos2 = vec3(0.5f, 0.5f, 0.5f);
	pos3 = vec3(0.5f, 0.5f, -0.5f);
	pos4 = vec3(-0.5f, 0.5f, -0.5f);

	uv1 = vec2(0.0f, 0.0f);
	uv2 = vec2(1.0f, 0.0f);
	uv3 = vec2(1.0f, 1.0f);
	uv4 = vec2(0.0f, 1.0f);

	nm = vec3(0.0f, 1.0f, 0.0f);	//Normal Vector

	// calculate tangent/bitangent vectors of both triangles
	// triangle 1
	// ----------
	edge1 = pos2 - pos1;
	edge2 = pos3 - pos1;
	deltaUV1 = uv2 - uv1;
	deltaUV2 = uv3 - uv1;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	tangent1 = glm::normalize(tangent1);

	bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	bitangent1 = glm::normalize(bitangent1);

	// triangle 2
	// ----------
	edge1 = pos4 - pos3;
	edge2 = pos1 - pos3;
	deltaUV1 = uv4 - uv3;
	deltaUV2 = uv1 - uv3;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	tangent2 = glm::normalize(tangent2);

	bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	bitangent2 = glm::normalize(bitangent2);

	//Append it to cubeVerticesData Vector.
	cubeVerticesData.push_back(pos1.x); cubeVerticesData.push_back(pos1.y); cubeVerticesData.push_back(pos1.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv1.x); cubeVerticesData.push_back(uv1.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos2.x); cubeVerticesData.push_back(pos2.y); cubeVerticesData.push_back(pos2.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv2.x); cubeVerticesData.push_back(uv2.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos3.x); cubeVerticesData.push_back(pos3.y); cubeVerticesData.push_back(pos3.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv3.x); cubeVerticesData.push_back(uv3.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos3.x); cubeVerticesData.push_back(pos3.y); cubeVerticesData.push_back(pos3.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv3.x); cubeVerticesData.push_back(uv3.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos4.x); cubeVerticesData.push_back(pos4.y); cubeVerticesData.push_back(pos4.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv4.x); cubeVerticesData.push_back(uv4.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	cubeVerticesData.push_back(pos1.x); cubeVerticesData.push_back(pos1.y); cubeVerticesData.push_back(pos1.z); cubeVerticesData.push_back(nm.x); cubeVerticesData.push_back(nm.y); cubeVerticesData.push_back(nm.z);
	cubeVerticesData.push_back(uv1.x); cubeVerticesData.push_back(uv1.y);
	cubeVerticesData.push_back(tangent1.x); cubeVerticesData.push_back(tangent1.y); cubeVerticesData.push_back(tangent1.z);
	cubeVerticesData.push_back(bitangent1.x); cubeVerticesData.push_back(bitangent1.y); cubeVerticesData.push_back(bitangent1.z);

	#pragma endregion

	#pragma endregion

	// xD
	float originVertices[] = { 0.0f, 0.0f, 0.0f };

	#pragma endregion

	#pragma region Shadow Framebuffers

	// Generate Shadow Cubemaps For both Point Lights.
	unsigned int shadowCubemap[2];
	glGenTextures(2, shadowCubemap);
	for (unsigned int i = 0; i < 2; ++i)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubemap[i]);
		for (unsigned int j = 0; j < 6; ++j)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}
	// Unbind to make sure we don't make any changes by Mistake.
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	// Generate Shadow FBO.
	unsigned int shadowFBO[2];
	glGenFramebuffers(2, shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO[0]);
	// attach first cubemap.
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowCubemap[0], 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Shadow 1 Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO[1]);
	// attach second cubemap.
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowCubemap[1], 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Shadow 2 Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	#pragma endregion

	#pragma region Geometry Framebuffer

	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

	// position color buffer
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

	// normal color buffer
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

	// Albedo color buffer
	glGenTextures(1, &gAlbedo);
	glBindTexture(GL_TEXTURE_2D, gAlbedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);

	// Emission color buffer
	glGenTextures(1, &gEmission);
	glBindTexture(GL_TEXTURE_2D, gEmission);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gEmission, 0);

	// Metallic Roughness color buffer
	glGenTextures(1, &gMetallicRoughness);
	glBindTexture(GL_TEXTURE_2D, gMetallicRoughness);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, bufferWidth, bufferHeight, 0, GL_RG, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, gMetallicRoughness, 0);

	unsigned int colorAttachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
	glDrawBuffers(5, colorAttachments);

	glGenRenderbuffers(1, &gDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, gDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, bufferWidth, bufferHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gDepth);
	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "Geometry buffer not complete!" << endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	#pragma endregion

	#pragma region SSAO Framebuffer

	glGenFramebuffers(1, &ssaoFBO);  
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	// SSAO Grayscale Buffer
	glGenTextures(1, &ssaoGrayscaleBuffer);
	glBindTexture(GL_TEXTURE_2D, ssaoGrayscaleBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, bufferWidth, bufferHeight, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoGrayscaleBuffer, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "SSAO Framebuffer not complete!" << endl;
	
	// SSAO Grayscale Blur Buffer
	glGenFramebuffers(1, &ssaoBlurFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
	glGenTextures(1, &ssaoGrayscaleBlurBuffer);
	glBindTexture(GL_TEXTURE_2D, ssaoGrayscaleBlurBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, bufferWidth, bufferHeight, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoGrayscaleBlurBuffer, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "SSAO Blur Framebuffer not complete!" << endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	#pragma endregion

	#pragma region Bloom Framebuffer

	//Generate Bloom FBO.
	glGenFramebuffers(2, bloomFBO);
	glGenTextures(2, bloomTexture);
	for (unsigned int i = 0; i < 2; ++i)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO[i]);
		glBindTexture(GL_TEXTURE_2D, bloomTexture[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bufferWidth, bufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomTexture[i], 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Bloom " + to_string(i) + " Framebuffer not complete!" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	#pragma endregion

	#pragma region Final Render HDR Framebuffer

	glGenFramebuffers(1, &renderFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, renderFBO);
	// generate texture		
	glGenTextures(2, finalColorBufferTexture);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindTexture(GL_TEXTURE_2D, finalColorBufferTexture[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bufferWidth, bufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// attach it to currently bound framebuffer object
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, finalColorBufferTexture[i], 0);
	}
	// Tell openGL to render to both color attachments.
	unsigned int drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, drawBuffers);

	glGenRenderbuffers(1, &finalRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, finalRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, bufferWidth, bufferHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, finalRBO);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Render Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	#pragma endregion

	#pragma region VAOs

	unsigned int VAO[2], VBO[2];
	glGenVertexArrays(2, VAO);
	glGenBuffers(2, VBO);
	
	//Cube VAO & VBO.
	glBindVertexArray(VAO[0]);
	glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, cubeVerticesData.size() * sizeof(float), &cubeVerticesData[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//Origin VAO & VBO.
	glBindVertexArray(VAO[1]);
	glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(originVertices), originVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	#pragma endregion

	#pragma region SSAO Sample Kernel & Noise Texture Generation

	// generate sample kernel
	// ----------------------
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;
	std::vector<glm::vec3> ssaoKernel;
	for (unsigned int i = 0; i < 16; ++i)
	{
		glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / 16.0f;

		// scale samples s.t. they're more aligned to center of kernel
		scale = 0.1f + scale * scale * (1.0f - 0.1f);
		sample *= scale;
		ssaoKernel.push_back(sample);
	}

	// generate noise texture
	// ----------------------
	std::vector<glm::vec3> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
		ssaoNoise.push_back(noise);
	}
	unsigned int noiseTexture; 
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);

	#pragma endregion

	//Create Shaders.
	Shader deferredCubeShader(PROJECT_DIR"/src/Shaders/deferredCube.vs", PROJECT_DIR"/src/Shaders/deferredCube.fs");
	Shader shadowShader(PROJECT_DIR"/src/Shaders/shadow.vs", PROJECT_DIR"/src/Shaders/shadow.gs", PROJECT_DIR"/src/Shaders/shadow.fs");
	Shader originShader(PROJECT_DIR"/src/Shaders/origin.vs", PROJECT_DIR"/src/Shaders/origin.gs", PROJECT_DIR"/src/Shaders/origin.fs");
	Shader deferredBedShader(PROJECT_DIR"/src/Shaders/deferredBed.vs", PROJECT_DIR"/src/Shaders/deferredBed.fs");
	Shader deferredLightingShader(PROJECT_DIR"/src/Shaders/deferredLighting.vs", PROJECT_DIR"/src/Shaders/deferredLighting.fs");
	Shader glassShader(PROJECT_DIR"/src/Shaders/glass.vs", PROJECT_DIR"/src/Shaders/glass.fs");
	Shader normalShader(PROJECT_DIR"/src/Shaders/bed_normal.vs", PROJECT_DIR"/src/Shaders/bed_normal.gs", PROJECT_DIR"/src/Shaders/bed_normal.fs");
	Shader ssaoShader(PROJECT_DIR"/src/Shaders/deferredLighting.vs", PROJECT_DIR"/src/Shaders/ssao.fs");
	Shader ssaoBlurShader(PROJECT_DIR"/src/Shaders/deferredLighting.vs", PROJECT_DIR"/src/Shaders/ssaoBlur.fs");
	Shader blurShader(PROJECT_DIR"/src/Shaders/blur.vs", PROJECT_DIR"/src/Shaders/blur.fs");
	Shader ppShader(PROJECT_DIR"/src/Shaders/postProcessing.vs", PROJECT_DIR"/src/Shaders/postProcessing.fs");
	Shader skyboxShader(PROJECT_DIR"/src/Shaders/skybox.vs", PROJECT_DIR"/src/Shaders/skybox.fs");

	//Load Models
	Model bed(PROJECT_DIR"/src/Assets/Models/bed.gltf");
	Model glass(PROJECT_DIR"/src/Assets/Models/glass.gltf");

	//Load Images As Texture.
	stbi_set_flip_vertically_on_load(true);

	// Load the HDR environment maps
	unsigned int hdrTextures[7] = { LoadHDRTexture(PROJECT_DIR"/src/Assets/EnvironmentMaps/1.hdr"), LoadHDRTexture(PROJECT_DIR"/src/Assets/EnvironmentMaps/2.hdr"),
									LoadHDRTexture(PROJECT_DIR"/src/Assets/EnvironmentMaps/3.hdr"), LoadHDRTexture(PROJECT_DIR"/src/Assets/EnvironmentMaps/4.hdr"),
									LoadHDRTexture(PROJECT_DIR"/src/Assets/EnvironmentMaps/5.hdr"), LoadHDRTexture(PROJECT_DIR"/src/Assets/EnvironmentMaps/6.hdr"),
									LoadHDRTexture(PROJECT_DIR"/src/Assets/EnvironmentMaps/7.hdr") };

	unsigned int cubeDiffuseTexture = LoadTexture(PROJECT_DIR"/src/Assets/Textures/bricks2.jpg", true);
	unsigned int cubeNormalTexture = LoadTexture(PROJECT_DIR"/src/Assets/Textures/bricks2_normal.png");
	unsigned int cubeDisplacementTexture = LoadTexture(PROJECT_DIR"/src/Assets/Textures/bricks2_disp.png");

	//Use Shader To Set Uniforms.
	deferredLightingShader.use();
	deferredLightingShader.setInt("gPosition", 0);
	deferredLightingShader.setInt("gNormal", 1);
	deferredLightingShader.setInt("gAlbedo", 2);
	deferredLightingShader.setInt("gEmission", 3);
	deferredLightingShader.setInt("gMetallicRoughness", 4);
	deferredLightingShader.setInt("pointLight[0].shadowMap", 5);
	deferredLightingShader.setInt("pointLight[1].shadowMap", 6);
	deferredLightingShader.setInt("ambientOcclusionMap", 7);
	deferredLightingShader.setInt("irradianceMap", 8);
	deferredLightingShader.setInt("prefilterMap", 9);
	deferredLightingShader.setInt("brdfLUT", 10);

	deferredCubeShader.use();
	deferredCubeShader.setInt("diffuse", 0);
	deferredCubeShader.setInt("normal", 1);
	deferredCubeShader.setInt("displacement", 2);

	ssaoShader.use();
	ssaoShader.setInt("gPosition", 0);
	ssaoShader.setInt("gNormal", 1);
	ssaoShader.setInt("texNoise", 2);

	ssaoBlurShader.use();
	ssaoBlurShader.setInt("ssaoInput", 0);

	normalShader.use();
	normalShader.setUInt("showFaceNormal", 0);
	normalShader.setUInt("showVertexNormal", 0);
	
	ppShader.use();
	ppShader.setInt("screenTexture", 0);
	ppShader.setInt("blurTexture", 1);
	
	blurShader.use();
	blurShader.setInt("brightnessTexture", 0);

	//Material & Lighting Data.
	float ambientStrength  = 0.2f;
	float specularStrength = 0.3f;
	float emissionStrength = 8.0f;
	
	//Default Environment - 0, 1, 2, 3, 4, 5, 6, 7
	int environment = 0;
	int prevEnvironment = 0;	//For PBR IBL Framework Dirty Check.

	//Setup PBR Workflow Based on The First Environment Map.
	SetupPBR(hdrTextures[environment]);

	//Only Initialize PBR IBL Workflow Again When its Dirty.
	bool pbrDirty = false;
	bool pbrEnabled = true;	//Tells us if we are using PBR workflow or not.
	bool physicallyCorrectAttenuation = true;	//If True, then the PBR Attenuation will be similar to how light attenuates irl.

	float l1l = 0.11f;
	float l2l = 0.11f;
	float l1q = 1.5f;
	float l2q = 2.0f;

	float l1P[3] = {-0.663f,  0.4f,  0.0f };
	float l1I	 =   1.0f;
	float l1C[3] = { 0.23f, 0.59f, 0.64f };
	bool  l1b    =   true;

	float l2P[3] = { 0.615f, 0.515f, -0.626f };
	float l2I	 =   1.0f;
	float l2C[3] = { 0.78f, 0.55f, 0.0f };
	bool  l2b    =   true;

	//Translation, Rotation & Scale Data of Models.
	float bT[3] = { 0.0f, 0.0f, 0.0f };
	float bR    =   0.0f;
	float bS[3] = { 1.0f, 1.0f, 1.0f };

	float cT[3] = { -0.608f, 0.505f, -0.434f };
	float cR    =  45.0f;
	float cS[3] = { 0.1f, 0.1f, 0.1f };

	float oS    = 1.0f;
	bool showOrigin = false;
	bool bloom = true;
	int bloomAmount = 1;
	int bloomBlurMethod = 0;
	bool showFaceNormal = false;
	bool showVertexNormal = false;
	bool showLightInfo = false;
	bool shadowForLight1 = true;
	bool shadowForLight2 = true;
	bool debugShadowForLight1 = false;
	bool debugShadowForLight2 = false;
	float softShadowOffsetLight1 = 0.004f;
	float softShadowOffsetLight2 = 0.004f;
	float fsoftShadowFactorLight1 = 800.0f;
	float fsoftShadowFactorLight2 = 800.0f;
	uint shadowTypeLight1 = 2;
	uint shadowTypeLight2 = 2;
	uint toneMapping = 9;
	const char* shadowTypes[] = { "Hard", "Soft", "Fast Soft" };
	static const char* current_shadowTypeL1 = "Fast Soft";
	static const char* current_shadowTypeL2 = "Fast Soft";
	const char* toneMappings[] = { "Exposure", "Reinhard", "Reinhard 2", "Filmic", "ACES Filmic", "Lottes", "Uchimura", "Uncharted 2", "Unreal", "NONE" };
	static const char* current_toneMapping = "NONE";
	float displacement_factor = 0.1f;
	int min_samples = 8, max_samples = 32;
	float exposure = 2.5f;
	bool ssao = true;
	float ssaoRadius = 0.064f;
	float ssaoBias = 0.01f;
	float ssaoStrength = 4.0f;

	//PBR Metallic Roughness Factors.
	float bedMetallic[8] = {0.0f, 0.0f, 0.5f, 0.7f, 1.0f, 0.0f, 0.0f, 0.0f };
	float bedRoughness[8] = {0.644f, 1.0f, 0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f };
	float cubeMetallic = 0.5f;
	float cubeRoughness = 0.5f;

	//FXAA uniforms
	bool fxaaOn = true;
	bool showEdges = false;
	float lumaThreshold = 0.5f;
	float mulReduceReciprocal = 8.0f;
	float minReduceReciprocal = 128.0f;
	float maxSpan = 8.0f;

	//Create Shadow Projection * View Matrices for both Point Lights.
	float near_plane[] = { 0.161f, 0.051f };
	float far_plane[] = { 5.0f, 5.0f };

	bool shadowMapDirty = true;
	float prevShadowMapDirtyIdentifier = cT[0] + cT[1] + cT[2] + cR + cS[0] + cS[1] + cS[2] + near_plane[0] + near_plane[1] + far_plane[0] + far_plane[1];

	//Perform Perspective Projection for our Projection Matrix.
	mat4 projection = perspective(radians(camera.Zoom), (float)bufferWidth / (float)bufferHeight, CAM_NEAR_DIST, CAM_FAR_DIST);

	unsigned int matricesUBO;
	glGenBuffers(1, &matricesUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, matricesUBO, 0, sizeof(mat4));	

	#pragma endregion

	#pragma region Render Loop

	while (!glfwWindowShouldClose(window))
	{
		//Calculate Delta Time.
		float currentFrame = (float)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		//To Make Sure Inputs Are Being Read.
		glfwPollEvents();
		//Process Input.
		ProcessInput(window);

		//A Common 4x4 Matrix Used By Different Meshes to Render Accordingly in World Space.
		mat4 model = mat4(1.0f);

		//Initialize PBR Workflow Again If its Enabled & Dirty.
		if (pbrDirty && pbrEnabled)
		{
			SetupPBR(hdrTextures[environment]);
			pbrDirty = false;
		}

		#pragma region Draw Shadow Cubemaps
		
		if (shadowMapDirty)
		{
			glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
			glDisable(GL_CULL_FACE);

			for (unsigned int i = 0; i < 2; ++i)
			{
				//Bind The Correct Framebuffer.
				glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO[i]);

				//Clear Depth Buffer Before Rendering.
				glClear(GL_DEPTH_BUFFER_BIT);

				//Bind The Correct Cubemap.
				glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowCubemap[i], 0);

				//Set Shadow Projection * View Matrices for both Point Lights.
				mat4 shadowProjection = perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane[i], far_plane[i]);
				vector<mat4> shadowTransforms;
				vec3 lightPos = i == 0 ? make_vec3(l1P) : make_vec3(l2P);
				shadowTransforms.push_back(shadowProjection * lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
				shadowTransforms.push_back(shadowProjection * lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
				shadowTransforms.push_back(shadowProjection * lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
				shadowTransforms.push_back(shadowProjection * lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
				shadowTransforms.push_back(shadowProjection * lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
				shadowTransforms.push_back(shadowProjection * lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

				//Generate Shadow Cubemap For Current Point Light.
				shadowShader.use();
				for (unsigned int j = 0; j < 6; ++j)
					shadowShader.setMat4("shadowMatrices[" + std::to_string(j) + "]", shadowTransforms[j]);
				shadowShader.setFloat("far_plane", far_plane[i]);
				shadowShader.setVector3("lightPos", lightPos);

				//Model Matrix For Cube.
				glBindVertexArray(VAO[2]);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, cubeDiffuseTexture);
				//Model Matrix.
				model = mat4(1.0f);
				model = translate(model, vec3(cT[0], cT[1], cT[2]));
				model = rotate(model, radians(cR), vec3(0.0f, 1.0f, 0.0f));
				model = scale(model, vec3(cS[0], cS[1], cS[2]));
				//Send The Model Matrix To The Shadow Shader.
				shadowShader.setMat4("model", model);
				//Draw Cube From The Current Point Light Position.
				glDrawArrays(GL_TRIANGLES, 0, 36);

				//Model Matrix For Bed.
				model = mat4(1.0f);
				model = translate(model, vec3(bT[0], bT[1], bT[2]));
				model = rotate(model, radians(bR), vec3(0.0f, 1.0f, 0.0f));
				model = scale(model, vec3(bS[0], bS[1], bS[2]));

				//Draw Bed From The Current Point Light Position.
				bed.SimpleDraw(shadowShader, model);
			}

			glEnable(GL_CULL_FACE);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindVertexArray(0);
			glViewport(0, 0, bufferWidth, bufferHeight);

			shadowMapDirty = false;
		}

		#pragma endregion

		#pragma region Deferred Rendering - Geometry Pass

		//Disable Blending.
		glDisable(GL_BLEND);

		// Bind gBuffer as Current Framebuffer & Draw all The Geomtry & Fill The Samplers.
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//Get Camera View Matrix.
		mat4 view = camera.GetViewMatrix();

		if (camZoomDirty)
		{
			//Perform Perspective Projection for our Projection Matrix With The New Field of View in Mind.
			projection = perspective(radians(camera.Zoom), (float)bufferWidth / (float)bufferHeight, CAM_NEAR_DIST, CAM_FAR_DIST);
			camZoomDirty = false;
		}

		//This Matrix Stores The Combined Effort Of Clipping To Camera & Perspective Projection.
		mat4 viewProjection = projection * view;

		//Set Data for UBO.
		glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), value_ptr(viewProjection));
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		#pragma region Draw Bed

		//Model Matrix.
		model = mat4(1.0f);
		model = translate(model, vec3(bT[0], bT[1], bT[2]));
		model = rotate(model, radians(bR), vec3(0.0f, 1.0f, 0.0f));
		model = scale(model, vec3(bS[0], bS[1], bS[2]));
		glFrontFace(GL_CW);
		deferredBedShader.use();
		deferredBedShader.setFloat("material.emissionStrength", emissionStrength);
		//Change The Metallic & Roughness Factors Accordingly.
		for (int i = 0; i < 8; i++)
		{
			bed.meshes[i].material.metallicFactor = bedMetallic[i];
			bed.meshes[i].material.roughnessFactor = bedRoughness[i];
		}
		bed.Draw(deferredBedShader, model);
		glFrontFace(GL_CCW);

		#pragma endregion

		#pragma region Draw Cube

		//Bind Cube VAO.
		glBindVertexArray(VAO[0]);

		//Model Matrix.
		model = mat4(1.0f);
		model = translate(model, vec3(cT[0], cT[1], cT[2]));
		model = rotate(model, radians(cR), vec3(0.0f, 1.0f, 0.0f));
		model = scale(model, vec3(cS[0], cS[1], cS[2]));

		deferredCubeShader.use();
		deferredCubeShader.setVector3("viewPos", camera.Position);
		deferredCubeShader.setMat4("model", model);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cubeDiffuseTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, cubeNormalTexture);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, cubeDisplacementTexture);
		deferredCubeShader.setFloat("roughness", cubeRoughness);
		deferredCubeShader.setFloat("metallicness", cubeMetallic);
		deferredCubeShader.setFloat("displacement_factor", displacement_factor);
		deferredCubeShader.setInt("min_samples", min_samples);
		deferredCubeShader.setInt("max_samples", max_samples);

		//Draw Cube
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glBindVertexArray(0);

		#pragma endregion

		#pragma endregion

		#pragma region SSAO Pass

		if (ssao)
		{
			//Calculate SSAO.
			glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
			glClear(GL_COLOR_BUFFER_BIT);

			ssaoShader.use();
			// Send kernel + rotation 
			for (unsigned int i = 0; i < 16; ++i)
				ssaoShader.setVector3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
			ssaoShader.setMat4("view", view);
			ssaoShader.setMat4("projection", projection);
			ssaoShader.setFloat("radius", ssaoRadius);
			ssaoShader.setFloat("bias", ssaoBias);
			ssaoShader.setFloat("strength", ssaoStrength);
			ssaoShader.setVector2("noiseScale", vec2(bufferWidth, bufferHeight) * 0.25f);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gPosition);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gNormal);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, noiseTexture);
			RenderQuad();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// Blur SSAO texture to remove noise
			glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
			glClear(GL_COLOR_BUFFER_BIT);
			ssaoBlurShader.use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, ssaoGrayscaleBuffer);
			RenderQuad();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		#pragma endregion

		#pragma region Deferred Rendering - Lighting Pass

		//Calculate Lighting Result Of gBuffer in HDR Render Buffer & Extract Fragment & Brightness Color.
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderFBO);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		deferredLightingShader.use();

		#pragma region Set Lighting Uniforms

		deferredLightingShader.setVector3("pointLight[0].position", l1P[0], l1P[1], l1P[2]);		
		deferredLightingShader.setVector3("pointLight[0].color",    l1C[0], l1C[1], l1C[2]);
		deferredLightingShader.setFloat("pointLight[0].intensity", l1I);
		deferredLightingShader.setInt("pointLight[0].blinn",        l1b);

		deferredLightingShader.setVector3("pointLight[1].position",  l2P[0], l2P[1], l2P[2]);
		deferredLightingShader.setVector3("pointLight[1].color",     l2C[0], l2C[1], l2C[2]);
		deferredLightingShader.setFloat("pointLight[1].intensity", l2I);
		deferredLightingShader.setInt    ("pointLight[1].blinn",     l2b);

		deferredLightingShader.setInt    ("pointLight[0].shadows", shadowForLight1);
		deferredLightingShader.setInt    ("pointLight[1].shadows", shadowForLight2);
		deferredLightingShader.setInt	("pointLight[0].debugShadow", debugShadowForLight1);
		deferredLightingShader.setInt	("pointLight[1].debugShadow", debugShadowForLight2);
		deferredLightingShader.setUInt   ("pointLight[0].shadowType", shadowTypeLight1);
		deferredLightingShader.setUInt   ("pointLight[1].shadowType", shadowTypeLight2);
		deferredLightingShader.setFloat  ("pointLight[0].softShadowOffset", softShadowOffsetLight1);
		deferredLightingShader.setFloat  ("pointLight[1].softShadowOffset", softShadowOffsetLight2);
		deferredLightingShader.setFloat  ("pointLight[0].fsoftShadowFactor", fsoftShadowFactorLight1);
		deferredLightingShader.setFloat  ("pointLight[1].fsoftShadowFactor", fsoftShadowFactorLight2);
		deferredLightingShader.setFloat  ("pointLight[0].shadowFarPlane", far_plane[0]);
		deferredLightingShader.setFloat  ("pointLight[1].shadowFarPlane", far_plane[1]);

		deferredLightingShader.setFloat("pointLight[0].linear",    l1l);
		deferredLightingShader.setFloat("pointLight[1].linear",    l2l);
		deferredLightingShader.setFloat("pointLight[0].quadratic", l1q);
		deferredLightingShader.setFloat("pointLight[1].quadratic", l2q);

		deferredLightingShader.setVector3("viewPos", camera.Position);
		deferredLightingShader.setInt("ssaoEnabled", ssao);
		deferredLightingShader.setFloat("ambientStrength", ambientStrength);
		deferredLightingShader.setFloat("specularStrength", specularStrength);

		deferredLightingShader.setInt("pbrEnabled", pbrEnabled);
		deferredLightingShader.setInt("physicallyCorrectAttenuation", physicallyCorrectAttenuation);

		#pragma endregion

		#pragma region Bind Textures

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gAlbedo);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, gEmission);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, gMetallicRoughness);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubemap[0]);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubemap[1]);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, ssaoGrayscaleBlurBuffer);
		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
		glActiveTexture(GL_TEXTURE9);
		glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
		glActiveTexture(GL_TEXTURE10);
		glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);

		#pragma endregion

		RenderQuad();

		#pragma endregion

		#pragma region Bloom Pass

		bool horizontal = true;
		if (bloom)
		{
			//Copy The Brightness Texture From renderFBO to bloomFBO.
			glBindFramebuffer(GL_READ_FRAMEBUFFER, renderFBO);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bloomFBO[0]);

			glReadBuffer(GL_COLOR_ATTACHMENT1);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glBlitFramebuffer(0, 0, bufferWidth, bufferHeight, 0, 0, bufferWidth, bufferHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			blurShader.use();
			blurShader.setInt("blurMethod", bloomBlurMethod);
			for (int i = 0; i < 2 * bloomAmount; ++i)
			{
				//Bind Bloom FBO for Further Blurring of Brightness Texture.
				glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO[horizontal]);
				blurShader.setInt("horizontal", horizontal);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, bloomTexture[!horizontal]);
				RenderQuad();
				horizontal = !horizontal;
			}
		}

		#pragma endregion

		#pragma region HDR Render/Transparency Pass

		//Copy The Depth Buffer From gBuffer To HDR Render Buffer.
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderFBO);

		//Copy Depth.
		glReadBuffer(GL_DEPTH_ATTACHMENT);
		glDrawBuffer(GL_DEPTH_ATTACHMENT);
		glBlitFramebuffer(0, 0, bufferWidth, bufferHeight, 0, 0, bufferWidth, bufferHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		//Draw with RenderFBO.
		glBindFramebuffer(GL_FRAMEBUFFER, renderFBO);

		#pragma region Draw Bed Normals

		//Draw Bed Normals if Enabled
		if (showFaceNormal || showVertexNormal)
		{
			//Model Matrix.
			model = mat4(1.0f);
			model = translate(model, vec3(bT[0], bT[1], bT[2]));
			model = rotate(model, radians(bR), vec3(0.0f, 1.0f, 0.0f));
			model = scale(model, vec3(bS[0], bS[1], bS[2]));
			normalShader.use();
			normalShader.setMat4("view", view);
			normalShader.setMat4("projection", projection);
			normalShader.setUInt("showFaceNormal", showFaceNormal);
			normalShader.setUInt("showVertexNormal", showVertexNormal);
			bed.SimpleDraw(normalShader, model);
		}

		#pragma endregion

		#pragma region Draw Origin/Light Position Vector

		if (showOrigin || showLightInfo)
		{
			originShader.use();
			originShader.setMat4("view", view);
			model = mat4(1.0f);
			originShader.setMat4("originModel", model);
			originShader.setMat4("projection", projection);

			//Bind Origin VAO.
			glBindVertexArray(VAO[1]);

			if (showOrigin)
			{
				originShader.setFloat("size", oS);
				glDrawArrays(GL_POINTS, 0, 6);
			}

			if (showLightInfo)
			{
				originShader.setFloat("size", 0.1f);

				for (int i = 0; i < 2; ++i)
				{
					vec4 lp = i == 0 ? vec4(l1P[0], l1P[1], l1P[2], 1.0f) : vec4(l2P[0], l2P[1], l2P[2], 1.0f);
					model = mat4(1.0f);
					model = translate(model, vec3(lp));
					originShader.setMat4("originModel", model);
					glDrawArrays(GL_POINTS, 0, 6);
				}
			}

			glBindVertexArray(0);
		}

		#pragma endregion

		#pragma region Draw Skybox

		glDepthFunc(GL_LEQUAL);
		mat4 skyViewProjection = projection * mat4(mat3(view));
		skyboxShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		skyboxShader.setMat4("viewProjection", skyViewProjection);
		RenderCube();
		glDepthFunc(GL_LESS);

		#pragma endregion

		#pragma region Draw Transparent Objects

		//Enable Blending.
		glEnable(GL_BLEND);

		//Model Matrix.
		model = mat4(1.0f);
		model = translate(model, vec3(bT[0], bT[1], bT[2]));
		model = rotate(model, radians(bR), vec3(0.0f, 1.0f, 0.0f));
		model = scale(model, vec3(bS[0], bS[1], bS[2]));
		glFrontFace(GL_CW);
		glassShader.use();
		glass.Draw(glassShader, model);
		glFrontFace(GL_CCW);

		//Disable Blending.
		glDisable(GL_BLEND);

		#pragma endregion

		#pragma endregion

		#pragma region Draw Screen Quad with Post Processing Shader

		// now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, finalColorBufferTexture[0]);	// use the color attachment texture as the texture of the quad plane
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, bloomTexture[!horizontal]);
		ppShader.use();
		ppShader.setMat4("view", view);
		ppShader.setFloat("exposure", exposure);
		ppShader.setUInt("toneMapping", toneMapping);
		ppShader.setUInt("bloom", bloom);
		ppShader.setBool("fxaaOn", fxaaOn);
		ppShader.setBool("showEdges", showEdges);
		ppShader.setVector2("texelStep", vec2(1.0f / (float)bufferWidth, 1.0f / (float)bufferHeight));
		ppShader.setFloat("lumaThreshold", lumaThreshold);
		ppShader.setFloat("mulReduce", 1.0f / mulReduceReciprocal);
		ppShader.setFloat("minReduce", 1.0f / minReduceReciprocal);
		ppShader.setFloat("maxSpan", maxSpan);

		RenderQuad();

		#pragma endregion

		#pragma region Draw ImGui

		#pragma region Initialize Frame

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		#pragma endregion

		#pragma region Show FPS

		ImGui::Begin("FPS");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();

		#pragma endregion

		#pragma region Model, Material & Shader Settings

		ImGui::Begin("Settings");
		ImGui::SliderInt("Environment", &environment, 0, 6);
		if (prevEnvironment != environment)	pbrDirty = true;
		prevEnvironment = environment;
		
		ImGui::Text("Tone Mapping");
		ImGui::SameLine(210.0f, -1.0f);
		if (ImGui::BeginCombo("##Tone Mapping", current_toneMapping, ImGuiComboFlags_NoArrowButton)) // The second parameter is the label previewed before opening the combo.
		{
			for (int n = 0; n < 10; n++)
			{
				bool is_selected = (current_toneMapping == toneMappings[n]); // You can store your selection however you want, outside or inside your objects
				if (ImGui::Selectable(toneMappings[n], is_selected))
				{
					current_toneMapping = toneMappings[n];
					// Get The Current Tone Mapping.
					toneMapping = n;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
			}
			ImGui::EndCombo();
		}

		if (toneMapping == 0)
			ImGui::SliderFloat("Exposure", &exposure, 0.0f, 10.0f);

		ImGui::NewLine();
		ImGui::Checkbox("SSAO", &ssao);
		ImGui::SliderFloat("SSAO Radius", &ssaoRadius, 0.0f, 0.2f);
		ImGui::SliderFloat("SSAO Bias", &ssaoBias, 0.0f, 0.2f);
		ImGui::SliderFloat("SSAO Strength", &ssaoStrength, 1.0f, 50.0f);

		ImGui::NewLine();
		ImGui::Checkbox("Bloom", &bloom);
		if (bloom)
		{
			ImGui::SliderInt("Bloom Amount", &bloomAmount, 0, 100);
			ImGui::SliderInt("Blur Method", &bloomBlurMethod, 0, 1);
		}
		
		ImGui::NewLine();
		ImGui::Checkbox("FXAA", &fxaaOn);
		if (fxaaOn)
		{	
			ImGui::Checkbox("Show Edges", &showEdges);
			ImGui::SliderFloat("Luma Threshold", &lumaThreshold, 0.0f, 1.0f);
			ImGui::SliderFloat("Mul Reduce Reciprocal", &mulReduceReciprocal, 0.0f, 256.0f);
			ImGui::SliderFloat("Min Reduce Reciprocal", &minReduceReciprocal, 0.0f, 512.0f);
			ImGui::SliderFloat("Max Span", &maxSpan, 0.0f, 16.0f);
		}

		ImGui::NewLine();
		ImGui::Checkbox("PBR Shading", &pbrEnabled);
		if (pbrEnabled)
		{
			ImGui::Checkbox("Physically Correct Attenuation", &physicallyCorrectAttenuation);

			ImGui::NewLine();
			ImGui::SliderFloat("Bed Metallicness", &bedMetallic[0], 0.0f, 1.0f);
			ImGui::SliderFloat("Bed Roughness",   &bedRoughness[0], 0.0f, 1.0f);

			ImGui::NewLine();
			ImGui::SliderFloat("External Lamp Metallicness", &bedMetallic[1], 0.0f, 1.0f);
			ImGui::SliderFloat("External Lamp Roughness",   &bedRoughness[1], 0.0f, 1.0f);

			ImGui::NewLine();
			ImGui::SliderFloat("Wall Lamp Metallicness", &bedMetallic[2], 0.0f, 1.0f);
			ImGui::SliderFloat("Wall Lamp Roughness",   &bedRoughness[2], 0.0f, 1.0f);

			ImGui::NewLine();
			ImGui::SliderFloat("Turtle Metallicness", &bedMetallic[3], 0.0f, 1.0f);
			ImGui::SliderFloat("Turtle Roughness",   &bedRoughness[3], 0.0f, 1.0f);

			ImGui::NewLine();
			ImGui::SliderFloat("Aladin Lamp Metallicness", &bedMetallic[4], 0.0f, 1.0f);
			ImGui::SliderFloat("Aladin Lamp Roughness",   &bedRoughness[4], 0.0f, 1.0f);

			ImGui::NewLine();
			ImGui::SliderFloat("Plant 1 Metallicness", &bedMetallic[5], 0.0f, 1.0f);
			ImGui::SliderFloat("Plant 1 Roughness",   &bedRoughness[5], 0.0f, 1.0f);

			ImGui::NewLine();
			ImGui::SliderFloat("Plant 2 Metallicness", &bedMetallic[6], 0.0f, 1.0f);
			ImGui::SliderFloat("Plant 2 Roughness",   &bedRoughness[6], 0.0f, 1.0f);

			ImGui::NewLine();
			ImGui::SliderFloat("Plant 3 Metallicness", &bedMetallic[7], 0.0f, 1.0f);
			ImGui::SliderFloat("Plant 3 Roughness",   &bedRoughness[7], 0.0f, 1.0f);

			ImGui::NewLine();
			ImGui::SliderFloat("Cube Metallicness", &cubeMetallic, 0.0f, 1.0f);
			ImGui::SliderFloat("Cube Roughness", &cubeRoughness, 0.0f, 1.0f);
		}

		ImGui::NewLine();
		ImGui::Checkbox("Show Origin", &showOrigin);
		if (showOrigin)
			ImGui::SliderFloat("Origin Size", &oS, 0.0f, 20.0f);

		ImGui::NewLine();
		ImGui::SliderFloat3("Bed  Position", bT, -10.0f, 10.0f);
		ImGui::SliderFloat("Bed  Rotation", &bR, 0.0f, 360.0f);
		ImGui::SliderFloat3("Bed  Scale", bS, 0.0f, 20.0f);
		ImGui::NewLine();
		if (!pbrEnabled)
		{
			ImGui::SliderFloat("Ambient Strength", &ambientStrength, 0.0f, 1.0f);
			ImGui::SliderFloat("Specular Strength", &specularStrength, 0.0f, 1.0f);
		}
		ImGui::SliderFloat("Emission Strength", &emissionStrength, 0.0f, 40.0f);
		ImGui::NewLine();
		ImGui::Checkbox("Show Bed Face Normals", &showFaceNormal);
		ImGui::Checkbox("Show Bed Vertex Normals", &showVertexNormal);
		ImGui::Checkbox("Show Light Info", &showLightInfo);

		ImGui::NewLine();
		ImGui::SliderFloat3("Cube Position", cT, -1.0f, 1.0f);
		ImGui::SliderFloat("Cube Rotation", &cR, 0.0f, 360.0f);
		ImGui::SliderFloat3("Cube Scale", cS, 0.0f, 20.0f);
		ImGui::SliderFloat("Displacement Factor", &displacement_factor, 0.0f, 1.0f);
		ImGui::SliderInt("Min Samples", &min_samples, 0, 100);
		ImGui::SliderInt("Max Samples", &max_samples, 1, 100);

		ImGui::End();

		#pragma endregion

		#pragma region Light 1 Settings

		ImGui::Begin("Light 1 Settings");

		ImGui::SliderFloat3("Position", l1P, -0.7f, -0.6f);
		ImGui::SliderFloat("Intensity", &l1I, 0.0f, 10.0f);
		ImGui::ColorEdit3("Color", l1C);
		ImGui::Checkbox("Blinn-Phong Lighting", &l1b);

		ImGui::NewLine();
		ImGui::Checkbox("Cast Shadows", &shadowForLight1);
		ImGui::Checkbox("Debug Shadows", &debugShadowForLight1);

		ImGui::Text("Shadow Type");
		ImGui::SameLine(210.0f, -1.0f);
		if (ImGui::BeginCombo("##Shadow Type", current_shadowTypeL1, ImGuiComboFlags_NoArrowButton)) // The second parameter is the label previewed before opening the combo.
		{
			for (int n = 0; n < 3; n++)
			{
				bool is_selected = (current_shadowTypeL1 == shadowTypes[n]); // You can store your selection however you want, outside or inside your objects
				if (ImGui::Selectable(shadowTypes[n], is_selected))
					current_shadowTypeL1 = shadowTypes[n];
				if (is_selected)
					ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
			}
			ImGui::EndCombo();
		}

		if (current_shadowTypeL1 == shadowTypes[2])
		{
			shadowTypeLight1 = 2;
			ImGui::SliderFloat("Fast Soft Shadow Factor", &fsoftShadowFactorLight1, 50.0f, 5000.0f);
		}
		else if (current_shadowTypeL1 == shadowTypes[1])
		{
			shadowTypeLight1 = 1;
			ImGui::SliderFloat("Soft Shadow Offset", &softShadowOffsetLight1, 0.0f, 0.2f);
		}
		else
			shadowTypeLight1 = 0;

		ImGui::NewLine();
		if (!pbrEnabled)
		{
			ImGui::SliderFloat("Linear Attenuation", &l1l, 0.0f, 2.0f);
			ImGui::SliderFloat("Quadratic Attenuation", &l1q, 0.0f, 2.0f);
		}else if(pbrEnabled && !physicallyCorrectAttenuation)
			ImGui::SliderFloat("Quadratic Attenuation", &l1q, 0.0f, 20.0f);
		ImGui::SliderFloat("Near Plane", &near_plane[0], 0.0f, 1.0f);
		ImGui::SliderFloat("Far Plane", &far_plane[0], 0.0f, 50.0f);

		ImGui::End();

		#pragma endregion

		#pragma region Light 2 Settings

		ImGui::Begin("Light 2 Settings");

		ImGui::SliderFloat3("Position", l2P, -1.0f, 0.0f);
		ImGui::SliderFloat("Intensity", &l2I, 0.0f, 10.0f);
		ImGui::ColorEdit3("Color", l2C);
		ImGui::Checkbox("Blinn-Phong Lighting", &l2b);

		ImGui::NewLine();
		ImGui::Checkbox("Cast Shadows", &shadowForLight2);
		ImGui::Checkbox("Debug Shadows", &debugShadowForLight2);

		ImGui::Text("Shadow Type");
		ImGui::SameLine(210.0f, -1.0f);
		if (ImGui::BeginCombo("##Shadow Type", current_shadowTypeL2, ImGuiComboFlags_NoArrowButton)) // The second parameter is the label previewed before opening the combo.
		{
			for (int n = 0; n < 3; n++)
			{
				bool is_selected = (current_shadowTypeL2 == shadowTypes[n]); // You can store your selection however you want, outside or inside your objects
				if (ImGui::Selectable(shadowTypes[n], is_selected))
					current_shadowTypeL2 = shadowTypes[n];
				if (is_selected)
					ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
			}
			ImGui::EndCombo();
		}

		if (current_shadowTypeL2 == shadowTypes[2])
		{
			shadowTypeLight2 = 2;
			ImGui::SliderFloat("Fast Soft Shadow Factor", &fsoftShadowFactorLight2, 50.0f, 5000.0f);
		}
		else if (current_shadowTypeL2 == shadowTypes[1])
		{
			shadowTypeLight2 = 1;
			ImGui::SliderFloat("Soft Shadow Offset", &softShadowOffsetLight2, 0.0f, 0.2f);
		}
		else
			shadowTypeLight2 = 0;

		ImGui::NewLine();
		if (!pbrEnabled)
		{
			ImGui::SliderFloat("Linear Attenuation", &l2l, 0.0f, 2.0f);
			ImGui::SliderFloat("Quadratic Attenuation", &l2q, 0.0f, 2.0f);
		}
		else if(pbrEnabled && !physicallyCorrectAttenuation)
			ImGui::SliderFloat("Quadratic Attenuation", &l2q, 0.0f, 20.0f);
		
		ImGui::SliderFloat("Near Plane", &near_plane[1], 0.0f, 0.2f);
		ImGui::SliderFloat("Far Plane", &far_plane[1], 0.0f, 50.0f);

		ImGui::End();

		#pragma endregion

		#pragma region Render 

		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		#pragma endregion

		#pragma endregion

		#pragma region Shadowmap Dirty Check

		float currentShadowMapDirtyIdentifier = cT[0] + cT[1] + cT[2] + cR + cS[0] + cS[1] + cS[2] + near_plane[0] + near_plane[1] + far_plane[0] + far_plane[1];
		if (abs(currentShadowMapDirtyIdentifier - prevShadowMapDirtyIdentifier) > 0.0001f)
			shadowMapDirty = true;
		prevShadowMapDirtyIdentifier = currentShadowMapDirtyIdentifier;

		#pragma endregion

		//Swap Buffers.
		glfwSwapBuffers(window);
	}

	#pragma endregion

	#pragma region Cleanup & Termination

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;

	#pragma endregion

}

#pragma endregion

#pragma region Window Resize Callback

/// <summary>
/// This Function Gets Called On The Event Of Window Resize To Recalculate glViewport.
/// </summary>
/// <param name="window">Current Window</param>
/// <param name="width">New Window Width</param>
/// <param name="height">New Window Height</param>
void Window_Resize_Callback(GLFWwindow* window, int width, int height)
{
	//Get The Frame Buffer Size.
	glfwGetFramebufferSize(window, &bufferWidth, &bufferHeight);

	UpdateAllFramebuffersSize(bufferWidth, bufferHeight);

	//Resize The Viewport.
	glViewport(0, 0, bufferWidth, bufferHeight);
}

#pragma endregion

#pragma region Process Input

/// <summary>
/// Process Input On Each Frame Of The Render Loop.
/// </summary>
/// <param name="window">Current Window</param>
void ProcessInput(GLFWwindow* window)
{
	//Tell GLFW to capture our mouse only if we held down the Right Mouse Button.
	if (glfwGetMouseButton(window, 1))
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		camera.updateRotation = true;
	}
	else
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		firstMouse = true;
		camera.updateRotation = false;
	}

	//Close Window On Pressing Escape Key.
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	//Stop Full Screen State Change Spam.
	timeSinceFullScreenStateSwitch += deltaTime;
	if (timeSinceFullScreenStateSwitch >= fullScreenSwitchWaitTime)
	{
		timeSinceFullScreenStateSwitch = 0.0f;
		fullScreenDirty = false;
	}

	//Set To Full Screen If In Windowed Or Vice Versa.
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fullScreenDirty)
	{
		if (fullScreen)
		{
			//Set To Windowed Mode.
			glfwSetWindowMonitor(window, NULL, 0, 0, SCR_WIDTH, SCR_HEIGHT, GLFW_DONT_CARE);
			//Get The Frame Buffer Size & Update Them.
			glfwGetFramebufferSize(window, &bufferWidth, &bufferHeight);
			UpdateAllFramebuffersSize(bufferWidth, bufferHeight);
			fullScreen = false;
			fullScreenDirty = true;
		}
		else
		{
			//Set To Fullscreen Mode.
			glfwSetWindowMonitor(window, primaryMonitor, 0, 0, videoMode->width, videoMode->height, videoMode->refreshRate);
			//Get The Frame Buffer Size & Update Them.
			glfwGetFramebufferSize(window, &bufferWidth, &bufferHeight);
			UpdateAllFramebuffersSize(bufferWidth, bufferHeight);
			fullScreen = true;
			fullScreenDirty = true;
		}
	}

	//Perform Camera Input.
	bool forwardPressed = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
	bool backwardPressed = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
	bool leftwardPressed = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
	bool rightwardPressed = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
	bool upwardPressed = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;
	bool downwardPressed = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
	bool hasCameraMovementInput = forwardPressed || backwardPressed || leftwardPressed ||
		rightwardPressed || upwardPressed || downwardPressed;

	timeSinceCameraMovementInput = hasCameraMovementInput ? timeSinceCameraMovementInput + deltaTime : 0.0f;

	if (forwardPressed)
		camera.ProcessKeyboard(FORWARD, deltaTime, timeSinceCameraMovementInput);
	if (backwardPressed)
		camera.ProcessKeyboard(BACKWARD, deltaTime, timeSinceCameraMovementInput);
	if (leftwardPressed)
		camera.ProcessKeyboard(LEFT, deltaTime, timeSinceCameraMovementInput);
	if (rightwardPressed)
		camera.ProcessKeyboard(RIGHT, deltaTime, timeSinceCameraMovementInput);
	if (upwardPressed)
		camera.ProcessKeyboard(UP, deltaTime, timeSinceCameraMovementInput);
	if (downwardPressed)
		camera.ProcessKeyboard(DOWN, deltaTime, timeSinceCameraMovementInput);
	if (glfwGetKey(window, GLFW_KEY_KP_ADD))
		camera.Zoom -= 50.0f * deltaTime;
	else if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT))
		camera.Zoom += 50.0f * deltaTime;
	camera.Zoom = clamp(camera.Zoom, 1.0f, 45.0f);
}

#pragma endregion

#pragma region Process Mouse Input

/// <summary>
/// Whenever the mouse moves, this callback is called.
/// </summary>
/// <param name="window">Window</param>
/// <param name="xpos">Mouse X Position in Screen Coordinates</param>
/// <param name="ypos">Mouse Y Position in Screen Coordinates</param>
void ProcessMouseInput(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = (float)xpos;
		lastY = (float)ypos;
		firstMouse = false;
	}

	float xoffset = (float)xpos - lastX;
	float yoffset = lastY - (float)ypos; // reversed since y-coordinates go from bottom to top

	lastX = (float)xpos;
	lastY = (float)ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

#pragma endregion

#pragma region Process Scroll Input

/// <summary>
/// Whenever the mouse scroll wheel scrolls, this callback is called.
/// </summary>
/// <param name="window">Window</param>
/// <param name="xoffset">Scroll X Input</param>
/// <param name="yoffset">Scroll Y Input</param>
void ProcessScrollInput(GLFWwindow* window, double xoffset, double yoffset)
{
	//Option To Control Movement Speed Mutliplier
	if (camera.updateRotation)
		camera.ProcessMouseScrollCameraSpeed((float)yoffset);
	else
	{
		//Only Handle Zooming When Not Updating Camera Rotation.
		camera.ProcessMouseScroll((float)yoffset);
		camZoomDirty = true;
	}
}

#pragma endregion

#pragma region Load Texture

/// <summary>
/// Utility function for loading a 2D texture from file
/// </summary>
/// <param name="path">Texture Path</param>
/// <param name="sRGB">Texture Color Space</param> 
/// <returns>Texture ID</returns>
unsigned int LoadTexture(char const* path, bool sRGB)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format = GL_RED;
		GLenum internalFormat = GL_RED;
		if (nrComponents == 1)
		{
			format = GL_RED;
			internalFormat = GL_RED;
		}
		else if (nrComponents == 3)
		{
			format = GL_RGB;
			internalFormat = sRGB ? GL_SRGB : GL_RGB;
		}
		else if (nrComponents == 4)
		{
			format = GL_RGBA;
			internalFormat = sRGB ? GL_SRGB_ALPHA : GL_RGBA;
		}

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

#pragma endregion

#pragma region Load HDR Texture

/// <summary>
/// Load A HDR Texture with all unclamped Floating Point Values.
/// </summary>
/// <param name="path">Path</param>
/// <returns></returns>
unsigned int LoadHDRTexture(char const* path)
{
	int width, height, nrComponents;
	float* data = stbi_loadf(path, &width, &height, &nrComponents, 0);
	unsigned int hdrTexture = 0;
	if (data)
	{
		glGenTextures(1, &hdrTexture);
		glBindTexture(GL_TEXTURE_2D, hdrTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); // note how we specify the texture's data value to be float

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Failed to load HDR image." << std::endl;
	}

	return hdrTexture;
}

#pragma endregion

#pragma region Load Cubemap

/// <summary>
/// Utility function for loading a Cubemap from path of all the faces of the cube.
/// </summary>
/// <param name="path">Textures Path</param>
/// <returns>Texture ID</returns>
unsigned int LoadCubemap(vector<string> path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < path.size(); i++)
	{
		unsigned char* data = stbi_load(path[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			GLenum format = nrChannels > 3 ? GL_RGBA : GL_RGB;
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap tex failed to load at path: " << path[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

#pragma endregion

#pragma region Update FrameBuffer

/// <summary>
/// Updates The FrameBuffer To Adapt To Screen Dimension Changes.
/// </summary>
/// <param name="bufferWidth">New Buffer Width</param>
/// <param name="bufferHeight">New Buffer Height</param>
void UpdateAllFramebuffersSize(int bufferWidth, int bufferHeight)
{
	
	#pragma region Resize HDR Render Buffer

	glBindFramebuffer(GL_FRAMEBUFFER, renderFBO);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindTexture(GL_TEXTURE_2D, finalColorBufferTexture[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bufferWidth, bufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
		// attach it to currently bound framebuffer object
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, finalColorBufferTexture[i], 0);
	}
	glBindRenderbuffer(GL_RENDERBUFFER, finalRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, bufferWidth, bufferHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, finalRBO);
	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	#pragma endregion

	#pragma region Resize gBuffer

	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

	// position color buffer
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

	// normal color buffer
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

	// Albedo color buffer
	glBindTexture(GL_TEXTURE_2D, gAlbedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);

	// Emission color buffer
	glBindTexture(GL_TEXTURE_2D, gEmission);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gEmission, 0);

	// Metallic Roughness color buffer
	glBindTexture(GL_TEXTURE_2D, gMetallicRoughness);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, bufferWidth, bufferHeight, 0, GL_RG, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, gMetallicRoughness, 0);

	unsigned int colorAttachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
	glDrawBuffers(5, colorAttachments);

	// gDepth Buffer
	glBindRenderbuffer(GL_RENDERBUFFER, gDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, bufferWidth, bufferHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gDepth);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	#pragma endregion

	#pragma region Resize SSAO Buffer

	glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	// SSAO Grayscale Buffer
	glBindTexture(GL_TEXTURE_2D, ssaoGrayscaleBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, bufferWidth, bufferHeight, 0, GL_RED, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoGrayscaleBuffer, 0);

	// SSAO Grayscale Blur Buffer
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
	glBindTexture(GL_TEXTURE_2D, ssaoGrayscaleBlurBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, bufferWidth, bufferHeight, 0, GL_RED, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoGrayscaleBlurBuffer, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	#pragma endregion

	#pragma region Resize Bloom Buffer

	for (unsigned int i = 0; i < 2; ++i)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO[i]);
		glBindTexture(GL_TEXTURE_2D, bloomTexture[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bufferWidth, bufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomTexture[i], 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	#pragma endregion

}

#pragma endregion

#pragma region Render Quad

/// <summary>
/// Renders a 1x1 XY quad in NDC
/// </summary>
void RenderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

#pragma endregion

#pragma region Render Cube

/// <summary>
/// Renders a 1x1 3D cube in NDC.
/// </summary>
void RenderCube()
{
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f, // bottom-left
			 1.0f,  1.0f, -1.0f, // top-right
			 1.0f, -1.0f, -1.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f, // top-right
			-1.0f, -1.0f, -1.0f, // bottom-left
			-1.0f,  1.0f, -1.0f, // top-left
			// front face		 
			-1.0f, -1.0f,  1.0f, // bottom-left
			 1.0f, -1.0f,  1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f, // top-right
			 1.0f,  1.0f,  1.0f, // top-right
			-1.0f,  1.0f,  1.0f, // top-left
			-1.0f, -1.0f,  1.0f, // bottom-left
			// left face		 
			-1.0f,  1.0f,  1.0f, // top-right
			-1.0f,  1.0f, -1.0f, // top-left
			-1.0f, -1.0f, -1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, // top-right
			// right face		 
			 1.0f,  1.0f,  1.0f, // top-left
			 1.0f, -1.0f, -1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f, // top-right         
			 1.0f, -1.0f, -1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f, // top-left
			 1.0f, -1.0f,  1.0f, // bottom-left     
			// bottom face		 
			-1.0f, -1.0f, -1.0f, // top-right
			 1.0f, -1.0f, -1.0f, // top-left
			 1.0f, -1.0f,  1.0f, // bottom-left
			 1.0f, -1.0f,  1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, // bottom-right
			-1.0f, -1.0f, -1.0f, // top-right
			// top face			 
			-1.0f,  1.0f, -1.0f, // top-left
			 1.0f,  1.0f , 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f, // top-right     
			 1.0f,  1.0f,  1.0f, // bottom-right
			-1.0f,  1.0f, -1.0f, // top-left
			-1.0f,  1.0f,  1.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glFrontFace(GL_CW);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glFrontFace(GL_CCW);
	glBindVertexArray(0);
}

#pragma endregion

#pragma region Setup PBR

/// <summary>
/// Initializes The EnvMap, irradianceMap, prefilterMap & bdrfLutTexture Based on the Given HDR Map.
/// </summary>
/// <param name="hdrTexture"></param>
void SetupPBR(unsigned int hdrTexture)
{
	// Setup framebuffer
	// ----------------------
	if(!pbrInitialized)
	{
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 1024, 1024);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

	// Setup cubemap to render to and attach to framebuffer
	// ---------------------------------------------------------
	if(!pbrInitialized)	glGenTextures(1, &envCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	if (!pbrInitialized)
	{
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 1024, 1024, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // enable pre-filter mipmap sampling (combatting visible dots artifact)
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	// pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
	// ----------------------------------------------------------------------------------------------
	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] =
	{
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	// pbr: convert HDR equirectangular environment map to cubemap equivalent
	// ----------------------------------------------------------------------
	Shader equirectangularToCubemapShader(PROJECT_DIR"/src/Shaders/cubemap.vs", PROJECT_DIR"/src/Shaders/equirectangular_to_cubemap.fs");
	equirectangularToCubemapShader.use();
	equirectangularToCubemapShader.setInt("equirectangularMap", 0);
	equirectangularToCubemapShader.setMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hdrTexture);

	glViewport(0, 0, 1024, 1024); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; ++i)
	{
		equirectangularToCubemapShader.setMat4("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		RenderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// Create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
	// --------------------------------------------------------------------------------
	if(!pbrInitialized)	glGenTextures(1, &irradianceMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	if (!pbrInitialized)
	{
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 64, 64, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	}
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 64, 64);

	// pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
	// -----------------------------------------------------------------------------
	Shader irradianceShader(PROJECT_DIR"/src/Shaders/cubemap.vs", PROJECT_DIR"/src/Shaders/irradiance_convolution.fs");
	irradianceShader.use();
	irradianceShader.setInt("environmentMap", 0);
	irradianceShader.setMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	glViewport(0, 0, 64, 64); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; ++i)
	{
		irradianceShader.setMat4("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		RenderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
	// --------------------------------------------------------------------------------
	if(!pbrInitialized)	glGenTextures(1, &prefilterMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	if (!pbrInitialized)
	{
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 256, 256, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	// Generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// Run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
	// ----------------------------------------------------------------------------------------------------
	Shader prefilterShader(PROJECT_DIR"/src/Shaders/cubemap.vs", PROJECT_DIR"/src/Shaders/prefilter.fs");
	prefilterShader.use();
	prefilterShader.setInt("environmentMap", 0);
	prefilterShader.setMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	unsigned int maxMipLevels = 5;
	for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
	{
		// reisze framebuffer according to mip-level size.
		unsigned int mipWidth = static_cast<unsigned int>(256 * std::pow(0.5, mip));
		unsigned int mipHeight = static_cast<unsigned int>(256 * std::pow(0.5, mip));
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float)mip / (float)(maxMipLevels - 1);
		prefilterShader.setFloat("roughness", roughness);
		for (unsigned int i = 0; i < 6; ++i)
		{
			prefilterShader.setMat4("view", captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			RenderCube();
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Generate a 2D LUT from the BRDF equations used.
	// ----------------------------------------------------
	if(!pbrInitialized)	glGenTextures(1, &brdfLUTTexture);
	Shader brdfShader(PROJECT_DIR"/src/Shaders/brdf.vs", PROJECT_DIR"/src/Shaders/brdf.fs");

	// pre-allocate enough memory for the LUT texture.
	glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
	if (!pbrInitialized)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 1024, 1024, 0, GL_RG, GL_FLOAT, 0);
		// be sure to set wrapping mode to GL_CLAMP_TO_EDGE
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	// then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 1024, 1024);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

	glViewport(0, 0, 1024, 1024);
	brdfShader.use();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	RenderQuad();

	//Reset Viewport Size.
	glViewport(0, 0, bufferWidth, bufferHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	//Set PBR Initialized as True.
	pbrInitialized = true;
}

#pragma endregion

#pragma endregion