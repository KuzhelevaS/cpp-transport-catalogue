#include "input_reader.h"


#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#include "geo.h"

namespace transport::input {
	namespace detail {
		using geo::Coordinates;

		enum class QueryType {
			ADD_BUS,
			ADD_STOP
		};

		struct Query {
			QueryType type;
			std::string item_name;
			std::vector<std::string> stops;
			bool is_route_looped;
			Coordinates coordinates;
			std::unordered_map<std::string, int> distances;
		};

		std::string_view Trim (std::string_view str) {
			auto trim_begin = std::min(str.find_first_not_of(' '), str.size());
			auto trim_end = std::min(str.find_last_not_of(' '), str.size());
			return str.substr(trim_begin, trim_end - trim_begin + 1);
		}

		std::vector<std::string_view> Split(std::string_view str, std::string_view divider_str) {
			std::vector<std::string_view> result;
			while (str.size() > 0) {
				size_t divider = std::min(str.find(divider_str), str.size());
				result.push_back(Trim(str.substr(0, divider)));
				str = str.substr(std::min(divider + divider_str.size(),str.size()), str.size());
			}
			return result;
		}

		template <typename Type>
		Type StringViewToType (std::string_view str) {
			std::istringstream input{std::string(str)};
			Type result;
			input >> result;
			return result;
		}

		std::istream& operator>> (std::istream & is, Query & query) {
			using namespace std::literals;

			//Считываем команду из ближайшей не пустой строки
			std::string type_str;
			is >> type_str;

			//Получаем оставшуюся часть строки
			std::string query_str;
			std::getline(is, query_str);
			std::string_view query_view = query_str;

			query = {};
			size_t divider = query_view.find(':');
			query.item_name = std::string(Trim(query_view.substr(0, divider)));
			query_view = query_view.substr(divider + 1, query_view.size());

			if (type_str == "Stop"sv) {
				query.type = QueryType::ADD_STOP;

				auto stop_data = Split(query_view, ","sv);
				query.coordinates.lat = StringViewToType<double>(stop_data[0]);
				query.coordinates.lng = StringViewToType<double>(stop_data[1]);

				for (size_t i = 2; i < stop_data.size(); ++i) {
					auto distance_divider = stop_data[i].find("to"s);

					std::string_view distance_view = Trim(stop_data[i].substr(0, distance_divider));
					int distance = StringViewToType<int>(distance_view);

					std::string_view stop_name = Trim(stop_data[i].substr(distance_divider + 2));
					query.distances[std::string(stop_name)] = distance;
				}
			} else if (type_str == "Bus"sv) {
				query.type = QueryType::ADD_BUS;

				auto stop_divider_pos = std::find_if(query_view.begin(), query_view.end(),
					[](char symbol){
						return symbol == '>' || symbol == '-';
					});

				std::string_view stop_divider;
				if (*stop_divider_pos == '>') {
					stop_divider = ">"sv;
					query.is_route_looped = true;
				} else {
					stop_divider = "-"sv;
					query.is_route_looped = false;
				}

				auto splitted_stops = Split(query_view, stop_divider);
				query.stops.reserve(splitted_stops.size());
				for (auto stop_view : splitted_stops) {
					query.stops.push_back(std::string(stop_view));
				}
			}
			return is;
		}

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

		ResultGroup ParseQueries(std::istream & input, size_t n) {
			ResultGroup result;
			for (size_t i = 0; i < n; ++i) {
				Query query;
				input >> query;
				if (query.type == QueryType::ADD_STOP) {
					result.stops[query.item_name] = {query.coordinates, query.distances};
				} else if (query.type == QueryType::ADD_BUS) {
					result.buses[query.item_name] = {query.is_route_looped,
						std::move(query.stops)};
				}
			}
			return result;
		}
	}

	void FillCatalogue(TransportCatalogue & t) {
		int n;
		std::cin >> n;
		auto input_request_queue = detail::ParseQueries(std::cin, n);

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
	}
}
