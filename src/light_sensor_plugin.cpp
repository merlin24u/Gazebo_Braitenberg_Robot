#include <ros/ros.h>
#include <sensor_msgs/Illuminance.h>
#include <gazebo/common/Plugin.hh>
#include <gazebo/sensors/Sensor.hh>
#include <gazebo/sensors/CameraSensor.hh>
#include <gazebo/sensors/SensorTypes.hh>
#include <gazebo_plugins/gazebo_ros_camera.h>
#include "gazebo_light_sensor_plugin/light_sensor_plugin.hpp"
#include <string>

namespace gazebo
{
  // Constructor
  GazeboRosLight::GazeboRosLight(): n("light_sensor_plugin"), it(n), fov(6), range(10){
    // Subscrive to input video feed
    image_sub = it.subscribe("/camera/rgb/image_raw", 100,
			     &gazebo::GazeboRosLight::imagePub, this);

    sensorPublisher = n.advertise<sensor_msgs::Illuminance>("lightSensor", 1);
  }

  // Destructor
  GazeboRosLight::~GazeboRosLight()
  {
    ROS_DEBUG_STREAM_NAMED("camera","Unloaded");
  }

  void GazeboRosLight::Load(sensors::SensorPtr parent, sdf::ElementPtr sdf)
  {
    // Make sure the ROS node for Gazebo has already been initialized
    if (!ros::isInitialized())
      {
	ROS_FATAL_STREAM("A ROS node for Gazebo has not been initialized, unable to load plugin. "
			 << "Load the Gazebo system plugin 'libgazebo_ros_api_plugin.so' in the gazebo_ros package)");
	return;
      }

    CameraPlugin::Load(parent, sdf);
    // copying from CameraPlugin into GazeboRosCameraUtils
    this->parentSensor_ = this->parentSensor;
    this->width_ = this->width;
    this->height_ = this->height;
    this->depth_ = this->depth;
    this->format_ = this->format;
    this->camera_ = this->camera;

    GazeboRosCameraUtils::Load(parent, sdf);
  }

  // Update the controller
  void GazeboRosLight::OnNewFrame(const unsigned char *_image,
				  unsigned int _width, unsigned int _height, unsigned int _depth,
				  const std::string &_format){
    
    this->sensor_update_time_ = this->parentSensor_->LastUpdateTime();

    if(!this->parentSensor->IsActive()){
      if((*this->image_connect_count_) > 0)
	// do this first so there's chance for sensor to run once after activated
	this->parentSensor->SetActive(true);
    }
    else{
      if((*this->image_connect_count_) > 0){
	common::Time cur_time = this->world_->SimTime();
	if(cur_time - this->last_update_time_ >= this->update_period_){
	  this->PutCameraData(_image);
	  this->PublishCameraInfo();
	  this->last_update_time_ = cur_time;
	}
      }
    }
  }
  
  void GazeboRosLight::imagePub(const sensor_msgs::ImageConstPtr &img){
    static int seq = 0;

    sensor_msgs::Illuminance msg;
    msg.header.stamp = ros::Time::now();
    msg.header.frame_id = "";
    msg.header.seq = seq;

    int startingPix = img->width * ((int)(img->height/2) - (int)(fov/2)) - (int)(fov/2);

    float illum = 0;
    for(int i = 0; i < fov; ++i){
      int index = startingPix + i * img->width;
      for (int j = 0; j < fov; ++j)
	illum += img->data[index + j];
    }

    msg.illuminance = illum / (fov * fov);
    msg.variance = 0.0;

    sensorPublisher.publish(msg);

    seq++;
  }
  
  // Register this plugin with the simulator
  GZ_REGISTER_SENSOR_PLUGIN(GazeboRosLight)
}
