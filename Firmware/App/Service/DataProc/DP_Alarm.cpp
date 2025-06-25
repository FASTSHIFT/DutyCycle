/*
 * MIT License
 * Copyright (c) 2021 - 2025 _VIFEXTech
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "DataProc.h"
#include "Service/HAL/HAL_Log.h"
#include "Utils/CommonMacro/CommonMacro.h"
#include "Utils/ToneMap/ToneMap.h"

#define KVDB_GET(value) _kvdb.get(#value, &value, sizeof(value))
#define KVDB_SET(value) _kvdb.set(#value, &value, sizeof(value))

#define HOUR_IS_VALID(h) ((h) >= -1 && (h) < 24)
#define MUSIC_SEQ_MAX 8

using namespace DataProc;

class DP_Alarm {
public:
    DP_Alarm(DataNode* node);

private:
    typedef struct Alarm_Item {
        Alarm_Item()
            : hour(-1)
            , minute(-1)
            , musicID(0)
        {
        }
        int8_t hour;
        int8_t minute;
        int8_t musicID;
    } Alarm_Item_t;

    typedef struct Alarm_Param {
        Alarm_Param()
            : hourlyAlarmFilter(0xFFFFFFFF)
        {
        }
        uint32_t hourlyAlarmFilter;
        Alarm_Item_t alarms[4];
    } Alarm_Param_t;

    typedef struct Alarm_Music {
        Alarm_Music()
            : bpm(0)
        {
        }
        Audio_Squence_t sequence[MUSIC_SEQ_MAX];
        uint16_t bpm;
    } Alarm_Music_t;

private:
    DataNode* _node;
    const DataNode* _nodeTimeMonitor;
    const DataNode* _nodeGlobal;
    KVDB_Helper _kvdb;
    Audio_Helper _audio;

    Audio_Squence_t _seq[4];
    Alarm_Param_t _alarmParam;
    Alarm_Music_t _alarmMusicCustom;

private:
    int onEvent(DataNode::EventParam_t* param);
    int onNotify(const Alarm_Info_t* info);
    void onGlobalEvent(const Global_Info_t* info);
    void onHourChanged(int hour);
    void onMinuteChanged(int hour, int minute);
    int setAlarmMusic(const Alarm_Info_t* info);
    int playAlarmMusic(int musicID);
    int playAlarmHourlyMusic(int hour);
    int playTone(uint32_t freq, uint32_t duration);
    void listAlarms();
    void listAlarmMusic();
};

DP_Alarm::DP_Alarm(DataNode* node)
    : _node(node)
    , _kvdb(node)
    , _audio(node)
{
    _nodeTimeMonitor = _node->subscribe("TimeMonitor");
    if (!_nodeTimeMonitor) {
        return;
    }

    _nodeGlobal = _node->subscribe("Global");

    _node->setEventCallback(
        [](DataNode* n, DataNode::EventParam_t* param) -> int {
            auto ctx = (DP_Alarm*)n->getUserData();
            return ctx->onEvent(param);
        },
        DataNode::EVENT_PUBLISH | DataNode::EVENT_NOTIFY);
}

int DP_Alarm::onEvent(DataNode::EventParam_t* param)
{
    switch (param->event) {
    case DataNode::EVENT_PUBLISH: {
        if (param->tran == _nodeTimeMonitor) {
            auto info = (const TimeMonitor_Info_t*)param->data_p;
            if (info->event == TIME_MONITOR_EVENT::HOUR_CHANGED) {
                onHourChanged(info->clock->hour);
            } else if (info->event == TIME_MONITOR_EVENT::MINUTE_CHANGED) {
                onMinuteChanged(info->clock->hour, info->clock->minute);
            }
        } else if (param->tran == _nodeGlobal) {
            onGlobalEvent((const Global_Info_t*)param->data_p);
        }
    } break;

    case DataNode::EVENT_NOTIFY:
        if (param->size != sizeof(Alarm_Info_t)) {
            return DataNode::RES_SIZE_MISMATCH;
        }
        return onNotify((const Alarm_Info_t*)param->data_p);

    default:
        return DataNode::RES_UNSUPPORTED_REQUEST;
    }

    return DataNode::RES_OK;
}

int DP_Alarm::onNotify(const Alarm_Info_t* info)
{
    switch (info->cmd) {
    case ALARM_CMD::SET: {
        if (info->id < 0 || info->id >= CM_ARRAY_SIZE(_alarmParam.alarms)) {
            HAL_LOG_ERROR("Invalid alarm ID: %d", info->id);
            return DataNode::RES_PARAM_ERROR;
        }

        if (!HOUR_IS_VALID(info->hour)) {
            HAL_LOG_ERROR("Invalid hour: %d", info->hour);
            return DataNode::RES_PARAM_ERROR;
        }

        if (info->minute < 0 || info->minute > 59) {
            HAL_LOG_ERROR("Invalid minute: %d", info->minute);
            return DataNode::RES_PARAM_ERROR;
        }

        if (info->hour < 0) {
            HAL_LOG_INFO("Disable alarm %d", info->id);
        }

        _alarmParam.alarms[info->id].hour = info->hour;
        _alarmParam.alarms[info->id].minute = info->minute;
        _alarmParam.alarms[info->id].musicID = info->musicID;

        return KVDB_SET(_alarmParam);
    }

    case ALARM_CMD::LIST:
        listAlarms();
        break;

    case ALARM_CMD::SET_FILTER:
        _alarmParam.hourlyAlarmFilter = info->filter;
        HAL_LOG_INFO("Set hourly alarm filter: 0x%08X", _alarmParam.hourlyAlarmFilter);
        return KVDB_SET(_alarmParam);

    case ALARM_CMD::SET_ALARM_MUSIC:
        return setAlarmMusic(info);

    case ALARM_CMD::LIST_ALARM_MUSIC:
        listAlarmMusic();
        break;

    case ALARM_CMD::CLEAR_ALARM_MUSIC:
        memset(&_alarmMusicCustom, 0, sizeof(_alarmMusicCustom));
        break;

    case ALARM_CMD::PLAY_ALARM_MUSIC:
        return playAlarmMusic(info->musicID);

    case ALARM_CMD::PLAY_ALARM_HOURLY:
        return playAlarmHourlyMusic(info->hour);

    case ALARM_CMD::PLAY_TONE:
        return playTone(info->frequency, info->duration);

    default:
        return DataNode::RES_UNSUPPORTED_REQUEST;
    }

    return DataNode::RES_OK;
}

void DP_Alarm::onGlobalEvent(const Global_Info_t* info)
{
    if (info->event == GLOBAL_EVENT::APP_STARTED) {
        KVDB_GET(_alarmParam);
        listAlarms();
    }
}

void DP_Alarm::onHourChanged(int hour)
{
    /* Apply hourly alarm filter */
    if (!(_alarmParam.hourlyAlarmFilter & (1 << hour))) {
        HAL_LOG_WARN("hour: %d is not in filter: 0x%08X, skip", hour, _alarmParam.hourlyAlarmFilter);
        return;
    }

    playAlarmHourlyMusic(hour);
}

void DP_Alarm::onMinuteChanged(int hour, int minute)
{
    for (int i = 0; i < CM_ARRAY_SIZE(_alarmParam.alarms); i++) {
        if (_alarmParam.alarms[i].hour < 0) {
            continue;
        }

        if (hour == _alarmParam.alarms[i].hour && minute == _alarmParam.alarms[i].minute) {
            HAL_LOG_INFO("Matched alarm %d: %02d:%02d, Music ID: %d",
                i, _alarmParam.alarms[i].hour, _alarmParam.alarms[i].minute, _alarmParam.alarms[i].musicID);

            playAlarmMusic(_alarmParam.alarms[i].musicID);
            break;
        }
    }
}

int DP_Alarm::setAlarmMusic(const Alarm_Info_t* info)
{
    if (info->index < 0 || info->index >= CM_ARRAY_SIZE(_alarmMusicCustom.sequence)) {
        HAL_LOG_ERROR("index: %d out of range: 0~%d", info->index, CM_ARRAY_SIZE(_alarmMusicCustom.sequence));
        return DataNode::RES_PARAM_ERROR;
    }

    Audio_Squence_t* seq = &_alarmMusicCustom.sequence[info->index];
    seq->frequency = info->frequency;
    seq->duration = info->duration;
    seq->time = info->time;

    _alarmMusicCustom.bpm = info->bpm;

    listAlarmMusic();
    return DataNode::RES_OK;
}

int DP_Alarm::playAlarmMusic(int musicID)
{
#define TONE_DUTY_CYCLE 0.8f
#define TONE_BEAT_MAKE(beat) ((uint32_t)((beat) * TONE_DUTY_CYCLE)), (beat)

    static constexpr Audio_Squence_t seq_mtag[] = {
        { ToneMap::M1, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        { ToneMap::M1, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        { ToneMap::M5, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        { ToneMap::M5, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        { ToneMap::L6h, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        { ToneMap::L6h, TONE_BEAT_MAKE(ToneMap::BEAT_1_8) },
        { ToneMap::M2h, TONE_BEAT_MAKE(ToneMap::BEAT_1_4 + ToneMap::BEAT_1_8) },
    };

    static constexpr Audio_Squence_t seq_mc_bgm[] = {
        { ToneMap::H5, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },
        { ToneMap::H4, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        { ToneMap::H1, TONE_BEAT_MAKE(ToneMap::BEAT_1_2) },
        { ToneMap::M6h, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },
        { 0, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },

        { ToneMap::H5, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },
        { ToneMap::H4, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        { ToneMap::H1, TONE_BEAT_MAKE(ToneMap::BEAT_1_2) },
        { ToneMap::H2h, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },
        { 0, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },

        //            { ToneMap::H5, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },
        //            { ToneMap::H4, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        //            { ToneMap::H1, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        //            { ToneMap::H2h, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        //            { ToneMap::M6h, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },
        //            { 0, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },

        //            { ToneMap::H5, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },
        //            { ToneMap::H4, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        //            { ToneMap::H1, TONE_BEAT_MAKE(ToneMap::BEAT_1_2) },
        //            { ToneMap::H2h, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },
    };

    static constexpr Audio_Squence_t seq_gta4_phone[] = {
        { ToneMap::H5, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        { ToneMap::H5, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        { ToneMap::H5, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        { ToneMap::H4, TONE_BEAT_MAKE(ToneMap::BEAT_1_8) },
        { ToneMap::H5, TONE_BEAT_MAKE(ToneMap::BEAT_1_8) },
        { ToneMap::H6h, TONE_BEAT_MAKE(ToneMap::BEAT_1_8) },
        { ToneMap::H5h, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        { ToneMap::H4, TONE_BEAT_MAKE(ToneMap::BEAT_1_8) },
        { ToneMap::H5, TONE_BEAT_MAKE(ToneMap::BEAT_1_2) },
        { 0, TONE_BEAT_MAKE(ToneMap::BEAT_1_8) },

        { ToneMap::H2, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        { ToneMap::H2, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        { ToneMap::H2, TONE_BEAT_MAKE(ToneMap::BEAT_1_8) },
        { ToneMap::H1h, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        { ToneMap::H1, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        { ToneMap::H1h, TONE_BEAT_MAKE(ToneMap::BEAT_1_8) },
        { ToneMap::H3, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
        { ToneMap::H4, TONE_BEAT_MAKE(ToneMap::BEAT_1_4 + ToneMap::BEAT_1_8) },
    };

    typedef struct MusicGrp {
        MusicGrp(const Audio_Squence_t* seq, uint16_t len = 0, uint16_t b = 0)
            : sequence(seq)
            , length(len)
            , bpm(b)
        {
        }
        const Audio_Squence_t* sequence;
        uint16_t length;
        uint16_t bpm;
    } MusicGrp_t;

    MusicGrp_t musics[] = {
        { seq_mtag, CM_ARRAY_SIZE(seq_mtag) },
        { seq_mc_bgm, CM_ARRAY_SIZE(seq_mc_bgm), 40 },
        { seq_gta4_phone, CM_ARRAY_SIZE(seq_gta4_phone), 50 },
        { _alarmMusicCustom.sequence, CM_ARRAY_SIZE(_alarmMusicCustom.sequence), _alarmMusicCustom.bpm }
    };

    if (musicID < 0 || musicID >= CM_ARRAY_SIZE(musics)) {
        HAL_LOG_ERROR("Invalid music ID: %d", musicID);
        return DataNode::RES_PARAM_ERROR;
    }

    return _audio.play(musics[musicID].sequence, musics[musicID].length, musics[musicID].bpm);
}

int DP_Alarm::playAlarmHourlyMusic(int hour)
{
    if (hour < 0 || hour >= 24) {
        HAL_LOG_ERROR("Invalid hour: %d", hour);
        return DataNode::RES_PARAM_ERROR;
    }

    static const uint16_t hourMap[] = {
        ToneMap::L1,
        ToneMap::L3,
        ToneMap::L5,
        ToneMap::M1,
        ToneMap::M3,
        ToneMap::M5,
        ToneMap::H1,
        ToneMap::H3,
        ToneMap::H5,
    };

    const uint32_t hourIndexMax = sizeof(hourMap) / sizeof(hourMap[0]) - 1;

    _seq[0].frequency = hourMap[hour / hourIndexMax];
    _seq[0].duration = ToneMap::BEAT_1_4;
    _seq[1].frequency = hourMap[hour % hourIndexMax + 1];
    _seq[1].duration = ToneMap::BEAT_1_4;
    _seq[2].frequency = hourMap[hour / hourIndexMax + 1];
    _seq[2].duration = ToneMap::BEAT_1_4;
    _seq[3].frequency = hourMap[hour % hourIndexMax];
    _seq[3].duration = ToneMap::BEAT_1_4;
    return _audio.play(AUDIO_HELPER_SEQ_DEF(_seq));
}

int DP_Alarm::playTone(uint32_t freq, uint32_t duration)
{
    _seq[0].frequency = freq;
    _seq[0].duration = duration;
    return _audio.play(_seq, 1, 0, true);
}

void DP_Alarm::listAlarms()
{
    HAL_LOG_INFO("Hourly alarm filter: 0x%08X", _alarmParam.hourlyAlarmFilter);
    for (int i = 0; i < CM_ARRAY_SIZE(_alarmParam.alarms); i++) {
        if (_alarmParam.alarms[i].hour < 0) {
            continue;
        }

        HAL_LOG_INFO("Alarm %d: %02d:%02d, Music ID: %d",
            i, _alarmParam.alarms[i].hour, _alarmParam.alarms[i].minute, _alarmParam.alarms[i].musicID);
    }
}

void DP_Alarm::listAlarmMusic()
{
    for (int i = 0; i < CM_ARRAY_SIZE(_alarmMusicCustom.sequence); i++) {
        HAL_LOG_INFO("[%d]: %d Hz, duration: %d ms, time: %d ms",
            i, _alarmMusicCustom.sequence[i].frequency, _alarmMusicCustom.sequence[i].duration,
            _alarmMusicCustom.sequence[i].time);
    }

    HAL_LOG_INFO("bpm: %d", _alarmMusicCustom.bpm);
}

DATA_PROC_DESCRIPTOR_DEF(Alarm)
