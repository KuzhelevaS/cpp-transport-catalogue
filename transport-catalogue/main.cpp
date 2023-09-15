#include <fstream>
#include <iostream>
#include <string_view>
#include "request_handler.h"
#include "json_reader.h"

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
	stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		PrintUsage();
		return 1;
	}

	using namespace transport;

	const std::string_view mode(argv[1]);

	json_reader::Reader json_reader(std::cin, std::cout);
	handler::RequestHandler app(&json_reader);

	if (mode == "make_base"sv) {

		app.MakeBase();

	} else if (mode == "process_requests"sv) {

		app.ProcessRequests();

	} else {
		PrintUsage();
		return 1;
	}
}
