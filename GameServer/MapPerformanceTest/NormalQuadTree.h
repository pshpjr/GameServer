#pragma once
#include "IGameObjectContainer.h"


#include "stdafx.h"
#include "ContentTypes.h"
#include "NormalQuadTree.h"
#include <vector>
#include <algorithm>
#include "../Player.h"

// 정의된 node의 최대 capacity


class NormalQuadTree : public psh::IGameObjectContainer {
    static constexpr int NODE_CAPACITY = 10;

    struct Node {
        Node *parent, *NE, *NW, *SE, *SW; // 부모 포인터를 포함하여 4분면 Node pointers
        std::vector<shared_ptr<psh::Player>> players; // 현재 노드에 저장된 player들
        psh::FVector minPoint, maxPoint; // Node의 영역을 정의하기 위한 최솟값, 최댓값

        Node(Node* parent, const psh::FVector& min, const psh::FVector& max) : 
            parent(parent), NE(nullptr), NW(nullptr), SE(nullptr), SW(nullptr), minPoint(min), maxPoint(max) {}
    };

public:
    NormalQuadTree(const psh::FVector& center, const psh::FVector& halfDimension) : IGameObjectContainer(center.X + halfDimension.X),
        root(new Node(nullptr, center - halfDimension, center + halfDimension)) {}

    // 플레이어를 삽입하거나 이동합니다
    void MovePlayer(const psh::FVector oldLocation, const psh::FVector newLocation, std::shared_ptr<psh::Player>& target) override {
        Node* oldNode = FindNodeContainingPoint(root, oldLocation);
        if(oldNode && oldNode != FindNodeContainingPoint(root,newLocation)) {
            {
                RemovePlayer(oldNode, target);
            }
            {
 
                AddPlayer(newLocation, target);
            }
        }
    }

    void AddPlayer(const psh::FVector location, std::shared_ptr<psh::Player>& target) {
        if(!Insert(root, nullptr, location, target))
        {
            DebugBreak();
        }
    }

    void RemovePlayer(Node* node, std::shared_ptr<psh::Player>& target) {
        const auto it = std::find(node->players.begin(), node->players.end(), target);
        if(it != node->players.end()) {
            node->players.erase(it);
        }
    }
    void Iterate(psh::FVector radius , const function<void(psh::Player&)>& toInvoke) override;

    void Iterate(psh::FVector start, psh::FVector end, const std::function<void(psh::Player&)>& toInvoke) override;

private:
    Node* root;

    bool Overlap(const psh::FVector min1, const psh::FVector max1, const psh::FVector min2, const psh::FVector max2) const
    {
        return min1.X <= max2.X && min2.X <= max1.X && min1.Y <= max2.Y && min2.Y <= max1.Y;
    }
    
    void GetPlayersInRange(Node* node, const psh::FVector start, const psh::FVector end, const function<void(psh::Player&)>& toInvoke) const
    {
        // 노드가 없거나 검색 범위밖인 경우 반환
        if (node == nullptr || !Overlap(node->minPoint, node->maxPoint, start, end))
        {
            return;
        }
        
        // 검색 범위 내부에 있는 플레이어 추가
        for (auto& player : node->players)
        {
            if (InBoundary(node, player->Location()))
            {
                toInvoke(*player.get());
            }
        }
        
        // 모든 자식 노드에 대해 재귀적으로 탐색합니다.
        GetPlayersInRange(node->NE, start, end, toInvoke);
        GetPlayersInRange(node->NW, start, end, toInvoke);
        GetPlayersInRange(node->SE, start, end, toInvoke);
        GetPlayersInRange(node->SW, start, end, toInvoke);
        
    }   
    
    void Subdivide(Node* node) {
        const psh::FVector center = (node->minPoint + node->maxPoint) / 2.0;
        const psh::FVector half = (node->maxPoint - node->minPoint) / 2.0;

        node->SE = new Node(node, center, center + half);
        node->SW = new Node(node, psh::FVector(node->minPoint.X, center.Y), psh::FVector(center.X, node->maxPoint.Y));
        node->NE = new Node(node, psh::FVector(center.X, node->minPoint.Y), psh::FVector(node->maxPoint.X, center.Y));
        node->NW = new Node(node, node->minPoint, center);

        // Redistribute players to child nodes
        auto& players = node->players;
        for (auto it = players.begin(); it != players.end(); /* it incremented in loop */) {
            shared_ptr<psh::Player> target = *it;
            const psh::FVector location = target->Location();

            // Remove player from current node and insert it into appropriate child node.
            if (Insert(node->NE, node, location, target) ||
                Insert(node->NW, node, location, target) ||
                Insert(node->SE, node, location, target) ||
                Insert(node->SW, node, location, target)) {

                // Iterator remains valid after erase
                it = players.erase(it);
                } else {
                    // If player wasn't inserted into any child node, advance iterator.
                    ++it;
                }
        }
    }

    bool Insert(Node* node, Node* parent, const psh::FVector location, shared_ptr<psh::Player>& target) {
        // Player's location is outside of the node's boundary, return false
        if (!InBoundary(node, location)) {
            return false;
        }
        // Try to insert the player into subdivision if exists.
        if(node->NE || node->NW || node->SE || node->SW){
            return Insert(node->NE, node, location, target) || 
                   Insert(node->NW, node, location, target) ||
                   Insert(node->SE, node, location, target) ||
                   Insert(node->SW, node, location, target);
        }
        // If space is available in the leaf node, add the player
        if (node->players.size() < NODE_CAPACITY){
            node->players.push_back(target);
            return true;
        }
        
        Subdivide(node);
        const bool result = Insert(node->NE, node, location, target) || 
               Insert(node->NW, node, location, target) ||
               Insert(node->SE, node, location, target) ||
               Insert(node->SW, node, location, target);

        if(!result)
            DebugBreak();
        
        return result;
    }

    // 위치가 주어진 범위 내부에 있는지 확인합니다.
    static bool InBoundary(Node* node, const psh::FVector location) {
        return (location.X >= node->minPoint.X && location.X < node->maxPoint.X &&
                location.Y >= node->minPoint.Y && location.Y < node->maxPoint.Y);
    }

    static Node* FindNodeContainingPoint(Node* node, const psh::FVector point) {
        // 노드의 범위를 벗어나는 점인 경우
        if (!InBoundary(node, point)) {
            return nullptr;
        }
        // NE, NW, SE, SW quadrant/cell 만약 내부에 포함되면, 재귀적으로 찾음
        if (node->NE && InBoundary(node->NE, point)) {
            return FindNodeContainingPoint(node->NE, point);
        }
        if (node->NW && InBoundary(node->NW, point)) {
            return FindNodeContainingPoint(node->NW, point);
        }
        if (node->SE && InBoundary(node->SE, point)) {
            return FindNodeContainingPoint(node->SE, point);
        }
        if (node->SW && InBoundary(node->SW, point)) {
            return FindNodeContainingPoint(node->SW, point);
        }
        // 아니면, 현재 노드를 반환
        return node;
    }
};

inline void NormalQuadTree::Iterate(psh::FVector radius , const function<void(psh::Player&)>& toInvoke)
{
}
