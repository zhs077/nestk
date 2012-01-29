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
 * Author: Nicolas Burrus <nicolas.burrus@uc3m.es>, (C) 2010
 */

#ifndef NTK_IMAGE_BILATERAL_FILTER_H
#define NTK_IMAGE_BILATERAL_FILTER_H

# include <ntk/core.h>

namespace ntk
{

void depth_bilateralFilter( const cv::Mat1f& src, cv::Mat1f& dst,
                            int d, double sigma_color, double sigma_space,
                            float maximal_delta_depth_percent = 0.005, // 5mm @ 1meter
                            int borderType = cv::BORDER_DEFAULT);

} // ntk

#endif // NTK_IMAGE_BILATERAL_FILTER_H