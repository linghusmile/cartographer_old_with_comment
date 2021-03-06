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

#ifndef CARTOGRAPHER_KALMAN_FILTER_ODOMETRY_STATE_TRACKER_H_
#define CARTOGRAPHER_KALMAN_FILTER_ODOMETRY_STATE_TRACKER_H_

#include <deque>

#include "../common/time.h"
#include "../transform/rigid_transform.h"

namespace cartographer {
namespace kalman_filter {

/*
 * 里程计的状态
*/
struct OdometryState
{
  OdometryState(common::Time time, const transform::Rigid3d& odometer_pose,
                const transform::Rigid3d& state_pose);
  OdometryState() {}

  //时间
  common::Time time = common::Time::min();

  //里程计位姿
  transform::Rigid3d odometer_pose = transform::Rigid3d::Identity();

  //状态位姿　即滤波器估计的时刻time的　机器人的真实位姿
  transform::Rigid3d state_pose = transform::Rigid3d::Identity();
};

// Keeps track of the odometry states by keeping sliding window over some
// number of them.
// 所谓的OdometryStateTracker相当于OdometryState的缓冲器
// 保存最近的winodw_size个里程计数据
class OdometryStateTracker
{
 public:
  //用来存储OdometryState的双端队列
  using OdometryStates = std::deque<OdometryState>;

  explicit OdometryStateTracker(int window_size);

  //得到一整个缓冲区
  const OdometryStates& odometry_states() const;

  // Adds a new 'odometry_state' and makes sure the maximum number of previous
  // odometry states is not exceeded.
  // 增加一个OdometryState到队列中
  void AddOdometryState(const OdometryState& odometry_state);

  // Returns true if no elements are present in the odometry queue.
  // 判断是否为空
  bool empty() const;

  // Retrieves the most recent OdometryState or an empty one if non yet present.
  //　返回最近的OdometryState
  const OdometryState& newest() const;

 private:
  OdometryStates odometry_states_;  //双端队列
  size_t window_size_;              //窗口大小
};

}  // namespace kalman_filter
}  // namespace cartographer

#endif  // CARTOGRAPHER_KALMAN_FILTER_ODOMETRY_STATE_TRACKER_H_
