cmake_minimum_required(VERSION 3.16)

# list the header files
set(HEADER_FILES
	include/renoir-gl450/Context.h
	include/renoir-gl450/Handle.h
	include/renoir-gl450/Renoir-gl450.h
)

# list the source files
set(SOURCE_FILES
	src/renoir-gl450/Renoir-gl450.cpp
)

# list os specfic files
if(WIN32)
	set(SOURCE_FILES ${SOURCE_FILES}
		src/renoir-gl450/winos/Context.cpp
	)
elseif(UNIX AND NOT APPLE)
	set(SOURCE_FILES ${SOURCE_FILES}
		src/renoir-gl450/linux/Context.cpp
	)
elseif(APPLE)
	set(SOURCE_FILES ${SOURCE_FILES}
	)
endif()

# add library target
add_library(renoir-gl450)

target_sources(renoir-gl450
	PRIVATE ${HEADER_FILES}
			${SOURCE_FILES}
			${PROJECT_SOURCE_DIR}/include/renoir/Renoir.h
)

set_target_properties(renoir-gl450 PROPERTIES PREFIX "")

if (RENOIR_UNITY_BUILD)
	set_target_properties(renoir-gl450
		PROPERTIES UNITY_BUILD_BATCH_SIZE 0
				   UNITY_BUILD true)
endif()

add_library(MoustaphaSaad::renoir-gl450 ALIAS renoir-gl450)

target_link_libraries(renoir-gl450
	PRIVATE
		renoir-glew
		mn
		"$<$<PLATFORM_ID:Windows>:opengl32>"
		"$<$<PLATFORM_ID:Linux>:X11;GL;GLU>"
)

# make it reflect the same structure as the one on disk
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADER_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})

# enable C++17
# disable any compiler specifc extensions
target_compile_features(renoir-gl450 PUBLIC cxx_std_17)
set_target_properties(renoir-gl450 PROPERTIES
	CXX_EXTENSIONS OFF
)

# generate exports header file
include(GenerateExportHeader)
generate_export_header(renoir-gl450
	EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/renoir-gl450/Exports.h
)

# list include directories
target_include_directories(renoir-gl450
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/include
)

if (${RENOIR_DEBUG_LAYER})
	message(STATUS "feature: gl450 debug layer enabled")
	target_compile_definitions(renoir-gl450 PRIVATE RENOIR_DEBUG_LAYER=1)
else()
	target_compile_definitions(renoir-gl450 PRIVATE RENOIR_DEBUG_LAYER=0)
endif()

if (${RENOIR_LEAK})
	message(STATUS "feature: gl450 leak detector enabled")
	target_compile_definitions(renoir-gl450 PRIVATE RENOIR_LEAK=1)
else()
	target_compile_definitions(renoir-gl450 PRIVATE RENOIR_LEAK=0)
endif()

if(WIN32)
	target_compile_definitions(renoir-gl450 PRIVATE OS_WINDOWS=1)
elseif(UNIX)
	target_compile_definitions(renoir-gl450 PRIVATE OS_LINUX=1)
endif()