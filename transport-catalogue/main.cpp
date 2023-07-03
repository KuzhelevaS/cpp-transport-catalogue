#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

int main()
{
	using namespace transport;

	TransportCatalogue t;
	input::FillCatalogue(t);
	stat::ExecuteRequests(t);

	return 0;
}
