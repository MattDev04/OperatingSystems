#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <ctime>
#include <algorithm>

#define BLOCK_SIZE 4096
#define MAX_NAME_SIZE 64
#define MAGIC_NUMBER 0xF11E

struct Superblock {
    unsigned int diskSize;
    unsigned int blockCount;
    unsigned int inodeCount;
    unsigned int magicNumber;
};

struct Inode {
    char name[MAX_NAME_SIZE];
    unsigned int size;
    unsigned int firstBlock;
    time_t createTime;
    bool valid;
};

struct DataBlock {
    char data[BLOCK_SIZE];
};

class VirtualFileSystem {
private:
    std::fstream file;
    Superblock superblock;
    std::vector<Inode> inodes;
    std::vector<int> fat;
    std::vector<DataBlock> dataBlocks;
    bool isOpen = false;

public:
    VirtualFileSystem() = default;

    bool create(const std::string& name, unsigned int size) {
        unsigned int blockCount = size / BLOCK_SIZE;
        unsigned int inodeCount = blockCount / 10;

        superblock = {size, blockCount, inodeCount, MAGIC_NUMBER};

        inodes = std::vector<Inode>(inodeCount);
        fat = std::vector<int>(blockCount, -2);
        dataBlocks = std::vector<DataBlock>(blockCount);

        file.open(name, std::ios::out | std::ios::binary);
        if (!file) return false;

        file.write(reinterpret_cast<char*>(&superblock), sizeof(Superblock));
        file.write(reinterpret_cast<char*>(inodes.data()), inodes.size() * sizeof(Inode));
        file.write(reinterpret_cast<char*>(fat.data()), fat.size() * sizeof(int));

        std::vector<char> zeroes(BLOCK_SIZE, 0);
        for (unsigned int i = 0; i < blockCount; i++) {
            file.write(zeroes.data(), BLOCK_SIZE);
        }

        file.close();
        return true;
    }

    bool open(const std::string& name) {
        file.open(name, std::ios::in | std::ios::out | std::ios::binary);
        if (!file) return false;

        file.read(reinterpret_cast<char*>(&superblock), sizeof(Superblock));
        if (superblock.magicNumber != MAGIC_NUMBER) {
            file.close();
            return false;
        }

        inodes = std::vector<Inode>(superblock.inodeCount);
        file.read(reinterpret_cast<char*>(inodes.data()), inodes.size() * sizeof(Inode));

        fat = std::vector<int>(superblock.blockCount);
        file.read(reinterpret_cast<char*>(fat.data()), fat.size() * sizeof(int));

        dataBlocks = std::vector<DataBlock>(superblock.blockCount);
        for (unsigned int i = 0; i < superblock.blockCount; i++) {
            file.read(dataBlocks[i].data, BLOCK_SIZE);
        }

        isOpen = true;
    return true;
}


    void close() {
        if (isOpen) {
            file.seekp(sizeof(Superblock), std::ios::beg);
            file.write(reinterpret_cast<char*>(inodes.data()), inodes.size() * sizeof(Inode));
            file.write(reinterpret_cast<char*>(fat.data()), fat.size() * sizeof(int));
            for(unsigned int i=0; i<superblock.blockCount; i++){
                file.write(dataBlocks[i].data, BLOCK_SIZE);
            }
            file.close();



            inodes.clear();
            fat.clear();
            dataBlocks.clear();
            isOpen = false;
        }
    }

    bool copyIntoVD(const std::string& sourceFile, const std::string& destFile) {
        if (!isOpen) {
            std::cout << "VFS is not open.\n";
            return false;
        }

        std::ifstream src(sourceFile, std::ios::binary | std::ios::ate);
        if (!src) return false;

        unsigned int fileSize = src.tellg();
        src.seekg(0, std::ios::beg);

        unsigned int requiredBlocks = (fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
        unsigned int freeBlocks = std::count(fat.begin(), fat.end(), -2);
        if (requiredBlocks > freeBlocks) return false;

        int inodeIndex = -1;
        for (unsigned int i = 0; i < inodes.size(); i++) {
            if (!inodes[i].valid) {
                inodeIndex = i;
                break;
            }
        }
        if (inodeIndex == -1) return false;

        std::vector<int> blocks;
        for (unsigned int i = 0; i < fat.size() && blocks.size() < requiredBlocks; i++) {
            if (fat[i] == -2) blocks.push_back(i);
        }

        for (unsigned int i = 0; i < blocks.size(); i++) {
            src.read(dataBlocks[blocks[i]].data, BLOCK_SIZE);
            unsigned int blockIndex = blocks[i];

            fat[blockIndex] = (i == blocks.size() - 1) ? -1 : blocks[i + 1];
        }

        Inode& inode = inodes[inodeIndex];
        strcpy(inode.name, destFile.c_str());
        inode.size = fileSize;
        inode.firstBlock = blocks[0];
        inode.createTime = time(nullptr);
        inode.valid = true;

        src.close();
        return true;
    }

    bool copyFromVD(const std::string& sourceFile, const std::string& destFile) {
        if (!isOpen) {
            std::cout << "VFS is not open.\n";
            return false;
        }

        int inodeIndex = -1;
        for (unsigned int i = 0; i < inodes.size(); i++) {
            if (inodes[i].valid && sourceFile == inodes[i].name) {
                inodeIndex = i;
                break;
            }
        }
        if (inodeIndex == -1) return false;

        std::ofstream dest(destFile, std::ios::binary);
        if (!dest) return false;

        int block = inodes[inodeIndex].firstBlock;
        unsigned int remaining = inodes[inodeIndex].size;

        while (block != -1) {
            unsigned int toRead = std::min(static_cast<unsigned int>(BLOCK_SIZE), remaining);
            dest.write(dataBlocks[block].data, toRead);
            remaining -= toRead;
            block = fat[block];
        }

        dest.close();
        return true;
    }

    void listFiles() {
        if (!isOpen) {
            std::cout << "VFS is not open.\n";
            return;
        }

        for (const auto& inode : inodes) {
            if (inode.valid) {
                std::cout << "Name: " << inode.name
                          << ", Size: " << inode.size
                          << ", Created: " << ctime(&inode.createTime);
            }
        }
    }

    bool removeFile(const std::string& name) {
        if (!isOpen) {
            std::cout << "VFS is not open.\n";
            return false;
        }

        int inodeIndex = -1;
        for (unsigned int i = 0; i < inodes.size(); i++) {
            if (inodes[i].valid && name == inodes[i].name) {
                inodeIndex = i;
                break;
            }
        }
        if (inodeIndex == -1) return false;

        int block = inodes[inodeIndex].firstBlock;
        while (block != -1) {
            int nextBlock = fat[block];
            fat[block] = -2;
            block = nextBlock;
        }

        inodes[inodeIndex] = Inode();
        return true;
    }

    void displayBlockMap() {
        if (!isOpen) {
            std::cout << "VFS is not open.\n";
            return;
        }

        std::cout << "Block Map (FAT): ";
        for (unsigned int i = 0; i < fat.size(); i++) {
            std::cout << (fat[i] == -2 ? "0" : "1");
        }
        std::cout << std::endl;

        std::cout << "Free Inodes: ";
        bool foundFreeInode = false;
        for (unsigned int i = 0; i < inodes.size(); i++) {
            if (!inodes[i].valid) {
                std::cout << i << " ";
                foundFreeInode = true;
            }
        }

        if (!foundFreeInode) {
            std::cout << "None";
        }

        std::cout << std::endl;
    }

    bool removeVFS(const std::string& name) {
    if (isOpen) {
        close();
    }
    if (std::remove(name.c_str()) == 0) {
        std::cout << "VFS removed successfully.\n";
        return true;
    } else {
        std::cout << "Error removing VFS.\n";
        return false;
    }
}
};

int main() {
    VirtualFileSystem vfs;
    std::string command;

    while (true) {
        std::cout << "Enter command: ";
        std::cin >> command;

        if (command == "createVFS") {
            std::string name;
            unsigned int size;
            std::cin >> name >> size;
            if (vfs.create(name, size)) std::cout << "VFS created.\n";
            else std::cout << "Error creating VFS.\n";
        } else if (command == "openVFS") {
            std::string name;
            std::cin >> name;
            if (vfs.open(name)) std::cout << "VFS opened.\n";
            else std::cout << "Error opening VFS.\n";
        } else if (command == "closeVFS") {
            vfs.close();
            std::cout << "VFS closed.\n";
        } else if (command == "copyIntoVD") {
            std::string src, dest;
            std::cin >> src >> dest;
            if (vfs.copyIntoVD(src, dest)) std::cout << "File copied to VFS.\n";
            else std::cout << "Error copying file to VFS.\n";
        } else if (command == "copyFromVD") {
            std::string src, dest;
            std::cin >> src >> dest;
            if (vfs.copyFromVD(src, dest)) std::cout << "File copied from VFS.\n";
            else std::cout << "Error copying file from VFS.\n";
        } else if (command == "ls") {
            vfs.listFiles();
        } else if (command == "rm") {
            std::string name;
            std::cin >> name;
            if (vfs.removeFile(name)) std::cout << "File removed.\n";
            else std::cout << "Error removing file.\n";
        } else if (command == "info") {
            vfs.displayBlockMap();
        } else if (command == "removeVFS") {
            std::string name;
            std::cin >> name;
            if (vfs.removeVFS(name)) std::cout << "VFS removed.\n";
            else std::cout << "Error removing VFS.\n";
        } else if (command == "exit") {
            vfs.close();
            break;
        } else {
            std::cout << "Unknown command.\n";
        }
    }

    return 0;
}
