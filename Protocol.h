#pragma once

struct Vec3                     //X, Y, Z. 
{
	float   x;
	float   y;
	float    z;
};
struct Vec2
{
	float x;
	float z;
};
struct InitInfo                  //������ �ʱⰪ�� ����ϴ� ����ü
{
	int playerIndex;
	Vec3 Player1Pos;            //ù��° �÷��̾��� ��ġ�� ���
	Vec3 Player2Pos;            //�ι�° �÷��̾��� ��ġ�� ���
	Vec3 MonsterPos[10];         //���͵��� ��ġ������ ���
								 //Vec3 containerPos[4];         // �ʿ��Ѱ�?
};

enum DataType                  //������ Ÿ���� ���� ������ ���� ������
{
	PLAYER,                     //����Ÿ�� -> �÷��̾�����
	BULLET,                     //����Ÿ�� -> �Ѿ�����
	MONSTER,                  //����Ÿ�� -> ��������
	SCENE,                     //����Ÿ�� -> ������
	PACKET                     //����Ÿ�� -> ��Ŷ ������ ������Ÿ��.
};

enum SceneList                  //�� ���� ������ ���� ������
{
	LOGO,
	LOBBY,                     //����Ÿ�� -> �κ� ��
	INGAME,                     //����Ÿ�� -> �������� ��
	RESULT                     //����Ÿ�� -> ���Ӱ�� ��
};

// Ÿ�� ������ �ʿ��� ����   
struct SceneInfo               //���� ������ ����ϴ� ����ü
{
	int type;                  //������ Ÿ���� ������ ���
	int SceneState;               //���� ���� ���¸� ����
};

struct PlayerInfo               //�÷��̾��� ������ ����ϴ� ����ü
{                           // 34byte
	int type;                  //������ Ÿ���� ������ ���
	int playerIndex;
	Vec3 PlayerPos;               //�÷��̾��� ��ġ�� ���
	Vec3 CameraDir;               //�÷��̾��� ������ ���
	bool AniandKeyState;         //Ű�� ���� ���� ���
	unsigned char charKey;         //� Ű?
};

struct BulletInfo               //�Ѿ��� ������ ����ϴ� ����ü
{
	Vec3 BulletDirection;         //�Ѿ��� ����.
	Vec3 BulletPos;               //�Ѿ��� ��ġ�� ���
	bool newBullet;               //�Ѿ��� ���� ������� �Ѿ����� �ƴ���
								  //True�� �������� �Ѿ˰��� ���Ϳ� �߰�.
	int bulletOwner;            //���� �߻��� �Ѿ����� ����
};

struct MonsterInfo               //������ ������ ����ϴ� ����ü
{                           //20byte;
	int type;                  //������ Ÿ���� ������ ���
	Vec3 MonsterPos;            //������ ��ġ�� ���
	int hp;                     //���Ϳ� ü���� ���, 
								//�⺻������ 3�� ü���� ������ �Ѿ˿� �ѹ� ������, ü���� 1�� ���δ�.
	int index;
};

struct ContainerInfo            //�����̳� ��ġ�� ����ü
{
	Vec3 position;               //�����̳��� �߽� ��ġ��.
};

struct ResultInfo               // ��� ������ ������ ���� ����ü
{
	int killCount;               // �÷��̾ �� ������ ���͸� ��Ҵ��� 
	int SceneState;               // ���� Ŭ���̾�Ʈ�� ���ȭ���� ���� ���� ������ ����
};
struct MapSizeInfo
{

};

//-----------------11/28 �߰�--------------
struct DataPacket
{
	PlayerInfo player1;         // 34
	PlayerInfo player2;         // 34
	MonsterInfo monsters[10];   // 240

};

struct playerSend
{
	int      playerIndex;               //4
	Vec2   playerPos;                  //8
	float   playerCam;                  //4
	int Killcount;
};

struct SendPacket
{
	playerSend player1;                  //16
	playerSend player2;                  //16
	Vec3   MonstersPosition[10];         //80
										 //Vec3   player1Bullets[5];            //60         
										 //Vec3   player2Bullets[5];            //60
										 //112
	bool gameOver;
};
struct RecvPacket
{
	Vec2 playerPos;         //8
	float playerCam;         //4
	bool makeBullet;         //1         //�̻��Ⱑ TRUe�� �Ѿ� ��ɷ� �����ϰ� ������ �Ѿ˰��� ���Ϳ� �߰�����.
	Vec3 bulletDirection;      //12
	Vec3 bulletPosition;      //12
							  //37
};


//-------���� �ʿ��ؼ� ����κ�-------
struct ThreadParam
{
	SOCKET clientsock;
	int clientInex;
};
struct MonsterPosForSend      //240byte
{
	MonsterInfo monsters[10];
};

void err_display(char * msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s \n", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
	exit(1);
}
void err_quit(char * msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}