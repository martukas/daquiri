#include "../../fitting/function_test.h"

#include <core/gamma/hypermet/step.h>
#include <core/fitting/parameter/positive_param.h>
#include <core/fitting/parameter/sine_bounded_param.h>

#include <core/util/UTF_extensions.h>

class FittableStep : public DAQuiri::DataModel
{
  std::uniform_real_distribution<double> x_dist {-M_PI_2, M_PI_2};

 public:
  DAQuiri::SineBoundedParam position;
  DAQuiri::PositiveParam amplitude;
  DAQuiri::SineBoundedParam width;

  DAQuiri::Step step;

  void update_indices() override
  {
    variable_count = 0;
    amplitude.update_index(variable_count);
    width.update_index(variable_count);
    position.update_index(variable_count);
    step.update_indices(variable_count);
  }

  Eigen::VectorXd variables() const override
  {
    Eigen::VectorXd ret;
    ret.setConstant(variable_count, 0.0);
    position.put(ret);
    amplitude.put(ret);
    width.put(ret);
    step.put(ret);
    return ret;
  }

  bool perturb(std::mt19937& rng) override
  {
    if (step.amplitude.valid_index())
      step.amplitude.x(x_dist(rng));
    if (width.valid_index())
      width.x(x_dist(rng));
    if (position.valid_index())
      position.x(x_dist(rng));
    if (amplitude.valid_index())
      amplitude.x(x_dist(rng));
    return true;
  }

  bool sane() const override
  {
    if (!step.sane(0.0, 0.0))
      return false;
    if (width.to_fit && width.at_extremum(0.0, 0.0))
      return false;
    if (position.to_fit && position.at_extremum(0.0, 0.0))
      return false;
    return true;
  }

  DAQuiri::PrecalcVals precalc(double chan) const
  {
    DAQuiri::PrecalcVals ret;
    ret.ampl = amplitude.val();
    ret.half_ampl = 0.5 * ret.ampl;
    ret.width = width.val();
    ret.spread = (chan - position.val()) / ret.width;

    ret.amp_grad = amplitude.grad();
    ret.width_grad = width.grad();
    ret.pos_grad = position.grad();

    ret.i_amp = amplitude.index();
    ret.i_width = width.index();
    ret.i_pos = position.index(); // should not matter?
    return ret;
  }

  DAQuiri::PrecalcVals precalc_at(double chan, const Eigen::VectorXd& fit) const
  {
    DAQuiri::PrecalcVals ret;
    ret.ampl = amplitude.val_from(fit);
    ret.half_ampl = 0.5 * ret.ampl;
    ret.width = width.val_from(fit);
    ret.spread = (chan - position.val_from(fit)) / ret.width;

    ret.amp_grad = amplitude.grad_from(fit);
    ret.width_grad = width.grad_from(fit);
    ret.pos_grad = position.grad_from(fit);

    ret.i_amp = amplitude.index();
    ret.i_width = width.index();
    ret.i_pos = position.index();
    return ret;
  }

  double eval(double chan) const override
  {
    return step.eval(precalc(chan));
  }

  double eval_at(double chan, const Eigen::VectorXd& fit) const override
  {
    return step.eval_at(precalc_at(chan, fit), fit);
  }

  double eval_grad_at(double chan, const Eigen::VectorXd& fit,
                      Eigen::VectorXd& grads) const override
  {
    return step.eval_grad_at(precalc_at(chan, fit), fit, grads);
  }

  void save_fit(const DAQuiri::FitResult& result) override
  {
    amplitude.get(result.variables);
    width.get(result.variables);
    position.get(result.variables);
    step.get(result.variables);

    if (!result.inv_hessian.innerSize() || !result.inv_hessian.outerSize())
      return;

    Eigen::VectorXd diags = result.inv_hessian.diagonal();
    diags *= degrees_of_freedom();

    auto chi = chi_sq();
    amplitude.get_uncert(diags, chi);
    width.get_uncert(diags, chi);
    position.get_uncert(diags, chi);
    step.get_uncerts(diags, chi);
  }

  std::string to_string(std::string prepend = "") const override
  {
    Eigen::VectorXd grads;
    auto x = variables();
    chi_sq_gradient(x, grads);
    std::stringstream ss;

    ss << "parent_pos   = " << position.to_string() << "\n";
    ss << prepend
       << "parent_amp   = " << amplitude.to_string() << "\n";
    ss << prepend
       << "parent_width = " << width.to_string() << "\n";
    ss << prepend
       << "step = " << step.to_string() << "\n";
    ss << prepend
       << "  chi" + UTF_superscript(2) + "=" + std::to_string(chi_sq())
       << "  grads=" << grads.transpose() << "\n";
    return ss.str();
  }
};

class Step : public FunctionTest
{
 protected:
  FittableStep fs;
  size_t region_size{100};
  size_t random_samples{100};

  void SetUp() override
  {
    print_outside_tolerance = true;

    //optimizer.verbosity = 5;
    optimizer.maximum_iterations = 150;
    optimizer.gradient_selection =
        DAQuiri::OptlibOptimizer::GradientSelection::AnalyticalAlways;
    optimizer.use_epsilon_check = false;
    optimizer.min_g_norm = 1e-7;
    optimizer.perform_sanity_checks = false;
    optimizer.maximum_perturbations = 0;

//    fs.amplitude.slope_ = 1e-5;
//    fs.amplitude.bound(0, 50000);
    fs.amplitude.val(40000);
    fs.amplitude.update_index(fs.variable_count);

//    fs.width.slope_ = 1e-4;
    fs.width.bound(0.8, 5.0);
    fs.width.val(3.2);
    fs.width.update_index(fs.variable_count);

//    fs.position.slope_ = 1e-3;
    fs.position.bound(44, 68);
    fs.position.val(51);
    fs.position.update_index(fs.variable_count);

//    fs.step.amplitude.slope_ = 1e-3;
    fs.step.amplitude.bound(1e-6, 0.05);
    fs.step.amplitude.val(0.0005);
  }
};

TEST_F(Step, CheckSetup)
{
  MESSAGE() << "Gaussian amp: " << fs.amplitude.to_string() << "\n";
  MESSAGE() << "Gaussian pos: " << fs.position.to_string() << "\n";
  MESSAGE() << "Gaussian width: " << fs.width.to_string() << "\n";
  MESSAGE() << "Step: " << fs.step.to_string() << "\n";
}

TEST_F(Step, Visualize)
{
  auto data = generate_data(&fs, region_size);
  visualize_data(data);
}

TEST_F(Step, WithinBounds)
{
  auto data = generate_data(&fs, region_size);
  EXPECT_NEAR(data.count_min(), 0.0, 1e-13);
  EXPECT_NEAR(data.count_max(), 20.0, 1e-13);
}

TEST_F(Step, LeftOriented)
{
  fs.step.side = DAQuiri::Side::left;
  auto data = generate_data(&fs, region_size);
  EXPECT_NEAR(data.count.front(), 20.0, 1e-13);
  EXPECT_NEAR(data.count.back(), 0.0, 1e-13);
}

TEST_F(Step, RightOriented)
{
  fs.step.side = DAQuiri::Side::right;
  auto data = generate_data(&fs, region_size);
  EXPECT_NEAR(data.count.front(), 0.0, 1e-13);
  EXPECT_NEAR(data.count.back(), 20.0, 1e-13);
}

TEST_F(Step, UpdateIndexInvalidThrows)
{
  int32_t i;

  i = -1;
  EXPECT_ANY_THROW(fs.step.update_indices(i));

  i = -42;
  EXPECT_ANY_THROW(fs.step.update_indices(i));
}

TEST_F(Step, UpdateIndex)
{
  int32_t i;

  i = 0;
  fs.step.update_indices(i);
  EXPECT_EQ(fs.step.amplitude.index(), 0);
  EXPECT_EQ(i, 1);

  fs.step.update_indices(i);
  EXPECT_EQ(fs.step.amplitude.index(), 1);
  EXPECT_EQ(i, 2);

  i = 42;
  fs.step.update_indices(i);
  EXPECT_EQ(fs.step.amplitude.index(), 42);
  EXPECT_EQ(i, 43);
}

TEST_F(Step, UpdateIndexInvalidates)
{
  int32_t i;

  i = 0;
  fs.step.update_indices(i);
  EXPECT_EQ(fs.step.amplitude.index(), 0);
  EXPECT_EQ(i, 1);

  fs.step.amplitude.to_fit = false;
  fs.step.update_indices(i);
  EXPECT_EQ(fs.step.amplitude.index(), -1);
  EXPECT_EQ(i, 1);

  fs.step.update_indices(i);
  EXPECT_EQ(fs.step.amplitude.index(), -1);
  EXPECT_EQ(i, 1);
}

TEST_F(Step, UpdateIndexDisabled)
{
  fs.step.enabled = false;
  int32_t i;

  i = 0;
  fs.step.update_indices(i);
  EXPECT_EQ(fs.step.amplitude.index(), -1);
  EXPECT_EQ(i, 0);

  fs.step.update_indices(i);
  EXPECT_EQ(fs.step.amplitude.index(), -1);
  EXPECT_EQ(i, 0);

  // \todo test resetting of indices
}

TEST_F(Step, Put)
{
  Eigen::VectorXd fit;
  fit.setConstant(1, 0.0);

  fs.step.put(fit);
  EXPECT_EQ(fit[0], 0.0);
  EXPECT_NE(fit[0], fs.step.amplitude.x());

  int32_t i{0};
  fs.step.update_indices(i);
  EXPECT_EQ(fs.step.amplitude.index(), 0);

  fs.step.put(fit);
  EXPECT_NE(fit[0], 0.0);
  EXPECT_EQ(fit[0], fs.step.amplitude.x());
}

TEST_F(Step, Get)
{
  Eigen::VectorXd fit;
  fit.setConstant(1, 0.005);

  fs.step.get(fit);
  EXPECT_NEAR(fs.step.amplitude.val(), 0.0005, 1e-15);

  int32_t i{0};
  fs.step.update_indices(i);
  EXPECT_DOUBLE_EQ(fs.step.amplitude.index(), 0.0);

  fs.step.get(fit);
  EXPECT_NE(fs.step.amplitude.val(), 0.0005);
  EXPECT_DOUBLE_EQ(fs.step.amplitude.val(), fs.step.amplitude.val_at(0.005));
}

TEST_F(Step, EvalAt)
{
  auto pre = fs.precalc(20);

  auto goal = fs.step.eval(pre);

  int32_t i{0};
  fs.step.update_indices(i);

  Eigen::VectorXd fit;
  fit.setConstant(1, 0.0);
  fs.step.put(fit);

  fs.step.amplitude.val(0.000001);

  EXPECT_NE(fs.step.eval(pre), goal);
  EXPECT_EQ(fs.step.eval_at(pre, fit), goal);
}

TEST_F(Step, EvalGrad)
{
  auto pre = fs.precalc(10);

  fs.step.update_indices(fs.variable_count);

  Eigen::VectorXd grad;
  grad.setConstant(fs.variable_count, 0.0);

  auto result = fs.step.eval_grad(pre, grad);

  EXPECT_EQ(result, fs.step.eval(pre));
  EXPECT_NE(grad[0], 0.0);
  EXPECT_NE(grad[1], 0.0);
  EXPECT_EQ(grad[2], 0.0); // pos gradient should be unaffected?
  EXPECT_NE(grad[3], 0.0);

  // \todo confirm that gradient is meaningful?
}

TEST_F(Step, EvalGradAt)
{
  auto pre = fs.precalc(10);

  int32_t i{3};
  fs.step.update_indices(i);

  Eigen::VectorXd grad_goal;
  grad_goal.setConstant(i, 0.0);
  fs.step.eval_grad(pre, grad_goal);

  Eigen::VectorXd fit, grad;
  fit.setConstant(i, 0.0);
  grad.setConstant(i, 0.0);

  fs.step.put(fit);
  fs.step.amplitude.val(0.000001);

  auto result = fs.step.eval_grad_at(pre, fit, grad);

  EXPECT_EQ(result, fs.step.eval_at(pre, fit));
  EXPECT_EQ(grad[0], grad_goal[0]);
  EXPECT_EQ(grad[1], grad_goal[1]);
  EXPECT_EQ(grad[2], grad_goal[2]);
  EXPECT_EQ(grad[3], grad_goal[3]);
}

TEST_F(Step, SurveyGradients)
{
  fs.data = generate_data(&fs, region_size);
  fs.update_indices();

  double goal_val = fs.step.amplitude.val();
  survey_grad(&fs, &fs.step.amplitude, 0.001);
  EXPECT_NEAR(check_chi_sq(false), goal_val, 1e-6);
  EXPECT_NEAR(check_gradients(false), goal_val, 1e-6);
//  survey_grad(&fs, &fs.step.amplitude, 1000, -1e5, 1e5);
  check_gradients(true);
  check_gradient_deltas(true);

  goal_val = fs.width.val();
//  survey_grad(&fs, &fs.width, 0.001);
//  EXPECT_NEAR(check_chi_sq(false), goal_val, 1e-2);
//  EXPECT_NEAR(check_gradients(false), goal_val, 1e-2);
//  survey_grad(&fs, &fs.width, 10, -1e5, 1e5);
//  check_gradients(true);
//  check_gradient_deltas(true);

// \todo of course this doesn't work!!! There is no dependent gradient implemented
//  goal_val = fs.position.val();
//  survey_grad(&fs, &fs.position, 0.0001, -800, -700);
//  EXPECT_NEAR(check_chi_sq(false), goal_val, 0.01);
//  EXPECT_NEAR(check_gradients(false), goal_val, 0.01);
//  survey_grad(&fs, &fs.position, 0.05);
//  check_gradients(true);
//  check_gradient_deltas(true);

  goal_val = fs.amplitude.val();
//  survey_grad(&fs, &fs.amplitude, 0.1, std::sqrt(30000), std::sqrt(40000));
//  EXPECT_NEAR(check_chi_sq(false), goal_val, 50);
//  EXPECT_NEAR(check_gradients(false), goal_val, 50);
//  survey_grad(&fs, &fs.amplitude, 0.5, std::sqrt(30000), std::sqrt(40000));
//  check_gradients(true);
//  check_gradient_deltas(true);
}

TEST_F(Step, FitAmplitude)
{
  fs.data = generate_data(&fs, region_size);
  fs.width.to_fit = false;
  fs.position.to_fit = false;
  fs.amplitude.to_fit = false;
  fs.update_indices();

  SetUp();
  test_fit_random(random_samples, &fs,
                  {"amplitude", &fs.step.amplitude,
                   fs.step.amplitude.min(),
                   fs.step.amplitude.max(), 1e-8});
  EXPECT_EQ(unconverged, 0u);
  EXPECT_EQ(not_sane, 0u);
  EXPECT_EQ(converged_finite, 0u);
  EXPECT_EQ(converged_perturbed, 0u);
  EXPECT_LE(max_iterations_to_converge, 13u);
  EXPECT_LE(max_perturbations_to_converge, 0u);
}

TEST_F(Step, FitParentWidth)
{
  fs.data = generate_data(&fs, region_size);
  fs.position.to_fit = false;
  fs.amplitude.to_fit = false;
  fs.step.amplitude.to_fit = false;
  fs.update_indices();

  SetUp();
  test_fit_random(random_samples, &fs,
                  {"parent_width", &fs.width,
                   fs.width.min(), fs.width.max(), 1e-8});
  EXPECT_EQ(unconverged, 0u);
  EXPECT_EQ(not_sane, 0u);
  EXPECT_EQ(converged_finite, 0u);
  EXPECT_EQ(converged_perturbed, 0u);
  EXPECT_LE(max_iterations_to_converge, 9u);
  EXPECT_LE(max_perturbations_to_converge, 0u);
}

TEST_F(Step, FitParentAmplitude)
{
  fs.data = generate_data(&fs, region_size);
  fs.width.to_fit = false;
  fs.position.to_fit = false;
  fs.step.amplitude.to_fit = false;
  fs.update_indices();

//  print_outside_tolerance = true;

//  verbose = true;
//  optimizer.verbosity = 5;

  SetUp();

  test_fit_random(random_samples, &fs,
                  {"parent_amplitude",
                   &fs.amplitude,
//                   fs.amplitude.min(), fs.amplitude.max(), 1e-2});
                   30000, 50000, 1e-3});
  EXPECT_EQ(unconverged, 0u);
  EXPECT_EQ(not_sane, 0u);
  EXPECT_EQ(converged_finite, 0u);
  EXPECT_LE(converged_perturbed, 0u);
  EXPECT_LE(max_iterations_to_converge, 6u);
  EXPECT_LE(max_perturbations_to_converge, 0u);
}

// \todo parent position should play a role?

// the 2 amplitudes are degenerate. can't deconvolute without peak
// so we must test their fitting separately in these tests
TEST_F(Step, FitTwoA)
{
  fs.data = generate_data(&fs, region_size);
  fs.position.to_fit = false;
  fs.amplitude.to_fit = false;
  fs.update_indices();

//  optimizer.gradient_selection =
//      DAQuiri::OptlibOptimizer::GradientSelection::FiniteAlways;

  std::vector<ValueToVary> vals;
  vals.push_back({"amplitude", &fs.step.amplitude,
                  fs.step.amplitude.min(),
                  fs.step.amplitude.max(), 1e-10});
  vals.push_back({"parent_width", &fs.width,
                  fs.width.min(), fs.width.max(), 1e-7});
  test_fit_random(random_samples, &fs, vals);

  EXPECT_EQ(unconverged, 0u);
  EXPECT_EQ(not_sane, 0u);
  EXPECT_EQ(converged_finite, 0u);
  EXPECT_EQ(converged_perturbed, 0u);
  EXPECT_LE(max_iterations_to_converge, 28u);
  EXPECT_LE(max_perturbations_to_converge, 0u);
}

TEST_F(Step, FitTwoB)
{
  fs.data = generate_data(&fs, region_size);
  fs.position.to_fit = false;
  fs.step.amplitude.to_fit = false;
  fs.update_indices();

//  print_outside_tolerance = true;
//  optimizer.verbosity = 5;

//  optimizer.min_g_norm = 1e-10;

  std::vector<ValueToVary> vals;
  vals.push_back({"parent_width", &fs.width,
                  fs.width.min(), fs.width.max(), 1e-8});
  vals.push_back({"parent_amplitude", &fs.amplitude,
//                  fs.amplitude.min(), fs.amplitude.max(), 1e-2});
                  30000, 50000, 1e-3});
  test_fit_random(random_samples, &fs, vals);

  EXPECT_EQ(unconverged, 0u);
  EXPECT_EQ(not_sane, 0u);
  EXPECT_EQ(converged_finite, 0u);
  EXPECT_LE(converged_perturbed, 0u);
  EXPECT_LE(max_iterations_to_converge, 19u);
  EXPECT_LE(max_perturbations_to_converge, 0u);
}