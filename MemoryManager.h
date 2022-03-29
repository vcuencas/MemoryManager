//
// Created by vcs04 on 3/25/2022.
//
#pragma once
#include <iostream>
#include <functional>
#include <list>
using namespace std;

struct block
{
    bool occupied;
    int offSet;
    int size;
    char* address;
    void operator<(const block& rhs);
    bool operator==(const block &rhs);
};

class MemoryManager {
public:
    unsigned int wordSize;
    int memoryLimit;
    function<int(int, void*)> allocator;

    MemoryManager(unsigned wordSize, function<int(int, void*)> allocator);
    ~MemoryManager();
    void initialize(size_t sizeInWords);
    void shutdown();
    void *allocate(size_t sizeInBytes);
    void free(void *address);
    void setAllocator(function<int(int, void*)> allocator);
    int dumpMemoryMap(char *filename);
    void *getList();
    void *getBitmap();
    unsigned getWordSize();
    void *getMemoryStart();
    unsigned getMemoryLimit();
    std::list<block> tracker;
    char *bigBlock;
};



int bestFit(int sizeInWords, void *list);
int worstFit(int sizeInWords, void *list);
