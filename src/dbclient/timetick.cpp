
#include "timetick.h"

TimeTick::TimeTick( uint32_t percision )
    : m_TimerManager( eTimeTick_BucketCount, percision )
{}

TimeTick::~TimeTick()
{}

bool TimeTick::start()
{
    if ( !m_TimerManager.init() )
    {
        stop();
        return false;
    }

    return true;
}

void TimeTick::stop()
{
    m_TimerManager.clear();
}

void TimeTick::run()
{
    m_TimerManager.update();
}

bool TimeTick::schedule( Utils::ITimerEvent * ev, uint32_t msecs )
{
    return m_TimerManager.schedule( ev, msecs );
}

bool TimeTick::cancel( Utils::ITimerEvent * ev )
{
    return m_TimerManager.cancel( ev );
}
