#include <sys/time.h>
#include <unistd.h>
#include <queue>



#include "ngx_timer.h"




Timer::Timer(requestData *req, int timeout)
    : deleted_(false), req(req) {
  struct timeval now;
  gettimeofday(&now, NULL);
  // 以毫秒计
  expiredTime_ =
      (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

Timer::~Timer() {
  if(req)
    delete req;
}



bool Timer::isValid() {
  struct timeval now;
  gettimeofday(&now, NULL);
  size_t temp = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
  if (temp < expiredTime_)
    return true;
  else {
    this->setDeleted();
    return false;
  }
}

void Timer::clearReq() {
  req->reset();
  this->setDeleted();
}



void TimerManager::addTimer(requestData *req, int timeout) {
  sp_timer new_node(new Timer(req, timeout));
  timerQueue.push(new_node);
  req->linkTimer(new_node);
}

/* 处理逻辑是这样的~
因为(1) 优先队列不支持随机访问
(2) 即使支持，随机删除某节点后破坏了堆的结构，需要重新更新堆结构。
所以对于被置为deleted的时间节点，会延迟到它(1)超时 或
(2)它前面的节点都被删除时，它才会被删除。
一个点被置为deleted,它最迟会在TIMER_TIME_OUT时间后被删除。
这样做有两个好处：
(1) 第一个好处是不需要遍历优先队列，省时。
(2)
第二个好处是给超时时间一个容忍的时间，就是设定的超时时间是删除的下限(并不是一到超时时间就立即删除)，如果监听的请求在超时后的下一次请求中又一次出现了，
就不用再重新申请RequestData节点了，这样可以继续重复利用前面的RequestData，减少了一次delete和一次new的时间。
*/

void TimerManager::handleExpireEvent() {
  // MutexLockGuard locker(lock);
  while (!timerQueue.empty()) {
    sp_timer ptimer_now = timerQueue.top();
    if (ptimer_now->isDeleted())
      timerQueue.pop();
    else if (ptimer_now->isValid() == false)
      timerQueue.pop();
    else
      break;
  }
}





