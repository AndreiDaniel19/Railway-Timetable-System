#include <iostream>
#include <sstream>
#include <cstring>
#include <vector>
#include <cstdint>

#ifdef _WIN32
    // Windows
    #include <winsock2.h>
    #include <ws2tcpip.h>
    
    // Define aliases to keep the code similar to Linux
    #define close closesocket
    #define sleep(x) Sleep((x) * 1000) // Sleep receives milliseconds on Windows
#else
    // Linux / MacOS
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <unistd.h>
    #include <netinet/in.h>
#endif

using namespace std;

const int MAX_RETRIES = 5;
const int WAIT_SECONDS = 2;

string receiveAll(int sock) {
    uint32_t networkLen;
    
    int bytes = recv(sock, (char*)&networkLen, sizeof(networkLen), MSG_WAITALL);
    
    if (bytes <= 0) return ""; // Server closed or error

    // Convert from network format to host format
    uint32_t length = ntohl(networkLen); 

    // Allocate memory
    vector<char> buffer(length);
    size_t totalReceived = 0;

    // Read loop
    while (totalReceived < length) {
        int chunk = recv(sock, buffer.data() + totalReceived, length - totalReceived, 0);
        if (chunk <= 0) {
            return ""; // Error
        }
        totalReceived += chunk;
    }

    return string(buffer.begin(), buffer.end());
}

int main() {
    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            cerr << "WSAStartup failed.\n";
            return 1;
        }
    #endif

    int sock = 0;
    sockaddr_in serverAddr{};

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(54000);
    //You can change the IP if needed into your own server ip
    inet_pton(AF_INET,"127.0.0.1",&serverAddr.sin_addr);

    bool isConnected = false;

    for (int i = 0; i < MAX_RETRIES; ++i) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        
        // Check socket validity (cross-platform compatible)
        #ifdef _WIN32
        if (sock == INVALID_SOCKET)
        #else
        if (sock < 0)
        #endif
        {
            cerr << "Error creating socket.\n";
            return -1;
        }

        cout << "Connection attempt " << (i + 1) << "/" << MAX_RETRIES << "...\n";

        if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            close(sock); // On Windows this calls closesocket
            if (i < MAX_RETRIES - 1) {
                sleep(WAIT_SECONDS); // On Windows this calls Sleep(2000)
            }
        } else {
            isConnected = true;
            break; 
        }
    }

    if (!isConnected) {
        cout << "Server inactive! Connection could not be established.\n";
        
        // Winsock cleanup before exit
        #ifdef _WIN32
            WSACleanup();
        #endif
        return -1;
    }

    cout << "Connected to server!\n";

    string input;
    while(true) {
        cout << "Enter command: ";
        getline(cin, input);

        if(input == "exit") break;

        // Send command (cast to const char* for maximum compatibility)
        send(sock, input.c_str(), static_cast<int>(input.size()), 0);

        cout << "--- Response from server ---\n";
        string response = receiveAll(sock);
        
        if(response.empty()) {
            cout << "The server closed the connection.\n";
            break;
        }
        cout << response << endl;
    }

    close(sock);

    #ifdef _WIN32
        WSACleanup();
    #endif

    return 0;
}