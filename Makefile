# |-------------------------------------------------------------------------|
# |	Directories																|
# |-------------------------------------------------------------------------|
# 'VULKAN_SDK' is setup in .zshrc
GLM_DIR				:= /opt/homebrew/Cellar/glm/1.0.1
GLFW_DIR			:= /opt/homebrew/Cellar/glfw/3.4
DEPENDENCY_DIR		:= ./Dependency
IMGUI_DIR			:= $(DEPENDENCY_DIR)/imgui
IMPLOT_DIR			:= $(DEPENDENCY_DIR)/implot
IMGUI_BACKEND_DIR	:= $(IMGUI_DIR)/backends
APP_DIR				:= ./SandBox
SHADER_DIR			:= $(APP_DIR)/Shader
BUILD_DIR			:= ./Build
BIN_DIR     		:= $(BUILD_DIR)/Bin
OBJ_DIR     		:= $(BUILD_DIR)/Obj
LOG_DIR				:= $(BUILD_DIR)/Log
# |-------------------------------------------------------------------------|
# | Sources																	|
# |-------------------------------------------------------------------------|
APP_SRCS			:= $(APP_DIR)/main.cpp									\
					   $(IMGUI_DIR)/imgui.cpp 								\
					   $(IMGUI_DIR)/imgui_demo.cpp							\
					   $(IMGUI_DIR)/imgui_draw.cpp 							\
					   $(IMGUI_DIR)/imgui_tables.cpp						\
					   $(IMGUI_DIR)/imgui_widgets.cpp						\
					   $(IMPLOT_DIR)/implot.cpp								\
					   $(IMPLOT_DIR)/implot_items.cpp						\
					   $(IMPLOT_DIR)/implot_demo.cpp						\
					   $(IMGUI_BACKEND_DIR)/imgui_impl_glfw.cpp				\
					   $(IMGUI_BACKEND_DIR)/imgui_impl_vulkan.cpp
VERT_SHADER_SRCS	:= $(wildcard $(SHADER_DIR)/*.vert)
FRAG_SHADER_SRCS	:= $(wildcard $(SHADER_DIR)/*.frag)
# |-------------------------------------------------------------------------|
# | Objects																	|
# |-------------------------------------------------------------------------|
OBJS				:= $(addprefix $(OBJ_DIR)/,								\
					   $(addsuffix .o, $(basename $(notdir $(APP_SRCS)))))
DEPS 				:= $(OBJS:.o=.d)
# |-------------------------------------------------------------------------|
# | Naming																	|
# |-------------------------------------------------------------------------|
APP_TARGET			:= app_exe
VERT_SHADER_TARGET	:= $(foreach file,$(notdir $(VERT_SHADER_SRCS)),		\
					   $(patsubst %.vert,%Vert.spv,$(file)))
FRAG_SHADER_TARGET	:= $(foreach file,$(notdir $(FRAG_SHADER_SRCS)), 		\
					   $(patsubst %.frag,%Frag.spv,$(file)))
# |-------------------------------------------------------------------------|
# | Flags																	|
# |-------------------------------------------------------------------------|
CXX        			:= clang++
CXXFLAGS   			:= -std=c++20 -Wall -Wextra -O3
LD         			:= clang++ -o
LDFLAGS    			:= -Wall -pedantic `pkg-config --static --libs glfw3` 	\
					   -lvulkan -Wl,-rpath,$(VULKAN_SDK)/lib
RM         			:= rm -f
RMDIR				:= rm -r -f

INCLUDES			:= -I$(VULKAN_SDK)/include								\
				   	   -I$(GLM_DIR)/include									\
				   	   -I$(GLFW_DIR)/include								\
					   -I$(DEPENDENCY_DIR)									\
					   -I$(IMGUI_DIR)										\
					   -I$(IMPLOT_DIR)										\
					   -I$(IMGUI_BACKEND_DIR)
# Setup glslc compiler path
GLSLC				:= $(VULKAN_SDK)/bin/glslc
# |-------------------------------------------------------------------------|
# | Rules																	|
# |-------------------------------------------------------------------------|
$(OBJ_DIR)/%.o: $(APP_DIR)/%.cpp
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -MMD -o $@
	@echo "[OK] compile" $<

$(OBJ_DIR)/%.o: $(IMGUI_DIR)/%.cpp
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -MMD -o $@
	@echo "[OK] compile" $<

$(OBJ_DIR)/%.o: $(IMPLOT_DIR)/%.cpp
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -MMD -o $@
	@echo "[OK] compile" $<

$(OBJ_DIR)/%.o: $(IMGUI_BACKEND_DIR)/%.cpp
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -MMD -o $@
	@echo "[OK] compile" $<

$(APP_TARGET): $(OBJS)
	@$(LD) $(BIN_DIR)/$@ $(LDFLAGS) $^
	@echo "[OK] link"

-include $(DEPS)

%Vert.spv: $(SHADER_DIR)/%.vert
	@$(GLSLC) $< -o $(BIN_DIR)/$@
	@echo "[OK] compile" $<

%Frag.spv: $(SHADER_DIR)/%.frag
	@$(GLSLC) $< -o $(BIN_DIR)/$@
	@echo "[OK] compile" $<
# |-------------------------------------------------------------------------|
# | Targets																	|
# |-------------------------------------------------------------------------|
.PHONY: all directories shaders app clean run

all: directories shaders app

directories:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(LOG_DIR)/Core
	@mkdir -p $(LOG_DIR)/Gui
	@mkdir -p $(LOG_DIR)/SandBox
	@echo "[OK] directories"

shaders: $(VERT_SHADER_TARGET) $(FRAG_SHADER_TARGET)

app: $(APP_TARGET)

clean:
	@$(RMDIR) $(BUILD_DIR)/*
	@echo "[OK] clean"

run:
	$(BIN_DIR)/$(APP_TARGET)