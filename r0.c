/*++
 * build x86/x64 with:
 *   cl -W4 r0.c -link
 *   cl -W4 -D_TRAP_GUI r0.c -link user32.lib
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

/*++ close a handle ... */
#define _SAFE_FREE(__b)     \
if((__b) != NULL)           \
{                           \
 _TrapFree((__b));          \
 (__b) = NULL;              \
}

/*++ close a handle ... */
#define _SAFE_CLOSE(__h)    \
if((__h) != NULL)           \
{                           \
 CloseHandle((__h));        \
 (__h) = NULL;              \
}

/*++ do a private arraysize macro and avoid including the stdlib.h header ... */
#ifndef _countof
 #define _countof(__arr)        (sizeof((__arr)) / sizeof((__arr)[0]))
#endif  /* _countof */

/*++ switch off debug strings for release builds ... */
#if defined(_DEBUG) || defined(_HYBRID)
 #define _DBGOUT(...)           { __TrapDbgout(__VA_ARGS__); }
#else
 #define _DBGOUT(...)
#endif

/*++
 */
typedef struct _TRAP_BUFFER
{
    ULONG length;           /*++ length of data in buffer ...               */
    ULONG max_length;       /*++ max length of the buffer ...               */
    char* buffer;           /*++ ptr to data ...                            */
} TRAP_BUFFER, *PTRAP_BUFFER;

/*++
 */
typedef struct _TRAP_INFO
{
    DWORD flags;            /*++ trap flags ...                             */
    DWORD status;           /*++ return code of the child process ...       */
    TRAP_BUFFER input;      /*++ input to child's stdin ...                 */
    TRAP_BUFFER output;     /*++ output from child ...                      */
} TRAP_INFO, *PTRAP_INFO;

/*++ trap_params.flags values ... */
#define _TRAP_DEFAULT           0x00000000
#define _TRAP_INPUT_FILENAME    0x00000001

/*++
 */
BOOL
NTAPI
_TrapOuttrap (
    __in char* cmdline,
    __inout PTRAP_INFO pparam
    );

/*++
 */
void*
NTAPI
_TrapAlloc (
    __in size_t size 
    );

/*++
 */
void*
NTAPI
_TrapRealloc (
    __in void* block,
    __in size_t size
    );

/*++
 */
int
NTAPI
_TrapFree (
    __in void* block
    );

/*++
 */
int
NTAPI
_Outtrap (
    __in DWORD flags,
    __in char* cmdline,
    __in char* input 
    );

/*++ >>>>>>>>>>>>>>>>>>>>>>>           |           <<<<<<<<<<<<<<<<<<<<<<< --*/
/*++ >>>>>>>>>>>>>>>>>>>>>>>           |           <<<<<<<<<<<<<<<<<<<<<<< --*/

char* nb =
    "From hence, ye beauties, undeceiv\'d,\n"
    "Know, one false step is ne\'er retriev\'d,\n"
    "And be with caution bold.\n"
    "Not all that tempts your wandering eyes\n"
    "And heedless hearts, is lawful prize;\n"
    "Nor all, that glisters, gold.\n";

#ifdef _TRAP_GUI
/*++
 */
int 
APIENTRY 
WinMain (
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance, 
    LPSTR lpCmdLine, 
    int nCmdShow )
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    _Outtrap(_TRAP_DEFAULT, "rev.exe", nb);
    _Outtrap(_TRAP_DEFAULT, "seq.exe 1 2 25", NULL);
    _Outtrap(_TRAP_DEFAULT, "ls.exe", NULL);
    _Outtrap(_TRAP_INPUT_FILENAME, "rev.exe", "nbrev");

    return TRUE;
}
#else
/*++
 */
int
main (
    int argc,
    char** argv )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    _Outtrap(_TRAP_DEFAULT, "rev.exe", nb);
    _Outtrap(_TRAP_DEFAULT, "seq.exe 1 2 25", NULL);
    _Outtrap(_TRAP_DEFAULT, "ls.exe", NULL);
    _Outtrap(_TRAP_INPUT_FILENAME, "rev.exe", "nbrev");

    return 0;
}
#endif

/*++
 * this is an example of a function included in an application that wishes to
 * trap command output.
 */
int
NTAPI
_Outtrap (
    __in DWORD flags,
    __in char* cmdline,
    __in char* input )
{
    char output[2048] = {0};
#ifndef _TRAP_GUI
    DWORD bytes;
#endif
    TRAP_INFO param = {0};

    /*++
     * setup input buffer, if one was specified. this will be pushed to the
     * stdin of the process created from the specified cmdline ...
     */
    param.input.length = param.input.max_length = ((input == NULL) ? 0 : (ULONG)strlen(input));
    param.input.buffer = ((input == NULL) ? NULL : input);
    param.flags = flags;

    if( _TrapOuttrap(cmdline, &param) == FALSE)
    {
        _snprintf_s(output, _countof(output), _TRUNCATE, "outtrap: output trap failed, status(%X)\n", GetLastError());
#ifdef _TRAP_GUI
        MessageBox(NULL, output, "r0", MB_OK);
#else
        WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), output, (DWORD)strlen(output), &bytes, NULL);
#endif
        return -1;
    }

    _snprintf_s(output, _countof(output), _TRUNCATE,
     "outtrap: app(%s) RC(%X) =L'%X, [max =L'%X]\n"
#ifdef _TRAP_GUI
     "\n"
#endif
     "%s\n",
     cmdline,
     param.status,
     param.output.length,
     param.output.max_length,
     param.output.buffer
     );

#ifdef _TRAP_GUI
    MessageBox(NULL, output, "r0", MB_OK);
#else
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), output, (DWORD)strlen(output), &bytes, NULL);
#endif

    /*++
     * the output buffer is automatically allocated and needs to be free'd by
     * the caller, here.
     */
    _SAFE_FREE(param.output.buffer);

    return 0;
}

/*++ >>>>>>>>>>>>>>>>>>>>>>>           |           <<<<<<<<<<<<<<<<<<<<<<< --*/
/*++ >>>>>>>>>>>>>>>>>>>>>>>        outtrap        <<<<<<<<<<<<<<<<<<<<<<< --*/

#define _TRAP_NUM_THREADS       4       /*++ thread arrary size             */
#define _TRAP_DEFAULT_ALLOC     512     /*++ default alloc granularity ...  */

/*++
 */
typedef struct _TRAP_PIPE
{
    HANDLE childin;         /*++ child stdin ...                            */
    HANDLE childout;        /*++ child stdout ...                           */
    HANDLE childerr;        /*++ child stderr ...                           */
    HANDLE parent_read;     /*++ parent reads from child stdout/stderr ...  */
    HANDLE parent_write;    /*++ parent writes to child stdin ...           */
} TRAP_PIPE, *PTRAP_PIPE;

/*++
 */
typedef struct _TRAP_PARAMS
{
    TRAP_INFO* pinfo;
    TRAP_PIPE io;           /*++ parent/child handles ...                   */
    PROCESS_INFORMATION process;    /*++ child process/thread handles ...   */
} TRAP_PARAMS, *PTRAP_PARAMS;

/*++
 */
static
BOOL
NTAPI
_TrapCreateHandles (
    __inout PTRAP_PARAMS pparam
    );

/*++
 */
static
BOOL
NTAPI
_TrapCreateChild (
    __in char* filename,
    __in char* cmdline,
    __inout PTRAP_PARAMS pparam
    );

/*++
 */
static
DWORD
NTAPI
_TrapReadChild (
    __in void* pparam
    );

/*++
 */
static
DWORD
NTAPI
_TrapWriteChild (
    __in void* pparam
    );

/*++
 */
int
NTAPI
__TrapDbgoutVa (
    __in const wchar_t* format,
    __in va_list ap 
    );

/*++
 */
int
NTAPI
__TrapDbgout (
    __in const wchar_t* format,
    ... 
    );

/*++
 */
BOOL
NTAPI
_TrapOuttrap (
    __in char* cmdline,
    __inout PTRAP_INFO pinfo )
{
    DWORD tids[_TRAP_NUM_THREADS] = {0};
    HANDLE threads[_TRAP_NUM_THREADS] = {0};
    TRAP_PARAMS params = {0};

    if((cmdline == NULL) || (pinfo == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    params.pinfo = pinfo;

    /*++ create the redirection pipes ... */
    if( _TrapCreateHandles(&params) == FALSE)
    {
        _DBGOUT(L"TRP001E  CREATE HANDLES FAILED, STATUS(%X)\n", GetLastError());
        return FALSE;
    }

    /*++ spin off the child app ... */
    if( _TrapCreateChild(NULL, cmdline, &params) == FALSE)
    {
        _DBGOUT(L"TRP002E  CREATE CHILD FAILED, STATUS(%X)\n", GetLastError());
        return FALSE;
    }

    /*++ close out the child handles ... */
    _SAFE_CLOSE(params.io.childin);
    _SAFE_CLOSE(params.io.childout);
    _SAFE_CLOSE(params.io.childerr);

    threads[0] = CreateThread(NULL, 0, _TrapReadChild, (void*)&params, 0, &(tids[0]));
    if(threads[0] == NULL)
    {
        _DBGOUT(L"TRP003E  CREATE READ THREAD FAILED, STATUS(%X)\n", GetLastError());
        return FALSE;
    }

    if(params.pinfo->input.buffer != NULL)
    {
        if( _TrapWriteChild((void*)&params) != ERROR_SUCCESS)
        {
            _DBGOUT(L"TRP004E  WRITE CHILD FAILED, STATUS(%X)\n", GetLastError());
            return FALSE;
        }
    }
    else
    {
        /*++ 
         * close the parent-end of the child's stdin. when writing to the
         * child's stdin, this is closed when the write is complete to let
         * the child know there's no more input. close it here, when there
         * isn't any input, just in case the child is blocked waiting for
         * input ...
         */
        _SAFE_CLOSE(params.io.parent_write);
    }

    /*++ 
     * add the process to handle list and wait for the read thread and 
     * the child to finish ... need to make this more robust. it's not
     * nice to wait forever for shit outside this code's control ...
     */
    threads[1] = params.process.hProcess;
    if( WaitForMultipleObjects(2, threads, TRUE, INFINITE) == WAIT_FAILED) 
    {
        _DBGOUT(L"TRP005E  WAIT FAILED, STATUS(%X)\n", GetLastError());
        return FALSE;
    }

    /*++ close the thread handle ... */
    _SAFE_CLOSE(threads[0]);

    /*++ get the child exit code and close the handle ... */
    GetExitCodeProcess(params.process.hProcess, &(params.pinfo->status));
    _SAFE_CLOSE(params.process.hProcess);

    /*++ close the parent handles ... */
    _SAFE_CLOSE(params.io.parent_read);
    _SAFE_CLOSE(params.io.parent_write);

    return TRUE;
}

/*++ >>>>>>>>>>>>>>>>>>>>>>>           |           <<<<<<<<<<<<<<<<<<<<<<< --*/
/*++ >>>>>>>>>>>>>>>>>>>>>>>        internal       <<<<<<<<<<<<<<<<<<<<<<< --*/

/*++
 */
static
DWORD
NTAPI
_TrapReadChild (
    __in void* pparam )
{
    char* ptr = NULL;
    DWORD bytes = 0;
    PTRAP_PARAMS pnp = (PTRAP_PARAMS)pparam;

    pnp->pinfo->output.length = 0;
    pnp->pinfo->output.max_length = _TRAP_DEFAULT_ALLOC;
    if((pnp->pinfo->output.buffer = _TrapAlloc(pnp->pinfo->output.max_length)) == NULL)
    {
        _DBGOUT(L"TRP006E  GETMAIN(0) FAILED, STATUS(%X)\n", GetLastError());
        return GetLastError();
    }

    ptr = pnp->pinfo->output.buffer;

    while( ReadFile(pnp->io.parent_read, ptr, _TRAP_DEFAULT_ALLOC, &bytes, NULL))
    {
        /*++ get more storage ... */
        pnp->pinfo->output.length += bytes;
        pnp->pinfo->output.max_length += _TRAP_DEFAULT_ALLOC;
        if((pnp->pinfo->output.buffer = _TrapRealloc(pnp->pinfo->output.buffer, pnp->pinfo->output.max_length)) == NULL)
        {
            _DBGOUT(L"TRP007E  GETMAIN(1) FAILED, STATUS(%X)\n", GetLastError());
            return GetLastError();
        }

        ptr = (pnp->pinfo->output.buffer + pnp->pinfo->output.length);
        bytes = 0;
    }
    return ERROR_SUCCESS;
}

/*++
 */
static
DWORD
NTAPI
_TrapWriteChild (
    __in void* pparam )
{
    DWORD bytes = 0;
    PTRAP_PARAMS pnp = (PTRAP_PARAMS)pparam;

    __try
    {
        if(pnp->pinfo->flags & _TRAP_INPUT_FILENAME)
        {
            /*++ input buffer is the name of a file ... */
            char buffer[_TRAP_DEFAULT_ALLOC];
            DWORD read;
            HANDLE osfh;

            osfh = CreateFileA(
             pnp->pinfo->input.buffer, 
             (GENERIC_READ | GENERIC_WRITE), 
             (FILE_SHARE_READ | FILE_SHARE_WRITE), 
             NULL, 
             OPEN_EXISTING, 
             FILE_ATTRIBUTE_NORMAL, 
             NULL
             );

            if(osfh == INVALID_HANDLE_VALUE) 
            {
                _DBGOUT(L"TRP008E  OPEN INPUT FILE FAILED, STATUS(%X)\n", GetLastError());
                return GetLastError();
            }

            __try
            {
                while( ReadFile(osfh, buffer, sizeof(buffer), &read, NULL))
                {
                    if(read == 0)
                    {
                        /*++ eof ... */
                        break;
                    }
                    if( WriteFile(pnp->io.parent_write, buffer, read, &bytes, NULL) == FALSE)
                    {
                        _DBGOUT(L"TRP009E  WRITE TO CHILD FAILED, STATUS(%X)\n", GetLastError());
                        return GetLastError();
                    }
                }
            }
            __finally
            {
                _SAFE_CLOSE(osfh);
            }
        }
        else
        {
            /*++ input buffer is data for child's stdin ... */
            if( WriteFile(pnp->io.parent_write, pnp->pinfo->input.buffer, pnp->pinfo->input.length, &bytes, NULL) == FALSE)
            {
                _DBGOUT(L"TRP010E  WRITE TO CHILD FAILED, STATUS(%X)\n", GetLastError());
                return GetLastError();
            }
        }
    }
    __finally
    {
        _SAFE_CLOSE(pnp->io.parent_write);
    }
    return ERROR_SUCCESS;
}

/*++
 */
static
BOOL
NTAPI
_TrapCreateChild (
    __in char* filename,
    __in char* cmdline,
    __inout PTRAP_PARAMS pparam )
{
    BOOL status;
    STARTUPINFO StartupInfo = {0};
    StartupInfo.cb = sizeof(STARTUPINFO);
    StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    StartupInfo.hStdInput  = pparam->io.childin;
    StartupInfo.hStdOutput = pparam->io.childout;
    StartupInfo.hStdError  = pparam->io.childerr;

    status = CreateProcess(
     filename,
     cmdline,
     NULL,
     NULL,
     TRUE,
     CREATE_NO_WINDOW,
     NULL,
     NULL,
     &StartupInfo,
     &(pparam->process)
     );

    if(status == FALSE)
    {
        _DBGOUT(L"TRP011E  CREATE PROCESS FAILED, STATUS(%X)\n", GetLastError());
        return FALSE;
    }

    if( CloseHandle(pparam->process.hThread) == FALSE)
    {
        _DBGOUT(L"TRP012W  CLOSE HANDLE(thread) FAILED, STATUS(%X)\n", GetLastError());
        /*++ no-fail ... */
    }
    return TRUE;
}

/*++
 */
static
BOOL
NTAPI
_TrapCreateHandles (
    __inout PTRAP_PARAMS pparam )
{
    /*++
     * there's a tiny dilema here: createpipe gives handles for both ends of a 
     * pipe. the handle for the child-end of the pipe needs to be inheritable
     * in order for the child to have access to it. the other, parent-end of 
     * the pipe needs to be UN-inheritable so that the parent is able to close
     * the handle after the child starts. createpipe allows inheritance to be
     * specified, but the same value is applied to both handles. in this code
     * the pipe handles are created inheritable and the parent-end handle is
     * modified afterwards to turn off inheritance.
     */

    /*++ null-sd, so all access allowed ... */
    SECURITY_ATTRIBUTES security_attributes = {0};
    security_attributes.nLength= sizeof(SECURITY_ATTRIBUTES);
    security_attributes.lpSecurityDescriptor = NULL;
    security_attributes.bInheritHandle = TRUE;

    /*++ do the child's stdout ... */
    if( CreatePipe(&(pparam->io.parent_read), &(pparam->io.childout), &security_attributes, 0) == FALSE)
    {
        _DBGOUT(L"TRP013E  CREATE PIPE(OUTPUT) FAILED, STATUS(%X)\n", GetLastError());
        return FALSE;
    }

    /*++ turn off inheritance for the parent-end handle ... */
    if( SetHandleInformation(pparam->io.parent_read, HANDLE_FLAG_INHERIT, 0) == FALSE)
    {
        _DBGOUT(L"TRP014E  SET HANDLE INFO FAILED, STATUS(%X)\n", GetLastError());
        return FALSE;
    }

    /*++ do the child's stderr ... */
    /*++
     * dup stdout to a new handle for stderr. if this is done:
     * 
     *   s_info.hStdInput  = pparam->io.childin;
     *   s_info.hStdOutput = pparam->io.childout;
     *   s_info.hStdError  = pparam->io.childout;
     * 
     * i.e., if the childout handle is used for both stdout and stderr, and
     * if the child program decides to close either it's stdout or stderr
     * handle, then it will close/break the pipe; it won't be able to write
     * and the parent won't be able to read.
     * 
     * by dup'ing the stdout handle, 
     * 
     *   s_info.hStdInput  = pparam->io.childin;
     *   s_info.hStdOutput = pparam->io.childout; - dup -\
     *   s_info.hStdError  = pparam->io.childerr; - <----/
     * 
     * there are distinct handles on stdout and stderr, albeit pointing at
     * the same pipe. the child app can close one of the handles without
     * tearing down the pipe.
     */
    HANDLE osph = GetCurrentProcess();
    if( DuplicateHandle(osph, pparam->io.childout, osph, &(pparam->io.childerr), 0, TRUE, DUPLICATE_SAME_ACCESS) == FALSE)
    {
        _DBGOUT(L"TRP015E  DUPE HANDLE(STDERR) FAILED, STATUS(%X)\n", GetLastError());
        return FALSE;
    }

    /*++ do the child's stdin ... */
    if( CreatePipe(&(pparam->io.childin), &(pparam->io.parent_write), &security_attributes, 0) == FALSE)
    {
        _DBGOUT(L"TRP016E  CREATE PIPE(STDIN) FAILED, STATUS(%X)\n", GetLastError());
        return FALSE;
    }

    /*++ turn off inheritance for the parent-end handle ... */
    if( SetHandleInformation(pparam->io.parent_write, HANDLE_FLAG_INHERIT, 0) == FALSE)
    {
        _DBGOUT(L"TRP017E  SET HANDLE INFO FAILED, STATUS(%X)\n", GetLastError());
        return FALSE;
    }
    return TRUE;
}

/*++ >>>>>>>>>>>>>>>>>>>>>>>           |           <<<<<<<<<<<<<<<<<<<<<<< --*/
/*++ >>>>>>>>>>>>>>>>>>>>>>>        support        <<<<<<<<<<<<<<<<<<<<<<< --*/

/*++
 */
void*
NTAPI
_TrapAlloc (
    __in size_t size )
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

/*++
 */
void*
NTAPI
_TrapRealloc (
    __in void* block,
    __in size_t size )
{
    if(block == NULL)
    {
        return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    }
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, block, size);
}

/*++
 */
int
NTAPI
_TrapFree (
    __in void* block )
{
    if(block == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    return ((HeapFree(GetProcessHeap(), 0, block) == 0) ? -1 : 0);
}

#define _MAX_DEBUG_BUFFER       1024    /*++ for the debugout functions ... */

/*++
 */
int
NTAPI
__TrapDbgoutVa (
    __in const wchar_t* format,
    __in va_list ap )
{
    int result;
    wchar_t buffer[_MAX_DEBUG_BUFFER] = { L'\0' };

    result = _vsnwprintf_s(buffer, _countof(buffer), _TRUNCATE, format, ap);
    if((result != STRUNCATE) && ((result < 0) || (result > _countof(buffer))))
    {
        OutputDebugStringW(L"CANNOT FORMAT MESSAGE\n");
        return -1;
    }

    OutputDebugStringW(buffer);
    return result;
}

/*++
 */
int
NTAPI
__TrapDbgout (
    __in const wchar_t* format,
    ... )
{
    int result;
    va_list va_args;

    va_start(va_args, format);
    result = __TrapDbgoutVa(format, va_args);
    va_end(va_args);
    return result;
}
