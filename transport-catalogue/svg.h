#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace svg {
	using namespace std::literals;

	struct Rgb {
		Rgb() = default;
		Rgb(unsigned r, unsigned g, unsigned b);

		uint8_t red = 0u;
		uint8_t green = 0u;
		uint8_t blue = 0u;
	};

	struct Rgba {
		Rgba() = default;
		Rgba(unsigned r, unsigned g, unsigned b, double a);

		uint8_t red = 0u;
		uint8_t green = 0u;
		uint8_t blue = 0u;
		double opacity = 1.0;
	};

	using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
	inline const Color NoneColor{"none"};

	std::ostream & operator << (std::ostream & out, const Color & color);

	enum class StrokeLineCap {
		BUTT,
		ROUND,
		SQUARE
	};

	std::ostream & operator << (std::ostream & out, StrokeLineCap linecap);

	enum class StrokeLineJoin {
		ARCS,
		BEVEL,
		MITER,
		MITER_CLIP,
		ROUND
	};

	std::ostream & operator << (std::ostream & out, StrokeLineJoin linejoin);

	template <typename Owner>
	class PathProps {
	public:
		// Задает цвет заливки
		Owner& SetFillColor(Color color) {
			fill_color_ = std::move(color);
			return AsOwner();
		}

		// Задает цвет обводки
		Owner& SetStrokeColor(Color color) {
			stroke_color_ = std::move(color);
			return AsOwner();
		}

		// Задает толщину обводки
		Owner& SetStrokeWidth(double width) {
			stroke_width_ = width;
			return AsOwner();
		}

		// Задает форму конца линии
		Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
			stroke_line_cap_ = line_cap;
			return AsOwner();
		}

		// Задает форму соединения линий
		Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
			stroke_line_join_ = line_join;
			return AsOwner();
		}

	protected:
		~PathProps() = default;

		void RenderAttrs(std::ostream& out) const {
			using namespace std::literals;

			if (fill_color_) {
				out << "fill=\""sv << *fill_color_ << "\" "sv;
			}
			if (stroke_color_) {
				out << "stroke=\""sv << *stroke_color_ << "\" "sv;
			}
			if (stroke_width_) {
				out << "stroke-width=\""sv << *stroke_width_ << "\" "sv;
			}
			if (stroke_line_cap_) {
				out << "stroke-linecap=\""sv << *stroke_line_cap_ << "\" "sv;
			}
			if (stroke_line_join_) {
				out << "stroke-linejoin=\""sv << *stroke_line_join_ << "\" "sv;
			}
		}

	private:
		std::optional<Color> fill_color_;
		std::optional<Color> stroke_color_;
		std::optional<double> stroke_width_;
		std::optional<StrokeLineCap> stroke_line_cap_;
		std::optional<StrokeLineJoin> stroke_line_join_;

		Owner& AsOwner() {
			// static_cast безопасно преобразует *this к Owner&,
			// если класс Owner — наследник PathProps
			return static_cast<Owner&>(*this);
		}
	};

	struct Point {
		Point() = default;
		Point(double x, double y)
			: x(x)
			, y(y) {
		}
		double x = 0;
		double y = 0;
	};

	// Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
	// Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
	struct RenderContext {
		RenderContext(std::ostream& out)
			: out(out) {
		}

		RenderContext(std::ostream& out, int indent_step, int indent = 0)
			: out(out)
			, indent_step(indent_step)
			, indent(indent) {
		}

		// Создает новый контекст со сдвигом
		RenderContext Indented() const {
			return {out, indent_step, indent + indent_step};
		}

		// Печатает сдвиг
		void RenderIndent() const {
			for (int i = 0; i < indent; ++i) {
				out.put(' ');
			}
		}

		std::ostream& out;
		int indent_step = 0;
		int indent = 0;
	};

	// Абстрактный базовый класс Object служит для унифицированного хранения
	// конкретных тегов SVG-документа
	// Реализует паттерн "Шаблонный метод" для вывода содержимого тега
	class Object {
	public:
		void Render(const RenderContext& context) const;
		virtual ~Object() = default;
	private:
		virtual void RenderObject(const RenderContext& context) const = 0;
	};

	// Класс Circle моделирует элемент <circle> для отображения круга
	// https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
	class Circle final : public Object, public PathProps<Circle> {
	public:
		// Задает центр окружности
		Circle& SetCenter(Point center);
		// Задает радиус окружности
		Circle& SetRadius(double radius);

	private:
		Point center_;
		double radius_ = 1.0;

		void RenderObject(const RenderContext& context) const override;
	};

	// Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
	// https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
	class Polyline final : public Object, public PathProps<Polyline> {
	public:
		// Добавляет очередную вершину к ломаной линии
		Polyline& AddPoint(Point point);

	private:
		std::vector<Point> points_;

		void RenderObject(const RenderContext& context) const override;
	};

	// Класс Text моделирует элемент <text> для отображения текста
	// https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
	class Text final : public Object, public PathProps<Text> {
	public:
		// Задаёт координаты опорной точки (атрибуты x и y)
		Text& SetPosition(Point pos);

		// Задаёт смещение относительно опорной точки (атрибуты dx, dy)
		Text& SetOffset(Point offset);

		// Задаёт размеры шрифта (атрибут font-size)
		Text& SetFontSize(uint32_t size);

		// Задаёт название шрифта (атрибут font-family)
		Text& SetFontFamily(std::string font_family);

		// Задаёт толщину шрифта (атрибут font-weight)
		Text& SetFontWeight(std::string font_weight);

		// Задаёт текстовое содержимое объекта (отображается внутри тега text)
		Text& SetData(std::string data);

	private:
		Point position_;
		Point offset_;
		uint32_t font_size_ = 1;
		std::optional<std::string> font_family_;
		std::optional<std::string> font_weight_;
		std::string data_;

		void RenderObject(const RenderContext& context) const override;
		void PrintPlacement(std::ostream& out) const;
		void PrintFontParametrs(std::ostream& out) const;
		void PrintText(std::ostream& out) const;
	};

	class ObjectContainer {
	public:
		virtual ~ObjectContainer() = default;

		// Метод Add добавляет в svg-документ любой объект-наследник svg::Object.
		template <typename Obj>
		void Add(const Obj & obj) {
			AddPtr(std::make_unique<Obj>(obj));
		}

		// Добавляет в svg-документ объект-наследник svg::Object
		virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
	};

	class Document : public ObjectContainer {
	public:
		void AddPtr(std::unique_ptr<Object>&& obj) override;

		// Выводит в ostream svg-представление документа
		void Render(std::ostream& out) const;

	private:
		std::vector<std::unique_ptr<Object>> objects_;
		void PrintObjects(std::ostream& out)const;
	};

	class Drawable {
	public:
		virtual ~Drawable() = default;
		virtual void Draw(ObjectContainer & g) const = 0;
	};

}  // namespace svg
