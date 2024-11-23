#include "LockFileGuard.h"


LockFileGuard::LockFileGuard(QLockFile& lockFile) : m_lockFile(lockFile)
{
}


LockFileGuard::~LockFileGuard()
{
    if (m_lockFile.isLocked()) {
        m_lockFile.unlock();
    }

    m_lockFile.removeStaleLockFile();
}
