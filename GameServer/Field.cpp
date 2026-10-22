/*
 * Field.cpp
 *
 * 서버 내에서 필드(맵) 로직을 담당하는 클래스 구현
 *
 * 파일: Field.cpp
 * 설명: 게임 내 플레이어, 몬스터, 아이템 등 필드 상의 오브젝트 관리 및 처리
 * 작성자: [작성자명]
 * 날짜: [작성일]
 */

#include "Field.h"
#include <memory>
#include "Profiler.h"
#include "Logger.h"
#include "DBData.h"
#include "DBThreadWrapper.h"
#include "GameObject.h"
#include "IVictimSelect.h"
#include "Player.h"
#include "Server.h"
#include "TableData.h"
#include "Monster.h"
#include "MonsterSpawner.h"
#include "Rand.h"

// 필드 클래스 생성자 및 소멸자
psh::Field::Field(Server& server, const ServerInitData& data, const ServerType type, short mapSize
                  , short sectorSize)
	: _dbCompAlert{std::make_unique<SPSCQueue<std::function<void()>, 1024>>()}
	  , _dbThread{
		  std::make_unique<DBThreadWrapper>(*_dbCompAlert, data.gameDBIP.c_str(), data.gameDBPort, data.gameDBID.c_str()
		                                    , data.gameDBPwd.c_str(), "mydb")
	  }
	  , _server(server)
	  , _initData(data)
	  , _groupType(type)
	  , _playerMap{std::make_unique<MapType>(mapSize, sectorSize)}
	  , _monsterMap{std::make_unique<MapType>(mapSize, sectorSize)}
	  , _itemMap{std::make_unique<MapType>(mapSize, sectorSize)}
	  , _victimSelect{victim_select::GetVictimByServerType(type)}
	  , _prevUpdate(std::chrono::steady_clock::now())
	  , _logger{std::make_unique<Logger>(std::format(L"field_{}", static_cast<int>(type)).c_str())}
	  , _spawner{std::move(MonsterSpawner::GetSpawner(type, *this, *_monsterMap))}
	  , _fieldSize(mapSize)
{
}

psh::Field::~Field() = default;

// 클라이언트가 필드에 입장할 때 호출
void psh::Field::OnEnter(SessionID id)
{
	PRO_FUNCTION();
	auto dataPtr = _server.GetDbData(id);
	if (static_cast<ServerType>(dataPtr->ServerType()) != _groupType)
	{
		dataPtr->ServerType(static_cast<char>(_groupType));

		const auto fieldSize = static_cast<float>(_fieldSize);
		dataPtr->Location({RandomUtil::Rand(0.0, fieldSize), RandomUtil::Rand(0.0, fieldSize)});
	}


	{
		PRO_BEGIN("PlayerGen other");
		//TODO: update 안 하는 gameObject로 만들고 OnCreate에서 처리하게 만들 수 있음.
		// 플레이어 객체 생성 및 세션 등록
		GameObjectData initData{dataPtr->Location(), {0, 0}, 200.0, eObjectType::Player, dataPtr->CharacterType()};
		auto playerPtr = std::make_shared<Player>(*this, initData, dataPtr, _dbThread.get());
		playerPtr->ObjectId(NextObjectId());

		// 플레이어 레벨 진입 패킷 생성 및 전송
		auto levelInfoPacket = SendBuffer::Alloc();
		MakeGame_ResLevelEnter(levelInfoPacket, playerPtr->AccountNumber(), playerPtr->ObjectId(), _groupType);
		SendPacket(playerPtr->SessionId(), levelInfoPacket);

		{
			PRO_BEGIN("PlayerInsert");
			_players.insert({id, std::move(playerPtr)});
		}
	}
}

// 클라이언트가 필드에서 퇴장할 때 호출
void psh::Field::OnLeave(const SessionID id, int wsaErrCode)
{
	PRO_FUNCTION();
	const auto it = _players.find(id);
	const PlayerRef playerPtr = it->second;

	if (wsaErrCode == 121)
	{
		_logger->Write(L"ServerLeave", Logger::LogLevel::Debug, L"Session Leave with 121:%d "
		               , playerPtr->AccountNumber());
	}

	if (playerPtr->Valid())
	{
		playerPtr->RemoveReason(removeReason::Disconnect);
		DestroyActor(playerPtr);
	}

	_players.erase(it);
}

void psh::Field::ProcessDatabaseAlerts()
{
	PRO_BEGIN("Database Alert");
	std::function<void()> job;
	while (_dbCompAlert->Dequeue(job))
	{
		try
		{
			job();
		}
		catch (const std::exception& e)
		{
			std::cerr << "Exception: " << e.what() << std::endl;
		} catch (...)
		{
			std::cerr << "Unknown exception caught." << std::endl;
		}
	}
}

// 필드 내 상태 업데이트
void psh::Field::OnUpdate(const int milli)
{
	UpdateContent(milli);

	InsertWaitObjectInMap();

	CleanupDeleteWait();


	//_fps++;

	if (_spawner)
	{
		_spawner->Update(milli);
	}

	ProcessDatabaseAlerts();
}

// 클라이언트로부터 받은 패킷 처리
void psh::Field::OnRecv(const SessionID id, RecvBuffer& recvBuffer)
{
	PRO_FUNCTION();
	ePacketType type;
	recvBuffer >> type;
	auto& [_, playerPtr] = *_players.find(id);

	switch (type)
	{
	case eGame_ReqChangeComplete:
		RecvChangeComp(id, recvBuffer);
		break;
	case eGame_ReqMove:
		RecvMove(id, recvBuffer);
		break;
	case eGame_ReqAttack:
		RecvAttack(id, recvBuffer);
		break;
	case eGame_ReqLevelEnter:
		RecvReqLevelChange(id, recvBuffer);
		break;
	case eGame_ReqChat:
		RecvChat(id, recvBuffer);
		break;
	default:
		psh::util::DebugBreak();
		break;
	}
}

// 클라이언트의 레벨 변경 요청을 처리
void psh::Field::RecvReqLevelChange(const SessionID id, RecvBuffer& recvBuffer)
{
	AccountNo accountNo;
	ServerType type;
	GetGame_ReqLevelEnter(recvBuffer, accountNo, type);
	auto [_, playerPtr] = *_players.find(id);

	if (playerPtr->Valid())
	{
		auto obj = std::static_pointer_cast<GameObject>(playerPtr);

		obj->RemoveReason(removeReason::GroupChange);
		playerPtr->_nextGroup = _server.GetGroupId(type);
		DestroyActor(obj);
	}
	else
	{
		MoveField(id, _server.GetGroupId(type));
	}
}

// 플레이어의 위치 변경 완료를 처리
void psh::Field::RecvChangeComp(const SessionID id, RecvBuffer& recvBuffer)
{
	PRO_FUNCTION();
	AccountNo accountNo;
	GetGame_ReqChangeComplete(recvBuffer, accountNo);
	auto& [_, playerPtr] = *_players.find(id);

	if (playerPtr->IsDead())
	{
		playerPtr->Revive();
	}
	AddActor(std::static_pointer_cast<GameObject>(playerPtr));
}

// 플레이어의 채팅 메시지 처리
void psh::Field::RecvChat(const SessionID id, RecvBuffer& recvBuffer)
{
	PRO_FUNCTION();
	auto& [_, player] = *_players.find(id);
	String chatData;
	GetGame_ReqChat(recvBuffer, chatData);

	if (player == nullptr)
	{
		_logger->Write(L"Disconnect", Logger::LogLevel::Invalid, L"recvChat. Not Found Player. SessionID : %d", id);
		_iocp->DisconnectSession(id);
		return;
	}

	if (player->Valid() == false)
	{
		return;
	}

	auto chatBuffer = SendBuffer::Alloc();
	MakeGame_ResChat(chatBuffer, player->ObjectId(), chatData);
	BroadcastToPlayer(player->Location(), {chatBuffer});
}

// 플레이어 이동 요청 처리
void psh::Field::RecvMove(const SessionID sessionId, RecvBuffer& buffer)
{
	PRO_FUNCTION();
	auto& [_, player] = *_players.find(sessionId);

	FVector location{};
	GetGame_ReqMove(buffer, location);

	if (player == nullptr)
	{
		_logger->Write(L"Disconnect", Logger::LogLevel::Invalid, L"recvMove. Not Found Player. SessionID : %d"
		               , sessionId);
		_iocp->DisconnectSession(sessionId);
		return;
	}

	//더미는 자기가 죽었는지 모름
	if (player->Valid() == false)
	{
		auto moveBuffer = SendBuffer::Alloc();
		MakeGame_ResMove(moveBuffer, player->ObjectId(), player->ObjectType(), player->Location());
		SendPacket(player->SessionId(), moveBuffer);
		return;
	}

	_playerMap->ClampLocationToMap(location);

	//더미 로직에 따라 정확히 같은 위치 이동을 요청하기도 함
	//이 경우 nan이 떠서 따로 처리해줌.
	//실제로 옮기진 않고 받았다고 응답만 함.
	if (location == player->Location())
	{
		auto moveBuffer = SendBuffer::Alloc();
		MakeGame_ResMove(moveBuffer, player->ObjectId(), player->ObjectType(), player->Location());
		SendPacket(player->SessionId(), moveBuffer);
		return;
	}
	{
		PRO_BEGIN("MoveStart");
		player->MoveStart(location);
	}
}

// 플레이어 공격 요청 처리
void psh::Field::RecvAttack(const SessionID sessionId, RecvBuffer& buffer)
{
	PRO_FUNCTION();
	auto& [_, player] = *_players.find(sessionId);
	char type;
	FVector dir{};
	GetGame_ReqAttack(buffer, type, dir);

	if (player == nullptr)
	{
		_logger->Write(L"Disconnect", Logger::LogLevel::Invalid, L"recvAttack. Not Found Player. SessionID : %d"
		               , sessionId);
		return;
	}

	//더미는 자기가 죽었는지 모름. 패킷 검증 위해서 더미에게만 전송
	if (player->Valid() == false)
	{
		auto attackBuffer = SendBuffer::Alloc();
		MakeGame_ResAttack(attackBuffer, player->ObjectId(), type, dir);
		SendPacket(player->SessionId(), attackBuffer);
		return;
	}

	//더미가 생각할 때 쿨타임이 끝났는데 서버가 생각할 땐 아닌 경우. 이것도 패킷 검증 위한 것
	if (player->IsCooldownEndDebug(type) == false)
	{
		_logger->Write(L"RecvAttack", Logger::LogLevel::Invalid, L"recvAttack. not cooldown end. Obj : %d",
		               player->ObjectId());
		auto attackBuffer = SendBuffer::Alloc();
		MakeGame_ResAttack(attackBuffer, player->ObjectId(), type, dir);
		SendPacket(player->SessionId(), attackBuffer);
	}

	player->Attack(type, dir);
}

// 특정 위치의 플레이어에게 데이터를 브로드캐스트
void psh::Field::BroadcastToPlayer(FVector targetLocation, const std::vector<SendBuffer>& packets)
{
	PRO_FUNCTION();
	for (auto view = GetObjectView(ViewObjectType::Player, targetLocation, SEND_OFFSETS::BROADCAST);
	     const auto& player : view)
	{
		for (const SendBuffer& packet : packets)
		{
			std::static_pointer_cast<psh::Player>(player)->SendPacket(packet);
		}
	}
}

// 몬스터 생성 및 필드에 추가
void psh::Field::SpawnMonster(const shared<Monster>& obj)
{
	AddActor(std::static_pointer_cast<GameObject>(obj));
}

void psh::Field::MoveField(SessionID id, GroupID gid)
{
	MoveSession(id, gid);
}

// 필드가 생성될 때 초기화
void psh::Field::OnCreate()
{
	String extraName = L"Group";
	extraName += std::to_wstring(static_cast<int>(static_cast<long>(GetGroupID())));
	_logger = std::make_unique<Logger>(extraName.c_str());
}

// 공격 처리 로직
psh::AttackResult psh::Field::ProcessAttack(AttackInfo info)
{
	PRO_FUNCTION();
	return _victimSelect(*this, info);
}

// 특정 오브젝트 타입에 대한 시야 반환
psh::Field::ViewType psh::Field::GetObjectView(ViewObjectType type, const FVector& location
                                               , std::span<const Sector> offsets)
{
	PRO_FUNCTION();
	std::vector<MapType::SectorView> returnView;

	if ((type & ViewObjectType::Player) == ViewObjectType::Player)
	{
		returnView.push_back(_playerMap->CreateSectorViewWithOffsets(location, offsets));
	}
	if ((type & ViewObjectType::Monster) == ViewObjectType::Monster)
	{
		returnView.push_back(_monsterMap->CreateSectorViewWithOffsets(location, offsets));
	}
	if ((type & ViewObjectType::Item) == ViewObjectType::Item)
	{
		returnView.push_back(_itemMap->CreateSectorViewWithOffsets(location, offsets));
	}

	return MapType::SectorView{returnView};
}

// 필드 내 특정 위치에서 시야 반환(구현되지 않음)
psh::Field::ViewType psh::Field::GetObjectViewByPoint(ViewObjectType type, const std::list<FVector>& coordinate)
{
	PRO_FUNCTION();
	std::vector<MapType::SectorView> returnView;

	if ((type & ViewObjectType::Player) == ViewObjectType::Player)
	{
		returnView.push_back(_playerMap->CreateSectorViewFromLocations(coordinate));
	}
	if ((type & ViewObjectType::Monster) == ViewObjectType::Monster)
	{
		returnView.push_back(_monsterMap->CreateSectorViewFromLocations(coordinate));
	}
	if ((type & ViewObjectType::Item) == ViewObjectType::Item)
	{
		returnView.push_back(_itemMap->CreateSectorViewFromLocations(coordinate));
	}

	return MapType::SectorView{returnView};
}

// 오브젝트를 대기 목록에 추가
void psh::Field::AddActor(const shared<GameObject>& obj)
{
	_createWaits.push_back(obj);
}

// 오브젝트 삭제 대기 목록에 추가
void psh::Field::DestroyActor(shared<GameObject>& obj)
{
	if (obj->Valid() == false)
	{
		_logger->Write(L"DoubleDelete", Logger::LogLevel::Invalid
		               , L"DoubleDelete. type : %d, remove reason : %d, templateID : %d, field : %d"
		               , obj->ObjectType(), obj->RemoveReason(), obj->TemplateId(), _groupType);
		return;
	}

	obj->Valid(false);
	_delWaits.emplace_back(obj);
}

// 오브젝트 삭제 대기 목록에 추가 (rvalue 참조)
void psh::Field::DestroyActor(shared<GameObject>&& obj)
{
	DestroyActor(obj);
}

// 대기 중인 오브젝트를 필드에 삽입
void psh::Field::InsertWaitObjectInMap()
{
	PRO_FUNCTION();
	while (!_createWaits.empty())
	{
		auto obj = _createWaits.front();
		_createWaits.pop_front();
		_objects.insert(obj);

		if (obj->ObjectType() != eObjectType::Player)
		{
			obj->ObjectId(NextObjectId());
		}

		const auto map = FindObjectMap(obj);
		obj->IntoMap(map);

		obj->OnCreate();
	}
}

// 오브젝트 타입에 따라 적절한 맵 반환
psh::GameMap<psh::ObjectID, psh::shared<psh::GameObject>>* psh::Field::FindObjectMap(
	const shared<GameObject>& obj) const
{
	switch (obj->ObjectType())
	{
	case eObjectType::Player:
		return _playerMap.get();
	case eObjectType::Monster:
		return _monsterMap.get();
	case eObjectType::Item:
		return _itemMap.get();
	default:
		ASSERT_CRASH(false, L"InvalidType");
		return nullptr;
	}
}

// 삭제 대기 중인 오브젝트 정리
void psh::Field::CleanupDeleteWait()
{
	PRO_FUNCTION();
	while (!_delWaits.empty())
	{
		auto obj = _delWaits.front();
		_delWaits.pop_front();
		obj->OnDestroy();

		if (const auto map = obj->GetMap();
			map != nullptr)
		{
			map->Delete(obj->ObjectId(), obj->Location());
		}
		_objects.erase(obj);
	}
}

// 다음 오브젝트 ID 반환
psh::ObjectID psh::Field::NextObjectId()
{
	return ++_objectId;
}

// 필드 내 오브젝트 업데이트
void psh::Field::UpdateContent(const int deltaMs)
{
	PRO_FUNCTION();
	for (auto& obj : _objects)
	{
		if (obj->Valid())
		{
			obj->Update(deltaMs);
		}
	}
}

// ViewObjectType 비트 연산자 오버로딩
psh::Field::ViewObjectType psh::operator|(Field::ViewObjectType lhs, Field::ViewObjectType rhs)
{
	using enumType = std::underlying_type_t<Field::ViewObjectType>;
	return static_cast<Field::ViewObjectType>(static_cast<enumType>(lhs) | static_cast<enumType>(rhs));
}

psh::Field::ViewObjectType psh::operator&(Field::ViewObjectType lhs, Field::ViewObjectType rhs)
{
	using enumType = std::underlying_type_t<Field::ViewObjectType>;
	return static_cast<Field::ViewObjectType>(static_cast<enumType>(lhs) & static_cast<enumType>(rhs));
}
