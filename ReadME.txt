서버 SendPacket 구조체에

bool gameOver 변수 하나추가

평소엔 false이다가

게임오버조건(player1 killcount + player2killcount == 10)
이면 트루로 변환

그리고 각 플레이어에게 킬카운트 변수(int)보내고

서버 종료시킴.