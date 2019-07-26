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

#include "../mapping_2d/scan_matching/ceres_scan_matcher.h"

#include <memory>

#include "../common/lua_parameter_dictionary.h"
#include "../common/lua_parameter_dictionary_test_helpers.h"
#include "../common/make_unique.h"
#include "../mapping_2d/probability_grid.h"
#include "../sensor/point_cloud.h"
#include "../transform/rigid_transform_test_helpers.h"
#include "gtest/gtest.h"

namespace cartographer {
namespace mapping_2d {
namespace scan_matching {
namespace {

class CeresScanMatcherTest : public ::testing::Test {
 protected:
  CeresScanMatcherTest()
      : probability_grid_(
            MapLimits(1., Eigen::Vector2d(10., 10.), CellLimits(20, 20))) {
    probability_grid_.SetProbability(
        probability_grid_.limits().GetXYIndexOfCellContainingPoint(-3.5, 2.5),
        mapping::kMaxProbability);

    point_cloud_.emplace_back(-3., 2.);

    auto parameter_dictionary = common::MakeDictionary(R"text(
        return {
          occupied_space_cost_functor_weight = 1.,
          previous_pose_translation_delta_cost_functor_weight = 0.1,
          initial_pose_estimate_rotation_delta_cost_functor_weight = 1.5,
          covariance_scale = 10.,
          ceres_solver_options = {
            use_nonmonotonic_steps = true,
            max_num_iterations = 50,
            num_threads = 1,
          },
        })text");
    const proto::CeresScanMatcherOptions options =
        CreateCeresScanMatcherOptions(parameter_dictionary.get());
    ceres_scan_matcher_ = common::make_unique<CeresScanMatcher>(options);
  }

  void TestFromInitialPose(const transform::Rigid2d& initial_pose) {
    transform::Rigid2d pose;
    kalman_filter::Pose2DCovariance covariance;
    const transform::Rigid2d expected_pose =
        transform::Rigid2d::Translation({-0.5, 0.5});
    ceres::Solver::Summary summary;
    ceres_scan_matcher_->Match(initial_pose, initial_pose, point_cloud_,
                               probability_grid_, &pose, &covariance, &summary);
    EXPECT_NEAR(0., summary.final_cost, 1e-2) << summary.FullReport();
    EXPECT_THAT(pose, transform::IsNearly(expected_pose, 1e-2))
        << "Actual: " << transform::ToProto(pose).DebugString()
        << "\nExpected: " << transform::ToProto(expected_pose).DebugString();
  }

  ProbabilityGrid probability_grid_;
  sensor::PointCloud2D point_cloud_;
  std::unique_ptr<CeresScanMatcher> ceres_scan_matcher_;
};

TEST_F(CeresScanMatcherTest, testPerfectEstimate) {
  TestFromInitialPose(transform::Rigid2d::Translation({-0.5, 0.5}));
}

TEST_F(CeresScanMatcherTest, testOptimizeAlongX) {
  TestFromInitialPose(transform::Rigid2d::Translation({-0.3, 0.5}));
}

TEST_F(CeresScanMatcherTest, testOptimizeAlongY) {
  TestFromInitialPose(transform::Rigid2d::Translation({-0.45, 0.3}));
}

TEST_F(CeresScanMatcherTest, testOptimizeAlongXY) {
  TestFromInitialPose(transform::Rigid2d::Translation({-0.3, 0.3}));
}

}  // namespace
}  // namespace scan_matching
}  // namespace mapping_2d
}  // namespace cartographer
