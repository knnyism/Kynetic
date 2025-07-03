
#include "Engine/App.h"

std::vector<char> ReadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        std::println("Failed to open file: {}\nCurrent working directory: {}",
            filename, std::filesystem::current_path().string());
        throw std::runtime_error("failed to open file: " + filename);
    }

    auto fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

    file.close();

    return buffer;
}

int main() {
    auto *app = new Kynetic::App();

    app->Start();

    delete app;

    return 0;
}
