// This has been adapted from the Vulkan tutorial

#include "MyProject.hpp"

const std::string MODEL_PATH = "Assets/models";
const std::string TEXTURE_PATH = "Assets/textures";
const std::string HITBOXDEC_PATH = "Assets/models/HitBoxDecorations";

bool cameraON = true;
const glm::vec3 CANNON_BOT_POS = glm::vec3(-0.45377f, 8.78275f, -3.0006f);
const glm::vec3 CANNON_TOP_POS = glm::vec3(-0.45377f, 9.50215f, -3.0006f);

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
enum GameController {
	CameraMovement,
	CannonMovement
};
GameController controller = CannonMovement;

// The global buffer object used for view and proj
struct GlobalUniformBufferObject {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

// The uniform buffer object used for models
struct UniformBufferObject {
	alignas(16) glm::mat4 model;
};

class GameTime
{

	/**
	 * The Singleton's constructor should always be private to prevent direct
	 * construction calls with the `new` operator.
	 */

protected:
	GameTime()
	{}

	static GameTime* singleton_;
	float deltaT;
	float time;

public:

	GameTime(GameTime& other) = delete;

	void operator=(const GameTime&) = delete;

	static GameTime* GetInstance();

	void setTime() {
		static auto startTime = std::chrono::high_resolution_clock::now();
		static float lastTime = 0.0f;
		auto currentTime = std::chrono::high_resolution_clock::now();
		time = std::chrono::duration<float, std::chrono::seconds::period>
			(currentTime - startTime).count();
		deltaT = time - lastTime;
		lastTime = time;
	}

	float getDelta() {
		return deltaT;
	}

	float getTime() {
		return time;
	}

};

GameTime* GameTime::singleton_ = nullptr;;

GameTime* GameTime::GetInstance()
{
	if (singleton_ == nullptr) {
		singleton_ = new GameTime();
	}
	return singleton_;
}

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

	const int MAX_VIEW = 4;
	const float ROT_SPEED = glm::radians(60.0f);
	const float MOVE_SPEED = 1.75f;

	int currView = 0;

public:
	// update the camera position
	glm::mat4 update(GLFWwindow* window) {
		float deltaT = GameTime::GetInstance()->getDelta();
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

		if (controller==CameraMovement) {
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
		}

		return CamDir = glm::translate(glm::transpose(glm::mat4(CamEye)), -CamPos);
	}

	void NextView() {
		currView = (currView % MAX_VIEW) + 1;
		ChangePositionAndAngular();
	}

	void SetView(int view) {
		currView = view;
		ChangePositionAndAngular();
	}

	void ShowStat() {
		std::cout << "Valori pos: " << CamPos.x << " " << CamPos.y << " " << CamPos.z << std::endl;
		std::cout << "Valori rot: " << CamAng.x << " " << CamAng.y << " " << CamAng.z << std::endl;
	}

	private:
	void ChangePositionAndAngular() {
		switch (currView)
		{
		case 1:
			CamPos = glm::vec3(-0.535644f, 15.9563f, -12.8586f);
			CamAng = glm::vec3(-0.27987f, 3.2264f, 0.0f);
			break;
		case 2:
			CamPos = glm::vec3(-7.24489f, 11.9762f, -5.34424f);
			CamAng = glm::vec3(-0.189157f, 3.90985f, 0.0f);
			break;
		case 3:
			CamPos = glm::vec3(2.14082f, 10.2845f, -6.47336f);
			CamAng = glm::vec3(-0.0441803f, 2.72603f, 0.0f);
			break;
		case 4:
			CamPos = glm::vec3(28.9848f, 27.9113f, -3.16643f);
			CamAng = glm::vec3(-0.459889f, 2.18553f, 0.0f);
			break;
		default:
			CamPos = glm::vec3(0.0f, 0.0f, 0.0f);
			CamAng = glm::vec3(0.0f, 0.0f, 0.0f);
			break;
		}
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
			vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(model.indices.size()), 1, 0, 0, 0);
		}
	}
};

class GameObject {
public:
	DescriptorSet dSet;

	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) = 0;

	void updateUniformBuffer(GLFWwindow* window, VkDevice device, int currentImage, void* data, UniformBufferObject ubo) {
		ubo = update(window, ubo);
		vkMapMemory(device, dSet.uniformBuffersMemory[0][currentImage], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, dSet.uniformBuffersMemory[0][currentImage]);
	}
};

class Decoration: public GameObject {
	std::vector<std::string> HitBoxObjs;
	std::vector < std::array<std::vector<float>,3>> hitBoxes;

public:
	void setHitBoxes(std::vector<std::string> HitBoxPaths) {
		for (std::string path : HitBoxPaths)
		{
			HitBoxObjs.push_back(path);
		}
		loadHitBoxes();
	}

	void loadHitBoxes() {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		for (std::string HitBox : HitBoxObjs)
		{
			if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
				HitBox.c_str())) {
				throw std::runtime_error(warn + err);
			}

			std::set<float> x, y, z;

			for (int i = 0; i < attrib.vertices.size(); i = i + 3)
			{
				x.insert(attrib.vertices[i + 0]);
				y.insert(attrib.vertices[i + 1]);
				z.insert(attrib.vertices[i + 2]);
			}

			std::array<std::vector<float>,3>hitBox;

			hitBox[0] = std::vector<float>(x.begin(), x.end());
			hitBox[1] = std::vector<float>(y.begin(), y.end());
			hitBox[2] = std::vector<float>(z.begin(), z.end());

			hitBoxes.push_back(hitBox);

			std::cout << "loaded hit terrain\n";
		}
	}

	std::vector <std::vector<glm::vec2>> getHitBox() {
		std::vector <std::vector<glm::vec2>> boxes;
		for (auto box : hitBoxes) {
			std::vector<glm::vec2> objHit;
			objHit.push_back(glm::vec2(box[0][0], box[0][1]));
			objHit.push_back(glm::vec2(box[1][0], box[1][1]));
			objHit.push_back(glm::vec2(box[2][0], box[2][1]));
			boxes.push_back(objHit);
		}
		return boxes;
	}
	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) override {
		ubo.model = glm::mat4(1.0f);
		return ubo;
	}
};

class Bird :public GameObject {
protected:
	glm::vec3 startPos = glm::vec3(0.0f);
	float shootAng = 0.0f;
	
	glm::vec3 birdPos = glm::vec3(0.0f);
	glm::vec3 birdAng = glm::vec3(0.0f);

	const float ROT_SPEED = 60.0f;

	bool isActive = false;

	bool isJumping = false;
	float v0;
	float startJumpTime = 0.0f;
	float deltaT = 0.0f;

	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) override {
		if (isJumping) {
			jump(v0, -shootAng, birdAng.x);
		}
		ubo.model = glm::translate(glm::mat4(1.0f), birdPos) * glm::rotate(glm::mat4(1.0f), glm::radians(birdAng.x), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(birdAng.y), glm::vec3(1.0f, 0.0f, 0.0f));
		return ubo;
	}

	void jump(float v0, float angY, float angX) {
		deltaT = glfwGetTime() - startJumpTime;

		birdPos.x = startPos.x + (v0 * cos(glm::radians(angY))) * deltaT * sin(glm::radians(angX));
		birdPos.z = startPos.z + (v0 * cos(glm::radians(angY))) * deltaT * cos(glm::radians(angX));
		birdPos.y = -(0.5 * 9.8f * pow(deltaT, 2)) + (v0 * sin(glm::radians(angY))) * deltaT + startPos.y;

		glm::vec3 a = glm::vec3(v0 * cos(glm::radians(angY)), -(9.8f * deltaT) + (v0 * sin(glm::radians(angY))), 0.0f);
		glm::vec3 b = glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3 origin = glm::vec3(0.0f, 0.0f, 0.0f);

		glm::vec3 da = glm::normalize(a - origin);
		glm::vec3 db = glm::normalize(b - origin);
		float newAngY = glm::degrees(glm::acos(glm::dot(da, db)));
		if (a.y > 0) {
			newAngY = -newAngY;
		}

		birdAng.y = newAngY;
		

		if (birdPos.y <= 0.0f) {
			birdPos.y = 0.0f;
			isJumping = false;
		}
	}

	public:
		std::string HitBoxObj;
		std::vector<float> hitBox[3];

	void startJump(float v0, float angY, float angX) {
		this->v0 = v0;
		this->birdAng.x = angX;
		this->birdAng.y = angY;
		this->shootAng = angY;
		this->isJumping = true;
		this->startJumpTime = glfwGetTime();
	}

	void showStat(int i) {
			std::cout << "----- Bird in " << i << "----- " << std::endl;
			std::cout << "Active: " << isActive << std::endl;
			std::cout << "Position: " << birdPos.x <<" " << birdPos.y << " " << birdPos.z << std::endl;
			std::cout << "-----------------------------" << std::endl;
	}

	void setActive() {
		isActive = true;
		startPos = CANNON_TOP_POS;
		birdPos = CANNON_TOP_POS;
	}

	void setHitBox(std::string HitBoxPath) {
		HitBoxObj = HitBoxPath;
		loadHitBox();
	}

	void loadHitBox() {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
			HitBoxObj.c_str())) {
			throw std::runtime_error(warn + err);
		}

		std::set<float> x, y, z;

		for (int i = 0; i < attrib.vertices.size(); i = i + 3)
		{
			x.insert(attrib.vertices[i + 0]);
			y.insert(attrib.vertices[i + 1]);
			z.insert(attrib.vertices[i + 2]);
		}

		hitBox[0] = std::vector<float>(x.begin(), x.end());
		hitBox[1] = std::vector<float>(y.begin(), y.end());
		hitBox[2] = std::vector<float>(z.begin(), z.end());

		std::cout << "bird hit box loaded";
	}

	std::vector<glm::vec2> getHitBox() {
		std::vector<glm::vec2> box;
		box.push_back(glm::vec2(birdPos.x - abs(hitBox[0][0]), birdPos.x + abs(hitBox[0][1])));
		box.push_back(glm::vec2(birdPos.y - abs(hitBox[1][0]), birdPos.y + abs(hitBox[1][1])));
		box.push_back(glm::vec2(birdPos.z - abs(hitBox[2][0]), birdPos.z + abs(hitBox[2][1])));
		return box;
	}
};

class BirdBlue :public Bird {

};

class BirdYellow : public Bird {
};

class Pig :public GameObject {

public:
	std::string HitBoxObj;
	std::vector<float> hitBox[3];



	void setHitBox(std::string HitBoxPath) {
		HitBoxObj = HitBoxPath;
		loadHitBox();
	}

	void loadHitBox() {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
			HitBoxObj.c_str())) {
			throw std::runtime_error(warn + err);
		}

		std::set<float> x, y, z;

		for (int i = 0; i < attrib.vertices.size(); i = i+3)
		{
			x.insert(attrib.vertices[i + 0]);
			y.insert(attrib.vertices[i + 1]);
			z.insert(attrib.vertices[i + 2]);
		}

		hitBox[0] = std::vector<float>(x.begin(), x.end());
		hitBox[1] = std::vector<float>(y.begin(), y.end());
		hitBox[2] = std::vector<float>(z.begin(), z.end());
	}

	std::vector<glm::vec2> getHitBox() {
		std::vector<glm::vec2> box;
		box.push_back(glm::vec2(hitBox[0][0], hitBox[0][1]));
		box.push_back(glm::vec2(hitBox[1][0], hitBox[1][1]));
		box.push_back(glm::vec2(hitBox[2][0], hitBox[2][1]));
		return box;
	}

	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) override {
		ubo.model = glm::mat4(1.0f);
		return ubo;
	}
};

class WhiteSphere : public GameObject {
	protected:
	glm::vec3 spherePos;

	public:
	void setBlockPos(glm::vec3 pos) {
		spherePos = pos;
	}

	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) override {
		//tutto fermo
		ubo.model = glm::translate(glm::mat4(1.0f), spherePos);
		return ubo;
	}
};

class CannonBot : public GameObject {
	protected:
	glm::vec3 cannonPos = CANNON_BOT_POS;
	glm::vec3 cannonAng = glm::vec3(0.0f);

	const float ROT_SPEED = 60.0f;

	public: 
	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) override {
		float deltaT = GameTime::GetInstance()->getDelta();
		if (controller==CannonMovement) {
			if (glfwGetKey(window, GLFW_KEY_A)) {
				cannonAng.x += ROT_SPEED * deltaT;
			}
			if (glfwGetKey(window, GLFW_KEY_D)) {
				cannonAng.x -= ROT_SPEED * deltaT;
			}
		}
		ubo.model = glm::rotate(glm::translate(glm::mat4(1.0f), cannonPos), glm::radians(cannonAng.x), glm::vec3(0.0f, 1.0f, 0.0f));
		return ubo;
	}
};

class CannonTop : public GameObject {
	protected:
	glm::vec3 cannonPos = CANNON_TOP_POS;
	glm::vec3 cannonAng = glm::vec3(0.0f);

	float v0 = 10.0f;

	Bird *bird;
	std::vector<WhiteSphere*> *trajectory;

	const float ROT_SPEED = 60.0f;
	const float POWER = 10.0f;

	public:
	void setBird(Bird *birdToLoad) {
		bird = birdToLoad;
	}

	void setTrajectory(std::vector<WhiteSphere*>* trajectoryBlocks) {
		trajectory = trajectoryBlocks;
	}

	void shoot() {
		bird->startJump(v0, cannonAng.y, cannonAng.x);
	}

	void computeTrajectory() {
		float a = v0 * sin(glm::radians(-cannonAng.y));
		float b = pow(v0, 2) * pow(sin(glm::radians(-cannonAng.y)), 2) + 2 * 9.8f * CANNON_TOP_POS.y;
		float time = (a + sqrt(b)) / 9.8f;
		
		float dTime = time / trajectory->size();
		for (int i = 0; i < trajectory->size(); i++)
		{
			trajectory->at(i)->setBlockPos(glm::vec3(CANNON_TOP_POS.x + (v0 * cos(glm::radians(-cannonAng.y))) * dTime * i * sin(glm::radians(cannonAng.x)),
				-(0.5 * 9.8f * pow(dTime * i, 2)) + (v0 * sin(glm::radians(-cannonAng.y))) * dTime * i + CANNON_TOP_POS.y,
				CANNON_TOP_POS.z + (v0 * cos(glm::radians(-cannonAng.y))) * dTime * i * cos(glm::radians(cannonAng.x))));
		}
	}

	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) override {
		float deltaT = GameTime::GetInstance()->getDelta();
		if (controller==CannonMovement) {
			if (glfwGetKey(window, GLFW_KEY_A)) {
				cannonAng.x += ROT_SPEED * deltaT;
				computeTrajectory();
			}
			if (glfwGetKey(window, GLFW_KEY_D)) {
				cannonAng.x -= ROT_SPEED * deltaT;
				computeTrajectory();
			}
			if (glfwGetKey(window, GLFW_KEY_S)) {
				if (cannonAng.y < 25.5746f) {
					cannonAng.y += ROT_SPEED * deltaT;
				}
				else {
					cannonAng.y = 25.5746;
				}
				computeTrajectory();
			}
			if (glfwGetKey(window, GLFW_KEY_W)) {
				if (cannonAng.y > -90.0f) {
					cannonAng.y -= ROT_SPEED * deltaT;
				}
				else {
					cannonAng.y = -90.0f;
				}
				computeTrajectory();
			}
			if (glfwGetKey(window, GLFW_KEY_Q)) {
				if (v0 > 0) {
					v0 -= POWER * deltaT;
				}
				else {
					v0 = 0;
				}
				computeTrajectory();
			}
			if (glfwGetKey(window, GLFW_KEY_E)) {
				v0 += POWER * deltaT;
				computeTrajectory();
			}
		}
		ubo.model = glm::rotate(glm::rotate(glm::translate(glm::mat4(1.0f), cannonPos), glm::radians(cannonAng.x), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(cannonAng.y), glm::vec3(1.0f, 0.0f, 0.0f));
		return ubo;
	}	
};


CannonTop *cTop;
Camera* cCamera;
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
	BirdBlue bird1, bird2, bird3;

	std::vector<Bird *> birds;
	int birdInCannon = 0;

	//--------------PIGS
	Asset A_PigStd;
	Pig pigStd;

	Asset A_PigHelmet;
	Pig pigBaloon;

	Asset A_PigKingHouse;
	Pig pigHouse;
	
	Asset A_PigKingShip;
	Pig pigShip;

	Asset A_PigStache;
	Pig pigCitySky;

	Asset A_PigMechanics;
	Pig pigShipMini;

	//------OTHERS
	Asset A_CannonBot;
	CannonBot cannonBot;

	Asset A_CannonTop;
	CannonTop cannonTop;

	Asset A_Sphere;
	WhiteSphere sphere0, sphere1, sphere2, sphere3, sphere4, sphere5, sphere6, sphere7, sphere8, sphere9;
	std::vector<WhiteSphere *> trajectorySpheres;


	//Decorations Assets and GO
	Asset A_Terrain;
	Decoration terrain;

	Asset A_TowerSiege;
	Decoration towerSiege;

	Asset A_Baloon;
	Decoration baloon;

	Asset A_SeaCity25;
	Decoration seaCity25;

	Asset A_SeaCity37;
	Decoration seaCity37;

	Asset A_ShipSmall;
	Decoration shipSmall;

	Asset A_ShipVikings;
	Decoration shipVikings;

	Asset A_SkyCity;
	Decoration skyCity;

	DescriptorSet DS_global;

	std::vector<Pig*> pigsHitBox;
	std::vector<Decoration*> decorHitBox;

	// Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 1600;
		windowHeight = 1200;
		windowTitle = "Hungry_Bird";
		initialBackgroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		// Descriptor pool sizes
		uniformBlocksInPool = 200;	//10
		texturesInPool = 200;		//9
		setsInPool = 200;			//10
	}

	void setGameState() {
		//set up callback for input
		cTop = &cannonTop;
		cCamera = &camera;

		glfwSetKeyCallback(window, keyCallback);

		cCamera->NextView();

		// -------------- BIRDS
		birds.push_back(&bird1);
		birds.push_back(&bird2);
		birds.push_back(&bird3);

		birdInCannon = 0;
		birds.at(birdInCannon)->setActive();
		cannonTop.setBird(birds.at(birdInCannon));

		// ------------ Trajectory
		trajectorySpheres.push_back(&sphere0);
		trajectorySpheres.push_back(&sphere1);
		trajectorySpheres.push_back(&sphere2);
		trajectorySpheres.push_back(&sphere3);
		trajectorySpheres.push_back(&sphere4);
		trajectorySpheres.push_back(&sphere5);
		trajectorySpheres.push_back(&sphere6);
		trajectorySpheres.push_back(&sphere7);
		trajectorySpheres.push_back(&sphere8);
		trajectorySpheres.push_back(&sphere9);
		cannonTop.setTrajectory(&trajectorySpheres);
		cannonTop.computeTrajectory();
	}

	void loadHitBoxes() {
		pigsHitBox.push_back(&pigStd);
		pigsHitBox.push_back(&pigBaloon);
		pigsHitBox.push_back(&pigHouse);
		pigsHitBox.push_back(&pigShip);
		pigsHitBox.push_back(&pigCitySky);
		pigsHitBox.push_back(&pigShipMini);

		pigStd.setHitBox(MODEL_PATH + "/PigCustom/PigStandardHB.obj");

		pigBaloon.setHitBox(MODEL_PATH + "/PigCustom/PigHelmetHB.obj");

		pigHouse.setHitBox(MODEL_PATH + "/PigCustom/PigKingHouseHB.obj");

		pigShip.setHitBox(MODEL_PATH + "/PigCustom/PigKingBoatHB.obj");

		pigCitySky.setHitBox(MODEL_PATH + "/PigCustom/PigStacheHB.obj");

		pigShipMini.setHitBox(MODEL_PATH + "/PigCustom/PigMechanicHB.obj");

		bird1.setHitBox(MODEL_PATH + "/Birds/bluesHitBox.obj");

		// --------------------- MAP
		decorHitBox.push_back(&terrain);

		std::vector<std::string> terrainHitBoxes;
		terrainHitBoxes.push_back(HITBOXDEC_PATH + "/Sea.obj");
		terrainHitBoxes.push_back(HITBOXDEC_PATH + "/Rock1.obj");
		terrainHitBoxes.push_back(HITBOXDEC_PATH + "/Rock2.obj");
		terrainHitBoxes.push_back(HITBOXDEC_PATH + "/Grass.obj");
		terrain.setHitBoxes(terrainHitBoxes);

		decorHitBox.push_back(&towerSiege);
		std::vector<std::string> towerHitBoxes;
		towerHitBoxes.push_back(HITBOXDEC_PATH + "/TowerBody.obj");
		towerHitBoxes.push_back(HITBOXDEC_PATH + "/TowerPlatform.obj");
		towerHitBoxes.push_back(HITBOXDEC_PATH + "/TowerRoof.obj");
		towerSiege.setHitBoxes(towerHitBoxes);

		decorHitBox.push_back(&seaCity25);
		std::vector<std::string> cityHitBox;
		cityHitBox.push_back(HITBOXDEC_PATH + "/HouseBot.obj");
		cityHitBox.push_back(HITBOXDEC_PATH + "/HouseTop.obj");
		seaCity25.setHitBoxes(cityHitBox);

		decorHitBox.push_back(&skyCity);
		std::vector<std::string> skyCityHitBoxes;
		skyCityHitBoxes.push_back(HITBOXDEC_PATH + "/SkyCityMid.obj");
		skyCityHitBoxes.push_back(HITBOXDEC_PATH + "/SkyCityTop.obj");
		skyCityHitBoxes.push_back(HITBOXDEC_PATH + "/SkyCityBot.obj");
		skyCity.setHitBoxes(skyCityHitBoxes);

		decorHitBox.push_back(&baloon);
		std::vector<std::string> baloonHitBoxs;
		baloonHitBoxs.push_back(HITBOXDEC_PATH + "/BaloonBot.obj");
		baloonHitBoxs.push_back(HITBOXDEC_PATH + "/BaloonMid.obj");
		baloonHitBoxs.push_back(HITBOXDEC_PATH + "/BaloonTop.obj");
		baloon.setHitBoxes(baloonHitBoxs);

		decorHitBox.push_back(&shipSmall);
		std::vector<std::string> shipSmallHitBox;
		shipSmallHitBox.push_back(HITBOXDEC_PATH + "/BoatMini.obj");
		shipSmall.setHitBoxes(shipSmallHitBox);

		decorHitBox.push_back(&shipVikings);
		std::vector<std::string> shipVikingsHitBox;
		shipVikingsHitBox.push_back(HITBOXDEC_PATH + "/BoatVikings.obj");
		shipVikings.setHitBoxes(shipVikingsHitBox);



	}

	// Here you load and setup all your Vulkan objects
	void localInit() {

		setGameState();

		loadHitBoxes();

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
		for (Bird *bird : birds) {
			A_BlueBird.addDSet(this, &DSLobj, &(*bird).dSet);
		}

		A_PigStd.init(this, "/PigCustom/PigStandard.obj", "/texture.png", &DSLobj);
		A_PigStd.addDSet(this, &DSLobj, &pigStd.dSet);

		A_PigHelmet.init(this, "/PigCustom/PigHelmet.obj", "/texture.png", &DSLobj);
		A_PigHelmet.addDSet(this, &DSLobj, &pigBaloon.dSet);

		A_PigKingHouse.init(this, "/PigCustom/PigKingHouse.obj", "/texture.png", &DSLobj);
		A_PigKingHouse.addDSet(this, &DSLobj, &pigHouse.dSet);

		A_PigKingShip.init(this, "/PigCustom/PigKingBoat.obj", "/texture.png", &DSLobj);
		A_PigKingShip.addDSet(this, &DSLobj, &pigShip.dSet);

		A_PigMechanics.init(this, "/PigCustom/PigMechanic.obj", "/texture.png", &DSLobj);
		A_PigMechanics.addDSet(this, &DSLobj, &pigShipMini.dSet);

		A_PigStache.init(this, "/PigCustom/PigStache.obj", "/texture.png", &DSLobj);
		A_PigStache.addDSet(this, &DSLobj, &pigCitySky.dSet);

		A_Terrain.init(this, "/Terrain/Terrain.obj", "/Terrain/terrain.png", &DSLobj);
		A_Terrain.addDSet(this, &DSLobj, &terrain.dSet);

		A_CannonBot.init(this, "/Cannon/BotCannon.obj", "/Cannon/map_CP_001.001_BaseColorRedBird.png", &DSLobj);
		A_CannonBot.addDSet(this, &DSLobj, &cannonBot.dSet);

		A_CannonTop.init(this, "/Cannon/TopCannon.obj", "/Cannon/map_CP_001.001_BaseColorRedBird.png", &DSLobj);
		A_CannonTop.addDSet(this, &DSLobj, &cannonTop.dSet);

		A_Sphere.init(this, "/Cannon/Trajectory.obj", "/Cannon/Trajectory.png", &DSLobj);
		for (WhiteSphere *block : trajectorySpheres) {
			A_Sphere.addDSet(this, &DSLobj, &(*block).dSet);
		}

		A_TowerSiege.init(this, "/Decorations/TowerSiege.obj", "/Decorations/TowerSiege.png", &DSLobj);
		A_TowerSiege.addDSet(this, &DSLobj, &towerSiege.dSet);

		A_Baloon.init(this, "/Decorations/Baloon.obj", "/Decorations/Baloon.png", &DSLobj);
		A_Baloon.addDSet(this, &DSLobj, &baloon.dSet);

		A_SeaCity25.init(this, "/Decorations/SeaCity25.obj", "/Decorations/SeaCity25.png", &DSLobj);
		A_SeaCity25.addDSet(this, &DSLobj, &seaCity25.dSet);

		A_SeaCity37.init(this, "/Decorations/SeaCity37.obj", "/Decorations/SeaCity37.png", &DSLobj);
		A_SeaCity37.addDSet(this, &DSLobj, &seaCity37.dSet);

		A_ShipSmall.init(this, "/Decorations/ShipSmall.obj", "/Decorations/ShipSmall.png", &DSLobj);
		A_ShipSmall.addDSet(this, &DSLobj, &shipSmall.dSet);

		A_ShipVikings.init(this, "/Decorations/ShipVikings.obj", "/Decorations/ShipVikings.png", &DSLobj);
		A_ShipVikings.addDSet(this, &DSLobj, &shipVikings.dSet);

		A_SkyCity.init(this, "/Decorations/SkyCity.obj", "/Decorations/SkyCity.png", &DSLobj);
		A_SkyCity.addDSet(this, &DSLobj, &skyCity.dSet);

		skyBox.init(this, DSLobj, DSLglobal);


		DS_global.init(this, &DSLglobal, {
		{0, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr},
			});
	}

	// Here you destroy all the objects you created!		
	void localCleanup() {

		A_BlueBird.cleanup();

		A_PigStd.cleanup();
		A_PigHelmet.cleanup();
		A_PigKingHouse.cleanup();
		A_PigKingShip.cleanup();
		A_PigMechanics.cleanup();
		A_PigStache.cleanup();

		A_Terrain.cleanup();

		A_CannonBot.cleanup();

		A_CannonTop.cleanup();

		A_Sphere.cleanup();

		//Decorations
		A_Baloon.cleanup();
		A_SeaCity25.cleanup();
		A_SeaCity37.cleanup();
		A_ShipSmall.cleanup();
		A_ShipVikings.cleanup();
		A_TowerSiege.cleanup();
		A_SkyCity.cleanup();

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
		A_PigHelmet.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		A_PigKingHouse.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		A_PigKingShip.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		A_PigMechanics.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		A_PigStache.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);

		// ------------------------ Terrain -----------------

		 A_Terrain.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);

		 // ----------------------- Cannon -------------------

		 A_CannonBot.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		 A_CannonTop.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);

		 // ----------------------- Trajectory -------------------

		 A_Sphere.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);

		 // ----------------------- DECORATIONS --------------------
		 A_Baloon.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		 A_SeaCity25.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		 A_SeaCity37.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		 A_ShipSmall.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		 A_ShipVikings.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		 A_TowerSiege.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		 A_SkyCity.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		 
	}

	void handleCollision()
	{
		std::vector<glm::vec2> birdHitBox = bird1.getHitBox();
		bool x = false, y = false, z = false;

		for (Pig *pig : pigsHitBox) {
			std::vector<glm::vec2> pigHitBox = pig->getHitBox();

			if ((birdHitBox[0][0] > pigHitBox[0][0] && birdHitBox[0][0] < pigHitBox[0][1]) || (birdHitBox[0][1] > pigHitBox[0][0] && birdHitBox[0][1] < pigHitBox[0][1]))
				x = true;
			if ((birdHitBox[1][0] > pigHitBox[1][0] && birdHitBox[1][0] < pigHitBox[1][1]) || (birdHitBox[1][1] > pigHitBox[1][0] && birdHitBox[1][1] < pigHitBox[1][1]))
				y = true;
			if ((birdHitBox[2][0] > pigHitBox[2][0] && birdHitBox[2][0] < pigHitBox[2][1]) || (birdHitBox[2][1] > pigHitBox[2][0] && birdHitBox[2][1] < pigHitBox[2][1]))
				z = true;

			if (x && y && z) {
				std::cout << "HIT PIG\n";
				
			}
		}



		for (Decoration *decor : decorHitBox)
		{
			std::vector <std::vector<glm::vec2>> decorHitBoxes = decor->getHitBox();
			for (std::vector<glm::vec2> HitBox : decorHitBoxes) {
				x = false; y = false; z = false;
				if ((birdHitBox[0][0] > HitBox[0][0] && birdHitBox[0][0] < HitBox[0][1]) || (birdHitBox[0][1] > HitBox[0][0] && birdHitBox[0][1] < HitBox[0][1]))
					x = true;
				if ((birdHitBox[1][0] > HitBox[1][0] && birdHitBox[1][0] < HitBox[1][1]) || (birdHitBox[1][1] > HitBox[1][0] && birdHitBox[1][1] < HitBox[1][1]))
					y = true;
				if ((birdHitBox[2][0] > HitBox[2][0] && birdHitBox[2][0] < HitBox[2][1]) || (birdHitBox[2][1] > HitBox[2][0] && birdHitBox[2][1] < HitBox[2][1]))
					z = true;

				if (x && y && z) {
					std::cout << "TERRAIN HIT\n";
				}
			}

		}
	}

	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {

		GameTime::GetInstance()->setTime();


		UniformBufferObject ubo{};
		GlobalUniformBufferObject gubo{};

		void* data;

		gubo.view = camera.update(window);
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

		// ------------------------ BIRDS ---------------------------

		birds.at(birdInCannon)->updateUniformBuffer(window, device, currentImage, data, ubo);

		// ------------------------ PIGS ---------------------------

		pigStd.updateUniformBuffer(window, device, currentImage, data, ubo);
		pigBaloon.updateUniformBuffer(window, device, currentImage, data, ubo);
		pigHouse.updateUniformBuffer(window, device, currentImage, data, ubo);
		pigShip.updateUniformBuffer(window, device, currentImage, data, ubo);
		pigCitySky.updateUniformBuffer(window, device, currentImage, data, ubo);
		pigShipMini.updateUniformBuffer(window, device, currentImage, data, ubo);

		//----------------------- TERRAIN --------------------------

		terrain.updateUniformBuffer(window, device, currentImage, data, ubo);

		// --------------------- Cannon ----------------------------

		cannonBot.updateUniformBuffer(window, device, currentImage, data, ubo);
		cannonTop.updateUniformBuffer(window, device, currentImage, data, ubo);

		// --------------------- Trajectory ------------------------

		for (WhiteSphere* block : trajectorySpheres) {
			block->updateUniformBuffer(window, device, currentImage, data, ubo);
		}

		// -------------------- DECORATION ---------------------------

		baloon.updateUniformBuffer(window, device, currentImage, data, ubo);
		seaCity25.updateUniformBuffer(window, device, currentImage, data, ubo);
		seaCity37.updateUniformBuffer(window, device, currentImage, data, ubo);
		shipSmall.updateUniformBuffer(window, device, currentImage, data, ubo);
		shipVikings.updateUniformBuffer(window, device, currentImage, data, ubo);
		towerSiege.updateUniformBuffer(window, device, currentImage, data, ubo);
		skyCity.updateUniformBuffer(window, device, currentImage, data, ubo);


		// ------------------------------ COLLISION
		handleCollision();
	}


};


//calbacks inputs
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	
	if (key == GLFW_KEY_X && action == GLFW_PRESS)
	{
		cameraON = !cameraON;
		if (controller == CameraMovement) {
			controller = CannonMovement;
		}
		else {
			controller = CameraMovement;
		}
	}
	if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
		cCamera->NextView();
	}
	//to debg the position of the camera if we want to add new view
	if (key == GLFW_KEY_L && action == GLFW_PRESS) {
		cCamera->ShowStat();
	}
	if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
		cCamera->SetView(1);
	}
	if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
		cCamera->SetView(3);
	}
	if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
		cCamera->SetView(2);
	}
	if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
		cCamera->SetView(4);
	}
	if (controller == CannonMovement) {
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
			cTop->shoot();
		}
	}
}


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