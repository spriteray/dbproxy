
#ifndef __SRC_UTILS_TIMER_H__
#define __SRC_UTILS_TIMER_H__

#include <stddef.h>
#include <stdint.h>

namespace Utils
{

class TimeEntry;
class TimeWheel;

//
// 定时器事件
//
class ITimerEvent
{
public :
    ITimerEvent() : m_Entry(NULL) {}
    virtual ~ITimerEvent() {}

public :
    // 是否暂停...
    bool isPaused() const;
    // 是否在调度中...
    bool isSchedule() const;

    // 需要调用者实现
    // NOTICE: onTimer() 和 onEnd() 不能调用cancel()和schedule()
    virtual bool onTimer( uint32_t & timeout ) = 0;
    virtual void onEnd() = 0;

private :
    friend class TimeWheel;
    TimeEntry * m_Entry;
};

//
// 定时器管理器(时间轮)
//
class TimeWheel
{
public :
    // count        - 桶的个数, 必须是2的N次幂
    // percision    - 最大精度, 精确到毫秒, update()的调用粒度
    TimeWheel( uint32_t count, uint32_t percision );
    ~TimeWheel();

public :
    // 初始化/清空
    bool init();
    void clear();

    // 更新
    int32_t update();

    // 添加定时器
    //      ev      - 定时器句柄;
    //      msecs   - 定时器的超时时间, 自动对齐到最大精度
    //
    // 根据ev的状态进行行为描述 :
    //      1. ev首次添加到管理器中, 超时时间msecs < 最大精度的情况下, 会自动对齐到最大精度
    //      2. ev已经在管理器的调度中, 添加时会先将上一次的调度取消, 重新添加到管理器中
    //         msecs == 0, 定时器的超时时间不做更改
    //         msecs > 最大精度, 定时器的超时时间更改为msecs
    //      3. ev处于暂停状态, 添加时会继续走完上一调度周期后, 继续以msecs为超时时间调度
    //         msecs == 0, 定时器的超时时间不做更改
    //         msecs > 最大精度, 定时器在走完上一周期后, 超时时间更改为msecs
    bool schedule( ITimerEvent * ev, uint32_t msecs = 0 );

    // 暂停定时器
    // 根据定时器的状态进行行为描述:
    //      1. 定时器未到期, 定时器挂起, 等待恢复schedule()
    //      2. 定时器刚到期, 定时器挂起, 等待恢复schedule(), 恢复时会多走一个最大精度时间
    bool pause( ITimerEvent * ev );

    // 取消定时器
    bool cancel( ITimerEvent * ev );

private :
    uint32_t getIndex( uint32_t seconds );
    uint32_t getStepCount( uint32_t seconds );

    void add2Tailer( uint32_t index, TimeEntry * entry );
    void removeEntry( TimeEntry * entry, bool notity = true );

private :
    uint32_t        m_DispatchRef;
    uint32_t        m_BucketCount;
    uint32_t        m_MaxPercision;
    TimeEntry *     m_EventBuckets;
};

}

#endif
