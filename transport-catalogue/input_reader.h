#pragma once

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>

#include "geo.h"
#include <sstream>

namespace transport::input {
	using geo::Coordinates;
	struct Stop {
		Coordinates coordinates;
		std::unordered_map<std::string, int> distances;
	};
	struct Route {
		bool is_looped; // > if looped, - if not looped
		std::vector<std::string> stops;
	};

	struct ResultGroup {
		std::unordered_map<std::string, Stop> stops;
		std::unordered_map<std::string, Route> buses;
	};

	ResultGroup ParseQueries(std::istream & input, size_t n);
}
