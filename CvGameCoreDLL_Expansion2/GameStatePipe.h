#pragma once

#include <cstddef>
#include <string>

class CvGame;

// Simple helper that pushes small bits of game state to an external named pipe.
class GameStatePipe
{
public:
	GameStatePipe();
	~GameStatePipe();

	void Initialize();
	void Shutdown(const char* context = NULL);
	void SendTurnData(const CvGame& game);
	void SendTurnComplete(const CvGame& game);
	void SendSampleData();
	void LogState(const char* context, bool activeFlag) const;
	void PollCommands(CvGame& game);
	bool SendLine(const std::string& message);

private:
#if defined(_WIN32)
	void EnsureConnected();
	void Disconnect();
	bool WriteBytes(const char* buffer, size_t length);

	void* m_hPipe;
	unsigned int m_failedConnectAttempts;
	std::string m_pendingInput;
#endif
};
