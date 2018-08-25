#include "gtest_color_print.h"
#include <core/util/time_extensions.h>
#include <core/util/time.h>

// \todo more and better tests

class TimeExtensions : public TestBase
{
};


TEST_F(TimeExtensions, time_to_iso_extended)
{
  hr_time_t t =
      date::sys_days(date::year{2011} / 12 / 13)
          + std::chrono::hours{14} + std::chrono::minutes{15} +
          std::chrono::seconds{16} + std::chrono::nanoseconds{777888999};

  EXPECT_EQ(to_iso_extended(t), "2011-12-13T14:15:16.777888999");
}

TEST_F(TimeExtensions, time_to_simple)
{
  hr_time_t t =
      date::sys_days(date::year{2011} / 12 / 13)
          + std::chrono::hours{14} + std::chrono::minutes{15} +
          std::chrono::seconds{16};

  EXPECT_EQ(to_simple(t), "2011-12-13 14:15:16");
}

TEST_F(TimeExtensions, time_from_iso_extended)
{

  hr_time_t t =
      date::sys_days(date::year{2011} / 12 / 13)
          + std::chrono::hours{14} + std::chrono::minutes{15} +
          std::chrono::seconds{16} + std::chrono::nanoseconds{777888999};

  EXPECT_EQ(from_iso_extended("2011-12-13T14:15:16.777888999"), t);
}


TEST_F(TimeExtensions, to_from_iso)
{
  hr_time_t in {std::chrono::high_resolution_clock::now()};
  EXPECT_EQ(in, from_iso_extended(to_iso_extended(in)));
}

TEST_F(TimeExtensions, duration_to_very_simple) {
  hr_time_t t1 = date::sys_days(date::year{2000} / 1 / 1) + std::chrono::hours{12};
  hr_time_t t2 = date::sys_days(date::year{2000} / 1 / 7) + std::chrono::hours{12} +
      std::chrono::minutes{5} + std::chrono::seconds{42} +
      std::chrono::nanoseconds{777888999};

  hr_duration_t d = t2 - t1;

  EXPECT_EQ(very_simple(d), "144:05:43");
}

TEST_F(TimeExtensions, duration_to_simple)
{
  hr_time_t t1 = date::sys_days(date::year{2000} / 1 / 1) + std::chrono::hours{12};
  hr_time_t t2 = date::sys_days(date::year{2000} / 1 / 7) + std::chrono::hours{12} +
      std::chrono::minutes{5} + std::chrono::seconds{42} +
      std::chrono::nanoseconds{777888999};

  hr_duration_t d = t2 - t1;

  EXPECT_EQ(to_simple(d), "144:05:42.777888999");
}

TEST_F(TimeExtensions, duration_from_string)
{
  hr_time_t t1 = date::sys_days(date::year{2000} / 1 / 1) + std::chrono::hours{12};
  hr_time_t t2 = date::sys_days(date::year{2000} / 1 / 7) + std::chrono::hours{12} +
      std::chrono::minutes{5} + std::chrono::seconds{42} +
      std::chrono::nanoseconds{777888999};

  hr_duration_t d = t2 - t1;
  EXPECT_EQ(d, duration_from_string("144:05:42.777888999"));
}

TEST_F(TimeExtensions, duration_to_from_string)
{
  hr_time_t t1 {std::chrono::high_resolution_clock::now()};
  hr_time_t t2 {std::chrono::high_resolution_clock::now()};
  hr_duration_t d = t2 - t1;
  EXPECT_EQ(d, duration_from_string(to_simple(d)));
}
