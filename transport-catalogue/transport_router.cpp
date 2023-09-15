#include "transport_router.h"

namespace transport {
	void TransportRouter::Init(RouterSettings settings, const TransportCatalogue & catalogue) {
		SetSettings(settings);
		MakeGraph(catalogue);
		router_ = std::make_unique<graph::Router<Weight>>(graph::Router{*graph_});
	}

	void TransportRouter::SetSettings(RouterSettings settings) {
		settings_.bus_wait_time = settings.bus_wait_time;
		constexpr double conversion_rate = 1000.0 / 60.0; // км/ч переводим в м/мин
		settings_.bus_velocity = settings.bus_velocity * conversion_rate;
	}

	void TransportRouter::MakeGraph(const TransportCatalogue & catalogue) {
		graph_ = std::make_unique<Graph>(Graph{static_cast<size_t>(catalogue.GetStopCount())});
		vertex_name_at_id_.resize(catalogue.GetStopCount());
		std::vector<domain::BusPtr> all_routes = catalogue.GetAllRoutes();
		int last_vertex_id = 0;
		for (auto bus_ptr : all_routes) {
			for (int i = 0; i < static_cast<int>(bus_ptr->stops.size()); ++i) {
				int distance = 0;
				domain::StopPtr from_stop = bus_ptr->stops.at(i);
				for (int j = i + 1; j < static_cast<int>(bus_ptr->stops.size()); ++j) {
					domain::StopPtr prev_stop = bus_ptr->stops.at(j-1);
					domain::StopPtr to_stop = bus_ptr->stops.at(j);
					distance += catalogue.GetStopDistance(prev_stop, to_stop);

					if (AddVertex(last_vertex_id, from_stop->name)) {
						++last_vertex_id;
					}
					if (AddVertex(last_vertex_id, to_stop->name)) {
						++last_vertex_id;
					}

					graph_->AddEdge(graph::Edge<Weight>{
						vertex_id_at_name_.at(from_stop->name),
						vertex_id_at_name_.at(to_stop->name),
						Weight{
							bus_ptr->name,
							1.0 * distance / settings_.bus_velocity + settings_.bus_wait_time,
							j - i
						}
					});
				}
			}
		}
	}

	bool TransportRouter::AddVertex(size_t id, std::string_view name) {
		if (!vertex_id_at_name_.count(name)) {
			vertex_id_at_name_[name] = id;
			vertex_name_at_id_.at(id) = name;
			return true;
		}
		return false;
	}

	std::optional<TransportRouter::Route> TransportRouter::GetRoute(
		std::string_view from, std::string_view to) const
	{
		if (!vertex_id_at_name_.count(from) || !vertex_id_at_name_.count(to)) {
			return std::nullopt;
		}

		auto graph_route = router_->BuildRoute(vertex_id_at_name_.at(from),
			vertex_id_at_name_.at(to));

		if (!graph_route.has_value()) {
			return std::nullopt;
		}

		Route result;
		result.total_time = graph_route->weight.time;
		for (auto edge_id : graph_route->edges) {
			auto edge = graph_->GetEdge(edge_id);

			result.items.push_back(Segment{
				SegmentType::WAIT,
				vertex_name_at_id_.at(edge.from),
				static_cast<double>(settings_.bus_wait_time)
			});

			result.items.push_back(Segment{
				SegmentType::BUS,
				edge.weight.name,
				edge.weight.time - settings_.bus_wait_time,
				edge.weight.span_count
			});
		}

		return result;
	}

}
