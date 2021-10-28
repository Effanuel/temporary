#include <deque>

// Some Linux distributions package the Eigen lib
// under .../include/eigen3/Eigen, including mine,
// but probably not yours
#if __has_include(<Eigen/Core>) && __has_include(<Eigen/LU>)
# include <Eigen/Core>
# include <Eigen/LU>
#else
# include <eigen3/Eigen/Core>
# include <eigen3/Eigen/LU>
#endif

class workspace_t {
protected:
  // A deque of layers, containing raw array mappings to image data
  std::deque<Eigen::Map<Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>> layers;
  Eigen::Vector3f world_coords{ 0.0f, 0.0f, 1.0f };

public:
  // Build the workspace ("world")
  template <typename... Args>
  workspace_t(Args... args);
  
  void rotate();
  void translate();
  void scale();
  void shear();
};
