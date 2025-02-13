#include "ParticleEmitterSprite.hpp"
#include "Engine/Shader.hpp"
#include "Cl/ClQueue.hpp"
#include "Particle/ParticleData.hpp"
#include "Particle/ParticleModule/ParticleSpawnModule.hpp"
#include "Engine/MainGraphic.hpp"
#include "Cl/ClProgram.hpp"
#include "OpenCGL_Tools.hpp"
#include "cl_type.hpp"
#include <Engine/Camera.hpp>
#include <Engine/ShaderManager.hpp>
#include "Cl/ClKernel.hpp"
#include <string.h>
#include <PathManager.hpp>

ParticleEmitterSprite::ParticleEmitterSprite(ParticleSystem &system, ClQueue &queue, std::string const &name, size_t nbParticleMax)
    :
		AParticleEmitter(system, queue, name, nbParticleMax, 0),
		OCGLBufferParticles_SpriteData_(nbParticleMax * sizeof(ParticleDataSprite)),
		atlas_("bloup.png", PathManager::Get().getPath("atlas"), 4),
		gpuBufferParticles_Dist_(ClContext::Get().context, CL_MEM_WRITE_ONLY, nbParticleMax * sizeof(CL_FLOAT)),
		gpuBufferOutput_nbParticleActive_(ClContext::Get().context, CL_MEM_READ_WRITE, sizeof(int))
{

	ClProgram &program = ClProgram::Get();
	program.addProgram(PathManager::Get().getPath("particleKernels") / "Sort.cl");
	program.addProgram(PathManager::Get().getPath("particleKernels") / "Sprite.cl");



	ShaderManager::Get().addShader("particleSprite");
	ShaderManager::Get().getShader("particleSprite").attach((PathManager::Get().getPath("shaders") / "particleSprite.vert").generic_string());
	ShaderManager::Get().getShader("particleSprite").attach((PathManager::Get().getPath("shaders") / "particleSprite.geom").generic_string());
	ShaderManager::Get().getShader("particleSprite").attach((PathManager::Get().getPath("shaders") / "particleSprite.frag").generic_string());
	ShaderManager::Get().getShader("particleSprite").link();

	float data[] = {
			// positions          // normal           // texture coords
			0.5f,  0.5f, 0.0f,   0.f, 0.f, 1.f,   1.0f, 1.0f, // top right
			0.5f, -0.5f, 0.0f,   0.f, 0.f, 1.f,   1.0f, 0.0f, // bottom right
			-0.5f, -0.5f, 0.0f,   0.f, 0.f, 1.f,   0.0f, 0.0f, // bottom left
			-0.5f,  0.5f, 0.0f,   0.f, 0.f, 1.f,   0.0f, 1.0f  // top left
	};
	unsigned int indices[] = {  // note that we start from 0!
			0, 1, 3,  // first Triangle
			1, 2, 3   // second Triangle
	};
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)nullptr);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	reload();
	glBindVertexArray(0);
}

void ParticleEmitterSprite::reload() {
	if (debug_)
		printf("%s\n", __FUNCTION_NAME__);
    AParticleEmitter::reload();

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, OCGLBufferEmitterParticles_.vbo);

    glEnableVertexAttribArray(3); //Position
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleData), reinterpret_cast<const void *>(offsetof(ParticleData, position)));
    glEnableVertexAttribArray(4); //Velocity
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleData), reinterpret_cast<const void *>(offsetof(ParticleData, velocity)));
    glEnableVertexAttribArray(5); //Color
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleData), reinterpret_cast<const void *>(offsetof(ParticleData, color)));
    glEnableVertexAttribArray(6); //Rotate
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleData), reinterpret_cast<const void *>(offsetof(ParticleData, rotate)));
    glEnableVertexAttribArray(7); //Size
    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleData), reinterpret_cast<const void *>(offsetof(ParticleData, size)));
    glEnableVertexAttribArray(8); //Age
    glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleData), reinterpret_cast<const void *>(offsetof(ParticleData, age)));
    glEnableVertexAttribArray(9); //LifeTime
    glVertexAttribPointer(9, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleData), reinterpret_cast<const void *>(offsetof(ParticleData, lifeTime)));
	glEnableVertexAttribArray(10); //IsAlive
	glVertexAttribIPointer(10, 1, GL_INT, sizeof(ParticleData), reinterpret_cast<const void *>(offsetof(ParticleData, isAlive)));

    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);
    glVertexAttribDivisor(7, 1);
    glVertexAttribDivisor(8, 1);
    glVertexAttribDivisor(9, 1);
	glVertexAttribDivisor(10, 1);


    OCGLBufferParticles_SpriteData_ = OCGL_Buffer(nbParticleMax_ * sizeof(ParticleDataSprite));
    glBindBuffer(GL_ARRAY_BUFFER, OCGLBufferParticles_SpriteData_.vbo);

    glEnableVertexAttribArray(11); //Offset1
    glVertexAttribPointer(11, 2, GL_FLOAT, GL_FALSE, sizeof(ParticleDataSprite), reinterpret_cast<const void *>(offsetof(ParticleDataSprite, offset1)));
    glEnableVertexAttribArray(12); //Offset2
    glVertexAttribPointer(12, 2, GL_FLOAT, GL_FALSE, sizeof(ParticleDataSprite), reinterpret_cast<const void *>(offsetof(ParticleDataSprite, offset2)));
    glEnableVertexAttribArray(13); //Blend
    glVertexAttribPointer(13, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleDataSprite), reinterpret_cast<const void *>(offsetof(ParticleDataSprite, blend)));

    glVertexAttribDivisor(11, 1);
    glVertexAttribDivisor(12, 1);
    glVertexAttribDivisor(13, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    gpuBufferParticles_Dist_ = cl::Buffer(ClContext::Get().context, CL_MEM_READ_WRITE, nbParticleMax_ * sizeof(CL_FLOAT));

}

void ParticleEmitterSprite::init() {
	if (debug_)
		printf("%s\n", __FUNCTION_NAME__);
	for (auto &module : modules_) {
		module->init();
	}
}

void	ParticleEmitterSprite::updateSpriteData() {
	if (debug_)
		printf("%s\n", __FUNCTION_NAME__);
	ClError err;
	std::vector<cl::Memory> cl_vbos;
	ClKernel kernel;
	kernel.setKernel(*this, "sprite");

	cl_vbos.push_back(OCGLBufferEmitterParticles_.mem);
	cl_vbos.push_back(OCGLBufferParticles_SpriteData_.mem);

	kernel.beginAndSetUpdatedArgs(OCGLBufferEmitterParticles_.mem,
			OCGLBufferParticles_SpriteData_.mem,
			atlas_.getNumberOfRows());

	OpenCGL::RunKernelWithMem(queue_.getQueue(), kernel, cl_vbos, cl::NullRange, cl::NDRange(nbParticleMax_));
}

void ParticleEmitterSprite::update(float deltaTime) {
    if (debug_)
        printf("%s\n", __FUNCTION_NAME__);

	AParticleEmitter::update(deltaTime);
    checkReload();
    updateSpriteData();
    getNbParticleActive_();

    for (auto &module : modules_) {
        module->update(deltaTime);
    }

	//sortDeviceBuffer_();
	//getNbParticleActive_();

	//std::cout << "-------- After ParticleEmitterSprite update" << std::endl;
	//printSubArrayParticle(*this, queue_.getQueue());
	//std::cout << "xxxxxxxx After ParticleEmitterSprite update" << std::endl;


}

void ParticleEmitterSprite::render() {
	if (debug_)
		printf("%s\n", __FUNCTION_NAME__);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDepthMask(GL_FALSE);
	ShaderManager::Get().getShader("particleSprite").activate();
	ShaderManager::Get().getShader("particleSprite").setMat4("projection", Camera::focus->getProjectionMatrix());
	ShaderManager::Get().getShader("particleSprite").setMat4("view", Camera::focus->getViewMatrix());
	ShaderManager::Get().getShader("particleSprite").setUInt("numberOfRows", atlas_.getNumberOfRows());
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, atlas_.getAtlasId());

	glBindVertexArray(VAO);

	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, nbParticleMax_);
	//glDrawElementsInstanced(GL_POINTS, 6, GL_UNSIGNED_INT, 0, nbParticleMax_);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_TRUE);
}


//Need sorted GPU Buffer for get NbParticleActive
void ParticleEmitterSprite::getNbParticleActive_() {


	nbParticleActive_ = (nbParticleMax_) - indexSub_[2];
	return ;
/*
	if (debug_)
		printf("%s\n", __FUNCTION_NAME__);
	cl::Kernel &kernel = ClProgram::Get().getKernel("getNbParticleActiveSafe");
	kernel.setArg(0, deviceBuffer_.mem);
	kernel.setArg(1, distBuffer_);
	kernel.setArg(2, nbParticleActiveOutpourBuffer_);
	kernel.setArg(3, nbParticleMax_);

    OpenCGL::RunKernelWithMem(queue_.getQueue(), kernel, deviceBuffer_.mem, cl::NullRange, cl::NDRange(1));
    queue_.getQueue().enqueueReadBuffer(nbParticleActiveOutpourBuffer_, CL_TRUE, 0, sizeof(int), &nbParticleActive_);
    */
}

void ParticleEmitterSprite::sortDeviceBufferCalculateDistanceParticle_() {
	if (debug_)
		printf("%s\n", __FUNCTION_NAME__);
	ClKernel kernel;
	kernel.setKernel(*this, "calculateDistanceBetweenParticleAndCamera");

	glm::vec3 cameraPosition = Camera::focus->getPosition();
	kernel.beginAndSetUpdatedArgs(OCGLBufferEmitterParticles_.mem,
			gpuBufferParticles_Dist_,
			glmVec3toClFloat3(cameraPosition));

    OpenCGL::RunKernelWithMem(queue_.getQueue(), kernel, OCGLBufferEmitterParticles_.mem, cl::NullRange, cl::NDRange(nbParticleMax_));
	if (debug_)
		std::cout << "nbParticleMax_ [" << nbParticleMax_ << "]" << std::endl;
}

void ParticleEmitterSprite::sortDeviceBuffer_() {
	sortDeviceBufferCalculateDistanceParticle_();
	if (debug_)
		printf("%s\n", __FUNCTION_NAME__);

	ClKernel kernel;
	kernel.setKernel(*this, "ParallelSelection");
			//cl::Kernel &kernel = programSort_.getKernel("ParallelSelection_Blocks");
	//cl::Kernel &kernel = programSort_.getKernel("ParallelMerge_Local");


	//kernel.setArg(0, deviceBuffer_.mem);
	//kernel.setArg(1, deviceBufferSpriteData_.mem);
	//size_t localWorkGroupeSize;
	//kernel.getWorkGroupInfo(ClContext::Get().deviceDefault, CL_KERNEL_WORK_GROUP_SIZE, &localWorkGroupeSize);
	//localWorkGroupeSize /= 2;
	//int blockFactor = 2;
	glm::vec3 cameraPosition = Camera::focus->getPosition();
	//kernel.setArg(2, distBuffer_);
	cl_float3 temp;
	temp.x = cameraPosition.x;
	temp.y = cameraPosition.y;
	temp.z = cameraPosition.z;
	//kernel.setArg(3, temp);

	kernel.beginAndSetUpdatedArgs(OCGLBufferEmitterParticles_.mem,
					  gpuBufferParticles_Alive_,
					  gpuBufferParticles_Spawned_,
					  gpuBufferParticles_Death_,
					  gpuBufferParticles_SubLength_,

					  OCGLBufferParticles_SpriteData_.mem,

					  gpuBufferParticles_Dist_,
					  temp);

	//kernel.setArg(1, sizeof(ParticleData) * localWorkGroupeSize /** blockFactor*/, nullptr);
	//kernel.setArg(2, cameraPosition);
	//kernel.setArg(3, blockFactor);

	ClError err;
	std::vector<cl::Memory> cl_vbos;

	cl_vbos.push_back(OCGLBufferEmitterParticles_.mem);
	cl_vbos.push_back(OCGLBufferParticles_SpriteData_.mem);

	std::cout << indexSub_[0] << std::endl;
	if (indexSub_[0]) {
		OpenCGL::RunKernelWithMem(queue_.getQueue(), kernel, cl_vbos, cl::NullRange, cl::NDRange(indexSub_[0]));


		int *arrayDeath = new int[nbParticleMax_];
		memset((void*)arrayDeath, -1, nbParticleMax_ * sizeof(int));
		int i = 0;
		while (i < nbParticleMax_ - indexSub_[0]){
			arrayDeath[i] = indexSub_[0] + i;
			i++;
		}
		queue_.getQueue().enqueueWriteBuffer(gpuBufferParticles_Death_,
				CL_TRUE, 0, sizeof(int) * nbParticleMax_, arrayDeath);


		indexSub_[2] = i;

		queue_.getQueue().enqueueWriteBuffer(gpuBufferParticles_SubLength_, CL_TRUE, 0, sizeof(int) * 3, &indexSub_);


		/*
		ClKernel kernelDeath;
		kernelDeath.setKernel(*this, "ParallelSelection");

		std::vector<cl::Memory> cl_vbos2;

		cl_vbos2.push_back(particleOCGL_BufferData_.mem);


		indexSub_[2] = 0;

		queue_.getQueue().enqueueWriteBuffer(particleSubBuffersLength_, CL_TRUE, 0, sizeof(int) * 3, &indexSub_);
		kernel.beginAndSetUpdatedArgs(
				particleOCGL_BufferData_.mem,
				particleBufferAlive_,
				particleBufferSpawned_,
				particleBufferDeath_,
				particleSubBuffersLength_);
		queue_.getQueue().enqueueReadBuffer(particleSubBuffersLength_, CL_TRUE, 0, sizeof(int) * 3, &indexSub_);
		*/
	}



}