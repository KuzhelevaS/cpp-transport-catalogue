#include "json_builder.h"

namespace json {
	KeyContext Builder::Key(const std::string & key) {
		if (has_key_) {
			throw std::logic_error("Use key after key");
		}
		if (nodes_stack_.empty() || !nodes_stack_.back()->IsDict()) {
			throw std::logic_error("Use key out of dictionary");
		}
		has_key_ = true;
		last_key_ = key;
		return KeyContext(*this);
	}

	Builder& Builder::Value(const Node & value) {
		InsertNode(value, false);
		return *this;
	}

	ArrayContext Builder::StartArray() {
		InsertNode(Array(), true);
		return ArrayContext(*this);
	}

	Builder& Builder::EndArray() {
		if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
			nodes_stack_.pop_back();
		} else {
			throw std::logic_error("Wrong type of closing (array)");
		}
		return *this;
	}

	DictContext Builder::StartDict() {
		InsertNode(Dict(), true);
		return DictContext(*this);
	}

	Builder& Builder::EndDict() {
		if (!nodes_stack_.empty() && nodes_stack_.back()->IsDict()) {
			nodes_stack_.pop_back();
		} else {
			throw std::logic_error("Wrong type of closing (dictionary)");
		}
		return *this;
	}

	Node Builder::Build() {
		if(is_empty_) {
			throw std::logic_error("Node wasn't create");
		}
		if(!nodes_stack_.empty()) {
			throw std::logic_error("Node wasn't closed");
		}
		return root_;
	}

	void Builder::InsertNode(Node node, bool need_put_at_stack) {
		CheckInsertExeption();
		if (is_empty_) {
			root_ = node;
			if (need_put_at_stack) {
				nodes_stack_.push_back(&root_);
			}
		} else {
			Node * inserded = nullptr;
			if (nodes_stack_.back()->IsArray()) {
				nodes_stack_.back()->AsArray().push_back(node);
				inserded = &(nodes_stack_.back()->AsArray().back());
			} else if (nodes_stack_.back()->IsDict()) {
				if (!has_key_) {
					throw std::logic_error("Add value in dictionary without key");
				}
				nodes_stack_.back()->AsDict()[last_key_] = node;
				inserded = &(nodes_stack_.back()->AsDict()[last_key_]);
			}
			if (need_put_at_stack) {
				nodes_stack_.push_back(inserded);
			}
		}
		has_key_ = false;
		is_empty_ = false;
	}

	void Builder::CheckInsertExeption() {
		if ((!is_empty_ && nodes_stack_.empty()) // Еще не было вставок
			|| (!nodes_stack_.empty() // или вставки были, но
				&& !(nodes_stack_.back()->IsArray() // это не массив
					|| nodes_stack_.back()->IsDict()))) // и не словарь
		{
			throw std::logic_error("Insert multiple values outside array or dictionary");
		}
		if (!nodes_stack_.empty() && nodes_stack_.back()->IsDict() && !has_key_) {
			throw std::logic_error("Insert value in the dictionary without key");
		}
	}

	ArrayContext BaseContext::StartArray() {
		return b_.StartArray();
	}
	Builder& BaseContext::EndArray() {
		return b_.EndArray();
	}
	DictContext BaseContext::StartDict() {
		return b_.StartDict();
	}
	Builder& BaseContext::EndDict() {
		return b_.EndDict();
	}
	KeyContext BaseContext::Key(const std::string & str) {
		return b_.Key(str);
	}
	BaseContext BaseContext::Value(const Node & value) {
		b_.Value(value);
		return *this;
	}
}
