

#if 0
// 你的任务是编写一个多线程的文件搜索程序，该程序能够在指定的目录下递归地搜索指定类型的文件并输出结果。具体要求如下：

// 创建一个结构体表示文件搜索的配置，包括搜索的根目录、搜索的文件类型、最大并发数等。
// 使用多线程并发地递归搜索目录下的文件，找到指定类型的文件并输出文件路径。
// 控制并发数，避免过多的线程占用系统资源。
// 设置最大搜索深度，避免无限制地搜索。
// 可以选择是否跳过指定目录或文件的搜索。 以下是任务要求的详细说明：   


// 可以使用 C++11 中的 std::thread 和 std::mutex 等多线程库，也可以使用 Boost 等第三方库。
// 可以使用 C++17 中的 std::filesystem 库来处理文件和目录，也可以使用 Boost.Filesystem 等第三方库。
// 需要注意线程安全和资源占用问题，避免死锁、竞争条件等问题。
// 可以为搜索结果添加排序、去重等功能。(可选)

#endif


#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <filesystem>
#include <atomic>
#include <chrono>
#include <condition_variable>

struct SearchConfig {
    std::string root_path;
    std::string file_type;
    int max_concurrency;
    int max_depth;
    bool skip_hidden;
    std::vector<std::string> skip_paths;
};

class FileSearch {
public:
    FileSearch(const SearchConfig& config): config_(config), active_threads_(0) {}

    void start() {
        std::unique_lock<std::mutex> lock(mutex_);
        search_directory(config_.root_path, 0);
        cv_.wait(lock, [this] { return active_threads_ == 0; });
    }

private:
    void search_directory(const std::string& path, int depth) {
        if (depth > config_.max_depth) {
            return;
        }

        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (config_.skip_hidden && entry.is_hidden()) {
                continue;
            }

            std::string entry_path = entry.path().string();
            if (std::find(config_.skip_paths.begin(), config_.skip_paths.end(), entry_path) != config_.skip_paths.end()) {
                continue;
            }

            if (entry.is_directory()) {
                std::unique_lock<std::mutex> lock(mutex_);
                while (active_threads_ >= config_.max_concurrency) {
                    cv_.wait(lock);
                }
                ++active_threads_;
                lock.unlock();

                std::thread t(&FileSearch::search_directory, this, entry_path, depth + 1);
                t.detach();
            } else if (entry.is_regular_file() && entry.path().extension() == config_.file_type) {
                std::cout << entry.path().string() << std::endl;
            }
        }

        std::unique_lock<std::mutex> lock(mutex_);
        --active_threads_;
        cv_.notify_all();
    }

    SearchConfig config_;
    std::mutex mutex_;
    std::atomic<int> active_threads_;
    std::condition_variable cv_;
};

int main() {
    SearchConfig config{
        "/home/yuanfang/Desktop", // root_path
        ".c", // file_type
        4, // max_concurrency
        10, // max_depth
        true, // skip_hidden
        {""} // skip_paths
    };

    FileSearch file_search(config);
    file_search.start();

    return 0;
}



