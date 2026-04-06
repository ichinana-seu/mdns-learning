// Command+Shift+B to build in vscode
#include <stdio.h>
#include <string.h>
#include "common_head.h"
#define TypeBufferSize 80

typedef uint32_t DNSServiceFlags;
typedef uint32_t DNSServiceProtocol;
typedef int32_t DNSServiceErrorType;

typedef struct _DNSServiceRef_t *DNSServiceRef;             //// 原来的写法（繁琐）struct _DNSServiceRef_t *ref;     // 现在的写法（简洁）DNSServiceRef ref;
typedef struct _DNSServiceRef_t DNSServiceOp;               // 完全不会嵌套死循环！这是 C 语言的标准合法用法（自引用结构体）
typedef struct _DNSRecordRef_t *DNSRecordRef;
typedef struct _DNSRecordRef_t DNSRecord;


typedef struct DNSServiceAttribute_s DNSServiceAttribute;
typedef DNSServiceAttribute *DNSServiceAttributeRef;

#if defined(_WIN32)
#include <winsock2.h>
typedef SOCKET dnssd_sock_t;
#else
typedef int dnssd_sock_t;
#endif

typedef packedunion
{
    void *context;
    uint32_t u32[2];
} client_context_t;

typedef void (DNSSD_API *DNSServiceBrowseReply)
(
    DNSServiceRef sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    DNSServiceErrorType errorCode,
    const char *serviceName,
    const char *regtype,
    const char *replyDomain,
    void *context
);

struct _DNSServiceRef_t
{
    DNSServiceOp     *next;             // When sharing a connection, this is the next subordinate DNSServiceRef in
                                        // the list. The connection being shared is the first in the list.
    DNSServiceOp     *primary;          // When sharing a connection, the primary pointer of each subordinate
                                        // DNSServiceRef points to the head of the list. If primary is null, and next is
                                        // null, this is not a shared connection. If primary is null and next is
                                        // non-null, this is the primary DNSServiceRef of the shared connection. If
                                        // primary is non-null, this is a subordinate DNSServiceRef for the connection
                                        // that is managed by the DNSServiceRef pointed to by primary.
    dnssd_sock_t sockfd;                // Connected socket between client and daemon
    dnssd_sock_t validator;             // Used to detect memory corruption, double disposals, etc.
    client_context_t uid;               // For shared connection requests, each subordinate DNSServiceRef has its own ID,
                                        // unique within the scope of the same shared parent DNSServiceRef. On the
                                        // primary DNSServiceRef, uid matches the uid of the most recently allocated
                                        // subordinate DNSServiceRef. Each time a new subordinate DNSServiceRef is
                                        // allocated, uid on the primary is incremented by one and copied to the
                                        // subordinate.
    uint32_t op;                        // request_op_t or reply_op_t
    uint32_t max_index;                 // Largest assigned record index - 0 if no additional records registered
    uint32_t logcounter;                // Counter used to control number of syslog messages we write
    int              *moreptr;          // Set while DNSServiceProcessResult working on this particular DNSServiceRef
    ProcessReplyFn ProcessReply;        // Function pointer to the code to handle received messages
    void             *AppCallback;      // Client callback function and context
    void             *AppContext;
    DNSRecord        *rec;
#if _DNS_SD_LIBDISPATCH
    dispatch_source_t disp_source;
    dispatch_queue_t disp_queue;
#endif
    void             *kacontext;
};





static char *gettype(char *buffer, char *typ)
{
    // 1. 处理无效输入：空指针/空字符串/单个点 → 强制使用默认服务类型 _http._tcp
    if (!typ || !*typ || (typ[0] == '.' && typ[1] == 0)) typ = "_http._tcp";
    // 2. 处理简写：如果服务类型没有小数点（如 _http），自动拼接 ._tcp
    if (!strchr(typ, '.')) { 
        snprintf(buffer, TypeBufferSize, "%s._tcp", typ); 
        typ = buffer; 
    }
    return(typ);
}

int print_help_info(){
    printf("  |    Help: mdnsb <servicename> <domain>\n");
    printf("  | Example: mdnsb _airplay local.\n");
    printf("  | Example: mdnsb _airplay local.\n");
    printf("  | Example: mdnsb _airplay._tcp local.\n");
    printf("  | Example: mdnsb _http local.\n");
    printf("  | Example: mdnsb _http._tcp local.\n");
    printf("  | Example: mdnsb _ipp local.\n");
    printf("  | Example: mdnsb _ipp._tcp local.\n");
    return 0;
}



DNSServiceErrorType DNSServiceBrowse
(
    DNSServiceRef                       *sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    const char                          *regtype,
    const char                          *domain,    /* may be NULL */
    DNSServiceBrowseReply callback,
    void                                *context    /* may be NULL */
)
{
    mStatus err = mStatus_NoError;
    const char *errormsg;
    domainname t, d;
    mDNS_DirectOP_Browse *x;
    (void)flags;            // Unused
    (void)interfaceIndex;   // Unused

    // Check parameters
    if (!regtype[0] || !MakeDomainNameFromDNSNameString(&t, regtype))      { errormsg = "Illegal regtype"; goto badparam; }
    if (!MakeDomainNameFromDNSNameString(&d, *domain ? domain : "local.")) { errormsg = "Illegal domain";  goto badparam; }

    // Allocate memory, and handle failure
    x = (mDNS_DirectOP_Browse *) mDNSPlatformMemAllocateClear(sizeof(*x));
    if (!x) { err = mStatus_NoMemoryErr; errormsg = "No memory"; goto fail; }

    // Set up object
    x->disposefn = DNSServiceBrowseDispose;
    x->callback  = callback;
    x->context   = context;
    x->q.QuestionContext = x;

    // Do the operation
    err = mDNS_StartBrowse(&mDNSStorage, &x->q, &t, &d, mDNSInterface_Any, flags, (flags & kDNSServiceFlagsForceMulticast) != 0, (flags & kDNSServiceFlagsBackgroundTrafficClass) != 0, FoundInstance, x);
    if (err) { mDNSPlatformMemFree(x); errormsg = "mDNS_StartBrowse"; goto fail; }

    // Succeeded: Wrap up and return
    *sdRef = (DNSServiceRef)x;
    return(mStatus_NoError);

badparam:
    err = mStatus_BadParamErr;
fail:
    LogMsg("DNSServiceBrowse(\"%s\", \"%s\") failed: %s (%ld)", regtype, domain, errormsg, err);
    return(err);
}



int main(int argc, char **argv){
    char buffer[TypeBufferSize], *typ, *dom;
    DNSServiceErrorType err;
    // Print Help Information
    printf(" (mdnsb) mDNS Service Discovery Tools\n");
    if(argc==1){
        print_help_info();
        return 0;
    }
    if( strcmp(argv[1], "-h")==0 || strcmp(argv[1], "--help")==0 ){
        print_help_info();
        return 0;
    }
    // Error process
    if(argc<3){
        printf("[ERROR] At least 2 arguments is required.\n");
        return -1;
    }
    // Normal Process
    typ = argv[1];
    dom = argv[2];
    if (dom[0] == '.' && dom[1] == 0) dom[0] = 0;               // 空域名。 We allow '.' on the command line as a synonym for empty string
    typ = gettype(buffer, typ);
    printf("  [INFO] Browsing for %s%s%s\n", typ, dom[0] ? "." : "", dom);    // 非空域名需要在服务名和域名中间加点。空域名不需要在结尾加上点。
    err = DNSServiceBrowse(&client, flags, opinterface, typ, dom, browse_reply, NULL);
    //
    //
    return 0;
}
