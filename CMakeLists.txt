cmake_minimum_required(VERSION 3.19)



#installation avec vcpkg
#   \vcpkg install vtk[qt] --triplet x64-windows
#   \vcpkg install vtk:x64-windows
#   \vcpkg install [core,qt]:x64-windows
#   .\vcpkg install opencascade --triplet x64-windows



# Déclaration de la toolchain vcpkg (Doit rester tout en haut)
set(CMAKE_TOOLCHAIN_FILE "J:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

project(Cad_IA LANGUAGES CXX)

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC_OPTIONS "--format-version" "1" "--verbose")

# ==============================================================================
#️ ZONE DE SÉCURITÉ : ANNIHILATION DE LA BOUCLE ET CONFIGURATION DE GLEW
# ==============================================================================
# On désactive le wrapper défaillant de vcpkg pour GLEW
set(CMAKE_DISABLE_FIND_PACKAGE_GLEW TRUE CACHE BOOL "" FORCE)

# Chemins physiques
set(GLEW_INCLUDE_DIR "J:/vcpkg/installed/x64-windows/include" CACHE PATH "" FORCE)
set(GLEW_LIBRARY "J:/vcpkg/installed/x64-windows/lib/glew32.lib" CACHE FILEPATH "" FORCE)
set(GLEW_SHARED_LIBRARY_RELEASE "J:/vcpkg/installed/x64-windows/bin/glew32.dll" CACHE FILEPATH "" FORCE)

# On force le statut trouvé
set(GLEW_FOUND TRUE CACHE BOOL "" FORCE)
set(GLEW_VERSION "2.2.0" CACHE STRING "" FORCE)

#  AJOUT ICI : On crée la cible factice "GLEW::GLEW" attendue par VTK
if(NOT TARGET GLEW::GLEW)
    add_library(GLEW::GLEW UNKNOWN IMPORTED GLOBAL)
    set_target_properties(GLEW::GLEW PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${GLEW_INCLUDE_DIR}"
        IMPORTED_LOCATION "${GLEW_LIBRARY}"
    )
endif()
# ==============================================================================

# Chargement de Qt6
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Svg)

# On charge LinguistTools de manière optionnelle
#find_package(Qt6 COMPONENT LinguistTools)

# Chargement de VTK (qui n'ira plus boucler sur GLEW grâce aux variables ci-dessus)
find_package(VTK REQUIRED COMPONENTS
    CommonCore
    RenderingCore
    InteractionStyle
    RenderingOpenGL2
    GUISupportQt
    CommonColor
    RenderingAnnotation
    FiltersExtraction
    FiltersGeometry
    FiltersSources
)

# Chargement d'OpenCASCADE
find_package(OpenCASCADE REQUIRED)

qt_standard_project_setup()



if(MSVC)
    add_compile_options(/bigobj)
endif()

# 2. Convertir automatiquement cette liste en drapeaux -I (ex: -Iinclude -Isrc/gui ...)
# INC_FLAGS = $(addprefix -I, $(INCLUDE_DIRS))

# 3. Ajouter ces drapeaux à vos options de compilation C++
#CXXFLAGS += $(INC_FLAGS) -std=c++17 -Wall



# Déclaration de l'exécutable et de ses sources
qt_add_executable(Cad_IA
    WIN32 MACOSX_BUNDLE
    src/Qt/main.cpp
    src/Qt/mainwindow.cpp
    src/Qt/mainwindow_toolbars.cpp
    src/Qt/mainwindow_menus.cpp
    src/Qt/mainwindow_slots.cpp
    src/Qt/mainwindow_CadResponses.cpp
    src/Qt/mainwindow_CadTreeView.cpp
    src/Qt/mainwindow.h
    src/Qt/extrusiondialog.h
    src/Qt/extrusiondialog.cpp
    src/Qt/resources.qrc
    src/Qt/CadTreeModel.cpp
    src/Qt/CadTreeModel.h
    src/Qt/CadResponseCustomEvent.h
    src/vtk3d/vtk3d_MainView.cpp
    src/vtk3d/vtk3d_MainView.h
    src/vtk3d/vtk3d_MainView_diag.cpp
    src/vtk3d/vtk3d_HighLighter.h
    src/vtk3d/vtk3d_HighLighter.cpp
    src/vtk3d/vtk3d_OccToVtkConverter.cpp
    src/vtk3d/vtk3d_OccToVtkConverter.h
    src/vtk3d/Vtk3d_abstractviewmode.h
    src/vtk3d/vtk3d_actors.h
    src/vtk3d/vtk3d_actors.cpp
    src/vtk3d/vtk3d_DimensionDisplay.cpp
    src/vtk3d/part/vtk3d_part.h
    src/vtk3d/part/vtk3d_part.cpp
    src/vtk3d/sketch/vtk3d_sketch.h
    src/vtk3d/sketch/vtk3d_sketch.cpp
    src/vtk3d/sketch/vtk3d_sketch_Context.h
    src/vtk3d/sketch/vtk3d_sketch_view.cpp
    src/vtk3d/sketch/vtk3d_sketch_events.cpp
    src/vtk3d/sketch/vtk3d_sketch_Tools.h
    src/vtk3d/sketch/vtk3d_sketch_ToolLine.h
    src/vtk3d/sketch/vtk3d_sketch_ToolLine.cpp
    src/vtk3d/sketch/vtk3d_sketch_ToolSelect.h
    src/vtk3d/sketch/vtk3d_sketch_ToolSelect.cpp
    src/vtk3d/sketch/vtk3d_sketch_ToolCircle.h
    src/vtk3d/sketch/vtk3d_sketch_ToolCircle.cpp
    src/vtk3d/sketch/vtk3d_sketch_ToolRectEdges.h
    src/vtk3d/sketch/vtk3d_sketch_ToolRectEdges.cpp
    src/vtk3d/sketch/vtk3d_sketch_ToolRectCenter.h
    src/vtk3d/sketch/vtk3d_sketch_ToolRectCenter.cpp
    src/vtk3d/sketch/vtk3d_sketch_ToolDimensions.cpp
    src/vtk3d/sketch/vtk3d_sketch_ToolDimensions.h
    src/vtk3d/sketch/vtk3d_sketch_ToolSetConstraints.cpp
    src/vtk3d/sketch/vtk3d_sketch_ToolSetConstraints.h
    src/vtk3d/sketch/vtk3d_Sketch_Render_Cotations.h
    src/vtk3d/sketch/vtk3d_Sketch_Render_Cotations.cpp
    src/vtk3d/sketch/vtk3d_sketch_SnapperManager.h
    src/vtk3d/sketch/vtk3d_sketch_SnapperManager.cpp
    src/Cad/CAD_Part.cpp
    src/Cad/CAD_Part_tests.cpp
    src/Cad/CAD_Part.h
    src/Cad/CAD_Operation.cpp
    src/Cad/CAD_Operation.h
    src/Cad/CAD_Operation_extrude.cpp
    src/Cad/CAD_Operation_sketch.cpp
    src/generic/DimensionEngine.h
    src/generic/Contours.cpp
    src/generic/Contours.h
    src/generic/Contours_tests.cpp
    src/generic/Contours_tests.h
    src/generic/cad_events.h
    src/generic/2DSolver_Constraints.h
    src/generic/2DSolver_Mapper.cpp
    src/generic/2DSolver_Mapper.h
    src/generic/2DSolver_Solver.cpp
    src/generic/2DSolver_Solver.h
    src/generic/Logger.cpp
    src/generic/Logger.h
    src/generic/chrono.h
)

# 1. Lister tous les dossiers contenant des fichiers .h / .hpp
target_include_directories(Cad_IA PRIVATE
    src
    src/Cad
    src/generic
    src/Qt
    src/vtk3d
    src/vtk3d/sketch
    src/vtk3d/part
    src/External/Eigen
)

#qt_add_translations(
#    TARGETS Cad_IA
#    TS_FILES Cad_IA_en_US.ts
#)

# Édition des liens avec les libs
target_link_libraries(Cad_IA
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Svg
        ${VTK_LIBRARIES}
        TKernel TKBRep TKPrim TKBO TKTopAlgo TKMesh
)

# Configuration de l'installation et du déploiement automatique
install(TARGETS Cad_IA
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Cad_IA
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})