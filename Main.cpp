#include <WinSock2.h>
#include <iostream>
#include <vector>
#include <Windows.h>
#include <ctime>
#include "Protocol.h"
#pragma comment(lib,"ws2_32")
using namespace std;
#define SERVERPORT 9595
#define BUFSIZE 512
//-----------전역변수들----------//
SOCKET global_client_sock[2];               // 클라이언트 소켓 변수
vector<PlayerInfo> playerVector;
vector<MonsterInfo> monsterVector;
vector<BulletInfo> bulletVector;
vector<ContainerInfo> conVector;
SceneInfo changeGameScene;                  //8byte 전송.  
InitInfo initInform;
HANDLE hEventMonsterUpdate;                  //이벤트 핸들
HANDLE hEventPlayerThread1;                  //이벤트 핸들.
HANDLE hEventPlayerThread2;
SendPacket sendPacket;                        //Send packet
double leftTime = (100 / 6);               //FrameTime 조절 변수
int player1KillCount = 0;
int player2KillCount = 0;
HANDLE hThreadHandle1;
HANDLE hThreadHandle2;
bool gameOver;
//-----------함수 선언----------//
DWORD WINAPI ThreadFunc1(LPVOID);
DWORD WINAPI ThreadFunc2(LPVOID);
void Init();
void MonsterUpdate();
bool CollisionBulletWithMonster(Vec3&, Vec3&);                        // 총알하고 몬스터 충돌함수
bool CollisionBulletWithMap(Vec3&);                                    // 맵전체하고 총알
bool CollisionBulletWithObstacle(Vec3&bullet, Vec3& container);         // 총알하고 장애물 충돌함수
bool CollisionMapWithPlayer(Vec2 &);
bool CollisionContainerWithPlayer(Vec2 &);
int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKADDR_IN server_addr;   // 서버 소켓 주소 구조체
	SOCKADDR_IN client_addr;   // 클라 소켓 주소 구조체
	SOCKET listen_sock;         // Listen Socket 변수
	int retval;               // return Value 값 받는 변수
	int ClientNum = 0;         // 클라 몇개 접속했는지
	int addrlen;            // 주소 길이
	int playerIndex;


	char buf[BUFSIZE];

	//윈속 초기화 작업
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	//Socket()
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
		err_quit("listen_sock()");

	//Bind()
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR*)&server_addr, sizeof(server_addr));
	if (retval == SOCKET_ERROR)
		err_quit("Bind()");

	//Listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
		err_quit("Listen()");
	int option = true;

	while (true)
	{
		ClientNum = 0;
		gameOver = false;
		printf("호롤롤롤ㄹ \n");
		while (true)
		{
			//Accept
			addrlen = sizeof(client_addr);
			global_client_sock[ClientNum] = accept(listen_sock, (SOCKADDR*)&client_addr, &addrlen);
			if (global_client_sock[ClientNum] == INVALID_SOCKET)
				err_quit("client_socket");
			printf("%d 번째 클라정보 : IP : %s, 포트번호 : %d \n", ClientNum + 1, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
			if (ClientNum < 2)
				ClientNum++;
			if (ClientNum == 2)
			{
				ClientNum++;
				break;
			}
		}


		gameOver = false;
		player1KillCount = 0;
		player2KillCount = 0;
		setsockopt(global_client_sock[0], IPPROTO_TCP, TCP_NODELAY, (const char*)&option, sizeof(option));
		setsockopt(global_client_sock[1], IPPROTO_TCP, TCP_NODELAY, (const char*)&option, sizeof(option));

		//쓰레드 생성.
		hThreadHandle1 = CreateThread(
			NULL, 0, ThreadFunc1, (LPVOID)global_client_sock[0], CREATE_SUSPENDED, NULL);

		hThreadHandle2 = CreateThread(
			NULL, 0, ThreadFunc2, (LPVOID)global_client_sock[1], CREATE_SUSPENDED, NULL);

		//이벤트 생성
		hEventMonsterUpdate = CreateEvent(NULL, FALSE, FALSE, 0);
		if (hEventMonsterUpdate == nullptr)
			cout << "hEventMonsterUpdate 생성 에러" << endl;
		hEventPlayerThread1 = CreateEvent(NULL, FALSE, FALSE, 0);
		if (hEventPlayerThread1 == nullptr)
			cout << "hEventPlayer1Thread 생성 에러" << endl;
		hEventPlayerThread2 = CreateEvent(NULL, FALSE, TRUE, 0);
		if (hEventPlayerThread2 == nullptr)
			cout << "hEventPlayer1Thread 생성 에러" << endl;

		Init();         //데이터들 초기화

		cout << "초기값 전송" << std::endl;
		for (int i = 0; i < 2; ++i)
		{
			retval = send(global_client_sock[i], (char*)&initInform, sizeof(InitInfo), 0);         //게임 초기값 전송.
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
			}
			initInform.playerIndex += 1;
		}
		cout << "초기값 전송 완료." << std::endl;

		std::cout << "준비완료 메세지 수신" << std::endl;         // 아직은 첫번째 클라 두번째 클라 순으로 받아야함
		for (int i = 0; i < 2; ++i)                           // 차후 수정 필요.
		{
			retval = recv(global_client_sock[i], buf, sizeof(buf), 0);         //준비완료 메세지 수신. 수신사이즈 생각해야됨.
			if (retval == SOCKET_ERROR)
			{
				err_display("recv()");
			}
		}
		std::cout << "준비완료 메세지 수신 완료" << std::endl;

		std::cout << "씬전환 메세지 송신" << std::endl;

		changeGameScene.type = DataType::SCENE;
		changeGameScene.SceneState = SceneList::INGAME;

		for (int i = 0; i < 2; ++i)            //send - 게임씬으로 넘어가라는 메세지 전송
		{
			retval = send(global_client_sock[i], (char*)&changeGameScene, sizeof(SceneInfo), 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
			}
		}
		std::cout << "씬전환 메세지 보내기 완료" << std::endl;

		//OK

		//ResumeThread 해주기전에 게임 초기값들 전송해주고 게임준비완료 되면 ResumeThread 해준다.
		ResumeThread(hThreadHandle1);
		ResumeThread(hThreadHandle2);

		//MontserPosUpdate()

		MonsterUpdate();


		cout << "Game End" << endl;
		playerVector.clear();
		conVector.clear();
		monsterVector.clear();


		WaitForSingleObject(hThreadHandle1, INFINITE);
		WaitForSingleObject(hThreadHandle2, INFINITE);

		CloseHandle(hThreadHandle1);
		CloseHandle(hThreadHandle2);
		cout << "쓰레드 2개 종료!" << endl;
		CloseHandle(hEventMonsterUpdate);
		CloseHandle(hEventPlayerThread1);
		CloseHandle(hEventPlayerThread2);


		//closesocket(listen_sock);
		closesocket(global_client_sock[0]);
		closesocket(global_client_sock[1]);
	}
	return 0;
}
void Init()
{
	playerVector.reserve(2);
	playerVector.push_back(PlayerInfo{ DataType::PLAYER,1,Vec3{ -1500,50,1900 },Vec3{ 0,0,0 },false });
	playerVector.push_back(PlayerInfo{ DataType::PLAYER,2,Vec3{ 1500,50,1900 },Vec3{ 0,0,0 },false });

	// 컨테이너 위치값 들 서버에 초기화 -> 충돌체크에 사용될것/
	conVector.reserve(4);
	conVector.push_back(ContainerInfo{ Vec3{ 0,300,1000 } });
	conVector.push_back(ContainerInfo{ Vec3{ 1500,300,0 } });
	conVector.push_back(ContainerInfo{ Vec3{ 0,300,-1000 } });
	conVector.push_back(ContainerInfo{ Vec3{ -1500,300,0 } });

	//Vec3  a{ 1,2,3 };
	int hp = 3;
	// 몬스터들 위치값 초기화
	monsterVector.reserve(10);
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ -1000,50,-1800 } ,3,0 });
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ -1000,50,-1000 } ,3,1 });
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ 1000,50,-1800 } ,3,2 });
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ 1000,50,-1000 } ,3,3 });
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ -500,50,-500 } ,3,4 });
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ 0,50,0 } ,3,5 });
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ 500,50,500 } ,3,6 });
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ -1000,50,1000 } ,3,7 });
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ 1000,50,1000 } ,3,8 });
	monsterVector.push_back(MonsterInfo{ DataType::MONSTER,Vec3{ 0,50,1500 } ,3,9 });

	//양클라이언트에 보낼 초기값들에 초기화.
	// Player1,2

	initInform.Player1Pos = Vec3{ -1500, 50, 1900 };
	initInform.Player2Pos = Vec3{ 1500, 50, 1900 };

	//Monsters
	initInform.MonsterPos[0] = Vec3{ -1000,50,-1800 };
	initInform.MonsterPos[1] = Vec3{ -1000,50,-1000 };
	initInform.MonsterPos[2] = Vec3{ 1000,50,-1800 };
	initInform.MonsterPos[3] = Vec3{ 1000,50,-1000 };
	initInform.MonsterPos[4] = Vec3{ -500,50,-500 };
	initInform.MonsterPos[5] = Vec3{ 0,50,0 };
	initInform.MonsterPos[6] = Vec3{ 500,50,500 };
	initInform.MonsterPos[7] = Vec3{ -1000,50,1000 };
	initInform.MonsterPos[8] = Vec3{ 1000,50,1000 };
	initInform.MonsterPos[9] = Vec3{ 0,50,1500 };

	initInform.playerIndex = 1;

	//Send,Recv전용 패킷 초기화
	for (int i = 0; i < 10; ++i)
	{
		sendPacket.MonstersPosition[i].x = monsterVector[i].MonsterPos.x;
		sendPacket.MonstersPosition[i].y = monsterVector[i].MonsterPos.y;
		sendPacket.MonstersPosition[i].z = monsterVector[i].MonsterPos.z;
	}
	sendPacket.player1.playerIndex = 1;
	sendPacket.player1.playerPos.x = playerVector[0].PlayerPos.x;
	sendPacket.player1.playerPos.z = playerVector[0].PlayerPos.z;
	sendPacket.player1.Killcount = 0;

	sendPacket.player2.playerIndex = 2;
	sendPacket.player2.playerPos.x = playerVector[1].PlayerPos.x;
	sendPacket.player2.playerPos.z = playerVector[1].PlayerPos.z;
	sendPacket.player2.Killcount = 0;

	sendPacket.gameOver = false;

}
void MonsterUpdate()
{
	float firstPosition[10];
	float monsterDirection[10];
	float moveRange = 300;
	int retval = 0;

	//몬스터들의 방향 랜덤하게 초기화.
	for (int i = 0; i < 10; ++i)
	{
		firstPosition[i] = monsterVector[i].MonsterPos.x;

		monsterDirection[i] = rand() % 2 - 1;
		if (monsterDirection[i] == 0)
			monsterDirection[i] = 1;
	}
	int monsterType = DataType::MONSTER;
	int bulletType = DataType::BULLET;
	int packetType = DataType::PACKET;
	double nextTime = 0.0f;
	double nowTime = clock();

	vector<BulletInfo>::iterator bulletIter;
	vector<MonsterInfo>::iterator monsterIter = monsterVector.begin();
	vector<ContainerInfo>::iterator conIter = conVector.begin();

	while (true)
	{
		WaitForSingleObject(hEventPlayerThread2, INFINITE);
		nowTime = std::clock();
		if (nowTime > nextTime)
		{
			//GameOver 조건
			if ((player1KillCount + player2KillCount) == 10)
			{
				gameOver = true;
				sendPacket.gameOver = true;
				for (int i = 0; i < 2; ++i)
				{
					retval = send(global_client_sock[i], (char*)&sendPacket, sizeof(SendPacket), 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("send()");
					}
				}
				SetEvent(hEventMonsterUpdate);
				cout << "MonsterUPdate REturn" << endl;
				return;
			}

			//몬스터 위치 업데이트
			for (int i = 0; i < 10; ++i)
			{
				if (monsterVector[i].MonsterPos.x < firstPosition[i] - moveRange)
					monsterDirection[i] *= -1;
				if (monsterVector[i].MonsterPos.x > firstPosition[i] + moveRange)
					monsterDirection[i] *= -1;
			}
			for (int i = 0; i < 10; ++i)
			{
				monsterVector[i].MonsterPos.x += (5 * monsterDirection[i]);
				sendPacket.MonstersPosition[i].x = monsterVector[i].MonsterPos.x;
				sendPacket.MonstersPosition[i].y = monsterVector[i].MonsterPos.y;
				sendPacket.MonstersPosition[i].z = monsterVector[i].MonsterPos.z;
			}

			//총알 위치 업데이트
			if (bulletVector.size() != 0)
			{
				for (bulletIter = bulletVector.begin(); bulletIter != bulletVector.end(); ++bulletIter)
				{
					bulletIter->BulletPos.x += bulletIter->BulletDirection.x * (float)20;
					bulletIter->BulletPos.y += bulletIter->BulletDirection.y * (float)20;
					bulletIter->BulletPos.z += bulletIter->BulletDirection.z * (float)20;
				}
			}
			//총알과 몬스터 충돌체크

			if (bulletVector.size() != 0)
			{
				for (monsterIter = monsterVector.begin(); monsterIter != monsterVector.end(); ++monsterIter)
				{
					for (bulletIter = bulletVector.begin(); bulletIter != bulletVector.end(); ++bulletIter)
					{
						if (CollisionBulletWithMonster(bulletIter->BulletPos, monsterIter->MonsterPos))
						{
							cout << "몬스터 충돌 " << endl;
							monsterIter->hp -= 1;
							bulletVector.erase(bulletIter);
							bulletIter = bulletVector.begin();
							if (monsterIter->hp == 0)
							{
								if (bulletIter->bulletOwner == 1)
								{
									player1KillCount += 1;
									sendPacket.player1.Killcount = player1KillCount;
								}
								else if (bulletIter->bulletOwner == 2)
								{
									player2KillCount += 1;
									sendPacket.player2.Killcount = player2KillCount;
								}
								cout << "플킬1 : " << player1KillCount << endl;
								cout << "플킬2 : " << player2KillCount << endl;
								monsterIter->MonsterPos.y = -300;
							}

							if (bulletVector.size() == 0)
								break;
						}
					}
				}
			}
			//총알과 장애물 충돌체크

			//cout << "BulletVEctor 3 : " << bulletVector.size() << endl;
			if (bulletVector.size() != 0)
			{

				for (conIter = conVector.begin(); conIter != conVector.end(); ++conIter)
				{
					for (bulletIter = bulletVector.begin(); bulletIter != bulletVector.end(); ++bulletIter)
					{
						if (CollisionBulletWithObstacle(bulletIter->BulletPos, conIter->position))
						{
							bulletVector.erase(bulletIter);
							bulletIter = bulletVector.begin();
							if (bulletVector.size() == 0)
								break;
						}
					}
				}
			}

			//cout << "BulletVEctor : 4" << bulletVector.size() << endl;
			//총알과 맵 충돌체크
			if (bulletVector.size() != 0)
			{

				for (bulletIter = bulletVector.begin(); bulletIter != bulletVector.end(); ++bulletIter)
				{
					if (CollisionBulletWithMap(bulletIter->BulletPos))
					{
						bulletVector.erase(bulletIter);
						bulletIter = bulletVector.begin();
						if (bulletVector.size() == 0)
							break;
					}
				}
			}
			if ((player1KillCount + player2KillCount) == 10)
			{
				changeGameScene.type = DataType::SCENE;
				changeGameScene.SceneState = SceneList::RESULT;
				for (int i = 0; i < 2; ++i)
				{
					retval = send(global_client_sock[i], (char*)&changeGameScene, sizeof(SceneInfo), 0);

				}
			}

			//내일 수정


			for (int i = 0; i < 2; ++i)
			{
				retval = send(global_client_sock[i], (char*)&sendPacket, sizeof(SendPacket), 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("send1()");
				}
			}

			nextTime = clock() + leftTime;

		}
		SetEvent(hEventMonsterUpdate);
	}
}

DWORD WINAPI ThreadFunc1(LPVOID param)
{
	cout << "쓰레드 시작!" << endl;
	SOCKET client_sock = (SOCKET)param;
	int retval;         //return value
	char buf[BUFSIZE];
	int dataType = 0;

	RecvPacket recvPacket;

	int playerIndex;

	double nextTime = 0.0f;
	double nowTime = clock();

	while (true)            //이 함수내에서 send,recv 작업이 이루어짐.
	{
		WaitForSingleObject(hEventMonsterUpdate, INFINITE);
		if (gameOver)
		{
			SetEvent(hEventPlayerThread1);
			cout << "ThreadFunc1 Return" << endl;
			return 0;
		}
		nowTime = std::clock();
		if (nowTime > nextTime)
		{

			retval = recv(client_sock, (char*)&recvPacket, sizeof(RecvPacket), 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("recv");
			}

			// 맵 충돌
			//printf("Before : X : %f , z : %f \n", recvPacket.playerPos.x, recvPacket.playerPos.z);
			CollisionMapWithPlayer(recvPacket.playerPos);
			CollisionContainerWithPlayer(recvPacket.playerPos);
			//printf("After  : X : %f , z : %f \n", recvPacket.playerPos.x, recvPacket.playerPos.z);
			//서버의 플레이어 벡터의 값 초기화
			playerVector[0].PlayerPos.x = recvPacket.playerPos.x;
			playerVector[0].PlayerPos.z = recvPacket.playerPos.z;

			//데이터패킷에 값 초기화
			sendPacket.player1.playerPos = recvPacket.playerPos;
			sendPacket.player1.playerCam = recvPacket.playerCam;

			if (recvPacket.makeBullet)
			{
				BulletInfo tempInfo;
				tempInfo.BulletPos = recvPacket.bulletPosition;
				tempInfo.BulletDirection = recvPacket.bulletDirection;
				tempInfo.bulletOwner = 1;

				bulletVector.push_back(tempInfo);
			}

			nextTime = clock() + leftTime;
		}
		SetEvent(hEventPlayerThread1);
	}
	return 0;
}
DWORD WINAPI ThreadFunc2(LPVOID param)
{
	cout << "쓰레드 시작!" << endl;
	SOCKET client_sock = (SOCKET)param;
	int retval;         //return value
	char buf[BUFSIZE];
	int dataType = 0;

	double nextTime = 0.0f;
	double nowTime = clock();
	RecvPacket recvPacket;

	while (true)            //이 함수내에서 send,recv 작업이 이루어짐.
	{
		WaitForSingleObject(hEventPlayerThread1, INFINITE);
		if (gameOver)
		{
			cout << "ThreadFunc2 Return" << endl;
			return 0;
		}
		nowTime = std::clock();
		if (nowTime > nextTime)
		{
			//Player2 로부터 데이터 받아옴.
			retval = recv(client_sock, (char*)&recvPacket, sizeof(RecvPacket), 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("recv");
			}

			// 맵 충돌
			CollisionMapWithPlayer(recvPacket.playerPos);
			CollisionContainerWithPlayer(recvPacket.playerPos);
			// 서버의 플레이어벡터의 값 초기화
			playerVector[1].PlayerPos.x = recvPacket.playerPos.x;
			playerVector[1].PlayerPos.z = recvPacket.playerPos.z;

			//샌드 데이터패킷의 값 초기화.
			sendPacket.player2.playerPos = recvPacket.playerPos;
			sendPacket.player2.playerCam = recvPacket.playerCam;

			if (recvPacket.makeBullet)
			{
				BulletInfo tempInfo;
				tempInfo.BulletPos = recvPacket.bulletPosition;
				tempInfo.BulletDirection = recvPacket.bulletDirection;
				tempInfo.bulletOwner = 2;

				bulletVector.push_back(tempInfo);
			}

			nextTime = clock() + leftTime;
		}
		SetEvent(hEventPlayerThread2);
	}
	return 0;
}
bool CollisionBulletWithMonster(Vec3 & bullet, Vec3& monster)
{
	return(
		bullet.x + 1 > monster.x - 30 &&
		bullet.x - 1 < monster.x + 30 &&
		bullet.y + 1 > monster.y - 55 &&
		bullet.y - 1 < monster.y + 55 &&
		bullet.z + 1 > monster.z - 15 &&
		bullet.z - 1 < monster.z + 15
		);
}
bool CollisionBulletWithObstacle(Vec3 &bullet, Vec3& container)
{
	return(
		bullet.x + 1 > container.x - 128 &&
		bullet.x - 1 < container.x + 128 &&
		bullet.y + 1 > container.y - 128 &&
		bullet.y - 1 < container.y + 128 &&
		bullet.z + 1 > container.z - 256 &&
		bullet.z - 1 < container.z + 256
		);
}
bool CollisionBulletWithMap(Vec3& bullet)
{
	if (bullet.x + 1 > 2000)
		return true;
	if (bullet.x - 1 < -2000)
		return true;
	if (bullet.z + 1 > 2000)
		return true;
	if (bullet.z - 1 < -2000)
		return true;

	return false;
}
bool CollisionObstacleWithPlayer(PlayerInfo&player, ContainerInfo& container)
{
	if (player.PlayerPos.x + 30 > container.position.x - 128 &&
		player.PlayerPos.x - 30 < container.position.x + 128 &&
		player.PlayerPos.z + 15 > container.position.z - 256 &&
		player.PlayerPos.z - 15 < container.position.z + 256)
	{
		return true;;
	}

	//트루면 이전값으로 돌려놓기.
}
bool CollisionMapWithPlayer(Vec2 & playerPos)
{
	if (playerPos.x + 30 > 2000)
	{
		playerPos.x -= 30;
		return true;
	}
	if (playerPos.x - 30 < -2000)
	{
		playerPos.x += 30;
		return true;
	}
	if (playerPos.z + 30 > 2000)
	{
		playerPos.z -= 30;
		return true;
	}
	if (playerPos.z - 30 < -2000)
	{
		playerPos.z += 30;
		return true;
	}
	return false;
}

bool CollisionContainerWithPlayer(Vec2 &playerPos)
{
	if (playerPos.x > -100.0f && playerPos.x < 100.0f &&
		playerPos.z > 750.0f && playerPos.z < 1250.0f)
	{
		playerPos.x = 0.0f;
		playerPos.z = 1900.0f;
		return true;
	}

	if (playerPos.x > 1400.0f && playerPos.x < 1600.0f &&
		playerPos.z > -250.0f && playerPos.z < 250.0f)
	{
		playerPos.x = 0.0f;
		playerPos.z = 1900.0f;
		return true;
	}

	if (playerPos.x > -100.0f && playerPos.x < 100.0f &&
		playerPos.z > -1250.0f && playerPos.z < -750.0f)
	{
		playerPos.x = 0.0f;
		playerPos.z = 1900.0f;
		return true;
	}

	if (playerPos.x > -1600.0f && playerPos.x < -1400.0f &&
		playerPos.z > -250.0f && playerPos.z < 250.0f)
	{
		playerPos.x = 0.0f;
		playerPos.z = 1900.0f;
		return true;
	}

	return false;
}