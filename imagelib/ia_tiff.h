#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>
#include <map>
#include <numeric>
#include <tiff.h>
#include <tiffio.h>
#include <unordered_map>
#include <vector>

// Some Linux distributions package the Eigen lib
// under .../include/eigen3/Eigen, including mine,
// but probably not yours
/*
#if __has_include(<Eigen/Core>) && __has_include(<Eigen/LU>)
# include <Eigen/Core>
# include <Eigen/LU>
#else
# include <eigen3/Eigen/Core>
# include <eigen3/Eigen/LU>
#endif
*/

namespace ia {

enum class channels_t {
  grayscale,
  rgb
};

enum class photometric_t: unsigned int {
  min_is_black = PHOTOMETRIC_MINISBLACK,
  min_is_white = PHOTOMETRIC_MINISWHITE,
  rgb = PHOTOMETRIC_RGB,
  ycbcr = PHOTOMETRIC_YCBCR
};

template<typename T>
constexpr auto to_underlying_type(T t) {
  return static_cast<typename std::underlying_type_t<T>>(t);
}

struct curve_segment_t {
  unsigned char begin_x, begin_y, end_x, end_y;
};

struct bounding_box_t {
  unsigned int left, bottom, right, top;
};

class tiff_t {
public:
  tiff_t();
  // tiff_t(unsigned char* data);
  tiff_t(const std::filesystem::path& p);
  tiff_t(tiff_t* other) noexcept;
  tiff_t(tiff_t* red, tiff_t* green, tiff_t* blue);
  ~tiff_t();

  unsigned int width() const;
  unsigned int length() const;
  unsigned int channels() const;
  unsigned int bit_depth() const;
  photometric_t photometric_repr() const;
  unsigned char* data() const;
  void write_to(const std::filesystem::path& p);
  static std::vector<tiff_t*> from_directories(const std::filesystem::path& p);
  // void set_bounds(bounding_box_t new_bbox);

  // Intensity transformations (all of them assume 8BPC grayscale)
  void negate();
  void threshold(const unsigned int value);
  void threshold(const unsigned int lower, const unsigned int upper);
  void gamma_correct(const float gamma);
  void equalize();
  template <typename ...Args>
  void curves(curve_segment_t segment = { 0, 0, 255, 255 }, Args... segments) {
    auto lut = std::vector<unsigned char>(std::numeric_limits<unsigned char>::max(), 0);

    for (auto&& s : { segment, segments... }) {
      // Interpolate segment values and populate the LUT
      for(auto x = s.begin_x; x < s.end_x; x++) {
        unsigned char lerp = (x - s.begin_x) * (s.end_y - s.begin_y) / (1 + s.end_x - s.begin_x);

        lut[x] = std::max(
          static_cast<unsigned char>(0),
          std::min(
            static_cast<unsigned char>(lerp),
            std::numeric_limits<unsigned char>::max()
          )
        );
      }
    }

    // Apply levels
    for (unsigned int y = 0; y < this->length(); y++) {
      for (unsigned int x = 0; x < this->width(); x++) {
        auto offset = y * this->width() + x;
        raw_data[offset] = lut[raw_data[offset]];
      }
    }
  }

protected:
  // Eigen::Vector3f world_coords{ 0.0f, 0.0f, 1.0f };
  // Eigen::Vector3i index_coords{ 0, 0, 1 };
  // bounding_box_t bbox;

  // using eigen_raw_data_t = Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
  // Eigen::Map<eigen_raw_data_t> eigen_raw_data{nullptr};
  // Eigen::Matrix3f matrix;
  // void index_to_local();
  // void local_to_index();

  unsigned int image_width;
  unsigned int image_length;
  unsigned int channels_count;
  unsigned int channel_bits;
  photometric_t photometric_representation;
  unsigned char* raw_data = nullptr;

  void tiff_read_strips(TIFF* handle);
  void tiff_read_tiles(TIFF* handle);
  std::vector<unsigned int> get_histogram();
};

}
