#include "logic.hpp"

#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <ctime>
#include <format>
#include <fstream>
#include <future>
#include <iostream>
#include <queue>
#include <string>

constexpr int DURATION_SECONDS = 10;

std::string getSystemTimeStr() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    char buf[64];
    strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&t));
    return std::string(buf);
}

void receiveMessages(int rank, const std::vector<int> &channels,
                     std::queue<Message> &messageQueue,
                     std::mutex &messageQueueMutex) {
    while (true) {
        Message msg;
        for (int i = 0; i < static_cast<int>(channels.size()); i++) {
            if (i == rank) continue;  // Skips self

            int sockFd = channels[i];
            ssize_t bytesReceived = recv(sockFd, &msg, sizeof(msg), 0);
            if (bytesReceived == sizeof(msg)) {
                std::unique_lock<std::mutex> lock(messageQueueMutex);
                messageQueue.push(msg);
            }
        }
    }
}

bool sendMessage(int sockFd, const Message &msg) {
    ssize_t bytesSent = send(sockFd, &msg, sizeof(msg), 0);
    return (bytesSent == sizeof(msg));
}

void runNode(int rank, std::ofstream &logFile,
             const std::vector<int> &socketFds) {
    srand(static_cast<unsigned>(time(NULL)) ^ rank);

    int clockRate = rand() % 6 + 1;
    int logicalClock = 0;

    // Start a 60 second iteration
    auto startTime = std::chrono::steady_clock::now();

    // This value controls how many “ticks” we’ve done in the current second
    int ticksThisSecond = 0;
    auto secondStart = std::chrono::steady_clock::now();

    std::queue<Message> messageQueue;
    std::mutex messageQueueMutex;

    // Receives messages asynchronously
    std::future<void> receiverThread = std::async(
        std::launch::async, receiveMessages, rank, std::ref(socketFds),
        std::ref(messageQueue), std::ref(messageQueueMutex));

    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsedSecs =
            std::chrono::duration_cast<std::chrono::seconds>(now - startTime)
                .count();

        // Break after set duration
        if (elapsedSecs >= DURATION_SECONDS) {
            break;
        }

        // Check if we need to reset the tick counter for a new second
        auto msElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             now - secondStart)
                             .count();
        if (msElapsed >= 1000) {
            // Reset counters for the new real second
            secondStart = std::chrono::steady_clock::now();
            ticksThisSecond = 0;
        }

        // Check if we need to tick
        if (ticksThisSecond >= clockRate) {
            usleep(1000);
            continue;
        }

        // tick logical clock
        ticksThisSecond++;

        if (!messageQueue.empty()) {
            Message msg = messageQueue.front();
            messageQueue.pop();
            logicalClock = std::max(logicalClock, msg.logicalTimestamp) + 1;
            logFile << "[RECEIVE] SysTime=" << getSystemTimeStr()
                    << " Node=" << rank << " From=" << msg.senderRank
                    << " QueueLen=" << messageQueue.size()
                    << " LogicalClock=" << logicalClock << "\n";
        } else {
            int randomEvent = rand() % 10 + 1;
            if (randomEvent >= 1 && randomEvent <= 3) {
                Message outMsg;
                outMsg.senderRank = rank;
                outMsg.logicalTimestamp = logicalClock;
                logicalClock++;

                // ensure 1 and 2 send message to opposite nodes from eachother
                // and 3 sends to both
                if (randomEvent == 1) {
                    int nodeToSend = (rank + 1) % socketFds.size();
                    if (nodeToSend == rank) {
                        nodeToSend = (rank + 2) % socketFds.size();
                    }
                    sendMessage(socketFds[nodeToSend], outMsg);
                    logFile << "[SEND to " << nodeToSend << "] "
                            << "SysTime=" << getSystemTimeStr()
                            << " LogicalClock=" << logicalClock << "\n";
                } else if (randomEvent == 2) {
                    int nodeToSend = (rank + 2) % socketFds.size();
                    if (nodeToSend == rank) {
                        nodeToSend = (rank + 1) % socketFds.size();
                    }
                    sendMessage(socketFds[nodeToSend], outMsg);
                    logFile << "[SEND to " << nodeToSend << "] "
                            << "SysTime=" << getSystemTimeStr()
                            << " LogicalClock=" << logicalClock << "\n";
                } else if (randomEvent == 3) {
                    for (int i = 0; i < (int)socketFds.size(); i++) {
                        if (i == rank) continue;  // skip self
                        sendMessage(socketFds[i], outMsg);
                        logFile << "[SEND to " << i << "] "
                                << "SysTime=" << getSystemTimeStr()
                                << " LogicalClock=" << logicalClock << "\n";
                    }
                }
            } else {
                logicalClock++;
                logFile << "[INTERNAL] SysTime=" << getSystemTimeStr()
                        << " Node=" << rank << " LogicalClock=" << logicalClock
                        << "\n";
            }
        }
    }

    logFile << "Node " << rank << " exiting.\n";
    logFile.close();
}
