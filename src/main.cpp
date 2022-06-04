#include <prometheus/gauge.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

#include <array>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <regex>
#include <iostream>

#include <dlsmon_config.h>

int main() {
  using namespace prometheus;

	char* serve_addr = getenv("DLSMON_ADDR");
	char* openwrt_cmd = getenv("DLSMON_CMD");
	int sleep_interval = std::atoi(getenv("DLSMON_SLEEP") ? getenv("DLSMON_SLEEP") : "1");

	if(serve_addr == nullptr || openwrt_cmd == nullptr) {
		std::cerr << "environment variables DLSMON_ADDR or DLSMON_CMD unspecified" << std::endl;
		return 1;
	}

  // create an http server
  Exposer exposer{serve_addr};

  // create a metrics registry
  // @note it's the users responsibility to keep the object alive
  auto registry = std::make_shared<Registry>();

  // add a new counter family to the registry (families combine values with the
  // same name, but distinct label dimensions)
  //
  // @note please follow the metric-naming best-practices:
  // https://prometheus.io/docs/practices/naming/
  auto& up_gauge = BuildGauge()
                             .Name("dlsmon_up")
                             .Help("whether dsl link is up")
                             .Register(*registry);

	auto& up = up_gauge.Add({});

  auto& uptime_gauge = BuildGauge()
                             .Name("dlsmon_uptime")
                             .Help("uptime in seconds")
                             .Register(*registry);

	auto& uptime = uptime_gauge.Add({});

  auto& noise_gauge = BuildGauge()
                             .Name("dlsmon_noise")
                             .Help("noise margin (SNR)")
                             .Register(*registry);

	auto& noise_up = noise_gauge.Add({{"direction", "up"}});
	auto& noise_down = noise_gauge.Add({{"direction", "down"}});

  // ask the exposer to scrape the registry on incoming HTTP requests
  exposer.RegisterCollectable(registry);

	const unsigned int LENGTH = 256;
	char buffer[LENGTH];

  for (;;) {
    std::this_thread::sleep_for(std::chrono::seconds(sleep_interval));

		FILE* response = popen(openwrt_cmd, "r");

		while(fgets(buffer, LENGTH, response)) {
			std::regex reg_is_up("^Line State:\\s*(UP)?");
			std::cmatch match_up;
			std::regex_search(buffer, match_up, reg_is_up);
			if(!match_up.empty()) {
				up.Set((int) (match_up[1].length() != 0));
			}

			std::regex reg_uptime("^Line Uptime Seconds:\\s*(\\d*)");
			std::cmatch match_uptime;
			std::regex_search(buffer, match_uptime, reg_uptime);
			if(!match_uptime.empty()) {
				uptime.Set(std::stoi(match_uptime[1].str()));
			}

			std::regex reg_noise_margin("^Noise Margin \\(SNR\\):\\s*Down: (\\d*\\.\\d) dB \\/ Up: (\\d*\\.\\d) dB");
			std::cmatch match_noise_margin;
			std::regex_search(buffer, match_noise_margin, reg_noise_margin);
			if(!match_noise_margin.empty()) {
				noise_down.Set(std::stof(match_noise_margin[1].str()));	
				noise_up.Set(std::stof(match_noise_margin[2].str()));	
			}
		}

		pclose(response);
  }
  return 0;
}
