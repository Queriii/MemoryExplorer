#include "stdafx.h"

#include "Typedefs.h"



static constexpr BYTE GenericNotificaton	= 1;
static constexpr BYTE ErrorNotification		= 2;

void LogMessage(PCWSTR szMessage, BYTE byMessageType)
{
#if DBG
	PCWSTR	szMessageHeader			= nullptr;
	PWSTR	szFormattedMessage		= nullptr;
	size_t	ullFormattedMessageSize	= 1;				//Initially account for null terminator.

	do
	{
		if (!szMessage)
		{
			break;
		}
		ullFormattedMessageSize += wcslen(szMessage);
		if (ullFormattedMessageSize == 1)				//Check if szMessage is an empty string.
		{
			break;
		}

		bool bUnknownMessageType = false;
		switch (byMessageType)
		{

		case GenericNotificaton:
		{
			szMessageHeader = L"[INFO] | ";
			break;
		}

		case ErrorNotification:
		{
			szMessageHeader = L"[ERROR] | ";
			break;
		}

		default:
		{
			bUnknownMessageType = true;
			break;
		}

		}
		if (bUnknownMessageType)
		{
			break;
		}
		ullFormattedMessageSize += wcslen(szMessageHeader);

		szFormattedMessage = reinterpret_cast<PWSTR>(ExAllocatePool(NonPagedPool, ullFormattedMessageSize * sizeof(WCHAR)));
		if (!szFormattedMessage)
		{
			break;
		}

		__try
		{
			wcscpy_s(szFormattedMessage, ullFormattedMessageSize, szMessageHeader);
			wcscat_s(szFormattedMessage, ullFormattedMessageSize, szMessage);

			KdPrint(("%ws", szFormattedMessage));
		}
		__finally
		{
			ExFreePool(szFormattedMessage);
		}

		return;
	} while (false);
	
	KdPrint(("[ERROR] | Failed to log message, most likely due to lack of memory."));
#endif
}