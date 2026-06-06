cmake_minimum_required(VERSION 3.8)
cmake_policy(SET CMP0057 NEW)
project(skylark_camera)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)

# GStreamer has no CMake module — locate it via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0)

add_executable(camera_node src/camera_node.cpp)

target_include_directories(camera_node PRIVATE
  ${GST_INCLUDE_DIRS}
  ${GST_APP_INCLUDE_DIRS}
  ${GST_VIDEO_INCLUDE_DIRS}
)

ament_target_dependencies(camera_node rclcpp std_msgs sensor_msgs)

target_link_libraries(camera_node
  ${GST_LIBRARIES}
  ${GST_APP_LIBRARIES}
  ${GST_VIDEO_LIBRARIES}
)

install(TARGETS
  camera_node
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
