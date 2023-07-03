#include "transport_catalogue.h"

#include <unordered_set>
#include <iostream>

namespace transport {
	using geo::Coordinates;
	void TransportCatalogue::AddStop(std::string_view name, Coordinates coord) {
		// Проверка защищает от добавления дублей в stops_storage
		if (stops.count(name)) {
			return;
		}
		stops_storage.emplace_back(Stop{std::string(name), coord, {}});
		stops[stops_storage.back().name] = &stops_storage.back();
	}

	std::optional<TransportCatalogue::StopPtr> TransportCatalogue::FindStop(std::string_view name) const {
		if (stops.count(name)) {
			return stops.at(name);
		}
		return std::nullopt;
	}

	std::vector<std::string_view> TransportCatalogue::GetBusesForStop(StopPtr stop) const {
		std::vector<std::string_view> result;
		result.reserve(stop->buses.size());
		for (auto bus : stop->buses) {
			result.push_back(bus);
		}
		return result;
	}

	void TransportCatalogue::SetStopDistance(StopPtr from, StopPtr to, int distance) {
		stop_distances[{from, to}] = distance;
		if (!stop_distances.count({to, from})) {
			stop_distances[{to, from}] = distance;
		}
	}

	int TransportCatalogue::GetStopDistance(StopPtr from, StopPtr to) const {
		return stop_distances.at({from, to});
	}

	void TransportCatalogue::AddRoute(std::string_view name,
		const std::vector<std::string> & route_stops, bool is_looped)
	{
		// Проверка защищает от добавления дублей в buses_storage
		if (buses.count(name)) {
			return;
		}
		Bus bus;
		bus.name = std::string(name);
		buses_storage.push_back(std::move(bus));
		buses[buses_storage.back().name] = &buses_storage.back();
		Bus & b = buses_storage.back();
		if (is_looped) {
			b.stops.reserve(route_stops.size());
		} else {
			b.stops.reserve(route_stops.size() * 2 - 1);
		}
		for (size_t i = 0; i < route_stops.size(); ++i) {
			b.stops.push_back(stops[route_stops[i]]);
			stops[route_stops[i]]->buses.insert(b.name);
		}

		if (!is_looped) {
			for (size_t i = b.stops.size() - 1; i > 0; --i) {
				b.stops.push_back(b.stops[i - 1]);
			}
		}
	}

	std::optional<TransportCatalogue::BusPtr> TransportCatalogue::FindRoute(std::string_view name) const {
		if (buses.count(name)) {
			return buses.at(name);
		}
		return std::nullopt;
	}

	TransportCatalogue::RouteInfo TransportCatalogue::GetRouteInfo(BusPtr bus) const {
		RouteInfo result;
		result.stops_count = bus->stops.size();
		result.unique_stops_count = GetUniqueStops(bus);
		result.length = GetRealRouteLength(bus);
		result.curvature = result.length / GetCoordinateRouteLength(bus);
		return result;
	}

	unsigned TransportCatalogue::GetUniqueStops(BusPtr bus) const {
		std::unordered_set<StopPtr> result;
		for (auto stop : bus->stops) {
			result.insert(stop);
		}
		return result.size();
	}

	double TransportCatalogue::GetCoordinateRouteLength(BusPtr bus) const {
		double sum = 0;
		for (size_t i = 0; i < bus->stops.size() - 1; ++i) {
			sum += ComputeDistance(bus->stops[i]->coordinates, bus->stops[i + 1]->coordinates);
		}
		return sum;
	}

	uint64_t TransportCatalogue::GetRealRouteLength(BusPtr bus) const {
		uint64_t sum = 0;
		for (size_t i = 0; i < bus->stops.size() - 1; ++i) {
			sum += stop_distances.at({bus->stops[i], bus->stops[i + 1]});
		}
		return sum;
	}

	size_t TransportCatalogue::PairStopsHasher::operator()(const std::pair<StopPtr, StopPtr>& stops) const {
		return std::hash<const void*>{}(stops.first) * 37 + std::hash<const void*>{}(stops.second);
	}
}
