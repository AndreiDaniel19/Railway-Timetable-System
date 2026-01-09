#include <iostream>
#include <thread>
#include <sstream>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include "TrainManager/TrainManager.h"
#include "Commands/Commandqueue.h"

using namespace std;

CommandQueue commandQueue;
TrainManager trainManager;

bool sendToClient(int sock, const string& data) {
    uint32_t length = htonl(data.size()); 
    // Send Header (Length)
    if (send(sock, &length, sizeof(length), 0) != sizeof(length)) return false;

    // Send Body (Content)
    size_t total = 0;
    while(total < data.size()) {
        ssize_t sent = send(sock, data.c_str() + total, data.size() - total, 0);
        if(sent <= 0) return false;
        total += sent;
    }
    return true;
}

// This thread runs infinitely and processes commands from the queue
void processCommands() {
    cout << "[Worker] Thread started. Waiting for commands...\n";
    while (true) {
        // Pop command from queue (blocks here if queue is empty)
        auto cmd = commandQueue.pop();
        
        // Execute command
        if (cmd) {
            cmd->execute(trainManager);
        }
    }
}

// CLIENT THREAD
void handleClient(int clientSocket) {
    char buffer[256];
    while(true) {
        memset(buffer, 0, sizeof(buffer));
        // Wait for data from client (blocking)
        int bytes = recv(clientSocket, buffer, sizeof(buffer), 0);
        if(bytes <= 0) break;

        string request(buffer,bytes);
        // Clean newline at the end if it exists
        if (!request.empty() && request.back() == '\n') request.pop_back();
        if (!request.empty() && request.back() == '\r') request.pop_back();

        stringstream ss(request);
        string keyword;
        ss >> keyword;

        cout << "[Client " << clientSocket << "] Request: " << keyword << endl;

        // Interpret command and push to QUEUE
        if(keyword == "GET_SCHEDULE") {
            string city1, city2;
            ss >> city1 >> city2;
            commandQueue.push(make_unique<GetScheduleCommand>(clientSocket, city1, city2));
        }
        else if(keyword == "GET_DEPARTURES") {
            string station;
            // Read station if exists, otherwise send empty string
            if(ss >> station) {
                commandQueue.push(make_unique<GetDeparturesCommand>(clientSocket, station));
            } else {
                commandQueue.push(make_unique<GetDeparturesCommand>(clientSocket));
            }
        }
        else if(keyword == "GET_ARRIVALS") {
            string station;
            if(ss >> station) {
                commandQueue.push(make_unique<GetArrivalsCommand>(clientSocket, station));
            } else {
                commandQueue.push(make_unique<GetArrivalsCommand>(clientSocket));
            }
        }
        else if(keyword == "REPORT_DELAY") {
            int id, delay; string est;
            ss >> id >> delay >> est;
            commandQueue.push(make_unique<ReportDelayCommand>(clientSocket, id, delay, est));
        }
        else if(keyword=="GET_TRAIN_INFO"){
            int id;
            if(ss >> id) {
                commandQueue.push(make_unique<GetTrainInfoCommand>(clientSocket, id));
            } else {
                string err = "Error: Use GET_TRAIN_INFO <ID>\n";
                sendToClient(clientSocket, err);
            }
        }
        else if(keyword == "help") {
            commandQueue.push(make_unique<HelpCommand>(clientSocket));
        }
        else {
            string err="ERROR: Unknown command. Type 'help' for list.\n";
            sendToClient(clientSocket, err);
        }
    }

    close(clientSocket);
    cout << "[Server] Client disconnected: " << clientSocket << endl;
}

int main() {
    signal(SIGPIPE, SIG_IGN);
    // Load data (Now using the vector function)
    // Note: Folder names translated to English
    trainManager.loadDataFromXML("TrainSchedule/schedule_mod.xml", "TrainSchedule/schedule_org.xml");

    // Start Worker Thread that will consume commands
    thread worker(processCommands);
    worker.detach(); 

    // Network configuration
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(54000);
    addr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    if(bind(serverSocket, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        return 1;
    }
    
    listen(serverSocket, 5);
    cout << "[Server] Listening on port 54000...\n";

    // Accepting new clients
    while(true) {
        sockaddr_in cliAddr;
        socklen_t cliLen = sizeof(cliAddr);
        int client = accept(serverSocket, (sockaddr*)&cliAddr, &cliLen);
        
        if (client >= 0) {
            cout << "[Server] New client connected: " << client << endl;
            // For each client, start a dedicated reading thread
            thread(handleClient, client).detach();
        }
    }

    return 0;
}