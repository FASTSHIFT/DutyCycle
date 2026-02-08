/*
 * MIT License
 * Copyright (c) 2021 - 2024 _VIFEXTech
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
#include "External/argparse/argparse.h"
#include "Service/HAL/HAL.h"
#include "Service/HAL/HAL_Log.h"
#include "Utils/CommonMacro/CommonMacro.h"
#include "Utils/Shell/Shell.h"
#include <stdio.h>
#include <stdlib.h>

using namespace DataProc;

class DP_Shell {
public:
    DP_Shell(DataNode* node);

private:
    template <typename T>
    class CMD_PAIR {
    public:
        constexpr CMD_PAIR(T c, const char* n)
            : cmd(c)
            , name(n)
        {
        }

    public:
        T cmd;
        const char* name;
    };
#define CMD_PAIR_DEF(CMD_TYPE, CMD) \
    {                               \
        CMD_TYPE::CMD, #CMD         \
    }

    template <typename T>
    class CmdMapHelper {
    public:
        constexpr CmdMapHelper(const CMD_PAIR<T>* map, size_t size)
            : _map(map)
            , _size(size)
        {
        }

        bool get(const char* name, T* cmd) const
        {
            if (!name) {
                shell_print_error(E_SHELL_ERR_PARSE, "command is null");
                return false;
            }

            for (size_t i = 0; i < _size; i++) {
                if (strcmp(name, _map[i].name) == 0) {
                    *cmd = _map[i].cmd;
                    return true;
                }
            }

            char buf[64];
            snprintf(buf, sizeof(buf), "Invalid command '%s', available commands are:", name);
            shell_print_error(E_SHELL_ERR_PARSE, buf);
            for (size_t i = 0; i < _size; i++) {
                shell_println(_map[i].name);
            }

            return false;
        }

    private:
        const CMD_PAIR<T>* _map;
        size_t _size;
    };

    template <typename PULL_TYPE, typename NOTIFY_TYPE>
    class ShellNodeHelper {
    public:
        ShellNodeHelper(const char* name)
            : _name(name)
        {
            _target = _node->subscribe(name);
            if (!_target) {
                char buf[64];
                snprintf(buf, sizeof(buf), "subscribe '%s' failed", _name);
                shell_print_error(E_SHELL_ERR_ACTION, buf);
                return;
            }
        }
        ~ShellNodeHelper()
        {
            _node->unsubscribe(_target);
        }

        operator const DataNode*() const
        {
            return _target;
        }

        bool pull(PULL_TYPE* info)
        {
            int ret = _node->pull(_target, info, sizeof(PULL_TYPE));
            if (ret != DataNode::RES_OK) {
                char buf[64];
                snprintf(buf, sizeof(buf), "pull '%s' failed: %d", _name, ret);
                shell_print_error(E_SHELL_ERR_IO, buf);
                return false;
            }

            return true;
        }

        bool notify(const NOTIFY_TYPE* info)
        {
            int ret = _node->notify(_target, info, sizeof(NOTIFY_TYPE));
            if (ret != DataNode::RES_OK) {
                char buf[64];
                snprintf(buf, sizeof(buf), "notify '%s' failed: %d", _name, ret);
                shell_print_error(E_SHELL_ERR_IO, buf);
                return false;
            }

            return true;
        }

    private:
        const char* _name;
        const DataNode* _target;
    };

private:
    static DataNode* _node;
    const DataNode* _nodeGlobal;
    static DeviceObject* _devSerial;

private:
    int onEvent(DataNode::EventParam_t* param);
    void onGlobalEvent(const Global_Info_t* info);

    static bool argparseHelper(int argc, const char** argv, struct argparse_option* options);

    static int cmdHelp(int argc, const char** argv);
    static int cmdLogLevel(int argc, const char** argv);
    static int cmdPs(int argc, const char** argv);

    static int cmdPublish(int argc, const char** argv);
    static int cmdClock(int argc, const char** argv);
    static int cmdPower(int argc, const char** argv);
    static int cmdCtrl(int argc, const char** argv);
    static int cmdAlarm(int argc, const char** argv);
    static int cmdKVDB(int argc, const char** argv);
};

DataNode* DP_Shell::_node = nullptr;
DeviceObject* DP_Shell::_devSerial = nullptr;

DP_Shell::DP_Shell(DataNode* node)
{
    _node = node;
    _devSerial = HAL::Manager()->getDevice("SerialIO");
    if (!_devSerial) {
        return;
    }

    _nodeGlobal = _node->subscribe("Global");
    if (!_nodeGlobal) {
        return;
    }

    _node->setEventCallback(
        [](DataNode* n, DataNode::EventParam_t* param) -> int {
            auto ctx = (DP_Shell*)n->getUserData();
            return ctx->onEvent(param);
        },
        DataNode::EVENT_PUBLISH);

    shell_init(
        /* reader */
        [](char* data) -> int {
            return DP_Shell::_devSerial->read(data, sizeof(char));
        },
        /* writer */
        [](char data) {
            DP_Shell::_devSerial->write(&data, sizeof(data));
        },
        /* tick_get */
        []() -> uint32_t {
            return HAL::GetTick();
        },
        nullptr, nullptr);

    shell_register(cmdHelp, "help");
    shell_register(cmdLogLevel, "loglevel");
    shell_register(cmdPs, "ps");

    shell_register(cmdPublish, "publish");
    shell_register(cmdClock, "clock");
    shell_register(cmdPower, "power");
    shell_register(cmdCtrl, "ctrl");
    shell_register(cmdAlarm, "alarm");
    shell_register(cmdKVDB, "kvdb");
}

int DP_Shell::onEvent(DataNode::EventParam_t* param)
{
    switch (param->event) {
    case DataNode::EVENT_PUBLISH: {
        if (param->tran == _nodeGlobal) {
            onGlobalEvent((const Global_Info_t*)param->data_p);
        }
    } break;

    default:
        break;
    }

    return DataNode::RES_OK;
}

void DP_Shell::onGlobalEvent(const Global_Info_t* info)
{
    if (info->event == GLOBAL_EVENT::APP_RUN_LOOP_BEGIN) {
        shell_task();
    }
}

bool DP_Shell::argparseHelper(int argc, const char** argv, struct argparse_option* options)
{
    struct argparse argparse;
    argparse_init(&argparse, options, nullptr, 0);
    if (argparse_parse(&argparse, argc, argv) > 0) {
        shell_print_error(E_SHELL_ERR_PARSE, argv[0]);
        return false;
    }

    return true;
}

int DP_Shell::cmdHelp(int argc, const char** argv)
{
    shell_print_commands();
    return SHELL_RET_SUCCESS;
}

int DP_Shell::cmdLogLevel(int argc, const char** argv)
{
    if (argc < 2) {
        shell_print_error(E_SHELL_ERR_ARGCOUNT, "Usage: loglevel <level>, level: 0~4");
        return SHELL_RET_FAILURE;
    }

    HAL_Log_SetLevel(atoi(argv[1]));
    return SHELL_RET_SUCCESS;
}

int DP_Shell::cmdPs(int argc, const char** argv)
{
    extern void HAL_MemoryDumpInfo();
    HAL_MemoryDumpInfo();
    return SHELL_RET_SUCCESS;
}

int DP_Shell::cmdPublish(int argc, const char** argv)
{
    if (argc < 2) {
        shell_print_error(E_SHELL_ERR_ARGCOUNT, "Usage: publish <topic> [data]");
        return SHELL_RET_FAILURE;
    }

    Shell_Info_t info;
    info.argc = argc;
    info.argv = argv;
    const int retval = _node->publish(&info, sizeof(info));
    shell_printf("publish finished: %d\r\n", retval);

    return retval == DataNode::RES_OK ? SHELL_RET_SUCCESS : SHELL_RET_FAILURE;
}

int DP_Shell::cmdClock(int argc, const char** argv)
{
    ShellNodeHelper<HAL::Clock_Info_t, Clock_Info_t> nodeClock("Clock");
    if (!nodeClock) {
        return SHELL_RET_FAILURE;
    }

    HAL::Clock_Info_t info;
    if (!nodeClock.pull(&info)) {
        return SHELL_RET_FAILURE;
    }

    static const char* week_str[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };

    shell_printf(
        "Current clock: %04d-%02d-%02d %s %02d:%02d:%02d.%d\r\n",
        info.year,
        info.month,
        info.day,
        week_str[info.week % 7],
        info.hour,
        info.minute,
        info.second,
        info.millisecond);

    const char* cmd = nullptr;
    int year = info.year;
    int month = info.month;
    int day = info.day;
    int hour = info.hour;
    int minute = info.minute;
    int second = info.second;
    int calPeriodSec = 0;
    int calOffset = 0;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('c', "cmd", &cmd, "clock command", nullptr, 0, 0),
        OPT_INTEGER('y', "year", &year, "year", nullptr, 0, 0),
        OPT_INTEGER('m', "month", &month, "month", nullptr, 0, 0),
        OPT_INTEGER('d', "day", &day, "day", nullptr, 0, 0),
        OPT_INTEGER('H', "hour", &hour, "hour", nullptr, 0, 0),
        OPT_INTEGER('M', "minute", &minute, "minute", nullptr, 0, 0),
        OPT_INTEGER('S', "second", &second, "second", nullptr, 0, 0),
        OPT_INTEGER(0, "cal-period", &calPeriodSec, "calibration period in seconds", nullptr, 0, 0),
        OPT_INTEGER(0, "cal-offset", &calOffset, "calibration offset in clock cycles", nullptr, 0, 0),
        OPT_END(),
    };

    if (!argparseHelper(argc, argv, options)) {
        return SHELL_RET_FAILURE;
    }

    Clock_Info_t clockInfo;
    clockInfo.base.year = year;
    clockInfo.base.month = month;
    clockInfo.base.day = day;
    clockInfo.base.hour = hour;
    clockInfo.base.minute = minute;
    clockInfo.base.second = second;
    clockInfo.base.calPeriodSec = calPeriodSec;
    clockInfo.base.calOffsetClk = calOffset;

    switch (calPeriodSec) {
    case 0:
    case 8:
    case 16:
    case 32:
        break;

    default:
        shell_print_error(E_SHELL_ERR_OUTOFRANGE, "invalid calibration period, must be 8, 16, or 32 seconds");
        return SHELL_RET_FAILURE;
    }

    if (calOffset < -511 || calOffset > 511) {
        shell_print_error(E_SHELL_ERR_OUTOFRANGE, "invalid calibration offset, must be between -511 and 511");
        return SHELL_RET_FAILURE;
    }

    static constexpr CMD_PAIR<CLOCK_CMD> cmd_map[] = {
        CMD_PAIR_DEF(CLOCK_CMD, SET),
    };

    static constexpr CmdMapHelper<CLOCK_CMD> cmdMap(cmd_map, CM_ARRAY_SIZE(cmd_map));
    if (!cmdMap.get(cmd, &clockInfo.cmd)) {
        return SHELL_RET_FAILURE;
    }

    if (!nodeClock.notify(&clockInfo)) {
        return SHELL_RET_FAILURE;
    }

    shell_printf(
        "New clock: %04d-%02d-%02d %02d:%02d:%02d.%d\r\n",
        clockInfo.base.year,
        clockInfo.base.month,
        clockInfo.base.day,
        clockInfo.base.hour,
        clockInfo.base.minute,
        clockInfo.base.second,
        clockInfo.base.millisecond);

    if (calPeriodSec > 0) {
        shell_printf(
            "Clock calibration set: period %d seconds, offset %d clocks\r\n",
            clockInfo.base.calPeriodSec,
            clockInfo.base.calOffsetClk);
    }

    return SHELL_RET_SUCCESS;
}

int DP_Shell::cmdPower(int argc, const char** argv)
{
    ShellNodeHelper<Power_Info_t, Power_Info_t> nodePower("Power");
    if (!nodePower) {
        return SHELL_RET_FAILURE;
    }

    const char* cmd = nullptr;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('c', "cmd", &cmd, "send power command", nullptr, 0, 0),
        OPT_END(),
    };

    if (!argparseHelper(argc, argv, options)) {
        return SHELL_RET_FAILURE;
    }

    static constexpr CMD_PAIR<POWER_CMD> cmd_map[] = {
        CMD_PAIR_DEF(POWER_CMD, SHUTDOWN),
        CMD_PAIR_DEF(POWER_CMD, REBOOT),
    };

    static constexpr CmdMapHelper<POWER_CMD> cmdMap(cmd_map, CM_ARRAY_SIZE(cmd_map));

    Power_Info_t info;
    if (!cmdMap.get(cmd, &info.cmd)) {
        return SHELL_RET_FAILURE;
    }

    if (!nodePower.notify(&info)) {
        return SHELL_RET_FAILURE;
    }

    return SHELL_RET_SUCCESS;
}

int DP_Shell::cmdCtrl(int argc, const char** argv)
{
    ShellNodeHelper<Ctrl_Info_t, Ctrl_Info_t> nodeCtrl("Ctrl");
    if (!nodeCtrl) {
        return SHELL_RET_FAILURE;
    }

    const char* cmd = nullptr;
    int hour = -1;
    int motorValue[2] = { 0, -1 };
    int mode = 0;
    int immediate = 0;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('c', "cmd", &cmd, "send ctrl command", nullptr, 0, 0),
        OPT_INTEGER('H', "hour", &hour, "the hour to set", nullptr, 0, 0),
        OPT_INTEGER('M', "motor", &motorValue[0], "the motor[0] value to set", nullptr, 0, 0),
        OPT_INTEGER(0, "motor1", &motorValue[1], "the motor[1] value to set", nullptr, 0, 0),
        OPT_INTEGER(0, "mode", &mode, "display mode, 0: cos-phi, 1: linear", nullptr, 0, 0),
        OPT_BOOLEAN('I', "immediate", &immediate, "immediately set the value", nullptr, 0, 0),
        OPT_END(),
    };

    if (!argparseHelper(argc, argv, options)) {
        return SHELL_RET_FAILURE;
    }

    static constexpr CMD_PAIR<CTRL_CMD> cmd_map[] = {
        CMD_PAIR_DEF(CTRL_CMD, SWEEP_TEST),
        CMD_PAIR_DEF(CTRL_CMD, ENABLE_CLOCK_MAP),
        CMD_PAIR_DEF(CTRL_CMD, LIST_CLOCK_MAP),
        CMD_PAIR_DEF(CTRL_CMD, SET_MOTOR_VALUE),
        CMD_PAIR_DEF(CTRL_CMD, SET_CLOCK_MAP),
        CMD_PAIR_DEF(CTRL_CMD, SET_MODE),
        CMD_PAIR_DEF(CTRL_CMD, SHOW_BATTERY_USAGE),
    };

    static constexpr CmdMapHelper<CTRL_CMD> cmdMap(cmd_map, CM_ARRAY_SIZE(cmd_map));

    Ctrl_Info_t info;

    switch (mode) {
    case 0:
        info.displayMode = CTRL_DISPLAY_MODE::COS_PHI;
        break;

    case 1:
        info.displayMode = CTRL_DISPLAY_MODE::LINEAR;
        break;

    case 2:
        info.displayMode = CTRL_DISPLAY_MODE::DUAL_LINEAR;
        break;

    default:
        shell_print_error(E_SHELL_ERR_OUTOFRANGE, "invalid display mode");
        return SHELL_RET_FAILURE;
    }

    info.hour = hour;
    info.motorValue[0] = motorValue[0];
    info.motorValue[1] = motorValue[1];
    info.immediate = immediate;
    if (!cmdMap.get(cmd, &info.cmd)) {
        return SHELL_RET_FAILURE;
    }

    if (!nodeCtrl.notify(&info)) {
        return SHELL_RET_FAILURE;
    }

    return SHELL_RET_SUCCESS;
}

int DP_Shell::cmdAlarm(int argc, const char** argv)
{
    ShellNodeHelper<Alarm_Info_t, Alarm_Info_t> nodeAlarm("Alarm");
    if (!nodeAlarm) {
        return SHELL_RET_FAILURE;
    }

    const char* cmd = nullptr;
    const char* filter_str = nullptr;
    Alarm_Info_t info;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('c', "cmd", &cmd, "send alarm command", nullptr, 0, 0),
        OPT_INTEGER('i', "ID", &info.id, "alarm ID", nullptr, 0, 0),
        OPT_INTEGER('H', "hour", &info.hour, "hour", nullptr, 0, 0),
        OPT_INTEGER('M', "minute", &info.minute, "minute", nullptr, 0, 0),
        OPT_INTEGER('m', "music", &info.musicID, "music ID", nullptr, 0, 0),
        OPT_STRING('f', "filter", &filter_str, "hourly alarm filter, e.g. 1,2,3,4", nullptr, 0, 0),
        OPT_INTEGER(0, "index", &info.index, "tone index", nullptr, 0, 0),
        OPT_INTEGER(0, "freq", &info.frequency, "tone frequency(Hz)", nullptr, 0, 0),
        OPT_INTEGER(0, "duration", &info.duration, "tone duration(ms)", nullptr, 0, 0),
        OPT_INTEGER(0, "time", &info.time, "tone time(ms)", nullptr, 0, 0),
        OPT_INTEGER(0, "bpm", &info.bpm, "tone bpm", nullptr, 0, 0),
        OPT_END(),
    };

    if (!argparseHelper(argc, argv, options)) {
        return SHELL_RET_FAILURE;
    }

    static constexpr CMD_PAIR<ALARM_CMD> cmd_map[] = {
        CMD_PAIR_DEF(ALARM_CMD, SET),
        CMD_PAIR_DEF(ALARM_CMD, LIST),
        CMD_PAIR_DEF(ALARM_CMD, SET_FILTER),
        CMD_PAIR_DEF(ALARM_CMD, SET_ALARM_MUSIC),
        CMD_PAIR_DEF(ALARM_CMD, LIST_ALARM_MUSIC),
        CMD_PAIR_DEF(ALARM_CMD, CLEAR_ALARM_MUSIC),
        CMD_PAIR_DEF(ALARM_CMD, SAVE_ALARM_MUSIC),
        CMD_PAIR_DEF(ALARM_CMD, PLAY_ALARM_MUSIC),
        CMD_PAIR_DEF(ALARM_CMD, PLAY_ALARM_HOURLY),
        CMD_PAIR_DEF(ALARM_CMD, PLAY_TONE),
    };

    static constexpr CmdMapHelper<ALARM_CMD> cmdMap(cmd_map, CM_ARRAY_SIZE(cmd_map));

    if (filter_str) {
        char buf[64] = { 0 };
        strncpy(buf, filter_str, sizeof(buf) - 1);

        char* token = strtok(buf, ",");
        while (token) {
            int hour = atoi(token);
            shell_printf("add hour: %d to filter\r\n", hour);
            if (hour < 0 || hour > 24) {
                shell_print_error(E_SHELL_ERR_OUTOFRANGE, "invalid hourly alarm filter");
                return SHELL_RET_FAILURE;
            }

            info.filter |= 1 << hour;

            token = strtok(nullptr, ",");
        }
    }

    if (!cmdMap.get(cmd, &info.cmd)) {
        return SHELL_RET_FAILURE;
    }

    if (!nodeAlarm.notify(&info)) {
        return SHELL_RET_FAILURE;
    }

    return SHELL_RET_SUCCESS;
}

int DP_Shell::cmdKVDB(int argc, const char** argv)
{
    ShellNodeHelper<KVDB_Info_t, KVDB_Info_t> nodeKVDB("KVDB");
    if (!nodeKVDB) {
        return SHELL_RET_FAILURE;
    }

    const char* cmd = nullptr;
    const char* key = nullptr;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('c', "cmd", &cmd, "send KVDB command", nullptr, 0, 0),
        OPT_STRING('k', "key", &key, "key of the value", nullptr, 0, 0),
        OPT_END(),
    };

    if (!argparseHelper(argc, argv, options)) {
        return SHELL_RET_FAILURE;
    }

    KVDB_Info_t info;
    info.key = key;

    static constexpr CMD_PAIR<KVDB_CMD> cmd_map[] = {
        CMD_PAIR_DEF(KVDB_CMD, DEL),
        CMD_PAIR_DEF(KVDB_CMD, LIST),
        CMD_PAIR_DEF(KVDB_CMD, SAVE),
    };

    static constexpr CmdMapHelper<KVDB_CMD> cmdMap(cmd_map, CM_ARRAY_SIZE(cmd_map));

    if (!cmdMap.get(cmd, &info.cmd)) {
        return SHELL_RET_FAILURE;
    }

    if (!nodeKVDB.notify(&info)) {
        return SHELL_RET_FAILURE;
    }

    return SHELL_RET_SUCCESS;
}

DATA_PROC_DESCRIPTOR_DEF(Shell)
