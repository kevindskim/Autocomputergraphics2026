include("D:/Graphics/opengl/cube-v0/out/build/msvc2022-x64/.qt/QtDeploySupport-RelWithDebInfo.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cube-plugins-RelWithDebInfo.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "D:/Graphics/opengl/cube-v0/out/build/msvc2022-x64/RelWithDebInfo/cube.exe"
    GENERATE_QT_CONF
)
