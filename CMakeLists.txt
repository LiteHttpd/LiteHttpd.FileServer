cmake_minimum_required (VERSION 3.15)

project (LiteHttpd.FileServer VERSION 1.0.0 LANGUAGES CXX C)

if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Release"  CACHE STRING "Choose the type of build." FORCE)
endif (NOT CMAKE_BUILD_TYPE)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_C_STANDARD 11)
set (CMAKE_C_EXTENSIONS OFF)
set (CMAKE_C_STANDARD_REQUIRED ON)

set (CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

if (MSVC)
    add_compile_definitions ("_CRT_SECURE_NO_WARNINGS")
endif (MSVC)

# Dev Kit
if (NOT TARGET LiteHttpdDev::core)
	add_subdirectory (LiteHttpd.Dev)
endif ()

# Source
file (GLOB_RECURSE FILESERVER_SRC CONFIGURE_DEPENDS "./source/*.cpp" "./source/*.c" "./source/*.rc")

# Target
add_library (LiteHttpd.FileServer SHARED ${FILESERVER_SRC})
target_compile_definitions (LiteHttpd.FileServer PRIVATE
	"PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}"
	"PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}"
	"PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}"
)
target_link_libraries (LiteHttpd.FileServer PRIVATE LiteHttpdDev::core)

if (NOT MSVC)
	target_compile_options (LiteHttpd.FileServer PRIVATE
		-pthread -fPIC)
endif (NOT MSVC)
if (WIN32)
    target_link_libraries (LiteHttpd.FileServer PRIVATE Dbghelp)
endif (WIN32)
if (MSVC)
	target_link_options (LiteHttpd.FileServer PRIVATE 
		/SUBSYSTEM:CONSOLE
	)
endif (MSVC)

# Output Directory
if (DEFINED OUTPUT_DIR)
	set_target_properties (LiteHttpd.FileServer PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
		LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
		ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
	)
	if (MSVC)
		set_target_properties (LiteHttpd.FileServer PROPERTIES
			PDB_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
		)	
	endif ()
endif ()
