#include "serialization.h"

#include <transport_catalogue.pb.h>
#include <map_renderer.pb.h>
#include <fstream>

namespace transport::serialization {
	using Path = std::filesystem::path;

	namespace detail {
		void SerializeStops(const std::vector<domain::StopPtr> & stops,
			data_base::TransportCatalogue & result_catalogue)
		{
			for (auto stop : stops) {
				data_base::Stop result_stop;
				result_stop.set_name(stop->name);
				result_stop.set_lat(stop->coordinates.lat);
				result_stop.set_lng(stop->coordinates.lng);
				*result_catalogue.add_stops() = result_stop;
			}
		}

		void SerializeBuses(const std::vector<domain::BusPtr> & buses,
			const std::unordered_map<domain::StopPtr, size_t> & stops_indexes,
			data_base::TransportCatalogue & result_catalogue )
		{
			for (auto bus : buses) {
				data_base::Bus result_bus;
				result_bus.set_name(bus->name);
				result_bus.set_is_looped(bus->is_looped);

				size_t route_size = bus->is_looped ? bus->stops.size() : bus->stops.size() / 2 + 1;
				for (size_t i = 0; i < route_size; ++i) {
					result_bus.add_stops_id(stops_indexes.at(bus->stops.at(i)));
				}

				*result_catalogue.add_buses() = result_bus;
			}
		}

		void SerializeDistances (const std::vector<TransportCatalogue::Distance> & distances,
			const std::unordered_map<domain::StopPtr, size_t> & stops_indexes,
			data_base::TransportCatalogue & result_catalogue)
		{
			for (auto distance : distances) {
				data_base::Distance result_distance;
				result_distance.set_stop_id_from(stops_indexes.at(distance.from_stop));
				result_distance.set_stop_id_to(stops_indexes.at(distance.to_stop));
				result_distance.set_distance(distance.distance);
				*result_catalogue.add_distances() = result_distance;
			}
		}

		void SerializeTransportCatalogue(data_base::TransportCatalogue & result_catalogue,
			const TransportCatalogue& transport_catalogue)
		{
			std::vector<domain::StopPtr> stops(std::move(transport_catalogue.GetAllStops()));
			std::unordered_map<domain::StopPtr, size_t> stops_indexes;
			for (size_t i = 0; i < stops.size(); ++i) {
				stops_indexes[stops.at(i)] = i;
			}
			SerializeStops(stops, result_catalogue);

			std::vector<domain::BusPtr> buses(std::move(transport_catalogue.GetAllRoutes()));
			SerializeBuses(buses, stops_indexes, result_catalogue);

			std::vector<TransportCatalogue::Distance> distances = std::move(transport_catalogue.GetAllDistances());
			std::sort(distances.begin(), distances.end(),
				[&stops_indexes](auto lhs, auto rhs){
					return std::tie(stops_indexes.at(lhs.from_stop), stops_indexes.at(lhs.to_stop))
						< std::tie(stops_indexes.at(rhs.from_stop), stops_indexes.at(rhs.to_stop));
				});
			SerializeDistances (distances, stops_indexes, result_catalogue);
		}

		data_base::Point SerializePoint(svg::Point point) {
			data_base::Point result;
			result.set_x(point.x);
			result.set_y(point.y);
			return result;
		}

		data_base::Color SerializeColor(svg::Color color) {
			data_base::Color result;
			if (std::holds_alternative<std::string>(color)) {
				result.set_type(data_base::ColorType::STRING);
				result.set_name(std::get<std::string>(color));
			} else if (std::holds_alternative<svg::Rgb>(color)) {
				result.set_type(data_base::ColorType::RGB);
				result.set_red(std::get<svg::Rgb>(color).red);
				result.set_green(std::get<svg::Rgb>(color).green);
				result.set_blue(std::get<svg::Rgb>(color).blue);
			} else if (std::holds_alternative<svg::Rgba>(color)) {
				result.set_type(data_base::ColorType::RGBA);
				result.set_red(std::get<svg::Rgba>(color).red);
				result.set_green(std::get<svg::Rgba>(color).green);
				result.set_blue(std::get<svg::Rgba>(color).blue);
				result.set_alpha(std::get<svg::Rgba>(color).opacity);
			} else {
				result.set_type(data_base::ColorType::NONE);
			}
			return result;
		}

		void SerializeRendererSettings(data_base::RenderSettings & result_settings,
			const renderer::Settings & render_settings)
		{
			result_settings.set_width(render_settings.width);
			result_settings.set_height(render_settings.height);
			result_settings.set_padding(render_settings.padding);

			result_settings.set_line_width(render_settings.line_width);
			result_settings.set_stop_radius(render_settings.stop_radius);

			result_settings.set_bus_label_font_size(render_settings.bus_label_font_size);
			*result_settings.mutable_bus_label_offset() = SerializePoint(render_settings.bus_label_offset);

			result_settings.set_stop_label_font_size(render_settings.stop_label_font_size);
			*result_settings.mutable_stop_label_offset() = SerializePoint(render_settings.stop_label_offset);

			*result_settings.mutable_underlayer_color() = SerializeColor(render_settings.underlayer_color);
			result_settings.set_underlayer_width(render_settings.underlayer_width);

			for (auto color : render_settings.color_palette) {
				*result_settings.add_color_palette() = SerializeColor(color);
			}
		}

		void SerializeRouterSettings(data_base::RouterSettings & result_settings,
			const TransportRouter::RouterSettings & router_settings)
		{
			result_settings.set_bus_wait_time(router_settings.bus_wait_time);
			result_settings.set_bus_velocity(router_settings.bus_velocity);
		}

		void DeserializeStops(const data_base::TransportCatalogue & loading_catalogue,
			handler::InputGroup & result)
		{
			for (int i = 0, distance_pos = 0; i < loading_catalogue.stops_size(); ++i) {
				const data_base::Stop & stop = loading_catalogue.stops(i);

				std::unordered_map<std::string, int> distances;
				while (distance_pos < loading_catalogue.distances_size() &&
					loading_catalogue.distances(distance_pos).stop_id_from() == static_cast<uint32_t>(i))
				{
					int to_distance_pos = loading_catalogue.distances(distance_pos).stop_id_to();
					const data_base::Stop & to_stop = loading_catalogue.stops(to_distance_pos);
					distances[to_stop.name()] = loading_catalogue.distances(distance_pos).distance();
					++distance_pos;
				}

				result.stops[stop.name()] = {
					geo::Coordinates{stop.lat(), stop.lng()},
					distances };
			}
		}

		void DeserializeBuses(const data_base::TransportCatalogue & loading_catalogue,
			handler::InputGroup & result)
		{
			for (int i = 0; i < loading_catalogue.buses_size(); ++i) {
				const data_base::Bus & bus = loading_catalogue.buses(i);

				std::vector<std::string> stops;
				for(int i = 0; i < bus.stops_id_size(); ++i) {
					stops.push_back(loading_catalogue.stops(bus.stops_id(i)).name());
				}

				result.buses[bus.name()] = {
					bus.is_looped(),
					std::move(stops) };
			}
		}

		void DeserializeTransportCatalogue(const data_base::TransportCatalogue & loading_catalogue, handler::InputGroup & result) {
			DeserializeStops(loading_catalogue, result);
			DeserializeBuses(loading_catalogue, result);
		}

		svg::Color DeserializeColor(data_base::Color color) {
			switch (color.type()) {
				case data_base::ColorType::STRING :
					return color.name();
				case data_base::ColorType::RGB :
					return svg::Rgb(color.red(), color.green(), color.blue());
				case data_base::ColorType::RGBA :
					return svg::Rgba(color.red(), color.green(), color.blue(), color.alpha());
				case data_base::ColorType::NONE:
				default:
					return svg::Color{};
			}
		}

		void DeserializeRenderSettings(const data_base::RenderSettings& loading_render_settings, renderer::Settings & result) {
			result.width = loading_render_settings.width();
			result.height = loading_render_settings.height();
			result.padding = loading_render_settings.padding();

			result.line_width = loading_render_settings.line_width();
			result.stop_radius = loading_render_settings.stop_radius();

			result.bus_label_font_size = loading_render_settings.bus_label_font_size();
			result.bus_label_offset = {loading_render_settings.bus_label_offset().x(),
				loading_render_settings.bus_label_offset().y()};

			result.stop_label_font_size = loading_render_settings.stop_label_font_size();
			result.stop_label_offset = {loading_render_settings.stop_label_offset().x(),
				loading_render_settings.stop_label_offset().y()};

			result.underlayer_color = DeserializeColor(loading_render_settings.underlayer_color());
			result.underlayer_width = loading_render_settings.underlayer_width();

			for (int i = 0; i < loading_render_settings.color_palette_size(); ++i) {
				result.color_palette.push_back(DeserializeColor(loading_render_settings.color_palette(i)));
			}
		}

		void DeserializeRouterSettings(const data_base::RouterSettings & loading_router_settings,
			TransportRouter::RouterSettings & router_settings)
		{
			router_settings.bus_wait_time = loading_router_settings.bus_wait_time();
			router_settings.bus_velocity = loading_router_settings.bus_velocity();
		}
	}

	void Serialize(Path file_name, const TransportCatalogue& transport_catalogue, const handler::InputResultGroup & settings) {
		std::ofstream output(file_name, std::ios::binary);
		data_base::GeneralMessage result;

		detail::SerializeTransportCatalogue(*result.mutable_transport_catalogue(), transport_catalogue);
		detail::SerializeRendererSettings(*result.mutable_render_settings(), settings.render_settings);
		detail::SerializeRouterSettings(*result.mutable_router_settings(), settings.router.settings);

		result.SerializeToOstream(&output);
	}

	std::optional<handler::InputResultGroup> Deserialize(Path file_name) {
		std::ifstream input(file_name, std::ios_base::in | std::ios::binary);

		data_base::GeneralMessage loading;
		if (!loading.ParseFromIstream(&input)) {
			return std::nullopt;
		}

		handler::InputResultGroup result;

		detail::DeserializeTransportCatalogue(*loading.mutable_transport_catalogue(), result.inputs);
		detail::DeserializeRenderSettings(*loading.mutable_render_settings(), result.render_settings);
		detail::DeserializeRouterSettings(*loading.mutable_router_settings(), result.router.settings);

		return result;
	}
}
