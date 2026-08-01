#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>

// Уровень важности сообщения, enum class для изоляции области видимости и типобезопасности 
enum class LogLevel {
    ERROR, // 0
    WARNING, // 1
    INFO     // 2
};

class Logger {
private:
    std::string filename_;       // Имя файла куда записываються сообщения
    LogLevel default_level_;     // Уровень важности сообщения по умолчаниию
    std::ofstream file_;         // Поток для записи в файл
    mutable std::mutex mutex_;   // mutable позволяет использовать мьютекс в const методах

    // Вспомогательный приватный метод для преобразования enum в строку
    std::string levelToString(LogLevel level) const;
    
    // Вспомогательный метод для получения текущей временной метки
    std::string getCurrentTimestamp() const;

public:
    // 2. Конструктор принимает строки по константной ссылке и сразу открывает файл
    Logger(const std::string& filename, LogLevel default_level = LogLevel::WARNING);

    // 3. Деструктор. При использовании std::string и std::ofstream 
    // ручное освобождение памяти не требуется
    ~Logger();

    // Запрещаем копирование логгера, чтобы избежать конкуренции за один файл
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // 4. Метод записи принимает сообщение и его уровень
    void log(const std::string& message, LogLevel level);

    // 5. Изменение уровня по умолчанию
    void setDefaultLevel(LogLevel new_level);
};