cmake_minimum_required(VERSION 2.8.3)
project(perception_msgs)
find_package(catkin REQUIRED COMPONENTS
  cv_bridge
  image_geometry
  message_generation
  std_msgs
  sensor_msgs
  geometry_msgs
  pcl_msgs
  image_transport
  pcl_ros
  roscpp
)

find_package(OpenCV REQUIRED)

add_message_files(FILES
  Obstacle.msg
  ObstacleList.msg
  ObstacleIndices.msg
  ObstacleIndicesList.msg
)

generate_messages(DEPENDENCIES geometry_msgs sensor_msgs std_msgs pcl_msgs)

catkin_package(
    INCLUDE_DIRS include
    LIBRARIES perception_msgs_lib
    CATKIN_DEPENDS message_runtime std_msgs sensor_msgs  geometry_msgs pcl_msgs
)


include_directories(
  include
  ${catkin_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
)

## Declare a C++ library
add_library(perception_msgs_lib
    src/${PROJECT_NAME}/perception_msgs_utils.cpp
)
add_dependencies(perception_msgs_lib
                ${catkin_EXPORTED_TARGETS}
                ${PROJECT_NAME}_generate_messages_cpp
                ${PROJECT_NAME}_gencfg)
target_link_libraries(perception_msgs_lib
  ${catkin_LIBRARIES}
)

add_executable(obstaclelist_cuboid_painter src/obstaclelist_cuboid_painter.cpp)

add_dependencies(obstaclelist_cuboid_painter ${catkin_EXPORTED_TARGETS} ${PROJECT_NAME}_generate_messages_cpp ${PROJECT_NAME}_gencfg)
target_link_libraries(obstaclelist_cuboid_painter
  ${catkin_LIBRARIES}
  ${OpenCV_LIBS}
  ${PCL_LIBRARIES}
)
