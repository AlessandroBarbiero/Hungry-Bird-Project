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
	void init(BaseProject *bp, DescriptorSetLayout DSLobj, DescriptorSetLayout DSLglobal) {
		P_SkyBox.init(bp, "shaders/skyBoxVert.spv", "shaders/skyBoxFrag.spv", { &DSLglobal, &DSLobj });
		M_skyBox.init(bp, MODEL_PATH + "/SkyBox/SkyBox.obj");
		T_skyBox.init(bp, TEXTURE_PATH + "/SkyBox/SkyBox.png");
		DS_skyBox.init(bp, &DSLobj, {
						{0, UNIFORM, sizeof(UniformBufferObject), nullptr},
						{1, TEXTURE, 0, &T_skyBox}
			});
	}

	void cleanup() {
		DS_skyBox.cleanup();
		T_skyBox.cleanup();
		M_skyBox.cleanup();
		P_SkyBox.cleanup();
	}

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

	UniformBufferObject update(UniformBufferObject ubo) {
		ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(50.0f, 50.0f, 50.0f));
		return ubo;
	}
	
	void updateUniformBuffer(VkDevice device, int currentImage, void *data, UniformBufferObject ubo){
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
	glm::mat4 update(GLFWwindow *window, float deltaT) {
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
		if (glfwGetKey(window, GLFW_KEY_Q)) {
			CamAng.z += deltaT * ROT_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_E)) {
			CamAng.z -= deltaT * ROT_SPEED;
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
	std::vector<DescriptorSet *> dSetVector;

	public:
	void init(BaseProject *bp, std::string modelPath, std::string texturePath, DescriptorSetLayout *DSLobj) {
		model.init(bp, MODEL_PATH + modelPath);
		texture.init(bp, TEXTURE_PATH + texturePath);
	}

	void addDSet(BaseProject *bp, DescriptorSetLayout *DSLobj, DescriptorSet *dSet) {
		dSetVector.push_back(dSet);
		(*dSet).init(bp, DSLobj, {
						{0, UNIFORM, sizeof(UniformBufferObject), nullptr},
						{1, TEXTURE, 0, &texture}
			});
	}

	void cleanup() {
		for (DescriptorSet *dSet : dSetVector)
		{
			(* dSet).cleanup();
		}
		texture.cleanup();
		model.cleanup();
	}

	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage, DescriptorSet DS_global, Pipeline *P1) {
		VkBuffer vertexBuffers[] = { model.vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, model.indexBuffer, 0,
			VK_INDEX_TYPE_UINT32);
		for (DescriptorSet *dSet : dSetVector)
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

		virtual UniformBufferObject update(UniformBufferObject ubo)=0; //must return ubo

		void updateUniformBuffer(VkDevice device, int currentImage, void* data, UniformBufferObject ubo) {
			ubo = update(ubo);
			vkMapMemory(device, dSet.uniformBuffersMemory[0][currentImage], 0,sizeof(ubo), 0, &data);
			memcpy(data, &ubo, sizeof(ubo));
			vkUnmapMemory(device, dSet.uniformBuffersMemory[0][currentImage]);
		}
};

class Bird:public GameObject {

};

class BirdBlue:public Bird{
	public:
	virtual UniformBufferObject update(UniformBufferObject ubo) override {
		ubo.model = glm::mat4(1.0f);

		return ubo;
	}

	/*
	UniformBufferObject jump(UniformBufferObject ubo) {
		bool isJumping = false;
		bool isDescending = false;
		float startJump = 0.0f;
		float deltaTime_Jump = 0.0f;

		float height = 2.0f;
		float currentHeight = 0.0f;

		static glm::mat4 birdPos = glm::mat4(1.0f);

		if (glfwGetKey(window, GLFW_KEY_J)) {
			if (!isJumping) {
				startJump = time;
				deltaTime_Jump = 0.0f;
				isJumping = true;
			}
		}
		if (isJumping)
		{
			deltaTime_Jump = time - startJump;
			if (!isDescending) {
				currentHeight = deltaTime_Jump * 1.0f;
				birdPos = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, currentHeight, 0.0f));
				if (currentHeight > height) {
					startJump = time;
					isDescending = true;
				}
			}
			else
			{
				currentHeight = height - deltaTime_Jump * 1.0f;
				birdPos = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, currentHeight, 0.0f));
				if (currentHeight <= 0) {
					isJumping = false;
					isDescending = false;
				}
			}
		}
	}
	*/

};

class Pig :public GameObject {

};

class PigStd:public Pig{
	virtual UniformBufferObject update(UniformBufferObject ubo) override {
		//what pig should do 

		return ubo;
	}
};

class Cannon :GameObject {

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


	Model M_pig;
	Texture T_pig;
	DescriptorSet DS_pig;


	DescriptorSet DS_global;
	

	
	// Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "Hungry_Bird";
		initialBackgroundColor = {0.0f, 0.0f, 0.0f, 1.0f};
		
		// Descriptor pool sizes
		uniformBlocksInPool = 4;
		texturesInPool = 3;
		setsInPool = 4;
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
		P1.init(this, "shaders/materialVert.spv", "shaders/materialFrag.spv", {&DSLglobal, &DSLobj});
		

		// Models, textures and Descriptors (values assigned to the uniforms)
		A_BlueBird.init(this, "/Birds/blues.obj", "/texture.png", &DSLobj);
		A_BlueBird.addDSet(this, &DSLobj, &birdBlue1.dSet);

		M_pig.init(this, MODEL_PATH + "/Pigs/pig.obj");
		T_pig.init(this, TEXTURE_PATH + "/texture.png");
		
		DS_pig.init(this, &DSLobj, {
						{0, UNIFORM, sizeof(UniformBufferObject), nullptr},
						{1, TEXTURE, 0, &T_pig}
			});


		skyBox.init(this, DSLobj, DSLglobal);


		DS_global.init(this, &DSLglobal, {
						{0, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr},
			});


	}

	// Here you destroy all the objects you created!		
	void localCleanup() {

		A_BlueBird.cleanup();

		DS_pig.cleanup();
		T_pig.cleanup();
		M_pig.cleanup();

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

		VkBuffer vertexBuffers_pig[] = { M_pig.vertexBuffer };
		VkDeviceSize offsets_pig[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers_pig, offsets_pig);
		vkCmdBindIndexBuffer(commandBuffer, M_pig.indexBuffer, 0,
			VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.pipelineLayout, 1, 1, &DS_pig.descriptorSets[currentImage],
			0, nullptr);
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(M_pig.indices.size()), 1, 0, 0, 0);




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
		
		skyBox.updateUniformBuffer(device, currentImage, data ,ubo);


		// Here is where you actually update your uniforms

		// ------------------------ BIRD BLUES ---------------------------

		birdBlue1.updateUniformBuffer(device, currentImage, data, ubo);

		// ------------------------ PIG ---------------------------
		ubo.model = glm::translate(glm::mat4(1.0f),glm::vec3(0.5f,0.0f,0.0f));
		vkMapMemory(device, DS_pig.uniformBuffersMemory[0][currentImage], 0,
			sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, DS_pig.uniformBuffersMemory[0][currentImage]);



	}	
};

// This is the main: probably you do not need to touch this!
int main() {
    MyProject app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}