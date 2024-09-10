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
OBJDIR     			:= $(BINDIR)/Obj
LOGDIR				:= $(BUILDDIR)/Log
SHADERDIR			:= $(SRCDIR)/Shader

SRCS   				:= $(wildcard $(SRCDIR)/*.cpp)
SRCS_VERTSHADER  	:= $(wildcard $(SHADERDIR)/*.vert)
SRCS_FRAGSHADER  	:= $(wildcard $(SHADERDIR)/*.frag)
OBJS   				:= $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
DEPS 				:= $(OBJS:.o=.d)

BIN 				= app
BINFMT				= _exe
TARGET 				:= $(addsuffix $(BINFMT),$(BIN))
# Create shader binary file names from shader source files as such. Let's say the shader source name in XYZ.vert, then 
# the binary name will be XYZVert.spv
BINFMT_SHADER      	= .spv
SFX_VERTSHADER		= vert
SFX_FRAGSHADER  	= frag
BINSFX_VERTSHADER	:= $(addsuffix $(BINFMT_SHADER),Vert)
BINSFX_FRAGSHADER	:= $(addsuffix $(BINFMT_SHADER),Frag)
TARGETS_VERTSHADER  := $(foreach file,$(notdir $(SRCS_VERTSHADER)), \
					   $(patsubst %.$(SFX_VERTSHADER),%$(BINSFX_VERTSHADER),$(file)))
TARGETS_FRAGSHADER  := $(foreach file,$(notdir $(SRCS_FRAGSHADER)), \
					   $(patsubst %.$(SFX_FRAGSHADER),%$(BINSFX_FRAGSHADER),$(file)))

CXX        			= clang++
CXXFLAGS   			= -std=c++20 -Wall -Wextra -O3
LD         			= clang++ -o
LDFLAGS    			= -Wall -pedantic `pkg-config --static --libs glfw3` -lvulkan -Wl,-rpath,$(VULKAN_SDK)/lib
RM         			= rm -f
RMDIR				= rm -r -f

INCLUDES			:= -I$(VULKAN_SDK)/include				\
				   	   -I$(GLM_INCDIR)/include				\
				   	   -I$(GLFW_INCDIR)/include				\
					   -I$(STB_INCDIR)						\
					   -I$(OBJLOADER_INCDIR)				\
					   -I$(JSONPARSER_INCDIR)/single_include
# Setup glslc compiler path
# This compiler (by Google) is designed to verify that your shader code is fully standards compliant (across GPU vendors) 
# and produces one  SPIR-V binary that you can ship with your program
GLSLC				:= $(VULKAN_SDK)/bin/glslc

# Targets
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -MMD -o $@
	@echo "[OK] app compile"

$(TARGET): $(OBJS)
	@$(LD) $(BINDIR)/$@ $(LDFLAGS) $<
	@echo "[OK] app link"

-include $(DEPS)

%$(BINSFX_VERTSHADER): $(SHADERDIR)/%.$(SFX_VERTSHADER)
	@$(GLSLC) $< -o $(BINDIR)/$@

%$(BINSFX_FRAGSHADER): $(SHADERDIR)/%.$(SFX_FRAGSHADER)
	@$(GLSLC) $< -o $(BINDIR)/$@

.PHONY: all directories shaders app clean run info

all: directories shaders app 

directories:
	@mkdir -p $(LOGDIR)/Core
	@mkdir -p $(LOGDIR)/SandBox
	@echo "[OK] directories"

shaders: $(TARGETS_VERTSHADER) $(TARGETS_FRAGSHADER)
	@echo "[OK] shader compile"

app: $(TARGET)

clean:
	@$(RM) $(OBJDIR)/* 
	@echo "[OK] objects clean"
	@$(RM) $(BINDIR)/*$(BINFMT)
	@echo "[OK] binary clean"
	@$(RM) $(BINDIR)/*$(BINFMT_SHADER)
	@echo "[OK] shader clean"
	@$(RMDIR) $(LOGDIR)/*
	@echo "[OK] log clean"

run:
	$(BINDIR)/$(addsuffix $(BINFMT),$(BIN))

info:
	@echo "[*] Source dir:		${SRCDIR}       	"
	@echo "[*] Build dir:		${BUILDDIR}     	"
	@echo "[*] Binary dir:		${BINDIR}       	"
	@echo "[*] Object dir:		${OBJDIR}       	"
	@echo "[*] Log save dir:	${LOGDIR}       	"
	@echo "[*] Shader dir:		${SHADERDIR}    	"
	@echo "[*] Source files:	${SRCS}      		"
	@echo "[*] Vert shaders:	$(SRCS_VERTSHADER) 	"
	@echo "[*] Frag shaders: 	$(SRCS_FRAGSHADER) 	"
	@echo "[*] Object files:	${OBJS}      		"
	@echo "[*] Dependencies:	${DEPS} 			"