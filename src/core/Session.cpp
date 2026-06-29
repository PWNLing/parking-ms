// core/Session.cpp
#include "Session.h"
#include <QDateTime>

SessionManager& SessionManager::instance() {
    static SessionManager inst;
    return inst;
}

void SessionManager::set(const Session& s) {
    session_ = s;
}

void SessionManager::clear() {
    session_.reset();
}
