/*
 * Copyright 2019 Bonn-Rhein-Sieg University
 *
 * Author: Mohammad Wasil, Santosh Thoduka
 *
 */

#include <fstream>
#include <iostream>

#include <opencv2/highgui/highgui.hpp>

#include <pcl/PCLPointCloud2.h>
#include <pcl/common/centroid.h>
#include <pcl/common/common.h>
#include <pcl/common/transforms.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/filters/passthrough.h>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl_ros/point_cloud.h>

#include <mir_perception_utils/object_utils_ros.h>
#include <mir_perception_utils/impl/helpers.hpp>

using namespace mir_perception_utils;
void object::estimatePose(const BoundingBox &box, geometry_msgs::PoseStamped &pose)
{
  BoundingBox::Points vertices = box.getVertices();
  Eigen::Vector3f n1;
  Eigen::Vector3f n2;
  Eigen::Vector3f n3 = (vertices[4] - vertices[0]) / (vertices[4] - vertices[0]).norm();
  if ((vertices[1] - vertices[0]).norm() > (vertices[3] - vertices[0]).norm()) {
    n1 = (vertices[1] - vertices[0]) / (vertices[1] - vertices[0]).norm();
  } else {
    n1 = (vertices[3] - vertices[0]) / (vertices[3] - vertices[0]).norm();
  }
  n2 = n3.cross(n1);
  ROS_INFO_STREAM("got norms");
  Eigen::Matrix3f m;
  m << n1, n2, n3;
  Eigen::Quaternion<float> q(m);
  q.normalize();

  Eigen::Vector3f centroid = box.getCenter();
  pose.pose.position.x = centroid(0);
  pose.pose.position.y = centroid(1);
  pose.pose.position.z = (vertices[0](2) + vertices[1](2) + vertices[2](2) + vertices[3](2)) / 4.0;
  pose.pose.orientation.x = q.x();
  pose.pose.orientation.y = q.y();
  pose.pose.orientation.z = q.z();
  pose.pose.orientation.w = q.w();
}

PointCloud object::estimatePose(const PointCloud::Ptr &xyz_input_cloud, geometry_msgs::PoseStamped &pose, 
                          mas_perception_msgs::Object &object,
                          std::string shape, float passthrough_lim_min_offset,
                          float passthrough_lim_max_offset,
                          std::string obj_category)
{
  // Apply filter to remove points belonging to the plane for non
  // circular/spherical object
  // to find its orientation
  PointCloud filtered_cloud;
  if (obj_category == "atwork" and (shape == "sphere" or shape == "flat")) {
    filtered_cloud = *xyz_input_cloud;
  } else {
    pcl::PassThrough<PointT> pass_through;
    pass_through.setFilterFieldName("z");
    PointT min_pt;
    PointT max_pt;
    pcl::getMinMax3D(*xyz_input_cloud, min_pt, max_pt);
    double limit_min = min_pt.z + passthrough_lim_min_offset;
    double limit_max = max_pt.z + passthrough_lim_max_offset;
    if (obj_category == "cavity")
    {
      limit_max = max_pt.z - 0.015; // TODO: make 0.01 as a dynamic configurable parameter
      if (object.name == "M20_H")
      {
          limit_max = max_pt.z - 0.02;
      }
      pass_through.setFilterLimits(limit_min, limit_max);
      pass_through.setInputCloud(xyz_input_cloud);
      pass_through.filter(filtered_cloud);

      // change z value to max(filterpointcloud z) This is only for cavity
      for (size_t i = 0; i < filtered_cloud.points.size(); ++i) {
        // check if filtered_cloud.points[i].z has not nan or inf value
        if (!std::isnan(filtered_cloud.points[i].z) && !std::isinf(filtered_cloud.points[i].z)) {
          filtered_cloud.points[i].z = 0.015; // 0.045 is an empirical values, TODO: get 0.045 from target_pose_z_pos from config file
        }
      }
    }
    else{
      if (object.name != "M20" && object.name != "M30" && object.name != "F20_20_G") {
        pass_through.setFilterLimits(limit_min, limit_max);
        pass_through.setInputCloud(xyz_input_cloud);
        pass_through.filter(filtered_cloud);
      }
      else{
        filtered_cloud = *xyz_input_cloud;
      }
    }
  }

  Eigen::Vector4f centroid;
  pcl::compute3DCentroid(filtered_cloud, centroid);

  Eigen::Matrix3f covariance;
  pcl::computeCovarianceMatrixNormalized(filtered_cloud, centroid, covariance);

  Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigen_solver(covariance,
                                                              Eigen::ComputeEigenvectors);
  Eigen::Matrix3f eigen_vectors = eigen_solver.eigenvectors();

  // swap largest and second largest eigenvector so that y-axis aligns with
  // largest eigenvector and z with the second largest
  eigen_vectors.col(0).swap(eigen_vectors.col(2));
  eigen_vectors.col(1) = eigen_vectors.col(2).cross(eigen_vectors.col(0));

  Eigen::Matrix4f eigen_vector_transform(Eigen::Matrix4f::Identity());
  eigen_vector_transform.block<3, 3>(0, 0) = eigen_vectors.transpose();
  eigen_vector_transform.block<3, 1>(0, 3) =
      -(eigen_vector_transform.block<3, 3>(0, 0) * centroid.head<3>());

  // transform cloud to eigenvector space
  pcl::PointCloud<pcl::PointXYZRGB> transformed_cloud;
  pcl::transformPointCloud(filtered_cloud, transformed_cloud, eigen_vector_transform);

  // find mean diagonal
  pcl::PointXYZRGB min_point, max_point;
  pcl::getMinMax3D(transformed_cloud, min_point, max_point);
  Eigen::Vector3f mean_diag = (max_point.getVector3fMap() + min_point.getVector3fMap()) / 2.0;

  // orientation and position of bounding box of cloud
  Eigen::Quaternionf orientation(eigen_vectors);
  Eigen::Vector3f position = eigen_vectors * mean_diag + centroid.head<3>();

  pose.pose.position.x = position(0);
  pose.pose.position.y = position(1);
  pose.pose.position.z = position(2);
  pose.pose.orientation.w = orientation.w();
  pose.pose.orientation.x = orientation.x();
  pose.pose.orientation.y = orientation.y();
  pose.pose.orientation.z = orientation.z();

  tf::Quaternion quaternion;
  tf::quaternionMsgToTF(pose.pose.orientation, quaternion);
  tf::Matrix3x3 m(quaternion);
  double roll, pitch, yaw;
  m.getRPY(roll, pitch, yaw);

  pose.pose.orientation = tf::createQuaternionMsgFromRollPitchYaw(0, 0, yaw);

  return filtered_cloud;
}

void object::transformPose(const boost::shared_ptr<tf::TransformListener> tf_listener,
                           const std::string &target_frame, const geometry_msgs::PoseStamped &pose,
                           geometry_msgs::PoseStamped &transformed_pose)
{
  if (tf_listener) {
    try {
      ros::Time common_time;
      tf_listener->getLatestCommonTime(pose.header.frame_id, target_frame, common_time, NULL);
      pose.header.stamp = common_time;
      tf_listener->waitForTransform(target_frame, pose.header.frame_id, common_time,
                                    ros::Duration(0.1));
      tf_listener->transformPose(target_frame, pose, transformed_pose);
    } catch (tf::LookupException &ex) {
      ROS_WARN("Failed to transform pose: (%s)", ex.what());
      transformed_pose = pose;
    }
  } else {
    ROS_ERROR_THROTTLE(2.0, "[ObjectUtils]: TF listener not initialized.");
    transformed_pose = pose;
  }
}

void object::get3DBoundingBox(const PointCloud::ConstPtr &cloud, const Eigen::Vector3f &normal,
                              BoundingBox &bbox, mas_perception_msgs::BoundingBox &bounding_box_msg)
{
  bbox = BoundingBox::create(cloud->points, normal);
  convertBoundingBox(bbox, bounding_box_msg);
}

void object::convertBboxToMsg(const BoundingBox &bbox,
                              mas_perception_msgs::BoundingBox &bounding_box_msg)
{
  convertBoundingBox(bbox, bounding_box_msg);
}

void object::savePcd(const PointCloud::ConstPtr &pointcloud, std::string log_dir,
                     std::string obj_name)
{
  std::stringstream filename;
  filename.str("");
  filename << log_dir << obj_name << ".pcd";
  pcl::io::savePCDFileASCII(filename.str(), *pointcloud);
}

void object::saveCVImage(const cv_bridge::CvImagePtr &cv_image, std::string log_dir,
                         std::string obj_name)
{
  std::stringstream filename;
  filename.str("");
  filename << log_dir << obj_name << ".jpg";
  cv::imwrite(filename.str(), cv_image->image);
}

bool object::getCVImage(const sensor_msgs::ImageConstPtr &image, cv_bridge::CvImagePtr &cv_image)
{
  try {
    cv_image = cv_bridge::toCvCopy(image, sensor_msgs::image_encodings::BGR8);
    return (true);
  } catch (cv_bridge::Exception &e) {
    ROS_ERROR("cv_bridge exception: %s", e.what());
    return (false);
  }
}
