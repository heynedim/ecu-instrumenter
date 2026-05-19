.PHONY: all run check server run-server run-dev probe probe-obd clean deploy

CXX ?= g++
TARGET := bin/ecu-instrumenter
SERVER_TARGET := bin/obd-sim-server
PROBE_TARGET := bin/obd-probe
SRC := src/main.cpp src/app_state.cpp src/config.cpp src/obd_client.cpp src/telemetry.cpp src/render.cpp src/input.cpp
SERVER_SRC := tools/obd_sim_server.cpp
PROBE_SRC := tools/obd_probe.cpp
BUILD_DIR := build
OBJ := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRC))
SERVER_OBJ := $(patsubst tools/%.cpp,$(BUILD_DIR)/tools_%.o,$(SERVER_SRC))
PROBE_OBJ := $(patsubst tools/%.cpp,$(BUILD_DIR)/tools_%.o,$(PROBE_SRC))
DEP := $(OBJ:.o=.d) $(SERVER_OBJ:.o=.d) $(PROBE_OBJ:.o=.d)

MIYOO_IP ?= 192.168.1.53
OBD_HOST ?= 127.0.0.1
OBD_PORT ?= 35000
OBD_MODE ?= normal

CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra -pedantic -MMD -MP
CXXFLAGS += $(shell sdl2-config --cflags)
LDFLAGS += $(shell sdl2-config --libs) -pthread

all: $(TARGET)

check: all server probe

$(TARGET): $(OBJ)
	@mkdir -p bin
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)

$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

server: $(SERVER_TARGET)

$(SERVER_TARGET): $(SERVER_OBJ)
	@mkdir -p bin
	$(CXX) $(SERVER_OBJ) -o $(SERVER_TARGET)

probe: $(PROBE_TARGET)

$(PROBE_TARGET): $(PROBE_OBJ)
	@mkdir -p bin
	$(CXX) $(PROBE_OBJ) -o $(PROBE_TARGET)

$(BUILD_DIR)/tools_%.o: tools/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	./$(TARGET)

run-server: server
	./$(SERVER_TARGET) $(OBD_HOST) $(OBD_PORT) $(OBD_MODE)

probe-obd: probe
	./$(PROBE_TARGET) $(OBD_HOST) $(OBD_PORT)

run-dev: all server
	@./$(SERVER_TARGET) $(OBD_HOST) $(OBD_PORT) $(OBD_MODE) & server_pid=$$!; \
	trap 'kill $$server_pid 2>/dev/null || true' EXIT INT TERM; \
	sleep 1; \
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) bin data

deploy: check
	@echo "Syncing ECU-INSTRUMENTER to Miyoo Mini using rsync..."
	@mkdir -p ./ECUInstrumenter/bin ./ECUInstrumenter/data ./ECUInstrumenter/assets
	@cp launch.sh config.json ecu_config.ini ./ECUInstrumenter/
	@cp $(TARGET) ./ECUInstrumenter/bin/
	@cp assets/icon.png ./ECUInstrumenter/assets/
	@rsync -rtvzc --progress ./ECUInstrumenter/ root@$(MIYOO_IP):/mnt/SDCARD/App/ECUInstrumenter/
	@rm -rf ./ECUInstrumenter
	@echo "Deployment complete."

-include $(DEP)
