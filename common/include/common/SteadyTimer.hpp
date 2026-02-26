#pragma once
#include<chrono>
#include<unordered_map>
#include<ostream>
#include <memory>
#include "utils/PrettyPrint.hpp"
#include <mutex>

//Type of duration of steady clock
typedef std::chrono::steady_clock::duration dur_steady;

struct SteadyTimer {
    std::chrono::time_point<std::chrono::steady_clock> _start;
    dur_steady _dur = dur_steady::zero();

    void start() {
    _start = std::chrono::steady_clock::now();
    }
    void restart() {
    _dur = dur_steady::zero();
    _start = std::chrono::steady_clock::now();
    }
    void stop() {
    auto stop = std::chrono::steady_clock::now();
    _dur += std::chrono::duration_cast<dur_steady>(stop - _start);
    }

    dur_steady duration() {
    return _dur;
    }
};

struct MultiTimerSteady {
  SteadyTimer& get(const std::string& name) {
    _timers.insert({name, std::make_unique<SteadyTimer>()});
    return *_timers[name];
  }
  SteadyTimer& get_threadsafe(const std::string& name) {
      SteadyTimer *t;
      map_lock.lock();
      _timers.insert({name, std::make_unique<SteadyTimer>()});
      t = _timers[name].get();
      map_lock.unlock();
      return *t;
  }
  void start(const std::string& name) {
    _timers.insert({name, std::make_unique<SteadyTimer>()});
    _timers[name]->start();
  }
  void restart(const std::string& name) {
    _timers.insert({name, std::make_unique<SteadyTimer>()});
    _timers[name]->restart();
  }
  void stop(const std::string& name) {
    _timers[name]->stop();
  }
  dur_steady duration(const std::string& name) {
    return _timers[name]->duration();
  }

  dur_steady sum() {
    dur_steady sum = dur_steady::zero();
    for(const auto& p : _timers) {
      sum += p.second->duration();
    }
    return sum;
  }

  std::unordered_map<std::string, std::unique_ptr<SteadyTimer>> _timers;
  std::mutex map_lock = std::mutex();
};

class ProfilerSteady {
private:
  inline static std::unique_ptr<MultiTimerSteady> _profiler = std::make_unique<MultiTimerSteady>();
public:
  ProfilerSteady(const ProfilerSteady&) = delete;
  ProfilerSteady& operator=(const ProfilerSteady&) = delete;

  template<typename F>
  inline static void profile(std::string name, F && lambda, bool restart=true) {
    SteadyTimer& t = _profiler->get(name);
    if (restart)
        t.restart();
    else
        t.start();
    std::forward<F&&>(lambda)();
    t.stop();
  }

    template<typename F>
    static void profile_thread(const std::string& name, F && lambda, bool restart=true) {
        SteadyTimer& t = _profiler->get_threadsafe(name);
        if (restart)
            t.restart();
        else
            t.start();
        std::forward<F&&>(lambda)();
        t.stop();
    }

  inline static size_t get_time(const std::string& name) {
      SteadyTimer& t = _profiler->get(name);
      return t.duration().count();
  }

  inline static void report(std::ostream& out) {
    for(const auto& p : _profiler->_timers) {
      out << "[PROFILE][" << p.first << "] " << p.second->duration().count() << " ns\n";
    }
    out << "[PROFILE][total] " << _profiler->sum().count() << " ns\n";
    //Pretty print it
    size_t n = _profiler->_timers.size();
    std::vector<alignment> al(n, RIGHT);
    std::vector<std::string> headers;
    std::vector<std::string> times;
    headers.reserve(n);
    for (const auto& p : _profiler->_timers) {
        headers.push_back(p.first);
        times.push_back(format_time(p.second->duration().count()));
    }
    std::cout << "Table of times:\n" << std::endl; //Two newlines
    print_header(std::cout, headers, al);
    print_row(std::cout, times);
  }
};
