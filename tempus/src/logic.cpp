#include "logic.hpp"

#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <ctime>
#include <future>
#include <queue>
#include <string>

constexpr uint64_t DURATION_SECONDS = 20;
constexpr uint64_t MAX_CLOCK_RATE = 6;
constexpr int INTERNAL_EVENT_PROBABILITY_CAP =
    10;  // x in [3, 10] where probability is (x - 3) / 10

std::string getSystemTimeStr() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    char buf[64];
    strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&t));
    return std::string(buf);
}

void timespec_add(struct timespec *t, long sec, long nsec) {
    t->tv_sec += sec;
    t->tv_nsec += nsec;

    // Handle overflow: if nanoseconds exceed 1 billion
    while (t->tv_nsec >= 1000000000L) {
        t->tv_sec++;                // Carry over to seconds
        t->tv_nsec -= 1000000000L;  // Subtract 1 billion nanoseconds
    }
}

int timespec_cmp(struct timespec *t1, struct timespec *t2) {
    if (t1->tv_sec > t2->tv_sec) return 1;   // t1 is later
    if (t1->tv_sec < t2->tv_sec) return -1;  // t1 is earlier

    // If seconds are equal, compare nanoseconds
    if (t1->tv_nsec > t2->tv_nsec) return 1;   // t1 is later
    if (t1->tv_nsec < t2->tv_nsec) return -1;  // t1 is earlier

    return 0;  // Both times are equal
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

bool sendMessage(int sockFd, const Message &msg) {
    ssize_t bytesSent = send(sockFd, &msg, sizeof(msg), 0);
    return (bytesSent == sizeof(msg));
}

void runNode(int rank, Logger &logger, const std::vector<int> &channels) {
    srand(static_cast<unsigned>(time(NULL)) ^ rank);

    const int clockRate = rand() % MAX_CLOCK_RATE + 1;
    int logicalClock = 0;

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
            messageQueueMutex.unlock();
            logicalClock = std::max(logicalClock, msg.logicalTimestamp) + 1;
            logger.write(
                "[RECEIVE] SysTime={} Node={} From={} QueueLen={} "
                "LogicalClock={}",
                getSystemTimeStr(), rank, msg.senderRank, messageQueue.size(),
                logicalClock);
        } else {
            messageQueueMutex.unlock();
            // No received messages: decide event and execute
            int randomEvent = rand() % 10 + 1;
            if (randomEvent <= 3) {
                Message outMsg;
                outMsg.senderRank = rank;
                outMsg.logicalTimestamp = logicalClock;

                // Updates logical clock
                logicalClock++;

                // ensure 1 and 2 send message to opposite nodes from
                // eachother and 3 sends to both
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
                    for (int i = 0; i < static_cast<int>(channels.size());
                         i++) {
                        sendMessage(channels[i], outMsg);
                        int other_rank = i + (i >= rank);
                        logger.write("[SEND to {}] SysTime={} LogicalClock={}",
                                     other_rank, getSystemTimeStr(),
                                     logicalClock);
                    }
                }
            } else if (randomEvent <= INTERNAL_EVENT_PROBABILITY_CAP) {
                logicalClock++;
                logger.write("[INTERNAL] SysTime={} Node={} LogicalClock={}",
                             getSystemTimeStr(), rank, logicalClock);
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
