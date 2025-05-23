#include "./httplib.h"

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>

#define BUFSIZE 4096

httplib::Server svr;

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;
//HANDLE g_hInputFile = NULL;

// Create a child process that uses the previously created pipes for STDIN and STDOUT.
void CreateChildProcess() {

    // Setup the process to run
    TCHAR szCmdline[]=TEXT("./crawl.exe");
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    BOOL bSuccess = FALSE;

    // Set up members of the PROCESS_INFORMATION structure.
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // Set up members of the STARTUPINFO structure.
    // This structure specifies the STDIN and STDOUT handles for redirection.
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = g_hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
    siStartInfo.hStdInput = g_hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process.
    bSuccess = CreateProcess(NULL,
        szCmdline,     // command line
        NULL,          // process security attributes
        NULL,          // primary thread security attributes
        TRUE,          // handles are inherited
        0,             // creation flags
        NULL,          // use parent's environment
        NULL,          // use parent's current directory
        &siStartInfo,  // STARTUPINFO pointer
        &piProcInfo);  // receives PROCESS_INFORMATION

    // If an error occurs, exit the application.
    if (!bSuccess) {
        std::cerr << "Error creating process" << std::endl;
        return;

    // Otherwise close the handles we won't use
    } else {

        // Close handles to the child process and its primary thread.
        // Some applications might keep these handles to monitor the status
        // of the child process, for example.
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);

        // Close handles to the stdin and stdout pipes no longer needed by the child process.
        // If they are not explicitly closed, there is no way to recognize that the child process has ended.
        CloseHandle(g_hChildStd_OUT_Wr);
        CloseHandle(g_hChildStd_IN_Rd);

    }

}

// Read from a file and write its contents to the pipe for the child's STDIN.
void WriteToPipe(std::string text) {

    // Write to the stdout
    std::cout << "started write" << std::endl;
    DWORD dwWritten;
    char chBuf[BUFSIZE];
    strncpy(chBuf, text.c_str(), sizeof(chBuf));
    chBuf[sizeof(chBuf) - 1] = 0;
    bool bSuccess = WriteFile(g_hChildStd_IN_Wr, chBuf, 1, &dwWritten, NULL);
    std::cout << bSuccess << std::endl;
    std::cout << "finished write" << std::endl;
    if (!bSuccess) {
        return;
    }

    // Close the pipe handle so the child process stops reading.
    if (!CloseHandle(g_hChildStd_IN_Wr)) {
        std::cerr << "Error closing handle" << std::endl;
    }

}

// Read output from the child process's pipe from STDOUT
std::string ReadFromPipe() {

    // Keep trying to read from the pipe
    std::cout << "started read" << std::endl;
    std::string ret;
    DWORD dwRead;
    CHAR chBuf[BUFSIZE]; 
    BOOL bSuccess = FALSE;
    for (int i=0; i<1000; i++) {

        // Check if we have data available to read
        DWORD bytesAvail = 0;
        if (!PeekNamedPipe(g_hChildStd_OUT_Rd, NULL, 0, NULL, &bytesAvail, NULL)) {
            break;
        }
        if (!bytesAvail) {
            break;
        }

        // If so, read the data
        bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
        if (!bSuccess || dwRead == 0) {
            break; 
        }

        // Add to the output string
        ret.append(chBuf, dwRead);

    }
    std::cout << "finished read" << std::endl;
    return ret;

}

// C++ entry point
int main(int argc, const char * argv[]) {

    // Set the bInheritHandle flag so pipe handles are inherited.
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDOUT.
    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0)) {
        std::cerr << "Error creating stdout pipe" << std::endl;
        return 1;
    }
    if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
        std::cerr << "Error ensuring stdout is not inherited" << std::endl;
        return 1;
    }

    // Create a pipe for the child process's STDIN.
    if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) {
        std::cerr << "Error creating stdin pipe" << std::endl;
        return 1;
    }
    if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
        std::cerr << "Error ensuring stdinis not inherited" << std::endl;
        return 1;
    }

    // Create the child process.
    CreateChildProcess();

    // For now, just read from our stdin
    std::string line;
    while (true) {
        std::getline(std::cin, line);
        if (line.size() > 0) {
            WriteToPipe(line);
        }
        std::string output = ReadFromPipe();
        std::cout << output << std::endl;
    }

    // Start the HTTP server
    //svr.Get("/hi", [](const httplib::Request &, httplib::Response &res) {
      //res.set_content("Hello World!", "text/plain");
    //});
    //std::cout << "Starting server on localhost:7777" << std::endl;
    //svr.listen("0.0.0.0", 7777);

    return 0;

}
