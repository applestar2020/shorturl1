#ifndef __NGX_TIMER__
#define __NGX_TIMER__

#include <unistd.h>
#include <deque>
#include <queue>
#include <memory>

#include "ngx_request.h"

using namespace std;

class requestData;
class Timer;

class Timer
{
public:
    Timer(requestData *req, int timeout);
    ~Timer();

    bool isValid();
    void clearReq();
    void setDeleted() { deleted_ = true; }
    bool isDeleted() const { return deleted_; }
    size_t getExpTime() const { return expiredTime_; }

private:
    bool deleted_;
    size_t expiredTime_;
    requestData *req;
};

struct TimerCmp {
  bool operator()(std::shared_ptr<Timer> &a,
                  std::shared_ptr<Timer> &b) const {
    return a->getExpTime() > b->getExpTime();
  }
};


class TimerManager{
    public:
    using sp_timer=shared_ptr<Timer>;
    void addTimer(requestData *req,int timeout);
    void handleExpireEvent();

    private:
    // typedef shared_ptr<Timer> sp_timer;
    priority_queue<sp_timer,std::deque<sp_timer>,TimerCmp> timerQueue;
};

#endif

