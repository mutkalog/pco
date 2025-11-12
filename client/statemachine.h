#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <cstddef>


class StateMachine
{
public:
    enum State : size_t
    {
        IDLE,
        CHECKING,
        DOWNLOADING,
        VERIFYING,
        TESTING,
        COMMITING,
        TOTAL = 6
    };

public:
    static StateMachine& instance() {
        static StateMachine sm;
        return sm;
    }

    enum State state()          { return m_state; }
    void setState(enum State s) { m_state = s; }

    void proceed() {
        m_state = static_cast<enum State>(m_state + 1);
        m_state = (m_state == TOTAL) ? IDLE : m_state;
    }

    void reset() { m_state = IDLE; }

private:
    StateMachine();
    enum State m_state;

};

#endif // STATEMACHINE_H
