#include "map_renderer.h"
#include <algorithm>
#include <sstream>

namespace transport::renderer {
	using namespace std::literals;

	namespace detail {
		template <typename Ptr>
		void DrawLayer(const std::map<std::string_view, Ptr>& container, svg::ObjectContainer& target) {
			for (auto & [key, drawable] : container) {
				drawable->Draw(target);
			}
		}
		template <typename Ptr>
		void DrawLayer(const std::vector<Ptr>& container, svg::ObjectContainer& target) {
			for (auto & drawable : container) {
				drawable->Draw(target);
			}
		}
	}

	svg::Point SphereProjector::operator()(transport::geo::Coordinates coords) const {
		return {
			(coords.lng - min_lon_) * zoom_coeff_ + padding_,
			(max_lat_ - coords.lat) * zoom_coeff_ + padding_
		};
	}

	Route& Route::SetPathProperties(double stroke_width, svg::Color stroke_color) {
		path_.SetStrokeColor(stroke_color)
			.SetStrokeWidth(stroke_width)
			.SetFillColor("none"s)
			.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
			.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

		return *this;
	}

	Route& Route::AddStop(svg::Point coordinates) {
		path_.AddPoint(coordinates);
		return *this;
	}

	void Route::Draw(svg::ObjectContainer & container) const {
		container.Add(path_);
	}

	Label& Label::SetText(std::string text, int font_size, bool has_font_weight) {
		back_.SetFontSize(font_size)
			.SetData(text)
			.SetFontFamily("Verdana");

		front_.SetFontSize(font_size)
			.SetData(text)
			.SetFontFamily("Verdana");

		if (has_font_weight) {
			back_.SetFontWeight("bold");
			front_.SetFontWeight("bold");
		}

		return *this;
	}

	Label& Label::SetGraphic(double stroke_width, svg::Color back, svg::Color front) {
		front_.SetFillColor(front);

		back_.SetFillColor(back)
			.SetStrokeColor(back)
			.SetStrokeWidth(stroke_width)
			.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
			.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

		return *this;
	}

	Label& Label::SetPosition(svg::Point position, svg::Point offset) {
		back_.SetPosition(position)
			.SetOffset(offset);

		front_.SetPosition(position)
			.SetOffset(offset);

		return *this;
	}

	void Label::Draw(svg::ObjectContainer & container) const {
		container.Add(back_);
		container.Add(front_);
	}

	StopPoint& StopPoint::SetProperties(svg::Point center, double radius) {
		stop_.SetCenter(center)
			.SetRadius(radius)
			.SetFillColor("white");
		return *this;
	}

	void StopPoint::Draw(svg::ObjectContainer & container) const {
		container.Add(stop_);
	}

	void MapRenderer::SetSettings(Settings settings) {
		settings_ = std::move(settings);
	}

	void MapRenderer::Print(std::vector<domain::BusPtr> && buses, std::ostringstream & str) const {
		std::sort(buses.begin(), buses.end(), [](auto lhs, auto rhs){
			return lhs->name < rhs->name;
		});
		SphereProjector proj = MakeProjector(buses);

		std::vector<DrawPtr> routes = std::move(MakeDrawableRoutes(proj, buses));
		std::vector<DrawPtr> route_names = std::move(MakeDrawableRouteNames(proj, buses));
		std::map<std::string_view, DrawPtr> stops = std::move(MakeDrawableStops(proj, buses));
		std::map<std::string_view, DrawPtr> stop_names = std::move(MakeDrawableStopNames(proj, buses));

		svg::Document doc;
		detail::DrawLayer(routes, doc);
		detail::DrawLayer(route_names, doc);
		detail::DrawLayer(stops, doc);
		detail::DrawLayer(stop_names, doc);
		doc.Render(str);
	}

	// Принадлежит классу, т.к. использует приватный параметр settings
	SphereProjector MapRenderer::MakeProjector(const std::vector<domain::BusPtr> & buses) const {
		std::vector<geo::Coordinates> geo_coords;
		for (auto bus : buses) {
			for (auto stop : bus->stops) {
				geo_coords.push_back(stop->coordinates);
			}
		}

		return SphereProjector {
			geo_coords.begin(), geo_coords.end(), settings_.width, settings_.height, settings_.padding
		};
	}

	std::vector<MapRenderer::DrawPtr> MapRenderer::MakeDrawableRoutes (
		const SphereProjector & proj,
		const std::vector<domain::BusPtr> & buses) const
	{
		std::vector<DrawPtr> routes;
		int current_color_num = 0;

		for (auto bus : buses) {
			Route r;
			r.SetPathProperties(settings_.line_width, settings_.color_palette.at(current_color_num));
			for (auto stop : bus->stops) {
				r.AddStop(proj(stop->coordinates));
			}
			if (!bus->stops.empty()) {
				++current_color_num;
				current_color_num %= settings_.color_palette.size();
			}
			routes.push_back(std::make_unique<Route>(r));
		}

		return routes;
	}

	std::vector<MapRenderer::DrawPtr> MapRenderer::MakeDrawableRouteNames(
		const SphereProjector & proj,
		const std::vector<domain::BusPtr> & buses) const
	{
		std::vector<DrawPtr> route_names;
		int current_color_num = 0;

		for (auto bus : buses) {
			if (!bus->stops.empty()) {
				Label label;
				label.SetText(bus->name, settings_.bus_label_font_size, true)
					.SetGraphic(settings_.underlayer_width, settings_.underlayer_color,
						settings_.color_palette.at(current_color_num))
					.SetPosition(proj(bus->start->coordinates),
						settings_.bus_label_offset);
				route_names.push_back(std::make_unique<Label>(label));

				if (bus->start != bus->finish) {
					// Если начальная и конечная остановки не совпадают
					// Добавляем еще одну точку
					label.SetPosition(proj(bus->finish->coordinates),
						settings_.bus_label_offset);
					route_names.push_back(std::make_unique<Label>(label));
				}

				++current_color_num;
				current_color_num %= settings_.color_palette.size();
			}
		}

		return route_names;
	}

	std::map<std::string_view, MapRenderer::DrawPtr> MapRenderer::MakeDrawableStops(
		const SphereProjector & proj,
		const std::vector<domain::BusPtr> & buses) const
	{
		std::map<std::string_view, DrawPtr> stops;

		for (auto bus : buses) {
			for (auto stop : bus->stops) {
				StopPoint sp;
				sp.SetProperties(proj(stop->coordinates), settings_.stop_radius);
				stops.emplace(stop->name, std::make_unique<StopPoint>(sp));
			}
		}

		return stops;
	}

	std::map<std::string_view, MapRenderer::DrawPtr> MapRenderer::MakeDrawableStopNames(
		const SphereProjector & proj,
		const std::vector<domain::BusPtr> & buses) const
	{
		std::map<std::string_view, DrawPtr> stop_names;

		for (auto bus : buses) {
			for (auto stop : bus->stops) {
				Label label;
				label.SetText(stop->name, settings_.stop_label_font_size)
					.SetGraphic(settings_.underlayer_width,
						settings_.underlayer_color)
					.SetPosition(proj(stop->coordinates),
						settings_.stop_label_offset);
				stop_names.emplace(stop->name, std::make_unique<Label>(label));
			}
		}

		return stop_names;
	}
}
