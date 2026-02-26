#pragma once
#include<chrono>
#include<unordered_map>
#include<ostream>

struct Timer {
  std::chrono::time_point<std::chrono::high_resolution_clock> _start;
  std::chrono::microseconds _dur = std::chrono::microseconds::zero();

  void start() {
    _start = std::chrono::high_resolution_clock::now();
  }
  void restart() {
    _dur = std::chrono::microseconds::zero();
    _start = std::chrono::high_resolution_clock::now();
  }
  void stop() {
    auto stop = std::chrono::high_resolution_clock::now();
    _dur += std::chrono::duration_cast<std::chrono::microseconds>(stop - _start);
  }

  std::chrono::microseconds duration() {
    return _dur;
  }
};

struct MultiTimer {
  Timer& get(const std::string& name) {
    _timers.insert({name, std::make_unique<Timer>()});
    return *_timers[name];
  }
  void start(const std::string& name) {
    _timers.insert({name, std::make_unique<Timer>()});
    _timers[name]->start();
  }
  void restart(const std::string& name) {
    _timers.insert({name, std::make_unique<Timer>()});
    _timers[name]->restart();
  }
  void stop(const std::string& name) {
    _timers[name]->stop();
  }
  std::chrono::microseconds duration(const std::string& name) {
    return _timers[name]->duration();
  }

  std::chrono::microseconds sum() {
    std::chrono::microseconds sum = std::chrono::microseconds::zero();
    for(const auto& p : _timers) {
      sum += p.second->duration();
    }
    return sum;
  }

  std::unordered_map<std::string, std::unique_ptr<Timer>> _timers;
};

template<auto Func>
struct FunctionTimerWrapper {
  Timer& _timer;
  FunctionTimerWrapper(Timer& timer): _timer(timer) {}
  template<typename... Args>
  auto operator()(Args&&... args) {
    _timer.restart();
    auto tmp(Func(std::forward<Args>(args)...));
    _timer.stop();
    return tmp;
  }
};

class Profiler {
private:
  inline static std::unique_ptr<MultiTimer> _profiler = std::make_unique<MultiTimer>();
public:
  Profiler(const Profiler&) = delete;
  Profiler& operator=(const Profiler&) = delete;

  template<typename F>
  inline static void profile(std::string name, F && lambda) {
    Timer& t = _profiler->get(name);
    t.restart();
    std::forward<F&&>(lambda)();
    t.stop();
  }

  inline static void report(std::ostream& out) {
    for(const auto& p : _profiler->_timers) {
      out << "[PROFILE][" << p.first << "] " << p.second->duration().count() << " us\n";
    }
    out << "[PROFILE][total] " << _profiler->sum().count() << " us\n";
  }
};
