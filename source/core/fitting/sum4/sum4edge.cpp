/**
 * @file sum4edge.cpp
 * @brief Construct for simple analysis of a background sample
 *
 * This is an abstraction for background estimation based on:
 * M. Lindstrom, Richard. (1994)
 * Sum and Mean Standard Programs for Activation Analysis.
 * Biological trace element research. 43-45. 597-603.
 * 10.1007/978-1-4757-6025-5_69.
 *
 * Two such samples can be used to generate a polynomial function
 * describing a straight line for background subtraction under a peak.
 *
 * @author Martin Shetty
 */

#include <core/fitting/sum4/sum4edge.h>
#include <core/util/more_math.h>
#include <fmt/format.h>
#include <core/fitting/hypermet/PolyBackground.h>

namespace DAQuiri
{

double SUM4Background::operator()(double x) const
{
  return slope * (x - x_offset) + base;
}

SUM4Edge::SUM4Edge(const WeightedData& spectrum_data)
{
  if (spectrum_data.empty())
    throw std::runtime_error("Cannot create SUM4Edge with empty data");

  dsum_ = {0.0, 0.0};

  if (spectrum_data.data.empty())
    return;

  Lchan_ = spectrum_data.data.front().channel;
  Rchan_ = spectrum_data.data.back().channel;

  min_ = std::numeric_limits<double>::max();
  max_ = std::numeric_limits<double>::min();

  for (const auto& p : spectrum_data.data)
  {
    min_ = std::min(min_, p.count);
    max_ = std::max(max_, p.count);
    dsum_ += {p.count, p.weight_true};
  }

  davg_ = dsum_ / width();
}

double SUM4Edge::width() const
{
  if (!std::isfinite(Rchan_) || !std::isfinite(Lchan_) || (Rchan_ < Lchan_))
    return 0;
  else
    return (Rchan_ - Lchan_ + 1.0);
}

double SUM4Edge::variance() const
{
  // \todo check this in paper
  return square(davg_.sigma());
}

std::string SUM4Edge::to_string() const
{
  return fmt::format("x=[{},{}] cts=[{},{}] sum={} avg={}",
      Lchan_, Rchan_, min_, max_, dsum_.to_string(), davg_.to_string());
}

void to_json(nlohmann::json& j, const SUM4Edge& s)
{
  j["left"] = s.Lchan_;
  j["right"] = s.Rchan_;
}

void from_json(const nlohmann::json& j, SUM4Edge& s)
{
  s.Lchan_ = j["left"];
  s.Rchan_ = j["right"];
}

SUM4Background SUM4Edge::sum4_background(const SUM4Edge& LB, const SUM4Edge& RB)
{
  if (!LB.width())
    throw std::runtime_error("Cannot generate background: empty LB");
  if (!RB.width())
    throw std::runtime_error("Cannot generate background: empty RB");
  if (LB.right() >= RB.left())
    throw std::runtime_error("Cannot generate background: RB must be to the right of LB");

  SUM4Background sum4back;
  sum4back.x_offset = LB.right();
  sum4back.base = LB.average().value();
  sum4back.slope = (RB.average().value() - LB.average().value()) /
      (RB.left() - LB.right());
  return sum4back;
}

}
