#ifndef FATXIO_H
#define FATXIO_H

#include "DeviceIO.h"
#include "FileIO.h"
#include "../Fatx/FatxConstants.h"

#include <iostream>
#include <vector>

struct Range
{
    UINT64 start;
    UINT64 len;
};

class XBOXINTERNALSSHARED_EXPORT FatxIO : public BaseIO
{
public:
    FatxIO(DeviceIO *device, FatxFileEntry *entry);

    // get the current file entry
    FatxFileEntry GetFatxFileEntry();

    // set the position
    void SetPosition(UINT64 position, std::ios_base::seek_dir dir = std::ios_base::beg);

    // get the current position
    UINT64 GetPosition();

    // read bytes at the current position
    void ReadBytes(BYTE *outBuffer, DWORD len);

    // write bytes at the current position
    void WriteBytes(BYTE *buffer, DWORD len);

    // save the file to disk
    void SaveFile(std::string savePath, void(*progress)(void*, DWORD, DWORD) = NULL, void *arg = NULL);

    // does nothing, required implementation
    void Flush();

    // does nothing, required implementation
    void Close();

    // expands the cluster chain if necessary
    void AllocateMemory(DWORD byteAmount);

    // convert a cluster to an offset
    static INT64 ClusterToOffset(Partition *part, DWORD cluster);

private:
    // find count amount of free custers
    std::vector<DWORD> getFreeClusters(Partition *part, DWORD count);

    // find count amount of free custers
    void rewriteEntryToDisk(FatxFileEntry *entry, std::vector<DWORD> clusterChain);

    // writes the cluster chain (and links them correctly) starting from startingCluster
    void writeClusterChain(Partition *part, DWORD startingCluster, std::vector<DWORD> clusterChain);

    // sets all the clusters equal to value
    void setAllClusters(Partition *part, std::vector<DWORD> clusters, DWORD value);

    DeviceIO *device;
    FatxFileEntry *entry;
    UINT64 pos;
    DWORD maxReadConsecutive;
};

#endif // FATXIO_H