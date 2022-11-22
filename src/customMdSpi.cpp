#include <iostream>
#include <fstream>
#include <string.h>
#include <string>
#include <unordered_map>
#include <stdio.h>

#include "customMdSpi.h"
#include "ThostFtdcUserApiStruct.h"
#include "ThostFtdcMdApi.h"
#include "kline.h"

using namespace std;
extern TThostFtdcBrokerIDType gBrokerID;                         // 模拟经纪商代码
extern TThostFtdcInvestorIDType gInvesterID;                         // 投资者账户名
extern TThostFtdcPasswordType gInvesterPassword;                     // 投资者密码
// 行情参数
extern CThostFtdcMdApi *g_pMdUserApi;                           // 行情指针
extern char gMdFrontAddr[];               // 模拟行情前置地址
extern char *g_pInstrumentID[]; 					   // 行情合约代码列表
extern int instrumentNum;                                             // 行情合约订阅数量
extern unordered_map<string, TickToKlineHelper> g_KlineHash;              // 不同合约的k线存储表e


// ---- ctp_api回调函数 ---- //
// 连接成功应答
void CustomMdSpi::OnFrontConnected() {
	std::cout << "=====建立网络连接成功=====" << std::endl;
	cout << "BrokerID: " << gBrokerID << endl;
	cout << "InvesterID: " << gInvesterID << endl;
	cout << "ApiVersion: " << CThostFtdcMdApi::GetApiVersion() << endl;
	// 开始登录
	CThostFtdcReqUserLoginField loginReq;
	memset(&loginReq, 0, sizeof(loginReq));
	strcpy(loginReq.BrokerID, gBrokerID);
	strcpy(loginReq.UserID, gInvesterID);
	strcpy(loginReq.Password, gInvesterPassword);
	static int requestID = 0; // 请求编号
	int rt = g_pMdUserApi->ReqUserLogin(&loginReq, requestID);
	if (!rt)
		std::cout << ">>>>>>发送登录请求成功" << std::endl;
	else
		std::cerr << "--->>>发送登录请求失败" << std::endl;
}

// 断开连接通知
void CustomMdSpi::OnFrontDisconnected(int nReason)
{
	std::cerr << "=====网络连接断开=====" << std::endl;
	std::cerr << "错误码： " << nReason << std::endl;
}

// 心跳超时警告
void CustomMdSpi::OnHeartBeatWarning(int nTimeLapse)
{
	std::cerr << "=====网络心跳超时=====" << std::endl;
	std::cerr << "距上次连接时间： " << nTimeLapse << std::endl;
}

// 登录应答
void CustomMdSpi::OnRspUserLogin(
	CThostFtdcRspUserLoginField *pRspUserLogin, 
	CThostFtdcRspInfoField *pRspInfo, 
	int nRequestID, 
	bool bIsLast)
{
	bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
	if (!bResult)
	{
		std::cout << "=====账户登录成功=====" << std::endl;
		std::cout << "交易日： " << pRspUserLogin->TradingDay << std::endl;
		std::cout << "登录时间： " << pRspUserLogin->LoginTime << std::endl;
		std::cout << "经纪商： " << pRspUserLogin->BrokerID << std::endl;
		std::cout << "帐户名： " << pRspUserLogin->UserID << std::endl;
		// 开始订阅行情
		int rt = g_pMdUserApi->SubscribeMarketData(g_pInstrumentID, instrumentNum);
		if (!rt)
			std::cout << ">>>>>>发送订阅行情请求成功" << std::endl;
		else
			std::cerr << "--->>>发送订阅行情请求失败" << std::endl;
	}
	else
		std::cerr << "返回错误--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
}

// 登出应答
void CustomMdSpi::OnRspUserLogout(
	CThostFtdcUserLogoutField *pUserLogout,
	CThostFtdcRspInfoField *pRspInfo, 
	int nRequestID, 
	bool bIsLast)
{
	bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
	if (!bResult)
	{
		std::cout << "=====账户登出成功=====" << std::endl;
		std::cout << "经纪商： " << pUserLogout->BrokerID << std::endl;
		std::cout << "帐户名： " << pUserLogout->UserID << std::endl;
	}
	else
		std::cerr << "返回错误--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
}

// 错误通知
void CustomMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
	if (bResult)
		std::cerr << "返回错误--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
}

// 订阅行情应答
void CustomMdSpi::OnRspSubMarketData(
	CThostFtdcSpecificInstrumentField *pSpecificInstrument, 
	CThostFtdcRspInfoField *pRspInfo, 
	int nRequestID, 
	bool bIsLast)
{
	bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
	if (!bResult)
	{
		std::cout << "=====订阅行情成功=====" << std::endl;
		std::cout << "合约代码： " << pSpecificInstrument->reserve1 << std::endl;
		// 如果需要存入文件或者数据库，在这里创建表头,不同的合约单独存储
		char filePath[100] = {'\0'};
		sprintf(filePath, "%s_market_data.csv", pSpecificInstrument->reserve1);
		std::ofstream outFile;
		outFile.open(filePath, std::ios::out); // 新开文件
		outFile << "合约代码" << ","
			<< "更新时间" << ","
			<< "最新价" << ","
			<< "上次结算价" << ","
			<< "昨收盘" << ","
			<< "昨持仓量" << ","
			<< "今开盘" << ","
			<< "最高价" << ","
			<< "最低价" << ","
			<< "数量" << ","
			<< "成交金额" << ","
			<< "持仓量" << ","
			<< "今收盘" << ","
			<< "本次结算价" << ","
			<< "涨停版价" << ","
			<< "跌停板价" << ","
			// << "昨虚实度" << ","
			// << "今虚实度" << ","
			<< "申买价一" << ","
			<< "申买量一" << ","
			<< "申卖价一" << ","
			<< "申卖量一" << ","
			// << "申买价二" << ","
			// << "申买量二" << ","
			// << "申卖价二" << ","
			// << "申卖量二" << ","
			// << "申买价三" << ","
			// << "申买量三" << ","
			// << "申卖价三" << ","
			// << "申卖量三" << ","
			// << "申买价四" << ","
			// << "申买量四" << ","
			// << "申卖价四" << ","
			// << "申卖量四" << ","
			// << "申买价五" << ","
			// << "申买量五" << ","
			// << "申卖价五" << ","
			// << "申卖量五" << ","
			<< "当日均价" << ","
			<< "业务日期"
			// << "上带价" << ","
			// << "下带价"
			<< std::endl;
		outFile.close();
	}
	else
		std::cerr << "返回错误--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
}

// 取消订阅行情应答
void CustomMdSpi::OnRspUnSubMarketData(
	CThostFtdcSpecificInstrumentField *pSpecificInstrument, 
	CThostFtdcRspInfoField *pRspInfo,
	int nRequestID, 
	bool bIsLast)
{
	bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
	if (!bResult)
	{
		std::cout << "=====取消订阅行情成功=====" << std::endl;
		std::cout << "合约代码： " << pSpecificInstrument->reserve1 << std::endl;
	}
	else
		std::cerr << "返回错误--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
}

// 订阅询价应答
void CustomMdSpi::OnRspSubForQuoteRsp(
	CThostFtdcSpecificInstrumentField *pSpecificInstrument,
	CThostFtdcRspInfoField *pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
	if (!bResult)
	{
		std::cout << "=====订阅询价成功=====" << std::endl;
		std::cout << "合约代码： " << pSpecificInstrument->reserve1 << std::endl;
	}
	else
		std::cerr << "返回错误--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
}

// 取消订阅询价应答
void CustomMdSpi::OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
	if (!bResult)
	{
		std::cout << "=====取消订阅询价成功=====" << std::endl;
		std::cout << "合约代码： " << pSpecificInstrument->reserve1 << std::endl;
	}
	else
		std::cerr << "返回错误--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
}

// 行情详情通知
void CustomMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	// 打印行情，字段较多，截取部分
	std::cout << "=====获得深度行情=====" << std::endl;
	std::cout << "交易日： " << pDepthMarketData->TradingDay << std::endl;
	std::cout << "合约代码： " << pDepthMarketData->reserve1 << std::endl;
	std::cout << "交易所代码： " << pDepthMarketData->ExchangeID << std::endl;
	std::cout << "最新价： " << pDepthMarketData->LastPrice << std::endl;
	std::cout << "上次结算价： " << pDepthMarketData->PreSettlementPrice << std::endl;
	std::cout << "昨收盘： " << pDepthMarketData->PreClosePrice << std::endl;
	std::cout << "昨持仓量： " << pDepthMarketData->PreOpenInterest << std::endl;
	std::cout << "今开盘： " << pDepthMarketData->OpenPrice << std::endl;
	std::cout << "最高价： " << pDepthMarketData->HighestPrice << std::endl;
	std::cout << "最低价： " << pDepthMarketData->LowestPrice << std::endl;
	std::cout << "数量： " << pDepthMarketData->Volume << std::endl;
	std::cout << "成交金额： " << pDepthMarketData->Turnover << std::endl;
	std::cout << "持仓量： " << pDepthMarketData->OpenInterest << std::endl;
	std::cout << "今收盘： " << pDepthMarketData->ClosePrice << std::endl;
	std::cout << "本次结算价： " << pDepthMarketData->SettlementPrice << std::endl;
	std::cout << "涨停板价： " << pDepthMarketData->UpperLimitPrice << std::endl;
	std::cout << "跌停板价： " << pDepthMarketData->LowerLimitPrice << std::endl;
	// std::cout << "昨虚实度： " << pDepthMarketData->PreDelta << std::endl;
	// std::cout << "今虚实度： " << pDepthMarketData->CurrDelta << std::endl;
	std::cout << "最后修改时间： " << pDepthMarketData->UpdateTime << std::endl;
	std::cout << "最后修改毫秒： " << pDepthMarketData->UpdateMillisec << std::endl;
	std::cout << "申买价一： " << pDepthMarketData->BidPrice1 << std::endl;
	std::cout << "申买量一： " << pDepthMarketData->BidVolume1 << std::endl;
	std::cout << "申卖价一： " << pDepthMarketData->AskPrice1 << std::endl;
	std::cout << "申卖量一： " << pDepthMarketData->AskVolume1 << std::endl;
	// std::cout << "申买价二： " << pDepthMarketData->BidPrice2 << std::endl;
	// std::cout << "申买量二： " << pDepthMarketData->BidVolume2 << std::endl;
	// std::cout << "申卖价二： " << pDepthMarketData->AskPrice2 << std::endl;
	// std::cout << "申卖量二： " << pDepthMarketData->AskVolume2 << std::endl;
	// std::cout << "申买价三： " << pDepthMarketData->BidPrice3 << std::endl;
	// std::cout << "申买量三： " << pDepthMarketData->BidVolume3 << std::endl;
	// std::cout << "申卖价三： " << pDepthMarketData->AskPrice3 << std::endl;
	// std::cout << "申卖量三： " << pDepthMarketData->AskVolume3 << std::endl;
	// std::cout << "申买价四： " << pDepthMarketData->BidPrice4 << std::endl;
	// std::cout << "申买量四： " << pDepthMarketData->BidVolume4 << std::endl;
	// std::cout << "申卖价四： " << pDepthMarketData->AskPrice4 << std::endl;
	// std::cout << "申卖量四： " << pDepthMarketData->AskVolume4 << std::endl;
	// std::cout << "申买价五： " << pDepthMarketData->BidPrice5 << std::endl;
	// std::cout << "申买量五： " << pDepthMarketData->BidVolume5 << std::endl;
	// std::cout << "申卖价五： " << pDepthMarketData->AskPrice5 << std::endl;
	// std::cout << "申卖量五： " << pDepthMarketData->AskVolume5 << std::endl;
	std::cout << "当日均价： " << pDepthMarketData->AveragePrice << std::endl;
	std::cout << "业务日期： " << pDepthMarketData->ActionDay << std::endl;
	// std::cout << "合约在交易所的代码： " << pDepthMarketData->ExchangeInstID << std::endl;
	// std::cout << "上带价： " << pDepthMarketData->BandingUpperPrice << std::endl;
	// std::cout << "下带价： " << pDepthMarketData->BandingLowerPrice << std::endl;
	
	// 如果只获取某一个合约行情，可以逐tick地存入文件或数据库
	char filePath[100] = {'\0'};
	sprintf(filePath, "%s_market_data.csv", pDepthMarketData->reserve1);
	std::ofstream outFile;
	outFile.open(filePath, std::ios::app); // 文件追加写入 
	outFile << pDepthMarketData->reserve1 << "," 
		<< pDepthMarketData->UpdateTime << "." << pDepthMarketData->UpdateMillisec << "," 
		<< pDepthMarketData->LastPrice << "," 
		<< pDepthMarketData->PreSettlementPrice << "," 
		<< pDepthMarketData->PreClosePrice << "," 
		<< pDepthMarketData->PreOpenInterest << "," 
		<< pDepthMarketData->OpenPrice << "," 
		<< pDepthMarketData->HighestPrice << "," 
		<< pDepthMarketData->LowestPrice << "," 
		<< pDepthMarketData->Volume << "," 
		<< pDepthMarketData->Turnover << "," 
		<< pDepthMarketData->OpenInterest << "," 
		<< pDepthMarketData->ClosePrice << "," 
		<< pDepthMarketData->SettlementPrice << "," 
		<< pDepthMarketData->UpperLimitPrice << "," 
		<< pDepthMarketData->LowerLimitPrice << "," 
		// << pDepthMarketData->PreDelta << "," 
		// << pDepthMarketData->CurrDelta << "," 
		<< pDepthMarketData->BidPrice1 << "," 
		<< pDepthMarketData->BidVolume1 << "," 
		<< pDepthMarketData->AskPrice1 << "," 
		<< pDepthMarketData->AskVolume1 << "," 
		// << pDepthMarketData->BidPrice2 << "," 
		// << pDepthMarketData->BidVolume2 << "," 
		// << pDepthMarketData->AskPrice2 << "," 
		// << pDepthMarketData->AskVolume2 << ","
		// << pDepthMarketData->BidPrice3 << "," 
		// << pDepthMarketData->BidVolume3 << "," 
		// << pDepthMarketData->AskPrice3 << "," 
		// << pDepthMarketData->AskVolume3 << ","
		// << pDepthMarketData->BidPrice4 << "," 
		// << pDepthMarketData->BidVolume4 << "," 
		// << pDepthMarketData->AskPrice4 << "," 
		// << pDepthMarketData->AskVolume4 << ","
		// << pDepthMarketData->BidPrice5 << "," 
		// << pDepthMarketData->BidVolume5 << "," 
		// << pDepthMarketData->AskPrice5 << "," 
		// << pDepthMarketData->AskVolume5 << ","
		<< pDepthMarketData->AveragePrice << ","
		<< pDepthMarketData->ActionDay
		// << pDepthMarketData->BandingUpperPrice << ","
		// << pDepthMarketData->BandingLowerPrice 
		<< std::endl;
	outFile.close();

	// 计算实时k线
	std::string instrumentKey = std::string(pDepthMarketData->reserve1);
	if (g_KlineHash.find(instrumentKey) == g_KlineHash.end())
		g_KlineHash[instrumentKey] = TickToKlineHelper();
	g_KlineHash[instrumentKey].KLineFromRealtimeData(pDepthMarketData);


	// 取消订阅行情
	//int rt = g_pMdUserApi->UnSubscribeMarketData(g_pInstrumentID, instrumentNum);
	//if (!rt)
	//	std::cout << ">>>>>>发送取消订阅行情请求成功" << std::endl;
	//else
	//	std::cerr << "--->>>发送取消订阅行情请求失败" << std::endl;
}

// 询价详情通知
void CustomMdSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp)
{
	// 部分询价结果
	std::cout << "=====获得询价结果=====" << std::endl;
	std::cout << "交易日： " << pForQuoteRsp->TradingDay << std::endl;
	std::cout << "交易所代码： " << pForQuoteRsp->ExchangeID << std::endl;
	std::cout << "合约代码： " << pForQuoteRsp->reserve1 << std::endl;
	std::cout << "询价编号： " << pForQuoteRsp->ForQuoteSysID << std::endl;
}