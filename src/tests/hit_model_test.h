#include "hit_model.h"
#include <gtest/gtest.h>

TEST(HitModel, AddValue)
{
  DAQuiri::HitModel h;
  EXPECT_EQ("", h.debug());

  h.add_value("a", 2);
  ASSERT_EQ(1, h.values.size());
  ASSERT_EQ(1, h.value_names.size());
  ASSERT_EQ(1, h.name_to_val.size());
  ASSERT_EQ(0, h.name_to_val.at("a"));
  ASSERT_EQ(2, h.values.at(0).bits());
  ASSERT_EQ("a", h.value_names.at(0));
  EXPECT_EQ("a(2b) ", h.debug());

  h.add_value("b", 7);
  ASSERT_EQ(2, h.values.size());
  ASSERT_EQ(2, h.value_names.size());
  ASSERT_EQ(2, h.name_to_val.size());
  ASSERT_EQ(1, h.name_to_val.at("b"));
  ASSERT_EQ(7, h.values.at(1).bits());
  ASSERT_EQ("b", h.value_names.at(1));
  EXPECT_EQ("a(2b) b(7b) ", h.debug());
}

TEST(HitModel, AddTrace)
{
  DAQuiri::HitModel h;
  EXPECT_EQ("", h.debug());

  h.add_trace("x", {2,3});
  ASSERT_EQ(1, h.traces.size());
  ASSERT_EQ(1, h.trace_names.size());
  ASSERT_EQ(1, h.name_to_trace.size());
  ASSERT_EQ(0, h.name_to_trace.at("x"));
  ASSERT_EQ(2, h.traces.at(0).size());
  ASSERT_EQ(std::vector<size_t>({2,3}), h.traces.at(0));
  ASSERT_EQ("x", h.trace_names.at(0));
  EXPECT_EQ("x( 2 3 ) ", h.debug());

  h.add_trace("y", {2,5,7});
  ASSERT_EQ(2, h.traces.size());
  ASSERT_EQ(2, h.trace_names.size());
  ASSERT_EQ(2, h.name_to_trace.size());
  ASSERT_EQ(1, h.name_to_trace.at("y"));
  ASSERT_EQ(3, h.traces.at(1).size());
  ASSERT_EQ(std::vector<size_t>({2,5,7}), h.traces.at(1));
  ASSERT_EQ("y", h.trace_names.at(1));
  EXPECT_EQ("x( 2 3 ) y( 2 5 7 ) ", h.debug());
}
