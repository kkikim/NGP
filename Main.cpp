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
//-----------����������----------//
SOCKET global_client_sock[2];               // Ŭ���̾�Ʈ ���� ����
vector<PlayerInfo> playerVector;
vector<MonsterInfo> monsterVector;
vector<BulletInfo> bulletVector;
vector<ContainerInfo> conVector;
SceneInfo changeGameScene;                  //8byte ����.  
InitInfo initInform;
HANDLE hEventMonsterUpdate;                  //�̺�Ʈ �ڵ�
HANDLE hEventPlayerThread1;                  //�̺�Ʈ �ڵ�.
HANDLE hEventPlayerThread2;
SendPacket sendPacket;                        //Send packet
double leftTime = (100 / 6);               //FrameTime ���� ����
int player1KillCount = 0;
int player2KillCount = 0;
HANDLE hThreadHandle1;
HANDLE hThreadHandle2;
bool gameOver;
//-----------�Լ� ����----------//
DWORD WINAPI ThreadFunc1(LPVOID);
DWORD WINAPI ThreadFunc2(LPVOID);
void Init();
void MonsterUpdate();
bool CollisionBulletWithMonster(Vec3&, Vec3&);                        // �Ѿ��ϰ� ���� �浹�Լ�
bool CollisionBulletWithMap(Vec3&);                                    // ����ü�ϰ� �Ѿ�
bool CollisionBulletWithObstacle(Vec3&bullet, Vec3& container);         // �Ѿ��ϰ� ��ֹ� �浹�Լ�
bool CollisionMapWithPlayer(Vec2 &);
bool CollisionContainerWithPlayer(Vec2 &);
int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKADDR_IN server_addr;   // ���� ���� �ּ� ����ü
	SOCKADDR_IN client_addr;   // Ŭ�� ���� �ּ� ����ü
	SOCKET listen_sock;         // Listen Socket ����
	int retval;               // return Value �� �޴� ����
	int ClientNum = 0;         // Ŭ�� � �����ߴ���
	int addrlen;            // �ּ� ����
	int playerIndex;


	char buf[BUFSIZE];

	//���� �ʱ�ȭ �۾�
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
		printf("ȣ�ѷѷѤ� \n");
		while (true)
		{
			//Accept
			addrlen = sizeof(client_addr);
			global_client_sock[ClientNum] = accept(listen_sock, (SOCKADDR*)&client_addr, &addrlen);
			if (global_client_sock[ClientNum] == INVALID_SOCKET)
				err_quit("client_socket");
			printf("%d ��° Ŭ������ : IP : %s, ��Ʈ��ȣ : %d \n", ClientNum + 1, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
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

		//������ ����.
		hThreadHandle1 = CreateThread(
			NULL, 0, ThreadFunc1, (LPVOID)global_client_sock[0], CREATE_SUSPENDED, NULL);

		hThreadHandle2 = CreateThread(
			NULL, 0, ThreadFunc2, (LPVOID)global_client_sock[1], CREATE_SUSPENDED, NULL);

		//�̺�Ʈ ����
		hEventMonsterUpdate = CreateEvent(NULL, FALSE, FALSE, 0);
		if (hEventMonsterUpdate == nullptr)
			cout << "hEventMonsterUpdate ���� ����" << endl;
		hEventPlayerThread1 = CreateEvent(NULL, FALSE, FALSE, 0);
		if (hEventPlayerThread1 == nullptr)
			cout << "hEventPlayer1Thread ���� ����" << endl;
		hEventPlayerThread2 = CreateEvent(NULL, FALSE, TRUE, 0);
		if (hEventPlayerThread2 == nullptr)
			cout << "hEventPlayer1Thread ���� ����" << endl;

		Init();         //�����͵� �ʱ�ȭ

		cout << "�ʱⰪ ����" << std::endl;
		for (int i = 0; i < 2; ++i)
		{
			retval = send(global_client_sock[i], (char*)&initInform, sizeof(InitInfo), 0);         //���� �ʱⰪ ����.
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
			}
			initInform.playerIndex += 1;
		}
		cout << "�ʱⰪ ���� �Ϸ�." << std::endl;

		std::cout << "�غ�Ϸ� �޼��� ����" << std::endl;         // ������ ù��° Ŭ�� �ι�° Ŭ�� ������ �޾ƾ���
		for (int i = 0; i < 2; ++i)                           // ���� ���� �ʿ�.
		{
			retval = recv(global_client_sock[i], buf, sizeof(buf), 0);         //�غ�Ϸ� �޼��� ����. ���Ż����� �����ؾߵ�.
			if (retval == SOCKET_ERROR)
			{
				err_display("recv()");
			}
		}
		std::cout << "�غ�Ϸ� �޼��� ���� �Ϸ�" << std::endl;

		std::cout << "����ȯ �޼��� �۽�" << std::endl;

		changeGameScene.type = DataType::SCENE;
		changeGameScene.SceneState = SceneList::INGAME;

		for (int i = 0; i < 2; ++i)            //send - ���Ӿ����� �Ѿ��� �޼��� ����
		{
			retval = send(global_client_sock[i], (char*)&changeGameScene, sizeof(SceneInfo), 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
			}
		}
		std::cout << "����ȯ �޼��� ������ �Ϸ�" << std::endl;

		//OK

		//ResumeThread ���ֱ����� ���� �ʱⰪ�� �������ְ� �����غ�Ϸ� �Ǹ� ResumeThread ���ش�.
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
		cout << "������ 2�� ����!" << endl;
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

	// �����̳� ��ġ�� �� ������ �ʱ�ȭ -> �浹üũ�� ���ɰ�/
	conVector.reserve(4);
	conVector.push_back(ContainerInfo{ Vec3{ 0,300,1000 } });
	conVector.push_back(ContainerInfo{ Vec3{ 1500,300,0 } });
	conVector.push_back(ContainerInfo{ Vec3{ 0,300,-1000 } });
	conVector.push_back(ContainerInfo{ Vec3{ -1500,300,0 } });

	//Vec3  a{ 1,2,3 };
	int hp = 3;
	// ���͵� ��ġ�� �ʱ�ȭ
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

	//��Ŭ���̾�Ʈ�� ���� �ʱⰪ�鿡 �ʱ�ȭ.
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

	//Send,Recv���� ��Ŷ �ʱ�ȭ
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

	//���͵��� ���� �����ϰ� �ʱ�ȭ.
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
			//GameOver ����
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

			//���� ��ġ ������Ʈ
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

			//�Ѿ� ��ġ ������Ʈ
			if (bulletVector.size() != 0)
			{
				for (bulletIter = bulletVector.begin(); bulletIter != bulletVector.end(); ++bulletIter)
				{
					bulletIter->BulletPos.x += bulletIter->BulletDirection.x * (float)20;
					bulletIter->BulletPos.y += bulletIter->BulletDirection.y * (float)20;
					bulletIter->BulletPos.z += bulletIter->BulletDirection.z * (float)20;
				}
			}
			//�Ѿ˰� ���� �浹üũ

			if (bulletVector.size() != 0)
			{
				for (monsterIter = monsterVector.begin(); monsterIter != monsterVector.end(); ++monsterIter)
				{
					for (bulletIter = bulletVector.begin(); bulletIter != bulletVector.end(); ++bulletIter)
					{
						if (CollisionBulletWithMonster(bulletIter->BulletPos, monsterIter->MonsterPos))
						{
							cout << "���� �浹 " << endl;
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
								cout << "��ų1 : " << player1KillCount << endl;
								cout << "��ų2 : " << player2KillCount << endl;
								monsterIter->MonsterPos.y = -300;
							}

							if (bulletVector.size() == 0)
								break;
						}
					}
				}
			}
			//�Ѿ˰� ��ֹ� �浹üũ

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
			//�Ѿ˰� �� �浹üũ
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

			//���� ����


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
	cout << "������ ����!" << endl;
	SOCKET client_sock = (SOCKET)param;
	int retval;         //return value
	char buf[BUFSIZE];
	int dataType = 0;

	RecvPacket recvPacket;

	int playerIndex;

	double nextTime = 0.0f;
	double nowTime = clock();

	while (true)            //�� �Լ������� send,recv �۾��� �̷����.
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

			// �� �浹
			//printf("Before : X : %f , z : %f \n", recvPacket.playerPos.x, recvPacket.playerPos.z);
			CollisionMapWithPlayer(recvPacket.playerPos);
			CollisionContainerWithPlayer(recvPacket.playerPos);
			//printf("After  : X : %f , z : %f \n", recvPacket.playerPos.x, recvPacket.playerPos.z);
			//������ �÷��̾� ������ �� �ʱ�ȭ
			playerVector[0].PlayerPos.x = recvPacket.playerPos.x;
			playerVector[0].PlayerPos.z = recvPacket.playerPos.z;

			//��������Ŷ�� �� �ʱ�ȭ
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
	cout << "������ ����!" << endl;
	SOCKET client_sock = (SOCKET)param;
	int retval;         //return value
	char buf[BUFSIZE];
	int dataType = 0;

	double nextTime = 0.0f;
	double nowTime = clock();
	RecvPacket recvPacket;

	while (true)            //�� �Լ������� send,recv �۾��� �̷����.
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
			//Player2 �κ��� ������ �޾ƿ�.
			retval = recv(client_sock, (char*)&recvPacket, sizeof(RecvPacket), 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("recv");
			}

			// �� �浹
			CollisionMapWithPlayer(recvPacket.playerPos);
			CollisionContainerWithPlayer(recvPacket.playerPos);
			// ������ �÷��̾���� �� �ʱ�ȭ
			playerVector[1].PlayerPos.x = recvPacket.playerPos.x;
			playerVector[1].PlayerPos.z = recvPacket.playerPos.z;

			//���� ��������Ŷ�� �� �ʱ�ȭ.
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

	//Ʈ��� ���������� ��������.
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