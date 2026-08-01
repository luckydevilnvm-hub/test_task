CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -fPIC
LDFLAGS = -pthread

# Цель по умолчанию: собираем и библиотеку, и приложение
all: liblogger.so logger_app

# Сборка динамической библиотеки
liblogger.so: logger/logger.cpp logger/logger.h
	$(CXX) $(CXXFLAGS) -shared -o $@ logger/logger.cpp

# Сборка приложения с подключением динамической библиотеки
logger_app: app/main.cpp liblogger.so
	$(CXX) $(CXXFLAGS) -o $@ app/main.cpp -L. -llogger -Wl,-rpath,. $(LDFLAGS)

# Очистка каталога от временных файлов и исполняемых файлов
clean:
	rm -f liblogger.so logger_app

.PHONY: all clean