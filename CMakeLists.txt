cmake_minimum_required (VERSION 3.15)

project (LiteHttpd.FileServer VERSION 1.1.0 LANGUAGES CXX C)

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

if (DEFINED OUTPUT_DIR)
	set (CONFIG_OUTPUT_DIR ${OUTPUT_DIR})
else ()
	set (CONFIG_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
endif ()

# Dev Kit
if (NOT TARGET LiteHttpdDev::core)
	add_subdirectory (LiteHttpd.Dev)
endif ()

# Generate Config
if (WIN32)
	set (FILESERVER_WWWROOT "C:/wwwroot")
elseif (UNIX)
	set (FILESERVER_WWWROOT "/home/wwwroot")
endif ()
configure_file (
	"${CMAKE_CURRENT_SOURCE_DIR}/configTemplate/LiteHttpd.FileServer.json.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/config/LiteHttpd.FileServer.json"
	@ONLY
)
set_source_files_properties (
	"${CMAKE_CURRENT_SOURCE_DIR}/config/LiteHttpd.FileServer.json"
	PROPERTIES GENERATED TRUE
)

# Copy Config
file (GLOB_RECURSE MODULE_CONFIG_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/config/*")
set (MODULE_CONFIG_OUTPUT "")
foreach (ITEM ${MODULE_CONFIG_SRC})
    string (REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/config" "${CONFIG_OUTPUT_DIR}" ITEM_TARGET ${ITEM})
	list (APPEND MODULE_CONFIG_OUTPUT ${ITEM_TARGET})

	cmake_path (CONVERT ${ITEM} TO_NATIVE_PATH_LIST ITEM_INPUT)
	cmake_path (CONVERT ${ITEM_TARGET} TO_NATIVE_PATH_LIST ITEM_OUTPUT)

	if (WIN32)
		add_custom_command (
			OUTPUT ${ITEM_TARGET}
			COMMAND copy ${ITEM_INPUT} ${ITEM_OUTPUT}
			DEPENDS ${ITEM}
			COMMENT "Copying module config file: ${ITEM}"
			VERBATIM)
	else ()
		add_custom_command (
			OUTPUT ${ITEM_TARGET}
			COMMAND cp ${ITEM_INPUT} ${ITEM_OUTPUT}
			DEPENDS ${ITEM}
			COMMENT "Copying module config file: ${ITEM}"
			VERBATIM)
	endif ()
endforeach (ITEM ${MODULE_CONFIG_SRC})
add_custom_target (fileserver_module_config_copy
	DEPENDS ${MODULE_CONFIG_OUTPUT}
	COMMENT "Copy fileserver module configs"
	VERBATIM)

# CJsonObject
set (CJSON_SRC "./CJsonObject/cJson.c" "./CJsonObject/CJsonObject.cpp")
set (CJSON_INC "./CJsonObject")

# Source
file (GLOB_RECURSE FILESERVER_SRC CONFIGURE_DEPENDS "./source/*.cpp" "./source/*.c" "./source/*.rc")

# Target
add_library (LiteHttpd.FileServer SHARED ${FILESERVER_SRC} ${CJSON_SRC})
target_include_directories (LiteHttpd.FileServer PRIVATE ${CJSON_INC})
target_compile_definitions (LiteHttpd.FileServer PRIVATE
	"PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}"
	"PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}"
	"PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}"
)
target_link_libraries (LiteHttpd.FileServer PRIVATE LiteHttpdDev::core)
add_dependencies (LiteHttpd.FileServer fileserver_module_config_copy)

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

set_target_properties (LiteHttpd.FileServer PROPERTIES PREFIX "" IMPORT_PREFIX "")

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
