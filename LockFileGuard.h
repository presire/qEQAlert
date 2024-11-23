#ifndef LOCKFILEGUARD_H
#define LOCKFILEGUARD_H


#include <QLockFile>


class LockFileGuard
{
private:    // Variables
    QLockFile   &m_lockFile;

public:     // Variables

private:    // Methods
    LockFileGuard(const LockFileGuard&)             = delete;
    LockFileGuard& operator=(const LockFileGuard&)  = delete;

public:     // Methods
    explicit LockFileGuard(QLockFile &lockFile);
    ~LockFileGuard();
};


#endif // LOCKFILEGUARD_H
