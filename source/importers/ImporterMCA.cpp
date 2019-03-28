#include "ImporterMCA.h"
#include <core/consumer_factory.h>
#include <date/date.h>
#include <iostream>
#include <core/util/string_extensions.h>

#include <core/util/custom_logger.h>

struct __attribute__ ((packed)) Time
{
  int32_t time;
  int16_t milli_time;
  int16_t time_zone;
  int16_t DST_flag;
};

struct __attribute__ ((packed)) Elapsed
{
  int32_t live_time;
  int32_t real_time;
  int32_t sweeps;
  double comp;
};

struct __attribute__ ((packed)) Calibration
{
  float ecal2;
  float ecal1;
  float ecal0;
  char unit_name[5];
  char unit_type;
  char cformat;
  char corder;
};

struct __attribute__ ((packed)) Header
{
  int16_t read_type;
  int16_t mca_number;
  int16_t read_region;
  int32_t tag;
  char spectrum_name[26];
  int16_t acquisition_mode;
  Time start_time;
  Elapsed elapsed;
  Calibration calibration;
  int16_t filler[19]; // \todo why is this higher than in specs?
  int16_t num_channels;
};

bool ImporterMCA::validate(const boost::filesystem::path& path) const
{
  (void) path;
  return true;
}

void ImporterMCA::import(const boost::filesystem::path& path, DAQuiri::ProjectPtr project)
{
  std::ifstream file(path.string(), std::ios::binary);

  Header header;
  file.read(reinterpret_cast<char*>(&header), sizeof(Header));
  std::vector<int32_t> spectrum(static_cast<size_t>(header.num_channels));
  file.read(reinterpret_cast<char*>(spectrum.data()), spectrum.size() * sizeof(int32_t));

  std::string spec_name(header.spectrum_name, header.spectrum_name + 25);
  INFO("Name: {}", spec_name);
  INFO("Mode: {}", header.acquisition_mode);
  INFO("Read type: {}", header.read_type);
  INFO("Read region: {}", header.read_region);
  INFO("MCA number: {}", header.mca_number);
  INFO("Tag: {}", header.tag);

  // \todo deal with time zone
  date::sys_time<std::chrono::seconds> t;
  t += std::chrono::seconds(header.start_time.time);
  INFO("time.ms_time: {}", header.start_time.milli_time);
  INFO("time.time_zone: {}", header.start_time.time_zone);
  INFO("time.DST_flag: {}", header.start_time.DST_flag);

  auto lt_ms = static_cast<int64_t>(header.elapsed.live_time) * 10;
  auto rt_ms = static_cast<int64_t>(header.elapsed.real_time) * 10;
  INFO("elapsed.sweeps: {}", header.elapsed.sweeps);
  INFO("elapsed.comp: {}", header.elapsed.comp);

  auto hist = DAQuiri::ConsumerFactory::singleton().create_type("Histogram 1D");
  if (!hist)
    throw std::runtime_error("<ImporterMCA> could not get a valid Histogram 1D from factory");

  hist->set_attribute(DAQuiri::Setting::text("name", path.stem().string()));
  hist->set_attribute(DAQuiri::Setting::boolean("visible", true));
  hist->set_attribute(DAQuiri::Setting("start_time", t));
  hist->set_attribute(DAQuiri::Setting("live_time", std::chrono::milliseconds(lt_ms)));
  hist->set_attribute(DAQuiri::Setting("real_time", std::chrono::milliseconds(rt_ms)));

  for (size_t i = 0; i < spectrum.size(); ++i)
  {
    DAQuiri::Entry new_entry;
    new_entry.first.resize(1);
    new_entry.first[0] = i;
    new_entry.second = PreciseFloat(spectrum[i]);
    entry_list.push_back(new_entry);
  }

  DAQuiri::Detector det;

  std::string cal_unit_type(&header.calibration.unit_type, 1);
  INFO("XCal.unit_type: {}", cal_unit_type);

  std::string cal_cformat(&header.calibration.cformat, 1);
  INFO("XCal.cformat: {}", cal_cformat);

  std::vector<double> calibration{header.calibration.ecal0,
                                  header.calibration.ecal1,
                                  header.calibration.ecal2};

  std::string cal_corder(&header.calibration.corder, 1);
  trim(cal_corder);
  if (!cal_corder.empty())
    calibration.resize(std::stoul(cal_corder) + 1);
  else
  {
    if (calibration.back() == 0.0)
      calibration.pop_back();
    if (calibration.back() == 0.0)
      calibration.pop_back();
  }

  DAQuiri::CalibID from("energy", "unknown", "");
  DAQuiri::CalibID to("energy", "unknown", "keV");

  std::string cal_units(header.calibration.unit_name, header.calibration.unit_name + 4);
  auto caluzeros = cal_units.find('\0');
  if (caluzeros != std::string::npos)
    cal_units.erase(caluzeros);
  trim(cal_units);
  if (!cal_units.empty())
    to.units = cal_units;

  DAQuiri::Calibration new_calib(from, to);
  new_calib.function("Polynomial", calibration);
  det.set_calibration(new_calib);
//  DBG("det = {}", det.debug(""));

  hist->set_detectors({det});

  hist->set_attribute(DAQuiri::Setting::text("value_latch", "energy"));

  hist->import(*this);

  project->add_consumer(hist);
}
