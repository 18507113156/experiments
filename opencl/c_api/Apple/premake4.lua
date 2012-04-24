	
	hasCL = findOpenCL_Apple()
	
	if (hasCL) then

		project "OpenCL_C_API_Test"

		initOpenCL_Apple()
	
		kind "ConsoleApp"
		targetdir "../../../bin"

		includedirs {"../../../bullet2"}
		links {"bullet2"}

		language "C"
		files {
			"../main.c",
		}

		language "C++"
		files {
			"../../broadphase_benchmark/btRadixSort32CL.cpp",
			"../../basic_initialize/btOpenCLUtils.cpp",
			"../../basic_initialize/btOpenCLUtils.h",
			"../btbDeviceCL.cpp",
			"../btbDeviceCL.h",
			"../btbPlatformDefinitions.h",
			"../btcFindPairs.cpp",
			"../btcFindPairs.h",
			"../Test_FindPairs.cpp",
			"../Test_FindPairs.h"
			
		}
		
	end
