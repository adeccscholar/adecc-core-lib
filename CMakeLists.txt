cmake_minimum_required(VERSION 3.25)
project(adecc-core-lib VERSION 0.1.0 LANGUAGES CXX)

# Header-only library. Consumers inherit C++23 and the repository-root include path.
add_library(adecc_core INTERFACE)
add_library(adecc::core ALIAS adecc_core)
set_target_properties(adecc_core PROPERTIES EXPORT_NAME core)
target_compile_features(adecc_core INTERFACE cxx_std_23)
target_include_directories(adecc_core INTERFACE
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
   $<INSTALL_INTERFACE:include>)

# Optional convenience components; external library dependencies remain the
# consumer's responsibility and are not imposed on the core headers.
foreach(_component IN ITEMS domain mssql postgre openxlsx)
   add_library(adecc_${_component} INTERFACE)
   add_library(adecc::${_component} ALIAS adecc_${_component})
   set_target_properties(adecc_${_component} PROPERTIES EXPORT_NAME ${_component})
   target_link_libraries(adecc_${_component} INTERFACE adecc::core)
endforeach()

include(GNUInstallDirs)
install(DIRECTORY adecc/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/adecc"
   FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
install(TARGETS adecc_core adecc_domain adecc_mssql adecc_postgre adecc_openxlsx
   EXPORT adecc-core-libTargets)
install(EXPORT adecc-core-libTargets NAMESPACE adecc::
   DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/adecc-core-lib")
