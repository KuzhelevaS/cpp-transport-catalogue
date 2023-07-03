#pragma once

#include <vector>
#include <string>
#include <iostream>

#include "transport_catalogue.h"

namespace transport::stat {
	enum class QueryType {
		BUS,
		STOP
	};
	struct Entity {
		QueryType type;
		std::string name;
	};

	std::vector<Entity> ParseQueries(std::istream & input, size_t n);

	void PrintBusInfo(std::ostream & out, std::string_view bus_name, TransportCatalogue::RouteInfo info);
	void PrintBusNotFound(std::ostream & out, std::string_view bus_name);

	void PrintBusesForStop(std::ostream & out, std::string_view stop_name, std::vector<std::string_view> buses);
	void PrintStopNotFound(std::ostream & out, std::string_view stop_name);
}
