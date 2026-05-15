# Sophon-Stream Web Management System - Makefile
# C++17 with cpp-httplib, sqlite3, OpenSSL, spdlog, jwt-cpp, nlohmann/json
# NO Drogon dependency required

CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -I src -I third_party/include \
           -DJWT_DISABLE_PICOJSON \
           $(shell pkg-config --cflags sqlite3 openssl)
LDFLAGS  = $(shell pkg-config --libs sqlite3 openssl) -lz -lpthread -ldl

TARGET   = sophon-stream-web

# Source files (all Drogon-free, using cpp-httplib)
SRCS = \
        src/main.cc \
        src/Router.cc \
        src/common/Version.cc \
        src/database/DbManager.cc \
        src/utils/JwtUtils.cc \
        src/utils/CryptoUtils.cc \
        src/utils/ConfigUtils.cc \
        src/services/AuthService.cc \
        src/services/DeviceService.cc \
        src/services/TaskService.cc \
        src/services/AlgorithmService.cc \
        src/services/AlertService.cc \
        src/services/MonitorService.cc \
        src/services/PluginService.cc \
        src/services/SystemService.cc \
        src/controllers/AuthController.cc \
        src/controllers/DeviceController.cc \
        src/controllers/TaskController.cc \
        src/controllers/AlgorithmController.cc \
        src/controllers/AlertController.cc \
        src/controllers/MonitorController.cc \
        src/controllers/PluginController.cc \
        src/controllers/SystemController.cc \
        src/sophon_bridge/EngineBridge.cc

OBJS = $(SRCS:.cc=.o)

.PHONY: all clean run debug

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

debug: CXXFLAGS = -std=c++17 -g -O0 -Wall -Wextra -I src -I third_party/include -DJWT_DISABLE_PICOJSON $(shell pkg-config --cflags sqlite3 openssl)
debug: clean $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET) -c ./config/config.json
