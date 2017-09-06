#include "stdafx.h"
#include "RPCLogger.h"
#define LINE_BUFFER_SIZE 2048

RPCLogger *RPCLogger::s_instance = NULL;


RPCLogger::RPCLogger(const TCHAR *file_name)
{
	sq_empty_ = CreateEvent(NULL,true,false,NULL);
	mutex_ = CreateMutex(NULL,false,NULL);
	file_name_ = _tcsdup(file_name);
	CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)&EnterPrintThread,(void *)this,0,&print_thread_id_);
}

void RPCLogger::Initialize()
{
	if(s_instance==NULL)
	{
		s_instance = new RPCLogger(LOG_FILE_NAME);
	}
}

void RPCLogger::DeInit()
{
	if(s_instance==NULL)
	{
		delete s_instance;
		s_instance = NULL;
	}
}

void RPCLogger::Log(int level, const TCHAR* format, ...)
{
	if(s_instance != NULL)
	{
		TCHAR* ap = NULL;
		//va_start(ap, format);
		s_instance->Print(level,format,ap);
		//va_end(ap);
	}
}

bool RPCLogger::OpenFile()
{
	if(file_name_==NULL)
	{
		printf("Error::RPCLogger:  file_name_=null");
		return false;
	}
	
	//string strLogFilePath = _T("");
	//TCHAR logPath[256] = {'\0'};
	//if (GetModuleFileName(NULL, logPath, 256))
	//{
	//	TCHAR* p = _tcsstr(logPath, _T("\\"));
	//	if (p != NULL)
	//	{
	//		*p = '\0';
	//		string pathIni(logPath);// m_iniPath;
	//		TCHAR path[256] = {'\0'};
	//		pathIni.append("\\Path.ini");
	//		GetPrivateProfileString(_T("Path"),_T("path"),NULL,path,256,pathIni);
	//		pathIni.append(path);
	//		strLogFilePath.operator= (path);
	//	}
	//}
	//strLogFilePath.append(file_name_);
	//fullPath = strLogFilePath;

	hlogfile_ = CreateFile(
		_T("E:\\abc.txt"),  GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL  );
	//hlogfile_ = CreateFile(
     //   file_name_,  GENERIC_WRITE, FILE_SHARE_READ, NULL,
      //  OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL  );
	if (hlogfile_ == INVALID_HANDLE_VALUE)
	{
		printf("Error::RPCLogger:CreateFile() error=%d",GetLastError());
		return false;	
	}
	SetFilePointer(hlogfile_,0,NULL,FILE_END );
	return true;
}
void RPCLogger::CloseFile() {
    if (hlogfile_ != NULL) {
        CloseHandle(hlogfile_);
        hlogfile_ = NULL;
    }
}
void RPCLogger::Print(int level, const TCHAR* format,TCHAR* ap)
{
	TCHAR log_buff[LINE_BUFFER_SIZE];
	_sntprintf_s(log_buff, LINE_BUFFER_SIZE, format, ap);

	/*TCHAR dbuffer[128],tbuffer[128],msg_buffer[2048];
	_tstrdate( dbuffer );
	_tstrtime( tbuffer );
	_stprintf(msg_buffer,_T("%s %s : %s\r\n"),dbuffer, tbuffer,log_buff);*/
	wstring test(&log_buff[0]); //convert to wstring
	string msg(test.begin(), test.end());


	//std::string msg = log_buff;

	WaitForSingleObject(mutex_,INFINITE);//Lock Mutex
	if( msg_queue_.size() < MAX_MSG_QUEUE)
	{
		msg_queue_.push(msg);
		if(msg_queue_.size()==1)
			SetEvent(sq_empty_);
	}
	ReleaseMutex(mutex_);//release Mutex
}

void RPCLogger::PrintLoop()
{
	OpenFile();
	while (1)//
	{
		WaitForSingleObject(mutex_,INFINITE);//Lock Mutex
		if(!msg_queue_.empty())
		{
			string msg = msg_queue_.front();
			msg_queue_.pop();
			ReleaseMutex(mutex_);//release Mutex
			printf(msg.c_str());
			DWORD byteswritten;
			try
			{
				std::fstream oldFile;
				oldFile.open(fullPath.c_str(), ios::out | ios::in);
				if(oldFile.is_open() == true)
				{
					int thresholdLength = (1*1024*1024);
					int newLength = (512*1024);
					oldFile.seekg( 0, ios::end );
					int currentlength = oldFile.tellg();
					if(currentlength > thresholdLength)
					{
						CloseHandle(hlogfile_);
						oldFile.seekg( -newLength, ios::end );
						char * buffer = new char [newLength];
						oldFile.read (buffer,newLength);
						oldFile.close(); 
						buffer[(newLength-1)] = _T('\0');
						string newLog(buffer);

						hlogfile_ = NULL;
						hlogfile_ = CreateFile(_T("E:\\abc.txt"),  GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL  );
						WriteFile(hlogfile_, newLog.c_str(), newLog.length(), &byteswritten, NULL); 
						delete[] buffer;
					}
				}
				else
				{
					//file Doesn't Exist
				}				
			}
			catch(...)
			{

			}

			
			WriteFile(hlogfile_, msg.c_str(), msg.length(), &byteswritten, NULL); 
		}
		else
		{
			ResetEvent(sq_empty_);
			ReleaseMutex(mutex_);//release Mutex
			WaitForSingleObject(sq_empty_,INFINITE); //wait when queue is empty
		}
	}
	CloseFile();
}

DWORD RPCLogger::EnterPrintThread(void* _this)
{
	RPCLogger *pt = (RPCLogger *)_this;
	pt->PrintLoop();
	return 0;
}

RPCLogger::~RPCLogger(void)
{
}
