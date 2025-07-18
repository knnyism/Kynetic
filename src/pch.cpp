//
// Created by kenny on 3-7-25.
//

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

std::vector<char> read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        std::println("Failed to open file: {}\nCurrent working directory: {}",
            filename, std::filesystem::current_path().string());
        throw std::runtime_error("failed to open file: " + filename);
    }

    const auto fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

    file.close();

    return buffer;
}