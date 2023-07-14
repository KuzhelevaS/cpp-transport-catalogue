#pragma once

#include <vector>
#include <unordered_map>
#include <deque>
#include <string>
#include <optional>
#include <utility>

#include "domain.h"

namespace transport {
	class TransportCatalogue {
		using Stop = domain::Stop;
		using Bus = domain::Bus;
	public:
		using StopPtr = domain::StopPtr;
		using BusPtr = domain::BusPtr;

		struct RouteInfo {
			int stops_count;
			int unique_stops_count;
			uint64_t length; //расстояние между остановками по дороге
			double curvature; //извилистость
		};

		void AddStop(std::string_view name, geo::Coordinates coord);
		std::optional<StopPtr> FindStop(std::string_view name) const;
		std::vector<std::string_view> GetBusesForStop(StopPtr stop) const;
		void SetStopDistance(StopPtr from, StopPtr to, int distance);
		int GetStopDistance(StopPtr from, StopPtr to) const;

		void AddRoute(std::string_view name, const std::vector<std::string> & stops,
			bool is_looped);
		std::optional<BusPtr> FindRoute(std::string_view name) const;
		RouteInfo GetRouteInfo(BusPtr bus) const;
		std::vector<BusPtr> GetAllRoutes() const;

	private:
		std::deque<Stop> stops_storage;
		std::deque<Bus> buses_storage;
		std::unordered_map<std::string_view, Stop*> stops;
		std::unordered_map<std::string_view, Bus*> buses;

		struct PairStopsHasher {
			size_t operator()(const std::pair<StopPtr, StopPtr>& stops) const;
		};
		std::unordered_map<std::pair<StopPtr, StopPtr>, int, PairStopsHasher> stop_distances;

		unsigned GetUniqueStops(BusPtr bus) const;
		double GetCoordinateRouteLength(BusPtr bus) const;
		uint64_t GetRealRouteLength(BusPtr bus) const;
	};
}
