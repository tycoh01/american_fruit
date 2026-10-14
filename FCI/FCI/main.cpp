#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "fci.h"
#include "usda_fruit_crawler.h"

int main(int argc, char* argv[])
{
	const bool has_crawl_command = argc >= 2 && std::string(argv[1]) == "crawl";
	const int argument_offset = has_crawl_command ? 1 : 0;
	if (argc >= 5 + argument_offset)
	{
		try
		{
			char* api_key_buffer = nullptr;
			std::size_t api_key_length = 0;
			_dupenv_s(&api_key_buffer, &api_key_length, "NASS_API_KEY");
			if (api_key_buffer == nullptr)
			{
				std::cerr << "Set NASS_API_KEY before crawling USDA data.\n";
				return 1;
			}

			const std::string api_key(api_key_buffer, api_key_length);
			free(api_key_buffer);
			const auto observations = fci::fetch_usda_fruit(api_key, argv[1 + argument_offset], std::stoi(argv[2 + argument_offset]), argv[3 + argument_offset]);
			const auto input = fci::to_fci_input(observations); const std::string output_path = argv[4 + argument_offset]; if (output_path.ends_with(".csv")) fci::write_csv_file(input, output_path); else if (output_path.ends_with(".md")) fci::write_markdown_file(input, output_path); else fci::write_json_file(input, output_path); std::cout << "Step 1/2 completed: fetched " << observations.size() << " observations and saved " << output_path << "." << std::endl; std::string first_target; std::string second_target; if (argc >= 7 + argument_offset) { first_target = argv[5 + argument_offset]; second_target = argv[6 + argument_offset]; } else { for (const auto& node : input.nodes) if (node.starts_with("fruit:")) { if (first_target.empty()) first_target = node; else if (second_target.empty()) { second_target = node; break; } } } if (!first_target.empty() && !second_target.empty()) { const auto parameters = fci::calculate_fci(input.nodes, input.edges, first_target, second_target); std::cout << "Step 3 completed: " << parameters.common_cause_expression << std::endl; for (const auto& cause : parameters.common_causes) std::cout << "- Common cause: " << cause << std::endl; } else { std::cout << "Step 3 skipped: provide two target nodes or crawl multiple fruit commodities." << std::endl; } return 0;
		}
		catch (const std::exception& error)
		{
			std::cerr << "USDA crawler failed: " << error.what() << '\n';
			return 1;
		}
	}

	std::cout << "Usage: FCI.exe <commodity> <year> <state_alpha> <output.csv> [target1 target2]\n";
	std::cout << "Example: FCI.exe APPLES 2023 WA american_fruit.csv\n";
	std::cout << "Set NASS_API_KEY to use the USDA NASS Quick Stats API.\n";

	const std::vector<std::string> nodes{ "U", "X", "Y", "Z" };
	const std::vector<fci::DirectedEdge> edges{
		{ "U", "X" }, { "U", "Y" }, { "X", "Z" }, { "Y", "Z" }
	};
	const auto parameters = fci::calculate_fci(nodes, edges, "X", "Y");

	std::cout << "Common causes of X and Y:\n";
	for (const auto& cause : parameters.common_causes)
	{
		std::cout << "- " << cause << '\n';
	}
	std::cout << parameters.common_cause_expression << '\n';
	return 0;
}
