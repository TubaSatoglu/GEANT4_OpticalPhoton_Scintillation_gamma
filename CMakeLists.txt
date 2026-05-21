cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(OpticalSim)

# Geant4 kütüphanelerini bul
find_package(Geant4 REQUIRED ui_all vis_all)

# Geant4 dosyalarını include et
include(${Geant4_USE_FILE})
include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src)

# Tüm kaynak dosyalarını al
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
set(sources ${sources} ${PROJECT_SOURCE_DIR}/sim.cc)

# Simülasyon programını derle
add_executable(sim ${sources})
target_link_libraries(sim ${Geant4_LIBRARIES} ${Geant4_UIVIS_LIBRARIES})

# Makro dosyalarını çalışma dizinine kopyala
file(GLOB MACRO_FILES "${PROJECT_SOURCE_DIR}/macros/*.mac")
file(COPY ${MACRO_FILES} DESTINATION ${PROJECT_BINARY_DIR}/macros)

# Varsayılan hedefi belirle
add_custom_target(OpticalSim DEPENDS sim)
