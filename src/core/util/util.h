#pragma once

#include <string>
#include <sstream>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/algorithm/string.hpp>

const std::string k_branch_mid = "\u251C\u2500";
const std::string k_branch_end = "\u2514\u2500";
const std::string k_branch_pre = "\u2502 ";

const std::string k_branch_mid_A = "\u2560\u2550";
const std::string k_branch_end_A = "\u255A\u2550";
const std::string k_branch_pre_A = "\u2551 ";

const std::string k_branch_mid_B = "\u2523\u2501";
const std::string k_branch_end_B = "\u2517\u2501";
const std::string k_branch_pre_B = "\u2503 ";

inline std::string to_max_precision(double number)
{
  std::stringstream ss;
  ss << std::setprecision(std::numeric_limits<double>::max_digits10) << number;
  return ss.str();
}

inline std::string to_str_precision(double number, int precision = -1) {
  std::ostringstream ss;
  if (precision < 0)
    ss << number;
  else
    ss << std::setprecision(precision) << number;
  return ss.str();
}

inline std::string trim_all(std::string text)
{
  std::istringstream iss(text);
  text = "";
  std::string s;
  while(iss >> s)
  {
    if ( text != "" ) text += " " + s;
    else text = s;
  }
  return text;
}

inline std::string to_str_decimals(double number, int decimals = 0) {
  std::ostringstream ss;
  ss << std::fixed << std::setprecision(decimals) << number;
  return ss.str();
}

template<typename T> inline bool is_number(T x)
{
  std::string s;
  std::stringstream ss;
  ss << x;
  ss >>s;
  if(s.empty() || std::isspace(s[0]) || std::isalpha(s[0])) return false ;
  char * p ;
  strtod(s.c_str(), &p) ;
  return (*p == 0) ;
}

inline uint16_t sig_digits(std::string st)
{
  boost::to_lower(st);
  boost::replace_all(st, "+", "");
  boost::replace_all(st, "-", "");
  if (boost::contains(st, "e")) {
    size_t l = st.find('e');
    st = st.substr(0,l);
  }
  //assume only one number in string
  uint16_t count=0; bool past_zeros = false;
  for(size_t i=0;i<st.size();i++) {
    bool digit = std::isdigit(st[i]);
    if (digit && (st[i] != '0'))
      past_zeros = true;
    if(past_zeros && digit)
      count++;
  }
  return count;
}

inline int16_t order_of(double val)
{
  if (!std::isfinite(val))
    return 0;
  else
    return std::floor(std::log10(std::abs(val)));
}

inline double get_precision(std::string value)
{
  boost::trim(value);
  boost::trim_if(value, boost::is_any_of("+-"));
  std::vector<std::string> parts;
  boost::split(parts, value, boost::is_any_of("Ee"));

  std::string mantissa;
  std::string expstr;

  if (parts.size() >= 1)
    mantissa = parts.at(0);
  if (parts.size() >= 2)
    expstr = parts.at(1);

  int exponent = 0;
  if (!expstr.empty() && is_number(expstr))
    exponent = boost::lexical_cast<double>(expstr);

  int sigpos = 0;
  size_t pointpos = mantissa.find('.');
  if (pointpos != std::string::npos)
    sigpos -= mantissa.size() - pointpos - 1;

  // return factor for shifting according to exponent
  return pow(10.0, double(sigpos + exponent));
}

inline std::string itobin16 (uint16_t bin)
{
  std::stringstream ss;
  for (int k = 0; k < 16; ++k) {
    if (bin & 0x8000)
      ss << "1";
    else
      ss << "0";
    bin <<= 1;
  }
  return ss.str();
}

inline std::string itobin32 (uint32_t bin)
{
  uint16_t lo = bin & 0x0000FFFF;
  uint16_t hi = (bin >> 16) & 0x0000FFFF;

  return (itobin16(hi) + " " + itobin16(lo));
}

inline std::string itobin64 (uint64_t bin)
{
  uint32_t lo = bin & 0x00000000FFFFFFFF;
  uint32_t hi = (bin >> 32) & 0x00000000FFFFFFFF;

  return (itobin32(hi) + " " + itobin32(lo));
}


inline std::string itohex64 (uint64_t bin)
{
  std::stringstream stream;
  stream << std::uppercase << std::setfill ('0') << std::setw(sizeof(uint64_t)*2)
         << std::hex << bin;
  return stream.str();
}

inline std::string itohex32 (uint32_t bin)
{
  std::stringstream stream;
  stream << std::uppercase << std::setfill ('0') << std::setw(sizeof(uint32_t)*2)
         << std::hex << bin;
  return stream.str();
}

inline std::string itohex16 (uint16_t bin)
{
  std::stringstream stream;
  stream << std::uppercase << std::setfill ('0') << std::setw(sizeof(uint16_t)*2)
         << std::hex << bin;
  return stream.str();
}

inline boost::posix_time::ptime from_iso_extended(std::string str)
{
  boost::posix_time::ptime tm;
  if (str.empty())
    return tm;
  boost::posix_time::time_input_facet *tif = new boost::posix_time::time_input_facet;
  tif->set_iso_extended_format();
  std::stringstream iss(str);
  iss.imbue(std::locale(std::locale::classic(), tif));
  iss >> tm;
  return tm;
}

inline boost::posix_time::ptime from_custom_format(std::string str, std::string format)
{
  boost::posix_time::ptime tm;
  if (str.empty())
    return tm;
  boost::posix_time::time_input_facet
      *tif(new boost::posix_time::time_input_facet(format));
  std::stringstream iss(str);
  iss.imbue(std::locale(std::locale::classic(), tif));
  iss >> tm;
  return tm;
}

inline std::string very_simple(const boost::posix_time::time_duration &duration)
{
  uint64_t s = duration.total_seconds();
  uint64_t min = s / 60;
  uint64_t h   = min / 60;
  std::stringstream ss;
  ss << h << ":"
     << std::setfill('0') << std::setw(2) << min - (h*60) << ":"
     << std::setfill('0') << std::setw(2) << s - (min*60);
  return ss.str();
}
