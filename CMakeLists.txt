cmake_minimum_required (VERSION 3.10)
project (ShadingTemplate)

# list of source files to compile
set(sources main.cpp GLSLProgram.cpp)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
message(PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

# find/include libraries
find_package(OpenGL REQUIRED)
# automatically finding GLEW FREEGLUT GLM ..
find_package(GLEW REQUIRED)
# find_package(GLUT REQUIRED)
#find_package(GLM  REQUIRED)

# use static lib for GLEW and FREEGLUT
set(GLEW_STATIC TRUE)
if(GLEW_STATIC)
   add_definitions(-DGLEW_STATIC)
endif(GLEW_STATIC)
set(GLUT_STATIC TRUE)
if(GLUT_STATIC)
   add_definitions(-DFREEGLUT_STATIC)
endif(GLUT_STATIC)

# include and link directories
include_directories(${PROJECT_SOURCE_DIR}/libs/glew-glew-2.3.1/include)
include_directories(${PROJECT_SOURCE_DIR}/libs/freeglut-3.8.0/include)
include_directories(${PROJECT_SOURCE_DIR}/libs/glm)
# please change the 2 lines below for your platform!
# here: vs2015_x64/Release
link_directories (${PROJECT_SOURCE_DIR}/libs/glew-glew-2.3.1/lib)
link_directories (${PROJECT_SOURCE_DIR}/libs/freeglut-3.8.0/lib)

# executable ShadingTemplate
add_executable (ShadingTemplate ${sources})
# target_link_libraries(ShadingTemplate ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${GLEW_LIBRARIES} ${GLM_LIBRARIES} libglew32.lib freeglut_static.lib)
target_link_libraries(ShadingTemplate ${OPENGL_LIBRARIES} libglut.a libglew.a)
# copy the shader directory relative to the executable
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shader
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
