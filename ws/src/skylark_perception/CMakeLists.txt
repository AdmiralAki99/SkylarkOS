cmake_minimum_required(VERSION 3.8)
cmake_policy(SET CMP0057 NEW)
project(skylark_perception)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(skylark_interfaces REQUIRED)
find_package(OpenCV REQUIRED)

# Adding dependencies for external libraries
set(ONNX_RUNTIME /usr/local/onnxruntime)

include_directories(${ONNX_RUNTIME}/include)

# Adding excecutable and linking libraries
add_executable(perception_node src/perception_node.cpp)
option(USE_TENSORRT "Enable Tensor RT/CUDA execution providers" OFF)
if(USE_TENSORRT)
  target_compile_definitions(perception_node PRIVATE USE_TENSORRT)
endif()
ament_target_dependencies(perception_node rclcpp rclcpp_lifecycle sensor_msgs cv_bridge skylark_interfaces)

# Link ONNX Runtime library and OpenCV libraries
target_link_libraries(perception_node
  ${ONNX_RUNTIME}/lib/libonnxruntime.so
  ${OpenCV_LIBS}
)

install(TARGETS
  perception_node
  DESTINATION lib/${PROJECT_NAME}
)

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
