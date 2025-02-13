#include "ModuleMoveToTarget.hpp"
#include "Cl/ClContext.hpp"
#include "Cl/ClProgram.hpp"
#include "Cl/ClQueue.hpp"
#include "Engine/Random.hpp"
#include "ModuleTarget.hpp"
#include "OpenCGL_Tools.hpp"
#include "Particle/ParticleSystem.hpp"
#include "Particle/PaticleEmitter/AParticleEmitter.hpp"
#include "cl_type.hpp"

ModuleMoveToTarget::ModuleMoveToTarget(AParticleEmitter &emitter)
    : AParticleModule(emitter) {
    if (debug_)
        printf("%s\n", __FUNCTION_NAME__);
    //if (!emitter_.contain<ModuleTarget>()) {
    //    emitter_.addModule<ModuleTarget>();
    //}

    ClProgram::Get().addProgram(pathKernel_ / "Target.cl");

    kernelUpdate_.setKernel(emitter_, "updateMoveToTarget");
    kernelUpdate_.setArgsGPUBuffers(eParticleBuffer::kData);
}

void ModuleMoveToTarget::init() {
    if (debug_)
        printf("%s\n", __FUNCTION_NAME__);
    gpuBufferParticles_Target_ = emitter_.getClBuffer<ModuleTarget>();
}

void ModuleMoveToTarget::update(float deltaTime) {
    if (!isActive_)
        return;
    if (debug_)
        printf("%s\n", __FUNCTION_NAME__);
    kernelUpdate_.beginAndSetUpdatedArgs(*gpuBufferParticles_Target_, deltaTime);
    std::vector<cl::Memory> cl_vbos;
    cl_vbos.push_back(emitter_.getParticleOCGL_BufferData().mem);
    OpenCGL::RunKernelWithMem(queue_.getQueue(), kernelUpdate_, cl_vbos, cl::NullRange, cl::NDRange(nbParticleMax_));
}
