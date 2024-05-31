# setup paths here
COLLECTIONS_DIR		:= ./Collections
# VULKAN_SDK is setup in .zshrc
GLM_INCDIR			:= /opt/homebrew/Cellar/glm/1.0.1
GLFW_INCDIR			:= /opt/homebrew/Cellar/glfw/3.4

SRCDIR     			:= ./App
INCDIR				:= -I$(COLLECTIONS_DIR)					\
					   -I$(VULKAN_SDK)/include				\
				   	   -I$(GLM_INCDIR)/include				\
				   	   -I$(GLFW_INCDIR)/include
# shader files path
SHADERDIR			:= ./Config/Shaders
VERTSHADER			:= $(SHADERDIR)/shader.vert
FRAGSHADER			:= $(SHADERDIR)/shader.frag

# do not edit below here
BIN 				= app
VERTSHADER_BIN  	= vert
FRAGSHADER_BIN  	= frag 
BINFMT				= _exe
SHADERBINFMT		= .spv
BINDIR     			= ./Build/Bin
LOGDIR				= ./Build/Log

SOURCES   			:= $(wildcard $(SRCDIR)/*.cpp)
OBJDIR     			:= $(BINDIR)/Objs
OBJECTS   			:= $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

CXX        			= clang++
CXXFLAGS   			= -std=c++20 -Wall -Wextra -O0
LD         			= clang++ -o
LDFLAGS    			= -Wall -pedantic `pkg-config --static --libs glfw3` -lvulkan -Wl,-rpath,$(VULKAN_SDK)/lib
RM         			= rm -f
RMDIR				= rm -r -f
TARGET 				= $(addsuffix $(BINFMT),$(BIN))
# setup glslc compiler path
# This compiler (by Google) is designed to verify that your shader code is fully standards compliant (across GPU vendors) 
# and produces one  SPIR-V binary that you can ship with your program
GLSLC				:= $(VULKAN_SDK)/bin/glslc

# targets
$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(LD) $@ $(LDFLAGS) $(OBJECTS)
	@echo "[OK] link"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@$(CXX) $(CXXFLAGS) $(INCDIR) -c $< -o $@
	@echo "[OK] compile"

.PHONY: clean
clean:
	@$(RM) $(OBJDIR)/* $(BINDIR)/*$(BINFMT) $(BINDIR)/*$(SHADERBINFMT)
	@$(RMDIR) $(LOGDIR)/*
	@echo "[OK] clean"

run:
	$(BINDIR)/$(addsuffix $(BINFMT),$(BIN))

.PHONY: shaders
shaders:
	$(GLSLC) $(VERTSHADER) -o $(BINDIR)/$(addsuffix $(SHADERBINFMT),$(VERTSHADER_BIN))
	$(GLSLC) $(FRAGSHADER) -o $(BINDIR)/$(addsuffix $(SHADERBINFMT),$(FRAGSHADER_BIN))
	@echo "[OK] shader compile"