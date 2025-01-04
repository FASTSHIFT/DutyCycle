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

using namespace DataProc;

class DP_Shell {
public:
    DP_Shell(DataNode* node);

private:
    static DataNode* _node;
    const DataNode* _nodeGlobal;
    static DeviceObject* _devSerial;

private:
    int onEvent(DataNode::EventParam_t* param);
    void onGlobalEvent(const Global_Info_t* info);
    static int shellReader(char* data);
    static void shellWriter(char data);
    static uint32_t shellTickGet();
    static int cmdHelp(int argc, const char** argv);
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

    shell_init(shellReader, shellWriter, shellTickGet, nullptr, nullptr);
    shell_register(cmdPublich, "publish");
    shell_register(cmdHelp, "help");
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

int DP_Shell::shellReader(char* data)
{
    return DP_Shell::_devSerial->read(data, sizeof(char));
}

void DP_Shell::shellWriter(char data)
{
    DP_Shell::_devSerial->write(&data, sizeof(data));
}

uint32_t DP_Shell::shellTickGet()
{
    return HAL::GetTick();
}

int DP_Shell::cmdHelp(int argc, const char** argv)
{
    shell_print_commands();
    return SHELL_RET_SUCCESS;
}

int DP_Shell::cmdPublich(int argc, const char** argv)
{
    if (argc < 2) {
        shell_print_error(E_SHELL_ERR_ARGCOUNT, argv[0]);
        shell_println("Usage: publish <topic> [data]");
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
    auto nodeClock = _node->subscribe("Clock");
    if (!nodeClock) {
        shell_print_error(E_SHELL_ERR_ACTION, argv[0]);
        return SHELL_RET_FAILURE;
    }

    HAL::Clock_Info_t info;
    if (_node->pull(nodeClock, &info, sizeof(info)) != DataNode::RES_OK) {
        shell_print_error(E_SHELL_ERR_ACTION, argv[0]);
        return SHELL_RET_FAILURE;
    }

    static const char* week_str[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };

    shell_printf(
        "Current time: %04d-%02d-%02d %s %02d:%02d:%02d.%d\r\n",
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
    const char* cmd = "SET_TIME";

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

    struct argparse argparse;
    argparse_init(&argparse, options, nullptr, 0);
    if (argparse_parse(&argparse, argc, argv) > 0) {
        argparse_usage(&argparse);
        return SHELL_RET_FAILURE;
    }

    Clock_Info_t clockInfo;
    clockInfo.base.year = year;
    clockInfo.base.month = month;
    clockInfo.base.day = day;
    clockInfo.base.hour = hour;
    clockInfo.base.minute = minute;
    clockInfo.base.second = second;

#define CMD_MAP_DEF(cmd)     \
    {                        \
        CLOCK_CMD::cmd, #cmd \
    }
    typedef struct
    {
        CLOCK_CMD cmd;
        const char* name;
    } cmd_map_t;
    static const cmd_map_t cmd_map[] = {
        CMD_MAP_DEF(SET_TIME),
        CMD_MAP_DEF(SET_ALARM),
    };
#undef CMD_MAP_DEF

    for (int i = 0; i < CM_ARRAY_SIZE(cmd_map); i++) {
        if (strcmp(cmd, cmd_map[i].name) == 0) {
            clockInfo.cmd = cmd_map[i].cmd;
            break;
        }
    }

    if (clockInfo.cmd == CLOCK_CMD::NONE) {
        shell_printf("Invalid command %s, available commands are:\r\n", cmd);
        for (int i = 0; i < CM_ARRAY_SIZE(cmd_map); i++) {
            shell_println(cmd_map[i].name);
        }
        return SHELL_RET_FAILURE;
    }

    if (_node->notify(nodeClock, &clockInfo, sizeof(clockInfo)) != DataNode::RES_OK) {
        shell_print_error(E_SHELL_ERR_ACTION, argv[0]);
        return SHELL_RET_FAILURE;
    }

    return SHELL_RET_SUCCESS;
}

int DP_Shell::cmdPower(int argc, const char** argv)
{
    auto nodePower = _node->subscribe("Power");
    if (!nodePower) {
        shell_print_error(E_SHELL_ERR_ACTION, argv[0]);
        return SHELL_RET_FAILURE;
    }

    const char* cmd = nullptr;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('c', "cmd", &cmd, "send power command", nullptr, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, nullptr, 0);
    if (argparse_parse(&argparse, argc, argv) > 0 || !cmd) {
        argparse_usage(&argparse);
        return SHELL_RET_SUCCESS;
    }

#define CMD_MAP_DEF(cmd)     \
    {                        \
        POWER_CMD::cmd, #cmd \
    }
    typedef struct
    {
        POWER_CMD cmd;
        const char* name;
    } cmd_map_t;
    static const cmd_map_t cmd_map[] = {
        CMD_MAP_DEF(SHUTDOWN),
        CMD_MAP_DEF(REBOOT),
    };
#undef CMD_MAP_DEF

    Power_Info_t info;

    for (int i = 0; i < CM_ARRAY_SIZE(cmd_map); i++) {
        if (strcmp(cmd, cmd_map[i].name) == 0) {
            info.cmd = cmd_map[i].cmd;
            break;
        }
    }

    if (info.cmd == POWER_CMD::NONE) {
        shell_printf("Invalid command %s, available commands are:\r\n", cmd);
        for (int i = 0; i < CM_ARRAY_SIZE(cmd_map); i++) {
            shell_println(cmd_map[i].name);
        }
        return SHELL_RET_FAILURE;
    }

    if (_node->notify(nodePower, &info, sizeof(info)) != DataNode::RES_OK) {
        shell_print_error(E_SHELL_ERR_ACTION, argv[0]);
        return SHELL_RET_FAILURE;
    }

    return SHELL_RET_SUCCESS;
}

int DP_Shell::cmdCtrl(int argc, const char** argv)
{
    auto nodeCtrl = _node->subscribe("Ctrl");
    if (!nodeCtrl) {
        shell_println("Ctrl node not found");
        shell_print_error(E_SHELL_ERR_ACTION, argv[0]);
        return SHELL_RET_FAILURE;
    }

    const char* cmd = nullptr;
    int hour = 0;
    int motorValue = 0;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('c', "cmd", &cmd, "send ctrl command", nullptr, 0, 0),
        OPT_INTEGER('h', "hour", &hour, "the hour to set", nullptr, 0, 0),
        OPT_INTEGER('m', "motor", &motorValue, "the motor value to set", nullptr, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, nullptr, 0);
    if (argparse_parse(&argparse, argc, argv) > 0 || !cmd) {
        argparse_usage(&argparse);
        return SHELL_RET_SUCCESS;
    }

#define CMD_MAP_DEF(cmd)    \
    {                       \
        CTRL_CMD::cmd, #cmd \
    }
    typedef struct
    {
        CTRL_CMD cmd;
        const char* name;
    } cmd_map_t;
    static const cmd_map_t cmd_map[] = {
        CMD_MAP_DEF(SWEEP_TEST),
        CMD_MAP_DEF(ENABLE_PRINT),
        CMD_MAP_DEF(DISABLE_PRINT),
        CMD_MAP_DEF(ENABLE_CLOCK_MAP),
        CMD_MAP_DEF(SET_MOTOR_VALUE),
        CMD_MAP_DEF(SET_CLOCK_MAP),
    };
#undef CMD_MAP_DEF

    Ctrl_Info_t info;
    info.hour = hour;
    info.motorValue = motorValue;

    for (int i = 0; i < CM_ARRAY_SIZE(cmd_map); i++) {
        if (strcmp(cmd, cmd_map[i].name) == 0) {
            info.cmd = cmd_map[i].cmd;
            break;
        }
    }

    if (info.cmd == CTRL_CMD::NONE) {
        shell_printf("Invalid command %s, available commands are:\r\n", cmd);
        for (int i = 0; i < CM_ARRAY_SIZE(cmd_map); i++) {
            shell_println(cmd_map[i].name);
        }
        return SHELL_RET_FAILURE;
    }

    if (_node->notify(nodeCtrl, &info, sizeof(info)) != DataNode::RES_OK) {
        shell_println("Ctrl node notify failed");
        shell_print_error(E_SHELL_ERR_ACTION, argv[0]);
        return SHELL_RET_FAILURE;
    }

    return SHELL_RET_SUCCESS;
}

DATA_PROC_DESCRIPTOR_DEF(Shell)
