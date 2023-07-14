#pragma once

#include "geo.h"
#include <string>
#include <set>
#include <vector>

namespace transport::domain {
	using geo::Coordinates;

	struct Stop {
		std::string name;
		Coordinates coordinates;
		std::set<std::string_view> buses;
	};
	using StopPtr = const Stop*;

	struct Bus {
		std::string name;
		StopPtr start;
		StopPtr finish;
		std::vector<const Stop*> stops;
	};
	using BusPtr = const Bus*;
}
