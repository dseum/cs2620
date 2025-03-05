#include "logic.hpp"

#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <ctime>
#include <future>
#include <string>

constexpr uint64_t DURATION_SECONDS = 60;
constexpr uint64_t MAX_CLOCK_RATE = 6;
constexpr int INTERNAL_EVENT_PROBABILITY_CAP = 10;

std::string getSystemTimeStr() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    char buf[64];
    strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&t));
    return std::string(buf);
}

bool sendMessage(int sockFd, const Message &msg) {
    ssize_t bytesSent = send(sockFd, &msg, sizeof(msg), 0);
    return (bytesSent == sizeof(msg));
}

EventType decideNextEvent(int random, int internalEventProbabilityCap) {
    if (random <= 1) {
        return EventType::SEND_TO_NEXT;
    } else if (random <= 2) {
        return EventType::SEND_TO_SECOND_NEXT;
    } else if (random <= 3) {
        return EventType::SEND_TO_ALL;
    } else if (random <= internalEventProbabilityCap) {
        return EventType::INTERNAL;
    }
    return EventType::NONE;
}

int calculateTargetRank(int sourceRank, int targetOffset, int numVms) {
    return (sourceRank + targetOffset) % numVms;
}

int calculateChannelIndex(int sourceRank, int targetRank) {
    return (targetRank > sourceRank) ? targetRank - 1 : targetRank;
}

void receiveMessages(Logger &logger, std::atomic<bool> &done, int rank,
                     const std::vector<int> &channels,
                     std::queue<Message> &messages,
                     std::mutex &messages_mutex) {
    std::ignore = logger;
    std::ignore = rank;

    std::vector<std::array<char, sizeof(Message)>> bufs(channels.size());
    std::vector<size_t> sizes(channels.size(), 0);

    std::vector<struct pollfd> fds(channels.size());
    for (int i = 0; i < static_cast<int>(channels.size()); i++) {
        fds[i].fd = channels[i];
        fds[i].events = POLLIN;
    }

    Message message;
    while (!done) {
        if (poll(fds.data(), channels.size(), 1000) > 0) {
            for (int i = 0; i < static_cast<int>(channels.size()); i++) {
                size_t &size = sizes[i];
                if (fds[i].revents & POLLIN) {
                    ssize_t bytes_read =
                        recv(channels[i], bufs[i].data() + size,
                             sizeof(Message) - size, 0);
                    if (bytes_read <= 0) {
                        throw std::runtime_error("Unexpected disconnect");
                    }
                    size += bytes_read;
                    if (size == sizeof(Message)) {
                        std::memcpy(&message, bufs[i].data(), sizeof(Message));
                        {
                            std::unique_lock<std::mutex> lock(messages_mutex);
                            messages.push(message);
                        }
                        size = 0;
                    }
                }
            }
        }
    }
}

void processReceivedMessage(Logger &logger, int rank, Message &msg,
                            LogicalClock &clock, size_t queueSize) {
    clock.updateOnReceive(msg.logicalTimestamp);
    logger.write(
        "[RECEIVE] SysTime={} Node={} From={} QueueLen={} LogicalClock={}",
        getSystemTimeStr(), rank, msg.senderRank, queueSize, clock.getValue());
}

void handleSendEvent(Logger &logger, int rank, EventType eventType,
                     LogicalClock &clock, const std::vector<int> &channels,
                     int numVms) {
    // Create message with updated logical clock
    Message outMsg;
    outMsg.senderRank = rank;

    // Increment logical clock for send event
    clock.increment();
    outMsg.logicalTimestamp = clock.getValue();

    switch (eventType) {
        case EventType::SEND_TO_NEXT: {
            int targetRank = calculateTargetRank(rank, 1, numVms);
            if (targetRank != rank) {  // Prevent self-send
                int channelIndex = calculateChannelIndex(rank, targetRank);
                if (sendMessage(channels[channelIndex], outMsg)) {
                    logger.write("[SEND to {}] SysTime={} LogicalClock={}",
                                 targetRank, getSystemTimeStr(),
                                 clock.getValue());
                }
            }
            break;
        }
        case EventType::SEND_TO_SECOND_NEXT: {
            int targetRank = calculateTargetRank(rank, 2, numVms);
            if (targetRank != rank) {  // Prevent self-send
                int channelIndex = calculateChannelIndex(rank, targetRank);
                if (sendMessage(channels[channelIndex], outMsg)) {
                    logger.write("[SEND to {}] SysTime={} LogicalClock={}",
                                 targetRank, getSystemTimeStr(),
                                 clock.getValue());
                }
            }
            break;
        }
        case EventType::SEND_TO_ALL: {
            for (int i = 0; i < static_cast<int>(channels.size()); i++) {
                int targetRank = (i >= rank) ? i + 1 : i;
                if (targetRank != rank) {
                    if (sendMessage(channels[i], outMsg)) {
                        logger.write("[SEND to {}] SysTime={} LogicalClock={}",
                                     targetRank, getSystemTimeStr(),
                                     clock.getValue());
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void runNode(int rank, Logger &logger, const std::vector<int> &channels,
             int num_vms) {
    srand(static_cast<unsigned>(time(NULL)) ^ rank);

    const int clockRate = rand() % MAX_CLOCK_RATE + 1;
    LogicalClock logicalClock(0);

    std::queue<Message> messageQueue;
    std::mutex messageQueueMutex;

    logger.write("[INFO]: Rank={} ClockRate={}", rank, clockRate);

    // Receives messages asynchronously
    std::atomic<bool> done = false;
    std::future<void> receiverThread =
        std::async(std::launch::async, receiveMessages, std::ref(logger),
                   std::ref(done), rank, std::ref(channels),
                   std::ref(messageQueue), std::ref(messageQueueMutex));

    auto now = std::chrono::high_resolution_clock::now();
    auto large_now = now;
    int large_count = 0;
    const auto end = now + std::chrono::seconds(DURATION_SECONDS);
    const auto increment =
        std::chrono::nanoseconds(static_cast<long long>(1e9 / clockRate));

    do {
        messageQueueMutex.lock();
        if (!messageQueue.empty()) {
            Message msg = messageQueue.front();
            messageQueue.pop();
            size_t queueSize =
                messageQueue.size();  // Get size before unlocking
            messageQueueMutex.unlock();

            processReceivedMessage(logger, rank, msg, logicalClock, queueSize);
        } else {
            messageQueueMutex.unlock();
            // No received messages: decide event and execute
            int randomEvent = rand() % 10 + 1;
            EventType nextEvent =
                decideNextEvent(randomEvent, INTERNAL_EVENT_PROBABILITY_CAP);

            if (nextEvent == EventType::SEND_TO_NEXT ||
                nextEvent == EventType::SEND_TO_SECOND_NEXT ||
                nextEvent == EventType::SEND_TO_ALL) {
                handleSendEvent(logger, rank, nextEvent, logicalClock, channels,
                                num_vms);
            } else if (nextEvent == EventType::INTERNAL) {
                logicalClock.increment();
                logger.write("[INTERNAL] SysTime={} Node={} LogicalClock={}",
                             getSystemTimeStr(), rank, logicalClock.getValue());
            }
        }

        large_count++;
        if (large_count == clockRate) {
            large_count = 0;
            large_now += std::chrono::seconds(1);
            now = large_now;
        } else {
            now += increment;
        }
        std::this_thread::sleep_until(now);
    } while (std::chrono::high_resolution_clock::now() < end);

    done = true;
    logger.write("[INFO]: Shutting down", rank);
    receiverThread.wait();
    logger.write("[INFO]: Shut down", rank);
}
