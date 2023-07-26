#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json {

	class Node;
	// Сохраните объявления Dict и Array без изменения
	using Dict = std::map<std::string, Node>;
	using Array = std::vector<Node>;

	// Эта ошибка должна выбрасываться при ошибках парсинга JSON
	class ParsingError : public std::runtime_error {
	public:
		using runtime_error::runtime_error;
	};

	class Node {
	public:
		using Data = std::variant<std::nullptr_t, bool, int, double, std::string, Array, Dict>;

		Node() = default;
		Node(std::nullptr_t null);
		Node(int value);
		Node(double value);
		Node(bool value);
		Node(std::string value);
		Node(Array array);
		Node(Dict map);

		bool IsInt() const;
		bool IsDouble() const;
		bool IsPureDouble() const;
		bool IsBool() const;
		bool IsString() const;
		bool IsNull() const;
		bool IsArray() const;
		bool IsDict() const;

		int AsInt() const;
		bool AsBool() const;
		double AsDouble() const;
		const std::string& AsString() const;
		const Array& AsArray() const;
		Array& AsArray();
		const Dict& AsDict() const;
		Dict& AsDict();

		const Data& GetValue() const;

		bool operator== (const Node & other) const;
		bool operator!= (const Node & other) const;

	private:
		Data data_;
	};

	class Document {
	public:
		explicit Document(Node root);

		const Node& GetRoot() const;

		bool operator== (const Document & other) const;
		bool operator!= (const Document & other) const;

	private:
		Node root_;
	};

	Document Load(std::istream& input);

	void Print(const Document& doc, std::ostream& output);

}  // namespace json
