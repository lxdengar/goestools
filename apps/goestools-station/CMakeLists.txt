find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(goestools-station
  resources/goestools-station.qrc
  src/managed_process.cpp
  src/managed_process.h
  src/product_catalog_model.cpp
  src/product_catalog_model.h
  src/product_scanner.cpp
  src/product_scanner.h
  src/main.cpp)

set_target_properties(goestools-station PROPERTIES
  AUTOMOC ON
  AUTORCC ON
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF)

target_link_libraries(goestools-station
  Qt6::Widgets)

install(
  TARGETS goestools-station
  RUNTIME DESTINATION bin
  COMPONENT goestools)
