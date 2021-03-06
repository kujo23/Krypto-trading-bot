#ifndef K_EV_H_
#define K_EV_H_

namespace K  {
  class EV: public Klass,
            public Events { public: EV() { events = this; };
    private:
      uWS::Hub  *hub   = nullptr;
      uS::Timer *timer = nullptr;
      uS::Async *loop  = nullptr;
      vector<function<void()>> slowFn;
      unsigned int tick = 0;
    protected:
      void load() {
        gw->screen = screen;
        gw->events = events;
        gw->hub = hub = new uWS::Hub(0, true);
      };
      void waitData() {
        hub->createGroup<uWS::CLIENT>();
      };
      void waitTime() {
        timer = new uS::Timer(hub->getLoop());
        timer->setData(this);
        timer->start([](uS::Timer *timer) {
          ((EV*)timer->getData())->timer_1s();
        }, 0, 1e+3);
      };
      void waitUser() {
        if (args.headless) return;
        client->hub = hub;
        hub->createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
      };
      void run() {
        loop = new uS::Async(hub->getLoop());
        loop->setData(this);
        loop->start(walk);
      };
      void end() {
        timer->stop();
        gw->close();
        hub->getDefaultGroup<uWS::CLIENT>().close();
        gw->clear();
        walk(loop);
        hub->getDefaultGroup<uWS::SERVER>().close();
      };
    public:
      void start() {
        if (gw->asyncWs()) gw->countdown = 1;
        hub->run();
      };
      void deferred(const function<void()> &fn) {
        slowFn.push_back(fn);
        loop->send();
      };
    private:
      void async(const function<bool()> &fn) {
        if (fn()) loop->send();
      };
      void (*walk)(uS::Async*) = [](uS::Async *const loop) {
        EV* k = (EV*)loop->getData();
        if (!k->slowFn.empty()) {
          for (function<void()> &it : k->slowFn) it();
          k->slowFn.clear();
        }
        if (gw->waitForData()) loop->send();
        screen->waitForUser();
      };
      void timer_1s() {
        if (!gw->countdown)                  engine->timer_1s();
        else if (gw->countdown-- == 1)       gw->connect();
        if (FN::trueOnce(&gw->refreshWallet)
          or !(tick % 15))                   async(gw->wallet);
        if (!gw->async) {
          if (!(tick % 2))                   async(gw->orders);
          if (!(tick % 3))                   async(gw->levels);
          if (!(tick % 60))                  async(gw->trades);
        }
        if (!(tick % 60))                    client->timer_60s();
        if (client->hub and qp.delayUI
          and !(tick % qp.delayUI))          client->timer_Xs();
        if (!(++tick % 300)) {
          if (qp.cancelOrdersAuto)           async(gw->cancelAll);
          if (tick >= 300 * (qp.delayUI?:1))
            tick = 0;
        }
      };
  };
}

#endif
