#pragma once
#include <memory>
#include <string>
#include "../TrainManager/TrainManager.h"

// Base class for commands
class Command {
protected:
    int clientSocket; // Client socket that sent the command
    bool sendAll(int sock, const std::string& data);
public:
    Command(int socket) : clientSocket(socket) {}
    virtual void execute(TrainManager& tm) = 0;
    virtual ~Command() = default;
};

class GetScheduleCommand : public Command {
private:
    std::string fromCity;
    std::string toCity;
public:
    GetScheduleCommand(int socket, std::string from = "", std::string to = "");
    void execute(TrainManager& tm) override;
};

class GetDeparturesCommand : public Command {
    std::string station;
public:
    GetDeparturesCommand(int socket, std::string st = "") : Command(socket), station(st) {}
    void execute(TrainManager& tm) override;
};

class GetArrivalsCommand : public Command {
    std::string station;
public:
    GetArrivalsCommand(int socket, std::string st = "") : Command(socket), station(st) {}
    void execute(TrainManager& tm) override;
};

class ReportDelayCommand : public Command {
private:
    int trainID, delay;
    std::string estimate;

public:
    ReportDelayCommand(int socket, int id, int delay, std::string est);
    void execute(TrainManager& tm) override;
};

class GetTrainInfoCommand : public Command {
    int trainID;
public:
    GetTrainInfoCommand(int socket, int id) : Command(socket), trainID(id) {}
    void execute(TrainManager& tm) override;
};

class HelpCommand : public Command {
public:
    HelpCommand(int socket) : Command(socket) {}
    void execute(TrainManager& tm) override;
};