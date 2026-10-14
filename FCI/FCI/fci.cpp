#include "fci.h"

#include <queue>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <sstream>

namespace fci
{
int add(int left, int right)
{
	return left + right;
}

std::vector<std::string> common_causes(
	const std::vector<std::string>& nodes,
	const std::vector<DirectedEdge>& edges,
	const std::string& first_target,
	const std::string& second_target)
{
	if (first_target.empty() || second_target.empty())
	{
		throw std::invalid_argument("Target node names cannot be empty.");
	}

	std::unordered_set<std::string> known_nodes(nodes.begin(), nodes.end());
	if (!known_nodes.contains(first_target) || !known_nodes.contains(second_target))
	{
		throw std::invalid_argument("Both target nodes must be present in nodes.");
	}

	std::unordered_map<std::string, std::vector<std::string>> parents;
	for (const auto& node : known_nodes)
	{
		parents.emplace(node, std::vector<std::string>{});
	}

	for (const auto& [cause, effect] : edges)
	{
		if (!known_nodes.contains(cause) || !known_nodes.contains(effect))
		{
			throw std::invalid_argument("Every edge endpoint must be present in nodes.");
		}
		parents[effect].push_back(cause);
	}

	const auto ancestors_of = [&parents](const std::string& target)
	{
		std::unordered_set<std::string> ancestors;
		std::queue<std::string> pending;
		pending.push(target);

		while (!pending.empty())
		{
			const auto current = pending.front();
			pending.pop();

			for (const auto& parent : parents.at(current))
			{
				if (ancestors.insert(parent).second)
				{
					pending.push(parent);
				}
			}
		}

		return ancestors;
	};

	const auto first_ancestors = ancestors_of(first_target);
	const auto second_ancestors = ancestors_of(second_target);
	std::set<std::string> result;

	for (const auto& ancestor : first_ancestors)
	{
		if (second_ancestors.contains(ancestor))
		{
			result.insert(ancestor);
		}
	}

	return { result.begin(), result.end() };
}

std::string common_cause_expression(
	const std::string& first_target,
	const std::string& second_target)
{
	if (first_target.empty() || second_target.empty())
	{
		throw std::invalid_argument("Target node names cannot be empty.");
	}

	return "CommonCause(" + first_target + ", " + second_target + ") = Anc(" +
		first_target + ") INTERSECT Anc(" + second_target + ")";
}

FciParameters calculate_fci(
	const std::vector<std::string>& nodes,
	const std::vector<DirectedEdge>& edges,
	const std::string& first_target,
	const std::string& second_target)
{
	return {
		first_target,
		second_target,
		common_causes(nodes, edges, first_target, second_target),
		common_cause_expression(first_target, second_target)
	};
}
}
