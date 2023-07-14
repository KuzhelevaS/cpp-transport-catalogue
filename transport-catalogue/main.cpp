#include "transport_catalogue.h"
#include "request_handler.h"
#include "json_reader.h"
#include "map_renderer.h"

int main()
{
	using namespace transport;

	json_reader::Reader json_reader(std::cin, std::cout);
	handler::RequestHandler app(&json_reader);
	app.RunInputOutput();

	return 0;
}

