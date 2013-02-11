/*
Copyright (c) 2012 Advanced Micro Devices, Inc.  

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/


#include <stdio.h>
#include "../basic_initialize/btOpenCLUtils.h"
#include "../broadphase_benchmark/btFillCL.h"
#include "../broadphase_benchmark/btBoundSearchCL.h"
#include "../broadphase_benchmark/btRadixSort32CL.h"
#include "../broadphase_benchmark/btPrefixScanCL.h"


#include "../../../bullet2/LinearMath/btMinMax.h"
int g_nPassed = 0;
int g_nFailed = 0;
bool g_testFailed = 0;

#define TEST_INIT g_testFailed = 0;
#define TEST_ASSERT(x) if( !(x) ){g_testFailed = 1;}
#define TEST_REPORT(testName) printf("[%s] %s\n",(g_testFailed)?"X":"O", testName); if(g_testFailed) g_nFailed++; else g_nPassed++;
#define NEXTMULTIPLEOF(num, alignment) (((num)/(alignment) + (((num)%(alignment)==0)?0:1))*(alignment))

cl_context g_context=0;
cl_device_id g_device=0;
cl_command_queue g_queue =0;
const char* g_deviceName = 0;

void initCL(int preferredDeviceIndex, int preferredPlatformIndex)
{
	void* glCtx=0;
	void* glDC = 0;
	int ciErrNum = 0;
	cl_device_type deviceType = CL_DEVICE_TYPE_ALL;
//	cl_device_type deviceType = CL_DEVICE_TYPE_GPU;
	g_context = btOpenCLUtils::createContextFromType(deviceType, &ciErrNum, 0,0,preferredDeviceIndex, preferredPlatformIndex);
	oclCHECKERROR(ciErrNum, CL_SUCCESS);
	int numDev = btOpenCLUtils::getNumDevices(g_context);
	if (numDev>0)
	{
		btOpenCLDeviceInfo info;
		g_device= btOpenCLUtils::getDevice(g_context,0);
		g_queue = clCreateCommandQueue(g_context, g_device, 0, &ciErrNum);
		oclCHECKERROR(ciErrNum, CL_SUCCESS);
        btOpenCLUtils::printDeviceInfo(g_device);
		btOpenCLUtils::getDeviceInfo(g_device,&info);
		g_deviceName = info.m_deviceName;
	}
}

void exitCL()
{
	clReleaseCommandQueue(g_queue);
	clReleaseContext(g_context);
}


inline void fillIntTest()
{
	TEST_INIT;

	btFillCL* fillCL = new btFillCL(g_context,g_device,g_queue);
	int maxSize=1024*256;
	btOpenCLArray<btInt2> intBuffer(g_context,g_queue,maxSize);
	intBuffer.resize(maxSize);
	
#define NUM_TESTS 7

	int dx = maxSize/NUM_TESTS;
	for (int iter=0;iter<NUM_TESTS;iter++)
	{
		int size = btMin( 11+dx*iter, maxSize );

		btInt2 value;
		value.x = iter;
		value.y = 2.f;

		int offset=0;
		fillCL->execute(intBuffer,value,size,offset);

		btAlignedObjectArray<btInt2> hostBuf2;
		hostBuf2.resize(size);
		fillCL->executeHost(hostBuf2,value,size,offset);

		btAlignedObjectArray<btInt2> hostBuf;
		intBuffer.copyToHost(hostBuf);

		for(int i=0; i<size; i++)
		{
				TEST_ASSERT( hostBuf[i].x == hostBuf2[i].x );
				TEST_ASSERT( hostBuf[i].y == hostBuf2[i].y );
		}
	}

	

	delete fillCL;

	TEST_REPORT( "fillIntTest" );
}


__inline
void seedRandom(int seed)
{
	srand( seed );
}

template<typename T>
__inline
T getRandom(const T& minV, const T& maxV)
{
	float r = (rand()%10000)/10000.f;
	T range = maxV - minV;
	return (T)(minV + r*range);
}


void boundSearchTest( )
{
	TEST_INIT;

	int maxSize = 1024*256;
	int bucketSize = 256;

	btOpenCLArray<btSortData> srcCL(g_context,g_queue,maxSize);
	btOpenCLArray<unsigned int> upperCL(g_context,g_queue,maxSize);
	btOpenCLArray<unsigned int> lowerCL(g_context,g_queue,maxSize);
	
	btAlignedObjectArray<btSortData> srcHost;
	btAlignedObjectArray<unsigned int> upperHost;
	btAlignedObjectArray<unsigned int> lowerHost;
	
	btBoundSearchCL* search = new btBoundSearchCL(g_context,g_device,g_queue, maxSize);

	btRadixSort32CL* radixSort = new btRadixSort32CL(g_context,g_device,g_queue);

	int dx = maxSize/NUM_TESTS;
	for(int iter=0; iter<NUM_TESTS; iter++)
	{
		
		int size = btMin( 128+dx*iter, maxSize );

		upperHost.resize(bucketSize);
		lowerHost.resize(bucketSize);
		srcHost.resize(size);

		for(int i=0; i<size; i++) 
		{
			btSortData v;
			v.m_key = getRandom(0,bucketSize);
			v.m_value = i;
			srcHost.at(i) = v;
		}

		radixSort->executeHost(srcHost);
		srcCL.copyFromHost(srcHost);

		{
			
			for(int i=0; i<bucketSize; i++) 
				upperHost[i] = -1;

			upperCL.copyFromHost(upperHost);
			lowerCL.copyFromHost(upperHost);
		}

		search->execute(srcCL,size,upperCL,bucketSize,btBoundSearchCL::BOUND_UPPER);
		search->execute(srcCL,size,lowerCL,bucketSize,btBoundSearchCL::BOUND_LOWER);

		lowerCL.copyToHost(lowerHost);
		upperCL.copyToHost(upperHost);
				
		/*
		for(int i=1; i<bucketSize; i++)
		{
			int lhi_1 = lowerHost[i-1];
			int lhi = lowerHost[i];

			for(int j=lhi_1; j<lhi; j++)
			//for(int j=lowerHost[i-1]; j<lowerHost[i]; j++)
			{
				TEST_ASSERT( srcHost[j].m_key < i );
			}
		}

		for(int i=0; i<bucketSize; i++)
		{
			int jMin = (i==0)?0:upperHost[i-1];
			for(int j=jMin; j<upperHost[i]; j++)
			{
				TEST_ASSERT( srcHost[j].m_key <= i );
			}
		}
		*/


		for(int i=0; i<bucketSize; i++)
		{
			int lhi = lowerHost[i];
			int uhi = upperHost[i];

			for(int j=lhi; j<uhi; j++)
			{
				if ( srcHost[j].m_key != i )
				{
					printf("error %d != %d\n",srcHost[j].m_key,i);
				}
				TEST_ASSERT( srcHost[j].m_key == i );
			}
		}

	}

	delete radixSort;
	delete search;

	TEST_REPORT( "boundSearchTest" );
}


void prefixScanTest()
{
	TEST_INIT;

	int maxSize = 1024*256;

	btAlignedObjectArray<unsigned int> buf0Host;
	btAlignedObjectArray<unsigned int> buf1Host;

	btOpenCLArray<unsigned int> buf2CL(g_context,g_queue,maxSize);
	btOpenCLArray<unsigned int> buf3CL(g_context,g_queue,maxSize);
	
	
	btPrefixScanCL* scan = new btPrefixScanCL(g_context,g_device,g_queue,maxSize);
		
	int dx = maxSize/NUM_TESTS;
	for(int iter=0; iter<NUM_TESTS; iter++)
	{
		int size = btMin( 128+dx*iter, maxSize );
		buf0Host.resize(size);
		buf1Host.resize(size);

		for(int i=0; i<size; i++) 
			buf0Host[i] = 1;
		
		buf2CL.copyFromHost( buf0Host);
	
		unsigned int sumHost, sumGPU;

		scan->executeHost(buf0Host, buf1Host, size, &sumHost );
		scan->execute( buf2CL, buf3CL, size, &sumGPU );

		buf3CL.copyToHost(buf0Host);
		
		TEST_ASSERT( sumHost == sumGPU );
		for(int i=0; i<size; i++) 
			TEST_ASSERT( buf1Host[i] == buf0Host[i] );
	}

	delete scan;

	TEST_REPORT( "scanTest" );
}


bool radixSortTest()
{
	TEST_INIT;
	
	int maxSize = 1024*256;

	btAlignedObjectArray<btSortData> buf0Host;
	buf0Host.resize(maxSize);
	btAlignedObjectArray<btSortData> buf1Host;
	buf1Host.resize(maxSize );
	btOpenCLArray<btSortData> buf2CL(g_context,g_queue,maxSize);

	btRadixSort32CL* sort = new btRadixSort32CL(g_context,g_device,g_queue,maxSize);

	int dx = maxSize/NUM_TESTS;
	for(int iter=0; iter<NUM_TESTS; iter++)
	{
		int size = btMin( 128+dx*iter, maxSize-512 );
		size = NEXTMULTIPLEOF( size, 512 );//not necessary
		
		buf0Host.resize(size);

		for(int i=0; i<size; i++)
		{
			btSortData v;
			v.m_key = getRandom(0,0xff);
			v.m_value = i;
			buf0Host[i] = v;
		}

		buf2CL.copyFromHost( buf0Host);
		

		sort->executeHost( buf0Host);
		sort->execute(buf2CL);

		buf2CL.copyToHost(buf1Host);
				
		for(int i=0; i<size; i++) 
		{
			TEST_ASSERT( buf0Host[i].m_value == buf1Host[i].m_value && buf0Host[i].m_key == buf1Host[i].m_key );
		}
	}

	delete sort;

	TEST_REPORT( "radixSort" );

	return g_testFailed;
}


int main()
{
	initCL(-1,-1);

	fillIntTest();

	boundSearchTest();

	prefixScanTest();

	radixSortTest();

	exitCL();

	printf("End, press <enter>\n");
	getchar();
}

