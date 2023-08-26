#pragma once

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include <memory>

namespace transport {
	class TransportRouter {
	private:
		struct Weight {
			std::string_view name;
			double time = 0;
			int span_count = 0;

			bool operator < (Weight rhs) const {
				return time < rhs.time;
			}

			bool operator > (Weight rhs) const {
				return rhs.time < time;
			}

			Weight operator + (Weight rhs) const {
				Weight result = *this;
				result.time += rhs.time;
				return result;
			}
		};

	public:
		enum class SegmentType {
			WAIT,
			BUS
		};
		struct Segment {
			SegmentType type = SegmentType::BUS;
			std::string_view name;
			double time = 0;
			int span_count = 0;
		};
		struct Route {
			double total_time;
			std::vector<Segment> items;
		};

		struct RouterSettings {
			int bus_wait_time = 0; // минуты
			double bus_velocity = 0; // метры в минуту
		};
		TransportRouter() = default;
		void Init(RouterSettings settings, const TransportCatalogue & catalogue);
		std::optional<Route> GetRoute(std::string_view from, std::string_view to) const;
	private:
		std::unordered_map<std::string_view, size_t> vertex_id_at_name_;
		std::vector<std::string_view> vertex_name_at_id_; // ключ - индекс в массиве
		RouterSettings settings_;

		using Graph = graph::DirectedWeightedGraph<Weight>;
		using GraphPtr = std::unique_ptr<Graph>;
		using RouterPtr = std::unique_ptr<graph::Router<Weight>>;
		GraphPtr graph_ = nullptr;
		RouterPtr router_ = nullptr;

		void MakeGraph(const TransportCatalogue & catalogue);
		bool AddVertex(size_t id, std::string_view name);
		void SetSettings(RouterSettings settings);
	};

}
