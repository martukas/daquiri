#pragma once

#include <string>
#include <list>

#include <nlohmann/json.hpp>

class UncertainDouble
{
 public:
  UncertainDouble() = default;
  UncertainDouble(double val, double sigma);

  static UncertainDouble from_int(int64_t val, double sigma);
  static UncertainDouble from_uint(uint64_t val, double sigma);
  static UncertainDouble from_double(double val, double sigma);

  double value() const;
  double uncertainty() const;
  double error() const;

  void setValue(double val);
  void setUncertainty(double sigma);

  bool finite() const;

  std::string to_string(bool ommit_tiny = true) const;
//  std::string to_markup() const; // outputs formatted text
  std::string error_percent() const;

  std::string debug() const;

  UncertainDouble& operator*=(const double& other);
  UncertainDouble operator*(const double& other) const;
  UncertainDouble& operator/=(const double& other);
  UncertainDouble operator/(const double& other) const;

  UncertainDouble& operator*=(const UncertainDouble& other);
  UncertainDouble operator*(const UncertainDouble& other) const;
  UncertainDouble& operator/=(const UncertainDouble& other);
  UncertainDouble operator/(const UncertainDouble& other) const;

  UncertainDouble& operator+=(const UncertainDouble& other);
  UncertainDouble operator+(const UncertainDouble& other) const;
  UncertainDouble& operator-=(const UncertainDouble& other);
  UncertainDouble operator-(const UncertainDouble& other) const;

  bool almost(const UncertainDouble& other) const;
  bool operator==(const UncertainDouble& other) const { return value() == other.value(); }
  bool operator<(const UncertainDouble& other) const { return value() < other.value(); }
  bool operator>(const UncertainDouble& other) const { return value() > other.value(); }

  static UncertainDouble average(const std::list<UncertainDouble>& list);

 private:
  double value_{std::numeric_limits<double>::quiet_NaN()};
  double sigma_{std::numeric_limits<double>::quiet_NaN()};

  UncertainDouble& additive_uncert(const UncertainDouble& other);
  UncertainDouble& multipli_uncert(const UncertainDouble& other);
  int exponent() const;
};

void to_json(nlohmann::json& j, const UncertainDouble& s);
void from_json(const nlohmann::json& j, UncertainDouble& s);
