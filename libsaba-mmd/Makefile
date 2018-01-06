#==================
# compile flags
#==================

PARENT = ..
INCLUDE_PATH = .
INCLUDE_PATH_EXTERN = external/spdlog/include \
                      external/tinyobjloader/include \
                      external/tinyxfileloader/include \
                      extlibs/bullet/bullet3/src
LIB_PATH_PARENT = $(PARENT)/lib
BUILD_PATH = build
BIN_PATH = bin
BINARY = $(BIN_PATH)/libsaba-mmd.a
INSTALL_BINARY = $(LIB_PATH_PARENT)/libsaba-mmd.a

INCLUDE_PATHS = $(INCLUDE_PATH) $(INCLUDE_PATH_EXTERN)
INCLUDE_PATH_FLAGS = $(patsubst %, -I%, $(INCLUDE_PATHS))

LIB_PATHS = ""
LIB_PATHS_EXTERN = ./libsaba-mmd/extlibs/bullet/bullet3/src/BulletDynamics \
                   ./libsaba-mmd/extlibs/bullet/bullet3/src/BulletCollision \
                   ./libsaba-mmd/extlibs/bullet/bullet3/src/LinearMath
LIB_PATH_FLAGS = $(patsubst %, -L%, $(LIB_PATHS)) \
                 $(patsubst %, -L%, $(LIB_PATHS_EXTERN))

LIB_STEMS = ""
LIB_STEMS_EXTERN = glut GLEW GL png pthread BulletDynamics BulletCollision LinearMath
LIB_FLAGS = $(patsubst %, -l%, $(LIB_STEMS)) \
            $(patsubst %, -l%, $(LIB_STEMS_EXTERN))

CXX = g++
DEBUG = -g
CXXFLAGS = -Wall $(DEBUG) $(INCLUDE_PATH_FLAGS) -std=c++14
LDFLAGS = -Wall $(DEBUG) $(LIB_PATH_FLAGS) $(LIB_FLAGS)

#==================
# all
#==================

.DEFAULT_GOAL : all
all : $(BINARY)

#==================
# objects
#==================

$(BUILD_PATH)/Saba/Base/%.o : Saba/Base/%.cpp
	mkdir -p $(BUILD_PATH)/Saba/Base
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(BUILD_PATH)/Saba/Model/MMD/%.o : Saba/Model/MMD/%.cpp
	mkdir -p $(BUILD_PATH)/Saba/Model/MMD
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(BUILD_PATH)/Saba/Model/OBJ/%.o : Saba/Model/OBJ/%.cpp
	mkdir -p $(BUILD_PATH)/Saba/Model/OBJ
	$(CXX) -c -o $@ $< $(CXXFLAGS)

.PHONY : clean_objects
clean_objects :
	-rm $(OBJECTS)
	-rm -rf $(BUILD_PATH)

#==================
# binary
#==================

CPP_STEMS = Saba/Base/File \
            Saba/Base/Log \
            Saba/Base/Path \
            Saba/Base/Singleton \
            Saba/Base/Time \
            Saba/Base/UnicodeUtil \
            Saba/Model/MMD/MMDCamera \
            Saba/Model/MMD/MMDIkSolver \
            Saba/Model/MMD/MMDMaterial \
            Saba/Model/MMD/MMDModel \
            Saba/Model/MMD/MMDMorph \
            Saba/Model/MMD/MMDNode \
            Saba/Model/MMD/MMDPhysics \
            Saba/Model/MMD/PMDFile \
            Saba/Model/MMD/PMDModel \
            Saba/Model/MMD/PMXFile \
            Saba/Model/MMD/PMXModel \
            Saba/Model/MMD/SjisToUnicode \
            Saba/Model/MMD/VMDAnimation \
            Saba/Model/MMD/VMDCameraAnimation \
            Saba/Model/MMD/VMDFile \
            Saba/Model/MMD/VPDFile \
            Saba/Model/OBJ/OBJModel \
            Saba/Model/OBJ/tinyobjloader
OBJECTS = $(patsubst %, $(BUILD_PATH)/%.o, $(CPP_STEMS))

$(BINARY) : $(OBJECTS)
	mkdir -p $(BIN_PATH)
	$(AR) -cvq $@ $^
	mkdir -p $(LIB_PATH_PARENT)
	cp $@ $(INSTALL_BINARY)

.PHONY : clean_binary
clean_binary :
	-rm $(BINARY)
	-rm $(INSTALL_BINARY)

#==================
# clean
#==================

.PHONY : clean
clean : clean_binary clean_objects
	-rmdir $(BIN_PATH) $(BUILD_PATH)
