# Establece la versión mínima requerida de CMake para este proyecto
cmake_minimum_required(VERSION 3.22)

# Declara el nombre del proyecto principal
project(Tournaments)

# Define que el proyecto utilizará el estándar C++23
set(CMAKE_CXX_STANDARD 23)

# Habilita la ejecución de pruebas unitarias con CTest
enable_testing()

# Busca e incluye la configuración del framework web Crow
find_package(Crow CONFIG REQUIRED)

# Busca la librería libpqxx para conectarse a bases de datos PostgreSQL
find_package(libpqxx CONFIG REQUIRED)

# Busca el directorio de encabezados de la librería Hypodermic (inyección de dependencias)
find_path(HYPODERMIC_INCLUDE_DIRS "Hypodermic/ActivatedRegistrationInfo.h")

# Busca e incluye la librería para manejar JSON en C++ (nlohmann_json)
find_package(nlohmann_json CONFIG REQUIRED)

# Agrega el subdirectorio del módulo común del torneo
add_subdirectory(tournament_common)

# Agrega el subdirectorio que contiene los servicios del torneo
add_subdirectory(tournament_services)

