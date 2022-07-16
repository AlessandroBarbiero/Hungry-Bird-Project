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


// MAIN ! 
class MyProject : public BaseProject {
	protected:
	// Here you list all the Vulkan objects you need:
	
	// Descriptor Layouts [what will be passed to the shaders]
	DescriptorSetLayout DSLglobal;
	DescriptorSetLayout DSLobj;

	// Pipelines [Shader couples]
	Pipeline P1;

	// Models, textures and Descriptors (values assigned to the uniforms)
	Model M_blues;
	Texture T_blues;
	DescriptorSet DS_blues;

	Model M_pig;
	Texture T_pig;
	DescriptorSet DS_pig;

	DescriptorSet DS_global;

	// Camera
	glm::vec3 CamPos = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 CamAng = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::mat4 CamDir;
	

	
	// Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "Hungry_Bird";
		initialBackgroundColor = {0.0f, 0.0f, 0.0f, 1.0f};
		
		// Descriptor pool sizes
		uniformBlocksInPool = 3;
		texturesInPool = 2;
		setsInPool = 3;
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
		M_blues.init(this, MODEL_PATH + "/Birds/blues.obj");
		T_blues.init(this, TEXTURE_PATH + "/texture.png");
		DS_blues.init(this, &DSLobj, {
		// the second parameter, is a pointer to the Uniform Set Layout of this set
		// the last parameter is an array, with one element per binding of the set.
		// first  elmenet : the binding number
		// second element : UNIFORM or TEXTURE (an enum) depending on the type
		// third  element : only for UNIFORMs, the size of the corresponding C++ object
		// fourth element : only for TEXTUREs, the pointer to the corresponding texture object
					{0, UNIFORM, sizeof(UniformBufferObject), nullptr},
					{1, TEXTURE, 0, &T_blues}
				});

		M_pig.init(this, MODEL_PATH + "/Pigs/pig.obj");
		T_pig.init(this, TEXTURE_PATH + "/texture.png");
		DS_pig.init(this, &DSLobj, {
						{0, UNIFORM, sizeof(UniformBufferObject), nullptr},
						{1, TEXTURE, 0, &T_pig}
			});



		DS_global.init(this, &DSLglobal, {
						{0, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr},
			});


	}

	// Here you destroy all the objects you created!		
	void localCleanup() {
		DS_blues.cleanup();
		T_blues.cleanup();
		M_blues.cleanup();

		DS_pig.cleanup();
		T_pig.cleanup();
		M_pig.cleanup();

		P1.cleanup();

		DS_global.cleanup();

		DSLglobal.cleanup();
		DSLobj.cleanup();
	}
	
	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw,
	// with their buffers and textures
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
				
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				P1.graphicsPipeline);

		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.pipelineLayout, 0, 1, &DS_global.descriptorSets[currentImage],
			0, nullptr);
				

		// ---------------------- BIRD BLUES ------------

		VkBuffer vertexBuffers_blues[] = {M_blues.vertexBuffer};
		// property .vertexBuffer of models, contains the VkBuffer handle to its vertex buffer
		VkDeviceSize offsets_blues[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers_blues, offsets_blues);
		// property .indexBuffer of models, contains the VkBuffer handle to its index buffer
		vkCmdBindIndexBuffer(commandBuffer, M_blues.indexBuffer, 0,
								VK_INDEX_TYPE_UINT32);

		// property .pipelineLayout of a pipeline contains its layout.
		// property .descriptorSets of a descriptor set contains its elements.
		vkCmdBindDescriptorSets(commandBuffer,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						P1.pipelineLayout, 1, 1, &DS_blues.descriptorSets[currentImage],
						0, nullptr);
						
		// property .indices.size() of models, contains the number of triangles * 3 of the mesh.
		vkCmdDrawIndexed(commandBuffer,
					static_cast<uint32_t>(M_blues.indices.size()), 1, 0, 0, 0);

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
	
		const float ROT_SPEED = glm::radians(60.0f);
		const float MOVE_SPEED = 1.75f;
		
		
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
		
		if(glfwGetKey(window, GLFW_KEY_A)) {
			CamPos -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) *  deltaT;
		}
		if(glfwGetKey(window, GLFW_KEY_D)) {
			CamPos += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) *  deltaT;
		}
		if(glfwGetKey(window, GLFW_KEY_S)) {
			CamPos += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
		}
		if(glfwGetKey(window, GLFW_KEY_W)) {
			CamPos -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
		}
		if(glfwGetKey(window, GLFW_KEY_F)) {
			CamPos -= MOVE_SPEED * glm::vec3(0,1,0) * deltaT;
		}
		if(glfwGetKey(window, GLFW_KEY_R)) {
			CamPos += MOVE_SPEED * glm::vec3(0,1,0) * deltaT;
		}

		CamDir = glm::translate(glm::transpose(glm::mat4(CamEye)), -CamPos);

		
		gubo.view = CamDir;
		gubo.proj = glm::perspective(glm::radians(45.0f),
			swapChainExtent.width / (float)swapChainExtent.height,
			0.1f, 10.0f);
		gubo.proj[1][1] *= -1;


		// global
		vkMapMemory(device, DS_global.uniformBuffersMemory[0][currentImage], 0,
			sizeof(gubo), 0, &data);
		memcpy(data, &gubo, sizeof(gubo));
		vkUnmapMemory(device, DS_global.uniformBuffersMemory[0][currentImage]);




		// Here is where you actually update your uniforms

		// ------------------------ BIRD BLUES ---------------------------
		static bool isJumping = false;
		static bool isDescending = false;
		static float startJump = 0.0f;
		static float deltaTime_Jump = 0.0f;

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


		ubo.model = birdPos;
		
		vkMapMemory(device, DS_blues.uniformBuffersMemory[0][currentImage], 0,
							sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, DS_blues.uniformBuffersMemory[0][currentImage]);

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