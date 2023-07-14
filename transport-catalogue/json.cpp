#include "json.h"

#include <algorithm>

namespace json {
	using namespace std::literals;
	namespace detail {
		bool CheckMask (std::istream& input, std::string_view mask) {
			char ch;
			size_t counter = 0;
			while (counter < mask.size() && input >> ch) {
				if (ch != mask[counter]) {
					input.putback(ch);
					return false;
				}
				++counter;
			}
			return counter == mask.size();
		}

		Node LoadNull(std::istream& input) {
			if (!CheckMask(input, "null")) {
				throw ParsingError("Failed to read NULL from stream. Broken mask"s);
			}
			return nullptr;
		}

		Node LoadBool(std::istream& input) {
			char ch = input.peek();
			if (ch == 't') {
				if (!CheckMask(input, "true")) {
					throw ParsingError("Failed to read Bool from stream. Broken true mask"s);
				}
				return true;
			} else {
				if (!CheckMask(input, "false")) {
					throw ParsingError("Failed to read Bool from stream. Broken false mask"s);
				}
				return false;
			}
		}

		Node LoadNumber(std::istream& input) {
			using namespace std::literals;

			std::string parsed_num;

			// Считывает в parsed_num очередной символ из input
			auto read_char = [&parsed_num, &input] {
				parsed_num += static_cast<char>(input.get());
				if (!input) {
					throw ParsingError("Failed to read number from stream"s);
				}
			};

			// Считывает одну или более цифр в parsed_num из input
			auto read_digits = [&input, read_char] {
				if (!std::isdigit(input.peek())) {
					throw ParsingError("A digit is expected"s);
				}
				while (std::isdigit(input.peek())) {
					read_char();
				}
			};

			if (input.peek() == '-') {
				read_char();
			}
			// Парсим целую часть числа
			if (input.peek() == '0') {
				read_char();
				// После 0 в JSON не могут идти другие цифры
			} else {
				read_digits();
			}

			bool is_int = true;
			// Парсим дробную часть числа
			if (input.peek() == '.') {
				read_char();
				read_digits();
				is_int = false;
			}

			// Парсим экспоненциальную часть числа
			if (int ch = input.peek(); ch == 'e' || ch == 'E') {
				read_char();
				if (ch = input.peek(); ch == '+' || ch == '-') {
					read_char();
				}
				read_digits();
				is_int = false;
			}

			try {
				if (is_int) {
					// Сначала пробуем преобразовать строку в int
					try {
						return std::stoi(parsed_num);
					} catch (...) {
						// В случае неудачи, например, при переполнении,
						// код ниже попробует преобразовать строку в double
					}
				}
				return std::stod(parsed_num);
			} catch (...) {
				throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
			}
		}

		Node LoadSimpleType(std::istream& input) {
			char ch = input.peek();
			if (ch == 'n') {
				return LoadNull(input);
			} else if (ch == 't' || ch == 'f') {
				return LoadBool(input);
			} else {
				return LoadNumber(input);
			}
		}

		// Считывает содержимое строкового литерала JSON-документа
		// Функцию следует использовать после считывания открывающего символа ":
		Node LoadString(std::istream& input) {
			using namespace std::literals;

			auto it = std::istreambuf_iterator<char>(input);
			auto end = std::istreambuf_iterator<char>();
			std::string s;
			while (true) {
				if (it == end) {
					// Поток закончился до того, как встретили закрывающую кавычку?
					throw ParsingError("String parsing error");
				}
				const char ch = *it;
				if (ch == '"') {
					// Встретили закрывающую кавычку
					++it;
					break;
				} else if (ch == '\\') {
					// Встретили начало escape-последовательности
					++it;
					if (it == end) {
						// Поток завершился сразу после символа обратной косой черты
						throw ParsingError("String parsing error");
					}
					const char escaped_char = *(it);
					// Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
					switch (escaped_char) {
						case 'n':
							s.push_back('\n');
							break;
						case 't':
							s.push_back('\t');
							break;
						case 'r':
							s.push_back('\r');
							break;
						case '"':
							s.push_back('"');
							break;
						case '\\':
							s.push_back('\\');
							break;
						default:
							// Встретили неизвестную escape-последовательность
							throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
					}
				} else if (ch == '\n' || ch == '\r') {
					// Строковый литерал внутри- JSON не может прерываться символами \r или \n
					throw ParsingError("Unexpected end of line"s);
				} else {
					// Просто считываем очередной символ и помещаем его в результирующую строку
					s.push_back(ch);
				}
				++it;
			}

			return s;
		}

		Node LoadNode(std::istream& input);

		Node LoadDict(std::istream& input) {
			Dict result;

			char ch;
			while (input >> ch && ch != '}') {
				if (ch == ',') {
					input >> ch;
				}

				std::string key = LoadString(input).AsString();
				input >> ch;
				result.insert({std::move(key), LoadNode(input)});

				char next;
				input >> next;
				input.putback(next);
				if (next != ',' && next != '}') {
					throw ParsingError("Failed to read Map from stream. Missed comma or closed bracket."s);
				}
			}

			if (ch != '}') {
				throw ParsingError("Failed to read Map from stream. Missed closed bracket."s);
			}

			return Node(std::move(result));
		}

		Node LoadArray(std::istream& input) {
			Array result;

			char ch;
			while (input >> ch && ch != ']') {
				if (ch != ',') {
					input.putback(ch);
				}
				result.push_back(LoadNode(input));

				char next;
				input >> next;
				input.putback(next);
				if (next != ',' && next != ']') {
					throw ParsingError("Failed to read Array from stream. Missed comma or closed bracket."s);
				}
			}

			if (ch != ']') {
				throw ParsingError("Failed to read Array from stream. Missed closed bracket."s);
			}

			return Node(std::move(result));
		}

		Node LoadNode(std::istream& input) {
			char ch;
			input >> ch;

			if (ch == '[') {
				return LoadArray(input);
			} else if (ch == '{') {
				return LoadDict(input);
			} else if (ch == '"') {
				return LoadString(input);
			} else {
				input.unget();
				return LoadSimpleType(input);
			}
		}

		bool CorrectEndOfStream(std::istream& input) {
			char ch;
			if (input >> ch) {
				return false;
			}
			return true;
		}

		Node LoadDocument(std::istream& input) {
			Node result = LoadNode(input);

			if(!CorrectEndOfStream(input)) {
				throw ParsingError("After read json expected symbol(s)"s);
			}

			return result;
		}

		struct RenderContext {
			RenderContext(std::ostream& out)
				: out(out) {
			}

			RenderContext(std::ostream& out, int indent_step, int indent = 0)
				: out(out)
				, indent_step(indent_step)
				, indent(indent) {
			}

			// Создает новый контекст со сдвигом вправо
			RenderContext Indented() const {
				return {out, indent_step, indent + indent_step};
			}

			// Создает новый контекст со сдвигом влево
			RenderContext BackIndented() const {
				return {out, indent_step,
				std::max(0, indent - indent_step)};
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
	}  // namespace

	Node::Node(std::nullptr_t value) : data_(value) {}
	Node::Node(int value) : data_(value) {}
	Node::Node(double value) : data_(value) {}
	Node::Node(bool value) : data_(value) {}
	Node::Node(std::string value) : data_(value) {}
	Node::Node(Array array) : data_(array) {}
	Node::Node(Dict map) : data_(map) {}

	bool Node::IsInt() const {
		return std::holds_alternative<int>(data_);
	}
	bool Node::IsDouble() const {
		return IsPureDouble() || IsInt();
	}
	bool Node::IsPureDouble() const {
		return std::holds_alternative<double>(data_);
	}
	bool Node::IsBool() const {
		return std::holds_alternative<bool>(data_);
	}
	bool Node::IsString() const {
		return std::holds_alternative<std::string>(data_);
	}
	bool Node::IsNull() const {
		return std::holds_alternative<std::nullptr_t>(data_);
	}
	bool Node::IsArray() const {
		return std::holds_alternative<Array>(data_);
	}
	bool Node::IsMap() const {
		return std::holds_alternative<Dict>(data_);
	}

	int Node::AsInt() const {
		if (!IsInt()) {
			throw std::logic_error("node is not int");
		}
		return std::get<int>(data_);
	}
	bool Node::AsBool() const {
		if (!IsBool()) {
			throw std::logic_error("node is not bool");
		}
		return std::get<bool>(data_);
	}
	double Node::AsDouble() const {
		if (!IsDouble()) {
			throw std::logic_error("node is not double");
		}
		if (IsInt()) {
			return static_cast<double>(std::get<int>(data_));
		}
		return std::get<double>(data_);
	}
	const std::string& Node::AsString() const {
		if (!IsString()) {
			throw std::logic_error("node is not string");
		}
		return std::get<std::string>(data_);
	}
	const Array& Node::AsArray() const {
		if (!IsArray()) {
			throw std::logic_error("node is not array");
		}
		return std::get<Array>(data_);
	}
	const Dict& Node::AsMap() const {
		if (!IsMap()) {
			throw std::logic_error("node is not map");
		}
		return std::get<Dict>(data_);
	}

	const Node::Data& Node::GetValue() const {
		return data_;
	}

	bool  Node::operator== (const Node & other) const {
		return this->data_ == other.data_;
	}
	bool  Node::operator!= (const Node & other) const {
		return !(*this == other);
	}

	Document::Document(Node root)
		: root_(std::move(root)) {
	}

	const Node& Document::GetRoot() const {
		return root_;
	}

	bool Document::operator== (const Document & other) const {
		return this->GetRoot() == other.GetRoot();
	}
	bool Document::operator!= (const Document & other) const {
		return !(this->GetRoot() == other.GetRoot());
	}

	Document Load(std::istream& input) {
		return Document{detail::LoadDocument(input)};
	}

	void PrintNode(const Node & node, const detail::RenderContext& context);

	struct NodePrinter {
		const detail::RenderContext& context;

		void operator()(std::nullptr_t) const {
			context.out << "null";
		}
		void operator()(bool b) const {
			context.out << std::boolalpha << b << std::noboolalpha;
		}
		void operator()(int i) const {
			context.out << i;
		}
		void operator()(double d) const {
			context.out << d;
		}
		void operator()(const std::string & str) const {
			context.out << '"';
			for (auto it = str.begin(); it != str.end(); ++it) {
				switch (*it) {
					case '\n' :
						context.out << "\\n";
						break;
					case '\r' :
						context.out << "\\r";
						break;
					case '\"' :
						context.out << "\\\"";
						break;
					case '\\' :
						context.out << "\\\\";
						break;
					case '\t' : // по условию выводим как есть
					default :
						context.out << *it;
						break;
				}
			}
			context.out << '"';
		}
		void operator()(const Array & arr) const {
			context.out << '[';
			bool is_first = true;
			for (auto node: arr) {
				if (is_first) {
					is_first = false;
				} else {
					context.out << ", "s;
				}
				PrintNode(node, context);
			}
			context.out << ']';
		}
		void operator()(const Dict & arr) const {
			context.out << "{";
			bool is_first = true;
			for (auto [key, value]: arr) {
				if (is_first) {
					is_first = false;
				} else {
					context.out << ","s;
				}
				context.out << "\n"s;
				context.RenderIndent();
				context.out << '"' << key << "\": "s;
				PrintNode(value, context.Indented());
			}
			context.out << "\n";
			auto prev_context = context.BackIndented();
			prev_context.RenderIndent();
			context.out << "}";
		}
	};

	void PrintNode(const Node & node, const detail::RenderContext& context) {
		std::visit(NodePrinter{context}, node.GetValue());
	}
	std::ostream & operator << (std::ostream & out, const Node & node) {
		detail::RenderContext context(out, 4, 4);
		PrintNode(node, context);
		return out;
	}

	void Print(const Document& doc, std::ostream& output) {
		output << doc.GetRoot();
	}

}  // namespace json
