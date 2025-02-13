#ifndef PERSO_HPP
#define PERSO_HPP

#include "glad/glad.h"
#include <NTL.hpp>

class ClKernel;
class ClContext;
class ClError;

namespace OpenCGL {
void RunKernelWithMem(cl::CommandQueue const &queue, ClKernel const &kernel,
                      std::vector<cl::Memory> const &cl_vbos,
                      cl::NDRange at = cl::NullRange, cl::NDRange global = cl::NullRange, cl::NDRange local = cl::NullRange);
void RunKernelWithMem(cl::CommandQueue const &queue, ClKernel const &kernel,
                      cl::Memory const &mem,
                      cl::NDRange at = cl::NullRange, cl::NDRange global = cl::NullRange, cl::NDRange local = cl::NullRange);

void RunKernelWithMem(cl::CommandQueue const &queue, ClKernel const &kernel,
                      cl::NDRange at = cl::NullRange, cl::NDRange global = cl::NullRange, cl::NDRange local = cl::NullRange);
} // namespace OpenCGL

#endif
