
#include <cassert>
#include "timer.h"

namespace Utils
{

class TimeEntry
{
public :
    TimeEntry()
        : timeout(0),
          stepcount(0),
          endref(0),
          leftmsecs(0),
          prev(NULL),
          next(NULL),
          event(NULL)
    {}

public :
    void remove()
    {
        if ( prev == NULL
                || next == NULL )
        {
            return;
        }

        prev->next = next;
        next->prev = prev;
        prev = next = NULL;
    }

public :
    uint32_t        timeout;    // 超时时间
    uint32_t        stepcount;  // 步长
    uint32_t        endref;     // 过期索引
    uint32_t        leftmsecs;  // 剩余时间

    TimeEntry *     prev;
    TimeEntry *     next;

    ITimerEvent *   event;
};

bool ITimerEvent::isPaused() const
{
    if ( m_Entry == NULL )
    {
        return false;
    }

    return m_Entry->leftmsecs != 0;
}

bool ITimerEvent::isSchedule() const
{
    // 未设置过Entry
    if ( m_Entry == NULL )
    {
        return false;
    }

    return m_Entry->leftmsecs == 0;
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

TimeWheel::TimeWheel(uint32_t count, uint32_t percision)
    : m_DispatchRef(0),
      m_BucketCount(count),
      m_MaxPercision(percision),
      m_EventBuckets(NULL)
{
    assert( !( count&(count-1) ) && "count must be 2^n" );
}

TimeWheel::~TimeWheel()
{}

bool TimeWheel::init()
{
    m_EventBuckets = new TimeEntry [m_BucketCount];
    if ( m_EventBuckets == NULL )
    {
        return false;
    }

    m_DispatchRef = 0;

    for ( uint32_t i = 0; i < m_BucketCount; ++i )
    {
        TimeEntry * entry = m_EventBuckets+i;

        entry->prev = entry;
        entry->next = entry;
    }

    return true;
}

void TimeWheel::clear()
{
    if ( m_EventBuckets )
    {
        // 将管理器中的所有timer都销毁
        for ( uint32_t i = 0; i < m_BucketCount; ++i )
        {
            TimeEntry * head = m_EventBuckets+i;
            TimeEntry * entry = head->next;
            while ( entry != head )
            {
                TimeEntry * nextentry = entry->next;
                removeEntry( entry );
                entry = nextentry;
            }
        }

        delete [] m_EventBuckets;
        m_EventBuckets = NULL;
    }
}

bool TimeWheel::schedule( ITimerEvent * ev, uint32_t msecs )
{
    uint32_t timeout = 0;
    TimeEntry * entry = NULL;

    if ( ev->isSchedule() )
    {
        // 正在调度中...
        entry = ev->m_Entry;
        entry->remove();
        timeout = msecs == 0 ? entry->timeout : msecs;
    }
    else if ( ev->isPaused() )
    {
        // 正在暂停
        entry = ev->m_Entry;
        timeout = msecs == 0 ? entry->timeout : msecs;
        msecs = entry->leftmsecs;
    }
    else
    {
        // 创建entry
        entry = new TimeEntry();
        assert( entry != NULL && "create new Entry failed" );
        timeout = msecs;
    }

    // 最大时间精度
    msecs = msecs < m_MaxPercision ? m_MaxPercision : msecs;

    uint32_t index = getIndex( msecs );
    uint32_t stepcount = getStepCount( msecs );

    // 初始化entry
    entry->event        = ev;
    entry->timeout      = timeout;
    entry->stepcount    = stepcount;
    entry->leftmsecs    = 0;
    entry->endref       = m_DispatchRef + msecs/m_MaxPercision;
    // 初始化事件
    ev->m_Entry         = entry;

    add2Tailer( index, entry );
    return true;
}

bool TimeWheel::pause( ITimerEvent * ev )
{
    uint32_t nbuckets = 0;
    TimeEntry * entry = ev->m_Entry;

    if ( entry )
    {
        // 计算剩余时间
        nbuckets = entry->endref - m_DispatchRef;

        // NOTICE: nbuckets == 0
        // 意味着定时器已经到期, 鉴于pause()的操作性, 现将定时器挂起
        // 并且标志nbuckets = 1, schedule()的下一时间精度立即回调
        nbuckets = nbuckets == 0 ? 1 : nbuckets;

        // 移除Entry, 设置剩余时间
        entry->remove();
        entry->leftmsecs = nbuckets * m_MaxPercision;
    }

    return true;
}

bool TimeWheel::cancel( ITimerEvent * ev )
{
    TimeEntry * entry = ev->m_Entry;

    if ( entry )
    {
        removeEntry( entry, false );
    }

    return true;
}

int32_t TimeWheel::update()
{
    int32_t count = 0;
    uint32_t index = ( m_DispatchRef & (m_BucketCount-1) );

    TimeEntry * header = m_EventBuckets + index;
    TimeEntry * entry = header->next;
    while ( entry != header )
    {
        TimeEntry * nextentry = entry->next;

        --entry->stepcount;

        if ( entry->stepcount == 0 )
        {
            // 定时器时间到了
            uint32_t mseconds = 0;
            ITimerEvent * event = entry->event;

            ++count;

            if ( !event->onTimer( mseconds ) )
            {
                // 移出定时器
                removeEntry( entry );
            }
            else
            {
                // 继续添加定时器
                if ( mseconds != 0 )
                {
                    entry->timeout = mseconds;
                }

                uint32_t newindex = getIndex(entry->timeout);
                entry->stepcount = getStepCount(entry->timeout);
                if ( newindex != index )
                {
                    // 删除, 添加到新的slot中
                    entry->remove();
                    add2Tailer( newindex, entry );
                }
            }
        }

        entry = nextentry;
    }

    ++m_DispatchRef;

    return count ;
}

inline uint32_t TimeWheel::getIndex( uint32_t seconds )
{
    uint32_t index = m_DispatchRef;

    index += (seconds/m_MaxPercision);
    index &= ( m_BucketCount - 1 );

    return index;
}

inline uint32_t TimeWheel::getStepCount( uint32_t seconds )
{
    uint32_t steps = seconds/m_MaxPercision/m_BucketCount;

    if ( seconds % (m_MaxPercision*m_BucketCount) != 0 )
    {
        steps += 1;
    }

    return steps;
}

inline void TimeWheel::add2Tailer( uint32_t index, TimeEntry * entry )
{
    TimeEntry * header = m_EventBuckets+index;

    entry->next = header;
    entry->prev = header->prev;
    entry->prev->next = entry;
    entry->next->prev = entry;
}

inline void TimeWheel::removeEntry( TimeEntry * entry, bool notity )
{
    ITimerEvent * ev = entry->event;

    ev->m_Entry = NULL;
    entry->remove();
    delete entry;

    if ( notity )
    {
        ev->onEnd();
    }
}

}
