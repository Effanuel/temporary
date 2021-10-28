#include <filesystem>
#include <functional>
#include <memory>
#include <tiffio.h>
#include <eigen3/Eigen/Dense>
#include <type_traits>
#include <exception>

namespace ia {

enum class channel_t: unsigned int {
  grayscale = 0,
  rgb = 3,
  rgba = 4
};

using bpc_8 = unsigned char;
using bpc_16 = unsigned short int;
using bpc_32 = unsigned long int;
using bpc_64 = unsigned long long int;

template<typename T>
constexpr auto to_underlying_type(T t) {
  return static_cast<typename std::underlying_type_t<T>>(t);
}

template <channel_t channels,
          typename bit_depth_t,
          std::enable_if_t<std::is_integral<bit_depth_t>::value ||
                           std::is_unsigned<bit_depth_t>::value,
                           bool> = true>
class pixel_image_t {
protected:
  /// Pixel class specializations

  // At compile time, either `pixel_rgb_t` or `pixel_grayscale_t` is picked
  // as the base class for `pixel_t` to inherit from

  // Used for RGB pixels of `bit_depth_t` bit depth
  class pixel_rgb_t {
  protected:
    bit_depth_t r = {}, g = {}, b = {};
    pixel_rgb_t(unsigned char* buffer, unsigned int offset):
      r(static_cast<unsigned char*>(buffer)[offset]),
      g(static_cast<unsigned char*>(buffer)[offset + 1]),
      b(static_cast<unsigned char*>(buffer)[offset + 2]) {}

  public:
    pixel_rgb_t(bit_depth_t red, bit_depth_t green, bit_depth_t blue): r(red), g(green), b(blue) {}
    pixel_rgb_t(): r(0), g(0), b(0) {}

    auto red() const {
      return this->r;
    }

    auto green() const {
      return this->g;
    }

    auto blue() const {
      return this->b;
    }
  };

  // Used for single-channel pixels of `bit_depth_t` bit depth
  class pixel_grayscale_t {
  protected:
    bit_depth_t inten = {};
    pixel_grayscale_t(unsigned char* buffer, unsigned int offset): inten(static_cast<unsigned char*>(buffer)[offset]) {}

  public:
    pixel_grayscale_t(bit_depth_t intensity): inten(intensity) {}
    pixel_grayscale_t(): inten(0) {}

    auto intensity() const {
      return this->inten;
    }
  };

  // Pixel class parent type alias, conditionally specializing in either RGB
  // or Grayscale at compile time
  using pixel_specialized_t =
    std::conditional_t<
      std::is_same_v<
        std::integral_constant<channel_t, channels>,
        std::integral_constant<channel_t, channel_t::rgb>
      >,
      pixel_rgb_t,
      pixel_grayscale_t
    >;

public:
  class pixel_t: public pixel_specialized_t {
  public:
    // Forward all arguments to the specialized pixel class c-tor
    template<typename... Args>
    pixel_t(Args&&... args): pixel_specialized_t(std::forward<Args>(args)...) {}

    pixel_t(): pixel_specialized_t() {}
  };

protected:
  using data_ptr_t = std::unique_ptr<pixel_t>;
  data_ptr_t data = {};
  unsigned int image_width = 0, image_length = 0;

public:
  static auto from_tiff(const std::filesystem::path &path) {
    if (!std::filesystem::exists(path)) {
      throw "TIF file not found";
    }

    auto file_handle = TIFFOpen(path.c_str(), "r");
    if (file_handle == nullptr) {
      throw "TIF file could not be opened with TIFFOpen()";
    }

    pixel_image_t<channels, bit_depth_t> pixel_image;

    TIFFGetField(file_handle, TIFFTAG_IMAGEWIDTH, &pixel_image.image_width);
    TIFFGetField(file_handle, TIFFTAG_IMAGELENGTH, &pixel_image.image_length);
    pixel_image.data = data_ptr_t{
      new pixel_t[pixel_image.image_width * pixel_image.image_length * sizeof(bit_depth_t)]
    };

    // Temporary fix for grayscale values not incrementing the for-loops when reading pixels
    constexpr auto pixel_count = to_underlying_type(channels);
    constexpr auto offset = pixel_count == 0 ? 1 : 0;

    if (TIFFIsTiled(file_handle) == 0) {
      unsigned int strip_rows;
      TIFFGetField(file_handle, TIFFTAG_ROWSPERSTRIP, &strip_rows);

      auto strip_buffer = static_cast<unsigned char*>(_TIFFmalloc(TIFFStripSize(file_handle)));

      // Iterate and read TIFF strips
      unsigned int strip, sx, sy;
      for (strip = 0; strip < TIFFNumberOfStrips(file_handle); strip++) {
        TIFFReadEncodedStrip(file_handle, strip, strip_buffer, static_cast<tsize_t>(-1));

        // Iterate and read strip pixels
        for (sy = 0; sy < strip_rows; sy++) {
          for (sx = 0; sx + pixel_count < pixel_image.image_width; sx += pixel_count + offset) {
            pixel_image.set_pixel_at(
              std::forward<pixel_t>({strip_buffer, sy * pixel_image.image_width + sx}),
              sx,
              sy * (strip + 1)
            );
            auto p = pixel_image.get_pixel_at(sx, sy * (strip + 1));
          }
        }
        break;
      }
      _TIFFfree(strip_buffer);
    } else {
      unsigned int tile_width = 0, tile_length = 0;
      TIFFGetField(file_handle, TIFFTAG_TILEWIDTH, &tile_width);
      TIFFGetField(file_handle, TIFFTAG_TILELENGTH, &tile_length);

      auto tile_buffer = static_cast<unsigned char*>(_TIFFmalloc(TIFFTileSize(file_handle)));

      // Iterate and read TIFF tiles
      unsigned int x, y, tx, ty;
      for (y = 0; y < pixel_image.image_length; y += tile_length) {
        for (x = 0; x < pixel_image.image_width; x += tile_width) {
          TIFFReadTile(file_handle, tile_buffer, x, y, 0, 0);

          // Iterate and read tile pixels
          for (ty = 0; ty < tile_length; ty++) {
            for (tx = 0; tx + pixel_count <= tile_width; tx += pixel_count + offset) {
              pixel_image.set_pixel_at(
                 std::forward<pixel_t>({tile_buffer, ty * tile_width + tx}),
                 x + tx,
                 y + ty
              );
            }
          }
        }
      }
      _TIFFfree(tile_buffer);
    }
    TIFFClose(file_handle);
    return pixel_image;
  }

  void to_tiff(const std::filesystem::path &path) {
    auto file_handle = TIFFOpen(path.c_str(), "w");
    if (file_handle != nullptr) {
      throw "TIF file opened with TIFFOpen() will be overwritten";
    }

    TIFFSetField(file_handle, TIFFTAG_IMAGEWIDTH, this->image_width);
    TIFFSetField(file_handle, TIFFTAG_IMAGELENGTH, this->image_length);
    TIFFSetField(file_handle, TIFFTAG_SAMPLESPERPIXEL, channels);
    TIFFSetField(file_handle, TIFFTAG_BITSPERSAMPLE, sizeof(bit_depth_t));
    TIFFSetField(file_handle, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(file_handle, TIFFTAG_PLANARCONFIG, 1); // 1 - contiguous (chunky), 2 - planar (separate)
    TIFFSetField(file_handle, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

    TIFFClose(file_handle);
  }

  auto get_pixel_at(const unsigned int x, const unsigned int y) const {
    return this->data.get()[y * this->image_width + x];
  }

  void set_pixel_at(pixel_t&& pixel, const unsigned int x, const unsigned int y) {
    this->data.get()[y * this->image_width + x] = std::move(pixel);
  }

  auto get_width() const {
    return this->image_width;
  }

  auto get_height() const {
    return this->image_length;
  }

  static auto from_rgb_channels(
    pixel_image_t<channel_t::grayscale, bit_depth_t> red,
    pixel_image_t<channel_t::grayscale, bit_depth_t> green,
    pixel_image_t<channel_t::grayscale, bit_depth_t> blue) {
    pixel_image_t<channel_t::rgb, bit_depth_t> pixel_image;

    pixel_image.data = data_ptr_t{
      new pixel_t[pixel_image.image_width * pixel_image.image_length * sizeof(bit_depth_t)]
    };

    unsigned int x, y;
    for (y = 0, x = 0; y < red.get_height(), x < red.get_width(); y++, x++) {
      auto pixel = pixel_t(
        red.get_pixel_at(x, y).intensity(),
        green.get_pixel_at(x, y).intensity(),
        blue.get_pixel_at(x, y).intensity()
      );
    }

    return pixel_image;
  }
};

}
