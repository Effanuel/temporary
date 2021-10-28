#include "ia_tiff.h"

namespace ia {

tiff_t::tiff_t() {

}

/*
tiff_t::tiff_t(unsigned char* data) {
  this->raw_data = data;
}
*/

tiff_t::tiff_t(const std::filesystem::path &p) {
  if (!std::filesystem::exists(p)) {
    printf("TIF file not found\n");
  }

  auto handle = TIFFOpen(p.c_str(), "r");
  if (handle == nullptr) {
    printf("TIF file could not be opened with TIFFOpen()\n");
  }

  if (TIFFIsTiled(handle) == 0) {
    tiff_read_strips(handle);
  } else {
    tiff_read_tiles(handle);
  }

  TIFFClose(handle);

  /*
  this->matrix.setIdentity();
  this->matrix(0, 0) = 2.0f;
  this->matrix(1, 1) = 2.0f;

  this->eigen_raw_data = Eigen::Map<eigen_raw_data_t>{ this->raw_data, this->width(), this->length() };
  this->bbox = bounding_box_t{ 0, 0, this->width() - 1, this->length() - 1 };
  */
}

/*
void tiff_t::set_bounds(bounding_box_t new_bbox) {
  this->bbox = new_bbox;
}
*/

tiff_t::tiff_t(tiff_t* other) noexcept :
  image_width(other->image_width),
  image_length(other->image_length),
  channels_count(other->channels_count),
  channel_bits(other->channel_bits),
  photometric_representation(other->photometric_representation) {
  this->raw_data = new unsigned char[other->width() * other->length() * other->channels() * other->bit_depth()];
  std::memcpy(this->raw_data, other->raw_data, other->width() * other->length() * other->channels() * other->bit_depth());
}

tiff_t::tiff_t(tiff_t* red, tiff_t* green, tiff_t* blue) {
  this->raw_data = new unsigned char[ red->width() * 3 * red->length() ];
  this->channel_bits = red->channel_bits;
  this->image_width = red->image_width;
  this->image_length = red->image_length;
  this->channels_count = 3;
  this->photometric_representation = photometric_t::rgb;
  for (unsigned int x = 0, y = 0; y < red->width() * red->length(); x += 3, y++) {
    this->raw_data[x] = red->raw_data[y];
    this->raw_data[x + 1] = green->raw_data[y];
    this->raw_data[x + 2] = blue->raw_data[y];
  }
}

tiff_t::~tiff_t() {
  delete[] this->raw_data;
}

unsigned int tiff_t::width() const {
  return this->image_width;
}

unsigned int tiff_t::length() const {
  return this->image_length;
}

unsigned int tiff_t::channels() const {
  return this->channels_count;
}

unsigned int tiff_t::bit_depth() const {
  return this->channel_bits;
}

photometric_t tiff_t::photometric_repr() const {
  return this->photometric_representation;
}

unsigned char* tiff_t::data() const {
  return this->raw_data;
}

void tiff_t::write_to(const std::filesystem::path& p) {
  auto handle = TIFFOpen(p.c_str(), "w");

  TIFFSetField(handle, TIFFTAG_IMAGEWIDTH, this->width());
  TIFFSetField(handle, TIFFTAG_IMAGELENGTH, this->length());
  TIFFSetField(handle, TIFFTAG_SAMPLESPERPIXEL, this->channels());
  TIFFSetField(handle, TIFFTAG_BITSPERSAMPLE, this->bit_depth());
  TIFFSetField(handle, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(handle, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(handle, TIFFTAG_PHOTOMETRIC, to_underlying_type(this->photometric_repr()));
  TIFFSetField(handle, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(handle, this->width() * this->channels()));

  auto scanline_size = TIFFScanlineSize(handle);
  for (unsigned int y = 0; y < this->length(); y++) {
    if (TIFFWriteScanline(handle, &this->raw_data[(this->length() - y - 1) * scanline_size], y, 0) < 0) {
      break;
    }
  }
  TIFFClose(handle);
}

std::vector<tiff_t*> tiff_t::from_directories(const std::filesystem::path& p) {
  if (!std::filesystem::exists(p)) {
    throw "TIF file not found";
  }

  auto handle = TIFFOpen(p.c_str(), "rh");
  if (handle == nullptr) {
    throw "TIF file could not be opened with TIFFOpen()";
  }

  auto dirs = 0;
  std::vector<tiff_t*> images_vec;
  TIFFSetDirectory(handle, dirs);
  do {
    auto image = new tiff_t();
    if (TIFFIsTiled(handle) == 0) {
      image->tiff_read_strips(handle);
    } else {
      image->tiff_read_tiles(handle);
    }
    images_vec.push_back(image);

    dirs++;
  } while(TIFFReadDirectory(handle) == 1);

  TIFFClose(handle);
  return images_vec;
}

void tiff_t::tiff_read_strips(TIFF* handle) {
  TIFFGetField(handle, TIFFTAG_IMAGEWIDTH, &this->image_width);
  TIFFGetField(handle, TIFFTAG_IMAGELENGTH, &this->image_length);
  TIFFGetField(handle, TIFFTAG_SAMPLESPERPIXEL, &this->channels_count);
  TIFFGetField(handle, TIFFTAG_BITSPERSAMPLE, &this->channel_bits);
  unsigned int photometric_raw;
  TIFFGetField(handle, TIFFTAG_PHOTOMETRIC, &photometric_raw);
  this->photometric_representation = photometric_t{photometric_raw};
  this->raw_data = new unsigned char[
    this->width() * this->length() * this->channels()
  ];

  unsigned int strip_rows;
  TIFFGetField(handle, TIFFTAG_ROWSPERSTRIP, &strip_rows);

  auto strip_size = TIFFStripSize(handle);
  auto strip_number = TIFFNumberOfStrips(handle);
  // Handle last strip having less than TIFFNumberOfStrips() rows
  auto last_strip_rows = this->length() - (this->length() / strip_rows) * strip_rows;

  auto strip_buffer = static_cast<unsigned char*>(_TIFFmalloc(strip_size));

  unsigned int strip, sx, sy;
  for (strip = 0; strip < strip_number; strip++) {
    TIFFReadEncodedStrip(handle, strip, strip_buffer, static_cast<tsize_t>(-1));

    // Last strip is not guaranteed to have the same row count as other strips,
    // so before we begin to interpret the last strip, set the correct number of rows
    for (sy = 0; sy < (strip == strip_number - 1 && last_strip_rows > 0 ? last_strip_rows : strip_rows); sy++) {
      for (sx = 0; sx < this->width() * this->channels(); sx++) {
        this->raw_data[
          (strip * strip_rows + sy) * this->width() * this->channels() + sx
        ] = strip_buffer[sy * this->width() * this->channels() + sx];
      }
    }
  }

  _TIFFfree(strip_buffer);
}

void tiff_t::tiff_read_tiles(TIFF* handle) {
  TIFFGetField(handle, TIFFTAG_IMAGEWIDTH, &this->image_width);
  TIFFGetField(handle, TIFFTAG_IMAGELENGTH, &this->image_length);
  TIFFGetField(handle, TIFFTAG_SAMPLESPERPIXEL, &this->channels_count);
  TIFFGetField(handle, TIFFTAG_BITSPERSAMPLE, &this->channel_bits);
  this->raw_data = new unsigned char[
    this->width() * this->length() * this->channels()
  ];

  unsigned int tile_width;
  unsigned int tile_length;
  TIFFGetField(handle, TIFFTAG_TILEWIDTH, &tile_width);
  TIFFGetField(handle, TIFFTAG_TILELENGTH, &tile_length);

  auto tile_buffer = static_cast<unsigned char*>(_TIFFmalloc(TIFFTileSize(handle)));

  unsigned int ix, iy, tx, ty;
  for (iy = 0; iy < this->length(); iy += tile_length) {
    for (ix = 0; ix < this->width() * this->channels(); ix += tile_width * this->channels()) {
      TIFFReadTile(handle, tile_buffer, ix / this->channels(), iy, 0, 0);

      for (ty = 0; ty < tile_length; ty++) {
        for (tx = 0; tx < tile_width * this->channels(); tx++) {
          this->raw_data[
            (iy + ty) * this->width() * this->channels() + ix + tx
          ] = tile_buffer[ty * tile_width * this->channels() + tx];
        }
      }
    }
  }
  _TIFFfree(tile_buffer);
}

void tiff_t::negate() {
  auto map = std::unordered_map<unsigned char, unsigned char>{};

  for (unsigned int y = 0; y < this->length(); y++) {
    for (unsigned int x = 0; x < this->width(); x++) {
      auto offset = y * this->width() + x;

      // Check the LUT for this intensity first,
      // if's not in yet, calculate and add to LUT
      auto search = map.find(this->raw_data[offset]);
      if (search == map.end()) {
        auto transform = std::numeric_limits<unsigned char>::max() - this->raw_data[offset];
        map[this->raw_data[offset]] = transform;
        this->raw_data[offset] = transform;
      } else {
        this->raw_data[offset] = map[this->raw_data[offset]];
      }
    }
  }
}

void tiff_t::threshold(const unsigned int value) {
  this->threshold(value, value);
}

void tiff_t::threshold(const unsigned int lower, const unsigned int higher) {
  for (unsigned int y = 0; y < this->length(); y++) {
    for (unsigned int x = 0; x < this->width(); x++) {
      auto offset = y * this->width() + x;
      if (this->raw_data[offset] < lower) {
        this->raw_data[offset] = std::numeric_limits<unsigned char>::min();
      } else if (this->raw_data[offset] >= higher) {
        this->raw_data[offset] = std::numeric_limits<unsigned char>::max();
      }
    }
  }
}

void tiff_t::gamma_correct(const float gamma) {
  auto map = std::unordered_map<unsigned char, unsigned char>{};

  for (unsigned int y = 0; y < this->length(); y++) {
    for (unsigned int x = 0; x < this->width(); x++) {
      auto offset = y * this->width() + x;

      // Check the LUT for this intensity first,
      // if's not in yet, calculate and add to LUT
      auto search = map.find(this->raw_data[offset]);
      if (search == map.end()) {
        auto transform = std::round(
          std::pow(
            this->raw_data[offset] / static_cast<float>(std::numeric_limits<unsigned char>::max()),
            1.0 / gamma
          ) * static_cast<float>(std::numeric_limits<unsigned char>::max())
        );
        map[this->raw_data[offset]] = transform;
        this->raw_data[offset] = transform;
      } else {
        this->raw_data[offset] = map[this->raw_data[offset]];
      }
    }
  }
}

std::vector<unsigned int> tiff_t::get_histogram() {
  auto hist = std::vector<unsigned int>(std::numeric_limits<unsigned char>::max(), 0);
  for (unsigned int y = 0; y < this->length(); y++) {
    for (unsigned int x = 0; x < this->width(); x++) {
      auto offset = y * this->width() + x;
      hist[this->raw_data[offset]]++;
    }
  }
  return hist;
}

// Performs normalization of the image based on an algorithm
void tiff_t::equalize() {
  auto total = this->width() * this->length();
  auto hist = this->get_histogram();

  // Find first non-zero value and build the LUT
  unsigned int i = 0;
  auto first_zero = std::find(hist.begin(), hist.end(), 0);
  if (first_zero != hist.end()) {
    i = std::distance(hist.begin(), first_zero);
  }
  auto scale = (std::numeric_limits<unsigned char>::max() - 1.f) / (total - hist[i]);
  auto lut = std::vector<unsigned int>(std::numeric_limits<unsigned char>::max(), 0);
  i++;

  for (unsigned int sum = 0; i < hist.size(); ++i) {
    sum += hist[i];
    lut[i] = std::max(
      static_cast<unsigned char>(0),
      std::min(
        static_cast<unsigned char>(std::round(sum * scale)),
        std::numeric_limits<unsigned char>::max()
      )
    );
  }

  // Apply equalization
  for (unsigned int y = 0; y < this->length(); y++) {
    for (unsigned int x = 0; x < this->width(); x++) {
      auto offset = y * this->width() + x;
      raw_data[offset] = lut[raw_data[offset]];
    }
  }
}

}
