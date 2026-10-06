include("D:/Graphics/opengl/cube-v1/out/build/msvc2022-x64/.qt/QtDeploySupport-Release.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cube-v1-plugins-Release.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "D:/Graphics/opengl/cube-v1/out/build/msvc2022-x64/Release/cube-v1.exe"
    GENERATE_QT_CONF
)
