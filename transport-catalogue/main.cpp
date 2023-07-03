#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

#include <iostream>

int main()
{
	using namespace transport;

	int n;
	std::cin >> n;
	auto input_request_queue = input::ParseQueries(std::cin, n);

	TransportCatalogue t;
	for (auto & [stop_name, stop_data] : input_request_queue.stops) {
		t.AddStop(stop_name, stop_data.coordinates);
	}
	for (auto & [stop_name, stop_data] : input_request_queue.stops) {
		for (auto & [other_name, distance] : stop_data.distances) {
			auto stop_from = t.FindStop(stop_name);
			auto stop_to = t.FindStop(other_name);
			t.SetStopDistance(stop_from.value(), stop_to.value(), distance);
		}
	}
	for (auto & [bus_name, route] : input_request_queue.buses) {
		t.AddRoute(bus_name, route.stops, route.is_looped);
	}

	int k;
	std::cin >> k;
	auto stat_request_queue = stat::ParseQueries(std::cin, k);

	for (auto & entity : stat_request_queue) {
		if (entity.type == stat::QueryType::BUS) {
			auto route = t.FindRoute(entity.name);
			if (route.has_value()) {
				stat::PrintBusInfo(std::cout, entity.name, t.GetRouteInfo(route.value()));
			} else {
				stat::PrintBusNotFound(std::cout, entity.name);
			}
		} else if (entity.type == stat::QueryType::STOP) {
			auto stop = t.FindStop(entity.name);
			if (stop.has_value()) {
				stat::PrintBusesForStop(std::cout, entity.name, t.GetBusesForStop(stop.value()));
			} else {
				stat::PrintStopNotFound(std::cout, entity.name);
			}
		}
	}

	return 0;
}
