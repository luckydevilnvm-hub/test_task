#include "logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>

// Конструктор
Logger::Logger(const std::string& filename, LogLevel default_level)
    : filename_(filename), default_level_(default_level) 
{
    // Открываем файл в режиме дозаписи (app), чтобы не стирать старые логи
    file_.open(filename_, std::ios::app);
    
    // проверка: удалось ли открыть файл
    if (!file_.is_open()) {
        throw std::runtime_error("Ошибка: не удалось открыть файл журнала: " + filename_);
    }
}

// Деструктор
Logger::~Logger() {
    if (file_.is_open()) {
        file_.close();
    }
}

// Метод записи в журнал
void Logger::log(const std::string& message, LogLevel level) {
    // Фильтрация: если уровень сообщения ниже уровня по умолчанию, игнорируем
    // (Чем меньше числовое значение enum, тем выше важность)
    if (level > default_level_) {
        return;
    }

    // Блокировка мьютекса для потокобезопасности
    std::lock_guard<std::mutex> lock(mutex_);

    // Формирование строки лога
    file_ << "[" << getCurrentTimestamp() << "] "
          << "[" << levelToString(level) << "] "
          << message << "\n";
          
    // Сброс буфера в файл, чтобы данные не потерялись при крахе
    file_.flush(); 
}

// Изменение уровня важности по умолчанию
void Logger::setDefaultLevel(LogLevel new_level) {
    std::lock_guard<std::mutex> lock(mutex_);
    default_level_ = new_level;
}

// Вспомогательный метод: преобразование enum в строку
std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::INFO:    return "INFO";
        default:                return "UNKNOWN";
    }
}

// Вспомогательный метод: получение текущего времени
std::string Logger::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    // Форматирование времени: ГГГГ-ММ-ДД ЧЧ:ММ:СС
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}