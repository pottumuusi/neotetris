EXE_NAME_GAME_CLIENT := neotetris

SRC_DIR_GAME_CLIENT := src
SRC_DIR_SHADERS := shaders
HEADERS_DIR_GAME_CLIENT := include

SRC_GAME_CLIENT := $(shell find $(SRC_DIR_GAME_CLIENT)/ -name "*.cpp")
HEADERS_GAME_CLIENT := $(shell find $(HEADERS_DIR_GAME_CLIENT) -name "*.hpp")

BIN_SHADER := \
	$(SRC_DIR_SHADERS)/vert.spv \
	$(SRC_DIR_SHADERS)/frag.spv

CXX = g++
SHADER_COMPIER := glslc

COMPILER_FLAGS_GAME_CLIENT := -I$(HEADERS_DIR_GAME_CLIENT)
LINKER_FLAGS_GAME_CLIENT := -lSDL2 -lvulkan -ldl -lpthread -lX11
# LINKER_FLAGS_GAME_CLIENT := -lSDL2 -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

define verify_build_tools_present
	$(info Checking presence of build/compilation tools)

	# glslc shader compiler is included in Vulkan SDK
	which $(SHADER_COMPIER)
endef

shaders/%.spv : shaders/shader.%
	$(call verify_build_tools_present)
	$(SHADER_COMPIER) $< -o $@

$(EXE_NAME_GAME_CLIENT): $(BIN_SHADER) $(SRC_GAME_CLIENT) $(HEADERS_GAME_CLIENT)
	$(CXX) $(SRC_GAME_CLIENT) $(COMPILER_FLAGS_GAME_CLIENT) $(LINKER_FLAGS_GAME_CLIENT) -o $@
