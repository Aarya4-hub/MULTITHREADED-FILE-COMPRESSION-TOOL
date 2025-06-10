#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <zlib.h>

struct Chunk {
    std::vector<unsigned char> data;
    size_t index;
};

void compressChunk(Chunk chunk, std::queue<Chunk>& compressedQueue,
                   std::mutex& queueMutex, std::condition_variable& queueCv)
{
    std::vector<unsigned char> compressedData(compressBound(chunk.data.size()));
    uLongf compressedSize = compressedData.size();

    int res = compress2(reinterpret_cast<Bytef*>(compressedData.data()), &compressedSize,
                        reinterpret_cast<Bytef*>(chunk.data.data()), chunk.data.size(), Z_BEST_COMPRESSION);

    if (res != Z_OK) {
        std::cerr << "Compression failed for chunk " << chunk.index << std::endl;
        return;
    }

    compressedData.resize(compressedSize);
    chunk.data = std::move(compressedData);

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        compressedQueue.push(chunk);
    }
    queueCv.notify_one();
}

void writerThread(std::queue<Chunk>& queue, std::mutex& queueMutex,
                  std::condition_variable& queueCv, std::ofstream& outputFile,
                  bool& finished, bool isCompression)
{
    size_t nextIndex = 0;
    std::map<size_t, Chunk> outOfOrderBuffer;

    while (!finished || !queue.empty() || !outOfOrderBuffer.empty()) {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCv.wait(lock, [&] { return !queue.empty() || finished; });

        while (!queue.empty()) {
            Chunk current = queue.front();
            queue.pop();

            if (current.index == nextIndex) {
                if (isCompression) {
                    size_t size = current.data.size();
                    outputFile.write(reinterpret_cast<char*>(&current.index), sizeof(current.index));
                    outputFile.write(reinterpret_cast<char*>(&size), sizeof(size));
                }
                outputFile.write(reinterpret_cast<char*>(current.data.data()), current.data.size());
                nextIndex++;

                while (outOfOrderBuffer.count(nextIndex)) {
                    Chunk buffered = outOfOrderBuffer[nextIndex];
                    if (isCompression) {
                        size_t s = buffered.data.size();
                        outputFile.write(reinterpret_cast<char*>(&buffered.index), sizeof(buffered.index));
                        outputFile.write(reinterpret_cast<char*>(&s), sizeof(s));
                    }
                    outputFile.write(reinterpret_cast<char*>(buffered.data.data()), buffered.data.size());
                    outOfOrderBuffer.erase(nextIndex);
                    nextIndex++;
                }
            } else {
                outOfOrderBuffer[current.index] = current;
            }
        }
    }
}

void decompressChunk(Chunk chunk, std::queue<Chunk>& decompressedQueue,
                     std::mutex& queueMutex, std::condition_variable& queueCv)
{
    std::vector<unsigned char> decompressedData(chunk.data.size() * 10);
    uLongf decompressedSize = decompressedData.size();

    int res = uncompress(reinterpret_cast<Bytef*>(decompressedData.data()), &decompressedSize,
                         reinterpret_cast<Bytef*>(chunk.data.data()), chunk.data.size());

    if (res != Z_OK) {
        std::cerr << "Decompression failed for chunk " << chunk.index << std::endl;
        return;
    }

    decompressedData.resize(decompressedSize);
    chunk.data = std::move(decompressedData);

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        decompressedQueue.push(chunk);
    }
    queueCv.notify_one();
}

void compression(const std::string& inputFile, const std::string& outputFile) {
    std::ifstream inFile(inputFile, std::ios::binary);
    std::ofstream outFile(outputFile, std::ios::binary);

    if (!inFile.is_open() || !outFile.is_open()) {
        std::cerr << "Error opening input or output file." << std::endl;
        return;
    }

    std::queue<Chunk> compressedQueue;
    std::mutex queueMutex;
    std::condition_variable queueCv;
    bool compressionFinished = false;

    std::vector<std::thread> compressorThreads;
    size_t chunkSize = 1024 * 1024;
    size_t chunkIndex = 0;

    while (true) {
        Chunk chunk;
        chunk.data.resize(chunkSize);
        inFile.read(reinterpret_cast<char*>(chunk.data.data()), chunkSize);
        std::streamsize bytesRead = inFile.gcount();

        if (bytesRead <= 0) break;
        chunk.data.resize(bytesRead);
        chunk.index = chunkIndex++;

        compressorThreads.emplace_back(compressChunk, chunk, std::ref(compressedQueue),
                                       std::ref(queueMutex), std::ref(queueCv));
    }

    std::thread writer(writerThread, std::ref(compressedQueue), std::ref(queueMutex),
                       std::ref(queueCv), std::ref(outFile), std::ref(compressionFinished), true);

    for (auto& t : compressorThreads) {
        if (t.joinable()) t.join();
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        compressionFinished = true;
    }
    queueCv.notify_one();

    if (writer.joinable()) writer.join();

    inFile.close();
    outFile.close();
    std::cout << "Compression completed successfully." << std::endl;
}

void Decompression(const std::string& inputFile, const std::string& outputFile) {
    std::ifstream inFile(inputFile, std::ios::binary);
    std::ofstream outFile(outputFile, std::ios::binary);

    if (!inFile.is_open() || !outFile.is_open()) {
        std::cerr << "Error opening input or output file." << std::endl;
        return;
    }

    std::queue<Chunk> decompressedQueue;
    std::mutex queueMutex;
    std::condition_variable queueCv;
    bool decompressionFinished = false;

    std::vector<std::thread> decompressorThreads;

    while (true) {
        size_t chunkIndex, size;
        if (!inFile.read(reinterpret_cast<char*>(&chunkIndex), sizeof(chunkIndex))) break;
        if (!inFile.read(reinterpret_cast<char*>(&size), sizeof(size))) break;

        Chunk chunk;
        chunk.data.resize(size);
        inFile.read(reinterpret_cast<char*>(chunk.data.data()), size);
        chunk.index = chunkIndex;

        decompressorThreads.emplace_back(decompressChunk, chunk, std::ref(decompressedQueue),
                                        std::ref(queueMutex), std::ref(queueCv));
    }

    std::thread writer(writerThread, std::ref(decompressedQueue), std::ref(queueMutex),
                       std::ref(queueCv), std::ref(outFile), std::ref(decompressionFinished), false);

    for (auto& t : decompressorThreads) {
        if (t.joinable()) t.join();
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        decompressionFinished = true;
    }
    queueCv.notify_one();

    if (writer.joinable()) writer.join();

    inFile.close();
    outFile.close();
    std::cout << "Decompression completed successfully." << std::endl;
}

int main() {
    int choice;
    std::cout << "Select operation:\n1. Compress File\n2. Decompress File\nChoice: ";
    std::cin >> choice;

    switch (choice) {
        case 1:
            Compression("input.txt", "output.z");
            break;
        case 2:
            Decompression("output.z", "decompressed.txt");
            break;
        default:
            std::cout << "Invalid choice!" << std::endl;
    }

    return 0;
}