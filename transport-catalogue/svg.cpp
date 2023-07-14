#include "svg.h"

namespace svg {

	using namespace std::literals;

	Rgb::Rgb(unsigned r, unsigned g, unsigned b)
		: red(static_cast<uint8_t>(r)),
		green(static_cast<uint8_t>(g)),
		blue(static_cast<uint8_t>(b)) {}

	Rgba::Rgba(unsigned r, unsigned g, unsigned b, double a)
		: red(static_cast<uint8_t>(r)),
		green(static_cast<uint8_t>(g)),
		blue(static_cast<uint8_t>(b)),
		opacity(a) {}

	struct ColorPrinter {
		std::ostream& out;

		void operator()(std::monostate) const {
			out << NoneColor;
		}
		void operator()(std::string str) const {
			out << str;
		}
		void operator()(Rgb rgb) const {
			out << "rgb("sv << static_cast<unsigned int>(rgb.red) << ','
			<< static_cast<unsigned int>(rgb.green) << ','
			<< static_cast<unsigned int>(rgb.blue) << ")"sv;
		}
		void operator()(Rgba rgba) const {
			out << "rgba("sv << static_cast<unsigned int>(rgba.red) << ','
				<< static_cast<unsigned int>(rgba.green) << ','
				<< static_cast<unsigned int>(rgba.blue) << ','
				<< rgba.opacity << ")"sv;
		}
	};

	std::ostream & operator << (std::ostream & out, const Color & color) {
		std::visit(ColorPrinter{out}, color);
		return out;
	}

	std::ostream & operator << (std::ostream & out, StrokeLineCap linecap) {
		switch (linecap) {
			case StrokeLineCap::BUTT :
				out << "butt"s;
				break;
			case StrokeLineCap::ROUND :
				out << "round"s;
				break;
			case StrokeLineCap::SQUARE :
				out << "square"s;
				break;
		}
		return out;
	}

	std::ostream & operator << (std::ostream & out, StrokeLineJoin linejoin) {
		switch (linejoin) {
			case StrokeLineJoin::ARCS :
				out << "arcs"s;
				break;
			case StrokeLineJoin::BEVEL :
				out << "bevel"s;
				break;
			case StrokeLineJoin::MITER :
				out << "miter"s;
				break;
			case StrokeLineJoin::MITER_CLIP :
				out << "miter-clip"s;
				break;
			case StrokeLineJoin::ROUND :
				out << "round"s;
				break;
		}
		return out;
	}

	void Object::Render(const RenderContext& context) const {
		context.RenderIndent();

		// Делегируем вывод тега своим подклассам
		RenderObject(context);

		context.out << std::endl;
	}

	// ---------- Circle ------------------

	Circle& Circle::SetCenter(Point center)  {
		center_ = center;
		return *this;
	}

	Circle& Circle::SetRadius(double radius)  {
		radius_ = radius;
		return *this;
	}

	void Circle::RenderObject(const RenderContext& context) const {
		auto& out = context.out;
		out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
		out << "r=\""sv << radius_ << "\" "sv;
		RenderAttrs(out);
		out << "/>"sv;
	}

	// ---------- Polyline ------------------

	Polyline& Polyline::AddPoint(Point point) {
		points_.push_back(point);
		return *this;
	}

	void Polyline::RenderObject(const RenderContext& context) const {
		auto& out = context.out;
		out << "<polyline points=\""sv;
		bool is_first = true;
		for (auto point: points_) {
			if (is_first) {
				is_first = false;
			} else {
				out << ' ';
			}
			out << point.x << ',' << point.y;
		}
		out << "\" ";
		RenderAttrs(out);
		out << "/>"sv;
	}

	// ---------- Text ------------------

	Text& Text::SetPosition(Point pos) {
		position_ = pos;
		return *this;
	}

	Text& Text::SetOffset(Point offset) {
		offset_ = offset;
		return *this;
	}

	Text& Text::SetFontSize(uint32_t size) {
		font_size_ = size;
		return *this;
	}

	Text& Text::SetFontFamily(std::string font_family) {
		font_family_ = std::move(font_family);
		return *this;
	}

	Text& Text::SetFontWeight(std::string font_weight){
		font_weight_ = std::move(font_weight);
		return *this;
	}

	Text& Text::SetData(std::string data){
		data_ = std::move(data);
		return *this;
	}

	void Text::RenderObject(const RenderContext& context) const {
		auto& out = context.out;
		out << "<text "sv;
		PrintPlacement(out);
		PrintFontParametrs(out);
		RenderAttrs(out);
		out << ">"sv;
		PrintText(out);
		out << "</text>"sv;
	}

	void Text::PrintPlacement(std::ostream& out) const {
		out << "x=\"" << position_.x << "\" y=\"" << position_.y << "\" "
			<< "dx=\"" << offset_.x << "\" dy=\"" << offset_.y << "\" ";
	}

	void Text::PrintFontParametrs(std::ostream& out) const {
		out << "font-size=\"" << font_size_ << "\" ";
		if (font_family_) {
			out << "font-family=\"" << *font_family_ << "\" ";
		}
		if (font_weight_) {
			out << "font-weight=\"" << *font_weight_ << "\" ";
		}
	}

	void Text::PrintText(std::ostream& out) const {
		for (auto symbol : data_) {
			switch(symbol) {
				case '"' :
					out << "&quot;";
					break;
				case '\'' :
					out << "&apos;";
					break;
				case '>' :
					out << "&gt;";
					break;
				case '<' :
					out << "&lt;";
					break;
				case '&' :
					out << "&amp;";
					break;
				default :
					out << symbol;
					break;
			}
		}
	}

	// ---------- Document ------------------

	void Document::AddPtr(std::unique_ptr<Object>&& obj) {
		objects_.push_back(std::move(obj));
	}

	void Document::Render(std::ostream& out) const {
		out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
		out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
		PrintObjects(out);
		out << "</svg>"sv << std::endl;
	}

	void Document::PrintObjects(std::ostream& out)const {
		RenderContext r(out, 2, 2);
		for (auto & obj : objects_) {
			obj->Render(r);
		}
	}

}  // namespace svg
