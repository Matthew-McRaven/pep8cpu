#ifndef SPECIFICATION_H
#define SPECIFICATION_H

#include "mainmemory.h"
#include "cpupane.h"

class Specification
{
public:
    virtual ~Specification() { }
    Specification();
    virtual void setPrecondition(MainMemory *, CpuPane *) { }
    virtual bool testPostcondition(MainMemory *, CpuPane *) { }
};

class MemSpecification: public Specification {    
public:
    MemSpecification(int memoryAddress, int memoryValue);
    void setPrecondition(MainMemory *mainMemory, CpuPane *cpuPane);
    bool testPostcondition(MainMemory *mainMemory, CpuPane *cpuPane);
private:
    int memAddress;
    int memValue;
};

class RegSpecification: public Specification {
public:
    RegSpecification(Enu::EMnemonic registerAddress, int registerValue);
    void setPrecondition(MainMemory *mainMemory, CpuPane *cpuPane);
    bool testPostcondition(MainMemory *mainMemory, CpuPane *cpuPane);
private:
    Enu::EMnemonic regAddress;
    int regValue;
};

class StatusBitSpecification: public Specification {
public:
    StatusBitSpecification(Enu::EMnemonic statusBitAddress, bool statusBitValue);
    void setPrecondition(MainMemory *mainMemory, CpuPane *cpuPane);
    bool testPostcondition(MainMemory *mainMemory, CpuPane *cpuPane);
private:
    Enu::EMnemonic nzvcAddress;
    bool nzvcValue;
};

#endif // SPECIFICATION_H
