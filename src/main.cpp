#include <iostream>
#include <stdio.h>
#include <string>
#include <unordered_map>

#include "kline.h"
#include "customMdSpi.h"
#include "ThostFtdcMdApi.h"

using namespace std;

// 链接库
#pragma comment (lib, "LinuxDataCollect.so")
#pragma comment (lib, "thosttraderapi_se.so")

// 账户参数
TThostFtdcBrokerIDType gBrokerID = "9999";                         // 模拟经纪商代码
TThostFtdcInvestorIDType gInvesterID = "";                   // 投资者账户名
TThostFtdcPasswordType gInvesterPassword = "";         // 投资者密码
// 行情参数
CThostFtdcMdApi *g_pMdUserApi = nullptr;                           // 行情指针
char gMdFrontAddr[] = "tcp://180.168.146.187:10211";               // 模拟行情前置地址
char *g_pInstrumentID[] = {(char *)"cu2212"}; 					   // 行情合约代码列表
int instrumentNum = 1;                                             // 行情合约订阅数量
unordered_map<string, TickToKlineHelper> g_KlineHash;              // 不同合约的k线存储表e

int main()
{
	// 账号密码
	cout << "BrokerID: " << gBrokerID << endl;
	cout << "InvesterID: " << gInvesterID << endl;
	cout << "ApiVersion: " << CThostFtdcMdApi::GetApiVersion() << endl;
 	// 初始化行情线程
	cout << "初始化行情..." << endl;
	g_pMdUserApi = CThostFtdcMdApi::CreateFtdcMdApi();   // 创建行情实例
	cout << "create API done." << endl;
	CThostFtdcMdSpi *pMdUserSpi = new CustomMdSpi;       // 创建行情回调实例
	cout << "create SPI done." << endl;
	g_pMdUserApi->RegisterSpi(pMdUserSpi);               // 注册事件类
	cout << "register SPI done." << endl;
	g_pMdUserApi->RegisterFront(gMdFrontAddr);           // 设置行情前置地址
	cout << "register MDfront done." << endl;
	g_pMdUserApi->Init();                                // 连接运行
	cout << "init done." << endl;
	
	// 等到线程退出
	g_pMdUserApi->Join();
	cout << "Join done." << endl;
	delete pMdUserSpi;
	cout << "Spi deleted." << endl;
	g_pMdUserApi->Release();
	cout << "Api Released." << endl;


	// 转换本地k线数据
	TickToKlineHelper tickToKlineHelper;
	tickToKlineHelper.KLineFromLocalData("market_data.csv", "K_line_data.csv");
	
	getchar();
	return 0;
}