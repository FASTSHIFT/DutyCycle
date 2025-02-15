/*  Command Line Interface (Command Shell) for microcontrollers.
    Copyright (C) 2014 Jesus Ruben Santa Anna Zamudio.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Author website: http://www.geekfactory.mx
    Author e-mail: ruben at geekfactory dot mx
 */
#include "Shell.h"
#include <stdarg.h>
#include <string.h>

/**
 * This structure array contains the available commands and they associated
 * function entry point, other data required by the commands may be added to
 * this structure
 */
static struct shell_command_entry list[CONFIG_SHELL_MAX_COMMANDS];

/**
 * This array of pointers to characters holds the addresses of the beginning of
 * the parameter strings passed to the programs
 */
static char * argv_list[CONFIG_SHELL_MAX_COMMAND_ARGS];

/**
 * This is the main buffer to store received characters from the user´s terminal
 */
static char shellbuf[CONFIG_SHELL_MAX_INPUT];

static shell_reader_t shell_reader = 0;
static shell_writer_t shell_writer = 0;
static shell_tick_get_t shell_tick_get = 0;
static struct shell_outbuffer_data * obhandle = 0;
static bool initialized = false;

static const char * shell_prompt_string = "device>";

/*-------------------------------------------------------------*
 *      Internal function prototypes            *
 *-------------------------------------------------------------*/
/**
 * Parses the string and finds all the substrings (arguments)
 *
 * @param buf The buffer containing the original string
 * @param argv Pointer to char * array to place the pointers to substrings
 * @param maxargs The maximum number of pointers that the previous array can hold
 *
 * @return The total of args parsed
 */
static int shell_parse(char * buf, char** argv, unsigned short maxargs);

/**
 * Helper function for formatting text in shell_printf and shell_printf_pm
 *
 * @param fmt The format string
 * @param va The list of arguments to use
 */
static void shell_format(const char * fmt, va_list va);

/**
 * Process escaped character inside command args string
 *
 * @param argc The total number of arguments received for the command
 * @param argv  Pointers to the argument strings
 */
static void shell_process_escape(int argc, char ** argv);

/**
 *  Prints the command shell prompt
 */
static void shell_prompt(void);

/*-------------------------------------------------------------*
 *      Public API Implementation           *
 *-------------------------------------------------------------*/
bool shell_init(shell_reader_t reader, shell_writer_t writer, shell_tick_get_t tick_get, const char * msg, const char * prompt)
{
    if (reader == 0 || writer == 0)
        return false;

    shell_unregister_all();

    shell_reader = reader;
    shell_writer = writer;
    shell_tick_get = tick_get;
    initialized = true;
    
    if(prompt)
    {
        shell_prompt_string = prompt;
    }

    // Print Message and draw command prompt
    if (msg != 0)
    {
        shell_println(msg);
    }
    else
    {
        shell_print("Microcontroller Shell library Ver. ");
        shell_println(SHELL_VERSION_STRING);
    }
    shell_prompt();
    return true;
}

void shell_use_buffered_output(shell_bwriter_t writer)
{
    static struct shell_outbuffer_data obd;

    obhandle = &obd;

    obd.shell_bwriter = writer;
    obd.buffercount = 0;
    obd.buffertimer = shell_tick_get();
}

bool shell_register(shell_program_t program, const char * string)
{
    uint8_t i;

    for (i = 0; i < CONFIG_SHELL_MAX_COMMANDS; i++)
    {
        if (list[i].shell_program != 0 || list[i].shell_command_string != 0)
            continue;
        list[i].shell_program = program;
        list[i].shell_command_string = string;
        return true;
    }
    return false;
}

void shell_unregister_all()
{
    uint8_t i;

    for (i = 0; i < CONFIG_SHELL_MAX_COMMANDS; i++)
    {
        list[i].shell_program = 0;
        list[i].shell_command_string = 0;
    }
}

void shell_putc(char c)
{
    if (initialized != false && shell_writer != 0)
        shell_writer(c);
    if (initialized != false && obhandle != 0)
    {
        // Keep track of last byte
        obhandle->buffertimer = shell_tick_get();
        // Empty buffer if it´s full before storing anything else
        if (obhandle->buffercount >= 30)
        {
            // Write output...
            if (obhandle->shell_bwriter != 0)
                obhandle->shell_bwriter(obhandle->outbuffer, obhandle->buffercount);
            // and clear counter
            obhandle->buffercount = 0;
        }
        // Write to buffer always
        obhandle->outbuffer[obhandle->buffercount++] = c;
    }
}

void shell_print(const char * string)
{
    while (* string != '\0')
        shell_putc(* string++);
}

void shell_println(const char * string)
{
    shell_print(string);
    shell_print("\r\n");
}

void shell_printf(const char * fmt, ...)
{
    va_list argl;
    va_start(argl, fmt);
    shell_format(fmt, argl);
    va_end(argl);
}

void shell_print_commands()
{
    uint8_t i;
    shell_println("Available Commands:");

    for (i = 0; i < CONFIG_SHELL_MAX_COMMANDS; i++)
    {
        if (list[i].shell_program != 0 || list[i].shell_command_string != 0)
        {
            shell_println(list[i].shell_command_string);
        }
    }
}

void shell_print_error(int error, const char * field)
{
    if (field != 0)
    {
        shell_print("#ERROR-PARAM:");
        shell_print(field);
        shell_print("\r\n");
    }

    shell_print("#ERROR-TYPE:");
    switch (error)
    {
    case E_SHELL_ERR_ARGCOUNT:
        shell_print("ARG-COUNT");
        break;
    case E_SHELL_ERR_OUTOFRANGE:
        shell_print("OUT-OF-RANGE");
        break;
    case E_SHELL_ERR_VALUE:
        shell_print("INVALID-VALUE");
        break;
    case E_SHELL_ERR_ACTION:
        shell_print("INVALID-ACTION");
        break;
    case E_SHELL_ERR_PARSE:
        shell_print("PARSING");
        break;
    case E_SHELL_ERR_STORAGE:
        shell_print("STORAGE");
        break;
    case E_SHELL_ERR_IO:
        shell_print("IO");
        break;
    default:
        shell_print("Unknown");
    }
    shell_print("\r\n");
}

int shell_task(void)
{
    // Number of characters written to buffer (this should be static var)
    static uint16_t count = 0;
    uint8_t i = 0;
    bool cc = 0;
    int retval = 0;
    int argc = 0;
    char rxchar = 0;

    if (!initialized)
        return -1;

    // Process buffered output if enabled
    if (obhandle != 0)
    {
        if (obhandle->buffercount != 0 && shell_tick_get() - obhandle->buffertimer >= 200)
        {
            obhandle->buffertimer = shell_tick_get();
            if (obhandle->shell_bwriter != 0)
                obhandle->shell_bwriter(obhandle->outbuffer, obhandle->buffercount);
            // and clear counter
            obhandle->buffercount = 0;
        }
    }

    // Process each one of the received characters
    while (shell_reader(&rxchar) > 0)
    {

        switch (rxchar)
        {
        case SHELL_ASCII_ESC: // For VT100 escape sequences
            // Process escape sequences: maybe later
            break;

        case SHELL_ASCII_HT:
            shell_putc(SHELL_ASCII_BEL);
            break;

        case SHELL_ASCII_CR: // Enter key pressed
            shellbuf[count] = '\0';
            shell_println("");
            cc = true;
            break;

        case SHELL_ASCII_DEL:
        case SHELL_ASCII_BS: // Backspace pressed
            if (count > 0)
            {
                count--;
                shell_putc(SHELL_ASCII_BS);
                shell_putc(SHELL_ASCII_SP);
                shell_putc(SHELL_ASCII_BS);
            }
            else
                shell_putc(SHELL_ASCII_BEL);
            break;
        default:
            // Process printable characters, but ignore other ASCII chars
            if (count < (CONFIG_SHELL_MAX_INPUT - 1) && rxchar >= 0x20 && rxchar < 0x7F)
            {
                shellbuf[count] = rxchar;
                shell_putc(rxchar);
                count++;
            }
        }
        // Check if a full command is available on the buffer to process
        if (cc)
        {
            argc = shell_parse(shellbuf, argv_list, CONFIG_SHELL_MAX_COMMAND_ARGS);
            // Process escape sequences before giving args to command implementation
            shell_process_escape(argc, argv_list);
            // sequential search on command table
            for (i = 0; i < CONFIG_SHELL_MAX_COMMANDS; i++)
            {
                if (list[i].shell_program == 0)
                    continue;
                // If string matches one on the list
                if (!strcmp(argv_list[0], list[i].shell_command_string))
                {
                    // Run the appropriate function
                    retval = list[i].shell_program(argc, (const char**)argv_list);
                    cc = false;
                }
            }
            // If no command found and buffer not empty
            if (cc != false && count != 0)
            {
                shell_println("Command NOT found."); // Print not found!!
            }
            count = 0;
            cc = false;
            shell_println("");
            shell_prompt();
        }
    }
    return retval;
}

/*-------------------------------------------------------------*
 *      Internal functions              *
 *-------------------------------------------------------------*/
static int shell_parse(char * buf, char ** argv, unsigned short maxargs)
{
    int i = 0;
    int argc = 0;
    int length = strlen(buf) + 1; //String lenght to parse = strlen + 1
    char toggle = 0;
    bool escape = false;

    argv[argc] = &buf[0];

    for (i = 0; i < length && argc < maxargs; i++)
    {
        switch (buf[i])
        {
        // Handle special char: String terminator
        case '\0':
            i = length;
            argc++;
            break;

        // Handle special char: Backslash for escape sequences
        case '\\':
            // Begin of escape sequence, the following char will get
            // to it´s case with the boolean "escape" flag set
            escape = true;
            continue;

        // Handle special char: Double quotes
        case '\"':
            // If double quote is not escaped, process it as string start and end character
            // If it´s escaped then we do nothing and let the next step remove character escaping
            if (!escape)
            {
                if (toggle == 0)
                {
                    toggle = 1;
                    buf[i] = '\0';
                    argv[argc] = &buf[i + 1];
                }
                else
                {
                    toggle = 0;
                    buf[i] = '\0';
                }
            }
            break;

        // Handle special char: Space is token separator
        case ' ':
            if (toggle == 0)
            {
                buf[i] = '\0';
                argc++;
                argv[argc] = &buf[i + 1];
            }
            break;

        }
        // For character escaping
        escape = false;
    }
    return argc;
}

static void shell_process_escape(int argc, char ** argv)
{
    uint8_t i, j;
    int sl;
    uint8_t readindex = 0;

    // loop for every parameter
    for (i = 0; i < argc; i++)
    {
        // get the length of the current arg
        sl = strlen(argv[i]);
        // loop through every character inside this argument
        for (j = 0; j < sl; j++)
        {
            // search for backslash character
            if (argv[i][j + readindex] == '\\')
            {
                // Process escaped characters here
                if (argv[i][j + readindex + 1] == '"')
                {
                    // Write quote double quote on the current position
                    argv[i][j] = '"';
                    readindex++;
                }
            }
            // read ahead and copy to current position only if escaped characters found
            if (readindex)
            {
                argv[i][j] = argv[i][j + readindex];
            }
        }
        // Reset for the next arg
        readindex = 0;
    }
}

static void shell_prompt()
{
    shell_print(shell_prompt_string);
}

/*-------------------------------------------------------------*
 *      Shell formatted print support           *
 *-------------------------------------------------------------*/
#ifdef SHELL_PRINTF_LONG_SUPPORT

static void uli2a(unsigned long int num, unsigned int base, int uc, char * bf)
{
    int n = 0;
    unsigned int d = 1;
    while (num / d >= base)
        d *= base;
    while (d != 0)
    {
        int dgt = num / d;
        num %= d;
        d /= base;
        if (n || dgt > 0 || d == 0)
        {
            *bf++ = dgt + (dgt < 10 ? '0' : (uc ? 'A' : 'a') - 10);
            ++n;
        }
    }
    *bf = 0;
}

static void li2a(long num, char * bf)
{
    if (num < 0)
    {
        num = -num;
        *bf++ = '-';
    }
    uli2a(num, 10, 0, bf);
}
#endif

static void ui2a(unsigned int num, unsigned int base, int uc, char * bf)
{
    int n = 0;
    unsigned int d = 1;
    while (num / d >= base)
        d *= base;
    while (d != 0)
    {
        int dgt = num / d;
        num %= d;
        d /= base;
        if (n || dgt > 0 || d == 0)
        {
            *bf++ = dgt + (dgt < 10 ? '0' : (uc ? 'A' : 'a') - 10);
            ++n;
        }
    }
    *bf = 0;
}

static void i2a(int num, char * bf)
{
    if (num < 0)
    {
        num = -num;
        *bf++ = '-';
    }
    ui2a(num, 10, 0, bf);
}

static int a2d(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    else if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    else return -1;
}

static char a2i(char ch, const char** src, int base, int* nump)
{
    const char* p = *src;
    int num = 0;
    int digit;
    while ((digit = a2d(ch)) >= 0)
    {
        if (digit > base) break;
        num = num * base + digit;
        ch = *p++;
    }
    *src = p;
    *nump = num;
    return ch;
}

static void putchw(int n, char z, char* bf)
{
    char fc = z ? '0' : ' ';
    char ch;
    char* p = bf;
    while (*p++ && n > 0)
        n--;
    while (n-- > 0)
        shell_putc(fc);
    while ((ch = *bf++) != 0)
        shell_putc(ch);
}

static void shell_format(const char * fmt, va_list va)
{
    char bf[12];
    char ch;


    while ((ch = *(fmt++)) != 0)
    {
        if (ch != '%')
            shell_putc(ch);
        else
        {
            char lz = 0;
#ifdef  PRINTF_LONG_SUPPORT
            char lng = 0;
#endif
            int w = 0;
            ch = *(fmt++);
            if (ch == '0')
            {
                ch = *(fmt++);
                lz = 1;
            }
            if (ch >= '0' && ch <= '9')
            {
                ch = a2i(ch, &fmt, 10, &w);
            }
#ifdef  PRINTF_LONG_SUPPORT
            if (ch == 'l')
            {
                ch = *(fmt++);
                lng = 1;
            }
#endif
            switch (ch)
            {
            case 0:
                goto abort;
            case 'u':
            {
#ifdef  PRINTF_LONG_SUPPORT
                if (lng)
                    uli2a(va_arg(va, unsigned long int), 10, 0, bf);
                else
#endif
                    ui2a(va_arg(va, unsigned int), 10, 0, bf);
                putchw(w, lz, bf);
                break;
            }
            case 'd':
            {
#ifdef  PRINTF_LONG_SUPPORT
                if (lng)
                    li2a(va_arg(va, unsigned long int), bf);
                else
#endif
                    i2a(va_arg(va, int), bf);
                putchw(w, lz, bf);
                break;
            }
            case 'x':
            case 'X':
#ifdef  PRINTF_LONG_SUPPORT
                if (lng)
                    uli2a(va_arg(va, unsigned long int), 16, (ch == 'X'), bf);
                else
#endif
                    ui2a(va_arg(va, unsigned int), 16, (ch == 'X'), bf);
                putchw(w, lz, bf);
                break;
            case 'c':
                shell_putc((char) (va_arg(va, int)));
                break;
            case 's':
                putchw(w, 0, va_arg(va, char*));
                break;
            case '%':
                shell_putc(ch);
            default:
                break;
            }
        }
    }
abort:
    ;
}
