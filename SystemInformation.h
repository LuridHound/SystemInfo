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

#ifndef __linux__
    #error "This platform is not supported"
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

bool getRamInfo(SystemInformation* systemInformation);
bool getHDDInformation(SystemInformation* systemInformation);
bool getProcInformation(SystemInformation* systemInformation);


bool getSystemInformation(SystemInformation& systemInformation);

//  Each cpuid call return values from the registers.
//
void call_cpuid(unsigned int value, int* registers);

#ifdef SYSTEM_INFORMATION_IMPLEMENTATION
#ifdef __linux__


    bool getRamInfo(SystemInformation* systemInformation)
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


    bool getHDDInformation(SystemInformation* systemInformation)
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


    bool getProcInformation(SystemInformation* systemInformation)
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


    void call_cpuid(unsigned int value, int* registers)
    {
        __cpuid(value, registers[0], registers[1], registers[2], registers[3]);
    }


#endif


//  false - during execution something went wrong and information may be incomplete(or absent at all)
//  true - information is stored in systemInformation
bool getSystemInformation(SystemInformation* systemInformation)
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


#endif  // SYSTEM_INFORMATION_IMPLEMENTATION

#endif  // SYSTEMINFORMATION_H