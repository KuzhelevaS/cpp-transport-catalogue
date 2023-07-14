#pragma once

#include "request_handler.h"
#include "map_renderer.h"
#include "json.h"

namespace transport::json_reader {
	using namespace std::literals;

	class Reader : public handler::InputOutput {
	public:
		Reader(std::istream & in = std::cin, std::ostream & out = std::cout);
		handler::InputResultGroup Read() const override;
		void Write(const handler::WritingResponces & responces) const override;
	private:
		std::istream & in_;
		std::ostream & out_;

		void ExtractBaseRequest(const ::json::Document & doc, handler::InputGroup & inputs) const;
		void ExtractStatRequest(const ::json::Document & doc, handler::OutputGroup & outputs) const;
		void ExtractRenderSettings(const ::json::Document & doc, renderer::Settings & settings) const;

		svg::Color ExtractColor(const ::json::Node & node) const;

		struct WriteVariant {
			int id;
			::json::Node operator()(handler::Errors) const {
				return ::json::Dict{{"request_id"s, id}, {"error_message"s, "not found"s}};
			}
			::json::Node operator()(std::vector<std::string_view> buses) const {
				::json::Array result_buses;
				result_buses.reserve(buses.size());
				for (auto bus : buses) {
					result_buses.emplace_back(std::string(bus));
				}
				return ::json::Dict{{"request_id"s, id}, {"buses"s, result_buses}};
			}
			::json::Node operator()(TransportCatalogue::RouteInfo route) const {
				::json::Dict result_route;
				result_route["request_id"] = id;
				result_route["curvature"] = route.curvature;
				result_route["route_length"] = static_cast<double>(route.length);
				result_route["stop_count"] = route.stops_count;
				result_route["unique_stop_count"] = route.unique_stops_count;
				return result_route;
			}
			::json::Node operator()(std::string str) const {
				::json::Dict result_map;
				result_map["request_id"] = id;
				result_map["map"] = std::move(str);
				return result_map;
			}
		};
	};
}
