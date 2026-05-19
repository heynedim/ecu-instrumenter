.PHONY: all run check clean deploy

CXX ?= g++
TARGET := bin/ecu-instrumenter
SRC := src/main.cpp src/app_state.cpp src/telemetry.cpp src/render.cpp src/input.cpp
BUILD_DIR := build
OBJ := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRC))
DEP := $(OBJ:.o=.d)

MIYOO_IP ?= 192.168.1.53

CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra -pedantic -MMD -MP
CXXFLAGS += $(shell sdl2-config --cflags)
LDFLAGS += $(shell sdl2-config --libs) -pthread

all: $(TARGET)

check: all

$(TARGET): $(OBJ)
	@mkdir -p bin
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)

$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) bin data

deploy: check
	@echo "Syncing ECU-INSTRUMENTER to Miyoo Mini using rsync..."
	@mkdir -p ./ECUInstrumenter/bin ./ECUInstrumenter/data ./ECUInstrumenter/assets
	@cp launch.sh config.json ./ECUInstrumenter/
	@cp $(TARGET) ./ECUInstrumenter/bin/
	@cp assets/icon.png ./ECUInstrumenter/assets/
	@rsync -rtvzc --progress ./ECUInstrumenter/ root@$(MIYOO_IP):/mnt/SDCARD/App/ECUInstrumenter/
	@rm -rf ./ECUInstrumenter
	@echo "Deployment complete."

-include $(DEP)
