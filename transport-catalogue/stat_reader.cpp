#include "stat_reader.h"

#include <vector>
#include <string>
#include <iostream>
#include<iomanip>

namespace transport::stat {
	namespace detail {
		enum class QueryType {
			BUS,
			STOP
		};
		struct Query {
			QueryType type;
			std::string item_name;
		};

		std::string_view Trim (std::string_view str) {
			auto trim_begin = std::min(str.find_first_not_of(' '), str.size());
			auto trim_end = std::min(str.find_last_not_of(' '), str.size());
			return str.substr(trim_begin, trim_end - trim_begin + 1);
		}

		std::istream& operator>> (std::istream & is, Query & query) {
			using namespace std::literals;

			std::string type_str;
			is >> type_str;
			std::string query_str;
			std::getline(is, query_str);

			query = {};
			query.item_name = std::string(Trim(query_str));
			if (type_str == "Bus"s) {
				query.type = QueryType::BUS;
			} else if (type_str == "Stop"s) {
				query.type = QueryType::STOP;
			}
			return is;
		}

		struct Entity {
			QueryType type;
			std::string name;
		};

		std::vector<Entity> ParseQueries(std::istream & input, size_t n) {
			std::vector<Entity> result;
			for (size_t i = 0; i < n; ++i) {
				detail::Query query;
				input >> query;
				result.emplace_back(Entity{query.type, std::move(query.item_name)});
			}
			return result;
		}

		void PrintBusInfo(std::ostream & out, std::string_view bus_name, TransportCatalogue::RouteInfo info) {
			out << "Bus " << bus_name << ": "
				<< info.stops_count << " stops on route, "
				<< info.unique_stops_count << " unique stops, "
				<< info.length << " route length, "
				<< std::setprecision(6) << info.curvature << " curvature" << std::endl;
		}

		void PrintBusNotFound(std::ostream & out, std::string_view bus_name) {
			out << "Bus " << bus_name << ": not found" << std::endl;
		}

		void PrintBusesForStop(std::ostream & out, std::string_view stop_name,
			std::vector<std::string_view> buses)
		{
			out << "Stop " << stop_name << ": ";
			if (buses.size() == 0) {
				out << "no buses";
			} else {
				out << "buses";
				for (auto bus : buses) {
					out << ' ' << bus;
				}
			}
			out << std::endl;
		}

		void PrintStopNotFound(std::ostream & out, std::string_view stop_name) {
			out << "Stop " << stop_name << ": not found" << std::endl;
		}
	}

	void ExecuteRequests(const TransportCatalogue & t) {
		int k;
		std::cin >> k;
		auto stat_request_queue = detail::ParseQueries(std::cin, k);

		for (auto & entity : stat_request_queue) {
			if (entity.type == detail::QueryType::BUS) {
				auto route = t.FindRoute(entity.name);
				if (route.has_value()) {
					detail::PrintBusInfo(std::cout, entity.name, t.GetRouteInfo(route.value()));
				} else {
					detail::PrintBusNotFound(std::cout, entity.name);
				}
			} else if (entity.type == detail::QueryType::STOP) {
				auto stop = t.FindStop(entity.name);
				if (stop.has_value()) {
					detail::PrintBusesForStop(std::cout, entity.name, t.GetBusesForStop(stop.value()));
				} else {
					detail::PrintStopNotFound(std::cout, entity.name);
				}
			}
		}
	}
}


