/*
 * Copyright 2016 The Cartographer Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../sensor/laser.h"
#include "../transform/transform.h"

#include "cartographer/sensor/proto/sensor.pb.h"

namespace cartographer {
namespace sensor {

namespace {

// Reorders reflectivities according to index mapping.
std::vector<uint8> ReorderReflectivities(
    const std::vector<uint8>& reflectivities,
    const std::vector<int>& new_to_old) {
  std::vector<uint8> reordered(reflectivities.size());
  for (size_t i = 0; i < reordered.size(); ++i) {
    reordered[i] = reflectivities[new_to_old[i]];
  }
  return reordered;
}

}  // namespace

LaserFan ToLaserFan(const proto::LaserScan& proto, const float min_range,
                    const float max_range,
                    const float missing_echo_ray_length) {
  CHECK_GE(min_range, 0.f);
  CHECK_GT(proto.angle_increment(), 0.f);
  CHECK_GT(proto.angle_max(), proto.angle_min());
  LaserFan laser_fan = {Eigen::Vector2f::Zero(), {}, {}};
  float angle = proto.angle_min();
  for (const auto& range : proto.range()) {
    if (range.value_size() > 0) {
      const float first_echo = range.value(0);
      if (!std::isnan(first_echo) && first_echo >= min_range) {
        if (first_echo <= max_range) {
          laser_fan.point_cloud.push_back(Eigen::Rotation2Df(angle) *
                                          Eigen::Vector2f(first_echo, 0.f));
        } else {
          laser_fan.missing_echo_point_cloud.push_back(
              Eigen::Rotation2Df(angle) *
              Eigen::Vector2f(missing_echo_ray_length, 0.f));
        }
      }
    }
    angle += proto.angle_increment();
  }
  return laser_fan;
}

LaserFan ProjectCroppedLaserFan(const LaserFan3D& laser_fan,
                                const Eigen::Vector3f& min,
                                const Eigen::Vector3f& max)
{
  return LaserFan{laser_fan.origin.head<2>(),
                  ProjectToPointCloud2D(Crop(laser_fan.returns, min, max)),
                  ProjectToPointCloud2D(Crop(laser_fan.misses, min, max))};
}

LaserFan TransformLaserFan(const LaserFan& laser_fan,
                           const transform::Rigid2f& transform)
{
  return LaserFan{
      transform * laser_fan.origin,
      TransformPointCloud2D(laser_fan.point_cloud, transform),
      TransformPointCloud2D(laser_fan.missing_echo_point_cloud, transform)};
}

LaserFan3D ToLaserFan3D(const LaserFan& laser_fan)
{
  return LaserFan3D{
      Eigen::Vector3f(laser_fan.origin.x(), laser_fan.origin.y(), 0.),
      ToPointCloud(laser_fan.point_cloud),
      ToPointCloud(laser_fan.missing_echo_point_cloud)};
}

LaserFan3D Decompress(const CompressedLaserFan3D& compressed_laser_fan)
{
  return LaserFan3D{compressed_laser_fan.origin,
                    compressed_laser_fan.returns.Decompress(),
                    compressed_laser_fan.misses.Decompress(),
                    compressed_laser_fan.reflectivities};
}

CompressedLaserFan3D Compress(const LaserFan3D& laser_fan)
{
  std::vector<int> new_to_old;
  CompressedPointCloud compressed_returns =
      CompressedPointCloud::CompressAndReturnOrder(laser_fan.returns,
                                                   &new_to_old);
  return CompressedLaserFan3D{
      laser_fan.origin, std::move(compressed_returns),
      CompressedPointCloud(laser_fan.misses),
      ReorderReflectivities(laser_fan.reflectivities, new_to_old)};
}

LaserFan3D TransformLaserFan3D(const LaserFan3D& laser_fan,
                               const transform::Rigid3f& transform)
{
  return LaserFan3D{
      transform * laser_fan.origin,
      TransformPointCloud(laser_fan.returns, transform),
      TransformPointCloud(laser_fan.misses, transform),
      laser_fan.reflectivities,
  };
}

proto::LaserFan3D ToProto(const LaserFan3D& laser_fan)
{
  proto::LaserFan3D proto;
  *proto.mutable_origin() = transform::ToProto(laser_fan.origin);
  *proto.mutable_point_cloud() = ToProto(laser_fan.returns);
  *proto.mutable_missing_echo_point_cloud() = ToProto(laser_fan.misses);
  std::copy(laser_fan.reflectivities.begin(), laser_fan.reflectivities.end(),
            RepeatedFieldBackInserter(proto.mutable_reflectivity()));
  return proto;
}

LaserFan3D FromProto(const proto::LaserFan3D& proto)
{
  auto laser_fan_3d = LaserFan3D{
      transform::ToEigen(proto.origin()), ToPointCloud(proto.point_cloud()),
      ToPointCloud(proto.missing_echo_point_cloud()),
  };
  std::copy(proto.reflectivity().begin(), proto.reflectivity().end(),
            std::back_inserter(laser_fan_3d.reflectivities));
  return laser_fan_3d;
}

/*
 * 对激光帧数据进行滤波，只保留测量范围在max_range范围内的点．
 * 那么miss point会被去除，同时反射率信息也会被去除
*/
LaserFan3D FilterLaserFanByMaxRange(const LaserFan3D& laser_fan,
                                    const float max_range)
{
  LaserFan3D result{laser_fan.origin, {}, {}, {}};
  for (const Eigen::Vector3f& return_ : laser_fan.returns)
  {
    if ((return_ - laser_fan.origin).norm() <= max_range)
    {
      result.returns.push_back(return_);
    }
  }
  return result;
}

}  // namespace sensor
}  // namespace cartographer
