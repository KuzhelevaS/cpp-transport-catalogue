#pragma once

#include <vector>
#include <unordered_map>
#include <deque>
#include <string>
#include <optional>
#include <set>
#include <utility>

#include "geo.h"

namespace transport {
	using geo::Coordinates;
	class TransportCatalogue {
		struct Stop;
		struct Bus;
	public:
		using StopPtr = const Stop*;
		using BusPtr = const Bus*;

		struct RouteInfo {
			unsigned stops_count;
			unsigned unique_stops_count;
			uint64_t length; //расстояние между остановками по дороге
			double curvature; //извилистость
		};

		void AddStop(std::string_view name, Coordinates coord);
		std::optional<StopPtr> FindStop(std::string_view name) const;
		std::vector<std::string_view> GetBusesForStop(StopPtr stop) const;
		void SetStopDistance(StopPtr from, StopPtr to, int distance);
		int GetStopDistance(StopPtr from, StopPtr to) const;

		void AddRoute(std::string_view name, const std::vector<std::string> & stops,
			bool is_looped);
		std::optional<BusPtr> FindRoute(std::string_view name) const;
		RouteInfo GetRouteInfo(BusPtr bus) const;

	private:
		std::deque<Stop> stops_storage;
		std::deque<Bus> buses_storage;
		std::unordered_map<std::string_view, Stop*> stops;
		std::unordered_map<std::string_view, Bus*> buses;

		struct Stop {
			std::string name;
			Coordinates coordinates;
			std::set<std::string_view> buses;
		};
		struct Bus {
			std::string name;
			std::vector<const Stop*> stops;
		};

		struct PairStopsHasher {
			size_t operator()(const std::pair<StopPtr, StopPtr>& stops) const;
		};
		std::unordered_map<std::pair<StopPtr, StopPtr>, int, PairStopsHasher> stop_distances;

		unsigned GetUniqueStops(BusPtr bus) const;
		double GetCoordinateRouteLength(BusPtr bus) const;
		uint64_t GetRealRouteLength(BusPtr bus) const;
	};
}
