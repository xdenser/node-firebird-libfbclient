/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */

#include <stdlib.h>
#include <string.h>
#include <v8.h>
#include <node.h>
#include <ibase.h>
#include <ctime> 
#include "./fb-bindings-fbresult.h"
 

 Persistent<FunctionTemplate> FBResult::constructor_template;
 
 void
   FBResult::Initialize (v8::Handle<v8::Object> target)
  {
    HandleScope scope;
    
    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    t->Inherit(FBEventEmitter::constructor_template);
    
    constructor_template = Persistent<FunctionTemplate>::New(t);
    constructor_template->Inherit(FBEventEmitter::constructor_template);
    constructor_template->SetClassName(String::NewSymbol("FBResult"));

    Local<ObjectTemplate> instance_template =
        constructor_template->InstanceTemplate();
        
    NODE_SET_PROTOTYPE_METHOD(t, "fetchSync", FetchSync);
    NODE_SET_PROTOTYPE_METHOD(t, "fetch", Fetch);


    instance_template->SetInternalFieldCount(1);
    
    Local<v8::ObjectTemplate> instance_t = t->InstanceTemplate();
    instance_t->SetAccessor(String::NewSymbol("inAsyncCall"),InAsyncGetter);
    
    target->Set(String::NewSymbol("FBResult"), t->GetFunction());
  }
  
 bool FBResult::process_result(XSQLDA **sqldap, isc_stmt_handle *stmtp, Local<Array> res)
  {
    int            fetch_stat;
    short 	   i, num_cols;	
    XSQLDA         *sqlda;
    
    uint32_t       j = 0;
    sqlda = *sqldap;
    if(!sqlda){ return true;}
    num_cols = (*sqldap)->sqld;
    
    HandleScope scope;
    
    Local<Object> js_result_row;
    Local<Value> js_field;
    
    while ((fetch_stat = isc_dsql_fetch(status, stmtp, SQL_DIALECT_V6, *sqldap)) == 0)
    {
        js_result_row = Array::New();
        for (i = 0; i < num_cols; i++)
        {
            js_field = FBResult::GetFieldValue((XSQLVAR *) &sqlda->sqlvar[i]);
            js_result_row->Set(Integer::NewFromUnsigned(i), js_field);
        }
        res->Set(Integer::NewFromUnsigned(j++), js_result_row);
    }
    return true;
  } 
  
  
  
  
Handle<Value>
  FBResult::New (const Arguments& args)
  {
  // XSQLDA **asqldap, isc_stmt_handle *astmtp
    HandleScope scope;
    
    REQ_EXT_ARG(0, js_sqldap);
    REQ_EXT_ARG(1, js_stmtp);
    
    XSQLDA *asqldap;
    asqldap = static_cast<XSQLDA *>(js_sqldap->Value());
    isc_stmt_handle *astmtp;
    astmtp = static_cast<isc_stmt_handle *>(js_stmtp->Value()); 
    
    FBResult *res = new FBResult(asqldap, astmtp);
    res->Wrap(args.This());

    return args.This();
  }
  
  bool FBResult::prepare_sqlda(XSQLDA *sqlda)
  {
    int i;
    XSQLVAR* var;
    for(i = 0, var = sqlda->sqlvar; i < sqlda->sqld;i++,var++)
    {
      switch(var->sqltype & ~1)
      {
        case SQL_ARRAY:
        case SQL_BLOB:      var->sqldata = (char*) new ISC_QUAD;
                            memset(var->sqldata, 0, sizeof(ISC_QUAD));
                            break;
        case SQL_TIMESTAMP: var->sqldata = (char*) new ISC_TIMESTAMP;
                            memset(var->sqldata, 0, sizeof(ISC_TIMESTAMP));
                            break;
        case SQL_TYPE_TIME: var->sqldata = (char*) new ISC_TIME;
                            memset(var->sqldata, 0, sizeof(ISC_TIME));
                            break;                                 
        case SQL_TYPE_DATE: var->sqldata = (char*) new ISC_DATE;
                            memset(var->sqldata, 0, sizeof(ISC_DATE));
                            break;                              
        case SQL_TEXT:      var->sqldata = new char[var->sqllen + 1];
                            memset(var->sqldata, ' ', var->sqllen);
                            var->sqldata[var->sqllen] = '\0';
                            break;
        case SQL_VARYING:   var->sqldata = new char[var->sqllen + 3];
                            memset(var->sqldata, 0, 2);
                            memset(var->sqldata + 2, ' ', var->sqllen);
                            var->sqldata[var->sqllen + 2] = '\0';
                            break;
        case SQL_SHORT:     var->sqldata = (char *) new int16_t(0); break;
        case SQL_LONG:      var->sqldata = (char *) new int32_t(0); break;
        case SQL_INT64:     var->sqldata = (char *) new int64_t(0); break;
        case SQL_FLOAT:     var->sqldata = (char *) new float(0.0); break;
        case SQL_DOUBLE:    var->sqldata = (char *) new double(0.0); break;
        default:            return false;                     
      }
      if(var->sqltype & 1) var->sqlind = new short(-1);
    }
    return true;
  }
  
  void FBResult::clean_sqlda(XSQLDA *sqlda)
  {
    int i;
    XSQLVAR* var;
    for(i = 0, var= sqlda->sqlvar; i < sqlda->sqld;i++,var++)
    {
      switch(var->sqltype & ~1)
      {
        case SQL_ARRAY:
        case SQL_BLOB:      delete (ISC_QUAD*) var->sqldata; break;
        case SQL_TIMESTAMP: delete (ISC_TIMESTAMP*) var->sqldata; break;
        case SQL_TYPE_TIME: delete (ISC_TIME*) var->sqldata; break;                                 
        case SQL_TYPE_DATE: delete (ISC_DATE*) var->sqldata; break;                              
        case SQL_TEXT:      
        case SQL_VARYING:   delete [] var->sqldata; break;
        case SQL_SHORT:     delete (int16_t *) var->sqldata; break;
        case SQL_LONG:      delete (int32_t *) var->sqldata; break;
        case SQL_INT64:     delete (int64_t *) var->sqldata; break;
        case SQL_FLOAT:     delete (float *) var->sqldata; break;
        case SQL_DOUBLE:    delete (double *) var->sqldata; break;
        default:            return;                     
      }
      if(var->sqlind != 0) delete var->sqlind;
    }
  }     
   
  double get_js_time(struct tm* times){
     double res = 0;
     
  }
  
Local<Value> 
  FBResult::GetFieldValue(XSQLVAR *var)
  {
    short       dtype;  
    PARAMVARY   *vary2;
    short       len; 
    struct tm   times;
    ISC_QUAD    bid;
    time_t      rawtime;
    double      time_val;
    int 	days; 
    
    
    HandleScope scope;
    
    Local<Function> con;
    Local<Value> argv[7];
    
    Local<Object> js_date;
    Local<Value> js_field = Local<Value>::New(Null());
    dtype = var->sqltype & ~1;
    if ((var->sqltype & 1) && (*var->sqlind < 0))
    {
     // NULL PROCESSING
    }
    else
    {
        switch (dtype)
        {
            case SQL_TEXT:
                //js_field = String::New(var->sqldata,var->sqllen);
                js_field = String::New(var->sqldata);
                break;

            case SQL_VARYING:
                vary2 = (PARAMVARY*) var->sqldata;
                vary2->vary_string[vary2->vary_length] = '\0';
                js_field = String::New((const char*)(vary2->vary_string));
                break;

            case SQL_SHORT:
            case SQL_LONG:
	    case SQL_INT64:
		{
		ISC_INT64	value;
		short		field_width;
		short		dscale;
		switch (dtype)
		    {
		    case SQL_SHORT:
			value = (ISC_INT64) *(short *) var->sqldata;
			field_width = 6;
			break;
		    case SQL_LONG:
			value = (ISC_INT64) *(int *) var->sqldata;
			field_width = 11;
			break;
		    case SQL_INT64:
			value = (ISC_INT64) *(ISC_INT64 *) var->sqldata;
			field_width = 21;
			break;
		    }
		ISC_INT64	tens;    
		short	i;
		dscale = var->sqlscale;
		
		if (dscale < 0)
		    {
		    tens = 1;
		    for (i = 0; i > dscale; i--) tens *= 10;
                    js_field = Number::New((double) value / tens); 
                    
		    }
		else if (dscale)
		      {
		       tens = 1;
		       for (i = 0; i < dscale; i++) tens *= 10;
		       js_field = Integer::New(value * tens); 
	              }		    
		else
		      js_field = Integer::New( value);
		}
                break;

            case SQL_FLOAT:
                js_field = Number::New(double( *(float *) (var->sqldata) ));  
                break;

            case SQL_DOUBLE:
                js_field = Number::New(*(double *) (var->sqldata));
                break;

            case SQL_TIMESTAMP: 
	            isc_decode_timestamp((ISC_TIMESTAMP *)var->sqldata, &times);
	            js_date = Date::New(0)->ToObject();
	            argv[0] = Integer::New(times.tm_year+1900);
	            Local<Function>::Cast(js_date->Get( String::New("setFullYear") ))->Call(js_date,1,argv);
	            argv[0] = Integer::New(times.tm_mon);
	            Local<Function>::Cast(js_date->Get( String::New("setMonth") ))->Call(js_date,1,argv);
	            argv[0] = Integer::New(times.tm_mday);
	            Local<Function>::Cast(js_date->Get( String::New("setDate") ))->Call(js_date,1,argv);
	            
	            argv[0] = Integer::New(times.tm_hour);
	            Local<Function>::Cast(js_date->Get( String::New("setHours") ))->Call(js_date,1,argv);
	            argv[0] = Integer::New(times.tm_min);
	            Local<Function>::Cast(js_date->Get( String::New("setMinutes") ))->Call(js_date,1,argv);
	            argv[0] = Integer::New(times.tm_sec);
	            Local<Function>::Cast(js_date->Get( String::New("setSeconds") ))->Call(js_date,1,argv);
	            argv[0] = Integer::New(  (( ((ISC_TIMESTAMP *)var->sqldata)->timestamp_time) % 10000) / 10);
	            Local<Function>::Cast(js_date->Get( String::New("setMilliseconds") ))->Call(js_date,1,argv);
	            
	            js_field = js_date;
	            break;
	            
            case SQL_TYPE_DATE:    
	            	          
	            isc_decode_sql_date((ISC_DATE *)var->sqldata, &times);
	            
	            // days from 1 jan 1858 	            
	            // days = * (int *) var->sqldata ; 
	            
	            js_date = Date::New(0)->ToObject();
	            
	            argv[0] = Integer::New(times.tm_year+1900);
	            Local<Function>::Cast(js_date->Get( String::New("setFullYear") ))->Call(js_date,1,argv);
	            argv[0] = Integer::New(times.tm_mon);
	            Local<Function>::Cast(js_date->Get( String::New("setMonth") ))->Call(js_date,1,argv);
	            argv[0] = Integer::New(times.tm_mday);
	            Local<Function>::Cast(js_date->Get( String::New("setDate") ))->Call(js_date,1,argv);
	            argv[0] = Integer::New(0);
	            Local<Function>::Cast(js_date->Get( String::New("setHours") ))->Call(js_date,1,argv);
	            Local<Function>::Cast(js_date->Get( String::New("setMinutes") ))->Call(js_date,1,argv);
	            Local<Function>::Cast(js_date->Get( String::New("setSeconds") ))->Call(js_date,1,argv);
	            Local<Function>::Cast(js_date->Get( String::New("setMilliseconds") ))->Call(js_date,1,argv);
	            
	            //js_field = con->Call(js_date, 3, argv);
	            /*js_date = Object::New();
	            js_date->Set(String::New("year"),
                             Integer::New(times.tm_year+1900));
                    js_date->Set(String::New("month"),
                             Integer::New(times.tm_mon+1));
                    js_date->Set(String::New("day"),
                             Integer::New(times.tm_mday));
                             
	            time_val = (static_cast<double>(days) - 40587.0) * 86400 * 1000;
	            js_date->Set(String::New("date"),
                             Date::New(time_val));*/
	            js_field = js_date;
	            break;
	            
           case SQL_TYPE_TIME:    
	            isc_decode_sql_time((ISC_TIME *)var->sqldata, &times);
	            
	            js_date = Date::New(0)->ToObject();
	            argv[0] = Integer::New(times.tm_year+1900);
	            Local<Function>::Cast(js_date->Get( String::New("setFullYear") ))->Call(js_date,1,argv);
	            argv[0] = Integer::New(times.tm_mon);
	            Local<Function>::Cast(js_date->Get( String::New("setMonth") ))->Call(js_date,1,argv);
	            argv[0] = Integer::New(times.tm_mday);
	            Local<Function>::Cast(js_date->Get( String::New("setDate") ))->Call(js_date,1,argv);
	            
	            argv[0] = Integer::New(times.tm_hour);
	            Local<Function>::Cast(js_date->Get( String::New("setHours") ))->Call(js_date,1,argv);
	            argv[0] = Integer::New(times.tm_min);
	            Local<Function>::Cast(js_date->Get( String::New("setMinutes") ))->Call(js_date,1,argv);
	            argv[0] = Integer::New(times.tm_sec);
	            Local<Function>::Cast(js_date->Get( String::New("setSeconds") ))->Call(js_date,1,argv);
	            argv[0] = Integer::New(  (( ((ISC_TIMESTAMP *)var->sqldata)->timestamp_time) % 10000) / 10);
	            Local<Function>::Cast(js_date->Get( String::New("setMilliseconds") ))->Call(js_date,1,argv);
	            
	            js_field = js_date;
	            break;            

            case SQL_BLOB:
            case SQL_ARRAY:
                /* Print the blob id on blobs or arrays */
                Local<Object> js_blob;
                bid = *(ISC_QUAD *) var->sqldata;
                js_blob = Object::New();
                js_blob->Set(String::New("q_hi"),
                             Integer::New(bid.gds_quad_high));
                js_blob->Set(String::New("q_lo"),
                             Integer::NewFromUnsigned(bid.gds_quad_low));             

                js_field = js_blob;
                
                break;

        }

    }
    
    return scope.Close(js_field);
  }

Handle<Value>
  FBResult::FetchSync(const Arguments& args) 
  {
  
    HandleScope scope;
    
    FBResult *fb_res = ObjectWrap::Unwrap<FBResult>(args.This());
    
    int            fetch_stat;
    short 	   i, num_cols;	
    XSQLDA         *sqlda;
    
    uint32_t       j = 0;
    sqlda = fb_res->sqldap;
    if(!sqlda){ return Undefined();}
    num_cols = sqlda->sqld;
    
    Local<Value> js_result = Local<Value>::New(Null());
    
    if (args.Length() < 2){
       return ThrowException(Exception::Error(
            String::New("Expecting 2 arguments")));
    }
    
    int rowCount = -1;
    if(args[0]->IsInt32()){
       rowCount = args[0]->IntegerValue();
    }
    else if(! (args[0]->IsString() && args[0]->Equals(String::New("all")))){
       return ThrowException(Exception::Error(
            String::New("Expecting integer or string as first argument")));
    };
    if(rowCount<=0) rowCount = -1;
    
    bool rowAsObject = false;
    if(args[1]->IsBoolean()){
         rowAsObject = args[1]->BooleanValue();
    }else if(args[1]->IsString() && args[1]->Equals(String::New("array"))){
         rowAsObject = false;
    }else if(args[1]->IsString() && args[1]->Equals(String::New("object"))){
         rowAsObject = true;
    } else{
     return ThrowException(Exception::Error(
            String::New("Expecting bool or string('array'|'object') as second argument")));
    };   
    
    Local<Value> js_field;
    Local<Object> js_result_row;   
        
    Local<Array> res = Array::New(); 
    while (((fetch_stat = isc_dsql_fetch(fb_res->status, &fb_res->stmt, SQL_DIALECT_V6, sqlda)) == 0)&&((rowCount==-1)||(rowCount>0)))
    {
        //js_result_row = Array::New();
         if(rowAsObject)
            js_result_row = Object::New();
         else 
            js_result_row = Array::New();
        
        for (i = 0; i < num_cols; i++)
        {
            js_field = FBResult::GetFieldValue((XSQLVAR *) &(sqlda->sqlvar[i]));
            if(rowAsObject)
            { 
              js_result_row->Set(String::New(sqlda->sqlvar[i].sqlname), js_field);
            }
            else
            js_result_row->Set(Integer::NewFromUnsigned(i), js_field);
        }
        res->Set(Integer::NewFromUnsigned(j++), js_result_row);
        if(rowCount>0) rowCount--;
    }
    
    if ((fetch_stat != 100L) && fetch_stat) 
          return ThrowException(Exception::Error(
            String::Concat(String::New("While FetchSync - "),ERR_MSG(fb_res,FBResult))));

    
//    if(j==1) js_result = res->Get(0);
//    else 
    js_result = res;
    
    return scope.Close(js_result);

  }

int FBResult::EIO_After_Fetch(eio_req *req)
  {
    ev_unref(EV_DEFAULT_UC);
    HandleScope scope;
    struct fetch_request *f_req = (struct fetch_request *)(req->data);
    short i, num_cols;
    num_cols = f_req->res->sqldap->sqld;
    
    Local<Value> js_field;
    Local<Object> js_result_row;   
    Local<Value> argv[3];
    int argc = 0;
    
    if(req->result)
    {
        if(f_req->rowCount>0) f_req->rowCount--;  
        
	if(f_req->rowAsObject)
    	    js_result_row = Object::New();
	else 
    	    js_result_row = Array::New();
    
	for (i = 0; i < num_cols; i++)
	{
    	    js_field = FBResult::GetFieldValue((XSQLVAR *) &f_req->res->sqldap->sqlvar[i]);
    	    if(f_req->rowAsObject)
    	    { 
        	js_result_row->Set(String::New(f_req->res->sqldap->sqlvar[i].sqlname), js_field);
    	    }
    	    else
        	js_result_row->Set(Integer::NewFromUnsigned(i), js_field);
        }
        
        if(f_req->rowCallback->IsFunction()){
    	    argv[0] = js_result_row;
        
	    TryCatch try_catch;

	    Local<Value> ret = Persistent<Function>::Cast(f_req->rowCallback)->Call(Context::GetCurrent()->Global(), 1, argv);

	    if (try_catch.HasCaught()) {
    		node::FatalException(try_catch);
	    }
	    else
	    if((!ret->IsBoolean() || ret->BooleanValue())&&f_req->rowCount!=0)
	    {
		eio_custom(EIO_Fetch, EIO_PRI_DEFAULT, EIO_After_Fetch, f_req);
        	ev_ref(EV_DEFAULT_UC);
        	return 0;
	    }
	    else
	    {
	      argc = 2;
	      argv[0] = Local<Value>::New(Null());
	      argv[1] = Local<Value>::New(False());
	    }
	}
	else
	{ 
	 /* TODO: 
	 *   accumulate result here 
	 */
	}
    }
    else 
    if(f_req->fetchStat!=100L){
          argc = 1;
          argv[0] = Exception::Error(
            String::Concat(String::New("While connecting - "),ERR_MSG(f_req->res,FBResult)));
    }
    else
    {
          argc = 2;
          argv[0] = Local<Value>::New(Null());
          argv[1] = Local<Value>::New(True());
    }

   
    TryCatch try_catch;
    f_req->eofCallback->Call(Context::GetCurrent()->Global(), argc, argv);

    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }
    

    f_req->rowCallback.Dispose();
    f_req->eofCallback.Dispose();
    f_req->res->stop_async();
    f_req->res->Unref();
    free(f_req);

    return 0;
  } 

int FBResult::EIO_Fetch(eio_req *req)
  {
    struct fetch_request *f_req = (struct fetch_request *)(req->data);
    
    f_req->fetchStat = isc_dsql_fetch(f_req->res->status, &f_req->res->stmt, SQL_DIALECT_V6, f_req->res->sqldap);
    
    req->result = (f_req->fetchStat == 0);

    return 0;
  }
  
Handle<Value>
  FBResult::Fetch(const Arguments& args) 
  {
    HandleScope scope;
    FBResult *res = ObjectWrap::Unwrap<FBResult>(args.This());
    
    struct fetch_request *f_req =
         (struct fetch_request *)calloc(1, sizeof(struct fetch_request));

    if (!f_req) {
      V8::LowMemoryNotification();
      return ThrowException(Exception::Error(
            String::New("Could not allocate memory.")));
    }
    
    if (args.Length() != 4){
       return ThrowException(Exception::Error(
            String::New("Expecting 4 arguments")));
    }
    
    f_req->rowCount = -1;
    if(args[0]->IsInt32()){
       f_req->rowCount = args[0]->IntegerValue();
    }
    else if(! (args[0]->IsString() && args[0]->Equals(String::New("all")))){
       return ThrowException(Exception::Error(
            String::New("Expecting integer or string as first argument")));
    };
    if(f_req->rowCount<=0) f_req->rowCount = -1;
    
    f_req->rowAsObject = false;
    if(args[1]->IsBoolean()){
         f_req->rowAsObject = args[1]->BooleanValue();
    }else if(args[1]->IsString() && args[1]->Equals(String::New("array"))){
         f_req->rowAsObject = false;
    }else if(args[1]->IsString() && args[1]->Equals(String::New("object"))){
         f_req->rowAsObject = true;
    } else{
     return ThrowException(Exception::Error(
            String::New("Expecting bool or string('array'|'object') as second argument")));
    };
    
/*    if(!args[2]->IsFunction())  f_req->rowCallback = Persistent<Value>::New(Null());
    else  f_req->rowCallback = Persistent<Function>::New(Local<Function>::Cast(args[2]));
*/   
    f_req->rowCallback  = Persistent<Value>::New(args[2]);
    
    if(!args[3]->IsFunction()) {
      return ThrowException(Exception::Error(
            String::New("Expecting Function as fourth argument")));
    }

    
    f_req->res = res;
    f_req->eofCallback = Persistent<Function>::New(Local<Function>::Cast(args[3]));
    
    res->start_async();
    eio_custom(EIO_Fetch, EIO_PRI_DEFAULT, EIO_After_Fetch, f_req);
    
    ev_ref(EV_DEFAULT_UC);
    res->Ref();
    
    return Undefined();
  }

  
 FBResult::FBResult (XSQLDA *asqldap, isc_stmt_handle *astmtp) : FBEventEmitter () 
  {
    sqldap = asqldap;
    stmt = *astmtp;
  }
  
 FBResult::~FBResult()
  {
   if(sqldap) {
     FBResult::clean_sqlda(sqldap);
     free(sqldap);
   }
   
  }
  
