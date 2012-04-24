

#include "btbDeviceCL.h"
#include "../broadphase_benchmark/btOpenCLArray.h"
#include "../broadphase_benchmark/btRadixSort32CL.h"

typedef struct
{
    cl_context m_ctx;
    cl_device_id m_device;
    cl_command_queue m_queue;
} btbMyDeviceCL;

btbDevice btbCreateDeviceCL(cl_context ctx, cl_device_id dev, cl_command_queue q)
{
    btbMyDeviceCL* clDev = (btbMyDeviceCL*)malloc(sizeof(btbMyDeviceCL));
    clDev->m_ctx = ctx;
    clDev->m_device = dev;
    clDev->m_queue = q;
	return (btbDevice) clDev;
}

void btbReleaseDevice(btbDevice d)
{
	btbMyDeviceCL* dev = (btbMyDeviceCL*)d;
	btbAssert(dev);
	delete dev;
}

cl_int btbGetLastErrorCL(btbDevice d)
{
	return 0;
}

void btbWaitForCompletion(btbDevice d)
{
	btbMyDeviceCL* dev = (btbMyDeviceCL*)d;
    clFinish(dev->m_queue);
}

btbBuffer btbCreateSortDataBuffer(btbDevice d, int numElements)
{
	btbMyDeviceCL* dev = (btbMyDeviceCL*) d;
    btOpenCLArray<btSortData>* buf = new btOpenCLArray<btSortData>(dev->m_ctx,dev->m_queue);
    buf->resize(numElements);
	return (btbBuffer)buf;
}

void btbReleaseBuffer(btbBuffer b)
{
	btOpenCLArray<btSortData>* buf = (btOpenCLArray<btSortData>*) b;
	delete buf;
}

///like memcpy destination comes first
void btbCopyHostToBuffer(btbBuffer devDest, const btbSortData2ui* hostSrc, int sizeInElements)
{
    const btSortData* hostPtr = (const btSortData*)hostSrc;
	btOpenCLArray<btSortData>* devBuf = (btOpenCLArray<btSortData>*)devDest;
	devBuf->copyFromHost(0,sizeInElements,hostPtr);
}

void btbCopyBufferToHost(btbSortData2ui* hostDest, const btbBuffer devSrc, int sizeInElements)
{
    btSortData* hostPtr = (btSortData*)hostDest;
	btOpenCLArray<btSortData>* devBuf = (btOpenCLArray<btSortData>*)devSrc;
	devBuf->copyToHost(0,sizeInElements,hostPtr);
}


/*

void btbCopyBuffer(btbBuffer dst, const btbBuffer src, int sizeInElements)
{

}

*/

void btbFillInt2Buffer(btbDevice d, btbBuffer dst,int v0, int v1)
{
    

}

btbRadixSort btbCreateRadixSort(btbDevice d, int maxNumElements)
{
	btbMyDeviceCL* dev = (btbMyDeviceCL*)d;
    btRadixSort32CL* radix = new btRadixSort32CL(dev->m_ctx,dev->m_device,dev->m_queue);
	return (btbRadixSort)radix;
}
void btbSort(btbRadixSort s, btbBuffer b, int numElements)
{
	btRadixSort32CL* radix = (btRadixSort32CL*)s;
	btOpenCLArray<btSortData>* buf = (btOpenCLArray<btSortData>*)b;
	radix->execute( *buf);
}
