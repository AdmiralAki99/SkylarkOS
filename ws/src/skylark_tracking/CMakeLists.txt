cmake_minimum_required(VERSION 3.8)
cmake_policy(SET CMP0057 NEW)
project(skylark_tracking)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(skylark_interfaces REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(tracking_node 
  src/tracking_node.cpp 
  src/Hungarian.cpp
  src/KalmanTrack.cpp
)

ament_target_dependencies(tracking_node rclcpp rclcpp_lifecycle skylark_interfaces)
target_link_libraries(tracking_node ${OpenCV_LIBS})
target_include_directories(tracking_node PUBLIC include/)

install(TARGETS tracking_node DESTINATION lib/${PROJECT_NAME})


if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
