#include "DbgEngine.h"
#include "../dbgUi/dbgUi.h"

DbgEngine::DbgEngine()
{
}

DbgEngine::~DbgEngine()
{
}

E_Status DbgEngine::Exec()
{
	static bool bIsSystemBreakpoint = true;
	DEBUG_EVENT dbgEvent = { 0 };
	DWORD dwStatus = 0;
	BOOL bRet = 0;

	bRet = WaitForDebugEvent(&dbgEvent , 30);
	if(bRet == FALSE)
		return e_s_sucess;

	m_pid = dbgEvent.dwProcessId;
	m_tid = dbgEvent.dwThreadId;

	// �رվɵľ��
	CloseHandle(m_hCurrProcess);
	CloseHandle(m_hCurrThread);
	m_hCurrThread = m_hCurrProcess = 0;
	// ��ȡ�����쳣ʹ�õ��ľ��
	m_hCurrProcess = OpenProcess(0xFFFF ,
								 FALSE ,
								 m_pid
								 );
	m_hCurrThread = OpenThread(0xFFFF ,
							   FALSE ,
							   m_tid
							   );
	dwStatus = DBG_CONTINUE;
	// ɸѡ�쳣
	switch(dbgEvent.dwDebugEventCode)
	{
		case EXCEPTION_DEBUG_EVENT:
		{
			if(bIsSystemBreakpoint)
			{
				// ��ʼ�����ŷ�����
				BreakpointEngine::InitSymbol(m_hCurrProcess);

				bIsSystemBreakpoint = false;
				// ��OEP���¶�
				AddBreakPoint(m_oep , e_bt_soft)->SetCondition(true);
				if(m_bStopOnSystemBreakpoint)
				{
					//���ô����ϵ�Ļص�����
					if(m_pfnBreakpointProc)
						m_pfnBreakpointProc(this);
				}
				goto _SUCESS;
			}

			// ���²���Ϊ����������ʧЧ�Ķϵ�
			ReInstallBreakpoint();

			// �����쳣��Ϣ���Ҷϵ�
			BpItr itr = FindBreakpoint(dbgEvent.u.Exception);
			// �жϵ������Ƿ���Ч, �����Ч,˵��û�ж�Ӧ�Ķϵ�
			if(IsInvalidIterator(itr))
				dwStatus = m_pfnOtherException?m_pfnOtherException(dbgEvent.u.Exception):DBG_EXCEPTION_HANDLED;
			else
			{
				// ȡ���ϵ�(��ͬ�Ķϵ��в�ͬ�ķ�ʽ).
				// ����ܹ��ɹ������ϵ�, ������û��Ĵ�������
				if(true == FixBreakpoint(itr))
				{
					if(m_pfnBreakpointProc)
						m_pfnBreakpointProc(this);
				}
			}
			break;
		}
		case CREATE_PROCESS_DEBUG_EVENT:
			// ����oep�ͼ��ػ�ַ
			m_oep = (uaddr)dbgEvent.u.CreateProcessInfo.lpStartAddress;
			m_imgBase = (uaddr)dbgEvent.u.CreateProcessInfo.lpBaseOfImage;
			m_hCurrProcess = dbgEvent.u.CreateProcessInfo.hProcess;

			AddProcess(dbgEvent.u.CreateProcessInfo.hProcess ,
					   dbgEvent.u.CreateProcessInfo.hThread);
			break;
		case CREATE_THREAD_DEBUG_EVENT:
			//AddThread(dbgEvent.u.CreateThread.hThread);
			break;
		case EXIT_PROCESS_DEBUG_EVENT:
			bIsSystemBreakpoint = true;
			return e_s_processQuit;
			break;
		case EXIT_THREAD_DEBUG_EVENT:
		{
			int n = 0;
		}
			break;
		case LOAD_DLL_DEBUG_EVENT:
			break;
		case UNLOAD_DLL_DEBUG_EVENT:
			break;
		case OUTPUT_DEBUG_STRING_EVENT:
			break;
	}


_SUCESS:
	ContinueDebugEvent(dbgEvent.dwProcessId ,
					   dbgEvent.dwThreadId ,
					   dwStatus);

	return e_s_sucess;
}

void DbgEngine::Close()
{
	DbgObject::Close();
	BreakpointEngine::Clear();
}