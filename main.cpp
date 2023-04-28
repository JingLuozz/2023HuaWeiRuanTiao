//Robot_gui.exe -m map.txt "main.exe"
#include <iostream>
#include "data.h"
#pragma GCC optimize(2)
using namespace std;


Solution s;
//初始化
bool Init() {
	char line[1024];
	int row = 100;
	fgets(line, sizeof line, stdin);
	if(line[0] == 'B') s.teamkind = 0;
	else s.teamkind = 1;
	logs = Logs(s.teamkind);
	while (fgets(line, sizeof line, stdin)) {
		if (line[0] == 'O' && line[1] == 'K') {
			return true;
		}
		row--;
		for (int i = 0; i < 100; i++) {
			if (line[i] == '.') {
				s.maps[i][row] = 0;
				continue;
			}
			double x = i * 0.5 + 0.25;
			double y = row * 0.5 + 0.25;
			if (line[i] == 'A') {
				if(s.teamkind == 0) s.addRot(Rot(x, y));
				s.maps[i][row] = 0;
			}
			if (line[i] == 'B') {
				if(s.teamkind == 1) s.addRot(Rot(x, y));
				s.maps[i][row] = 0;
			}				
			if('1' <= line[i]  && line[i] <= '9'){
				s.maps[i][row] = line[i]-'0';
				if(s.teamkind == 0) s.addWorkplace(Workplace(x, y, line[i] - '0'));
				else s.addRivalWorkplace(Workplace(x, y, line[i] - '0'));

			}
			if('a' <= line[i] && line[i] <='i') {
				s.maps[i][row] = line[i]-'a'+1;
				if(s.teamkind == 1) s.addWorkplace(Workplace(x, y, line[i] - 'a'+1));
				else s.addRivalWorkplace(Workplace(x, y, line[i] - 'a'+1));			
			}
			if(line[i] == '#') s.maps[i][row] = -2;
		}
	}

	for(int i = 0;i<100;i++) {
		for(int j =0;j<100;j++) {
			s.e_map[i][j] = 0;
		}
	}
	return false;
}

//frameUpdate
void updateFrame() {
	srand((int)time(0));
	int k;
	scanf("%d", &k);
	// lg.fp << k << endl;
	//读取每个工作台状态
	for (int i = 0; i < k; i++) {
		int kind, res, sta, okg;
		double x, y;
		scanf("%d %lf %lf %d %d %d", &kind, &x, &y, &res, &sta, &okg);
		// logs.fp << kind << ' ' << x << ' ' << y << ' ' << res << ' ' << sta << ' ' << okg << endl;
		s.updateWorkplace(i, sta, res, okg);
	}
	//读取每个机器人状态
	for (int i = 0; i < 4; i++) {
		int nowWp, goods;
		double tf, cf, vRad,v_x,v_y,dir,p_x,p_y;
		scanf("%d %d %lf %lf %lf %lf %lf %lf %lf %lf", &nowWp, &goods, &tf, &cf, &vRad, &v_x, &v_y, &dir, &p_x, &p_y);
		// lg.fp << nowWp << ' ' << goods << ' ' << tf << ' ' << cf << ' ' << vRad << ' ' << v_x << ' '<< v_y<<' '<< dir<<' '<< p_x<<' '<<p_y<<endl;
		s.updateRobot(i,nowWp,goods,tf,cf,vRad,dir,v_x,v_y,p_x,p_y);
	}
	for(int i = 0; i < 4;i++) {
		for(int j = 0;j<360;j++) {
			double dist;
			scanf("%lf", &dist);
			s.rts[i].radar.updateRadar((j+s.rts[i].angle)%360, dist);
			s.rts[i].sradar.updateRadar((j+s.rts[i].angle)%360, dist);
		}
	}
}

bool readUntilOK() {
	char line[1024];
	while (fgets(line, sizeof line, stdin)) {
		if (line[0] == 'O' && line[1] == 'K') {
			return true;
		}
		//do something
	}
	return false;
}

int main() {
	Init();
	operate_init();
	s.handleMaps();
	s.dealDisfra();
	s.judgemapkind();
	s.recoverSearch();
	s.usingmapkindpolicy();
	// logs.fp<<s.mapkind<<endl;
	for(int i = 0;i<10;i++) befPP[i] = 1.0;
	for(int i = 0;i<4 ;i++) trackID[i] = -1;
	puts("OK");
	fflush(stdout);
	int frameID;
	while (scanf("%d", &frameID) != EOF) {
		s.frameId = frameID;
		scanf("%d", &s.money);
		// logs.fp << "id = "<< frameID << endl;
		updateFrame();
		readUntilOK(); //TODO 读取当前所有数据
		// 处理
		s.updatePolicy();
		// 输出策略
		printf("%d\n", frameID);
		s.printCmd();
		printf("OK\n", frameID);
		fflush(stdout);
		// logs.fp <<endl;
	}
	return 0;
}
/*


*/