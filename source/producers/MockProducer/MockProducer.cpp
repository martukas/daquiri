#include "MockProducer.h"
#include "custom_timer.h"

#include "custom_logger.h"


void ValueDefinition::define(EventModel& def)
{
  dist = std::normal_distribution<double>(center * max, spread);
  def.add_value(name, max);

  if (trace_size)
    def.add_trace(name, {trace_size});

  DBG << "Adding " << name
         << " at center=" << center
            << " spread=" << spread;
}

void ValueDefinition::generate(size_t index, Event& event, std::default_random_engine& gen)
{
  auto val = generate(gen);
  event.set_value(index, val);
  if (trace_size)
    make_trace(index, event, val);
}

uint32_t ValueDefinition::generate(std::default_random_engine& gen)
{
  return std::round(std::max(std::min(dist(gen), double(max)), 0.0));
}

void ValueDefinition::make_trace(size_t index, Event& e, uint32_t val)
{
  auto& trc = e.trace(index);

  size_t onset = double(trc.size()) * trace_onset;
  size_t peak = double(trc.size()) * (trace_onset + trace_risetime);

  //rise
  double slope_up = double(val) / double(peak-onset);
  for (size_t i = onset; i < peak; ++i)
    trc[i] = (i - onset) * slope_up;

  //fall
  double slope_down = double(val) / double(trc.size() * 10);
  for (size_t i = peak; i < trc.size(); ++i)
    trc[i] = val - (i - peak) * slope_down;

  // add baseline & noise
  for (size_t i=0; i < trc.size(); ++i)
    trc[i] += trace_baseline
        + trace_baseline ? ((rand() % trace_baseline) / 5
                             - trace_baseline/10) : 0;
}



MockProducer::MockProducer()
{
  std::string mp {"MockProducer/"};

  SettingMeta streamid(mp + "StreamID", SettingType::text, "DAQuiri stream ID");
  streamid.set_flag("preset");
  add_definition(streamid);

  SettingMeta si(mp + "SpillInterval", SettingType::integer, "Interval between spills");
  si.set_val("min", 1);
  si.set_val("max", 1000000);
  si.set_val("units", "s");
  add_definition(si);

  SettingMeta res(mp + "Resolution", SettingType::integer, "Resolution");
  res.set_val("min", 4);
  res.set_val("max", 32);
  res.set_val("units", "bits");
  add_definition(res);

  SettingMeta cr(mp + "CountRate", SettingType::floating, "Event rate");
  cr.set_val("min", 1);
  cr.set_val("units", "cps");
  add_definition(cr);

  SettingMeta tm(mp + "TimebaseMult", SettingType::integer, "Timebase multiplier");
  tm.set_val("min", 1);
  tm.set_val("units", "ns");
  add_definition(tm);

  SettingMeta td(mp + "TimebaseDiv", SettingType::integer, "Timebase divider");
  td.set_val("min", 1);
  td.set_val("units", "1/ns");
  add_definition(td);

  SettingMeta dt(mp + "DeadTime", SettingType::floating, "Dead time (% of real time)");
  dt.set_val("min", 0);
  dt.set_val("max", 100);
  dt.set_val("step", 0.01);
  add_definition(dt);

  SettingMeta lambda(mp + "Lambda", SettingType::floating, "Decay constant (λ)");
  lambda.set_val("min", 0);
  lambda.set_val("step", 0.01);
  add_definition(lambda);

  SettingMeta lambda2(mp + "SpillLambda", SettingType::floating, "Decay constant (λ) per spill");
  lambda2.set_val("min", 0);
  lambda2.set_val("max", 100);
  lambda2.set_val("units", "%");
  lambda2.set_val("step", 0.1);
  add_definition(lambda2);

  SettingMeta vc(mp + "ValueCount", SettingType::integer, "Value count");
  vc.set_flag("preset");
  vc.set_val("min", 1);
  vc.set_val("max", 16);
  add_definition(vc);


  SettingMeta valname(mp + "Value/Name", SettingType::text, "Value name");
  valname.set_flag("preset");
  add_definition(valname);

  SettingMeta pc(mp + "Value/PeakCenter", SettingType::floating, "Peak center (% resolution)");
  pc.set_flag("preset");
  pc.set_val("min", 0);
  pc.set_val("max", 100);
  pc.set_val("step", 0.1);
  add_definition(pc);

  SettingMeta ps(mp + "Value/PeakSpread", SettingType::floating, "Peak spread (stddev)");
  ps.set_flag("preset");
  ps.set_val("min", 0);
  ps.set_val("step", 0.01);
  add_definition(ps);

  SettingMeta val(mp + "Value", SettingType::stem);
  val.set_enum(0, mp + "Value/Name");
  val.set_enum(1, mp + "Value/PeakCenter");
  val.set_enum(2, mp + "Value/PeakSpread");
  add_definition(val);



  SettingMeta root("MockProducer", SettingType::stem);
  root.set_flag("producer");
  root.set_enum(0, mp + "StreamID");
  root.set_enum(1, mp + "SpillInterval");
  root.set_enum(2, mp + "Resolution");
  root.set_enum(3, mp + "CountRate");
  root.set_enum(4, mp + "DeadTime");
  root.set_enum(5, mp + "TimebaseMult");
  root.set_enum(6, mp + "TimebaseDiv");
  root.set_enum(7, mp + "Lambda");
  root.set_enum(8, mp + "SpillLambda");
  root.set_enum(9, mp + "ValueCount");
  add_definition(root);

  status_ = ProducerStatus::loaded | ProducerStatus::can_boot;
}

MockProducer::~MockProducer()
{
  daq_stop();
  if (runner_ != nullptr)
  {
    runner_->detach();
    delete runner_;
  }
  die();
}

bool MockProducer::daq_start(SpillQueue out_queue)
{
  if (run_status_.load() > 0)
    return false;

  run_status_.store(1);

  if (runner_ != nullptr)
    delete runner_;

  runner_ = new std::thread(&worker_run, this, out_queue);

  return true;
}

bool MockProducer::daq_stop()
{
  if (run_status_.load() == 0)
    return false;

  run_status_.store(2);

  if ((runner_ != nullptr) && runner_->joinable())
  {
    runner_->join();
    delete runner_;
    runner_ = nullptr;
  }

  wait_ms(500);

  run_status_.store(0);
  return true;
}

bool MockProducer::daq_running()
{
  if (run_status_.load() == 3)
    daq_stop();
  return (run_status_.load() > 0);
}

void MockProducer::read_settings_bulk(Setting &set) const
{
  set = enrich_and_toggle_presets(set);

  set.set(Setting::text("MockProducer/StreamID", stream_id));
  set.set(Setting::integer("MockProducer/SpillInterval", spill_interval_));
  set.set(Setting::integer("MockProducer/Resolution", bits_));
  set.set(Setting::floating("MockProducer/CountRate", count_rate_));
  set.set(Setting::floating("MockProducer/DeadTime", dead_*100));
  set.set(Setting::integer("MockProducer/TimebaseMult", event_definition_.timebase.multiplier()));
  set.set(Setting::integer("MockProducer/TimebaseDiv", event_definition_.timebase.divider()));
  set.set(Setting::floating("MockProducer/Lambda", lambda_));
  set.set(Setting::floating("MockProducer/SpillLambda", spill_lambda_));
  set.set(Setting::integer("MockProducer/ValueCount", integer_t(val_defs_.size())));

  while (set.branches.has_a(Setting({"MockProducer/Value", SettingType::stem})))
    set.branches.remove_a(Setting({"MockProducer/Value", SettingType::stem}));

  for (int i=0; i < int(val_defs_.size()); ++i)
  {
    Setting v = get_rich_setting("MockProducer/Value");
    v.set_indices({i});
    v.branches = get_rich_setting("MockProducer/Value").branches;
    v.set(Setting::text("MockProducer/Value/Name", val_defs_[i].name));
    v.set(Setting::floating("MockProducer/Value/PeakCenter", val_defs_[i].center*100));
    v.set(Setting::floating("MockProducer/Value/PeakSpread", val_defs_[i].spread));
    for (auto& vv : v.branches)
      vv.set_indices({i});
    set.branches.add_a(v);
  }
}


void MockProducer::write_settings_bulk(const Setting &settings)
{
  auto set = enrich_and_toggle_presets(settings);

  stream_id = set.find({"MockProducer/StreamID"}).get_text();
  spill_interval_ = set.find({"MockProducer/SpillInterval"}).get_number();
  bits_ = set.find({"MockProducer/Resolution"}).get_number();
  count_rate_ = set.find({"MockProducer/CountRate"}).get_number();
  lambda_ = set.find({"MockProducer/Lambda"}).get_number();
  spill_lambda_ = set.find({"MockProducer/SpillLambda"}).get_number();
  dead_ = set.find({"MockProducer/DeadTime"}).get_number() * 0.01;

  uint16_t val_count_ = std::max(int(set.find({"MockProducer/ValueCount"}).get_number()), 1);

  if (val_defs_.size() != val_count_)
    val_defs_.resize(val_count_);

  uint32_t resolution = pow(2, uint32_t(bits_));

  for (Setting v : set.branches)
  {
    if (v.id() != "MockProducer/Value")
      continue;
    auto indices = v.indices();
    if (!indices.size())
      continue;
    size_t idx = *indices.begin();
//    DBG << "Write idx " << idx;
    if (idx >= val_defs_.size())
      continue;
    val_defs_[idx].center= v.find({"MockProducer/Value/PeakCenter"}).get_number() * 0.01;
    val_defs_[idx].spread = v.find({"MockProducer/Value/PeakSpread"}).get_number();
    val_defs_[idx].name = v.find({"MockProducer/Value/Name"}).get_text();
    val_defs_[idx].max = resolution;
  }

  event_definition_ = EventModel();
  event_definition_.timebase = TimeBase(set.find({"MockProducer/TimebaseMult"}).get_number(),
                                set.find({"MockProducer/TimebaseDiv"}).get_number());

  for (size_t i=0; i < val_defs_.size(); ++i)
    val_defs_[i].define(event_definition_);
}

void MockProducer::boot()
{
  if (!(status_ & ProducerStatus::can_boot))
  {
    WARN << "<MockProducer> Cannot boot MockProducer. Failed flag check (can_boot == 0)";
    return;
  }

  status_ = ProducerStatus::loaded | ProducerStatus::can_boot;

  INFO << "<MockProducer> Booting"
        << " rate=" << count_rate_ << "cps";
//        << " with peak at " << peak_center_ << "%   stdev=" << peak_spread_;


  clock_ = 0;
  status_ = ProducerStatus::loaded | ProducerStatus::booted | ProducerStatus::can_run;
}

void MockProducer::die()
{
//  INFO << "<MockProducer> Shutting down";
  status_ = ProducerStatus::loaded | ProducerStatus::can_boot;
}

void MockProducer::worker_run(MockProducer* callback,
                              SpillQueue spill_queue)
{
  DBG << "<MockProducer> Starting run   "
      << "  timebase " << callback->event_definition_.timebase.debug() << "ns"
      << "  init_rate=" << callback->count_rate_ << "cps"
      << "  lambda=" << callback->lambda_;

  CustomTimer timer(true);

  spill_queue->enqueue(callback->get_spill(StatusType::start, timer.s()));
  while (callback->run_status_.load() != 2)
  {
    wait_ms(callback->spill_interval_ * 1000);
    spill_queue->enqueue(callback->get_spill(StatusType::running, timer.s()));
  }
  spill_queue->enqueue(callback->get_spill(StatusType::stop, timer.s()));

  callback->run_status_.store(3);
}

void MockProducer::add_hit(Spill& spill)
{
  auto& e = spill.events.last();
  e.set_time(clock_);
  for (size_t i=0; i < val_defs_.size(); ++i)
    val_defs_[i].generate(i, e, gen_);

  ++spill.events;
}

SpillPtr MockProducer::get_spill(StatusType t, double seconds)
{
  SpillPtr spill = std::make_shared<Spill>(stream_id, t);

  recent_pulse_time_ = clock_;

  if (t == StatusType::running)
  {
    double rate = count_rate_;

    if (lambda_)
    {
      double frac = exp(0.0 - lambda_ * seconds);
      rate *= frac;
      DBG << "<MockProducer> s=" << seconds
          << "  frac=" << frac
          << "  rate=" << rate;
    }

    uint64_t event_interval
        = event_definition_.timebase.to_native(spill_interval_ * pow(10,9)) / rate;

    std::uniform_real_distribution<> dis(0, 1);
    uint32_t tothits = (rate * spill_interval_);

    spill->events.reserve(tothits, event_definition_);

    for (uint32_t i=0; i< tothits; i++)
    {
      if (spill_lambda_ < 100)
      {
        double diff = (spill_lambda_ * 0.01)  +
            (1.0 - (spill_lambda_ * 0.01)) *
            (double(tothits - i) / double(tothits));
        if (dis(gen_) < diff)
          add_hit(*spill);
      }
      else
        add_hit(*spill);
      clock_ += event_interval;
    }
  }

  spill->events.finalize();

  fill_stats(*spill);

  return spill;
}

void MockProducer::fill_stats(Spill &spill) const
{
  spill.event_model = event_definition_;

  double duration = clock_;
  double duration_live = duration * (1.0 - dead_);
  double duration_trigger = duration * (1.0 - 0.5 * dead_);

  spill.state.branches.add(Setting::floating("native_time", duration));
  spill.state.branches.add(Setting::floating("live_time", duration_live));
  spill.state.branches.add(Setting::floating("live_trigger", duration_trigger));
  spill.state.branches.add(Setting::floating("pulse_time", double(recent_pulse_time_)));
}