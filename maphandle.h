#include "print.h"
#include <iomanip>
#include <vector>
#include <map>
#include "distance.h"
#include "sagmentdis.h"
#define fir first
#define sec second
#pragma GCC optimize(2)
using namespace std;

typedef pair<int,int> P;
const int inf = 1e8+7;
const int sz = 100;
int domain[2][sz][sz];
int f[sz*sz];
int rowh[sz][sz],colh[sz][sz];
int domainmap[sz][sz];
class Domain {
public:
	int kind; // 0:上下移动区域 1:左右移动区域 2:被合并
	int x1,x2,y1,y2;
	vector<P> edge[2];//0: 没有商品 1:有商品
	vector< pair<P,P> > eRange; 
	vector<int> eF;
	Domain() {

	}
	Domain(int kind,int x1,int x2,int y1,int y2) {
		this->kind = kind;
		this->x1 = x1;
		this->x2 = x2;
		this->y1 = y1;
		this->y2 = y2;
	}
	void addPoint(int x,int y) {
		x1 = min(x,x1);
		x2 = max(x,x2);
		y1 = min(y,y1);
		y2 = max(y,y2);
	}
	void addedge(int kind, int id, int l,int r, P com,int f) {
		int le,ri;
		if(kind == 0) {
			le = max(l,x1);
			ri = min(r,x2);
		}
		else {
			le = max(l,y1);
			ri = min(r,y2);
		}
		if(ri-le >=2) {
			if(!kind) eRange.push_back(pair<P,P>(P(le,ri), com));
			else 	  eRange.push_back(pair<P,P>(com, P(le,ri)));
			eF.push_back(f);
			edge[0].push_back(P(id,eRange.size()-1));
		}
		if(ri-le >= 3) {
			edge[1].push_back(P(id,eRange.size()-1));
		}
	}
};

vector<Domain> domains;



class RotMove{
public:
	double sx,sy;
	double tx,ty;
	double r;
	int ready;
	RotMove() {

	}
	RotMove(double sx, double sy, double tx, double ty, int sta, int ready) {
		this->sx = sx;
		this->sy = sy;
		this->tx = tx;
		this->ty = ty;
		this->r = sta? 0.53:0.45;
		this->ready = ready;
	}
};
RotMove rtm[4];

//求两条线段的最小距离
double distance_between_RotMove(RotMove& rm1, RotMove& rm2) {
    return minDis(pair<double,double>(rm1.sx,rm1.sy),pair<double,double>(rm1.tx,rm1.ty), pair<double,double>(rm2.sx,rm2.sy),pair<double,double>(rm2.tx,rm2.ty));
}


bool checkMove(int rid) {
	for(int i = 0;i < 4;i++) {
		if(!rtm[i].ready) continue;
		if(i == rid) continue;
		double madis = rtm[i].r + rtm[rid].r;
		double stadis = ptoldis(pair<double,double>(rtm[i].sx,rtm[i].sy), pair<double,double>(rtm[i].tx,rtm[i].ty), pair<double,double>(rtm[rid].sx,rtm[rid].sy));
		double enddis = ptoldis(pair<double,double>(rtm[i].sx,rtm[i].sy), pair<double,double>(rtm[i].tx,rtm[i].ty), pair<double,double>(rtm[rid].tx,rtm[rid].ty));
		double rotModis = distance_between_RotMove(rtm[i], rtm[rid]);
		// logs.fp<<"madis = "<<madis<<" stadis = "<<stadis<<" rotModis = "<< rotModis<<endl;
		if(stadis < madis) {//如果起始点在内部
			if(rotModis < stadis && abs(rotModis-stadis) > 1e-4) return false;
			if(enddis > madis) continue;
			return false;
		}
		if(rotModis < madis) return false;
	}
	return true;
}

bool merge(Domain& d1, Domain& d2,int kind) {
	if(d1.kind != kind || d2.kind != d1.kind) return false;
	if(kind == 0) {
		if(d1.x1 == d2.x1 && d1.x2==d2.x2) {
			d1.y1 = min(d1.y1,d2.y1);
			d1.y2 = max(d1.y2,d2.y2);
			d2.kind = 2;
			return true;
		}
	}
	else {
		if(d1.y1 == d2.y1 && d1.y2 == d2.y2) {
			d1.x1 = min(d1.x1,d2.x1);
			d1.x2 = max(d1.x2,d2.x2);
			d2.kind = 2;
			return true;
		}
	}
	return false;
}

bool checkCross(int kind ,int id1, int id2,int sta) {
	Domain& dom1 = domains[id1];
	Domain& dom2 = domains[id2];
	int le = (!kind? max(dom1.x1,dom2.x1):max(dom1.y1,dom2.y1));
	int ri = (!kind? min(dom1.x2,dom2.x2):min(dom1.y2,dom2.y2));
	return (!sta)? ri-le>=1:ri-le>=2;
}


int finds(int x) {
	return x==f[x]? f[x]:f[x] = finds(f[x]);
}

void unions(int x, int y) {
	x = finds(x);
	y = finds(y);
	if(x == y) return ;
	f[y] =x;
}

void maphandle(int maps[100][100]) {
	for(int j = 0;j<sz;j++) {
		for(int i =0;i<sz;i++) {
			rowh[i][j] = -1;
			domainmap[i][j] = maps[i][j];
			if(i != 0 && maps[i][j] != -2) rowh[i][j] = rowh[i-1][j];
			if(maps[i][j] == -2) rowh[i][j] = i;
		}
	}

	for(int i = 0;i<sz;i++) {
		for(int j = 0;j<sz;j++) {
			colh[i][j] = -1;
			if(j != 0 && maps[i][j] != -2) colh[i][j] = colh[i][j-1];
			if(maps[i][j] == -2) colh[i][j] = j;
		}
	}

	//处理上下移动区域
	int kind = 0;
	int index = 0;
	for(int j = 0;j<sz;j++) {
		for(int i = 0;i<sz;i++) {
			f[i+100*j] = i+100*j;
			domain[kind][i][j] = -1;
			if(maps[i][j] == -2) continue;
			if(i == 0 || domain[kind][i-1][j] == -1) {
				domains.push_back(Domain(0,i,i,j,j));
				domain[kind][i][j] = index++;
			}
			else {
				domain[kind][i][j] = domain[kind][i-1][j];
				unions((i-1)+100*j, i+100*j);
				domains[domain[kind][i-1][j]].addPoint(i,j);
			}
		}
		

		if(j == 0) continue;
		for(int i = 0;i<sz;i++) {
			if(maps[i][j] == -2) continue;
			if(domain[kind][i][j-1] != -1) {
				if(merge(domains[domain[kind][i][j]], domains[domain[kind][i][j-1]], kind)){
					unions(i+j*100, i+(j-1)*100);
				}
			}
		}
	}

	for(int i = 0; i<sz;i++) {
		for(int j = 0;j<sz;j++) {
			if(domain[kind][i][j] == -1) continue;
			int num = finds(i+j*100);
			int x = num%100;
			int y = num/100;
			domain[kind][i][j] = domain[kind][x][y];

		}
	}

	//处理左右移动的区域
	kind++;
	for(int i = 0;i<sz;i++) {
		for(int j = 0;j<sz;j++){
			f[i+100*j] = i+100*j;
			domain[kind][i][j] = -1;
			if(maps[i][j] == -2) continue;
			if(j == 0 || domain[kind][i][j-1] == -1) {
				domains.push_back(Domain(0,i,i,j,j));
				domain[kind][i][j] = index++;
			}
			else {
				domain[kind][i][j] = domain[kind][i][j-1];
				unions(i+100*j, i+100*(j-1));
				domains[domain[kind][i][j-1]].addPoint(i,j);
			}
		}
		if(i == 0) continue;
		for(int j = 0;j<sz;j++) {
			if(maps[i][j] == -2) continue;
			if(domain[kind][i-1][j] != -1) {
				if(merge(domains[domain[kind][i][j]], domains[domain[kind][i][j-1]], kind)){
					unions(i+j*100, i-1+j*100);
				}
			}
		}
	}
	for(int i = 0; i<sz;i++) {
		for(int j = 0;j<sz;j++) {
			if(domain[kind][i][j] == -1) continue;
			int num = finds(i+j*100);
			int x = num%100;
			int y = num/100;
			domain[kind][i][j] = domain[kind][x][y];
		}
	}

	//开始建边
	kind = 0;
	int befDomain = -1;
	int befDomain2 = -1;
	for(int j = 0;j<sz;j++) {
		for(int i = 0;i<sz;i++) {
			if(domain[kind][i][j] == -1) continue;
			if(j != 0 && domain[kind][i][j-1] != -1 && (befDomain != domain[kind][i][j-1] || befDomain2 != domain[kind][i][j]) && domain[kind][i][j-1] != domain[kind][i][j]) {
				Domain& dom1 = domains[domain[kind][i][j]];
				Domain& dom2 = domains[domain[kind][i][j-1]];
				dom1.addedge(kind, domain[kind][i][j-1], dom2.x1, dom2.x2, P(dom2.y2,dom1.y1),0);
				dom2.addedge(kind, domain[kind][i][j], dom1.x1, dom1.x2, P(dom2.y2,dom1.y1),1);
				befDomain = domain[kind][i][j-1];
				befDomain2 = domain[kind][i][j];
			}
		}
	}
	kind = 1,befDomain = befDomain2 = -1;
	for(int i = 0;i<sz;i++) {
		for(int j =0;j<sz;j++) {
			if(domain[kind][i][j] == -1) continue;
			if(i != 0 && domain[kind][i-1][j] != -1 && (befDomain != domain[kind][i-1][j] || befDomain2 != domain[kind][i][j]) && domain[kind][i-1][j] != domain[kind][i][j]) {
				Domain& dom1 = domains[domain[kind][i][j]];
				Domain& dom2 = domains[domain[kind][i-1][j]];
				dom1.addedge(kind, domain[kind][i-1][j], dom2.y1, dom2.y2, P(dom2.x2,dom1.x1), 0);
				dom2.addedge(kind, domain[kind][i][j], dom1.y1, dom1.y2, P(dom2.x2,dom1.x1), 1);
				befDomain = domain[kind][i-1][j];
				befDomain2 = domain[kind][i][j];
			}
		}
	}
}

priority_queue< P,vector<P>,greater<P> > q;
int vis[100][100];

P getNode(int num) {
	return P(num%sz,num/sz);
}

int getNum(P node) {
	return node.fir + node.sec*100;
}

void dijkstra(P start, int (*d)[100], int sta) {
	for(int i = 0;i< sz;i++) {
		for(int j = 0;j<sz;j++) {
			d[i][j] = inf;
			vis[i][j] = 0;
		}
	}
	while(!q.empty()) q.pop();
	int num = getNum(start);
	d[start.fir][start.sec] = 0;
	q.push(P(0, getNum(start)));
	while(!q.empty()) {
		P u = q.top();
		P uNode = getNode(u.sec);
		q.pop();
		if(vis[uNode.fir][uNode.sec]) continue;
		vis[uNode.fir][uNode.sec] = 1;
		//横向移动
		for(int i = -1; i<=1 ;i +=2) {
			if(uNode.fir + i < 0 || uNode.fir + i >= 100) continue;
			if(domain[1][uNode.fir+i][uNode.sec] != -1) {
				if(checkCross(1,domain[1][uNode.fir][uNode.sec], domain[1][uNode.fir+i][uNode.sec], sta)) {
					P v = P(uNode.fir+i, uNode.sec);
					if(v.fir > 0 &&v.fir < sz-1 && v.sec > 0 &&  v.sec < sz -1) {
						if(domainmap[v.fir-1][v.sec-1] == -2 && domainmap[v.fir+1][v.sec+1] == -2) continue;
						if(domainmap[v.fir+1][v.sec-1] == -2 && domainmap[v.fir-1][v.sec+1] == -2) continue;
					}					
					if(!vis[v.fir][v.sec] && d[v.fir][v.sec] > d[uNode.fir][uNode.sec] + 5) {
						d[v.fir][v.sec] = d[uNode.fir][uNode.sec] + 5;
						q.push(P(d[v.fir][v.sec], getNum(v)));
					}
				}
			}
		}
		//竖向移动
		for(int i = -1; i<=1 ;i +=2) {
			if(uNode.sec + i < 0 || uNode.sec + i >= 100) continue;
			if(domain[0][uNode.fir][uNode.sec+i] != -1) {
				if(checkCross(0,domain[0][uNode.fir][uNode.sec], domain[0][uNode.fir][uNode.sec+i], sta)) {
					P v = P(uNode.fir, uNode.sec+i);
					if(v.fir > 0 &&v.fir < sz-1 && v.sec > 0 &&  v.sec < sz -1) {
						if(domainmap[v.fir-1][v.sec-1] == -2 && domainmap[v.fir+1][v.sec+1] == -2) continue;
						if(domainmap[v.fir+1][v.sec-1] == -2 && domainmap[v.fir-1][v.sec+1] == -2) continue;
					}
					if(!vis[v.fir][v.sec] && d[v.fir][v.sec] > d[uNode.fir][uNode.sec] + 5) {
						d[v.fir][v.sec] = d[uNode.fir][uNode.sec] + 5;
						q.push(P(d[v.fir][v.sec], getNum(v)));
					}
				}
			}
		}		
	}	
	return ;
}

bool checkTryCross(P s, P t,int sta,int f/* 0:横向 1:纵向 */, int& cost) {
	int len = sta? 3:2;
	bool tryCross = false;
	if(!f) {
		for(int i = s.sec + len - 1 ; i >=s.sec ;i --) {
			bool flg = true;
			int edge = 5;
			for(int j = 0; j <len;j++) {
				if(!flg) continue;
				if( i-j >= sz || i-j < 0 ) flg = false;
				if( domainmap[s.fir][i-j] == -2 || domainmap[t.fir][i-j] == -2) flg = false; 
				if( domainmap[s.fir][i-j] == -3 || domainmap[t.fir][i-j] == -3) edge = 50;
				if( domainmap[s.fir][i-j] == -4 || domainmap[t.fir][i-j] == -4) edge = 1;
			}
			if(flg) {
				cost = min(cost, edge);
				tryCross = true;
			}
		}
	}
	else {
		for(int i = s.fir + len - 1; i >= s.fir; i--) {
			bool flg = true;
			int edge = 5;
			for(int j = 0; j< len;j++) {
				if(!flg) continue;
				if( i-j >= sz || i-j < 0 ) flg = false;
				if( domainmap[i-j][s.sec] == -2 || domainmap[i-j][t.sec] == -2) flg = false;
				if( domainmap[i-j][s.sec] == -3 || domainmap[i-j][t.sec] == -3) edge = 50;
				if( domainmap[i-j][s.sec] == -4 || domainmap[i-j][t.sec] == -4) edge = 1;
			}
			if(flg) {
				cost = min(cost, edge);
				tryCross = true;
			}
		}
	}
	return tryCross;
}

int maxroad = 0;
int ThisRoundRoad = 0;
void getOtherRoad(P start, int (*d)[100], int sta) {
	for(int i = 0;i< sz;i++) {
		for(int j = 0;j<sz;j++) {
			d[i][j] = inf;
			vis[i][j] = 0;
		}
	}
	ThisRoundRoad = 0;
	while(!q.empty()) q.pop();
	int num = getNum(start);
	d[start.fir][start.sec] = 0;
	q.push(P(0, getNum(start)));
	while(!q.empty()) {
		P u = q.top();
		P uNode = getNode(u.sec);
		maxroad = max(maxroad, u.fir);
		ThisRoundRoad = max(ThisRoundRoad, u.fir);
		q.pop();
		if(vis[uNode.fir][uNode.sec]) continue;
		vis[uNode.fir][uNode.sec] = 1;
		
		//横向移动
		for(int i = -1; i<=1 ;i +=2) {
			P v = P(uNode.fir+i, uNode.sec);
			if(v.fir < 0 || v.fir >= sz) continue;
			if(v.fir > 0 &&v.fir < sz-1 && v.sec > 0 &&  v.sec < sz -1) {
				if(domainmap[v.fir-1][v.sec-1] == -2 && domainmap[v.fir+1][v.sec+1] == -2) continue;
				if(domainmap[v.fir+1][v.sec-1] == -2 && domainmap[v.fir-1][v.sec+1] == -2) continue;
			}
			int cost = inf;			
			if(checkTryCross(uNode, v, sta, 0, cost)) {			
				if(!vis[v.fir][v.sec] && d[v.fir][v.sec] > d[uNode.fir][uNode.sec] + cost) {
					d[v.fir][v.sec] = d[uNode.fir][uNode.sec] + cost;
					q.push(P(d[v.fir][v.sec], getNum(v)));
				}
			}
		}
		//竖向移动
		for(int i = -1; i<=1 ;i +=2) {
			P v = P(uNode.fir, uNode.sec+i);
			if(v.sec < 0 || v.sec >= sz) continue;
			if(v.fir > 0 && v.fir < sz-1 && v.sec > 0 &&  v.sec < sz -1) {
				if(domainmap[v.fir-1][v.sec-1] == -2 && domainmap[v.fir+1][v.sec+1] == -2) continue;
				if(domainmap[v.fir+1][v.sec-1] == -2 && domainmap[v.fir-1][v.sec+1] == -2) continue;
			}	
			int cost = inf;
			if(checkTryCross(uNode, v, sta, 1, cost)) {
				if(!vis[v.fir][v.sec] && d[v.fir][v.sec] > d[uNode.fir][uNode.sec] + cost) {
					d[v.fir][v.sec] = d[uNode.fir][uNode.sec] + cost;
					q.push(P(d[v.fir][v.sec], getNum(v)));
				}
			}
		}		
	}

	return ;	
}