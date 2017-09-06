#ifndef REMOTEPC_PROJECT_LOGGER_H
#define REMOTEPC_PROJECT_LOGGER_H 
#include <windows.h>
#include <string>
#include <queue>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <fstream>

#define LOG_FILE_NAME _T("RemotePCDesktop.txt")
#define MAX_MSG_QUEUE 1000
#define DEBUG_TRACE 0

using namespace std;
class RPCLogger
{
	static RPCLogger *s_instance;
	std::queue<string> msg_queue_;   //Queue of logs
	TCHAR *file_name_;				//Name of log file
	string fullPath;
	HANDLE hlogfile_;				//
	HANDLE sq_empty_;				//Signalled when queue is not not empty
	HANDLE mutex_;					//Mutex for msg_queue_
	DWORD print_thread_id_;			//id of print thread

	RPCLogger(const TCHAR *file_name);
	bool RPCLogger::OpenFile();
	void RPCLogger::CloseFile();
	void PrintLoop();
public:
	
	static DWORD EnterPrintThread(void* _this);
	~RPCLogger(void);
	void static Initialize();
	void static DeInit();
	void static Log(int level, const TCHAR* format, ...);
private:
	void Print(int level,const TCHAR* format, TCHAR* ap);
};







#endif