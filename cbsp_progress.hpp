#ifndef _CBSP_PROGRESS_H_
#define _CBSP_PROGRESS_H_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>

namespace cbsp
{
    typedef struct _CBSP_PROGRESS
    {
        std::string msg = "";
        size_t beg = 0;
        size_t cur = 0;
        size_t end = 1;
    } CBSP_PROGRESS;

    class Progress final
    {
    public:
        ~Progress() = default;

        static void set(CBSP_PROGRESS pgs)
        {
            std::lock_guard<std::mutex> lk(lock);
            progress = pgs;
        }

        static void start()
        {
            sleep = false;
            cv.notify_all();
        }

        static void wait()
        {
            sleep = true;
        }

        static void stop()
        {
            quit = true;
            sleep = false;
            cv.notify_all();
        }

        static void print()
        {
            float _rate = .0f;
            while (!quit)
            {
                {
                    std::unique_lock<std::mutex> lk(lock);
                    cv.wait(lk, []
                            { return !sleep; });
                }
                {
                    std::lock_guard<std::mutex> lk(lock);
                    _rate = rate(progress.cur, progress.beg, progress.end);
                }

                char _rate_str[16];
                sprintf(_rate_str, "[%3.2lf%%] ", _rate);

                for (auto &c : str) {
                    c = ' ';
                }
                fprintf(stdout, "\r%s", str.c_str());

                str = _rate_str;
                for (size_t i = 0; i < _rate; i++)
                {
                    str += "█|";
                }
                fprintf(stdout, "\r%s", str.c_str());
            }
        }

    private:
        static float rate(size_t cur, size_t beg, size_t end)
        {
            return 100 * static_cast<float>(cur) / static_cast<float>(end - beg);
        }

    private:
        Progress() = delete;
        static bool quit;
        static bool sleep;
        static std::mutex lock;
        static std::thread printer;
        static std::condition_variable cv;
        static CBSP_PROGRESS progress;
        static std::string str;
    };

    bool Progress::quit(false);
    bool Progress::sleep(false);
    std::mutex Progress::lock;
    std::thread Progress::printer{print};
    std::condition_variable Progress::cv;
    CBSP_PROGRESS Progress::progress;
    std::string Progress::str{""};
};

#endif