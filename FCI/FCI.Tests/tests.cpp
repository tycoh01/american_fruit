#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "..\FCI\fci.h"
#include "..\FCI\usda_fruit_crawler.h"

int main()
{
	assert(fci::add(2, 3) == 5);
	assert(fci::add(-2, 2) == 0);

	const std::vector<std::string> nodes{ "A", "B", "U", "X", "Y", "Z" };
	const std::vector<fci::DirectedEdge> edges{
		{ "A", "U" }, { "U", "X" }, { "U", "Y" }, { "B", "X" }, { "Y", "Z" }
	};

	const auto causes = fci::common_causes(nodes, edges, "X", "Y");
	assert((causes == std::vector<std::string>{ "A", "U" }));

	const auto no_causes = fci::common_causes(nodes, edges, "X", "Z");
	assert((no_causes == std::vector<std::string>{ "A", "U" }));

	bool rejected_unknown_target = false;
	try
	{
		(void)fci::common_causes(nodes, edges, "X", "missing");
	}
	catch (const std::invalid_argument&)
	{
		rejected_unknown_target = true;
	}
	assert(rejected_unknown_target);

	const auto parameters = fci::calculate_fci(nodes, edges, "X", "Y");
	assert((parameters.common_causes == std::vector<std::string>{ "A", "U" }));
	assert(parameters.common_cause_expression ==
		"CommonCause(X, Y) = Anc(X) INTERSECT Anc(Y)");

	assert(fci::url_encode("FRUIT & NUTS") == "FRUIT%20%26%20NUTS");
	bool rejected_zero_concurrency = false;
	try { (void)fci::fetch_urls({}, 0); } catch (const std::invalid_argument&) { rejected_zero_concurrency = true; }
	assert(rejected_zero_concurrency);
	const auto url = fci::build_nass_url("test key", "APPLES", 2023, "WA");
	assert(url.find("commodity_desc=APPLES") != std::string::npos);
	assert(url.find("state_alpha=WA") != std::string::npos);

	const std::string response = R"({"data":[
		{"state_alpha":"WA","commodity_desc":"APPLES","year":"2023","Value":"1,234"},
		{"state_alpha":"CA","commodity_desc":"ORANGES","year":"2023","Value":"567"}
	]})";
	const auto observations = fci::parse_nass_observations(response);
	assert(observations.size() == 2);
	assert(observations[0].value == 1234.0);
	const auto numeric_year_response = R"({"data":[{"state_alpha":"WA","commodity_desc":"APPLES","year":2023,"Value":"174,000"}]})";
	const auto numeric_year_observations = fci::parse_nass_observations(numeric_year_response);
	assert(numeric_year_observations.size() == 1);
	assert(numeric_year_observations[0].year == 2023);
	const auto input = fci::to_fci_input(observations);
	assert(input.nodes.size() == 4);
	assert(input.edges.size() == 2);
	assert(fci::to_json(input).find("fruit:APPLES") != std::string::npos);
	const auto markdown = fci::to_markdown(input);
	assert(markdown.find("# FCI Fruit Input") != std::string::npos);
	assert(markdown.find("CommonCause(X, Y) = Anc(X) INTERSECT Anc(Y)") != std::string::npos);
	assert(markdown.find("fruit:APPLES") != std::string::npos);
	fci::write_json_file(input, "fci_test_input.json");
	std::ifstream input_file("fci_test_input.json");
	assert(input_file.good());
	input_file.close();
	std::remove("fci_test_input.json");
	fci::write_markdown_file(input, "fci_test_input.md");
	std::ifstream markdown_file("fci_test_input.md");
	assert(markdown_file.good());
	markdown_file.close();
	std::remove("fci_test_input.md");

	std::cout << "All FCI tests passed.\n";
	return 0;
}
