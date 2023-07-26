#pragma once
#include "json.h"

namespace json {

	class ArrayContext; // внутри Array
	class DictContext; // в Dict сразу после вызова StartDict или очередного Value
	class KeyContext; // в Dict после вызова Key

	class Builder {
	public:
		Builder() = default;

		ArrayContext StartArray();
		Builder& EndArray();
		DictContext StartDict();
		Builder& EndDict();
		KeyContext Key(const std::string &);
		Builder& Value(const Node & value);

		Node Build();

	private:
		Node root_;
		std::vector<Node*> nodes_stack_;
		bool is_empty_ = true;
		std::string last_key_ = "";
		bool has_key_ = false;

		void CheckInsertExeption();
		void InsertNode(Node node, bool need_put_at_stack);
	};

	class BaseContext {
	public:
		BaseContext(Builder& b) : b_(b){}

		ArrayContext StartArray();
		Builder& EndArray();
		DictContext StartDict();
		Builder& EndDict();
		KeyContext Key(const std::string & str);
		BaseContext Value(const Node & value);
	private:
		Builder & b_;
	};

	class ArrayContext: public BaseContext {
	public:
		ArrayContext (BaseContext base) : BaseContext(base) {}

		Builder& EndDict() = delete;
		DictContext Key(std::string) = delete;
		ArrayContext Value(const Node & value) {
			return BaseContext::Value(value);
		}
	};

	class DictContext: public BaseContext {
	public:
		DictContext (BaseContext base) : BaseContext(base) {}

		ArrayContext StartArray() = delete;
		Builder& EndArray() = delete;
		DictContext StartDict() = delete;
		BaseContext Value(const Node & value) = delete;
	};

	class KeyContext: public BaseContext {
	public:
		KeyContext (BaseContext base) : BaseContext(base) {}

		Builder& EndArray() = delete;
		Builder& EndDict() = delete;
		KeyContext Key(std::string) = delete;
		DictContext Value(const Node & value) {
			return BaseContext::Value(value);
		}
	};
}
