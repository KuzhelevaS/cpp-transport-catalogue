#include "request_handler.h"
#include <iostream>
#include <sstream>

namespace transport::handler {
	RequestHandler::RequestHandler(InputOutput* io)
		: io_(io) {}

	void RequestHandler::RunInputOutput() {
		// Читаем внешние данные
		auto queries = io_->Read();

		// Передаем настройки в систему
		renderer_.SetSettings(queries.settings);
		FillTransportCatalogue(queries.inputs);
		router_.Init(queries.router.settings, db_);

		// Формируем и печатаем ответы на запросы
		auto printable_result = GetTransportData(queries.outputs);
		io_->Write(printable_result);
	}

	std::string RequestHandler::GenerateMap() {
		auto buses = db_.GetAllRoutes();
		std::ostringstream out;
		renderer_.Print(std::move(buses), out);
		return out.str();
	}

	void RequestHandler::FillTransportCatalogue(const InputGroup & inputs) {
		for (auto & [stop_name, stop_data] : inputs.stops) {
			db_.AddStop(stop_name, stop_data.coordinates);
		}

		for (auto & [stop_name, stop_data] : inputs.stops) {
			for (auto & [other_name, distance] : stop_data.distances) {
				auto stop_from = db_.FindStop(stop_name);
				auto stop_to = db_.FindStop(other_name);
				db_.SetStopDistance(stop_from.value(), stop_to.value(), distance);
			}
		}

		for (auto & [bus_name, route] : inputs.buses) {
			db_.AddRoute(bus_name, route.stops, route.is_looped);
		}
	}

	WritingResponces RequestHandler::GetTransportData(const OutputGroup & outputs){
		WritingResponces result;
		for (auto & entity : outputs.queries) {
			std::pair<int, Responce> responce;
			responce.first = entity.id;

			if (entity.type == QueryType::BUS) {
				auto route = db_.FindRoute(entity.name);
				if (route.has_value()) {
					responce.second = db_.GetRouteInfo(route.value());
				} else {
					responce.second = Errors::NOT_FOUND;
				}
			} else if (entity.type == QueryType::STOP) {
				auto stop = db_.FindStop(entity.name);
				if (stop.has_value()) {
					responce.second = db_.GetBusesForStop(stop.value());
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
