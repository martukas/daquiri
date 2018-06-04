#include <gtest/gtest.h>
#include "periodic_trigger.h"

TEST(PeriodicTrigger, Init)
{
  DAQuiri::PeriodicTrigger pt;
  EXPECT_FALSE(pt.enabled_);
  EXPECT_TRUE(pt.timeout_.is_not_a_date_time());
  EXPECT_FALSE(pt.triggered_);
}

TEST(PeriodicTrigger, GetSettings)
{
  DAQuiri::PeriodicTrigger pt;
  pt.enabled_ = true;
  pt.clear_using_ = DAQuiri::ClearReferenceTime::NativeTime;
  pt.timeout_ = boost::posix_time::seconds(1);

  auto sets = pt.settings(3);

  auto enabled = sets.find(DAQuiri::Setting("enabled"));
  EXPECT_TRUE(enabled.get_bool());
  EXPECT_TRUE(enabled.has_index(3));

  auto cusing = sets.find(DAQuiri::Setting("clear_using"));
  EXPECT_EQ(cusing.selection(), DAQuiri::ClearReferenceTime::NativeTime);
  EXPECT_TRUE(cusing.has_index(3));

  auto cat = sets.find(DAQuiri::Setting("clear_at"));
  EXPECT_EQ(cat.duration(), boost::posix_time::seconds(1));
  EXPECT_TRUE(cat.has_index(3));
}

TEST(PeriodicTrigger, SetSettings)
{
  DAQuiri::PeriodicTrigger pt;
  auto sets = pt.settings(0);

  sets.set(DAQuiri::Setting::boolean("enabled", true));
  sets.set(DAQuiri::Setting::integer("clear_using", DAQuiri::ClearReferenceTime::NativeTime));
  sets.set(DAQuiri::Setting("clear_at", boost::posix_time::seconds(1)));

  pt.settings(sets);

  EXPECT_TRUE(pt.enabled_);
  EXPECT_EQ(pt.clear_using_, DAQuiri::ClearReferenceTime::NativeTime);
  EXPECT_EQ(pt.timeout_, boost::posix_time::seconds(1));
}

TEST(PeriodicTrigger, UseProducer)
{
  DAQuiri::PeriodicTrigger pt;
  pt.enabled_ = true;

  DAQuiri::Status s1;
  s1.producer_time = boost::posix_time::microsec_clock::universal_time();

  auto s2 = s1;
  s2.producer_time += boost::posix_time::milliseconds(500);
  pt.update_times(s1, s2);
  EXPECT_EQ(pt.recent_time_, boost::posix_time::milliseconds(500));

  auto s3 = s2;
  s3.producer_time += boost::posix_time::milliseconds(1000);
  pt.update_times(s2, s3);
  EXPECT_EQ(pt.recent_time_, boost::posix_time::milliseconds(1500));
}

TEST(PeriodicTrigger, UseProducerPathological)
{
  DAQuiri::PeriodicTrigger pt;
  pt.enabled_ = true;

  DAQuiri::Status s_bad;
  s_bad.producer_time = boost::posix_time::not_a_date_time;

  DAQuiri::Status s1;
  s1.producer_time = boost::posix_time::microsec_clock::universal_time();

  pt.update_times(s_bad, s1);
  EXPECT_EQ(pt.recent_time_, boost::posix_time::milliseconds(0));

  pt.update_times(s1, s_bad);
  EXPECT_EQ(pt.recent_time_, boost::posix_time::milliseconds(0));
}

TEST(PeriodicTrigger, UseConsumer)
{
  DAQuiri::PeriodicTrigger pt;
  pt.enabled_ = true;
  pt.clear_using_ = DAQuiri::ClearReferenceTime::ConsumerWallClock;

  DAQuiri::Status s1;
  s1.consumer_time = boost::posix_time::microsec_clock::universal_time();

  auto s2 = s1;
  s2.consumer_time += boost::posix_time::milliseconds(500);
  pt.update_times(s1, s2);
  EXPECT_EQ(pt.recent_time_, boost::posix_time::milliseconds(500));

  auto s3 = s2;
  s3.consumer_time += boost::posix_time::milliseconds(1000);
  pt.update_times(s2, s3);
  EXPECT_EQ(pt.recent_time_, boost::posix_time::milliseconds(1500));
}

TEST(PeriodicTrigger, UseConsumerPathological)
{
  DAQuiri::PeriodicTrigger pt;
  pt.enabled_ = true;
  pt.clear_using_ = DAQuiri::ClearReferenceTime::ConsumerWallClock;

  DAQuiri::Status s_bad;
  s_bad.consumer_time = boost::posix_time::not_a_date_time;

  DAQuiri::Status s1;
  s1.consumer_time = boost::posix_time::microsec_clock::universal_time();

  pt.update_times(s_bad, s1);
  EXPECT_EQ(pt.recent_time_, boost::posix_time::milliseconds(0));

  pt.update_times(s1, s_bad);
  EXPECT_EQ(pt.recent_time_, boost::posix_time::milliseconds(0));
}

TEST(PeriodicTrigger, UseNative)
{
  DAQuiri::PeriodicTrigger pt;
  pt.enabled_ = true;
  pt.clear_using_ = DAQuiri::ClearReferenceTime::NativeTime;

  DAQuiri::Status s1;
  s1.stats["native_time"] = DAQuiri::Setting::precise("native_time", 0);

  auto s2 = s1;
  s2.stats["native_time"] = DAQuiri::Setting::precise("native_time", 5000);
  pt.update_times(s1, s2);
  EXPECT_EQ(pt.recent_time_, boost::posix_time::microseconds(5));

  auto s3 = s2;
  s3.stats["native_time"] = DAQuiri::Setting::precise("native_time", 15000);
  pt.update_times(s2, s3);
  EXPECT_EQ(pt.recent_time_, boost::posix_time::microseconds(15));
}

TEST(PeriodicTrigger, UseNativePathological)
{
  DAQuiri::PeriodicTrigger pt;
  pt.enabled_ = true;
  pt.clear_using_ = DAQuiri::ClearReferenceTime::NativeTime;

  DAQuiri::Status s_bad;

  DAQuiri::Status s1;
  s1.stats["native_time"] = DAQuiri::Setting::precise("native_time", 0);

  pt.update_times(s_bad, s1);
  EXPECT_EQ(pt.recent_time_, boost::posix_time::milliseconds(0));

  pt.update_times(s1, s_bad);
  EXPECT_EQ(pt.recent_time_, boost::posix_time::milliseconds(0));
}

TEST(PeriodicTrigger, EvalTrigger)
{
  DAQuiri::PeriodicTrigger pt;
  pt.enabled_ = true;
  pt.timeout_ = boost::posix_time::seconds(1);

  pt.eval_trigger();
  EXPECT_FALSE(pt.triggered_);

  pt.recent_time_ = boost::posix_time::milliseconds(500);
  pt.eval_trigger();
  EXPECT_FALSE(pt.triggered_);

  pt.recent_time_ = boost::posix_time::milliseconds(1000);
  pt.eval_trigger();
  EXPECT_TRUE(pt.triggered_);
  EXPECT_EQ(pt.recent_time_, boost::posix_time::seconds(0));

  pt.triggered_ = false;
  pt.recent_time_ = boost::posix_time::milliseconds(1500);
  pt.eval_trigger();
  EXPECT_TRUE(pt.triggered_);
  EXPECT_EQ(pt.recent_time_, boost::posix_time::milliseconds(500));
}