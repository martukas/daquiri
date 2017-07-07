#pragma once

#include "setting_metadata.h"
#include <boost/date_time.hpp>
#include "pattern.h"
#include "precise_float.h"
#include "container.h"

namespace DAQuiri {

enum Match
{
  id      = 1 << 0,
  name    = 1 << 1, //deprecate?
  address = 1 << 2, //deprecate?
  indices = 1 << 3,
  stype   = 1 << 4
};

inline Match operator|(Match a, Match b)
{
  return static_cast<Match>(static_cast<int>(a) | static_cast<int>(b));
}

inline Match operator&(Match a, Match b)
{
  return static_cast<Match>(static_cast<int>(a) & static_cast<int>(b));
}

class Setting;

class Setting
{
public:
  Container<Setting> branches;

  Setting() {}
  Setting(std::string id);
  Setting(SettingMeta meta);

  // id, type, metadata
  explicit operator bool() const;
  std::string id() const;
  bool is(SettingType type) const;
  SettingMeta metadata() const;
  void strip_metadata();
  void enrich(const std::map<std::string, SettingMeta> &,
              bool impose_limits = false);

  // equality
  bool shallow_equals(const Setting& other) const;
  bool operator== (const Setting& other) const;
  bool operator!= (const Setting& other) const;
  bool compare(const Setting &other, Match m) const;

  // indices
  void clear_indices();
  void set_indices(std::initializer_list<int32_t> l);
  void add_indices(std::initializer_list<int32_t> l);
  bool has_index(int32_t i) const;

  // clone value
  void set_val(const Setting &other);

  // recursive access
  void set(const Setting &s, Match m, bool greedy = false);
  //template for all containers?
  void set(const std::list<Setting> &s, Match m, bool greedy = false);
  bool has(Setting address, Match m) const;
  void erase(Setting address, Match m);
  Setting find(Setting address, Match m) const;
  std::list<Setting> find_all(const Setting &setting, Match m) const;

  // metadata-related convenience functions
  void enable_if_flag(bool enable, const std::string &flag);
  void hide(bool h = true);
  void condense();
  void cull_hidden();
  void cull_readonly();
  void enforce_limits();

  // to string
  std::string debug(std::string prepend = std::string()) const;
  std::string val_to_pretty_string() const;
  std::string indices_to_string(bool showblanks = false) const;

  //time
  void set_time(boost::posix_time::ptime v);
  boost::posix_time::ptime time() const;

  //duration
  void set_duration(boost::posix_time::time_duration v);
  boost::posix_time::time_duration duration() const;

  //pattern
  void set_pattern(Pattern v);
  Pattern pattern() const;

  //text
  void set_text(std::string val);
  std::string get_text() const;

  // numerics
  bool numeric() const;
  double get_number();
  void set_number(double);

  // prefix
  Setting& operator++();
  Setting& operator--();

  // postfix
  Setting operator++(int);
  Setting operator--(int);

  // serialization
  friend void to_json(json& j, const Setting &s);
  friend void from_json(const json& j, Setting &s);

private:
  bool may_have_branches() const;
  bool compare_indices(const Setting &other) const;

  bool find_dfs(Setting& result, const Setting& root, Match m) const;
  void erase_matches(const Setting&, Setting&, Match m);
  bool set_first(const Setting &setting, Match m);
  void set_all(const Setting &setting, Match m);

  std::string val_to_string() const;
  json val_to_json() const;
  void val_from_json(const json &j);

  SettingMeta        metadata_;
  std::set<int32_t>  indices_;

  int64_t                          value_int     {0};
  double                           value_dbl     {0};
  PreciseFloat                     value_precise {0};
  std::string                      value_text;
  boost::posix_time::ptime         value_time;
  boost::posix_time::time_duration value_duration;
  Pattern                          value_pattern;
};

}
