/**
 * This file is part of the nestk library.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Nicolas Burrus <nicolas.burrus@manctl.com>, (C) 2012
 */

#ifndef NESTK_GEOMETRY_RELATIVE_POSE_ESTIMATOR_RGBD_ICP_H
#define NESTK_GEOMETRY_RELATIVE_POSE_ESTIMATOR_RGBD_ICP_H

#include "relative_pose_estimator_icp.h"

namespace ntk
{

template <class PointT>
class RelativePoseEstimatorRGBDICP : public RelativePoseEstimatorICPWithNormals<PointT>
{
protected:
    typedef RelativePoseEstimatorICPWithNormals<PointT> super;
    typedef pcl::PointCloud<PointT> PointCloudType;
    typedef typename PointCloudType::ConstPtr PointCloudConstPtr;
    typedef typename PointCloudType::Ptr PointCloudPtr;

    using super::m_max_iterations;
    using super::m_distance_threshold;
    using super::m_ransac_outlier_threshold;

public:
    RelativePoseEstimatorRGBDICP() {}


protected:
    virtual bool computeRegistration(Pose3D& relative_pose,
                                     PointCloudConstPtr source_cloud,
                                     PointCloudConstPtr target_cloud,
                                     PointCloudType& aligned_cloud);
};

} // ntk

#endif // NESTK_GEOMETRY_RELATIVE_POSE_ESTIMATOR_RGBD_ICP_H
