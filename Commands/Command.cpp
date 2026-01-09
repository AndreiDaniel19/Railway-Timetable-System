#include "Command.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;

bool Command::sendAll(int sock, const std::string& data) {
    uint32_t length = htonl(data.size()); 
    if (send(sock, &length, sizeof(length), 0) != sizeof(length)) {
        perror("Failed to send message length");
        return false;
    }

    size_t total = 0;
    while (total < data.size()) {
        ssize_t sent = send(sock, data.c_str() + total, data.size() - total, 0);
        if (sent <= 0) {
            perror("Send failed");
            return false;
        }
        total += sent;
    }
    return true;
}
// The constructor receives the socket and filters
GetScheduleCommand::GetScheduleCommand(int socket, std::string from, std::string to) 
    : Command(socket), fromCity(from), toCity(to) {}

void GetScheduleCommand::execute(TrainManager& tm) {
    // The worker thread calls this.
    auto res = tm.getSchedule(fromCity, toCity);
    
    // Send response back to the client that generated the command
    sendAll(clientSocket,res);
}

void GetDeparturesCommand::execute(TrainManager& tm) {
    auto res = tm.getDeparturesNextHour(station);
    sendAll(clientSocket,res);
}

void GetArrivalsCommand::execute(TrainManager& tm) {
    auto res = tm.getArrivalsNextHour(station);
    sendAll(clientSocket,res);
}

ReportDelayCommand::ReportDelayCommand(int socket, int id, int d, string est)
    : Command(socket), trainID(id), delay(d), estimate(move(est)) {}

void ReportDelayCommand::execute(TrainManager& tm) {
    tm.updateDelay(trainID, delay, estimate);
    string msg = "OK: Delay updated!\n";
    sendAll(clientSocket,msg);
}

void GetTrainInfoCommand::execute(TrainManager& tm) {
    auto res = tm.getTrainDetails(trainID);
    sendAll(clientSocket,res);
}

// Help command implementation
void HelpCommand::execute(TrainManager& tm) {
    string helpMsg = 
        "=== CFR STATION - COMMAND LIST ===\n"
        "1. GET_SCHEDULE <Departure> <Arrival>\n"
        "   -> Search route (ex: GET_SCHEDULE Iasi Bucharest).\n"
        "2. GET_DEPARTURES <Optional: Station>\n"
        "   -> Departures in the next hour (ex: GET_DEPARTURES or GET_DEPARTURES Roman).\n"
        "3. GET_ARRIVALS <Optional: Station>\n"
        "   -> Arrivals in the next hour (ex: GET_ARRIVALS or GET_ARRIVALS Iasi).\n"
        "4. GET_TRAIN_INFO <ID>\n"
        "   -> Complete details about a train (ex: GET_TRAIN_INFO 1).\n"
        "5. REPORT_DELAY <ID> <Min> <Est>\n"
        "   -> Report delay.\n"
        "6. help / exit\n"
        "================================\n";

        sendAll(clientSocket,helpMsg);
}