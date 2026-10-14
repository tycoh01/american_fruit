#pragma once

#include "fci.h"

#include <cstddef>
#include <string>
#include <vector>

namespace fci
{
struct FruitObservation
{
	std::string state;
	std::string commodity;
	int year{};
	double value{};
};

struct FciInput
{
	std::vector<std::string> nodes;
	std::vector<DirectedEdge> edges;
	std::vector<FruitObservation> observations;
};

struct HttpResponse
{
	std::string url;
	std::string body;
	long status_code{};
};

std::string url_encode(const std::string& value);
std::string build_nass_url(
	const std::string& api_key,
	const std::string& commodity,
	int year,
	const std::string& state_alpha = {});

std::vector<FruitObservation> parse_nass_observations(const std::string& json);
FciInput to_fci_input(const std::vector<FruitObservation>& observations);
std::string to_json(const FciInput& input);
void write_json_file(const FciInput& input, const std::string& file_path);
std::string to_markdown(const FciInput& input);
void write_markdown_file(const FciInput& input, const std::string& file_path);
std::string to_csv(const FciInput& input);
void write_csv_file(const FciInput& input, const std::string& file_path);
std::string fetch_url(const std::string& url);
std::vector<HttpResponse> fetch_urls(const std::vector<std::string>& urls, std::size_t max_concurrency = 32);
std::vector<FruitObservation> fetch_usda_fruit(
	const std::string& api_key,
	const std::string& commodity,
	int year,
	const std::string& state_alpha = {});
}
