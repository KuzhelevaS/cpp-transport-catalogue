#include "request_handler.h"
#include "serialization.h"
#include <iostream>
#include <sstream>

namespace transport::handler {
	RequestHandler::RequestHandler(InputOutput* io)
		: io_(io) {}

	void RequestHandler::MakeBase() {
		auto queries = io_->Read();
		FillTransportCatalogue( queries.inputs);
		serialization::Serialize(queries.data_base.file_name, tc_, queries);
	}

	void RequestHandler::ProcessRequests() {
		auto queries = io_->Read();

		auto settings = serialization::Deserialize(queries.data_base.file_name);
		if (settings.has_value()) {
			renderer_.SetSettings(settings->render_settings);
			FillTransportCatalogue(settings->inputs);
			router_.Init(settings->router.settings, tc_);

			auto printable_result = GetTransportData(queries.outputs);
			io_->Write(printable_result);
		}

	}

	std::string RequestHandler::GenerateMap() {
		auto buses = tc_.GetAllRoutes();
		std::ostringstream out;
		renderer_.Print(std::move(buses), out);
		return out.str();
	}

	void RequestHandler::FillTransportCatalogue(const InputGroup & inputs) {
		for (auto & [stop_name, stop_data] : inputs.stops) {
			tc_.AddStop(stop_name, stop_data.coordinates);
		}

		for (auto & [stop_name, stop_data] : inputs.stops) {
			for (auto & [other_name, distance] : stop_data.distances) {
				auto stop_from = tc_.FindStop(stop_name);
				auto stop_to = tc_.FindStop(other_name);
				tc_.SetStopDistance(stop_from.value(), stop_to.value(), distance);
			}
		}

		for (auto & [bus_name, route] : inputs.buses) {
			tc_.AddRoute(bus_name, route.stops, route.is_looped);
		}
	}

	WritingResponces RequestHandler::GetTransportData(const OutputGroup & outputs){
		WritingResponces result;
		for (auto & entity : outputs.queries) {
			std::pair<int, Responce> responce;
			responce.first = entity.id;

			if (entity.type == QueryType::BUS) {
				auto route = tc_.FindRoute(entity.name);
				if (route.has_value()) {
					responce.second = tc_.GetRouteInfo(route.value());
				} else {
					responce.second = Errors::NOT_FOUND;
				}
			} else if (entity.type == QueryType::STOP) {
				auto stop = tc_.FindStop(entity.name);
				if (stop.has_value()) {
					responce.second = tc_.GetBusesForStop(stop.value());
				} else {
					responce.second = Errors::NOT_FOUND;
				}
			} else if (entity.type == QueryType::MAP) {
				responce.second = GenerateMap();
			} else if (entity.type == QueryType::ROUTE) {
				auto route = router_.GetRoute(entity.from, entity.to);
				if (route.has_value()) {
					responce.second = route.value();
				}else {
					responce.second = Errors::NOT_FOUND;
				}
			}
			result.push_back(responce);
		}
		return result;
	}
}
