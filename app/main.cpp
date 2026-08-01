#include <iostream>
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <algorithm>

#include "../logger/logger.h"

// Структура для передачи данных между потоками
struct LogTask {
    std::string message;
    LogLevel level;
};

// Потокобезопасная очередь (Producer-Consumer)
class ThreadSafeQueue {
private:
    std::queue<LogTask> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool is_done_ = false;

public:
    void push(LogTask task) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(task));
        cv_.notify_one();
    }

    bool pop(LogTask& task) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty() || is_done_; });
        
        if (queue_.empty()) {
            return false;
        }
        
        task = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        is_done_ = true;
        cv_.notify_one();
    }
};

// Функция рабочего потока (Потребитель)
void workerFunction(ThreadSafeQueue& queue, Logger& logger) {
    LogTask task;
    while (queue.pop(task)) {
        logger.log(task.message, task.level);
    }
}

// Вспомогательная функция: очистка строки от скобок и пробелов по краям
std::string trimAndClean(const std::string& str) {
    std::string result = str;
    // Удаляем квадратные скобки
    result.erase(std::remove(result.begin(), result.end(), '['), result.end());
    result.erase(std::remove(result.begin(), result.end(), ']'), result.end());
    // Приводим к верхнему регистру
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

// Парсинг уровня из строки (возвращает true, если уровень распознан)
bool tryParseLevel(const std::string& raw_token, LogLevel& out_level) {
    std::string cleaned = trimAndClean(raw_token);
    
    if (cleaned == "ERROR") { out_level = LogLevel::ERROR; return true; }
    if (cleaned == "WARNING") { out_level = LogLevel::WARNING; return true; }
    if (cleaned == "INFO") { out_level = LogLevel::INFO; return true; }
    
    return false; // Уровень не распознан
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Использование: " << argv[0] << " <имя_файла> <уровень_по_умолчанию>\n";
        std::cerr << "Пример: " << argv[0] << " app.log INFO\n";
        return 1;
    }

    std::string filename = argv[1];
    
    // Парсим уровень по умолчанию из аргументов
    LogLevel default_level;
    if (!tryParseLevel(argv[2], default_level)) {
        std::cerr << "Неизвестный уровень по умолчанию: " << argv[2] << "\n";
        std::cerr << "Допустимые значения: ERROR, WARNING, INFO\n";
        return 1;
    }

    try {
        Logger logger(filename, default_level);
        ThreadSafeQueue task_queue;

        std::thread worker(workerFunction, std::ref(task_queue), std::ref(logger));

        std::cout << "Введите сообщения. Формат: [УРОВЕНЬ] Текст или просто Текст.\n";
        std::cout << "Допустимые уровни: ERROR, WARNING, INFO (регистр не важен).\n";
        std::cout << "Для выхода введите 'exit'.\n";

        std::string input_line;
        while (std::getline(std::cin, input_line)) {
            if (input_line.empty()) continue;
            
            if (input_line == "exit") {
                break;
            }

            std::istringstream iss(input_line);
            std::string first_token;
            iss >> first_token;

            LogLevel msg_level = default_level;
            std::string message;

            // Пытаемся распознать первый токен как уровень
            if (tryParseLevel(first_token, msg_level)) {
                // Уровень распознан — читаем остаток строки как сообщение
                std::getline(iss, message);
                // Удаляем ведущий пробел
                if (!message.empty() && message[0] == ' ') {
                    message = message.substr(1);
                }
            } else {
                // Первый токен не является уровнем — вся строка есть сообщение
                message = input_line;
                msg_level = default_level;
            }

            task_queue.push(LogTask{std::move(message), msg_level});
        }

        task_queue.shutdown();
        worker.join();
        
        std::cout << "Приложение корректно завершено. Данные записаны в " << filename << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Критическая ошибка: " << e.what() << "\n";
        return 1;
    }

    return 0;
}