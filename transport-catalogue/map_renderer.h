#pragma once

#include "domain.h"
#include "geo.h"
#include "svg.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>
#include <map>

namespace transport::renderer {

	struct Settings {
		double width;
		double height;
		double padding;

		double line_width;
		double stop_radius;

		int bus_label_font_size;
		svg::Point bus_label_offset;

		int stop_label_font_size;
		svg::Point stop_label_offset;

		svg::Color underlayer_color;
		double underlayer_width;

		std::vector<svg::Color> color_palette;
	};

	class SphereProjector {
	public:
		// points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
		template <typename PointInputIt>
		SphereProjector(PointInputIt points_begin, PointInputIt points_end,
			double max_width, double max_height, double padding);

		// Проецирует широту и долготу в координаты внутри SVG-изображения
		svg::Point operator()(transport::geo::Coordinates coords) const;

	private:
		double padding_;
		double min_lon_ = 0;
		double max_lat_ = 0;
		double zoom_coeff_ = 0;
	};

	class Route : public svg::Drawable {
	public:
		Route& SetPathProperties(double stroke_width, svg::Color stroke_color);
		Route& AddStop(svg::Point coordinates);
		void Draw(svg::ObjectContainer & container) const override;
	private:
		svg::Polyline path_;
	};

	class Label : public svg::Drawable {
	public:
		Label& SetText(std::string text, int font_size, bool has_font_weight = false);
		Label& SetGraphic(double stroke_width, svg::Color back, svg::Color front = "black");
		Label& SetPosition(svg::Point coordinates, svg::Point offset);
		void Draw(svg::ObjectContainer & container) const override;
	private:
		svg::Text back_, front_;
	};

	class StopPoint : public svg::Drawable {
	public:
		StopPoint& SetProperties(svg::Point center, double radius);
		void Draw(svg::ObjectContainer & container) const override;
	private:
		svg::Circle stop_;
	};

	class MapRenderer {
	public:
		void SetSettings(Settings settings);
		void Print(std::vector<domain::BusPtr> && buses, std::ostringstream & str) const;
	private:
		Settings settings_;

		using DrawPtr = std::unique_ptr<svg::Drawable>;

		SphereProjector MakeProjector(const std::vector<domain::BusPtr> & buses) const;

		std::vector<DrawPtr> MakeDrawableRoutes(const SphereProjector & proj,
			const std::vector<domain::BusPtr> & buses) const;

		std::vector<DrawPtr> MakeDrawableRouteNames(const SphereProjector & proj,
			const std::vector<domain::BusPtr> & buses) const;

		std::map<std::string_view, DrawPtr> MakeDrawableStops(const SphereProjector & proj,
			const std::vector<domain::BusPtr> & buses) const;

		std::map<std::string_view, DrawPtr> MakeDrawableStopNames(const SphereProjector & proj,
			const std::vector<domain::BusPtr> & buses) const;
	};


	inline const double EPSILON = 1e-6;
	inline bool IsZero(double value) {
		return std::abs(value) < EPSILON;
	}

	template <typename PointInputIt>
	SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end,
		double max_width, double max_height, double padding)
		: padding_(padding)
	{
		// Если точки поверхности сферы не заданы, вычислять нечего
		if (points_begin == points_end) {
			return;
		}

		// Находим точки с минимальной и максимальной долготой
		const auto [left_it, right_it] = std::minmax_element(
			points_begin, points_end,
			[](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
		min_lon_ = left_it->lng;
		const double max_lon = right_it->lng;

		// Находим точки с минимальной и максимальной широтой
		const auto [bottom_it, top_it] = std::minmax_element(
			points_begin, points_end,
			[](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
		const double min_lat = bottom_it->lat;
		max_lat_ = top_it->lat;

		// Вычисляем коэффициент масштабирования вдоль координаты x
		std::optional<double> width_zoom;
		if (!IsZero(max_lon - min_lon_)) {
			width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
		}

		// Вычисляем коэффициент масштабирования вдоль координаты y
		std::optional<double> height_zoom;
		if (!IsZero(max_lat_ - min_lat)) {
			height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
		}

		if (width_zoom && height_zoom) {
			// Коэффициенты масштабирования по ширине и высоте ненулевые,
			// берём минимальный из них
			zoom_coeff_ = std::min(*width_zoom, *height_zoom);
		} else if (width_zoom) {
			// Коэффициент масштабирования по ширине ненулевой, используем его
			zoom_coeff_ = *width_zoom;
		} else if (height_zoom) {
			// Коэффициент масштабирования по высоте ненулевой, используем его
			zoom_coeff_ = *height_zoom;
		}
	}
}
