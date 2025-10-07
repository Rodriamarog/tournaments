# Define el nombre del proyecto actual de CMake
project(tournament_common)

# Establece la versión del estándar de C++ que se usará para compilar
# En este caso, C++23 (permite nuevas características del lenguaje)
set(CMAKE_CXX_STANDARD 23)

# Define los archivos fuente que se van a compilar dentro de esta librería
# Aquí solo se incluye TournamentRepository.cpp, pero podrían agregarse más.
set(COMMON_SOURCES
        src/persistence/repository/TournamentRepository.cpp
)

# Indica a CMake que agregue la carpeta "include" al listado de directorios de cabeceras
# Esto permite que los archivos .hpp puedan ser incluidos sin rutas absolutas.
include_directories(include)

# Crea una librería estática (STATIC) con el nombre del proyecto ("tournament_common")
# A partir de los archivos fuente definidos en COMMON_SOURCES.
# El resultado será algo como "libtournament_common.a"
add_library(${PROJECT_NAME} STATIC ${COMMON_SOURCES})

# Define los directorios que otros proyectos pueden usar para incluir los headers de esta librería
# INTERFACE significa que los proyectos que dependan de esta librería verán esos headers.
target_include_directories(${PROJECT_NAME}
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Especifica las dependencias externas que necesita esta librería para compilarse correctamente
# En este caso:
# - nlohmann_json: librería de manejo de JSON en C++
# - libpqxx: librería para conectarse y trabajar con bases de datos PostgreSQL desde C++
target_link_libraries(${PROJECT_NAME} PRIVATE
        nlohmann_json::nlohmann_json
        libpqxx::pqxx
)
