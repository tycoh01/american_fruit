#pragma once

#include <string>
#include <utility>
#include <vector>

namespace fci
{
using DirectedEdge = std::pair<std::string, std::string>;

struct FciParameters
{
	std::string first_target;
	std::string second_target;
	std::vector<std::string> common_causes;
	std::string common_cause_expression;
};

int add(int left, int right);

std::vector<std::string> common_causes(
	const std::vector<std::string>& nodes,
	const std::vector<DirectedEdge>& edges,
	const std::string& first_target,
	const std::string& second_target);

FciParameters calculate_fci(
	const std::vector<std::string>& nodes,
	const std::vector<DirectedEdge>& edges,
	const std::string& first_target,
	const std::string& second_target);

std::string common_cause_expression(
	const std::string& first_target,
	const std::string& second_target);
}
