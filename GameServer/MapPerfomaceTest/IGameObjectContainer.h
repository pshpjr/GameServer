#pragma once
#include <functional>
#include <memory>
/**
 * @brief The IPlayerContainer class is an interface for a player container.
 * 
 *  
 * 
 */


namespace psh
{
 class Player;
 struct FVector;

 class IGameObjectContainer
 {
 public:
  IGameObjectContainer(const int mapSize) : MAP_SIZE(mapSize){}
  //특정 위치에 플레이어를 추가한다. 
  virtual void AddPlayer(FVector location, std::shared_ptr<psh::Player>& target) = 0;
  
  virtual void Iterate(FVector radius , const std::function<void(psh::Player&)>& toInvoke) =0;

  virtual void Iterate(psh::FVector start, psh::FVector end  , const std::function<void(psh::Player&)>& toInvoke) = 0;


  virtual void MovePlayer(psh::FVector oldLocation, psh::FVector newLocation, std::shared_ptr<psh::Player>& target)  = 0;
  virtual ~IGameObjectContainer() = default;
  
  IGameObjectContainer(const IGameObjectContainer& other) = delete;
  IGameObjectContainer(IGameObjectContainer&& other) noexcept = delete;
  IGameObjectContainer& operator=(const IGameObjectContainer& other) = delete;
  IGameObjectContainer& operator=(IGameObjectContainer&& other) noexcept = delete;

 protected:
   const short MAP_SIZE = 6400;
 };

}
