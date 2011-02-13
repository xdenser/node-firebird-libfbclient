/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */

#ifndef SRC_FB_BINDINGS_EVENTBLOCK_H_
#define SRC_FB_BINDINGS_EVENTBLOCK_H_

#include <ibase.h>
#include <node.h>
#include <v8.h>
#include "./fb-bindings.h"


#define MAX_EVENTS_PER_BLOCK	15

using namespace node;
using namespace v8;

/*
*	Class event_block 
*   	Firebird accepts events in blocks by 15 event names.
*	This class helps to organize linked list chain of such blocks
*	to support "infinite" number of events.
*/
typedef char* ev_names[MAX_EVENTS_PER_BLOCK];
static Persistent<String> fbevent_symbol;

class event_block {

public:
    Connection *conn;
    ISC_UCHAR *event_buffer;
    ISC_UCHAR *result_buffer;
    ISC_LONG event_id;
    ev_names event_names;
    int count;
    long blength;
    event_block *next, *prev;
    ev_async *event_;
    
    
    ISC_STATUS_ARRAY status;
    char err_message[MAX_ERR_MSG_LEN];
    isc_db_handle *db;
    bool traped, queued;
    
    static void Init();
    
#ifdef DEBUG    
    void dump_buf(char* buf, int len);
#endif    
    
    // calculates event counts
    // as difference between result_buffer and event_buffer
    // places result in Vector
    void get_counts(ISC_ULONG* Vector);
    
    // queue event_buffer to trap event
    bool queue();

    // cancel previously queued trap
    bool cancel();
    
    static void isc_ev_callback(void *aeb, ISC_USHORT length, const ISC_UCHAR *updated);

    // this is event nitification proc
    // here we emit node events 
    static void event_notification(EV_P_ ev_async *w, int revents);
    
    static Handle<Value> 
    que_event(event_block *eb);
    
    int hasEvent(char *Event);
    
    bool addEvent(char *Event);
    
    void removeEvent(char *Event);
    
    static Handle<Value> 
    RegEvent(event_block** rootp, char *Event, Connection *aconn, isc_db_handle *db);
    
    static event_block* FindBlock(event_block* root, char *Event);
    
    static Handle<Value> 
    RemoveEvent(event_block** root, char *Event);
    
    event_block(Connection *aconn,isc_db_handle *adb);
    
    ~event_block();
};

#endif
