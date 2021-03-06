// __BEGIN_LICENSE__
//  Copyright (c) 2006-2012, United States Government as represented by the
//  Administrator of the National Aeronautics and Space Administration. All
//  rights reserved.
//
//  The NASA Vision Workbench is licensed under the Apache License,
//  Version 2.0 (the "License"); you may not use this file except in
//  compliance with the License. You may obtain a copy of the License at
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
// __END_LICENSE__

#ifdef _MSC_VER
#pragma warning(disable:4244)
#pragma warning(disable:4267)
#pragma warning(disable:4996)
#endif

#include <cstdlib>
#include <iostream>
#include <cmath>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/foreach.hpp>
namespace fs = boost::filesystem;
namespace po = boost::program_options;

#include <vw/Core/Functors.h>
#include <vw/Image/Algorithms.h>
#include <vw/Image/ImageMath.h>
#include <vw/Image/ImageViewRef.h>
#include <vw/Image/PerPixelViews.h>
#include <vw/Image/PixelMask.h>
#include <vw/Image/MaskViews.h>
#include <vw/Image/PixelTypes.h>
#include <vw/Image/Statistics.h>
#include <vw/FileIO/DiskImageView.h>
#include <vw/Cartography/GeoReference.h>
#include <vw/tools/Common.h>
#include <vw/FileIO/DiskImageResourceGDAL.h>
#include <vw/Image/Interpolation.h>
#include <asp/Core/Macros.h>
#include <asp/Core/Common.h>
#include <asp/Tools/point2dem.h>
#include <cstdlib>
using namespace vw;
using namespace vw::cartography;

// Allows FileIO to correctly read/write these pixel types
namespace vw {
  typedef Vector<float64,6> Vector6;
  template<> struct PixelFormatID<Vector3>   { static const PixelFormatEnum value = VW_PIXEL_GENERIC_3_CHANNEL; };
  template<> struct PixelFormatID<Vector3f>  { static const PixelFormatEnum value = VW_PIXEL_GENERIC_3_CHANNEL; };
  template<> struct PixelFormatID<Vector4>   { static const PixelFormatEnum value = VW_PIXEL_GENERIC_4_CHANNEL; };
  template<> struct PixelFormatID<Vector6>   { static const PixelFormatEnum value = VW_PIXEL_GENERIC_6_CHANNEL; };
}

struct Options : asp::BaseOptions {};

int main( int argc, char *argv[] ){

  double shiftx = 0, shifty = 0, angle = 0;
  if (argc < 6){
    std::cout << "Usage: input.tif output.tif shiftx shifty angle" << std::endl;
    return 1;
  }

  std::string in_pc_file = argv[1];
  std::string out_pc_file = argv[2];
  std::cout << "real shift + rotation" << std::endl;
  shiftx = atof(argv[3]);
  shifty = atof(argv[4]);
  angle  = atof(argv[5]);

  Vector3 shift(shiftx, shifty, 0);
  std::cout << "pc shift and angle " << in_pc_file << " "
            << shiftx << ' ' << shifty << ' ' << angle << std::endl;
  std::cout << "Reading: " << in_pc_file << std::endl;
  ImageView<Vector3> in_point_image = asp::read_cloud<3>(in_pc_file);

  double theta = angle*2*M_PI/360;
  Matrix3x3 M;
  M(0, 0) = cos(theta);
  M(0, 1) = -sin(theta);
  M(1, 0) = sin(theta);
  M(1, 1) = cos(theta);
  M(2, 2) = 1.0;
  std::cout << "rotation matrix is " << M << std::endl;

  double max_disp = 0.0;
  ImageView<Vector3> out_point_image(in_point_image.cols(), in_point_image.rows());
  for (int col = 0; col < out_point_image.cols(); col++){
    for (int row = 0; row < out_point_image.rows(); row++){
      if (in_point_image(col, row) == Vector3()){
        out_point_image(col, row) =  Vector3();
      }else{
        out_point_image(col, row) = M*in_point_image(col, row) + shift;
        max_disp = std::max(max_disp, norm_2(out_point_image(col, row) - in_point_image(col, row)));
      }
    }
  }

  std::cout << "max_disp is " << max_disp << std::endl;

  std::cout << "Writing: " << out_pc_file << std::endl;
  Options opt;
 asp::block_write_gdal_image(out_pc_file, out_point_image, opt,
                              TerminalProgressCallback("asp", "\t-->: "));

  return 0;

}
