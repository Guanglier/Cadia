include("C:/__Perso/test_prog/Cadia/trunk/build/Desktop_MSVC_2022_amd64_debug-Debug/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Cad_IA-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "C:/__Perso/test_prog/Cadia/trunk/build/Desktop_MSVC_2022_amd64_debug-Debug/Cad_IA.exe"
    GENERATE_QT_CONF
)
