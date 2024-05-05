#include <Arduino.h>
#include "Scheduler.h"
#include "ChatGPT.h"
#include "Speech.h"
#include "Expression.h"
#include <Avatar.h>
#include "MailClient.h"
using namespace m5avatar;

extern Avatar avatar;

void sched_fn_sleep(void)
{
    Serial.printf("sched_fn_sleep\n");
    setCpuFrequencyMhz(80);                         // 動作周波数を落とす
    M5.Display.setBrightness(8);                    // LDCの輝度を下げる
    avatar.setExpression(Expression::Sleepy);
}

void sched_fn_wake(void)
{
    Serial.printf("sched_fn_wake\n");
    setCpuFrequencyMhz(240);                         // 動作周波数を元に戻す
    M5.Display.setBrightness(127);                   // LDCの輝度を元に戻す
    avatar.setExpression(Expression::Neutral);
}

void sched_fn_morning_info(void)
{
    Serial.printf("sched_fn_morning_info\n");
    exec_chatGPT("今日の日付と天気とメモの内容を教えて。海上の天気は不要です。");
}

void sched_fn_announce_time(void)
{
    struct tm now_time;
    Serial.printf("sched_fn_announce_time\n");   

    if (!getLocalTime(&now_time)) {
        Serial.printf("failed to get time\n");
        return;
    }

    speech(String(now_time.tm_hour) + "時です");
}

void sched_fn_recv_mail(void)
{
    avatar.setSpeechText("受信メール確認中");
    Serial.println("imapReadMail()");
    imapReadMail();
    avatar.setSpeechText("");

    int nMail = recvMessages.size();
    if(nMail > prev_nMail){
      prev_nMail = nMail;
      speech(String(nMail) + "件のメールを受信しています。");
    }
}

void init_schedule(void)
{
    add_schedule(new ScheduleEveryDay(6, 30, sched_fn_wake));               //6:30 省エネモードから復帰
    add_schedule(new ScheduleEveryDay(7, 00, sched_fn_morning_info));       //7:00 今日の日付、天気、メモの内容を話す
    add_schedule(new ScheduleEveryDay(7, 30, sched_fn_morning_info));       //7:30 同上
    add_schedule(new ScheduleEveryDay(23, 30, sched_fn_sleep));             //23:30 省エネモードに遷移

    add_schedule(new ScheduleEveryHour(sched_fn_announce_time, 7, 23));     //時報（7時から23時の間）

    add_schedule(new ScheduleIntervalMinute(5, sched_fn_recv_mail, 7, 23)); //5分置きに受信メールを確認（7時から23時の間）
}