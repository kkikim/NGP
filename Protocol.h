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
struct InitInfo                  //게임의 초기값을 담당하는 구조체
{
	int playerIndex;
	Vec3 Player1Pos;            //첫번째 플레이어의 위치를 담당
	Vec3 Player2Pos;            //두번째 플레이어의 위치를 담당
	Vec3 MonsterPos[10];         //몬스터들의 위치정보를 담당
								 //Vec3 containerPos[4];         // 필요한가?
};

enum DataType                  //데이터 타입의 구분 변수를 가진 열거형
{
	PLAYER,                     //구분타입 -> 플레이어정보
	BULLET,                     //구분타입 -> 총알정보
	MONSTER,                  //구분타입 -> 몬스터정보
	SCENE,                     //구분타입 -> 씬정보
	PACKET                     //구분타입 -> 패킷 묶은거 데이터타입.
};

enum SceneList                  //씬 구분 변수를 가진 열거험
{
	LOGO,
	LOBBY,                     //구분타입 -> 로비 씬
	INGAME,                     //구분타입 -> 게임진행 씬
	RESULT                     //구분타입 -> 게임결과 씬
};

// 타입 구분이 필요한 정보   
struct SceneInfo               //씬의 구분을 담당하는 구조체
{
	int type;                  //데이터 타입의 구분을 담당
	int SceneState;               //현재 씬의 상태를 구분
};

struct PlayerInfo               //플레이어의 정보를 담당하는 구조체
{                           // 34byte
	int type;                  //데이터 타입의 구분을 담당
	int playerIndex;
	Vec3 PlayerPos;               //플레이어의 위치를 담당
	Vec3 CameraDir;               //플레이어의 방향을 담당
	bool AniandKeyState;         //키의 눌림 여부 담당
	unsigned char charKey;         //어떤 키?
};

struct BulletInfo               //총알의 정보를 담당하는 구조체
{
	Vec3 BulletDirection;         //총알의 방향.
	Vec3 BulletPos;               //총알의 위치를 담당
	bool newBullet;               //총알이 새로 만들어진 총알인지 아닌지
								  //True면 서버내에 총알관리 벡터에 추가.
	int bulletOwner;            //누가 발사한 총알인지 구분
};

struct MonsterInfo               //몬스터의 정보를 담당하는 구조체
{                           //20byte;
	int type;                  //데이터 타입의 구분을 담당
	Vec3 MonsterPos;            //몬스터의 위치를 담당
	int hp;                     //몬스터에 체력을 담당, 
								//기본적으로 3에 체력을 가지며 총알에 한발 맞으면, 체력이 1이 깎인다.
	int index;
};

struct ContainerInfo            //컨테이너 위치값 구조체
{
	Vec3 position;               //컨테이너의 중심 위치값.
};

struct ResultInfo               // 결과 조건을 보내기 위한 구조체
{
	int killCount;               // 플레이어가 몇 마리의 몬스터를 잡았는지 
	int SceneState;               // 양쪽 클라이언트에 결과화면을 띄우기 위해 보내는 변수
};
struct MapSizeInfo
{

};

//-----------------11/28 추가--------------
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
	bool makeBullet;         //1         //이색기가 TRUe면 총알 쏜걸로 간주하고 서버내 총알관리 벡터에 추가해줌.
	Vec3 bulletDirection;      //12
	Vec3 bulletPosition;      //12
							  //37
};


//-------내가 필요해서 만든부분-------
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