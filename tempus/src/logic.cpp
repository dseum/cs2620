#include "logic.hpp"

#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <ctime>
#include <future>
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

void receiveMessages(Logger &logger, std::atomic<bool> &done, int rank,
                     const std::vector<int> &channels,
                     std::queue<Message> &messages,
                     std::mutex &messages_mutex) {
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

bool sendMessage(int sockFd, const Message &msg) {
    ssize_t bytesSent = send(sockFd, &msg, sizeof(msg), 0);
    return (bytesSent == sizeof(msg));
}

void runNode(int rank, Logger &logger, const std::vector<int> &channels) {
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

    logger.write("[INFO]: Rank={} ClockRate={}", rank, clockRate);

    // Receives messages asynchronously
    std::atomic<bool> done = false;
    std::future<void> receiverThread =
        std::async(std::launch::async, receiveMessages, std::ref(logger),
                   std::ref(done), rank, std::ref(channels),
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

        {
            std::unique_lock<std::mutex> lock(messageQueueMutex);
            if (!messageQueue.empty()) {
                Message msg = messageQueue.front();
                messageQueue.pop();
                logicalClock = std::max(logicalClock, msg.logicalTimestamp) + 1;
                logger.write(
                    "[RECEIVE] SysTime={} Node={} From={} QueueLen={} "
                    "LogicalClock={}",
                    getSystemTimeStr(), rank, msg.senderRank,
                    messageQueue.size(), logicalClock);
                continue;
            }
        }

        int randomEvent = rand() % 10 + 1;
        if (randomEvent >= 1 && randomEvent <= 3) {
            Message outMsg;
            outMsg.senderRank = rank;
            outMsg.logicalTimestamp = logicalClock;
            logicalClock++;

            // ensure 1 and 2 send message to opposite nodes from eachother
            // and 3 sends to both
            if (randomEvent == 1) {
                int nodeToSend = (rank) % channels.size();
                sendMessage(channels[nodeToSend], outMsg);
                logger.write("[SEND to {}] SysTime={} LogicalClock={}",
                             nodeToSend, getSystemTimeStr(), logicalClock);
            } else if (randomEvent == 2) {
                int nodeToSend = (rank + 1) % channels.size();
                sendMessage(channels[nodeToSend], outMsg);
                logger.write("[SEND to {}] SysTime={} LogicalClock={}",
                             nodeToSend, getSystemTimeStr(), logicalClock);
            } else if (randomEvent == 3) {
                for (int i = 0; i < static_cast<int>(channels.size()); i++) {
                    sendMessage(channels[i], outMsg);
                    int other_rank = i < rank ? i : i + 1;
                    logger.write("[SEND to {}] SysTime={} LogicalClock={}",
                                 other_rank, getSystemTimeStr(), logicalClock);
                }
            }
        } else {
            logicalClock++;
            logger.write("[INTERNAL] SysTime={} Node={} LogicalClock={}",
                         getSystemTimeStr(), rank, logicalClock);
        }
    }

    done = true;
    logger.write("[INFO]: Shutting down", rank);
    receiverThread.wait();
    logger.write("[INFO]: Shut down", rank);
}
