#pragma once

#include "request_handler.h"
#include "map_renderer.h"
#include "json.h"
#include "json_builder.h"

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
		void ExtractRouteSettings(const ::json::Document & doc, handler::RouteGroup & router) const;
		void ExtractDateBaseInfo(const ::json::Document & doc, handler::DateBase & router) const;

		svg::Color ExtractColor(const ::json::Node & node) const;

		struct WriteVariant {
			int id;
			::json::Node operator()(handler::Errors) const {
				return ::json::Builder{}
					.StartDict()
						.Key("request_id"s).Value(id)
						.Key("error_message"s).Value("not found"s)
					.EndDict()
					.Build().AsDict();
			}
			::json::Node operator()(std::vector<std::string_view> buses) const {
				auto builder = json::Builder{};
				auto buses_result = builder.StartArray();
				for (std::string_view bus : buses) {
					buses_result.Value(::json::Node(std::string(bus)));
				}

				return ::json::Builder{}
					.StartDict()
						.Key("request_id"s).Value(id)
						.Key("buses"s).Value(buses_result.EndArray().Build().AsArray())
					.EndDict()
					.Build().AsDict();
			}
			::json::Node operator()(TransportCatalogue::RouteInfo route) const {
				return ::json::Builder{}
					.StartDict()
						.Key("request_id"s).Value(id)
						.Key("curvature"s).Value(route.curvature)
						.Key("route_length"s).Value(static_cast<double>(route.length))
						.Key("stop_count"s).Value(route.stops_count)
						.Key("unique_stop_count"s).Value(route.unique_stops_count)
					.EndDict()
					.Build().AsDict();
			}
			::json::Node operator()(std::string str) const {
				return ::json::Builder{}
					.StartDict()
						.Key("request_id"s).Value(id)
						.Key("map"s).Value(std::move(str))
					.EndDict()
					.Build().AsDict();
			}
			::json::Node operator()(TransportRouter::Route route) const {
				auto builder = json::Builder{};
				auto route_result = builder.StartArray();
				for (TransportRouter::Segment item : route.items) {
					if (item.type == TransportRouter::SegmentType::WAIT) {
						route_result.StartDict()
							.Key("type"s).Value("Wait"s)
							.Key("stop_name"s).Value(std::string(item.name))
							.Key("time"s).Value(item.time)
						.EndDict();
					} else if (item.type == TransportRouter::SegmentType::BUS) {
						route_result.StartDict()
							.Key("type"s).Value("Bus"s)
							.Key("bus"s).Value(std::string(item.name))
							.Key("span_count"s).Value(item.span_count)
							.Key("time"s).Value(item.time)
						.EndDict();
					}
				}

				return ::json::Builder{}
					.StartDict()
						.Key("request_id"s).Value(id)
						.Key("total_time"s).Value(route.total_time)
						.Key("items"s).Value(route_result.EndArray().Build().AsArray())
					.EndDict()
					.Build().AsDict();
			}
		};
	};
}
