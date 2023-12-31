#pragma once

#include "transport_catalogue.h"
#include "transport_router.h"
#include "map_renderer.h"
#include <variant>

namespace transport::handler {
	struct Stop {
		geo::Coordinates coordinates;
		std::unordered_map<std::string, int> distances;
	};
	struct Route {
		bool is_looped;
		std::vector<std::string> stops;
	};
	struct InputGroup {
		std::unordered_map<std::string, Stop> stops;
		std::unordered_map<std::string, Route> buses;
	};

	enum class QueryType {
		BUS,
		STOP,
		MAP,
		ROUTE
	};
	struct Query {
		int id;
		QueryType type;
		std::string name;
		std::string from;
		std::string to;
	};
	struct OutputGroup {
		std::vector<Query> queries;
	};
	struct RouteGroup {
		TransportRouter::RouterSettings settings;
	};

	struct DateBase {
		std::string file_name;
	};

	struct InputResultGroup {
		InputGroup inputs;
		OutputGroup outputs;
		renderer::Settings render_settings;
		RouteGroup router;
		DateBase data_base;
	};

	enum class Errors {
		NOT_FOUND
	};
	using Responce = std::variant<Errors, std::vector<std::string_view>,
		TransportCatalogue::RouteInfo, std::string, TransportRouter::Route>;
	using WritingResponces = std::vector<std::pair<int, Responce>>; // id, data

	// Интерфейс чтения/записи
	class InputOutput {
	public:
		virtual InputResultGroup Read() const = 0;
		virtual void Write(const WritingResponces & data) const = 0;
	};

	class RequestHandler {
	public:
		RequestHandler(InputOutput* io);
		void MakeBase();
		void ProcessRequests();

	private:
		TransportCatalogue tc_;
		renderer::MapRenderer renderer_;
		const InputOutput* io_;
		TransportRouter router_;

		void FillTransportCatalogue(const InputGroup & inputs);
		WritingResponces GetTransportData(const OutputGroup & outputs);
		std::string GenerateMap();
	};
}


