#include <cstring>
#include <filesystem>
#include <tiff.h>
#include <tiffio.h>

namespace ia {

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

class tiff_t {
public:
  tiff_t(const std::filesystem::path& p);
  ~tiff_t();

  void write_to(const std::filesystem::path& p);

protected:
  unsigned int image_width;
  unsigned int image_length;
  unsigned int channels_count;
  unsigned int channel_bits;
  photometric_t photometric_representation;
  unsigned char* raw_data = nullptr;

  void tiff_read_strips(TIFF* handle);
  void tiff_read_tiles(TIFF* handle);
};

}
