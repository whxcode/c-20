#pragma once

class Wake {
public:
    Wake();
    ~Wake();

    Wake(const Wake&) = delete;
    Wake& operator=(const Wake&) = delete;

    int fd() const;
    bool notify() const;
    void consume();

private:
    int cFd{-1};
};
