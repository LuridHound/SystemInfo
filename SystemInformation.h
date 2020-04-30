#ifndef SYSTEMINFORMATION_H
#define SYSTEMINFORMATION_H

//  For more information about cpuid check the following documents :
//  https://software.intel.com/sites/default/files/managed/c5/15/architecture-instruction-set-extensions-programming-reference.pdf
//  https://software.intel.com/sites/default/files/managed/ad/01/253666-sdm-vol-2a.pdf


#ifdef __linux__

    #include <sys/sysinfo.h>
    #include <sys/statvfs.h>
    #include <unistd.h>
    #include <cpuid.h>

#endif


#ifdef _WIN32

    #include <windows.h>
    #include <Powrprof.h>
    #pragma comment(lib, "Powrprof.lib")

#endif

#if !defined __linux__ && !defined _WIN32
    #error "This platform is not supported."
#endif


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


struct SystemInformation
{
    struct CPU
            {
        uint32_t coresNumber;
        uint32_t frequency;

        uint32_t family;
        uint32_t model;

        uint8_t vendor[12];
        uint8_t name[48];

        uint8_t MMX;
        uint8_t SSE;
        uint8_t SSE2;
        uint8_t SSE3;
        uint8_t SSE4_1;
        uint8_t SSE4_2;
        uint8_t AVX;

            } cpu;

    struct RAM
            {
        // In MB.
        size_t total;
        size_t free;
        // How much memory is in use. Value in range  [0;100]
        uint32_t inUse;

            } ram;


    struct HDD
            {
        // In MB.
        size_t total;
        size_t free;
        // How much memory is in use. Value in range  [0;100]
        uint32_t inUse;
            } hdd;

};


bool getRamInfo(struct SystemInformation* systemInformation);
bool getHDDInformation(struct SystemInformation* systemInformation);
bool getProcInformation(struct SystemInformation* systemInformation);


bool getSystemInformation(struct SystemInformation* systemInformation);


//  Each cpuid call return values from the registers.
//
void call_cpuid(unsigned int value, int* registers);


#ifdef SYSTEM_INFORMATION_IMPLEMENTATION
#ifdef __linux__


    bool getRamInfo(struct SystemInformation* systemInformation)
    {
        struct sysinfo systemInfo;

        if (sysinfo(&systemInfo) == -1)
        {
            return false;
        }

        systemInformation->ram.total = systemInfo.totalram >> (10 + 10);

        systemInformation->ram.free = systemInfo.freeram >> (10 + 10);

        systemInformation->ram.inUse = (uint32_t)(100 * (1.0f- (systemInformation->ram.free / float(systemInformation->ram.total))));

        return true;
    }


    bool getHDDInformation(struct SystemInformation* systemInformation)
    {
        struct statvfs hddInfo;

        if (statvfs("./", &hddInfo) == -1)
        {
            return false;
        }

        systemInformation->hdd.total = (hddInfo.f_frsize * hddInfo.f_blocks) >> (10 + 10);

        systemInformation->hdd.free = (hddInfo.f_frsize * hddInfo.f_bfree) >> (10 + 10);

        systemInformation->hdd.inUse = (uint32_t)(100 * (1.0f - (systemInformation->hdd.free / float(systemInformation->hdd.total))));

        return true;
    }


    bool getProcInformation(struct SystemInformation* systemInformation)
    {
        systemInformation->cpu.coresNumber = sysconf(_SC_NPROCESSORS_ONLN);

        FILE* file;
        file = fopen("/proc/cpuinfo", "r");

        //  If descriptor wasn't given.
        if (!file)
        {
            return false;
        }

        char* line = NULL;
        size_t lineLength = 0;

        while (getline(&line, &lineLength, file) != -1)
        {
            // 7 == length("cpu MHz")
            if (memcmp(&line, "cpu MHz", 7) == 0)
            {
                systemInformation->cpu.frequency = atol(strchr(line, ':'));
            }
        }

        fclose(file);

        int registers[4];

        call_cpuid(0x00000000, registers);

        for (int i = 0; i < 4; i++)
        {
            systemInformation->cpu.vendor[0 + i] = (registers[1] >> (i * 8)) & 0xFF;
            systemInformation->cpu.vendor[4 + i] = (registers[3] >> (i * 8)) & 0xFF;
            systemInformation->cpu.vendor[8 + i] = (registers[2] >> (i * 8)) & 0xFF;
        }

        call_cpuid(0x80000002, registers); memcpy(systemInformation->cpu.name +  0, registers, 16);
        call_cpuid(0x80000003, registers); memcpy(systemInformation->cpu.name + 16, registers, 16);
        call_cpuid(0x80000004, registers); memcpy(systemInformation->cpu.name + 32, registers, 16);

        call_cpuid(0x00000001, registers);

        systemInformation->cpu.family = ((registers[0] >>  8) & 0xF) +  ((registers[0] >> 20) & 0xF);
        systemInformation->cpu.model  = ((registers[0] >>  4) & 0xF) + (((registers[0] >> 16) & 0xF) << 4);
        systemInformation->cpu.MMX = (registers[3] >> 23) & 0x1;
        systemInformation->cpu.SSE = (registers[3] >> 25) & 0x1;
        systemInformation->cpu.SSE2 = (registers[3] >> 26) & 0x1;
        systemInformation->cpu.SSE3 = (registers[2] >> 0) & 0x1;
        systemInformation->cpu.SSE4_1 = (registers[2] >> 19) & 0x1;
        systemInformation->cpu.SSE4_2 = (registers[2] >> 20) & 0x1;
        systemInformation->cpu.AVX = (registers[2] >> 28) & 0x1;

        return true;
    }


#endif


#ifdef _WIN32


	bool getRamInfo(struct SystemInformation* systemInformation)
	{
		MEMORYSTATUSEX mem;
		//  https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/ns-sysinfoapi-memorystatusex
		//  You must set it before calling "GlobalMemoryStatusEx".
		mem.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&mem);

		systemInformation->ram.total = mem.ullTotalPhys >> (10 + 10);
		systemInformation->ram.free = mem.ullAvailPhys >> (10 + 10);

        systemInformation->ram.inUse = (uint32_t)(100 * (1.0f- (systemInformation->ram.free / float(systemInformation->ram.total))));

		return true;
	}


	bool getHDDInformation(struct SystemInformation* systemInformation)
	{
		ULARGE_INTEGER free_bytes_avail;
		ULARGE_INTEGER total_bytes;
		ULARGE_INTEGER total_free_bytes;

		if (!GetDiskFreeSpaceEx(NULL, &free_bytes_avail, &total_bytes, &total_free_bytes))
		{
			return false;
		}

		systemInformation->hdd.total = total_bytes.QuadPart >> (10 + 10);
		systemInformation->hdd.free = free_bytes_avail.QuadPart >> (10 + 10);

        systemInformation->hdd.inUse = (uint32_t)(100 * (1.0f - (systemInformation->hdd.free / float(systemInformation->hdd.total))));

		return true;
	}


	bool getProcInformation(struct SystemInformation* systemInformation)
	{
		SYSTEM_INFO sys;
		GetSystemInfo(&sys);

		systemInformation->cpu.count = sys.dwNumberOfProcessors;

		int cpu[4];

		call_cpuid(0x00000000, cpu);

		for (int i = 0; i < 4; i++)
		{
			systemInformation->cpu.vendor[0 + i] = (cpu[1] >> (i * 8)) & 0xFF;
			systemInformation->cpu.vendor[4 + i] = (cpu[3] >> (i * 8)) & 0xFF;
			systemInformation->cpu.vendor[8 + i] = (cpu[2] >> (i * 8)) & 0xFF;
		}

		call_cpuid(0x80000002, cpu);
		memcpy(systemInformation->cpu.name +  0, cpu, 16);

		call_cpuid(0x80000003, cpu);
		memcpy(systemInformation->cpu.name + 16, cpu, 16);

		call_cpuid(0x80000003, cpu);
		memcpy(systemInformation->cpu.name + 32, cpu, 16);

		call_cpuid(0x00000001, cpu);

		systemInformation->cpu.family = ((cpu[0] >>  8) & 0xF) +  ((cpu[0] >> 20) & 0xF);
		systemInformation->cpu.model  = ((cpu[0] >>  4) & 0xF) + (((cpu[0] >> 16) & 0xF) << 4);
		systemInformation->cpu.MMX    =  (cpu[3] >> 23) & 0x1;
		systemInformation->cpu.SSE    =  (cpu[3] >> 25) & 0x1;
		systemInformation->cpu.SSE2   =  (cpu[3] >> 26) & 0x1;
		systemInformation->cpu.SSE3   =  (cpu[2] >>  0) & 0x1;
		systemInformation->cpu.SSE41  =  (cpu[2] >> 19) & 0x1;
		systemInformation->cpu.SSE42  =  (cpu[2] >> 20) & 0x1;
		systemInformation->cpu.AVX    =  (cpu[2] >> 28) & 0x1;

		struct PPIStruct
		{
			ULONG  number;
			ULONG  maxMhz;
			ULONG  currentMhz;
			ULONG  MhzLimit;
			ULONG  maxIdleState;
			ULONG  currentIdleState;
		} PROCESSOR_POWER_INFORMATION, *PPI;

		BYTE* buf = (BYTE*)malloc(sizeof(PROCESSOR_POWER_INFORMATION) * sys.dwNumberOfProcessors);
		if (buf == NULL)
		{
		    return false;
		}

		CallNtPowerInformation(ProcessorInformation, NULL, 0, buf, sizeof(PROCESSOR_POWER_INFORMATION) * sys.dwNumberOfProcessors);
		PPI = (PPIStruct*)buf;

		if (PPI != NULL)
		{
		    info->cpu.frequency = PPI->maxMhz;
		}

		if (buf != NULL)
		{
		    free(buf);
		}

		return true;
	}


#endif


//  false - during execution something went wrong and information may be incomplete(or absent at all)
//  true - information is stored in systemInformation
bool getSystemInformation(struct SystemInformation* systemInformation)
{
    if (!systemInformation)
    {
        return false;
    }

    // Clear the previous state of the system information.
    memset(systemInformation, 0, sizeof(systemInformation));

    return
        getRamInfo(systemInformation) &&
        getHDDInformation(systemInformation) &&
        getProcInformation(systemInformation);
}

void call_cpuid(unsigned int value, int* registers)
{

#ifdef __linux__
    __cpuid(value, registers[0], registers[1], registers[2], registers[3]);
#endif

#ifdef _WIN32
    __cpuid(registers, value);
#endif

}

#endif  // SYSTEM_INFORMATION_IMPLEMENTATION

#endif  // SYSTEMINFORMATION_H