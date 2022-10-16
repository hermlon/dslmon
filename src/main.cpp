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

int main() {
  using namespace prometheus;

	char* serve_addr = getenv("DSLMON_ADDR");
	char* cmd = getenv("DSLMON_CMD");
	int sleep_interval = std::atoi(getenv("DSLMON_SLEEP") ? getenv("DSLMON_SLEEP") : "1");

	if(serve_addr == nullptr || cmd == nullptr) {
		std::cerr << "environment variables DSLMON_ADDR or DSLMON_CMD unspecified" << std::endl;
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
  auto& cpu_temp_gauge = BuildGauge()
                             .Name("cpu_temp")
                             .Help("cpu temperature")
                             .Register(*registry);

	auto& cpu_temp = cpu_temp_gauge.Add({});

  // ask the exposer to scrape the registry on incoming HTTP requests
  exposer.RegisterCollectable(registry);

	const unsigned int LENGTH = 256;
	char buffer[LENGTH];

  for (;;) {
    std::this_thread::sleep_for(std::chrono::seconds(sleep_interval));

		FILE* response = popen(cmd, "r");

		while(fgets(buffer, LENGTH, response)) {
			std::regex reg_cpu_temp("^temp=(\\d*\\.\\d*)");
			std::cmatch match_cpu_temp;
			std::regex_search(buffer, match_cpu_temp, reg_cpu_temp);
			if(!match_cpu_temp.empty()) {
				cpu_temp.Set(std::stof(match_cpu_temp[1].str()));
			} else {
				std::cerr << "Could not parse response: " << buffer;
			}
		}

		pclose(response);
  }
  return 0;
}
