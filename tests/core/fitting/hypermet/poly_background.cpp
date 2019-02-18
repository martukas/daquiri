#include "../function_test.h"

#include <core/util/visualize_vector.h>

#include <core/fitting/hypermet/PolyBackground.h>

#include <core/fitting/optimizers/dlib_adapter.h>

class FittableBackground : public DAQuiri::FittableRegion
{
 public:
  DAQuiri::PolyBackground background;

  void update_indices() override
  {
    variable_count = 0;
    background.update_indices(variable_count);
  }

  Eigen::VectorXd variables() const override
  {
    Eigen::VectorXd ret;
    ret.setConstant(variable_count, 0.0);
    background.put(ret);
    return ret;
  }

  double eval(double chan) const override
  {
    return background.eval(chan);
  }

  double eval_at(double chan, const Eigen::VectorXd& fit) const override
  {
    return background.eval_at(chan, fit);
  }

  double eval_grad_at(double chan, const Eigen::VectorXd& fit,
                      Eigen::VectorXd& grads) const override
  {
    return background.eval_grad_at(chan, fit, grads);
  }

  void save_fit(const DAQuiri::FitResult& result) override
  {
    background.get(result.variables);

    // \todo uncerts
  }
};

class PolyBackground : public FunctionTest
{
 protected:

  FittableBackground fb;

  virtual void SetUp()
  {
    fb.background.x_offset = 0;
//    fb.background.base.bound(0, 7792);
    fb.background.base.val(70);
//    fb.background.slope.bound(0, 198);
    fb.background.slope.val(3);
//    fb.background.curve.bound(0, 15);
    fb.background.curve.val(5);

    // \todo make these more permissive
  }

  void auto_bound()
  {
    auto lb = DAQuiri::SUM4Edge(fb.data.left(3));
    auto rb = DAQuiri::SUM4Edge(fb.data.right(3));
    fb.background = DAQuiri::PolyBackground(fb.data, lb, rb);

    fb.background.base.val(70);
    fb.background.slope.val(3);
    fb.background.curve.val(5);
  }

};

TEST_F(PolyBackground, CheckSetup)
{
  MESSAGE() << "PolyBackground: " << fb.background.to_string() << "\n";
}

TEST_F(PolyBackground, Visualize)
{
  auto data = generate_data(&fb, 40);
  visualize_data(data);
}

TEST_F(PolyBackground, WithinBounds)
{
  auto data = generate_data(&fb, 40);
  EXPECT_NEAR(data.count_min, 70, 1e-10);
  EXPECT_NEAR(data.count_max, 70 + 3 * 39 + 5 * square(39), 1e-10);
}

TEST_F(PolyBackground, UpdateIndexInvalidThrows)
{
  int32_t i;

  i = -1;
  EXPECT_ANY_THROW(fb.background.update_indices(i));

  i = -42;
  EXPECT_ANY_THROW(fb.background.update_indices(i));
}

TEST_F(PolyBackground, UpdateIndex)
{
  int32_t i;

  i = 0;
  fb.background.update_indices(i);
  EXPECT_EQ(fb.background.base.index(), 0);
  EXPECT_EQ(fb.background.slope.index(), 1);
  EXPECT_EQ(fb.background.curve.index(), 2);
  EXPECT_EQ(i, 3);

  fb.background.update_indices(i);
  EXPECT_EQ(fb.background.base.index(), 3);
  EXPECT_EQ(fb.background.slope.index(), 4);
  EXPECT_EQ(fb.background.curve.index(), 5);
  EXPECT_EQ(i, 6);

  i = 42;
  fb.background.update_indices(i);
  EXPECT_EQ(fb.background.base.index(), 42);
  EXPECT_EQ(fb.background.slope.index(), 43);
  EXPECT_EQ(fb.background.curve.index(), 44);
  EXPECT_EQ(i, 45);
}

TEST_F(PolyBackground, UpdateIndexInvalidates)
{
  int32_t i;

  i = 0;
  fb.background.update_indices(i);
  EXPECT_EQ(fb.background.base.index(), 0);
  EXPECT_EQ(fb.background.slope.index(), 1);
  EXPECT_EQ(fb.background.curve.index(), 2);
  EXPECT_EQ(i, 3);

  fb.background.base.to_fit = false;
  fb.background.update_indices(i);
  EXPECT_EQ(fb.background.base.index(), -1);
  EXPECT_EQ(fb.background.slope.index(), 3);
  EXPECT_EQ(fb.background.curve.index(), 4);
  EXPECT_EQ(i, 5);

  fb.background.base.to_fit = true;
  fb.background.slope.to_fit = false;
  fb.background.update_indices(i);
  EXPECT_EQ(fb.background.base.index(), 5);
  EXPECT_EQ(fb.background.slope.index(), -1);
  EXPECT_EQ(fb.background.curve.index(), 6);
  EXPECT_EQ(i, 7);

  fb.background.base.to_fit = true;
  fb.background.slope.to_fit = true;
  fb.background.curve.to_fit = false;
  fb.background.update_indices(i);
  EXPECT_EQ(fb.background.base.index(), 7);
  EXPECT_EQ(fb.background.slope.index(), 8);
  EXPECT_EQ(fb.background.curve.index(), -1);
  EXPECT_EQ(i, 9);

  fb.background.base.to_fit = false;
  fb.background.slope.to_fit = false;
  fb.background.curve.to_fit = false;
  fb.background.update_indices(i);
  EXPECT_EQ(fb.background.base.index(), -1);
  EXPECT_EQ(fb.background.slope.index(), -1);
  EXPECT_EQ(fb.background.curve.index(), -1);
  EXPECT_EQ(i, 9);
}

TEST_F(PolyBackground, UpdateIndexDisabled)
{
  int32_t i = 0;

  fb.background.slope_enabled = false;
  fb.background.update_indices(i);
  EXPECT_EQ(fb.background.base.index(), 0);
  EXPECT_EQ(fb.background.slope.index(), -1);
  EXPECT_EQ(fb.background.curve.index(), 1);
  EXPECT_EQ(i, 2);

  fb.background.slope_enabled = true;
  fb.background.curve_enabled = false;
  fb.background.update_indices(i);
  EXPECT_EQ(fb.background.base.index(), 2);
  EXPECT_EQ(fb.background.slope.index(), 3);
  EXPECT_EQ(fb.background.curve.index(), -1);
  EXPECT_EQ(i, 4);

  fb.background.slope_enabled = false;
  fb.background.curve_enabled = false;
  fb.background.update_indices(i);
  EXPECT_EQ(fb.background.base.index(), 4);
  EXPECT_EQ(fb.background.slope.index(), -1);
  EXPECT_EQ(fb.background.curve.index(), -1);
  EXPECT_EQ(i, 5);
}

TEST_F(PolyBackground, Put)
{
  Eigen::VectorXd fit;
  fit.setConstant(3, 0.0);

  fb.background.put(fit);
  EXPECT_EQ(fit[0], 0.0);
  EXPECT_NE(fit[0], fb.background.base.x());
  EXPECT_EQ(fit[1], 0.0);
  EXPECT_NE(fit[1], fb.background.slope.x());
  EXPECT_EQ(fit[2], 0.0);
  EXPECT_NE(fit[2], fb.background.curve.x());

  fb.background.update_indices(fb.variable_count);
  fb.background.put(fit);
  EXPECT_NE(fit[0], 0.0);
  EXPECT_EQ(fit[0], fb.background.base.x());
  EXPECT_NE(fit[1], 0.0);
  EXPECT_EQ(fit[1], fb.background.slope.x());
  EXPECT_NE(fit[2], 0.0);
  EXPECT_EQ(fit[2], fb.background.curve.x());
}

TEST_F(PolyBackground, Get)
{
  Eigen::VectorXd fit;
  fit.setConstant(3, 0.0);
  fit[0] = 10;
  fit[1] = 0.03;
  fit[2] = 0.05;

  fb.background.get(fit);
  EXPECT_NEAR(fb.background.base.val(), 70, 0.00001);
  EXPECT_NEAR(fb.background.slope.val(), 3, 0.00001);
  EXPECT_NEAR(fb.background.curve.val(), 5, 0.00001);
  EXPECT_NE(fb.background.base.val(), fb.background.base.val_at(10));
  EXPECT_NE(fb.background.slope.val(), fb.background.slope.val_at(0.03));
  EXPECT_NE(fb.background.curve.val(), fb.background.curve.val_at(0.05));

  fb.background.update_indices(fb.variable_count);
  fb.background.get(fit);
  EXPECT_NE(fb.background.base.val(), 70);
  EXPECT_EQ(fb.background.base.val(), fb.background.base.val_at(10));
  EXPECT_NE(fb.background.slope.val(), 3);
  EXPECT_EQ(fb.background.slope.val(), fb.background.slope.val_at(0.03));
  EXPECT_NE(fb.background.curve.val(), 5);
  EXPECT_EQ(fb.background.curve.val(), fb.background.curve.val_at(0.05));
}

TEST_F(PolyBackground, EvalAt)
{
  auto goal = fb.background.eval(20);

  fb.background.update_indices(fb.variable_count);

  Eigen::VectorXd fit;
  fit.setConstant(fb.variable_count, 0.0);
  fb.background.put(fit);

  fb.background.base.val(10);
  fb.background.slope.val(0.000001);
  fb.background.curve.val(0.000001);

  EXPECT_NE(fb.background.eval(20), goal);
  EXPECT_EQ(fb.background.eval_at(20, fit), goal);
}

TEST_F(PolyBackground, EvalGrad)
{
  fb.background.update_indices(fb.variable_count);

  Eigen::VectorXd grad;
  grad.setConstant(fb.variable_count, 0.0);

  auto result = fb.background.eval_grad(10, grad);

  EXPECT_EQ(result, fb.background.eval(10));
  EXPECT_NE(grad[0], 0.0);
  EXPECT_NE(grad[1], 0.0);
  EXPECT_NE(grad[2], 0.0);

  // \todo confirm that gradient is meaningful?
}

TEST_F(PolyBackground, EvalGradAt)
{
  fb.background.update_indices(fb.variable_count);

  Eigen::VectorXd grad_goal;
  grad_goal.setConstant(fb.variable_count, 0.0);
  fb.background.eval_grad(10, grad_goal);

  Eigen::VectorXd fit, grad;
  fit.setConstant(fb.variable_count, 0.0);
  grad.setConstant(fb.variable_count, 0.0);

  fb.background.put(fit);
  fb.background.base.val(0.000001);
  fb.background.slope.val(0.000001);
  fb.background.curve.val(0.000001);

  auto result = fb.background.eval_grad_at(10, fit, grad);

  EXPECT_EQ(result, fb.background.eval_at(10, fit));
  EXPECT_EQ(grad[0], grad_goal[0]);
  EXPECT_EQ(grad[1], grad_goal[1]);
  EXPECT_EQ(grad[2], grad_goal[2]);
}

TEST_F(PolyBackground, GradBaseOnly)
{
  double goal_val = fb.background.base.val();
  fb.data = generate_data(&fb, 40);

  fb.update_indices();
  survey_grad(&fb, &fb.background.base, 0.05, std::sqrt(50.0), std::sqrt(100.0));
//  survey_grad(&fb, &fb.background.base, 0.001);
  EXPECT_NEAR(check_chi_sq(false), goal_val, 0.5);
  EXPECT_NEAR(check_gradients(false), goal_val, 0.1);
}

TEST_F(PolyBackground, FitBaseOnly)
{
  fb.data = generate_data(&fb, 40);
  auto_bound();

  fb.background.slope.to_fit = false;
  fb.background.curve.to_fit = false;
  fb.update_indices();
  MESSAGE() << "Start:\n" << fb.background.to_string();

  DAQuiri::DLibOptimizer optimizer;
  test_fit(5, &optimizer, &fb, &fb.background.base, 100, 0.1);
  auto_bound();
  test_fit_random(20, &optimizer, &fb, &fb.background.base, 50, 7792, 0.1);

  MESSAGE() << "Result:\n" << fb.background.to_string();
}

TEST_F(PolyBackground, FitBase)
{
  fb.data = generate_data(&fb, 40);
  auto_bound();

  fb.update_indices();
  MESSAGE() << "Start:\n" << fb.background.to_string();

  DAQuiri::DLibOptimizer optimizer;
  test_fit(5, &optimizer, &fb, &fb.background.base, 100, 0.5);
  auto_bound();
  test_fit_random(20, &optimizer, &fb, &fb.background.base, 50, 7792, 0.5);

  MESSAGE() << "Result:\n" << fb.background.to_string();
}

TEST_F(PolyBackground, GradSlope)
{
  double goal_val = fb.background.slope.val();
  fb.data = generate_data(&fb, 40);
  fb.update_indices();
  survey_grad(&fb, &fb.background.slope, 0.001, -460, 460);
  EXPECT_NEAR(check_chi_sq(false), goal_val, 0.001);
  EXPECT_NEAR(check_gradients(false), goal_val, 0.001);
}

TEST_F(PolyBackground, FitSlopeOnly)
{
  fb.data = generate_data(&fb, 40);
  auto_bound();

  fb.background.base.to_fit = false;
  fb.background.curve.to_fit = false;
  fb.update_indices();
  MESSAGE() << "Start:\n" << fb.background.to_string();

  DAQuiri::DLibOptimizer optimizer;
  test_fit(5, &optimizer, &fb, &fb.background.slope, 10, 1e-3);
  auto_bound();
  test_fit_random(20, &optimizer, &fb, &fb.background.slope, -460, 460, 1e-3);

  MESSAGE() << "Result:\n" << fb.background.to_string();
}

TEST_F(PolyBackground, FitSlope)
{
  fb.data = generate_data(&fb, 40);
  auto_bound();

  fb.update_indices();
  MESSAGE() << "Start:\n" << fb.background.to_string();

  DAQuiri::DLibOptimizer optimizer;
  test_fit(5, &optimizer, &fb, &fb.background.slope, 10, 0.02);
  auto_bound();
  test_fit_random(20, &optimizer, &fb, &fb.background.slope, -460, 460, 0.02);

  MESSAGE() << "Result:\n" << fb.background.to_string();
}

TEST_F(PolyBackground, GradCurve)
{
  double goal_val = fb.background.curve.val();
  fb.data = generate_data(&fb, 40);

  MESSAGE() << "PolyBackground: " << fb.background.to_string();

  fb.background.base.to_fit = false;
  fb.background.slope.to_fit = false;
  fb.update_indices();
  survey_grad(&fb, &fb.background.curve, 0.0001, -10, 10);
  EXPECT_NEAR(check_chi_sq(false), goal_val, 0.002);
  EXPECT_NEAR(check_gradients(false), goal_val, 0.001);
}

TEST_F(PolyBackground, FitCurveOnly)
{
  fb.data = generate_data(&fb, 40);
  auto_bound();

  fb.background.base.to_fit = false;
  fb.background.slope.to_fit = false;
  fb.update_indices();
  MESSAGE() << "Start:\n" << fb.background.to_string();

  DAQuiri::DLibOptimizer optimizer;
  test_fit(5, &optimizer, &fb, &fb.background.curve, 0, 1e-4);
  auto_bound();
  test_fit_random(20, &optimizer, &fb, &fb.background.curve, -10, 10, 1e-3);

  MESSAGE() << "Result:\n" << fb.background.to_string();
}

TEST_F(PolyBackground, FitCurve)
{
  fb.data = generate_data(&fb, 40);
  auto_bound();

  fb.update_indices();
  MESSAGE() << "Start:\n" << fb.background.to_string();

  DAQuiri::DLibOptimizer optimizer;
  test_fit(5, &optimizer, &fb, &fb.background.curve, 0, 1e-3);
  auto_bound();
  test_fit_random(20, &optimizer, &fb, &fb.background.curve, -10, 10, 1e-3);

  MESSAGE() << "Result:\n" << fb.background.to_string();
}

TEST_F(PolyBackground, FitAllThree)
{
  double goal_base = fb.background.base.val();
  double goal_slope = fb.background.slope.val();
  double goal_curve = fb.background.curve.val();

  fb.data = generate_data(&fb, 40);
  auto_bound();

  fb.update_indices();

  std::mt19937 random_generator;
  random_generator.seed(std::random_device()());
  std::uniform_real_distribution<double> base_dist(50, 7792);
  std::uniform_real_distribution<double> slope_dist(-460, 460);
  std::uniform_real_distribution<double> curve_dist(-10, 10);

  DAQuiri::DLibOptimizer optimizer;

  for (size_t i = 0; i < 50; ++i)
  {
    fb.background.base.val(base_dist(random_generator));
    fb.background.slope.val(slope_dist(random_generator));
    fb.background.curve.val(curve_dist(random_generator));
    MESSAGE() << "Attempt[" << i << "]\n" << fb.background.to_string();
    fb.save_fit(optimizer.minimize(&fb));
    MESSAGE() << "Result:               \n"
              << fb.background.to_string("                      ");
    MESSAGE() << "           base delta="
              << (goal_base - fb.background.base.val()) << "\n";
    MESSAGE() << "          slope delta="
              << (goal_slope - fb.background.slope.val()) << "\n";
    MESSAGE() << "          curve delta="
              << (goal_curve - fb.background.curve.val()) << "\n";

    EXPECT_NEAR(fb.background.base.val(), goal_base, 0.001);
    EXPECT_NEAR(fb.background.slope.val(), goal_slope, 0.001);
    EXPECT_NEAR(fb.background.curve.val(), goal_curve, 0.001);
  }
}
