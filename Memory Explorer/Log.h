#pragma once

#include "stdafx.h"

#include "Typedefs.h"



void LogMessage(PCWSTR szMessage, BYTE byMessageType);
constexpr BYTE GenericNotificaton	= 1;
constexpr BYTE ErrorNotification	= 2;