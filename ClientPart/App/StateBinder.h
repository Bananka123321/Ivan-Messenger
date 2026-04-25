#pragma once

#include "AppState.h"
#include "Handler.h"

#include <QObject>

class StateChanger : public QObject {
    Q_OBJECT
public:
    StateChanger(Handler* handler_, AppState* state_);

private:
    Handler* handler;
    AppState* state;
};
