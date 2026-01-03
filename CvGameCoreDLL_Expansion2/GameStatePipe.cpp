#include "CvGameCoreDLLPCH.h"
#include "GameStatePipe.h"

#include "CvGame.h"

#include <cstdarg>
#include <sstream>

#if defined(_WIN32)
#ifndef CSIDL_PERSONAL
#define CSIDL_PERSONAL 0x0005
#endif
#ifndef CSIDL_FLAG_CREATE
#define CSIDL_FLAG_CREATE 0x8000
#endif
#ifndef SHGFP_TYPE_CURRENT
#define SHGFP_TYPE_CURRENT 0
#endif
extern "C" __declspec(dllimport) HRESULT __stdcall SHGetFolderPathA(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath);
extern "C" __declspec(dllimport) int __stdcall SHCreateDirectoryExA(HWND hwnd, LPCSTR pszPath, const SECURITY_ATTRIBUTES* psd);
#endif

namespace
{
#if defined(_WIN32)
	const char* GAME_STATE_PIPE_NAME = "\\\\.\\pipe\\civv_llm";
	const char* GAME_STATE_PIPE_LOG_DIR = "\\My Games\\Sid Meier's Civilization 5\\Logs\\LLMCiv";
	const char* GAME_STATE_PIPE_LOG_FILE = "\\game_state_pipe.log";

	std::string BuildLogPath()
	{
		char docsPath[MAX_PATH] = {0};
		if (FAILED(SHGetFolderPathA(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, docsPath)))
		{
			return std::string();
		}

		std::string directory = std::string(docsPath) + GAME_STATE_PIPE_LOG_DIR;
		SHCreateDirectoryExA(NULL, directory.c_str(), NULL);
		return directory + GAME_STATE_PIPE_LOG_FILE;
	}

	void LogMessage(const char* format, ...)
	{
		static std::string s_logPath = BuildLogPath();
		static bool s_reportedMissingLogPath = false;
		if (s_logPath.empty())
		{
			if (!s_reportedMissingLogPath)
			{
				OutputDebugStringA("GameStatePipe: unable to determine log path\r\n");
				s_reportedMissingLogPath = true;
			}
			return;
		}

		char messageBuffer[512];
		va_list args;
		va_start(args, format);
		_vsnprintf_s(messageBuffer, _TRUNCATE, format, args);
		va_end(args);

		SYSTEMTIME systemTime;
		GetLocalTime(&systemTime);
		char timestamp[64];
		_snprintf_s(timestamp, _TRUNCATE, "%04u-%02u-%02u %02u:%02u:%02u.%03u ",
		            systemTime.wYear,
		            systemTime.wMonth,
		            systemTime.wDay,
		            systemTime.wHour,
		            systemTime.wMinute,
		            systemTime.wSecond,
		            systemTime.wMilliseconds);

		std::string finalMessage = std::string(timestamp) + messageBuffer + "\r\n";

		HANDLE fileHandle = CreateFileA(s_logPath.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fileHandle == INVALID_HANDLE_VALUE)
		{
			return;
		}

		const DWORD messageLen = static_cast<DWORD>(finalMessage.size());
		DWORD bytesWritten = 0;
		WriteFile(fileHandle, finalMessage.c_str(), messageLen, &bytesWritten, NULL);

		CloseHandle(fileHandle);
	}
#endif
}

namespace
{
#if defined(_WIN32)
	void TrimLine(std::string& line)
	{
		while (!line.empty())
		{
			size_t lastPos = line.size() - 1;
			char ch = line[lastPos];
			if (ch == '\r' || ch == '\n')
			{
				line.erase(lastPos);
			}
			else
			{
				break;
			}
		}
	}

	std::string JsonEscape(const std::string& value)
	{
		std::string escaped;
		escaped.reserve(value.size());
		for (size_t i = 0; i < value.size(); ++i)
		{
			const char ch = value[i];
			switch (ch)
			{
			case '\\':
			case '\"':
				escaped.push_back('\\');
				escaped.push_back(ch);
				break;
			case '\n':
				escaped.append("\\n");
				break;
			case '\r':
				escaped.append("\\r");
				break;
			case '\t':
				escaped.append("\\t");
				break;
			default:
				escaped.push_back(ch);
				break;
			}
		}
		return escaped;
	}
#endif
}

GameStatePipe::GameStatePipe()
{
#if defined(_WIN32)
	m_hPipe = INVALID_HANDLE_VALUE;
	m_failedConnectAttempts = 0;
#endif
}

GameStatePipe::~GameStatePipe()
{
	Shutdown();
}

void GameStatePipe::Initialize()
{
#if defined(_WIN32)
	LogMessage("GameStatePipe: Initialize requested (handle=%p, failedConnectAttempts=%u)", m_hPipe, m_failedConnectAttempts);
	m_failedConnectAttempts = 0;
	EnsureConnected();
	LogState("GameStatePipe::Initialize (post)", m_hPipe != INVALID_HANDLE_VALUE);
	LogMessage("GameStatePipe initialized");
#endif
}

void GameStatePipe::Shutdown(const char* context)
{
#if defined(_WIN32)
	const char* safeContext = (context != NULL && context[0] != '\0') ? context : "GameStatePipe::Shutdown";
	LogMessage("GameStatePipe shutting down (context=%s, handle=%p)", safeContext, m_hPipe);
	Disconnect();
	LogState("GameStatePipe::Shutdown (post)", m_hPipe != INVALID_HANDLE_VALUE);
#endif
}

void GameStatePipe::SendSampleData()
{
#if defined(_WIN32)
	EnsureConnected();
	if (m_hPipe == INVALID_HANDLE_VALUE)
	{
		LogMessage("GameStatePipe: Sample Failed");
		return;
	}

	std::ostringstream payload;
	payload << "{ \"Sample\": \"Foo\"}\n";

	const std::string data = payload.str();
	LogMessage("GameStatePipe: sending sample payload (%zu bytes): %s", data.size(), data.c_str());
	if (!WriteBytes(data.c_str(), data.size()))
	{
		LogMessage("GameStatePipe: write failed Sample");
	}
	else
	{
		LogMessage("GameStatePipe: sample payload sent (%zu bytes)", data.size());
	}
#else
	UNUSED_VARIABLE(this);
#endif
}

void GameStatePipe::SendTurnData(const CvGame& game)
{
#if defined(_WIN32)
	EnsureConnected();
	if (m_hPipe == INVALID_HANDLE_VALUE)
	{
		LogMessage("GameStatePipe: no connection; skipping turn %d payload", game.getGameTurn());
		return;
	}

	std::ostringstream payload;

	// Send turn_start message according to protocol
	payload << "{\"type\":\"turn_start\"";
	payload << ",\"player_id\":0";  // TODO: Determine actual LLM player ID
	payload << ",\"turn\":" << game.getGameTurn();
	payload << ",\"state\":{";
	payload << "\"turn\":" << game.getGameTurn();
	payload << ",\"playersAlive\":" << game.countCivPlayersAlive();
	payload << ",\"civsEver\":" << game.countCivPlayersEverAlive();
	// TODO: Add more state fields (cities, units, tech, etc.)
	payload << "}}\n";

	const std::string data = payload.str();
	LogMessage("GameStatePipe: sending turn_start for turn %d (%zu bytes)", game.getGameTurn(), data.size());
	if (!WriteBytes(data.c_str(), data.size()))
	{
		LogMessage("GameStatePipe: write failed for turn %d", game.getGameTurn());
	}
	else
	{
		LogMessage("GameStatePipe: successfully sent turn %d (%zu bytes)", game.getGameTurn(), data.size());
	}
#else
	UNUSED_VARIABLE(game);
#endif
}

void GameStatePipe::SendTurnComplete(const CvGame& game)
{
#if defined(_WIN32)
	EnsureConnected();
	if (m_hPipe == INVALID_HANDLE_VALUE)
	{
		LogMessage("GameStatePipe: no connection; skipping turn complete for turn %d", game.getGameTurn());
		return;
	}

	PlayerTypes activePlayer = game.getActivePlayer();
	const CvPlayer& kActivePlayer = GET_PLAYER(activePlayer);

	std::ostringstream payload;

	// Send turn_complete message
	payload << "{\"type\":\"turn_complete\"";
	payload << ",\"turn\":" << game.getGameTurn();
	payload << ",\"player_id\":" << activePlayer;
	if (activePlayer != NO_PLAYER)
	{
		std::string playerName = kActivePlayer.getName();
		payload << ",\"player_name\":\"" << JsonEscape(playerName) << "\"";
		payload << ",\"is_human\":" << (kActivePlayer.isHuman() ? "true" : "false");
	}
	payload << ",\"state\":{";
	payload << "\"turn\":" << game.getGameTurn();
	payload << ",\"playersAlive\":" << game.countCivPlayersAlive();
	payload << ",\"civsEver\":" << game.countCivPlayersEverAlive();
	// TODO: Add more state fields (cities, units, tech, etc.)
	payload << "}}\n";

	const std::string data = payload.str();
	LogMessage("GameStatePipe: sending turn_complete for turn %d, player %d (%zu bytes)", game.getGameTurn(), activePlayer, data.size());
	if (!WriteBytes(data.c_str(), data.size()))
	{
		LogMessage("GameStatePipe: write failed for turn_complete turn %d", game.getGameTurn());
	}
	else
	{
		LogMessage("GameStatePipe: successfully sent turn_complete turn %d (%zu bytes)", game.getGameTurn(), data.size());
	}
#else
	UNUSED_VARIABLE(game);
#endif
}

void GameStatePipe::LogState(const char* context, bool activeFlag) const
{
#if defined(_WIN32)
	const char* safeContext = (context != NULL && context[0] != '\0') ? context : "GameStatePipe::LogState";
	LogMessage("GameStatePipe: %s (activeFlag=%d, handle=%p, failedConnectAttempts=%u)",
	           safeContext,
	           activeFlag ? 1 : 0,
	           m_hPipe,
	           m_failedConnectAttempts);
#else
	UNUSED_VARIABLE(context);
	UNUSED_VARIABLE(activeFlag);
#endif
}

#if defined(_WIN32)

void GameStatePipe::EnsureConnected()
{
	if (m_hPipe != INVALID_HANDLE_VALUE)
	{
		return;
	}

	if (m_failedConnectAttempts == 0)
	{
		LogMessage("GameStatePipe: attempting to open named pipe for the first time");
	}

	HANDLE pipeHandle = CreateFileA(GAME_STATE_PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (pipeHandle == INVALID_HANDLE_VALUE)
	{
		const DWORD lastError = GetLastError();
		if (lastError == ERROR_FILE_NOT_FOUND || lastError == ERROR_PIPE_BUSY)
		{
			// Non-fatal: server not present yet. Try again on the next call.
			if ((m_failedConnectAttempts % 60) == 0)
			{
				LogMessage("GameStatePipe: pipe unavailable (error=%lu). Attempt=%u", lastError, m_failedConnectAttempts + 1);
			}
		}
		else
		{
			LogMessage("GameStatePipe: failed to open named pipe. error=%lu", lastError);
		}

		if (m_failedConnectAttempts < UINT_MAX)
		{
			++m_failedConnectAttempts;
		}

		return;
	}

	DWORD mode = PIPE_READMODE_MESSAGE;
	SetNamedPipeHandleState(pipeHandle, &mode, NULL, NULL);

	m_hPipe = pipeHandle;
	LogMessage("GameStatePipe: connected");
}

void GameStatePipe::Disconnect()
{
	if (m_hPipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(static_cast<HANDLE>(m_hPipe));
		m_hPipe = INVALID_HANDLE_VALUE;
		LogMessage("GameStatePipe: disconnected");
	}
}

bool GameStatePipe::WriteBytes(const char* buffer, size_t length)
{
	if (m_hPipe == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD bytesWritten = 0;
	const BOOL result = WriteFile(static_cast<HANDLE>(m_hPipe), buffer, static_cast<DWORD>(length), &bytesWritten, NULL);
	if (!result || bytesWritten != length)
	{
		LogMessage("GameStatePipe: write failed (lastError=%lu, wrote=%lu/%zu)", GetLastError(), bytesWritten, length);
		Disconnect();
		return false;
	}

	return true;
}

bool GameStatePipe::SendLine(const std::string& message)
{
#if defined(_WIN32)
	if (message.empty())
	{
		return false;
	}

	std::string payload = message;
	if (payload.empty() || payload[payload.size() - 1] != '\n')
	{
		payload.push_back('\n');
	}

	if (!WriteBytes(payload.c_str(), payload.size()))
	{
		LogMessage("GameStatePipe: failed to send response bytes");
		return false;
	}

	return true;
#else
	UNUSED_VARIABLE(message);
	return false;
#endif
}

void GameStatePipe::PollCommands(CvGame& game)
{
#if defined(_WIN32)
	EnsureConnected();
	if (m_hPipe == INVALID_HANDLE_VALUE)
	{
		return;
	}

	// Check if we already have a complete command in the buffer
	size_t newlinePos = m_pendingInput.find('\n');
	if (newlinePos != std::string::npos)
	{
		// Process the first complete command in the buffer
		std::string command = m_pendingInput.substr(0, newlinePos);
		m_pendingInput.erase(0, newlinePos + 1);
		TrimLine(command);
		if (!command.empty())
		{
			LogMessage("GameStatePipe: received command '%s'", command.c_str());
			game.HandlePipeCommand(command);
		}
		// Return immediately after processing one command
		// The next call to PollCommands will process the next command
		return;
	}

	// No complete command in buffer, check for new data
	DWORD bytesAvailable = 0;
	if (!PeekNamedPipe(static_cast<HANDLE>(m_hPipe), NULL, 0, NULL, &bytesAvailable, NULL))
	{
		const DWORD lastError = GetLastError();
		LogMessage("GameStatePipe: PeekNamedPipe failed (error=%lu)", lastError);
		Disconnect();
		return;
	}

	if (bytesAvailable == 0)
	{
		return;
	}

	// Read only enough bytes to potentially complete a command
	// We'll read in chunks but stop as soon as we get a newline
	char readBuffer[256];
	const DWORD maxChunk = static_cast<DWORD>(sizeof(readBuffer) - 1);
	const DWORD bytesToRead = (bytesAvailable < maxChunk) ? bytesAvailable : maxChunk;
	DWORD bytesRead = 0;
	if (!ReadFile(static_cast<HANDLE>(m_hPipe), readBuffer, bytesToRead, &bytesRead, NULL) || bytesRead == 0)
	{
		const DWORD lastError = GetLastError();
		LogMessage("GameStatePipe: read failed for command stream (error=%lu)", lastError);
		Disconnect();
		return;
	}

	readBuffer[bytesRead] = '\0';
	m_pendingInput.append(readBuffer, bytesRead);

	// Check again for a complete command after reading new data
	newlinePos = m_pendingInput.find('\n');
	if (newlinePos != std::string::npos)
	{
		// Process the first complete command
		std::string command = m_pendingInput.substr(0, newlinePos);
		m_pendingInput.erase(0, newlinePos + 1);
		TrimLine(command);
		if (!command.empty())
		{
			LogMessage("GameStatePipe: received command '%s'", command.c_str());
			game.HandlePipeCommand(command);
		}
		// Return immediately after processing one command
		// Any remaining data stays in m_pendingInput for the next call
	}
#else
	UNUSED_VARIABLE(game);
#endif
}

#endif // defined(_WIN32)
