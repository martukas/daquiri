#pragma once

#include <cstdint>
#include <string>

#include <core/util/eigen_fix.h>

namespace DAQuiri
{

class EfficiencyCal
{
  // This class loads and calculates the efficiency from an existing fit, as done by Hypermet-PC.
  // See also in:
  // G.L. Molnar, Zs. Revay, T. Belgya:
  // Wide energy range efficiency calibration method for Ge detectors,
  // Nucl. Instr. Meth. A 489 (2002) 140?159
 private:
  std::vector<double> apol_, bpol_, norm_factors_, coefficients_;

  Eigen::SparseMatrix<double> variance_;

  double e_c0, e_c1;
  bool initialized_{false};

 public:
  void load(std::string flnm);
  void close();
  bool initialized() const;
  double val(double energy) const;
  double sigma_rel(double val) const;

 private:
  double e_ortpol(size_t n, double X) const;
};

}