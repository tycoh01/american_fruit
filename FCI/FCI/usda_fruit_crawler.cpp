#include "usda_fruit_crawler.h"

#include <curl/curl.h>


#include <algorithm>
#include <cctype>
#include <iomanip>
#include <fstream>
#include <map>
#include <memory>
#include <queue>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>



namespace fci
{
namespace
{
std::string get_json_string(const std::string& object, const std::string& field)
{
	const std::regex pattern(
		"\\\"" + field + "\\\"\\s*:\\s*(?:\\\"([^\\\"]*)\\\"|([^,}\\s]+))",
		std::regex::icase);
	std::smatch match;
	if (!std::regex_search(object, match, pattern))
	{
		return {};
	}
	return match[1].matched ? match[1].str() : match[2].str();
}

std::string get_json_value(const std::string& object)
{
	auto value = get_json_string(object, "Value");
	value.erase(std::remove_if(value.begin(), value.end(), [](const unsigned char character)
	{
		return character == ',' || std::isspace(character);
	}), value.end());
	return value;
}

std::string normalize_node_part(std::string value)
{
	std::replace(value.begin(), value.end(), ' ', '_');
	return value;
}
}

std::string to_markdown(const FciInput& input)
{
	std::ostringstream output;
	output << "# FCI Fruit Input\n\n";
	output << "## FCI expression\n\n";
	output << "`CommonCause(X, Y) = Anc(X) INTERSECT Anc(Y)`\n\n";
	output << "## Nodes\n\n";
	for (const auto& node : input.nodes)
	{
		output << "- `" << node << "`\n";
	}
	output << "\n## Candidate edges\n\n";
	output << "| Cause | Effect |\n|---|---|\n";
	for (const auto& [cause, effect] : input.edges)
	{
		output << "| `" << cause << "` | `" << effect << "` |\n";
	}
	output << "\n## Observations\n\n";
	output << "| State | Commodity | Year | Value |\n|---|---|---:|---:|\n";
	for (const auto& observation : input.observations)
	{
		output << "| " << observation.state << " | " << observation.commodity
			   << " | " << observation.year << " | " << observation.value << " |\n";
	}
	output << "\n> Edges are candidate associations generated from the crawled data; they are not proven causal relationships.\n";
	return output.str();
}

void write_markdown_file(const FciInput& input, const std::string& file_path)
{
	std::ofstream output(file_path, std::ios::binary);
	if (!output)
	{
		throw std::runtime_error("Unable to open Markdown file for writing: " + file_path);
	}

	output << to_markdown(input);
	if (!output)
	{
		throw std::runtime_error("Unable to write Markdown file: " + file_path);
	}
}

std::string url_encode(const std::string& value)
{
	std::ostringstream encoded;
	encoded << std::uppercase << std::hex;
	for (const unsigned char character : value)
	{
		if (std::isalnum(character) || character == '-' || character == '_' || character == '.' || character == '~')
		{
			encoded << character;
		}
		else
		{
			encoded << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(character);
		}
	}
	return encoded.str();
}

std::string build_nass_url(
	const std::string& api_key,
	const std::string& commodity,
	int year,
	const std::string& state_alpha)
{
	if (api_key.empty() || commodity.empty() || year <= 0)
	{
		throw std::invalid_argument("API key, commodity, and year are required.");
	}

	std::ostringstream url;
	url << "https://quickstats.nass.usda.gov/api/api_GET/?key=" << url_encode(api_key)
		<< "&commodity_desc=" << url_encode(commodity)
		<< "&year=" << year
		<< "&format=JSON";
	if (!state_alpha.empty())
	{
		url << "&state_alpha=" << url_encode(state_alpha);
	}
	return url.str();
}

std::vector<FruitObservation> parse_nass_observations(const std::string& json)
{
	std::vector<FruitObservation> observations;
	std::size_t object_start = json.find('{');
	while (object_start != std::string::npos)
	{
		const auto object_end = json.find('}', object_start);
		if (object_end == std::string::npos)
		{
			break;
		}
		const std::string object = json.substr(object_start, object_end - object_start + 1);
		const std::string state = get_json_string(object, "state_alpha");
		const std::string commodity = get_json_string(object, "commodity_desc");
		const std::string year_text = get_json_string(object, "year");
		const std::string value_text = get_json_value(object);
		if (state.empty() || commodity.empty() || year_text.empty() || value_text.empty())
		{
			continue;
		}

		try
		{
			observations.push_back({ state, commodity, std::stoi(year_text), std::stod(value_text) });
		}
		catch (const std::exception&)
		{
		}
		object_start = json.find('{', object_end + 1);
	}
	return observations;
}

FciInput to_fci_input(const std::vector<FruitObservation>& observations)
{
	FciInput input;
	std::set<std::string> nodes;
	std::set<DirectedEdge> edges;

	for (const auto& observation : observations)
	{
		const auto state_node = "state:" + normalize_node_part(observation.state);
		const auto fruit_node = "fruit:" + normalize_node_part(observation.commodity);
		nodes.insert(state_node);
		nodes.insert(fruit_node);
		edges.emplace(state_node, fruit_node);
	}

	input.nodes.assign(nodes.begin(), nodes.end());
	input.edges.assign(edges.begin(), edges.end());
	input.observations = observations;
	return input;
}

std::string to_json(const FciInput& input)
{
	std::ostringstream output;
	output << "{\n  \"nodes\": [";
	for (std::size_t index = 0; index < input.nodes.size(); ++index)
	{
		if (index != 0) output << ", ";
		output << '\"' << input.nodes[index] << '\"';
	}
	output << "],\n  \"edges\": [";
	for (std::size_t index = 0; index < input.edges.size(); ++index)
	{
		if (index != 0) output << ", ";
		output << "{\"cause\":\"" << input.edges[index].first
			   << "\",\"effect\":\"" << input.edges[index].second << "\"}";
	}
	output << "],\n  \"observations\": [";
	for (std::size_t index = 0; index < input.observations.size(); ++index)
	{
		if (index != 0) output << ", ";
		const auto& observation = input.observations[index];
		output << "{\"state\":\"" << observation.state
			   << "\",\"commodity\":\"" << observation.commodity
			   << "\",\"year\":" << observation.year
			   << ",\"value\":" << observation.value << "}";
	}
	output << "]\n}\n";
	return output.str();
}

void write_json_file(const FciInput& input, const std::string& file_path)
{
	std::ofstream output(file_path, std::ios::binary);
	if (!output)
	{
		throw std::runtime_error("Unable to open FCI input file for writing: " + file_path);
	}

	output << to_json(input);
	if (!output)
	{
		throw std::runtime_error("Unable to write FCI input file: " + file_path);
	}
}

namespace
{
struct Transfer
{
	std::string url;
	std::string body;
};

std::size_t write_callback(char* data, std::size_t size, std::size_t count, void* user_data)
{
	auto& body = *static_cast<std::string*>(user_data);
	body.append(data, size * count);
	return size * count;
}

}

std::vector<HttpResponse> fetch_urls(const std::vector<std::string>& urls, std::size_t max_concurrency)
{
	if (max_concurrency == 0) throw std::invalid_argument("max_concurrency must be greater than zero.");
	if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) throw std::runtime_error("curl_global_init failed.");
	CURLM* multi = curl_multi_init();
	if (!multi) throw std::runtime_error("curl_multi_init failed.");
	std::queue<std::string> pending;
	for (const auto& url : urls) pending.push(url);
	std::map<CURL*, std::unique_ptr<Transfer>> active;
	std::vector<HttpResponse> completed;

	const auto add_pending = [&]()
	{
		while (!pending.empty() && active.size() < max_concurrency)
		{
			auto transfer = std::make_unique<Transfer>();
			transfer->url = pending.front();
					pending.pop();
			CURL* easy = curl_easy_init();
			if (!easy) throw std::runtime_error("curl_easy_init failed.");
					curl_easy_setopt(easy, CURLOPT_URL, transfer->url.c_str());
					curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_callback);
					curl_easy_setopt(easy, CURLOPT_WRITEDATA, &transfer->body);
					curl_easy_setopt(easy, CURLOPT_USERAGENT, "FCI-USDA-Crawler/2.0");
					curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
					curl_easy_setopt(easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
					curl_easy_setopt(easy, CURLOPT_ACCEPT_ENCODING, "");
			if (curl_multi_add_handle(multi, easy) != CURLM_OK) throw std::runtime_error("curl_multi_add_handle failed.");
			active.emplace(easy, std::move(transfer));
		}
	};

	int running = 0;
	add_pending();
	while (!active.empty())
	{
		CURLMcode result;
		do { result = curl_multi_perform(multi, &running); } while (result == CURLM_CALL_MULTI_PERFORM);
		if (result != CURLM_OK) throw std::runtime_error("curl_multi_perform failed.");
		int messages = 0;
		while (CURLMsg* message = curl_multi_info_read(multi, &messages))
		{
			if (message->msg != CURLMSG_DONE) continue;
			auto it = active.find(message->easy_handle);
			if (it == active.end()) continue;
			long status = 0;
			curl_easy_getinfo(message->easy_handle, CURLINFO_RESPONSE_CODE, &status);
			if (message->data.result != CURLE_OK) throw std::runtime_error("libcurl request failed: " + std::string(curl_easy_strerror(message->data.result)));
			completed.push_back({ it->second->url, it->second->body, status });
			curl_multi_remove_handle(multi, message->easy_handle);
			curl_easy_cleanup(message->easy_handle);
			active.erase(it);
			add_pending();
		}
		if (!active.empty() && running > 0) { int sockets = 0; curl_multi_poll(multi, nullptr, 0, 1000, &sockets); }
	}
	curl_multi_cleanup(multi);
	curl_global_cleanup();
	return completed;
}

std::string fetch_url(const std::string& url)
{
	const auto responses = fetch_urls({ url }, 1);
	if (responses.empty() || responses.front().status_code < 200 || responses.front().status_code >= 300) throw std::runtime_error("HTTP request returned status " + std::to_string(responses.empty() ? 0 : responses.front().status_code));
	return responses.front().body;
}


std::vector<FruitObservation> fetch_usda_fruit(
	const std::string& api_key,
	const std::string& commodity,
	int year,
	const std::string& state_alpha)
{
	return parse_nass_observations(fetch_url(build_nass_url(api_key, commodity, year, state_alpha)));
}
}
