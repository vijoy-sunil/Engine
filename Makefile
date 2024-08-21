# VULKAN_SDK is setup in .zshrc
GLM_INCDIR			= /opt/homebrew/Cellar/glm/1.0.1
GLFW_INCDIR			= /opt/homebrew/Cellar/glfw/3.4
STB_INCDIR			= /opt/homebrew/Cellar/stb-master
OBJLOADER_INCDIR	= /opt/homebrew/Cellar/tinyobjloader-release
JSONPARSER_INCDIR   = /opt/homebrew/Cellar/json-develop

# Setup directories and file paths
SRCDIR     			= ./SandBox
BUILDDIR			= ./Build
BINDIR     			:= $(BUILDDIR)/Bin
OBJDIR     			:= $(BINDIR)/Objs
LOGDIR				:= $(BUILDDIR)/Log
SHADERDIR			:= $(SRCDIR)/Shaders
VERTSHADER			:= $(SHADERDIR)/shader.vert
FRAGSHADER			:= $(SHADERDIR)/shader.frag

SOURCES   			:= $(wildcard $(SRCDIR)/*.cpp)
OBJECTS   			:= $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
DEPENDENCIES 		:= $(OBJECTS:.o=.d)

BIN 				= app
VERTSHADER_BIN  	= vert
FRAGSHADER_BIN  	= frag 
BINFMT				= _exe
SHADERBINFMT		= .spv

CXX        			= clang++
CXXFLAGS   			= -std=c++20 -Wall -Wextra -O3
LD         			= clang++ -o
LDFLAGS    			= -Wall -pedantic `pkg-config --static --libs glfw3` -lvulkan -Wl,-rpath,$(VULKAN_SDK)/lib
RM         			= rm -f
RMDIR				= rm -r -f
TARGET 				:= $(addsuffix $(BINFMT),$(BIN))
INCLUDES			:= -I$(VULKAN_SDK)/include				\
				   	   -I$(GLM_INCDIR)/include				\
				   	   -I$(GLFW_INCDIR)/include				\
					   -I$(STB_INCDIR)						\
					   -I$(OBJLOADER_INCDIR)				\
					   -I$(JSONPARSER_INCDIR)/single_include
# setup glslc compiler path
# This compiler (by Google) is designed to verify that your shader code is fully standards compliant (across GPU vendors) 
# and produces one  SPIR-V binary that you can ship with your program
GLSLC				:= $(VULKAN_SDK)/bin/glslc

# targets
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -MMD -o $@
	@echo "[OK] compile"

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(LD) $@ $(LDFLAGS) $(OBJECTS)
	@echo "[OK] link"

-include $(DEPENDENCIES)

.PHONY: clean run shaders info
clean:
	@$(RM) $(OBJDIR)/* 
	@echo "[OK] objects clean"
	@$(RM) $(BINDIR)/*$(BINFMT)
	@echo "[OK] binary clean"
	@$(RM) $(BINDIR)/*$(SHADERBINFMT)
	@echo "[OK] shader clean"
	@$(RMDIR) $(LOGDIR)/*
	@echo "[OK] log clean"

run:
	$(BINDIR)/$(addsuffix $(BINFMT),$(BIN))

shaders:
	$(GLSLC) $(VERTSHADER) -o $(BINDIR)/$(addsuffix $(SHADERBINFMT),$(VERTSHADER_BIN))
	$(GLSLC) $(FRAGSHADER) -o $(BINDIR)/$(addsuffix $(SHADERBINFMT),$(FRAGSHADER_BIN))
	@echo "[OK] shader compile"

info:
	@echo "[*] Source dir:		${SRCDIR}       "
	@echo "[*] Build dir:		${BUILDDIR}     "
	@echo "[*] Binary dir:		${BINDIR}       "
	@echo "[*] Object dir:		${OBJDIR}       "
	@echo "[*] Log save dir:	${LOGDIR}       "
	@echo "[*] Shader dir:		${SHADERDIR}    "
	@echo "[*] Source files:	${SOURCES}      "
	@echo "[*] Object files:	${OBJECTS}      "
	@echo "[*] Dependencies:	${DEPENDENCIES} "