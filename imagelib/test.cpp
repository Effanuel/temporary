#include "ia_tiff.h"

int main() {
  std::filesystem::create_directory("data_out");
  {
    // Combine 3 fluorescence images into one contiguous RGB image
    auto region_red = new ia::tiff_t("../data/Region_001_FOV_00041_Acridine_Or_Gray.tif");
    auto region_green = new ia::tiff_t("../data/Region_001_FOV_00041_DAPI_Gray.tif");
    auto region_blue = new ia::tiff_t("../data/Region_001_FOV_00041_FITC_Gray.tif");
    auto region_combined = new ia::tiff_t(region_red, region_green, region_blue);
    region_combined->write_to("data_out/Combined_RGB.tif");

    delete region_red, region_green, region_blue, region_combined;
  }
  {
    // Gamma correction tests
    auto gamma_1 = new ia::tiff_t("../data/Region_001_FOV_00041_Acridine_Or_Gray.tif");
    auto gamma_2 = new ia::tiff_t("../data/Region_001_FOV_00041_Acridine_Or_Gray.tif");

    gamma_1->gamma_correct(0.2);
    gamma_2->gamma_correct(2.0);

    gamma_1->write_to("./data_out/Gamma_a.tif");
    gamma_2->write_to("./data_out/Gamma_b.tif");

    delete gamma_1, gamma_2;
  }
  {
    // Piecewise linear intensity functions: Contrast stretching and thresholding
    auto contrast_test = new ia::tiff_t("../data/Region_001_FOV_00041_Acridine_Or_Gray.tif");
    auto threshold_test = new ia::tiff_t("../data/Region_001_FOV_00041_Acridine_Or_Gray.tif");

    contrast_test->curves(
      ia::curve_segment_t{ 0, 0, 47, 176 },
      ia::curve_segment_t{ 47, 176, 255, 255 }
    );
    threshold_test->curves(
      ia::curve_segment_t{ 0, 0, 95, 97 },
      ia::curve_segment_t{ 95, 224, 159, 223 },
      ia::curve_segment_t{ 159, 160, 255, 255 }
    );

    contrast_test->write_to("./data_out/Contrast.tif");
    threshold_test->write_to("./data_out/Threshold.tif");

    delete contrast_test, threshold_test;
  }
  {
    // Histogram normalization
    auto norm_test = new ia::tiff_t("../data/Region_001_FOV_00041_Acridine_Or_Gray.tif");
    norm_test->equalize();
    norm_test->write_to("./data_out/Normalization.tif");

    delete norm_test;
  }
}

