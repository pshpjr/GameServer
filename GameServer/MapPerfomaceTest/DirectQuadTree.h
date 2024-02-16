#pragma once
#include "IGameObjectContainer.h"
#include "ContentTypes.h"
#include "../Player.h"

class DirectQuadTree : public psh::IGameObjectContainer
{
	static constexpr int NODE_CAPACITY = 10;
	static constexpr int MAX_DEPTH = 6;
	

	struct Node
	{
		std::vector<psh::Player*> players;
		psh::FVector minPoint = {};
		psh::FVector maxPoint = {};
	};

public:
	bool Insert(psh::Player* target) {
		// Calculate the index based on player's location
		int index = CalcIndex(target->Location());
        
		// Start from the leaf node and move up if necessary
		for(int depth = 0; depth <= MAX_DEPTH; depth++) {
			if(Insert(target, index, depth)) {
				return true;
			}
			// isMove up to the parent node
			index /= 2;
		}
		return false;
	}

	DirectQuadTree(int mapSize, int minSector) : IGameObjectContainer(mapSize), _nodes(mapSize/minSector * mapSize/minSector,Node())
	{
		
	}

	static bool InBoundary(int index, psh::FVector location)
	{
		
	}

	static int GetFirstBitFromMSB(int data)
	{
		unsigned long result;
		_BitScanReverse(&result,data);
		return result;
	}
	
	bool Move(psh::Player* target, psh::FVector new_location) {
		// Calculate old and new index
		int old_index = CalcIndex(target->Location());
		int new_index = CalcIndex(new_location);
        
		// If index does not change, no move operation is needed
		if(old_index == new_index) return true;
        
		// Remove player from old node
		auto& old_node = _nodes[old_index];
		old_node.players.erase(std::remove(old_node.players.begin(), old_node.players.end(), target), old_node.players.end());
        
		// Update player's location
		target->Location( new_location);
        
		// Insert player to new node
		// Start from the leaf node and move up if necessary
		for(int depth = 0; depth <= MAX_DEPTH; depth++) {
			if(Insert(target, new_index, depth)) {
				return true;
			}
			// isMove up to the parent node
			new_index /= 2;
		}
		return false;
	}
	
	std::vector<psh::Player*> RangeQuery(psh::FVector location, float range) {
		// perform range query and return players within range
		return {};
	}

private:
	bool Insert(psh::Player* target, int index, int depth) {
		auto& node = _nodes[index];
		if(depth == MAX_DEPTH || node.players.size() < NODE_CAPACITY) {
			node.players.push_back(target);
			return true;
		} else {
			// If the node is not subdivided yet, subdivide it now
			Subdivide(index);
			// Determine the correct child index and recurrence
			int child_index = CalcIndex(target->Location(), depth + 1);
			return Insert(target, child_index, depth + 1);
		}
	}

	void Subdivide(int index) {
		// Create four children nodes
		for(int i=0; i<4; i++) _nodes.push_back(Node());
        
		// Distribute players to children nodes
		auto& node = _nodes[index];
		for(auto player : node.players) {
			int child_index = CalcIndex(player->Location(), MAX_DEPTH);
			_nodes[child_index].players.push_back(player);
		}
		// Clear current node's players
		node.players.clear();
	}

	int CalcIndex(psh::FVector location, int depth = MAX_DEPTH) {
		// Convert 2D location to 1D index
		int index = std::round(location.X * std::pow(2, depth)) +
					std::round(location.Y * std::pow(2, depth)) *
					std::round(MAP_SIZE / std::pow(2, depth));
		return index;
	}
	
	std::vector<Node> _nodes;
};
