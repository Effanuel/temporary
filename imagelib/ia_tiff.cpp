#include "ia_tiff.h"

namespace ia {

tiff_t::tiff_t() { }

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
}

tiff_t::~tiff_t() {
  delete[] this->raw_data;
}

void tiff_t::write_to(const std::filesystem::path& p) {
  auto handle = TIFFOpen(p.c_str(), "w");

  TIFFSetField(handle, TIFFTAG_IMAGEWIDTH, this->image_width);
  TIFFSetField(handle, TIFFTAG_IMAGELENGTH, this->image_length);
  TIFFSetField(handle, TIFFTAG_SAMPLESPERPIXEL, this->channels_count);
  TIFFSetField(handle, TIFFTAG_BITSPERSAMPLE, this->channel_bits);
  TIFFSetField(handle, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(handle, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(handle, TIFFTAG_PHOTOMETRIC, to_underlying_type(this->photometric_representation));
  TIFFSetField(handle, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(handle, this->image_width * this->channels_count));

  auto scanline_size = TIFFScanlineSize(handle);
  for (unsigned int y = 0; y < this->image_length; y++) {
    if (TIFFWriteScanline(handle, &this->raw_data[(this->image_length - y - 1) * scanline_size], y, 0) < 0) {
      break;
    }
  }
  TIFFClose(handle);
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
    this->image_width * this->image_length * this->channels_count
  ];

  unsigned int strip_rows;
  TIFFGetField(handle, TIFFTAG_ROWSPERSTRIP, &strip_rows);

  auto strip_size = TIFFStripSize(handle);
  auto strip_number = TIFFNumberOfStrips(handle);
  // Handle last strip having less than TIFFNumberOfStrips() rows
  auto last_strip_rows = this->image_length - (this->image_length / strip_rows) * strip_rows;

  auto strip_buffer = static_cast<unsigned char*>(_TIFFmalloc(strip_size));

  unsigned int strip, sx, sy;
  for (strip = 0; strip < strip_number; strip++) {
    TIFFReadEncodedStrip(handle, strip, strip_buffer, static_cast<tsize_t>(-1));

    // Last strip is not guaranteed to have the same row count as other strips,
    // so before we begin to interpret the last strip, set the correct number of rows
    for (sy = 0; sy < (strip == strip_number - 1 && last_strip_rows > 0 ? last_strip_rows : strip_rows); sy++) {
      for (sx = 0; sx < this->image_width * this->channels_count; sx++) {
        this->raw_data[
          (strip * strip_rows + sy) * this->image_width * this->channels_count + sx
        ] = strip_buffer[sy * this->image_width * this->channels_count + sx];
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
    this->image_width * this->image_length * this->channels_count
  ];

  unsigned int tile_width;
  unsigned int tile_length;
  TIFFGetField(handle, TIFFTAG_TILEWIDTH, &tile_width);
  TIFFGetField(handle, TIFFTAG_TILELENGTH, &tile_length);

  auto tile_buffer = static_cast<unsigned char*>(_TIFFmalloc(TIFFTileSize(handle)));

  unsigned int ix, iy, tx, ty;
  for (iy = 0; iy < this->image_length; iy += tile_length) {
    for (ix = 0; ix < this->image_width * this->channels_count; ix += tile_width * this->channels_count) {
      TIFFReadTile(handle, tile_buffer, ix / this->channels_count, iy, 0, 0);

      for (ty = 0; ty < tile_length; ty++) {
        for (tx = 0; tx < tile_width * this->channels_count; tx++) {
          this->raw_data[
            (iy + ty) * this->image_width * this->channels_count + ix + tx
          ] = tile_buffer[ty * tile_width * this->channels_count + tx];
        }
      }
    }
  }
  _TIFFfree(tile_buffer);
}

}
