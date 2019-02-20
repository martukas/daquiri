#pragma once

#include "gtest_color_print.h"
#include "clever_hist.h"

#include <core/fitting/fittable_region.h>
#include <core/fitting/optimizers/abstract_optimizer.h>
#include <core/fitting/hypermet/Value.h>

#include <random>

struct ValueToVary
{
  ValueToVary() = default;
  ValueToVary(std::string var_name, DAQuiri::AbstractValue* var,
              double minimum, double maximum, double eps);

  DAQuiri::AbstractValue* variable;
  double min, max;
  double epsilon;
  std::uniform_real_distribution<double> distribution;
  std::string name;
  double goal;

  double max_delta{0};
  std::vector<double> deltas;

  std::string name_var() const;

  std::string declare() const;

  void vary(std::mt19937& rng);

  void record_delta();

  double get_delta() const;

  std::string print_delta();

  std::string summary() const;

  CleverHist deltas_hist() const;
};

class FunctionTest : public TestBase
{
 protected:
  std::vector<double> val_proxy;
  std::vector<double> val_val;
  std::vector<double> chi_sq_norm;
  std::vector<double> gradient;

  DAQuiri::WeightedData generate_data(
      const DAQuiri::FittableRegion* fittable, size_t bins) const;

  void visualize_data(const DAQuiri::WeightedData& data) const;

  void survey_grad(const DAQuiri::FittableRegion* fittable,
                   DAQuiri::AbstractValue* variable,
                   double step_size = 0.1, double xmin = -M_PI, double xmax = M_PI);

  double check_chi_sq(bool print) const;

  double check_gradients(bool print) const;

  void test_fit(size_t attempts,
                DAQuiri::AbstractOptimizer* optimizer,
                DAQuiri::FittableRegion* fittable,
                DAQuiri::AbstractValue* variable,
                double wrong_value,
                double epsilon);

  void test_fit_random(size_t attempts,
                       DAQuiri::AbstractOptimizer* optimizer,
                       DAQuiri::FittableRegion* fittable,
                       DAQuiri::AbstractValue* variable,
                       double wrong_min, double wrong_max,
                       double epsilon);

  void test_fit_random(size_t attempts,
                       DAQuiri::AbstractOptimizer* optimizer,
                       DAQuiri::FittableRegion* fittable,
                       std::vector<ValueToVary> vals,
                       bool verbose = false);
};
