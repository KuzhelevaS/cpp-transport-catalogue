#pragma once

#include <filesystem>
#include <vector>
#include <optional>

#include "request_handler.h"
#include "transport_catalogue.h"

namespace transport::serialization {
	using Path = std::filesystem::path;

	void Serialize(Path file_name, const TransportCatalogue& catalogue, const handler::InputResultGroup & settings);
	std::optional<handler::InputResultGroup> Deserialize(Path file_name);
}
