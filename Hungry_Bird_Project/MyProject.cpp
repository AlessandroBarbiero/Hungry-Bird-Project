// This has been adapted from the Vulkan tutorial

#include "MyProject.hpp"

const std::string MODEL_PATH = "Assets/models";
const std::string TEXTURE_PATH = "Assets/textures";


// The global buffer object used for view and proj
struct GlobalUniformBufferObject {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

// The uniform buffer object used for models
struct UniformBufferObject {
	alignas(16) glm::mat4 model;
};

class SkyBox {
protected:
	Pipeline P_SkyBox;
	Model M_skyBox;
	Texture T_skyBox;
	DescriptorSet DS_skyBox;

public:
	// initialize all attributes
	void init(BaseProject* bp, DescriptorSetLayout DSLobj, DescriptorSetLayout DSLglobal) {
		P_SkyBox.init(bp, "shaders/skyBoxVert.spv", "shaders/skyBoxFrag.spv", { &DSLglobal, &DSLobj });
		M_skyBox.init(bp, MODEL_PATH + "/SkyBox/SkyBox.obj");
		T_skyBox.init(bp, TEXTURE_PATH + "/SkyBox/SkyBox.png");
		DS_skyBox.init(bp, &DSLobj, {
		{0, UNIFORM, sizeof(UniformBufferObject), nullptr},
		{1, TEXTURE, 0, &T_skyBox}
			});
	}

	// cleanup all the attributes
	void cleanup() {
		DS_skyBox.cleanup();
		T_skyBox.cleanup();
		M_skyBox.cleanup();
		P_SkyBox.cleanup();
	}


	// Populate command buffer ( bind pipeline, descriptorSet global and descriptorSet skyBox )
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage, DescriptorSet DS_global) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			P_SkyBox.graphicsPipeline);

		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P_SkyBox.pipelineLayout, 0, 1, &DS_global.descriptorSets[currentImage],
			0, nullptr);

		VkBuffer vertexBuffers_skyBox[] = { M_skyBox.vertexBuffer };
		VkDeviceSize offsets_skyBox[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers_skyBox, offsets_skyBox);
		vkCmdBindIndexBuffer(commandBuffer, M_skyBox.indexBuffer, 0,
			VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P_SkyBox.pipelineLayout, 1, 1, &DS_skyBox.descriptorSets[currentImage],
			0, nullptr);
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(M_skyBox.indices.size()), 1, 0, 0, 0);
	}

	// update before rendering
	UniformBufferObject update(UniformBufferObject ubo) {
		ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(50.0f, 50.0f, 50.0f));
		return ubo;
	}

	// update ubo and render
	void updateUniformBuffer(VkDevice device, int currentImage, void* data, UniformBufferObject ubo) {
		ubo = update(ubo);
		vkMapMemory(device, DS_skyBox.uniformBuffersMemory[0][currentImage], 0,
			sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, DS_skyBox.uniformBuffersMemory[0][currentImage]);
	}
};

class Camera {
protected:
	glm::vec3 CamPos = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 CamAng = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::mat4 CamDir;

	const float ROT_SPEED = glm::radians(60.0f);
	const float MOVE_SPEED = 1.75f;

public:
	// update the camera position
	glm::mat4 update(GLFWwindow* window, float deltaT) {
		// Camera movements
		if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			CamAng.y += deltaT * ROT_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			CamAng.y -= deltaT * ROT_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_UP)) {
			CamAng.x += deltaT * ROT_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN)) {
			CamAng.x -= deltaT * ROT_SPEED;
		}

		glm::mat3 CamEye = glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.y, glm::vec3(0.0f, 1.0f, 0.0f))) *
			glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.x, glm::vec3(1.0f, 0.0f, 0.0f))) *
			glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.z, glm::vec3(0.0f, 0.0f, 1.0f)));

		if (glfwGetKey(window, GLFW_KEY_A)) {
			CamPos -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_D)) {
			CamPos += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_S)) {
			CamPos += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_W)) {
			CamPos -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_F)) {
			CamPos -= MOVE_SPEED * glm::vec3(0, 1, 0) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_R)) {
			CamPos += MOVE_SPEED * glm::vec3(0, 1, 0) * deltaT;
		}

		return CamDir = glm::translate(glm::transpose(glm::mat4(CamEye)), -CamPos);
	}

};

class Asset {
protected:
	Model model;
	Texture texture;
	std::vector<DescriptorSet*> dSetVector;

public:
	// initialize model and texture
	void init(BaseProject* bp, std::string modelPath, std::string texturePath, DescriptorSetLayout* DSLobj) {
		model.init(bp, MODEL_PATH + modelPath);
		texture.init(bp, TEXTURE_PATH + texturePath);
	}

	// Add a descriptorSet which means a new gameObject of the asset to render
	void addDSet(BaseProject* bp, DescriptorSetLayout* DSLobj, DescriptorSet* dSet) {
		dSetVector.push_back(dSet);
		(*dSet).init(bp, DSLobj, {
		{0, UNIFORM, sizeof(UniformBufferObject), nullptr},
		{1, TEXTURE, 0, &texture}
			});
	}

	// cleanup all the attributes
	void cleanup() {
		for (DescriptorSet* dSet : dSetVector)
		{
			(*dSet).cleanup();
		}
		texture.cleanup();
		model.cleanup();
	}

	// Populate command buffer (vertex, descriptor set, indices)
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage, DescriptorSet DS_global, Pipeline* P1) {
		VkBuffer vertexBuffers[] = { model.vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, model.indexBuffer, 0,
			VK_INDEX_TYPE_UINT32);
		for (DescriptorSet* dSet : dSetVector)
		{
			vkCmdBindDescriptorSets(commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				(*P1).pipelineLayout, 1, 1, &(*dSet).descriptorSets[currentImage],
				0, nullptr);
		}
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(model.indices.size()), 1, 0, 0, 0);
	}
};

class GameObject {
public:
	DescriptorSet dSet;

	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) = 0; //must return ubo

	void updateUniformBuffer(GLFWwindow* window, VkDevice device, int currentImage, void* data, UniformBufferObject ubo) {
		ubo = update(window, ubo);
		vkMapMemory(device, dSet.uniformBuffersMemory[0][currentImage], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, dSet.uniformBuffersMemory[0][currentImage]);
	}
};

class Bird :public GameObject {
protected:
	glm::vec3 startPos = glm::vec3(0.0f);
	glm::vec3 birdPos = glm::vec3(0.0f);
	glm::vec3 birdAng = glm::vec3(0.0f);

	const float ROT_SPEED = 60.0f;

	bool isJumping = false;
	float startJump = 0.0f;
	float deltaT = 0.0f;

	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) override {
		static auto startTime = glfwGetTime();
		static float lastTime = 0.0f;
		auto currentTime = glfwGetTime();
		float time = std::chrono::duration<float, std::chrono::seconds::period>
			(currentTime - startTime).count();
		float deltaT = time - lastTime;
		lastTime = time;

		if (glfwGetKey(window, GLFW_KEY_J) && !isJumping) {
			isJumping = true;
			startJump = glfwGetTime();
		}

		if (glfwGetKey(window, GLFW_KEY_Q) && !isJumping) {
			birdAng.x += ROT_SPEED * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_E) && !isJumping) {
			birdAng.x -= ROT_SPEED * deltaT;
		}

		if (isJumping) {
			jump(10.0f, glm::radians(45.0f), glm::radians(birdAng.x));
		}

		ubo.model = glm::translate(glm::mat4(1.0f), birdPos) * glm::rotate(glm::mat4(1.0f), glm::radians(birdAng.x), glm::vec3(0.0f, 1.0f, 0.0f));
		return ubo;
	}

	void jump(float v0, float angY, float angX) {
		deltaT = glfwGetTime() - startJump;

		birdPos = startPos.x + (v0 * cos(angY)) * deltaT * glm::vec3(glm::rotate(glm::mat4(1.0f), angX,
			glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1));

		birdPos += glm::vec3(0.0f, -(0.5 * 9.8f * pow(deltaT, 2)) + (v0 * sin(angY)) * deltaT + startPos.y, 0.0f);
		if (birdPos.y <= 0.0f) {
			birdPos.y = 0.0f;
			isJumping = false;
		}

	}
};

class BirdBlue :public Bird {

};

class Pig :public GameObject {
	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) override {
		ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		return ubo;
	}
};

class PigStd :public Pig {

};

class Cannon : public GameObject {
	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) override {
		ubo.model = glm::mat4(1.0f);
		return ubo;
	}
};

class Terrain : public GameObject {
	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) override {
		ubo.model = glm::mat4(1.0f);
		return ubo;
	}
};

// MAIN ! 
class MyProject : public BaseProject {
protected:
	// Here you list all the Vulkan objects you need:

	// Descriptor Layouts [what will be passed to the shaders]
	DescriptorSetLayout DSLglobal;
	DescriptorSetLayout DSLobj;

	Camera camera;
	SkyBox skyBox;

	// Pipelines [Shader couples]
	Pipeline P1;

	// Models, textures and Descriptors (values assigned to the uniforms)

	Asset A_BlueBird;
	BirdBlue birdBlue1;

	Asset A_PigStd;
	PigStd pigStd;

	Asset A_Terrain;
	Terrain terrain;

	Asset A_CannonBot;
	Cannon cannonBot;

	Asset A_CannonTop;
	Cannon cannonTop;


	DescriptorSet DS_global;



	// Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 1600;
		windowHeight = 1200;
		windowTitle = "Hungry_Bird";
		initialBackgroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		// Descriptor pool sizes
		uniformBlocksInPool = 7;
		texturesInPool = 6;
		setsInPool = 7;
	}

	// Here you load and setup all your Vulkan objects
	void localInit() {
		// Descriptor Layouts [what will be passed to the shaders]
		DSLobj.init(this, {
			// this array contains the binding:
			// first  element : the binding number
			// second element : the time of element (buffer or texture)
			// third  element : the pipeline stage where it will be used
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
			});
		DSLglobal.init(this, {
		{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
			});

		// Pipelines [Shader couples]
		// The last array, is a vector of pointer to the layouts of the sets that will
		// be used in this pipeline. The first element will be set 0, and so on..
		P1.init(this, "shaders/materialVert.spv", "shaders/materialFrag.spv", { &DSLglobal, &DSLobj });


		// Models, textures and Descriptors (values assigned to the uniforms)
		A_BlueBird.init(this, "/Birds/blues.obj", "/texture.png", &DSLobj);
		A_BlueBird.addDSet(this, &DSLobj, &birdBlue1.dSet);


		A_PigStd.init(this, "/Pigs/pig.obj", "/texture.png", &DSLobj);
		A_PigStd.addDSet(this, &DSLobj, &pigStd.dSet);

		A_Terrain.init(this, "/Terrain/Terrain.obj", "/Terrain/terrain.png", &DSLobj);
		A_Terrain.addDSet(this, &DSLobj, &terrain.dSet);

		A_CannonBot.init(this, "/Cannon/BotCannon.obj", "/Cannon/map_CP_001.001_BaseColorRedBird.png", &DSLobj);
		A_CannonBot.addDSet(this, &DSLobj, &cannonBot.dSet);

		A_CannonTop.init(this, "/Cannon/TopCannon.obj", "/Cannon/map_CP_001.001_BaseColorRedBird.png", &DSLobj);
		A_CannonTop.addDSet(this, &DSLobj, &cannonTop.dSet);


		skyBox.init(this, DSLobj, DSLglobal);


		DS_global.init(this, &DSLglobal, {
		{0, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr},
			});


	}

	// Here you destroy all the objects you created!		
	void localCleanup() {

		A_BlueBird.cleanup();

		A_PigStd.cleanup();

		A_Terrain.cleanup();

		A_CannonBot.cleanup();

		A_CannonTop.cleanup();

		skyBox.cleanup();

		P1.cleanup();


		DS_global.cleanup();

		DSLglobal.cleanup();
		DSLobj.cleanup();
	}

	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw,
	// with their buffers and textures
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {

		// --------------------- SKYBOX -------------------------

		skyBox.populateCommandBuffer(commandBuffer, currentImage, DS_global);


		// -------------------- Pipeline 1 -----------------------------


		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.graphicsPipeline);

		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.pipelineLayout, 0, 1, &DS_global.descriptorSets[currentImage],
			0, nullptr);


		// ---------------------- BIRD BLUES ------------

		A_BlueBird.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);

		// ------------------------ PIG --------------------

		A_PigStd.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);

		// ------------------------ Terrain -----------------

		 A_Terrain.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);

		 // ----------------------- Cannon -------------------

		 A_CannonBot.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		 A_CannonTop.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);

	}

	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();
		static float lastTime = 0.0f;
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>
			(currentTime - startTime).count();
		float deltaT = time - lastTime;
		lastTime = time;

		UniformBufferObject ubo{};
		GlobalUniformBufferObject gubo{};

		void* data;

		gubo.view = camera.update(window, deltaT);
		gubo.proj = glm::perspective(glm::radians(45.0f),
			swapChainExtent.width / (float)swapChainExtent.height,
			0.1f, 200.0f);
		gubo.proj[1][1] *= -1;


		// global
		vkMapMemory(device, DS_global.uniformBuffersMemory[0][currentImage], 0,
			sizeof(gubo), 0, &data);
		memcpy(data, &gubo, sizeof(gubo));
		vkUnmapMemory(device, DS_global.uniformBuffersMemory[0][currentImage]);


		// SkyBox

		skyBox.updateUniformBuffer(device, currentImage, data, ubo);


		// Here is where you actually update your uniforms

		// ------------------------ BIRD BLUES ---------------------------

		birdBlue1.updateUniformBuffer(window, device, currentImage, data, ubo);

		// ------------------------ PIG ---------------------------

		pigStd.updateUniformBuffer(window, device, currentImage, data, ubo);

		//----------------------- TERRAIN --------------------------

		terrain.updateUniformBuffer(window, device, currentImage, data, ubo);

		// --------------------- Cannon ----------------------------

		cannonBot.updateUniformBuffer(window, device, currentImage, data, ubo);
		cannonTop.updateUniformBuffer(window, device, currentImage, data, ubo);
	}
};

// This is the main: probably you do not need to touch this!
int main() {
	MyProject app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}