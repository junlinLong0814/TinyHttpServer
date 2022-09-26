aux_source_directory (. SRC_LIST)

include_directories (../include)

add_compile_options (-std=c++17)

include_directories("/usr/include/mysql")
link_directories("/usr/lib/x86_64-linux-gnu")
 
set(MYSQL_LIBS mysqlclient pthread z m rt atomic ssl crypto dl)

add_executable (tomdog ${SRC_LIST})

target_link_libraries(tomdog  ${MYSQL_LIBS})

target_link_libraries (tomdog -lpthread)

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
