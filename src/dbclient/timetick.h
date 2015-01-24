
#ifndef __SRC_DBCLIENT_TIMETICK_H__
#define __SRC_DBCLIENT_TIMETICK_H__

#include <time.h>

#include "base.h"
#include "define.h"
#include "utils/timer.h"

class TimeTick
{
public :
    TimeTick( uint32_t percision );
    ~TimeTick();

public :
    bool start();
    void run();
    void stop();

    bool schedule( Utils::ITimerEvent * ev, uint32_t msecs );
    bool cancel( Utils::ITimerEvent * ev );

    Utils::TimeWheel * getTimer() { return & m_TimerManager; }

private :
    enum
    {
        eTimeTick_BucketCount    = 1024,        // 桶个数
    };

    Utils::TimeWheel    m_TimerManager;
};

#endif
