#pragma once

class Wake {
public:
    Wake();
    ~Wake();

    Wake(const Wake&) = delete;
    Wake& operator=(const Wake&) = delete;

    int readFd() const;
    int writeFd() const;
    bool notify() const;
    void consume();

private:
    int cReadFd{-1};
    int cWriteFd{-1};
};
