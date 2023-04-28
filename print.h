#ifndef PRINTH
#define PRINTH
#include <iostream>
#include <fstream>
#pragma GCC optimize(2)
using namespace std;
//日志文件
class Logs {
public:
	ofstream fp;
	Logs() {
		fp.open("log.txt");
	}
	Logs(int teamkind) {
		if(teamkind == 0) fp.open("Bulelog.txt");
		else fp.open("Readlog.txt");
	}	
	void close() {
		fp.close();
	}
};
Logs logs;

class Cmd {
public:
	int opr, rid;
	double pram;

	Cmd(int opr, int rid, double pram = 0) {
		this->opr = opr;
		this->rid = rid;
		this->pram = pram;
	}

	void print() {
		if (opr < 2) {
			if (opr == 0) printf("forward %d %.6f\n", rid, pram);
			else printf("rotate %d %.6f\n", rid, pram);
		}
		else {
			if (opr == 2) printf("buy %d\n", rid);
			else if (opr == 3) printf("sell %d\n", rid);
			else  printf("destroy %d\n", rid);
		}
	}
};
#endif