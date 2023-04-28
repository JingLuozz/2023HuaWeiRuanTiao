#include <algorithm>
#include <iostream>
#include <vector>
#include "distance.h"
#include <queue>
#include "maphandle.h"
#include "rotradar.h"
#include "sagmentdis.h"
#define getsta(i) (1<<(i))
#pragma GCC optimize(2)
using namespace std;
typedef pair<double, double> CircleNode;
static int idd = 0;
static int levelup = 0;
static int leveldown = -1;
//工作台原料状态
int ksta[10] = { 0,
	0,0,0,
	getsta(1) + getsta(2),
	getsta(1) + getsta(3),
	getsta(2) + getsta(3),
	getsta(4) + getsta(5) + getsta(6),
	getsta(7),
	getsta(8) - 1
};

//工作台生产周期
int kindgds[10] = {
	0,
	50,50,50,
	500,500,500,
	1000,1,1
};

//每个产品的状态
int gsta[8] = {
	0,getsta(1),getsta(2),
	getsta(3),getsta(4), getsta(5), 
	getsta(6), getsta(7),
};

double profit[10] = { 0,
	3000 , 7600 - 4400, 9200 - 5800,
	(22500 - 15400)  , (25000 - 17200) , (27500 - 19200) ,
	(105000 - 76000)  ,0,0
};
//生产惩罚 
double befPP[10];
double penaltyP[10];
double penaltyF = 0.85;//惩罚系数


class enemy_rot {/* new function:敌人类*/
public:
	double x;
	double y;
	int nx, ny;
	int id;
	double radius;
	double v;		 //不知道取-1
	int jiao_angle;	 //不知道取-1
	int frameid;	 //第几帧被发现
	int nearby_ewpk; //半径突变时，该敌人临近工作台下标 若找不到 返回-1
	int goods; //商品
	int nearby_wpk;	 //敌人在我方哪个工作台附近
	int stach;
	enemy_rot(double x, double y, double radius, int frameid, int id) {
		this->x = x;
		this->y = y;
		this->id = id;
		this->nx = (int)(x / 0.5);
		this->ny = (int)(y / 0.5);
		this->radius = radius;
		this->frameid = frameid;
		this->goods = (abs(radius-0.53) < 0.01? 0: -1);	
	}

	enemy_rot(double x, double y, double radius, int frameid, int stach, int v, int jiao_angle)
	{
		this->x = x;
		this->y = y;
		this->nx = (int)(x / 0.5);
		this->ny = (int)(y / 0.5);
		this->radius = radius;
		this->frameid = frameid;
		this->stach = stach;
		this->v = v;
		this->jiao_angle = jiao_angle;
		this->goods = -1;
	}

	int updateposition(double x, double y) {
		this->x=x;
		this->y=y;
		this->nx = (int) (x/0.5);
		this->ny = (int) (y/0.5);		
		return 0;
	}
};
int trackID[4];

vector<P> e_rotAtk;
vector<int> e_rotOld;
int vissearch[50][50];
class Rot{
public:
	int x,y;
	int noRoad;
	double v_x,v_y; // 速度向量
	double p_x,p_y; // 位置
	int nowWp,goods;//当前可操作工作台，产品
	double tf, cf;
	double vrad, dir; //角速度，方向
	int wait;
	queue<int> tarWp;
	pair<double,double> pos;
	Radar radar;
	Radar sradar;
	int angle;
	int forward;
	int rtlevel;
	int rtdis[100][100];
	int recovering = 0;
	int needWait = 0;
	int startRecovering;
	int rival_policy; // 对敌策略
	int befswitch;
	int starttime, maybetime;
	int attackPos;
	int befx, befy, beftimes;
	double v_f;
	Rot(double p_x, double p_y) {
		this->p_x = p_x;
		this->p_y = p_y;
		this->x = (int) (p_x/0.5);
		this->y = (int) (p_y/0.5);
		this->v_x = this->v_y = 0;
		nowWp = -1, goods = 0;
		befx = befy = beftimes = 0;
		tf = cf = 0;
		noRoad = 0;
		rival_policy = 0;
		recovering = needWait = 0;
		befswitch = -1;
	};

};

class Workplace{
public:
	int x,y;
	double p_x,p_y; // 位置
	int kind; // 工作台种类
	int sta; // 原料格状态
	int rtime; //生产剩余时间
	int okgoods; //产品格状态
	double tarmoneyf; //到达该目标点利润系数
	int staflg;
	vector<int> sellwps; //原料来源: 有哪些工作台可以为此工作台提供原料
	vector<int> nexgoods; //接下来几个生产品完成的时间
	queue<int> stalist[10]; //每个产品到达时间
	int nowp, fid, restf, lastc;
	int recover, needclear;
	int occupy;
	pair<double,double> recovePoint;
	Workplace(double p_x, double p_y, int kind) {
		this->p_x = p_x;
		this->p_y = p_y;
		this->x = (int) (p_x/0.5);
		this->y = (int) (p_y/0.5);		
		this->kind = kind;
		rtime = -1;
		okgoods = 0;
		sta = 0;
		recover = 0;
		needclear = 0;
		occupy = 0;
	}

	//判断原料是否存在
	bool exsist(int goods) {
		return (sta & gsta[goods]) || !(ksta[kind]&gsta[goods]);
	}

	void addsta(int goods) {
		sta |= gsta[goods];
	
	}

	void updateBybuy(int newfid) {
		//买走当前的产品，最多影响一个
		//logs.fp << nowp << ' ' << nexgoods.size() <<' '<<kind<< endl;
		if (nowp >= nexgoods.size()) return;
		if (newfid < fid+nexgoods[nowp]) newfid = fid + nexgoods[nowp];
		int cur = newfid;
		newfid -= fid;
		
		newfid -= nexgoods[nowp];
		nowp++;
		if(kind >=4 && kind <=6)penaltyP[kind] /= penaltyF;
		if (nowp < nexgoods.size()) {
			nexgoods[nowp] = max(0, nexgoods[nowp] - newfid);
		}
		if (nowp == nexgoods.size()) {
			restf = newfid;
		}
		fid = cur;
	}

	void updateBysell(int newfid, int goods) {
		stalist[goods].push(max(newfid,lastc));
		int mi_sz = 100;
		int ma_fid = 0;
		if (rtime != -1) return;
		for (int i = 1; i <= 7; i++) {
			int bt = getsta(i);
			if (!(bt&ksta[kind])) continue;
			mi_sz = min(mi_sz, (int) stalist[i].size());
			ma_fid = max(ma_fid, stalist[i].empty()? 9000:stalist[i].front());
		}

		if (mi_sz == 0) return; //不会产生新的产品
		if(kind >= 4 && kind <= 6) penaltyP[kind] *= penaltyF;
		staflg = 1;
		lastc = ma_fid;
		if (nowp >= nexgoods.size()) {
			nexgoods.push_back(ma_fid <= fid ? max(0, kindgds[kind] - restf):ma_fid-fid+ kindgds[kind]);
			restf = 0;
		}
		else nexgoods.push_back(kindgds[kind]);

		for (int i = 1; i <= 7; i++) {
			int bt = getsta(i);
			if (!(bt&ksta[kind])) continue;
			stalist[i].pop();
		}
		rtime = kindgds[kind];
	}
	double getProductprofitf() {
		double f = 1;
		for(int i = 1; i<=7;i ++) {
			if(stalist[i].size() != 0) {
				f += 0.5;
			}
		}
		return f;
	}
};


class Solution{
public:
	int teamkind; //队伍种类: 0:蓝方 1:红方
	int mapkind; //图的种类
	int maps[100][100];
	double sumdis = 0, sumframe = 0;
	int money,frameId;
	vector<Workplace> wps;
	vector<Rot> rts;
	vector<int> wpk[10];
	vector<Cmd> cmds;
	queue< pair<double,double> > qu; ///* new function:运行时队列*/
	vector<enemy_rot> e_rot;/* new function:敌人数组*/
	int recover;
	//地方工作台
	vector<Workplace> e_wps;
	vector<int> e_wpk[10];
	//处理两个工作台的距离
	// vector<vector<int>> disfra;
	int wpdis[50][2][100][100];
	int e_wpdis[50][2][100][100];
	int repanit = 0;
	int wpkpaint = 0;
	int stapaint = 0;
	Radar wps_radar[50];
	int e_map[100][100];
	int wpk_vis[50];
	int e_wpk_vis[50];
	int search_enemy_wpk(double x, double y, int answer)
	{ // x,y为敌方下标
		int size = wps.size();
		for (int i = 0; i < size; i++)
		{
			if (abs(x - wps[i].p_x) < 0.3 && abs(y - wps[i].p_y) < 0.3)
			{ //敌方机器人与敌方工作台小于0.3米即产生交互
				return i;
			}
		}
		return -1;
	}
	int search_enemy_ewpk(double x, double y, int answer)
	{ // x,y为敌方下标
		int size = e_wps.size();
		for (int i = 0; i < size; i++)
		{
			if (abs(x - e_wps[i].p_x) < 0.3 && abs(y - e_wps[i].p_y) < 0.3)
			{ //敌方机器人与敌方工作台小于0.3米即产生交互
				return i;
			}
		}
		return -1;
	}

	int operate2(pair<double, double> a, double radius)
	{ //对刚求得的敌方圆a进行处理 并观察其相邻帧状态变化  /* new function:求得的狄方圆放进敌人数组*/
		//返回值为1 说明其保持不变或者第一次加入
		//返回值为2 其半径从0.53->0.45 其完成售卖
		//返回值为3 其半径从0.45->0.53 完成购买
		int answer;
		if (e_rot.size() == 0)
		{ //原来vector为空
			enemy_rot temp(a.first, a.second, radius, frameId, 1, -1, -1);
			temp.id = idd;
			idd++;
			temp.nearby_wpk = search_enemy_wpk(a.first, a.second, 1);
			temp.nearby_ewpk = search_enemy_ewpk(a.first, a.second, 1);
			answer = 1;
			if(abs(0.45-radius)<0.01) {
				temp.goods=-1;
			}
			else temp.goods = 0;
			e_rot.push_back(temp);
			return answer;
		}

		bool tag = false; //该点之前是否存过
		for (auto &b : e_rot)
		{
			//            auto b=e_rot.back();
			if (abs(a.first - b.x) <= 0.3 && abs(a.second - b.y) <= 0.3)
			{ //有点存进来过要更新原来点 新点与原来点可能为同一帧 也可能不为同一帧 id不变
				tag = true;
				if (frameId > b.frameid)
				{
					double tempx = a.first - b.x, tempy = a.second - b.y;
					double delta_vx = tempx / (double)((frameId - b.frameid) * 0.02);
					double delta_vy = tempy / (double)((frameId - b.frameid) * 0.02);
					double v = sqrt((delta_vx * delta_vx) + (delta_vy * delta_vy));
					double temp = sqrt((tempx * tempx) + (tempy * tempy));
					int jiao_angle = (asin(tempy / temp) / PI) * 180;
					if (tempx < 0 && tempy >= 0)
						jiao_angle = 180 - jiao_angle;
					else if (tempx <= 0 && tempy < 0)
						jiao_angle = (-1 * jiao_angle) + 180;
					else if (tempx > 0 && tempy < 0)
						jiao_angle = jiao_angle + 360;
					b.v = v;
					b.jiao_angle = jiao_angle;
				}
				b.updateposition(a.first, a.second);
				// jiao_angle有效值为0~359
				b.frameid = frameId;
				//沿用原来旧点的id
				if (b.radius == radius)
					answer = 1;
				else if (b.radius == 0.53 && radius == 0.45)
				{
					answer = 2;
					b.nearby_ewpk = search_enemy_ewpk(a.first, a.second, 2);
					b.nearby_wpk = search_enemy_wpk(a.first, a.second, 2);
					b.goods = -1;
					b.radius = 0.45;
				}

				else if (b.radius == 0.45 && radius == 0.53)
				{
					answer = 3;
					b.nearby_ewpk = search_enemy_ewpk(a.first, a.second, 3);
					b.nearby_wpk = search_enemy_wpk(a.first, a.second, 3);
					if(b.nearby_ewpk!=-1) b.goods = e_wps[b.nearby_ewpk].kind;
					else b.goods = 0;
					b.radius = 0.53;
				}
				b.stach = answer; //改变状态
				break;
			}
		}
		if (tag == false)
		{ //该点之前并未存过但敌人数组不空
			enemy_rot temp(a.first, a.second, radius, frameId, 1, -1, -1);
			answer = 1;
			temp.nearby_wpk = search_enemy_wpk(a.first, a.second, 1);
			temp.nearby_ewpk = search_enemy_ewpk(a.first, a.second, 1);
			temp.id = idd;
			if(abs(0.45-radius)<0.01) {
				temp.goods=-1;
			}
			else temp.goods=0;			
			idd++;
			e_rot.push_back(temp);
			return answer;
		}
		return answer;
	}

	void sandiangongyuan() { //改版 /* new function:求敌方圆位置半径*/
		double point1_x, point1_y, point2_x, point2_y, point3_x, point3_y;

		if (qu.size() < 3)
			return; //如果没存够三个 直接返回 不用算了

		auto a = qu.front();
		qu.pop();
		point1_x = a.first;
		point1_y = a.second;
		a = qu.front();
		point2_x = a.first;
		point2_y = a.second;
		a = qu.back();
		point3_x = a.first;
		point3_y = a.second;

		double midx1, midy1, midx2, midy2;
		double h1, h2;
		midx1 = (point1_x + point2_x) / 2;
		midy1 = (point1_y + point2_y) / 2;
		h1 = -(point2_x - point1_x) / (point2_y - point1_y);
		midx2 = (point2_x + point3_x) / 2;
		midy2 = (point2_y + point3_y) / 2;
		h2 = -(point3_x - point2_x) / (point3_y - point2_y);
		double x = (midy2 - midy1 + midx1 * h1 - midx2 * h2) / (h1 - h2);
		double y = h1 * (x - midx1) + midy1;
		double radius = hypot(fabs(x - point1_x), fabs(y - point1_y)); //求直角三角形斜边长
		bool tag = false;											   //是否为敌方机器人
		for (int i = 0; i < 4; i++)
		{
			if (abs(rts[i].p_x - x) < 0.1 && abs(rts[i].p_y - y) < 0.1)
			{ //为我方机器人
				tag = true;
				break;
			}
		}

		if (tag == false)
		{ //为敌人
			double radius = hypot(fabs(x - point1_x), fabs(y - point1_y));
			if (abs(radius - 0.45) < 0.01)
			{
				int zhuangtai = operate2(pair<double, double>(x, y), 0.45); //状态为当新节点进入敌人数组，其对应的当前状态
			}
			else if (abs(radius - 0.53) < 0.01)
			{
				int zhuangtai = operate2(pair<double, double>(x, y), 0.53);
			}
		}
		return;
	}

	void operate(int index, double x, double y, int angle, double dist, int &count)
	{
		double duandian_x = x + dist * coss[angle];
		double duandian_y = y + dist * sinn[angle];
		int x1 = (int)((x + dist * coss[angle]) / 0.5);
		int y1 = (int)((y + dist * sinn[angle]) / 0.5);
		int x2 = (int)((x + (dist + 0.1) * coss[angle]) / 0.5); //加了0.1的修正
		int y2 = (int)((y + (dist + 0.1) * sinn[angle]) / 0.5);

		//检测越界
		if (x1 <= 0.0 || x1 >= 100.0 || y1 <= 0.0 || y1 >= 100.0)
		{
			count = 0;
			while (!qu.empty())
				qu.pop();
			return;
		}
		//检测墙壁
		else if (maps[x1][y1] == -2 || maps[x2][y2] == -2)
		{
			count = 0;
			while (!qu.empty())
				qu.pop();
			return;
		}
		else
		{
			qu.push(pair<double, double>{duandian_x, duandian_y});
			count++;
		}

		if (count == 3)
		{
			sandiangongyuan();
			count--;
		}
		return;
	}

	void func3(int frameid)
	{ /* new function:处理敌人数组中存放的旧节点*/
		if (e_rot.size() == 0)
			return;
		auto a = e_rot.begin();
		for (; a != e_rot.end();)
		{
			if (frameid - (a->frameid) >= 1)
			{ //如果当前帧与之前存放的圆帧差为100帧 即2s 算出四个机器人与该位置的角度偏向 查找该位置的射线 如果该射线可以穿过原先圆心 即将该圆清除
				bool tag = false;
				for (int i = 0; i < 4; i++)
				{
					double delta_x, delta_y;
					delta_x = (a->x) - rts[i].p_x;
					delta_y = (a->y) - rts[i].p_y;
					double xiebian = sqrt((delta_x * delta_x) + (delta_y * delta_y));
					double hu_angle = asin(delta_y / xiebian);
					int jiao_angle = (hu_angle / PI) * 180;
					if (delta_x < 0 && delta_y >= 0)
						jiao_angle = 180 - jiao_angle;
					else if (delta_x <= 0 && delta_y < 0)
						jiao_angle = (-1 * jiao_angle) + 180;
					else if (delta_x > 0 && delta_y < 0)
						jiao_angle = jiao_angle + 360;
					// jiao_angle有效值为0~359
					double dist = rts[i].radar.dist[jiao_angle];
					if (dist > dis(rts[i].p_x, rts[i].p_y, a->x, a->y) - a->radius )
					{ //当前这个角度的射线长度已经大于当前我方扫描机器人圆心到这个旧位置圆心的距离
						auto temp = a;
						a = e_rot.erase(temp); // a移到被删除元素的下一位
						tag = true;
					}
					if (tag == true)
					{
						break; //保存的旧敌人节点已从vector中删除	 从遍历本方机器人的循环中跳出
					}
				}
				if (tag == false)
					++a; //本方机器人四次循环 均不能照到那个点 那个点暂时不删
			}
			else
				++a; //当前帧与之前存放的圆帧小于100帧 暂时不删
		}
		return;
	}

	void func(int index)
	{ /* new function:调用函数*/
		int angle;
		double rot_x = rts[index].p_x;
		double rot_y = rts[index].p_y;
		int count = 0;
		for (angle = 0; angle < 360; angle++)
		{
			operate(index, rot_x, rot_y, angle, rts[index].radar.dist[angle], count);
		}
		return;
	}

	bool GetCircle(CircleNode a, CircleNode b, CircleNode c, CircleNode& heart, double& radius){//三点共圆 true表示能找到，否则找不到
		double cur1=2 * (a.sec - c.sec) * (a.fir - b.fir) - 2 * (a.sec - b.sec) * (a.fir - c.fir);
		double cur2=2 * (a.sec - b.sec) * (a.fir - c.fir) - 2 * (a.sec - c.sec) * (a.fir - b.fir);

		if (cur1 == 0 || cur2 == 0) return false;
		double val1=a.fir * a.fir - b.fir * b.fir + a.sec * a.sec - b.sec * b.sec;
		double val2=a.fir * a.fir - c.fir * c.fir + a.sec * a.sec - c.sec * c.sec;
		heart = CircleNode((val1 * (a.sec - c.sec) - val2 * (a.sec - b.sec)) / cur1 , (val1 * (a.fir - c.fir) - val2 * (a.fir - b.fir)) / cur2 );
		radius = dis(heart.fir, heart.sec, a.fir, a.sec);
		return true;
	}

	bool checkCircleNode(double x, double y, int angle) { //检查碰撞点是否合法
		for(int i = 0;i<20;i++) {
			int xx = (int) ((x + 0.01 * i * coss[angle]) / 0.5);
			int yy = (int) ((y + 0.01 * i * sinn[angle] )/ 0.5);
			int flgx = x + i * 0.01 * coss[angle] < 1e-4? 1:0;
			int flgy = y + i * 0.01 * sinn[angle] < 1e-4? 1:0;
			if(flgx || xx >= 100 || flgy || yy >= 100 || maps[xx][yy] == -2)return false;
		}
		return true;
	}

	bool checkSameCir(CircleNode c1, CircleNode c2) {
		// logs.fp<<"	checkSameCir: ( "<< c1.fir<<" , "<< c1.sec <<" ) ( "<< c2.fir<< " , " <<c2.sec <<" )"<<endl;
		return abs(c1.fir - c2.fir) < 0.3 && abs(c1.sec-c2.sec) < 0.3; 
	}

	vector<CircleNode> cirhearts;
	vector<double> cirRs;
	void serchEnemy(int rid) {
		vector<CircleNode> cns;
		double x = rts[rid].p_x, y = rts[rid].p_y;
		for(int angle = 0; angle < 360; angle++) {
			double dist = rts[rid].radar.dist[angle];
			double duandian_x = x + dist * coss[angle];
			double duandian_y = y + dist * sinn[angle];			
			if(checkCircleNode(duandian_x, duandian_y, angle)) {
				//如果检测到不是障碍物的点则放入队列
				cns.push_back(CircleNode(duandian_x, duandian_y));
			}
		}
		int n = cns.size();
		//每三个相邻的圆点求三点共圆
		if(n < 3) return ;
		for(int i = 0; i<n;i++){
			CircleNode cnode;
			double radius;
			if(!GetCircle(cns[i], cns[(i+1)%n], cns[(i+2)%n], cnode, radius)) continue;
			if(abs(radius - 0.45) > 0.01 && abs(radius - 0.53) > 0.01)  continue;
			int flg = 1;
			radius = abs(radius-0.45) < 0.01? 0.45: 0.53;
			//区分敌我,并判断之间的距离
			for(int rtid = 0; rtid < 4; rtid++) {
				if(!flg) break;
				if(checkSameCir(CircleNode(rts[rtid].p_x, rts[rtid].p_y), cnode)) flg = 0;
				double cirdis = dis(rts[rtid].p_x, rts[rtid].p_y, cnode.fir, cnode.sec);
				if(cirdis < (rts[rtid].goods? 0.53: 0.45) + radius - 0.2) flg = 0;
			}
			//检测是否是重复圆
			for(int cid = 0; cid < cirhearts.size(); cid++) {
				if(!flg) break;
				if(checkSameCir(cirhearts[cid], cnode)) {
					flg = 0;
					//对误差进行更新
					cirhearts[cid] = CircleNode( (cirhearts[cid].fir + cnode.fir) /2, (cirhearts[cid].sec + cnode.sec) /2);
				}
			}
			if(flg) cirhearts.push_back(cnode),cirRs.push_back(radius);
		}
	}

	int searchEwpkNearBy(CircleNode ch) {
		for(int i = 0; i < e_wps.size(); i++) {
			if(dis(ch.fir, ch.sec, e_wps[i].p_x, e_wps[i].p_y) < 0.5) return i;
		}
		return -1;
	}

	void updateEnemy() {
		//获取到所有圆后对圆做更新
		if(!cirhearts.size()) return ;
		for(int newc = 0; newc< cirhearts.size();newc++) {
			CircleNode nch = cirhearts[newc];
			double ncr = cirRs[newc];
			logs.fp<<"	( "<< nch.fir<< " , "<< nch.sec <<" ) "<<endl;
			int newflg = 1; //检测是否是新的圆
			for(int oldc = 0; oldc < e_rot.size(); oldc++) {
				enemy_rot& ert = e_rot[oldc];
				if(frameId - ert.frameid > 2) continue; //相差太久的我们不做检测;
				double hdis = dis(nch.fir, nch.sec, ert.x, ert.y);
				if(hdis > 0.3) continue;
				newflg = 0;
				ert.updateposition(nch.fir, nch.sec); //更新圆心位置
				ert.frameid = frameId;
				if(abs(ert.radius - ncr) > 0.01) { //如果检测到半径大小变换，修改手上的商品
					if(abs(ncr - 0.45) < 0.01) { //此时ncr 为0.01
						ert.radius = 0.45;
						ert.goods = -1;
					} 
					else { //寻找最近工作台
						int e_wpid = searchEwpkNearBy(nch);
						ert.goods = e_wpid == -1? 0: e_wps[e_wpid].kind;
					}
				} 

			}
			if(!newflg) continue;
			// 新出现的圆
			e_rot.push_back(enemy_rot(nch.fir, nch.sec, ncr, frameId, idd++));
		}
		cirhearts.clear();
		cirRs.clear();			
	}

	void deleteEnemy() {
		vector<enemy_rot> cur;
		for(int eid = 0; eid <e_rot.size(); eid++) {
			int deleteflg = 0;
			//先检查是否能被探测到
			enemy_rot& ert = e_rot[eid];
			for(int rid = 0; rid < 4; rid ++) {
				if(deleteflg) break;
				Rot& rt = rts[rid];
				int anglebetweenrot = getAngle(getPi_to_rightline(pair<double,double>(ert.x - rt.p_x, ert.y - rt.p_y)));
				double cirdis = dis(rt.p_x, rt.p_y, ert.x, ert.y);
				if(rt.radar.dist[anglebetweenrot] > cirdis - ert.radius + 0.1) deleteflg = 1;
			}
			//检测是否是已经被现在的某个圆覆盖了
			for(int eeid = 0; eeid < e_rot.size(); eeid++) {
				if(deleteflg) break;
				if(eeid == eid) continue;
				enemy_rot& ert_2 = e_rot[eeid];
				if(ert_2.frameid < ert.frameid) continue;
				if(ert_2.frameid == ert.frameid && eeid < eid) continue;
				double cirdis = dis(ert.x, ert.y, ert_2.x, ert_2.y);
				if(cirdis < ert.radius + ert_2.radius - 0.1) deleteflg = 1;
			}				
			if(!deleteflg) cur.push_back(ert);
		}
		e_rot.clear();
		for(int i = 0;i<cur.size(); i++) e_rot.push_back(cur[i]);
	}


	void handleMaps() {
		maphandle(maps);
	}

	void addWorkplace(Workplace wp) {
		// logs.fp<<"addwpk: kind: "<<wp.kind<<' '<<wp.p_x<<' '<<wp.p_y<<endl;
		if (wp.kind <= 8) wp.tarmoneyf = 1.0;
		else wp.tarmoneyf = 0.7;
		wpk[wp.kind].push_back(wps.size());
		wps.push_back(wp);
	}

	void addRivalWorkplace(Workplace wp) {
		// logs.fp<<"addwpk: kind: "<<wp.kind<<' '<<wp.p_x<<' '<<wp.p_y<<endl;
		if (wp.kind <= 8) wp.tarmoneyf = 1.0;
		else wp.tarmoneyf = 0.7;
		e_wpk[wp.kind].push_back(e_wps.size());
		e_wps.push_back(wp);
	}	

	void addRot(Rot rt) {
		rt.wait = 0;
		rt.pos.fir = rt.pos.sec = -1;
		rt.rtlevel = levelup++;
		rts.push_back(rt);
	}

	void updateWorkplace(int index, int sta, int res,int okgo) {
		wps[index].sta = sta;
		wps[index].rtime = res;
		wps[index].okgoods = okgo;
		wps[index].nexgoods.clear();
		wps[index].lastc = 0;
		wps[index].nexgoods.clear();
		if(wps[index].okgoods) wps[index].nexgoods.push_back(0);
		if (res > 0) wps[index].nexgoods.push_back(res);
		else if (res == 0) wps[index].nexgoods.push_back(res+1);

		for (int i = 1; i <= 7; i++) {
			while (!wps[index].stalist[i].empty()) wps[index].stalist[i].pop();
		}
		if (wps[index].sta == ksta[wps[index].kind]) {
			wps[index].nexgoods.push_back(kindgds[wps[index].kind]);
			wps[index].lastc = res;
		}
		else {
			for (int i = 1; i <= 7; i++) {
				int bt = getsta(i);
				if (!(bt&sta)) continue;
				wps[index].stalist[i].push(0);
			}
		}
		//工作台类型小于3的可以直接生产
		if (wps[index].kind <= 3) {
			for (int i = 1; i <= 3; i++) {
				wps[index].nexgoods.push_back(kindgds[wps[index].kind]);
			}
		}

		wps[index].nowp = 0;
		wps[index].fid = 0;
		wps[index].restf = 0;
		wps[index].staflg = 0;

	}
	
	void updateRobot(int index, int nowWp, int goods, double tf, double cf, double vrad, double dir, double v_x, double v_y,double p_x, double p_y) {
		rts[index].nowWp = nowWp;
		rts[index].goods = goods;
		rts[index].tf = tf;
		rts[index].cf = cf;
		rts[index].vrad = vrad;
		rts[index].dir = dir;
		rts[index].v_x = v_x;
		rts[index].v_y = v_y;
		rts[index].x = (int) (p_x/0.5);
		rts[index].y = (int) (p_y/0.5);
		rts[index].p_x = p_x;
		rts[index].p_y = p_y;
		rts[index].angle = getAngle(dir);
		rts[index].forward = 1;
		if(rts[index].befx == rts[index].x && rts[index].befy == rts[index].y) {
			rts[index].beftimes++;
		}
		else {
			rts[index].befx = rts[index].x;
			rts[index].befy = rts[index].y;
			rts[index].beftimes = 1; 
		}
	}

	//寻找原料来源
	void findsellwps(Workplace& wp) {
		for (int i = 1; i < 10; i++) {
			int kdbit = (getsta(i));
			if(!(ksta[wp.kind] & kdbit)) continue;
			for (int j = 0; j < wpk[i].size(); j++) {
				int wpid = wpk[i][j];
				wp.sellwps.push_back(wpid);
			}
		}
	}

	void findsellwpsRival(Workplace& wp) {
		for (int i = 1; i < 10; i++) {
			int kdbit = (getsta(i));
			if(!(ksta[wp.kind] & kdbit)) continue;
			for (int j = 0; j < e_wpk[i].size(); j++) {
				int wpid = e_wpk[i][j];
				wp.sellwps.push_back(wpid);
			}
		}		
	}

	void dealDisfra() {
		//对本方工作台处理
		for (int i = 0; i < wps.size(); i++) {
			double xx = wps[i].p_x/0.5;
			double yy = wps[i].p_y/0.5;
			getOtherRoad(P(wps[i].x, wps[i].y) ,wpdis[i][0], 0);
			getOtherRoad(P(wps[i].x, wps[i].y) ,wpdis[i][1], 1);
			findsellwps(wps[i]);
		}
		//对地方工作台进行处理
		for(int i = 0; i< e_wps.size(); i++) {
			getOtherRoad(P(e_wps[i].x, e_wps[i].y) ,e_wpdis[i][0], 0);
			getOtherRoad(P(e_wps[i].x, e_wps[i].y) ,e_wpdis[i][1], 1);
			findsellwpsRival(e_wps[i]);
		}
	}

	void recoverSearch() {
		for(int i = 0;i<wps.size();i++) {
			for(int moveAngle = 0; moveAngle < 360;moveAngle++) {
				double nowdis = 0;
				// logs.fp<<i<<' '<<moveAngle<<endl;
				pair<double,double> a = get_offset(0.1, moveAngle), nowpos = pair<double,double>(wps[i].p_x, wps[i].p_y);
				pair<int,int> befpos = pair<int,int>(wps[i].x, wps[i].y), curpos = pair<int,int>(wps[i].x, wps[i].y);
				while(nowdis < 10) {
					// logs.fp<<nowdis<<endl;
					if(nowpos.fir < 0 || nowpos.fir >= 50 || nowpos.fir < 0 || nowpos.sec >= 50) break; 
					if(befpos != curpos && maps[curpos.fir][curpos.sec] == -2) {
						nowdis -= 0.1;
						break;
					}
					nowpos.fir += a.fir;
					nowpos.sec += a.sec;
					curpos.fir = (int) (nowpos.fir/0.5);
					curpos.sec = (int) (nowpos.sec/0.5);
					nowdis += 0.1;
				}
				wps_radar[i].dist[moveAngle] = nowdis;
			}
			pair<double, double> miposition;
			int mi = -1;

			for(int moveAngle = 0; moveAngle < 360;moveAngle++) {

				double madis = wps_radar[i].getmaxdistance(moveAngle, 0.53);
				double midis = wps_radar[i].getmaxdistance((moveAngle+180)%360, 0.45);
				if(madis >= 4.0 && midis >=0.5) {
					pair<double,double> a = get_offset(1, moveAngle);
					wps[i].recover = 1;
					madis = min(madis, 5.0);
					int sum = 0;
					pair<double, double> nodes = pair<double, double>(wps[i].p_x + a.fir*madis, wps[i].p_y + a.sec*madis);
					int x = (int) (nodes.fir/0.5);
					int y = (int) (nodes.sec/0.5);
					for(int j = 0;j<wps[i].sellwps.size();j++) {
						int wpid = wps[i].sellwps[j];
						sum += wpdis[wpid][1][x][y];
						break;
					}									
					if(mi == -1 || sum < mi) {
						miposition = nodes;
						mi = sum;
					}
				}
			}
			wps[i].recovePoint = miposition;
		}
	}

	void judgemapkind() {
		mapkind = 0;
		if(maxroad >= 1024) mapkind = 3;
		else if(wps.size() == 0 || e_wps.size() == 0) mapkind = 4;
		else {
			for(int rid = 0; rid < 4; rid++) {
				Rot& rt = rts[rid];
				int flg = 0;
				for(int i = 0; i<wps.size();i++) {
					if(flg) break;
					if(wpdis[i][0][rt.x][rt.y] < inf) flg = 1;
					if(wpdis[i][1][rt.x][rt.y] < inf) flg = 1;
				}
				if(!flg) mapkind = 2;
			}
			if(mapkind == 0) {
				mapkind = 1;
			}
		}
	}

	void usingmapkindpolicy() {
		for(int i = 0;i<4;i++) {
			rts[i].rival_policy = 0;
			if(teamkind==0) rts[i].rival_policy = 3;
		}

		/*
			metho 0: 主动避开敌方，认为避开敌方是最优的 (雷达不合并, 地图距离大增)
			metho 1: {
				在地方工作台之间随机移动，如果碰到障碍不进行操作
				不避开敌方，并认为阻碍敌方是优秀的
				在此策略,随时可以转到 4策略
			}
			metho 2: 不主动避开敌方，但认为敌方的无法阻碍移动 (雷达合并, 地图距离小增)
			metho 3: 不主动避开敌方，但认为敌方阻碍了我们的移动 (雷达合并, 地图距离大增)
			metho 4: 当无法找到工作任务，不妨直接来进行攻击

		*/
		if(mapkind == 3) {
			// 3类地图蓝方派两个机器人进行干扰，红方派一个进行干扰
			// if(teamkind) rts[0].rival_policy = 1;
			rts[0].rival_policy = 1;
			if(teamkind == 0) rts[1].rival_policy = 1;
		}


		//
	}

	bool nowPositonInEwpk(double x, double y, int wpid) {
		double diss= dis(x, y, e_wps[wpid].p_x, e_wps[wpid].p_y);
		return diss < 0.45;
	}

	bool searchRivalWpk(Rot& rt) {
		int buywp = -1, sellwp = -1, mi = -1, cur = 0, cmi = -1;
		int cbuywp,csellwp;
		if(rt.rival_policy == 1 || rt.rival_policy == 4) {
			for(int i = 9;i > 3;i--) {
				for(int j = 0;j < e_wpk[i].size();j++) {
					int id = e_wpk[i][j];
					if(nowPositonInEwpk(rt.p_x, rt.p_y, id)) {
						continue;
					}
					for(int l = 0; l < e_wps[id].sellwps.size(); l++) {
						int bid = e_wps[id].sellwps[l];
						if(nowPositonInEwpk(rt.p_x, rt.p_y, bid)) continue; 
						int gtbuy = e_wpdis[bid][rt.goods? 1:0][rt.x][rt.y];
						int gtsell = e_wpdis[id][1][e_wps[bid].x][e_wps[bid].y] + 1;
						int tartime = gtbuy + gtsell;
						if(mi == -1 || mi > tartime) {
							cmi = mi, cbuywp = buywp, csellwp = sellwp;
							buywp = bid, sellwp = id;
							mi = tartime;					
						}
						else if(cmi == -1 || cmi > tartime) {
							cmi = tartime, cbuywp = buywp, csellwp = sellwp;
						}
					}
				}
			}
		}
		if(cmi != -1 && (frameId&1)) {
			buywp = cbuywp;
			sellwp = csellwp;
		}
		// logs.fp<<"rival search: buywp = "<< buywp <<" sellwp = "<<sellwp<<" policy = "<< rt.rival_policy<<endl;
		if (buywp != -1) {
			rt.tarWp.push(buywp);
			rt.tarWp.push(sellwp);
			return true;
		}
		return false;
	}


	// 寻找最近的可以操作的的工作台
	bool searchwpk(Rot &rt) {
		int buywp = -1, sellwp = -1, mi = 0, cur = 0;
		// int needbuywp = -1, needsellwp = -1, needmi = 0, needcur = 0;
		int befswitchtime = -1;
		double maxprofit = 0;
		// logs.fp << "break1" << endl;
		if(rt.goods == 0) {
			for (int i = 9; i > 3; i--) {
				for (int j = 0; j < wpk[i].size(); j++) {
					int id = wpk[i][j];
					int x = (int) (wps[id].recovePoint.fir/0.5);
					int y = (int) (wps[id].recovePoint.sec/0.5);
					if (wps[id].nexgoods.size() - wps[id].nowp >= 3) continue;
					double staf = wps[id].getProductprofitf();
					for (int l = 0; l < wps[id].sellwps.size(); l++) {
						int bid = wps[id].sellwps[l];
						if(bid == rt.befswitch) continue;
						//计算整段路程的所需的时间
						int xx = (int) (wps[bid].recovePoint.fir/0.5);
						int yy = (int) (wps[bid].recovePoint.sec/0.5);					
						int gtbuy =  wpdis[bid][0][rt.x][rt.y];
						int gtsell = wpdis[id][1][wps[bid].x][wps[bid].y] + 1;
						if(wps[bid].needclear == 1) gtsell = wpdis[id][1][xx][yy]+1;
						// if(wps[bid].kind == 7) {
						// 	logs.fp<<"this way: " <<gtbuy<<' '<<gtsell<<endl;
						// }
						// if(wps[id].kind == 5) {
						// 	logs.fp<<"why way: " <<gtbuy<<' '<<gtsell<<endl;
						// }					
						// if(wps[bid].kind == 1 && bid == 9 )logs.fp<<"searchwpk: "<< wps[bid].x<<' '<<wps[bid].y<<endl;
						if(gtbuy >= inf || gtsell >= inf) continue;
						if (wps[bid].nexgoods.size() <= wps[bid].nowp) continue; //如果购买的平台没有商品直接跳过
						if (wps[id].needclear && (wps[id].exsist(wps[bid].kind) || wps[id].staflg)) continue;
						if (wps[id].stalist[wps[bid].kind].size() != 0) continue;
						//double prof = profit[wps[bid].kind] * wps[id].tarmoneyf;
						double prof = profit[wps[bid].kind]*wps[id].tarmoneyf*(wps[id].rtime==-1? (wps[id].kind == 7 ? 1.4 : 1.25): (wps[id].rtime == 0? 0.8:1.0))*staf*penaltyP[wps[id].kind];
						if(i == 9 && wps[bid].kind != 7) prof *= 0.1;
						int buywait = max(wps[bid].nexgoods[wps[bid].nowp] + wps[bid].fid - gtbuy, 0);
						int sellwait = max(wps[id].lastc - (gtbuy + buywait + gtsell), 0);
						int tartime = gtbuy + buywait+ gtsell + sellwait;
						if(buywait + sellwait > 150) continue;
						double prodt = gtbuy + (buywait + sellwait) * (buywait + sellwait) / 75.0 + gtsell;					
						if (buywp == -1 || prof/ prodt > maxprofit) {
							buywp = bid, sellwp = id;
							mi = tartime, cur = gtbuy;
							maxprofit = prof/ prodt;
						}
					}
				}
			}
		}
		else {
			int kdbit = getsta(rt.goods);
			for (int i = 9; i > 3; i--) {
				if(!(ksta[i] & kdbit)) continue;
				for (int j = 0; j < wpk[i].size(); j++) {
					int id = wpk[i][j];
					if (wps[id].nexgoods.size() - wps[id].nowp >= 3) continue;
					// int x = (int) (wps[id].recovePoint.fir/0.5);
					// int y = (int) (wps[id].recovePoint.sec/0.5);
					int gtsell = wpdis[id][1][rt.x][rt.y] + 1;
					if(gtsell >= inf) continue;
					if (wps[id].stalist[rt.goods].size() != 0) continue;
					int sellwait = max(wps[id].lastc - gtsell, 0);
					int tartime = gtsell + sellwait;
					if(sellwait > 150) continue;
					if(sellwp == -1 || mi > tartime) {
						if(id == rt.befswitch) {
							befswitchtime = mi;
							continue;
						}
						mi = tartime;
						sellwp = id;
					}			
				}
			}
		}
		// logs.fp<<"serch buy sell now mi: "<< buywp<<' '<<sellwp<< ' '<< rt.nowWp <<' '<<mi<< endl;
		// logs.fp<<"buy Point("<<' '<<wps[buywp].p_x<<", "<<wps[buywp].p_y<<')'<<endl;
		if (buywp != -1 && sellwp != -1 && mi <= 12000-frameId) {
			rt.tarWp.push(buywp);
			rt.tarWp.push(sellwp);
			wps[buywp].updateBybuy(cur);
			wps[sellwp].updateBysell(mi, wps[buywp].kind);
			rt.starttime = frameId;
			rt.maybetime = mi;
			return true;
		}
		if(buywp == -1 &&sellwp == -1 && befswitchtime != -1) {
			sellwp = rt.befswitch;
			mi = befswitchtime;
		}
		if (buywp == -1 && sellwp != -1 && mi <= 12000-frameId) {
			rt.tarWp.push(sellwp);
			wps[sellwp].updateBysell(mi, rt.goods);
			rt.starttime = frameId;
			rt.maybetime = mi;
		}
		// logs.fp<<sellwp<<' '<<sellwp<<' '<< befswitchtime <<endl;
		if(buywp == -1 && sellwp == -1 && befswitchtime == -1) {
			//你都找不到工作台了，干！
			// logs.fp<<"attacking " <<endl;
		 	rt.rival_policy = 4;
		}
		return false;
	}

	void buy(int index, Rot& rt) {
		// logs.fp << "buying " << rt.tarWp.size() << endl;
		if (rt.tarWp.empty()) return;
		int nowtar = rt.tarWp.front();
		rt.tarWp.pop();
		int nextar = rt.tarWp.front();
		rt.tarWp.pop();
		rt.tarWp.push(nowtar);
		rt.tarWp.push(nextar);
		int x = wps[nowtar].x, y = wps[nowtar].y;
		if(wps[nowtar].needclear) {
			x = (int) (wps[nowtar].recovePoint.fir/0.5);
			y = (int) (wps[nowtar].recovePoint.sec/0.5);
		}
		if(12000 - frameId < wpdis[nextar][1][x][y]) {
			while(!rt.tarWp.empty()) rt.tarWp.pop();
			return ;
		}
		rt.tarWp.pop();
		// logs.fp << "buying " << rt.tarWp.size() << endl;
		
		if(rt.goods >= 4 && rt.goods <= 6) befPP[rt.goods] /= penaltyF;
		cmds.push_back(Cmd(2, index));
		rt.goods = wps[nowtar].kind;
		wps[nowtar].okgoods = 0;
		rt.pos.fir = rt.pos.sec = -1;
		rt.rtlevel = levelup++;
		rt.recovering = rt.needWait = 0;
		rt.befswitch = -1;
		if(teamkind == 1) {
			if(rt.rival_policy == 3) rt.rival_policy = 0;
		}		
		// logs.fp<<"now target is " << rt.tarWp.front()<<' '<<nowtar<<' '<<nextar<<endl;

	}

	void sell(int index, Rot& rt) {
		//logs.fp << "selling" << rt.tarWp.size() << endl;
		// if(index == 0) return ;
		if (rt.tarWp.empty()) return;
		if(teamkind == 1) {
			if(rt.rival_policy == 3) rt.rival_policy = 0;
		}
		cmds.push_back(Cmd(3, index));
		int nowtar = rt.tarWp.front();
		wps[nowtar].addsta(rt.goods);
		rt.goods = 0;
		if(wps[nowtar].sta == ksta[wps[nowtar].kind]) befPP[wps[nowtar].kind] *= penaltyF;
		rt.tarWp.pop();
		rt.pos.fir = rt.pos.sec = -1;
		rt.rtlevel = leveldown--;
		rt.recovering = rt.needWait = 0;
		rt.befswitch = -1;
	}

	void loadrotcmd() {
		// 预处理把已经既定的目标载入
		vector<pair<int, int>> sellList;
		vector<int> buyList;
		for (int i = 1; i <= 9; i++) {
			for (int j = 0; j < wpk[i].size(); j++) {
				int wpID = wpk[i][j];
				sellList.clear();
				buyList.clear();
				for (int rid = 0; rid < 4; rid++) {
					Rot rt = rts[rid];
					if(rt.rival_policy == 1 || rt.rival_policy == 4) continue;
					int befx = rt.x, befy = rt.y;//TODO
					int fra = 0, goods = rt.goods, bef = rt.nowWp;
					while (!rt.tarWp.empty()) {
						int sta = goods? 1:0;
						int wpid = rt.tarWp.front();
						fra += wpdis[wpid][sta][befx][befy];
						// if (bef == -1) fra += culfra(befx, befy, wps[wpid].p_x, wps[wpid].p_y);
						// else fra += disfra[bef][wpid];
						befx = wps[wpid].x;
						befy = wps[wpid].y;
						bef = wpid;
						rt.tarWp.pop();
						if (wpid == wpID) {
							//如果相等则表示机器人存在操作该工作台的指令
							if (goods == 0) {
								if(rt.goods == 0 && wps[wpID].okgoods == 1 && rt.nowWp == wpID) {
									buy(rid, rts[rid]);
								}
								if(rts[rid].tarWp.size()) buyList.push_back(fra);
							}
							if(goods != 0){
								sellList.push_back(pair<int,int>(fra,goods));
							}
						}
						goods = (goods ? 0 : wps[wpid].kind);
					}
				}
				sort(sellList.begin(), sellList.end());
				sort(buyList.begin(), buyList.end());
				for (int i = 0; i < sellList.size(); i++) wps[wpID].updateBysell(sellList[i].first, sellList[i].second);
				for (int i = 0; i < buyList.size(); i++) wps[wpID].updateBybuy(buyList[i]);
				//处理已经可以执行指令的机器人
				for (int rid = 0; rid < 4; rid++) {
					Rot rt = rts[rid];
					if (rt.tarWp.empty()) continue;
					if (rt.nowWp != rt.tarWp.front()) continue;
					if (rt.nowWp != wpID) continue;
					int nowtar = rt.tarWp.front();

					// if (rt.goods == 0 && wps[nowtar].okgoods == 1) {
					// 	buy(rid, rt);
					// }
					
					if (rt.goods != 0 && !wps[nowtar].exsist(rt.goods)) {
						sell(rid, rt);
					}
					rts[rid] = rt;
				}
				
			}
		}

		for(int i = 0;i<4;i++) {
			if(rts[i].rival_policy != 1 && rts[i].rival_policy != 4) continue;
			if(rts[i].tarWp.empty()) continue;
			int tar = rts[i].tarWp.front();
			double diss = dis(rts[i].p_x, rts[i].p_y, e_wps[tar].p_x, e_wps[tar].p_y);
			if(diss < 0.4) {
				rts[i].tarWp.pop();
			}
		}
	}

	// void checkStar

	void loophandle(int rid, Rot& rt, int sta,int moveAngle, int& mi,pair<double,double> &p, int& mimoveAngle, int& rival_mi){
		double madis = rt.sradar.getmaxdistance(moveAngle, sta? 0.53:0.45);
		double nowdis = madis;
		int cmi = -1;
		int rival_cmi = -1;
		pair<double,double> a = get_offset(0.1, moveAngle), nowpos = pair<double,double>(rt.p_x, rt.p_y);
		pair<int,int> befpos = pair<int,int>(rt.x, rt.y), curpos = pair<int,int>(rt.x, rt.y);
		pair<double,double> addv = get_offset(madis, moveAngle);

		rtm[rid] = RotMove(rt.p_x, rt.p_y, rt.p_x + addv.fir, rt.p_y + addv.sec,sta, 1);
		bool rival = false;
		if(!checkMove(rid)) return ;
		while(nowdis > 0) {
			if(nowpos.fir < 0 || nowpos.fir >= 50 || nowpos.fir < 0 || nowpos.sec >= 50) break; 
			if(befpos == curpos) {
				nowpos.fir += a.fir;
				nowpos.sec += a.sec;
				curpos.fir = (int) (nowpos.fir/0.5);
				curpos.sec = (int) (nowpos.sec/0.5);
				nowdis -= 0.1;
			}
			else {
				//TODO:检查是否可行
				befpos = curpos;
				int d = rt.rtdis[curpos.fir][curpos.sec];
				if(teamkind == 0 && e_map[curpos.fir][curpos.sec] != 0 && rt.goods == 0 && d < rt.rtdis[rt.x][rt.y]) {
					rival = 1;
				}
				if(cmi == -1 || cmi > d) {
					cmi = d;
				} 
				if(rival) {
					if(rival_cmi = -1 || rival_cmi > d) {
						rival_cmi = d;
					}
				}
			}
		}
		if(rival_mi == -1) {
			if(cmi!= -1 && (mi == -1 || mi > cmi)) {
				mi = cmi;
				p = pair<double,double>(rt.p_x + addv.fir + a.fir, rt.p_y + addv.sec + a.sec);
				mimoveAngle = moveAngle;
				// logs.fp<<""
			}
		}
		if(rival_cmi != -1 && (rival_mi == -1 || rival_mi > rival_cmi)) {
			rival_mi = rival_cmi;
			p = pair<double,double>(rt.p_x + addv.fir + a.fir, rt.p_y + addv.sec + a.sec);
			mimoveAngle = moveAngle;
		}

		// checkMove(rid);
		return ;
	}


	void rotRadarMerge(int rid, Rot& rt) {
		if(rt.rival_policy == 1 || rt.rival_policy == 2 || rt.rival_policy == 3 || rt.rival_policy == 4) { //合并敌方机器人的雷达
			for(int i = 0;i<e_rot.size();i++) {
				enemy_rot& cur = e_rot[i];
				int anglebetweenrot = getAngle(getPi_to_rightline(pair<double,double>(cur.x - rt.p_x, cur.y - rt.p_y)));
				double cirdis = dis(rt.p_x, rt.p_y, cur.x, cur.y);
				double radis = cur.radius;
				for(int j = 0; j<360; j++) {
					pair<double, double> addv = get_offset(rt.radar.dist[j], j);
					pair<double,double> node = pair<double,double>(rt.p_x + addv.fir, rt.p_y + addv.sec);
					double curdis = dis(node.fir, node.sec, cur.x, cur.y);
					if(!(curdis - radis > bias)) {
						int sita;
						if(anglebetweenrot <= j) {
							sita = j - anglebetweenrot;
						}
						else {
							sita = j + (360- anglebetweenrot);
						}
						sita = min(sita, 360-sita);
						rt.sradar.dist[j] = max(rt.sradar.dist[j], (cirdis+2*radis)/coss[sita]);						
					}
				}
			}
		}
		//将己方机器人合并进雷达
		for(int i = 0;i<4;i++) {
			if(i == rid) continue;
			if(rtm[i].ready) continue;
			Rot& cur = rts[i];
			int sta = cur.goods? 1:0;
			int anglebetweenrot = getAngle(getPi_to_rightline(pair<double,double>(cur.p_x - rt.p_x, cur.p_y - rt.p_y)));
			double cirdis = dis(rt.p_x, rt.p_y, cur.p_x, cur.p_y);
			double radis = (sta? 0.53:0.45);
			// if(rid == 0) logs.fp<<"rid = "<<rid<<" i = "<< i<<" anglebetweenrot = "<<anglebetweenrot<<" radis = "<< radis<< endl;
			for(int j = 0;j<360;j++) {
				pair<double,double> addv = get_offset(rt.radar.dist[j], j);
				pair<double,double> node = pair<double,double>(rt.p_x + addv.fir, rt.p_y + addv.sec);
				double curdis = dis(node.fir, node.sec, cur.p_x, cur.p_y);
				// if(rid == 0) logs.fp<<"	j = "<< j<<" curdis = " << curdis <<endl;
				if(!(curdis - radis > bias)) {
					// logs.fp<<"merge "<< rid<<' '<<i<<endl;
					//证明是己方的点,在该角度进行雷达合并
					int sita;
					if(anglebetweenrot <= j) {
						sita = j - anglebetweenrot;
					}
					else {
						sita = j + (360- anglebetweenrot);
					}
					sita = min(sita, 360-sita);
					// rt.sradar.dist[j] += getdelta(1, 2*coss[sita]*cirdis, cirdis* cirdis - radis*radis);
					rt.sradar.dist[j] = max(rt.sradar.dist[j], (cirdis+2*radis)/coss[sita]);
					// if(rid == 0) logs.fp<<"		j = "<< j<<" "<<rt.radar.dist[j]<<" -> "<<rt.sradar.dist[j]<<endl;
				}
			}
		}
	}

	//更换为角度驱动
	pair<double,double> radarSearch(Rot& rt,int rid) {
		//在此之前完成搜索雷达的合并
		pair<double, double> p = pair<double, double>(rt.p_x, rt.p_y);
		int sta = rt.goods? 1:0;
		int mi = -1,mimoveAngle = 0;
		int rival_mi = -1;
		for(int sita = 0; sita<=180;sita+=10) {
			int moveAngle = (rt.angle + sita) % 360;
			loophandle(rid, rt, sta, moveAngle, mi, p, mimoveAngle, rival_mi);
			if(sita == 0 || sita == 180) continue;
			moveAngle = (rt.angle - sita+360) % 360;
			loophandle(rid, rt, sta, moveAngle, mi, p, mimoveAngle, rival_mi);
		}
		if(mi==-1 && rival_mi == -1) {
			if(levelup == rt.rtlevel+1 && rt.rival_policy == 0) {
				rt.rival_policy = 3;
			}
			rt.rtlevel = levelup++;
		}
		// if(rival_mi != -1) {
		// 	logs.fp<<"try to attack rid = "<<rid<<endl;
		// }
		// logs.fp<<"rid = "<< rid<< " level =  "<< rt.rtlevel <<" policy = "<<rt.rival_policy<< " levelup = "<< levelup<<endl;;
		// if(mi > wpdis[wpid][sta][rt.x][rt.y] &&abs(rt.angle - mimoveAngle) > 145) rt.forward = 0;
		return p;
	}
	
	void move(Rot& rt, int rid) {
		//不同区域进行区域转移
		pair<double, double> nexPoint = radarSearch(rt,rid);
		rts[rid].pos = nexPoint;
	}

	// 检查是否可以跟踪，或者开始跟踪
	bool checkCanTrack(int eid, int rid, Rot& rt, int& p) {
		for(int i = 0; i< e_rot.size(); i++) {
			enemy_rot& ert = e_rot[i];
			if(ert.id != eid) continue;
			int moveAngle = getAngle(getPi_to_rightline(pair<double,double>(ert.x-rt.p_x, ert.y-rt.p_y)));
			double diss = dis(ert.x, ert.y, rt.p_x, rt.p_y);
			double madis = rt.sradar.getmaxdistance(moveAngle, rt.goods? 0.53:0.45);	
			// logs.fp<< "		eid = " <<eid<<" ( "<< ert.x<<" , "<< ert.y <<" ) "<< diss<< " " << madis << " " <<  moveAngle <<endl;
			if(diss > 5) return false;
			if(diss - madis < 1.1) {
				for(int i = 0; i<4;i++) {
					if(rid == i) continue;
					if(eid  == trackID[i]) return false; //如果已经被标记就不攻击了
				}				
				p = i;
				return true;
			}
		}
		return false;
	}

	bool SearchERot(Rot& rt, int rid, pair<double,double>& targetP) {
		int p = -1;
		//先检测自己的追踪的单位是否可以追踪
		if(trackID[rid] != -1){
			if(checkCanTrack(trackID[rid],  rid, rt, p)) {
				//可以跟踪继续跟踪
				//如果追的太远，可以考虑切换了
				if((e_rot[p].stach && e_rot[p].goods == -1) || (rt.tarWp.size() && e_wpdis[rt.tarWp.front()][rt.goods? 1:0][rt.x][rt.y] - rt.attackPos > 100)) {
					trackID[rid] = -1; //如果已经没有商品了，我们可以不考虑继续追踪了.
					rt.attackPos = 0;
				}
				else {
					int bef_p = p;
					//虽然可以继续跟踪,但可能需要更新一个价值更高的单位进行攻击
					for(int i = 0; i<e_rot.size(); i++) {
						if(checkCanTrack(e_rot[i].id, rid, rt, p)) {
							if(e_rot[p].goods == 7) {
								trackID[rid] = e_rot[p].id;
								if(rt.tarWp.size()) rt.attackPos = e_wpdis[rt.tarWp.front()][rt.goods? 1:0][rt.x][rt.y];
								else rt.attackPos = 0;								
							}
							else if(e_rot[p].goods > e_rot[bef_p].goods) {
								trackID[rid] = e_rot[p].id;
								if(rt.tarWp.size()) rt.attackPos = e_wpdis[rt.tarWp.front()][rt.goods? 1:0][rt.x][rt.y];
								else rt.attackPos = 0;								
							}
						}
					}
				}
			}
			else {
				trackID[rid] = -1;//等于负一表示不追踪了
			}
		}
		
		if(trackID[rid] == -1) {
			// logs.fp<<"( "<<rt.p_x<<" , "<<rt.p_y<< ")"<<endl;
			for(int i = 0; i<e_rot.size(); i++) {
				// logs.fp<<"	SearchERot : i = "<<i<< " id = "<< e_rot[i].id<< endl;
				if(checkCanTrack(e_rot[i].id, rid, rt, p)) {
					trackID[rid] = e_rot[i].id;
					if(rt.tarWp.size()) rt.attackPos = e_wpdis[rt.tarWp.front()][rt.goods? 1:0][rt.x][rt.y];
					else rt.attackPos = 0;
				}
			}				
		}
		if(trackID[rid] != -1) {
			targetP = pair<double, double>(e_rot[p].x, e_rot[p].y);
		}
		return trackID[rid] == -1? false:true;
	}

	void dealWait(int rid) {
		rotRadarMerge(rid, rts[rid]);
		Rot& rt = rts[rid];
		rt.v_f = 1.0;
		pair<double,double> targetP;
		// logs.fp<<"rid = "<< rid << " nexwpk = "<< rt.tarWp.front()<<" wps[wpid].needclear = "<<wps[wpid].needclear<< endl;
		// logs.fp<<"needWait: "<< rt.needWait<<endl;
		if((rts[rid].rival_policy == 1) || (rts[rid].rival_policy == 4)) {
			bool canAtk = SearchERot(rts[rid], rid, targetP);
			if(canAtk) rts[rid].pos = targetP;
			else move(rt, rid);
		}
		else if((rts[rid].tarWp.size() && !wps[rts[rid].tarWp.front()].needclear) || rts[rid].tarWp.empty()) {
			move(rt, rid);
		}
		else if(rts[rid].tarWp.size() && wps[rts[rid].tarWp.front()].needclear){
			int wpid = rts[rid].tarWp.front();
			double diss = dis(rt.p_x, rt.p_y, wps[wpid].recovePoint.fir, wps[wpid].recovePoint.sec);
			if(diss < 1) rt.v_f = 0.4;
			if(diss < 0.4 || rt.recovering == 1) {
				rt.pos = pair<double, double>(wps[wpid].p_x, wps[wpid].p_y);
				rtm[rid] = RotMove(rt.p_x, rt.p_y, rt.pos.fir, rt.pos.sec, rt.goods? 1:0, 1);
				rt.needWait = 1;
			}
			else {
				rt.needWait = 0;
				move(rt, rid);
			}
		}

	}

	void RotMaprepanit(int& repanit, int& wpkpaint,int &stapaint) {
		if(frameId & 1) {
			if(wps.size() == 0) return ;
			int sta = stapaint;
			int wpid = wpkpaint;
			stapaint ++;
			if(stapaint == 2) {
				wpkpaint = (wpkpaint + 1)%wps.size();
				stapaint = 0;
			}
			vector<P> modify;
			for(int i = 0; i< e_rotAtk.size(); i++) {
				P node = e_rotAtk[i];
				domainmap[node.fir][node.sec] = teamkind == 0? -3:-2;
				modify.push_back(node);			
			}
			int x = (int) (wps[wpid].recovePoint.fir/0.5);
			int y = (int) (wps[wpid].recovePoint.sec/0.5);

			if(domainmap[wps[wpid].x][wps[wpid].y] < 0) { //为0表示被敌方卡死了
				if(wps[wpid].recover) wps[wpid].needclear = 1;
				// wps[wpid].occupy = 1;
			} 
			else {
				wps[wpid].needclear = 0;
				// wps[wpid].occupy = 1;
			}			
			P st = wps[wpid].needclear? P(x,y): P(wps[wpid].x, wps[wpid].y);
			getOtherRoad(st, wpdis[wpid][sta], sta);
			
			for(int i = 0;i<modify.size();i++) {
				domainmap[modify[i].fir][modify[i].sec] = maps[modify[i].fir][modify[i].sec];
			}		
		}
		else {
			int rid = repanit;
			repanit = (repanit + 1)%4;
			vector<P> modify;
			//考虑敌方机器人如何加入我们的地图
			for(int i = 0; i< e_rotAtk.size(); i++) {
				P node = e_rotAtk[i];
				if(rts[rid].rival_policy == 0) {
					if(teamkind == 1) domainmap[node.fir][node.sec] = -2;
					else if(rts[rid].goods <= 3) domainmap[node.fir][node.sec] = teamkind == 0? -4:-2;
					else domainmap[node.fir][node.sec] = teamkind == 0? -3:-2;
				}
				if(rts[rid].rival_policy == 1 || rts[rid].rival_policy == 4) {
					domainmap[node.fir][node.sec] = -4;
				}
				if(rts[rid].rival_policy == 2) {
					domainmap[node.fir][node.sec] = -2;
				}
				if(rts[rid].rival_policy == 3) {
					domainmap[node.fir][node.sec] = -3;
				}
				if(e_rotOld[i] == 1) {
					domainmap[node.fir][node.sec] = -3;
				}
				modify.push_back(node);
			}
			for(int i = 0;i<4;i++) {
				if(i == rid) continue;
				if(rts[i].rtlevel > rts[rid].rtlevel) {
					int x = rts[i].x, y = rts[i].y;
					for(int ix = -1; ix < 2; ix++) {
						if(x + ix < 0 || x + ix >= sz) continue;
						for(int iy = -1; iy < 2; iy ++) {
							if(y + iy < 0 || y + iy >= sz) continue;
							domainmap[x + ix][y+iy] = -3;
							modify.push_back(P(x+ix, y+iy));
						}
					}
				}
			}
			P start;
			
			if(!rts[rid].tarWp.empty()) {
				int wpid = rts[rid].tarWp.front();
				if(rts[rid].rival_policy != 1 && rts[rid].rival_policy != 4) {
					start = P(wps[wpid].x, wps[wpid].y);
					int x = (int) (wps[wpid].recovePoint.fir/0.5);
					int y = (int) (wps[wpid].recovePoint.sec/0.5);				
					if(wps[wpid].needclear) start = P(x,y);
				}
				else start = P(e_wps[wpid].x, e_wps[wpid].y);
			}
			else {
				start = P(rts[rid].x, rts[rid].y);
				// if(rts[rid].goods != 0) {
				// 	//如果手上有商品, 说明是被卡住了或者操作超时了.

				// }
			}
			int sta = rts[rid].goods? 1:0;
			getOtherRoad(start, rts[rid].rtdis, sta);
			for(int i = 0;i<modify.size();i++) {
				domainmap[modify[i].fir][modify[i].sec] = maps[modify[i].fir][modify[i].sec];
			}
		}
		return ;
	}

	void updatePolicy() {
		for(int i = 0; i<4;i++) {
			if(rts[i].beftimes > 250) {
				if(rts[i].goods != 0 && rts[i].goods <= 3) {
					cmds.push_back(Cmd(4, i));
					rts[i].goods = 0;
					rts[i].rival_policy = 0;
					while(!rts[i].tarWp.empty()) rts[i].tarWp.pop();
				}
			}			
			if(rts[i].rival_policy == 1 || rts[i].rival_policy == 4) continue;
			if(rts[i].tarWp.size() != 0 ) {
				if(frameId -  rts[i].starttime  >  max(250,rts[i].maybetime * 2)) {
					rts[i].befswitch = rts[i].tarWp.front();
					while(!rts[i].tarWp.empty()) rts[i].tarWp.pop();
				}
			}
		}
		// 雷达扫描敌人位置
		for(int i=0;i<4;i++) {
			while(!qu.empty()) qu.pop();
			func(i);//i传入的是机器人下标
		}
		func3(frameId);//处理敌人数组中的旧节点
		// for(int i = 0;i<4;i++) {
		// 	serchEnemy(i);
		// }
		// updateEnemy();
		// logs.fp<<"update e_rot : "<< e_rot.size()<<endl;
		// for(int i = 0; i< e_rot.size(); i++) {
		// 	logs.fp << "	id = "<< e_rot[i].id<< " ( "<< e_rot[i].nx<< " , " <<e_rot[i].ny <<" ) "<<  " ( "<< e_rot[i].x<< " , " <<e_rot[i].y <<" ) frameid = "<< e_rot[i].frameid<<endl;
		// 	logs.fp<<"	goods: "<<e_rot[i].goods<<endl;
		// }		
		// deleteEnemy();
		// logs.fp<<"search e_rot : "<< e_rot.size()<<endl;
		// for(int i = 0; i< e_rot.size(); i++) {
		// 	logs.fp << "	id = "<< e_rot[i].id<< " ( "<< e_rot[i].nx<< " , " <<e_rot[i].ny <<" ) "<<  " ( "<< e_rot[i].x<< " , " <<e_rot[i].y <<" ) frameid = "<< e_rot[i].frameid<<endl;
		// 	logs.fp<<"	goods: "<<e_rot[i].goods<<endl;
		// }
		//处理地方机器人的侵占位置
		e_rotAtk.clear();
		e_rotOld.clear();
		for(int i = 0; i< e_rot.size(); i++) {
			int x = e_rot[i].nx, y = e_rot[i].ny;
			e_map[x][y] = e_rot[i].goods+2;
			if(e_rot[i].stach)
			for(int ix = -1; ix < 2; ix++) {
				if(x + ix < 0 || x + ix >= sz) continue;
				for(int iy = -1; iy < 2; iy ++) {
					if(y + iy < 0 || y + iy >= sz) continue;
					e_rotAtk.push_back(P(x+ix, y+iy));
					e_rotOld.push_back(frameId - e_rot[i].frameid > 750? 1:0);
				}
			}				
		}
		for(int i = 0;i< 10;i++) penaltyP[i] = befPP[i];
		RotMaprepanit(repanit, wpkpaint, stapaint);
		
		loadrotcmd();
		//给目前没有终点的机器人提供目标
		for (int i = 0; i < 4; i++) {
			Rot& rt = rts[i];
			if(frameId % 500 == 0) {
				if(rt.rival_policy == 4) {
					// logs.fp<<"i = "<< i<<" restart "<<endl;
					rt.rival_policy = 3;
					while(!rt.tarWp.empty()) rt.tarWp.pop();
				}
			}			
			if (!rt.tarWp.empty()) continue;
			// 上帝是仁慈的 所有每500帧我们考虑把流浪的人重置
			
			if(rt.rival_policy != 1 && rt.rival_policy != 4) searchwpk(rt);
			if(rt.rival_policy == 1 || rt.rival_policy == 4) searchRivalWpk(rt);
		}
		//给出移动指令
		for(int i = 0;i<4;i++) {
			rtm[i].ready = 0;
			rtm[i].sx = rts[i].p_x;
			rtm[i].sy = rts[i].p_y;
			if(rts[i].tarWp.empty()) continue;
			int nowtar = rts[i].tarWp.front();
			if(rts[i].rival_policy !=1 && rts[i].rival_policy != 4) {
				if((rts[i].goods != 0 && wps[nowtar].exsist(rts[i].goods)) || (rts[i].goods == 0 && wps[nowtar].okgoods == 0)) {
					rts[i].rtlevel = leveldown--;
				}
			}
			if(frameId - rts[i].startRecovering > 250) {
				rts[i].needWait = rts[i].recovering = 0;
			}
			if(rts[i].recovering) {
				rts[i].rtlevel = levelup++;
			}
		}
		for(int ti = 0; ti<4;ti++) {
			int rid = -1;
			for(int i = 0;i<4;i++) {
				if(rtm[i].ready) continue;
				// if(rts[i].tarWp.empty()) continue;
				if(rid == -1 || rts[rid].rtlevel < rts[i].rtlevel) {
					rid = i;
				}
			}
			if(rid != -1) {
				// logs.fp<<"move rid = "<<rid<<endl;
				dealWait(rid);
				rtm[rid].ready = 1;
			}
		}
		for(int i = 0;i<4;i++){
			Rot& rt = rts[i];
			// logs.fp<<"rid= "<< i<< " ( "<<rt.pos.fir<<" , "<<rt.pos.sec<<" ) ( "<<rt.p_x<<" , "<<rt.p_y<<" ) ("<<rt.x<<" , "<<rt.y<<" ) "<< rt.tarWp.size() <<endl;
			double f = 1.0;
			if(!rts[i].tarWp.empty() && (rts[i].rival_policy != 1 && rts[i].rival_policy != 4)) {
				int nowtar = rts[i].tarWp.front();
				double diss = dis(rt.p_x, rt.p_y, wps[nowtar].p_x, wps[nowtar].p_y);
				if(diss - 0.4 < 0.8) rt.v_f = 0.3;
			}
			if(!rts[i].tarWp.empty() && (rts[i].rival_policy == 1 || rts[i].rival_policy == 4)) {
				int nowtar = rts[i].tarWp.front();
				double diss = dis(rt.p_x, rt.p_y, e_wps[nowtar].p_x, e_wps[nowtar].p_y);
				if(diss - 0.4 < 0.4) rt.v_f = 0.6;
			}
			if(!rt.needWait) move_to_target(rt.pos.fir,rt.pos.sec, rt.p_x, rt.p_y, i, rt.dir, cmds, rt.v_f, teamkind);
			else {
				int bef = rt.recovering;
				rt.recovering = move_to_moon(rt.pos.fir,rt.pos.sec, rt.p_x, rt.p_y, i, rt.dir, cmds, rt.v_f, teamkind);
				if(bef == 0 && rt.recovering == 1) {
					rt.startRecovering = frameId;
				}
			}
		}
		// if(teamkind == 0) move_to_target(22.75,44.25, rts[1].p_x, rts[1].p_y, 1, rts[1].dir, cmds, 1, teamkind);
		for(int i = 0; i< e_rot.size(); i++) {
			int x = e_rot[i].nx, y = e_rot[i].ny;
			e_map[x][y] = 0;			
		}		
	}

	void printCmd() {
		for (int i = 0; i < cmds.size(); i++) cmds[i].print();
		cmds.clear();
	}
};
/*
*/