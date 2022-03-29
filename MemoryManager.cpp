//
// Created by vcs04 on 3/25/2022.
//
#include <cstring>
#include "MemoryManager.h"
#include <list>
#include <unistd.h>
#include <functional>
#include <math.h>
#include <vector>
#include <fcntl.h>
#include <algorithm>

using namespace std;

MemoryManager::MemoryManager(unsigned wordSize, function<int(int, void*)> allocator) {
    this->wordSize = wordSize;
    this->allocator = allocator;
    bigBlock = nullptr;
}

MemoryManager::~MemoryManager() {
    shutdown();
}

void MemoryManager::initialize(size_t sizeInWords) {
    shutdown();

    // dynamically allocated array to the total size of the block
    if (sizeInWords > 65536)
        memoryLimit = sizeInWords;
    else {
        memoryLimit = wordSize * sizeInWords;
        //memoryLimit = sizeInBytes;
    }

    if (bigBlock == nullptr)
        bigBlock = new char[memoryLimit];

    //pass in a big empty block to the list
    block b1 = {};
    b1.occupied = false;
    b1.offSet = 0;
    b1.size = sizeInWords;
    b1.address = &bigBlock[0 * wordSize];
    tracker.push_back(b1);
}

void MemoryManager::shutdown() {
    if (bigBlock != nullptr) {
//        bigBlock = nullptr;
        delete[] bigBlock;
        tracker.clear();
    }
    bigBlock = nullptr;
}

void *MemoryManager::allocate(size_t sizeInBytes) {
    //calculate sizeInWords to pass into the allocator function
    int sizeInWords = sizeInBytes / wordSize + ((sizeInBytes % wordSize) != 0);

    if (sizeInBytes > memoryLimit)
        return nullptr;

    uint16_t *arr = static_cast<uint16_t *>(getList());
    int wordOffset = allocator(sizeInWords, arr);

    // since using getList, delete arr to avoid leak.
    //arr = nullptr;
    delete[] arr;

    // check if memory is available
    if (wordOffset == -1)
        return nullptr;

    // create new block
    block newBlock = {};
    newBlock.occupied = true;
    newBlock.offSet = wordOffset;
    newBlock.size = sizeInWords;// previously
    newBlock.address = &bigBlock[wordOffset * wordSize];

    // iterate through the list to find the "index" to insert new block
    for (auto it = tracker.begin(); it != tracker.end(); it++) {
        // change hole offset and size, and then insert.
        if (it->offSet == newBlock.offSet) {
            it->offSet = sizeInWords + wordOffset;
            it->size -= sizeInWords;

            tracker.insert(it, newBlock);
            next(it);
            if (it->size == 0) {
                // not erase it on loop
                tracker.erase(it);
                break;
            }
        }

    }

    // return the "supposed" allocated address of the new block
    return newBlock.address;
}

void MemoryManager::free(void *address) {
    //*address is the location of the wanted to be deleted item from allocate
    // address is &bigBlock[sizeInBytes]

    bool deletion = false;
    for (auto it = tracker.begin(); it != tracker.end(); it++) {
        if (it->address == address) {
            it->occupied = false;

            // Combines hole with the next block
            if (next(it) != tracker.end() && !next(it)->occupied) {
                it->size += next(it)->size;
                tracker.erase(next(it));
                deletion = true;
            }
            // Combine holes with the prev block
            if (prev(it)->offSet != 0 && !prev(it)->occupied) {
                prev(it)->size += it->size;
                tracker.erase(it);
                deletion = true;
            }
        }
        if (deletion)
            break;
    }
}

void MemoryManager::setAllocator(function<int(int, void *)> allocator) {
    this->allocator = allocator;
}

// TODO
int MemoryManager::dumpMemoryMap(char *filename) {
    // create the block array that will be written into the file
    std::list<block> holesList = {};
    //char buffer[128];

    for (auto it = tracker.begin(), end = tracker.end(); it != end; ++it) {
        // determine what are holes/what is occupied.
        if (!it->occupied) {
            block newBlock;
            newBlock.size = it->size; //sizeInWords
            newBlock.offSet = it->offSet;
            newBlock.occupied = false;

            holesList.push_back(newBlock);
        }
    }
     //check if list is empty
//    if (holesList.empty())
//        return nullptr;

    uint16_t sizeArr = holesList.size() * 2;
    //cout << sizeArr;
    int arr[sizeArr];
    string dumpMem = "";

    int i = 0;
    for (auto it = holesList.begin(), end = holesList.end(); it != end; ++it) {
        arr[i] = it->offSet;
        //cout << arr[i] << endl;
        arr[i + 1] = it->size;
        //cout << arr[i+1] << endl;
        i+= 2;

    }


    for (int i = 0; i <= sizeArr - 2; i+=2) {
//        if (i != 0)
//            dumpMem += " [";
//        else
//            dumpMem += "[";
        dumpMem += "[";
        dumpMem += to_string(arr[i]);
        dumpMem += ", " ;
        dumpMem += to_string(arr[i+1]);

        if (i == sizeArr - 2) {
            dumpMem += "]";
            //cout << "i= " << i << endl;
        }
        else
               dumpMem += "] - ";
    }
//    dumpMem += "]";

    int n = dumpMem.length();
    char dumpArr[n];

    strcpy(dumpArr, dumpMem.c_str());

    //dumpMem = to_string(arr[1]);
    //cout << dumpMem << endl;

    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if (fd == -1) {
        //cout << "not opening" << endl;
        return -1;
    }
    else {
        //cout << "I am open!" << endl;
        write(fd, dumpArr, n );
        close(fd);
        return 0;
    }
}

void *MemoryManager::getList() {
    std::list<block> holesList = {};

    for (auto it = tracker.begin(), end = tracker.end(); it != end; ++it) {
        // determine what are holes/what is occupied.
        if (!it->occupied) {
            block newBlock;
            newBlock.size = it->size; //sizeInWords
            newBlock.offSet = it->offSet;
            newBlock.occupied = false;

            holesList.push_back(newBlock);
        }
    }
    // check if list is empty
    if (holesList.empty())
        return nullptr;

    uint16_t sizeArr = holesList.size() * 2 + 1;
    uint16_t *arr = new uint16_t[sizeArr];

    arr[0] = (uint16_t) holesList.size();

    int i = 1;
    for (auto it = holesList.begin(), end = holesList.end(); it != end; ++it) {
        arr[i] = it->offSet;
        arr[i + 1] = it->size;
        i+= 2;
    }
    return arr;
}

void *MemoryManager::getBitmap() {
    vector<string> binaryBitMap = {};
    vector<string> decimalBitMap = {};
    vector<int> decimalConvertedBitMap = {};
    int sizeInWords = 0;

    // 1. transform bits allocated/empty to binary
    for (auto it = tracker.begin(), end = tracker.end(); it != end; ++it)  {
        sizeInWords = it->size; // 1 byte = 8 bits
        if (it->occupied) {
            for (int i = 0; i < sizeInWords; i++) {
                binaryBitMap.emplace_back("1");
            }
        }
        else {
            for (int i = 0; i < sizeInWords; i++) {
                binaryBitMap.emplace_back("0");
            }
        }
    }

    // 2. divide into 8
    string convertString = "";
    for(int i = 0, a = 0; i < binaryBitMap.size(); i++) {
        if (i / 8 == a) {
            string temp = binaryBitMap.at(i);
            convertString += temp;
        }
        else {
            decimalBitMap.push_back(convertString);
            convertString = "";
            a++;
            i--;
        }
    }
    if (convertString != "")
        decimalBitMap.push_back(convertString);

    // 3. reverse to little endian
    reverse(decimalBitMap.begin(), decimalBitMap.end());

    // 4. flip/mirror index on the vector
    for(int i = 0, a = 0; i < decimalBitMap.size(); i++) {
        reverse(decimalBitMap.at(i).begin(),decimalBitMap.at(i).end());
    }

    // 5. convert from int binary to decimal
    for(int i = 0, a = 0; i < decimalBitMap.size(); i++) {
        int decimalNum = stoi(decimalBitMap.at(i),nullptr,2);
        decimalConvertedBitMap.push_back(decimalNum);
    }
    reverse(decimalConvertedBitMap.begin(), decimalConvertedBitMap.end());

    size_t sizeBitMapBefore = decimalConvertedBitMap.size();

    string toPadSize = to_string(sizeBitMapBefore);

    cout << toPadSize.size() << " " << decimalConvertedBitMap.size() << endl;
    size_t arrSize = 2 + decimalConvertedBitMap.size();
    auto *arr = new uint8_t [arrSize];

    arr[0] = (uint8_t) stoi(toPadSize.substr(0,2));
    arr[1] = 0;

    for (int i = 2, a = 0; i < arrSize; i++, a++) {
        arr[i] = decimalConvertedBitMap[a];
    }
    return arr;
}

//returns the wordSize from the constructor
unsigned MemoryManager::getWordSize() {
    return wordSize;
}

void *MemoryManager::getMemoryStart() {
    return bigBlock;
}

unsigned MemoryManager::getMemoryLimit() {
    return memoryLimit;
}

int bestFit(int sizeInWords, void *list) {
    // searches the entire list and takes the smallest hole that is adequate.
    // a hole that is close to the actual size needed.

    // receiving a list like
    // [numOfHoles, offSet1, hole1, offSet2, hole2...]
    auto arr = static_cast<uint16_t *>(list);
    if (arr == nullptr) {
        // no holes
        return -1;
    }
    int sizeArr = arr[0] * 2 + 1;
    int bestHoleIndex = 0, bestHoleSize = 0;
    bool holeExist = false;
    for (int i = 2; i < sizeArr; i+=2) {
        if (arr[i] == sizeInWords) {
            bestHoleIndex = i;
            bestHoleSize = arr[i];
            holeExist = true;
        } else if (!holeExist && arr[i] > sizeInWords) {
            bestHoleIndex = i;
            bestHoleSize = arr[i];
            holeExist = true;
        } else if (bestHoleSize - sizeInWords > arr[i] - sizeInWords) {
            bestHoleIndex = i;
            bestHoleSize = arr[i];
            holeExist = true;
        }

    }
    if (holeExist)
        return arr[bestHoleIndex - 1];
    else
        return -1;
}

int worstFit(int sizeInWords, void *list) {
    // searches the entire list and takes the largest available hole.
    auto arr = static_cast<uint16_t *>(list);

    if (arr == nullptr) {
        // no holes
        return -1;
    }

    int sizeArr = arr[0] * 2 + 1;
    int worstHoleIndex = 0, worstHoleSize = 0;
    bool holeExist = false;

    for (int i = 2; i < sizeArr; i += 2) {
        if (arr[i] > sizeInWords) {
                worstHoleIndex = i;
                worstHoleSize = arr[i];
                holeExist = true;
        }
        else if (arr[i] > worstHoleSize) {
            worstHoleSize = arr[i];
            worstHoleIndex = i;
            holeExist = true;
        }
    }
    if (holeExist)
        return arr[worstHoleIndex - 1];
    else
        return -1;
}

void block::operator<(const block &rhs) {
    offSet < rhs.offSet;
}

bool block::operator==(const block &rhs) {
    size = rhs.size;
    offSet = rhs.offSet;
    occupied = rhs.occupied;
    return true;
}
