#include "usda_fruit_crawler.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace fci
{
std::string to_csv(const FciInput& input)
{
	std::ostringstream output;
	output << "state,commodity,year,value,cause,effect\n";
	for (const auto& observation : input.observations)
	{
		output << observation.state << ',' << observation.commodity << ','
			   << observation.year << ',' << observation.value << ",state:"
			   << observation.state << ",fruit:" << observation.commodity << '\n';
	}
	return output.str();
}

void write_csv_file(const FciInput& input, const std::string& file_path)
{
	std::ofstream output(file_path, std::ios::binary);
	if (!output)
	{
		throw std::runtime_error("Unable to open CSV file for writing: " + file_path);
	}
	output << to_csv(input);
	if (!output)
	{
		throw std::runtime_error("Unable to write CSV file: " + file_path);
	}
}
}
