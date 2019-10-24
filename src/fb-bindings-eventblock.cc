/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */
#define BUILDING_NODE_EXTENSION 1
#include <stdlib.h>
#include <string.h>
#include "./fb-bindings-fbeventemitter.h"
#include "./fb-bindings-eventblock.h"

/*
*	Class event_block 
*   	Firebird accepts events in blocks by 15 event names.
*	This class helps to organize linked list chain of such blocks
*	to support "infinite" number of events.
*/

void event_block::Init()
{
  fbevent_symbol.Reset(Nan::New<String>("fbevent").ToLocalChecked());
}

#ifdef DEBUG    
void event_block::dump_buf(char* buf, int len){
      printf("buff dump %d: ",len);
      //printf("buff[0] = %d\n", buf[0]);
	  printf("%d : [", buf[0]);
      buf++; len--;
      int c;
      char curr_name[127];
      char* cn;
      while(len > 0){
        c = (unsigned char) buf[0];
        //printf("name len = %d\n",c);
		printf("%d\"", c);
        buf++;len--;
        for(cn = curr_name;c--;len--){
          *cn++ = *buf++;
        }
        *cn++ = 0;
        //printf("event_name = %s\n",curr_name);
		printf("%s\": ", curr_name);
        //printf("count = %d\n",(uint32_t) *buf);
		printf("%d, ", (uint32_t)*buf);
        len = len - sizeof(uint32_t);
        buf = buf + sizeof(uint32_t);
      }
	  printf("]\n");
    }
#endif    
    
    // calculates event counts
    // as difference between result_buffer and event_buffer
    // places result in Vector
void event_block::get_counts(ISC_STATUS* Vector){
       char *eb, *rb;
       long len = blength;
	   /*
	   printf("result_buffer: ");
	   dump_buf((char*)result_buffer, blength);
	   printf("event_buffer: ");
	   dump_buf((char*)event_buffer, blength);
	   */
       eb = (char*) event_buffer;
       rb = (char*) result_buffer;
       int idx = 0;
       eb++;rb++;len--;
       int c;
       int32_t vold;
       int32_t vnew;
       while(len){
         c = (unsigned char) rb[0];
         eb =   eb  + c + 1;
         rb =   rb  + c + 1;
         len =  len - c - 1;
         len = len - sizeof(uint32_t);
         
         vnew = (int32_t) *rb;
         vold = (int32_t) *eb;
		 //printf("o:%d,n:%d,v:%d\n", vold, vnew, (int) Vector[idx]);
         eb = eb + 4;
         rb = rb + 4;
		 if (vold == -1 && vnew > 0)
		 {
			 vnew--;
			 Vector[idx] = vnew;
		 }
		 else
	     if(vnew > vold){
           Vector[idx] = (vnew - vold);
         }
         else Vector[idx] = 0;
		// printf("o:%d,n:%d,v:%d\n", vold, vnew, (int) Vector[idx]);
         idx++;
       }
       memcpy(event_buffer,result_buffer,blength);
    }
    
    // queue event_buffer to trap event
bool event_block::queue()
    {
      if(!queued)
      {
        traped = false;
        queued = true;
        
        if(isc_que_events(
         status,
         db,
         &event_id,
         blength,
         event_buffer,
         event_block::isc_ev_callback,
         (char*)this))
        {  
          queued = false;
          event_id = 0;
          return false;
        } 
       }    
      return true; 
    }

    // cancel previously queued trap
bool event_block::cancel() 
    {
      if(queued)
      {
       traped = false;
       queued = false;  
       //printf("isc_cancel_events \n");
       if(isc_cancel_events(status, db, &event_id)) return false; 
       event_id = 0;
      } 
      return true;
    }
    
void event_block::isc_ev_callback(void *aeb, ISC_USHORT length, const ISC_UCHAR *updated)
    {
      // this callback is called by libfbclient when event occures in FB
      // have not found that in documentation
      // but on isc_cancel_events this callback is called
      // with updated == NULL, length==0
      // here we have potential race condition 
      // when cancel and callback get called simultanously .. hmm..
      if( updated == 0 || aeb == 0 || length == 0) return;
      
      event_block* eb = static_cast<event_block*>(aeb);
      
      if(eb->queued)
      {
        // copy updated to result buffer
        // all examples use loop
        // why not to use mmcpy ???
	     
	      
        if((ISC_USHORT) eb->blength < length) length = (ISC_USHORT) eb->blength;
        ISC_UCHAR *r = eb->result_buffer;
        while(length--) *r++ = *updated++;
        
       // dump_buf((char*) eb->result_buffer, eb->blength);
        
        eb->traped = true;
	    eb->queued = false;
      
        // schedule event_notification call
        uv_async_send(eb->event_);
      }    
    }
    // this is event nitification proc
    // here we emit node events 
void event_block::event_notification _UV_NOTIFICATION_SIGNATURE {
        Nan::HandleScope scope;
        ISC_STATUS_ARRAY Vector;
	    Local<Value> argv[2];
	
        event_block* eb = static_cast<event_block*>(w->data);
        if(eb->count==0) return;
        
        
        
        // get event counts
        // in ibpp they check if event was traped
        // we should not as if this proc was called - the event was trapped for sure
        // get_counts takes care about first (intialization callback call)
        // this is handled with proper buffer initialization with uint32_t(-1)
        eb->get_counts((ISC_STATUS*) Vector);
	
	    int count = eb->count;
	    int i;
	    Local<Value> EvNames[MAX_EVENTS_PER_BLOCK];
        
        bool emit_events = false;
        // collect event names to emit
        // do not emit here as in event callback 
        // we may add or remove events
        // or even close a connection
        // so collect them, finish with firebird calls
        // and then emit
	for(i=0; i < count; i++){
	  if(Vector[i]){
	   EvNames[i] = Nan::New<String>(eb->event_names[i]).ToLocalChecked();
	   emit_events = true;
	  } 
	}
	// requeue events
	if(!eb->queue()) {
            Nan::ThrowError(
            String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("While isc_que_events in event_notification - ").ToLocalChecked(),ERR_MSG(eb, event_block)));
        }			      
        
        // exit if block was marked as bad
        
        if(!emit_events) return;
	Connection *conn = eb->conn;

        // emit events 
	for( i=0; i < count; i++){
	   if(Vector[i]) {
	     argv[0] = EvNames[i];
	     argv[1] = Nan::New<Integer>(uint32_t(Vector[i]));
             ((FBEventEmitter*) conn)->Emit(Nan::New(fbevent_symbol),2,argv);
           }     
        }

    }
    
    
void
    event_block::que_event(event_block *eb)
    { 
      Nan::HandleScope scope;
      if(!eb->queue()) {
            Nan::ThrowError(
            String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("While isc_que_events - ").ToLocalChecked(),ERR_MSG(eb, event_block)));
            return ;
      }
    }
    
int event_block::hasEvent(char *Event)
    {
      int i;
      for(i=0;i<count;i++){
       if(event_names[i]&&(strcmp(event_names[i],Event)==0)) return i;
      } 
      return -1;
    }
    
bool event_block::addEvent(char *Event)
    {
      // allocate memory for event name
      size_t len = strlen(Event);
	  if (len > 255) {
		  len = 255;
	  }
      event_names[count] = (char*) calloc(1,len+1);
      if(!event_names[count]) return false;
      
      
      // copy event name
      char *p, *t;
      t = event_names[count];
      for(p=Event;(*p);){ *t++ = *p++; }
      *t = 0;
      
      count++;
      // (re)allocate and fill buffer new event
      size_t prev_size = blength;
      size_t needed = ( (prev_size == 0) ? 1 : 0 ) + len + 5;
      event_buffer = (ISC_UCHAR*) realloc(event_buffer, prev_size + needed);
      if(!event_buffer) return false;
      result_buffer = (ISC_UCHAR*) realloc(result_buffer, prev_size + needed);
      if(!result_buffer) return false;
      if(prev_size == 0){
        event_buffer[0] = result_buffer[0] = 1;
      }
      char* buf = (char*) event_buffer + ( (prev_size == 0) ? 1 : prev_size );
      *(buf++) = static_cast<char>(len);
      for(p=Event;*p;){ *buf++ = *p++; };
      int *pi = (int*) buf;
      *pi = -1;
      //(int)(*(buf++)) = -1;// *(buf++) = -1; *(buf++) = -1; *buf = -1;
	  size_t needed_size = prev_size + needed;
	  if (needed_size > SHRT_MAX) {
		  return false;
	  }
	  blength = (short) needed_size;
      memcpy(result_buffer+prev_size,event_buffer+prev_size,needed);
      
      return true;
    }
    
void event_block::removeEvent(char *Event)
    { 
      int idx = hasEvent(Event); 
      if(idx<0) return;
      free(event_names[idx]);
      for(int i = idx;i < count;i++) event_names[i] = event_names[i+1];
      count--;
      
      // realocate buffers
      char *buf = (char*) event_buffer + 1;
      char *rb  = (char*) result_buffer + 1; 
      while(idx)
      { 
        buf = buf + (*(buf)) + 4; buf++;
        rb  = rb + (*(rb)) + 4; rb++;
        idx--;
      }
      char sz = (*buf) + 5;
      size_t new_length = blength - sz;
      size_t tale_length = blength - (buf - (char*)event_buffer) - sz;    
      if(tale_length){
        memmove(buf, buf+sz, tale_length);
        memmove(rb, rb+sz, tale_length);
      }    
      if(new_length==1) new_length = 0;
      
      event_buffer = (ISC_UCHAR*) realloc(event_buffer, new_length); 
      result_buffer = (ISC_UCHAR*) realloc(result_buffer, new_length);             
      blength = (short) new_length;
          
    }
    
void
    event_block::RegEvent(event_block** rootp, char *Event, Connection *aconn, isc_db_handle *db)
    {
      Nan::HandleScope scope;
	  event_block* root = *rootp;
      // Check if we already have registered that event
      event_block* res = event_block::FindBlock(root,Event);
      // Exit if yes
      if(res) return;
  
      // Search for available event block
      res = root;
      while(res)
      {
        if(res->count < MAX_EVENTS_PER_BLOCK) break;
        root = res;
        res = res->next;
      }
      // Have we found one ?
      if(!res){
        // Create new event block
		//printf("create new event block\n");
        res = new event_block(aconn, db);
        if(!res){
        	Nan::LowMemoryNotification();
    	    Nan::ThrowError("Could not allocate memory.");
    	    return ;
	}
        //uv_unref((uv_handle_t*) res->event_);
       // uv_async_start(EV_DEFAULT_UC_ res->event_);
      //  uv_unref(uv_default_loop());
        
        // Set links
        if(root) root->next = res;
        res->prev = root;
      }
      
      // Set first element in chain if it is not set yet
      if(!*rootp) *rootp = res;
      
      
      // Cancel old queue if any 
      if(!res->cancel()){
    	    Nan::ThrowError(
        	String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("While cancel_events - ").ToLocalChecked(),ERR_MSG(res, event_block)));
    	    return;
      }        

      // Add event to block
      if(!res->addEvent(Event)) {
    	    Nan::LowMemoryNotification();
    	    Nan::ThrowError("Could not allocate memory.");
    	    return ;
      }

      event_block::que_event(res);

    }
    
event_block* event_block::FindBlock(event_block* root, char *Event)
    {
      // Find block with Event in linked list
      event_block* res = root;
      while(res)
       if(res->hasEvent(Event)==-1) res = res->next;
       else break;
      return res;
    }
    
void 
    event_block::RemoveEvent(event_block** root, char *Event)
    {
      Nan::HandleScope scope;
	  // Find event_block with Event name
      event_block* eb = event_block::FindBlock( *root, Event);
      if(eb)
      {
        // If we have found it
        // it was queued and should be canceled
        if(!eb->cancel()){
        	Nan::ThrowError(
        	        	    String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("While cancel_events - ").ToLocalChecked(),ERR_MSG(eb, event_block)));
    	    return ;
    	}        
    	// Remove it from event list
        eb->removeEvent(Event);
///        if(eb->lock) return Undefined();
        if(!eb->count) {
         // If there is no more events in block
         // we can free it
         // but keep link to next block
         if(eb->prev) 
         {
          eb->prev->next = eb->next;
         }
         else
         {
          *root = NULL;
         }
         eb->next = NULL;
         delete eb;
        } 
        else {
            // if block still has events we should requeue it
    	    return event_block::que_event(eb);    
        }
      }	
      // No block with that name 
      // may be throw error ???
      // return;
    }

    
    event_block::event_block(Connection *aconn,isc_db_handle *adb)
    { 
      conn = aconn;
      db = adb;
      count = 0;
      next = NULL;
      prev = NULL;
      event_buffer = NULL;
      result_buffer = NULL;
      blength = 0;
      event_id = 0;
      event_ = new uv_async_t();
	  event_->data = this;
	  uv_async_init(uv_default_loop(), event_, event_block::event_notification);
	        
      queued = false;
      traped = false;
    }

	void close_cb(uv_handle_t* handle) {
		uv_async_t* event_ = (uv_async_t*)handle;
		delete event_;
    }

event_block::~event_block()
    {
      cancel();
      for(int i=0;i<count;i++) free(event_names[i]);
      free(event_buffer);
      free(result_buffer);
	  uv_close((uv_handle_t*)event_, close_cb);
      if(next) delete next;
    }

