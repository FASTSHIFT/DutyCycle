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
#define CMD_PAIR_DEF(CMD_TYPE, CMD) { CMD_TYPE::CMD, #CMD }

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

    static int cmdPublich(int argc, const char** argv);
    static int cmdClock(int argc, const char** argv);
    static int cmdPower(int argc, const char** argv);
    static int cmdCtrl(int argc, const char** argv);
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

    shell_register(cmdPublich, "publish");
    shell_register(cmdClock, "clock");
    shell_register(cmdPower, "power");
    shell_register(cmdCtrl, "ctrl");
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

int DP_Shell::cmdPublich(int argc, const char** argv)
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

    int year = info.year;
    int month = info.month;
    int day = info.day;
    int hour = info.hour;
    int minute = info.minute;
    int second = info.second;
    const char* cmd = nullptr;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('c', "cmd", &cmd, "clock command", nullptr, 0, 0),
        OPT_INTEGER('y', "year", &year, "year", nullptr, 0, 0),
        OPT_INTEGER('m', "month", &month, "month", nullptr, 0, 0),
        OPT_INTEGER('d', "day", &day, "day", nullptr, 0, 0),
        OPT_INTEGER('H', "hour", &hour, "hour", nullptr, 0, 0),
        OPT_INTEGER('M', "minute", &minute, "minute", nullptr, 0, 0),
        OPT_INTEGER('S', "second", &second, "second", nullptr, 0, 0),
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

    static constexpr CMD_PAIR<CLOCK_CMD> cmd_map[] = {
        CMD_PAIR_DEF(CLOCK_CMD, SET_TIME),
        CMD_PAIR_DEF(CLOCK_CMD, SET_ALARM),
    };

    static constexpr CmdMapHelper<CLOCK_CMD> cmdMap(cmd_map, CM_ARRAY_SIZE(cmd_map));
    if (!cmdMap.get(cmd, &clockInfo.cmd)) {
        return SHELL_RET_FAILURE;
    }

    if (!nodeClock.notify(&clockInfo)) {
        return SHELL_RET_FAILURE;
    }

    shell_printf(
        "New clock: %04d-%02d-%02d %s %02d:%02d:%02d.%d\r\n",
        clockInfo.base.year,
        clockInfo.base.month,
        clockInfo.base.day,
        week_str[clockInfo.base.week % 7],
        clockInfo.base.hour,
        clockInfo.base.minute,
        clockInfo.base.second,
        clockInfo.base.millisecond);

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
    int motorValue = 0;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('c', "cmd", &cmd, "send ctrl command", nullptr, 0, 0),
        OPT_INTEGER('H', "hour", &hour, "the hour to set", nullptr, 0, 0),
        OPT_INTEGER('M', "motor", &motorValue, "the motor value to set", nullptr, 0, 0),
        OPT_END(),
    };

    if (!argparseHelper(argc, argv, options)) {
        return SHELL_RET_FAILURE;
    }

    static constexpr CMD_PAIR<CTRL_CMD> cmd_map[] = {
        CMD_PAIR_DEF(CTRL_CMD, SWEEP_TEST),
        CMD_PAIR_DEF(CTRL_CMD, ENABLE_CLOCK_MAP),
        CMD_PAIR_DEF(CTRL_CMD, SET_MOTOR_VALUE),
        CMD_PAIR_DEF(CTRL_CMD, SET_CLOCK_MAP),
    };

    static constexpr CmdMapHelper<CTRL_CMD> cmdMap(cmd_map, CM_ARRAY_SIZE(cmd_map));

    Ctrl_Info_t info;
    info.hour = hour;
    info.motorValue = motorValue;
    if (!cmdMap.get(cmd, &info.cmd)) {
        return SHELL_RET_FAILURE;
    }

    if (!nodeCtrl.notify(&info)) {
        return SHELL_RET_FAILURE;
    }

    return SHELL_RET_SUCCESS;
}

DATA_PROC_DESCRIPTOR_DEF(Shell)
