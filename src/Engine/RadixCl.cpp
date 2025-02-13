#include "RadixCl.hpp"
#include "Cl/ClProgram.hpp"
#include "Debug.hpp"
#include "PathManager.hpp"
#include <climits>

RadixCl::RadixCl()
    : queue_(ClContext::Get().context, ClContext::Get().deviceDefault),
      kernelCount_(ClProgram::Get().getKernel("count")),
      kernelScan_(ClProgram::Get().getKernel("scan")),
      kernelCoalesce_(ClProgram::Get().getKernel("coalesce")),
      kernelReorder_(ClProgram::Get().getKernel("reorder")),
      kernelBlocksum_(ClProgram::Get().getKernel("scan2")) {
    //    ClProgram::Get().addProgram(PathManager::Get().getPath("particleKernels") / "Test.cl");
    Debug::Get().setDebug("Radix", RadixCl::debug_);
}

void RadixCl::radix(cl::Buffer &arrayGpuCompare, cl::Buffer &arrayGpuToSort, unsigned int lengthArrayGpu, bool debug) {

    //
    //	SETUP SIZE
    //

    unsigned int groups = 16;
    unsigned int localWorkSize = 16;
    unsigned int globalWorkSize = localWorkSize * groups;

    const unsigned int baseArrayLength = lengthArrayGpu;
    const unsigned int rest = lengthArrayGpu % (groups * localWorkSize);
    const unsigned int arrayLength = (rest == 0 ? lengthArrayGpu : (lengthArrayGpu - rest + (groups * localWorkSize)));

    //
    //	CREATE BUFFERS
    //

    cl::Buffer bufferArrayToCompare(ClContext::Get().context, CL_MEM_READ_WRITE, sizeof(cl_uint) * arrayLength);
    cl::Buffer bufferArrayToSort(ClContext::Get().context, CL_MEM_READ_WRITE, sizeof(cl_uint) * arrayLength);

    cl::Buffer bufferHisto(ClContext::Get().context, CL_MEM_READ_WRITE, sizeof(cl_uint) * BUCK * groups * localWorkSize);
    cl::Buffer bufferScan(ClContext::Get().context, CL_MEM_READ_WRITE, sizeof(cl_uint) * BUCK * groups * localWorkSize);
    cl::Buffer bufferBlocksum(ClContext::Get().context, CL_MEM_READ_WRITE, sizeof(cl_uint) * groups);
    cl::Buffer bufferOutputToCompare(ClContext::Get().context, CL_MEM_READ_WRITE, sizeof(cl_uint) * arrayLength);
    cl::Buffer bufferOutputToSort(ClContext::Get().context, CL_MEM_READ_WRITE, sizeof(cl_uint) * arrayLength);

    cl::LocalSpaceArg bufferLocalHisto = cl::Local(sizeof(cl_uint) * BUCK * localWorkSize);
    cl::LocalSpaceArg bufferLocalScan = cl::Local(sizeof(cl_uint) * BUCK * localWorkSize);
    cl::LocalSpaceArg bufferLocalBloclsum = cl::Local(sizeof(cl_uint) * groups);

    //Write
    err_.err = queue_.enqueueCopyBuffer(arrayGpuCompare, bufferArrayToCompare, 0, 0, sizeof(cl_uint) * baseArrayLength);
    err_.err |= queue_.enqueueFillBuffer<cl_uint>(bufferArrayToCompare, UINT_MAX, sizeof(cl_uint) * baseArrayLength, sizeof(cl_uint) * (arrayLength - baseArrayLength));
    err_.clCheckError();

    err_.err = queue_.enqueueCopyBuffer(arrayGpuToSort, bufferArrayToSort, 0, 0, sizeof(cl_uint) * baseArrayLength);
    err_.err |= queue_.enqueueFillBuffer<cl_uint>(bufferArrayToSort, UINT_MAX, sizeof(cl_uint) * baseArrayLength, sizeof(cl_uint) * (arrayLength - baseArrayLength));
    err_.clCheckError();

    if (debug) {
        printIntArrayGpu(arrayGpuToSort, baseArrayLength, "StartArray arrayGpuToSort");
        printIntArrayGpu(arrayGpuCompare, baseArrayLength, "StartArray arrayGpuCompare");
    }

    //
    //		SET FIXED ARGS
    //
    //Histo
    err_.err = kernelCount_.setArg(2, bufferLocalHisto); // Local Histogram
    err_.clCheckError();
    err_.err = kernelCount_.setArg(4, arrayLength);
    err_.clCheckError();

    //Scan
    unsigned int scanGlobalWorkSize = (BUCK * groups * localWorkSize) / 2;
    unsigned int scanLocalWorkSize = scanGlobalWorkSize / groups;
    unsigned int doBlockSum2 = 1;
    err_.err = kernelScan_.setArg(1, bufferScan);
    err_.err |= kernelScan_.setArg(2, bufferLocalScan);
    err_.err |= kernelScan_.setArg(3, bufferBlocksum);
    err_.err |= kernelScan_.setArg(4, doBlockSum2);
    err_.clCheckError();

    //Blocksum
    unsigned int blocksumGlobalWorkSize = groups / 2;
    unsigned int blocksumLocalWorkSize = groups / 2;
    unsigned int doBlockSum = 0;
    err_.err = kernelBlocksum_.setArg(0, bufferBlocksum);
    err_.err |= kernelBlocksum_.setArg(1, bufferBlocksum);
    err_.err |= kernelBlocksum_.setArg(2, bufferLocalBloclsum);
    err_.err |= kernelBlocksum_.setArg(3, bufferBlocksum);
    err_.err |= kernelBlocksum_.setArg(4, doBlockSum);
    err_.clCheckError();

    //Coalesce
    unsigned int coalesceGlobalWorkSize = (BUCK * localWorkSize * groups) / 2;
    unsigned int coalesceLocalWorkSize = coalesceGlobalWorkSize / groups;
    err_.err = kernelCoalesce_.setArg(0, bufferScan);
    err_.err |= kernelCoalesce_.setArg(1, bufferBlocksum);
    err_.clCheckError();

    //Reorder
    unsigned int reorderGlobalWorkSize = groups * localWorkSize;
    unsigned int reorderLocalWorkSize = groups;
    err_.err = kernelReorder_.setArg(2, bufferScan);
    err_.err |= kernelReorder_.setArg(6, arrayLength);
    err_.err |= kernelReorder_.setArg(7, bufferLocalHisto);
    err_.clCheckError();

    //
    //		PASS
    //
    for (unsigned int pass = 0; pass < BITS / RADIX; pass++) {

        //
        //	Count
        //
        err_.err = kernelCount_.setArg(0, bufferArrayToCompare);
        err_.err |= kernelCount_.setArg(1, bufferHisto);
        err_.err |= kernelCount_.setArg(3, pass);
        err_.clCheckError();

        if (debug_) {
            std::cout << "globalWorkSize : " << globalWorkSize << std::endl;
            std::cout << "localWorkSize : " << localWorkSize << std::endl;
        }
        err_.err = queue_.enqueueNDRangeKernel(kernelCount_, cl::NullRange, cl::NDRange(globalWorkSize),
                                               cl::NDRange(localWorkSize));
        err_.clCheckError();
        queue_.finish();
        if (debug_)
            printIntArrayGpu(bufferHisto, BUCK * groups * localWorkSize, "Count : bufferHisto");

        //
        //	Scan
        //
        err_.err = kernelScan_.setArg(0, bufferHisto);
        err_.clCheckError();

        if (debug_) {
            std::cout << "scanGlobalWorkSize : " << scanGlobalWorkSize << std::endl;
            std::cout << "scanLocalWorkSize : " << scanLocalWorkSize << std::endl;
        }

        err_.err = queue_.enqueueNDRangeKernel(kernelScan_, cl::NullRange, cl::NDRange(scanGlobalWorkSize),
                                               cl::NDRange(scanLocalWorkSize));
        err_.clCheckError();
        queue_.finish();
        if (debug_) {
            printIntArrayGpu(bufferScan, BUCK * groups * localWorkSize, "Scan : bufferScan");
            printIntArrayGpu(bufferBlocksum, groups, "Scan : bufferBlocksum");
        }

        //
        //	Blocksum
        //

        if (debug_) {
            std::cout << "blocksumGlobalWorkSize : " << blocksumGlobalWorkSize << std::endl;
            std::cout << "blocksumLocalWorkSize : " << blocksumLocalWorkSize << std::endl;
        }
        err_.err = queue_.enqueueNDRangeKernel(kernelBlocksum_, cl::NullRange, cl::NDRange(blocksumGlobalWorkSize),
                                               cl::NDRange(blocksumLocalWorkSize));
        err_.clCheckError();
        queue_.finish();

        if (debug_)
            printIntArrayGpu(bufferBlocksum, groups, "Blocksum : bufferBlocksum");

        //
        //	Coalesce
        //
        if (debug_) {
            std::cout << "coalesceGlobalWorkSize : " << coalesceGlobalWorkSize << std::endl;
            std::cout << "coalesceLocalWorkSize : " << coalesceLocalWorkSize << std::endl;
        }
        err_.err = queue_.enqueueNDRangeKernel(kernelCoalesce_, cl::NullRange, cl::NDRange(coalesceGlobalWorkSize),
                                               cl::NDRange(coalesceLocalWorkSize));
        err_.clCheckError();
        queue_.finish();
        if (debug_)
            printIntArrayGpu(bufferScan, BUCK * localWorkSize * groups, "Coalesce : bufferScan");

        //
        //	Reorder
        //

        //Reorder arguments
        err_.err = kernelReorder_.setArg(0, bufferArrayToCompare);
        err_.err = kernelReorder_.setArg(1, bufferArrayToSort);
        err_.err |= kernelReorder_.setArg(3, bufferOutputToCompare);
        err_.err |= kernelReorder_.setArg(4, bufferOutputToSort);
        err_.err |= kernelReorder_.setArg(5, pass);
        err_.clCheckError();

        if (debug_) {
            std::cout << "ReorderGlobalWorkSize : " << reorderGlobalWorkSize << std::endl;
            std::cout << "ReorderLocalWorkSize : " << reorderLocalWorkSize << std::endl;
        }

        err_.err = queue_.enqueueNDRangeKernel(kernelReorder_, cl::NullRange, cl::NDRange(reorderGlobalWorkSize),
                                               cl::NDRange(reorderLocalWorkSize));
        err_.clCheckError();
        queue_.finish();

        if (debug_)
            printIntArrayGpu(bufferOutputToSort, arrayLength, "Reorder : bufferOutput");

        cl::Buffer tmp = bufferArrayToSort;
        bufferArrayToSort = bufferOutputToSort;
        bufferOutputToSort = tmp;

        tmp = bufferArrayToCompare;
        bufferArrayToCompare = bufferOutputToCompare;
        bufferOutputToCompare = tmp;
    }
    //
    //printIntArrayGpu(bufferArrayToCompare, baseArrayLength, "FF-----");

    queue_.enqueueCopyBuffer(bufferArrayToSort, arrayGpuToSort, 0, 0, sizeof(cl_uint) * baseArrayLength);
    if (debug) {
        printIntArrayGpu(arrayGpuToSort, baseArrayLength, "FinalArray arrayGpuToSort");
        printIntArrayGpu(arrayGpuCompare, baseArrayLength, "FinalArray arrayGpuCompare");
    }
}

void RadixCl::printIntArrayGpu(cl::Buffer &buffer, unsigned int length, std::string const &name) {
    std::cout << name << std::endl;
    unsigned int *arrayCpu = new unsigned int[length];
    err_.err = queue_.enqueueReadBuffer(buffer, CL_TRUE, 0, sizeof(cl_uint) * length, arrayCpu);
    err_.clCheckError();
    queue_.finish();

    std::cout << "start [" << name << "]" << std::endl;
    for (unsigned int i = 0; i < length; i++)
        std::cout << i << " : " << arrayCpu[i] << std::endl;
    std::cout << "end [" << name << "]" << std::endl;
}

bool RadixCl::debug_ = false;