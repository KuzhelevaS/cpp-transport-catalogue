#include "json_reader.h"


namespace transport::json_reader {
	using namespace ::json;

	Reader::Reader(std::istream & in, std::ostream & out)
		: in_(in), out_(out) {}

	handler::InputResultGroup Reader::Read() const {
		Document doc = Load(in_);
		handler::InputResultGroup result;

		ExtractBaseRequest(doc, result.inputs);
		ExtractStatRequest(doc, result.outputs);
		ExtractRenderSettings(doc, result.render_settings);
		ExtractRouteSettings(doc, result.router);
		ExtractDateBaseInfo(doc, result.data_base);

		return result;
	}

	void Reader::ExtractBaseRequest(const Document & doc, handler::InputGroup & inputs) const {
		if (doc.GetRoot().AsDict().count("base_requests"s)) {
			for (auto request : doc.GetRoot().AsDict().at("base_requests"s).AsArray()) {
				if (request.AsDict().at("type"s).AsString() == "Stop"s) {
					std::unordered_map<std::string, int> distances;
					for (auto [road, distance] : request.AsDict().at("road_distances"s).AsDict()) {
						distances[road] = distance.AsDouble();
					}
					handler::Stop stop {
						{request.AsDict().at("latitude"s).AsDouble(),
						request.AsDict().at("longitude"s).AsDouble()},
						distances
					};
					inputs.stops.emplace(request.AsDict().at("name"s).AsString(), stop);
				} else if (request.AsDict().at("type"s).AsString() == "Bus"s) {
					std::vector<std::string> stops;
					for (auto & stop : request.AsDict().at("stops"s).AsArray()) {
						stops.push_back(std::move(stop.AsString()));
					}
					inputs.buses.emplace(request.AsDict().at("name"s).AsString(),
						handler::Route {request.AsDict().at("is_roundtrip"s).AsBool(), std::move(stops)});
				}
			}
		}
	}

	void Reader::ExtractStatRequest(const Document & doc, handler::OutputGroup & outputs) const {
		if (doc.GetRoot().AsDict().count("stat_requests"s)) {
			for (auto request : doc.GetRoot().AsDict().at("stat_requests"s).AsArray()) {
				handler::Query query;
				if (request.AsDict().at("type"s).AsString() == "Stop"s) {
					query.type = handler::QueryType::STOP;
				} else if (request.AsDict().at("type"s).AsString() == "Bus"s) {
					query.type = handler::QueryType::BUS;
				} else if (request.AsDict().at("type"s).AsString() == "Map"s) {
					query.type = handler::QueryType::MAP;
				} else if (request.AsDict().at("type"s).AsString() == "Route"s) {
					query.type = handler::QueryType::ROUTE;
				} else {
					continue;
				}
				query.id = request.AsDict().at("id"s).AsInt();
				if (request.AsDict().count("name"s)) {
					query.name = request.AsDict().at("name"s).AsString();
				}
				if (request.AsDict().count("from"s)) {
					query.from = request.AsDict().at("from"s).AsString();
				}
				if (request.AsDict().count("to"s)) {
					query.to = request.AsDict().at("to"s).AsString();
				}
				outputs.queries.push_back(std::move(query));
			}
		}
	}

	void Reader::ExtractRouteSettings(const Document & doc, handler::RouteGroup & router) const {
		if (doc.GetRoot().AsDict().count("routing_settings"s) && !doc.GetRoot().AsDict().at("routing_settings"s).AsDict().empty()) {
			const ::json::Dict& route_settings = doc.GetRoot().AsDict().at("routing_settings"s).AsDict();
			if (route_settings.count("bus_wait_time"s)) {
				router.settings.bus_wait_time = route_settings.at("bus_wait_time"s).AsInt();
			}
			if (route_settings.count("bus_velocity"s)) {
				router.settings.bus_velocity = route_settings.at("bus_velocity"s).AsDouble();
			}
		}
	}

	void Reader::ExtractDateBaseInfo(const Document & doc, handler::DateBase & date_base) const {
		if (doc.GetRoot().AsDict().count("serialization_settings"s) && !doc.GetRoot().AsDict().at("serialization_settings"s).AsDict().empty()) {
			const ::json::Dict& date_base_settings = doc.GetRoot().AsDict().at("serialization_settings"s).AsDict();
			if (date_base_settings.count("file"s)) {
				date_base.file_name = date_base_settings.at("file"s).AsString();
			}
		}
	}

	void Reader::ExtractRenderSettings(const Document & doc, renderer::Settings & settings) const {
		if (doc.GetRoot().AsDict().count("render_settings"s) && !doc.GetRoot().AsDict().at("render_settings"s).AsDict().empty()) {
			const ::json::Dict& doc_settings = doc.GetRoot().AsDict().at("render_settings"s).AsDict();
			if (doc_settings.count("width"s)) {
				settings.width = doc_settings.at("width"s).AsDouble();
			}
			if (doc_settings.count("height"s)) {
				settings.height = doc_settings.at("height"s).AsDouble();
			}
			if (doc_settings.count("padding"s)) {
				settings.padding = doc_settings.at("padding"s).AsDouble();
			}
			if (doc_settings.count("line_width"s)) {
				settings.line_width = doc_settings.at("line_width"s).AsDouble();
			}
			if (doc_settings.count("stop_radius"s)) {
				settings.stop_radius = doc_settings.at("stop_radius"s).AsDouble();
			}
			if (doc_settings.count("bus_label_font_size"s)) {
				settings.bus_label_font_size = doc_settings.at("bus_label_font_size"s).AsInt();
			}
			if (doc_settings.count("bus_label_offset"s)) {
				settings.bus_label_offset = {
					doc_settings.at("bus_label_offset"s).AsArray().at(0).AsDouble(),
					doc_settings.at("bus_label_offset"s).AsArray().at(1).AsDouble()
				};
			}
			if (doc_settings.count("stop_label_font_size"s)) {
				settings.stop_label_font_size = doc_settings.at("stop_label_font_size"s).AsInt();
			}
			if (doc_settings.count("stop_label_offset"s)) {
				settings.stop_label_offset = {
					doc_settings.at("stop_label_offset"s).AsArray().at(0).AsDouble(),
					doc_settings.at("stop_label_offset"s).AsArray().at(1).AsDouble()
				};
			}
			if (doc_settings.count("underlayer_color"s)) {
				settings.underlayer_color = ExtractColor(doc_settings.at("underlayer_color"s));
			}
			if (doc_settings.count("underlayer_width"s)) {
				settings.underlayer_width = doc_settings.at("underlayer_width"s).AsDouble();
			}
			if (doc_settings.count("color_palette"s)) {
				for (auto & color : doc_settings.at("color_palette"s).AsArray()) {
					settings.color_palette.push_back(std::move(ExtractColor(color)));
				}
			}
		}
	}

	svg::Color Reader::ExtractColor(const ::json::Node & node) const {
		if (node.IsArray()) {
			if (node.AsArray().size() == 3) {
				return svg::Rgb{
					static_cast<uint8_t>(node.AsArray().at(0).AsInt()),
					static_cast<uint8_t>(node.AsArray().at(1).AsInt()),
					static_cast<uint8_t>(node.AsArray().at(2).AsInt())
				};
			} else {
				return svg::Rgba{
					static_cast<uint8_t>(node.AsArray().at(0).AsInt()),
					static_cast<uint8_t>(node.AsArray().at(1).AsInt()),
					static_cast<uint8_t>(node.AsArray().at(2).AsInt()),
					node.AsArray().at(3).AsDouble()
				};
			}
		} else if (node.IsString()) {
			return node.AsString();
		} else {
			return svg::Color{};
		}
	}

	void Reader::Write(const handler::WritingResponces & responces) const {
		auto builder = json::Builder{};
		auto result_builder = builder.StartArray();
		for (auto & responce : responces) {
			result_builder.Value(visit(WriteVariant{responce.first}, responce.second));
		}
		Document doc(result_builder.EndArray().Build().AsArray());
		Print(doc, out_);
	}

}
