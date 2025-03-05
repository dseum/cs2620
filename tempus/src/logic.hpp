#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "logger.hpp"

struct Message {
    int senderRank;
    int logicalTimestamp;
};

std::string getSystemTimeStr();

class LogicalClock {
   public:
    LogicalClock(int initialValue = 0) : value(initialValue) {}

    int getValue() const { return value; }

    void setValue(int newValue) { value = newValue; }

    // Update logical clock based on received message
    void updateOnReceive(int receivedTimestamp) {
        value = std::max(value, receivedTimestamp) + 1;
    }

    // Update logical clock for internal or send event
    void increment() { value++; }

   private:
    int value;
};

// Network utilities
bool sendMessage(int sockFd, const Message &msg);

// Event types for testability
enum class EventType {
    RECEIVE,
    SEND_TO_NEXT,
    SEND_TO_SECOND_NEXT,
    SEND_TO_ALL,
    INTERNAL,
    NONE
};

// Event decision logic
EventType decideNextEvent(int random, int internalEventProbabilityCap);

// Target calculation
int calculateTargetRank(int sourceRank, int targetOffset, int numVms);

// Channel index calculation
int calculateChannelIndex(int sourceRank, int targetRank);

// Receiver function
void receiveMessages(Logger &logger, std::atomic<bool> &done, int rank,
                     const std::vector<int> &channels,
                     std::queue<Message> &messages, std::mutex &messages_mutex);

// Process a received message
void processReceivedMessage(Logger &logger, int rank, Message &msg,
                            LogicalClock &clock, size_t queueSize);

// Handle send event
void handleSendEvent(Logger &logger, int rank, EventType eventType,
                     LogicalClock &clock, const std::vector<int> &channels,
                     int numVms);

// Run the node's main loop
void runNode(int rank, Logger &logger, const std::vector<int> &channels,
             int num_vms);
