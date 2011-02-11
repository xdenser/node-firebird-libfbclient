/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */

 static Persistent<FunctionTemplate> constructor_template;
 
 void
   FBResult::Initialize (v8::Handle<v8::Object> target)
  {
    HandleScope scope;
    
    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    t->Inherit(EventEmitter::constructor_template);
    constructor_template = Persistent<FunctionTemplate>::New(t);
    constructor_template->Inherit(EventEmitter::constructor_template);
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
    
    HandleScope scope;
    
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
                js_field = String::New(var->sqldata,var->sqllen);
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
                    js_field = Number::New(value / tens); 
                    
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
                js_field = Number::New(*(float *) (var->sqldata));  
                break;

            case SQL_DOUBLE:
                js_field = Number::New(*(double *) (var->sqldata));
                break;

            case SQL_TIMESTAMP: 
	            isc_decode_timestamp((ISC_TIMESTAMP *)var->sqldata, &times);
	            rawtime = mktime(&times);// + Connection::get_gmt_delta();
	            time_val = static_cast<double>(rawtime)*1000 + ((*((ISC_TIME *)var->sqldata)) % 10000)/10;
	            js_field = Date::New(time_val);
	            break;
	            
            case SQL_TYPE_DATE:    
	            	          
	            isc_decode_sql_date((ISC_DATE *)var->sqldata, &times);
	            rawtime = mktime(&times);// + Connection::get_gmt_delta();
	            js_date = Object::New();
	            js_date->Set(String::New("year"),
                             Integer::New(times.tm_year+1900));
                    js_date->Set(String::New("month"),
                             Integer::New(times.tm_mon+1));
                    js_date->Set(String::New("day"),
                             Integer::New(times.tm_mday));
	            time_val = static_cast<double>(rawtime)*1000;
	            js_date->Set(String::New("date"),
                             Date::New(time_val));
	            js_field = js_date;
	            break;
	            
           case SQL_TYPE_TIME:    
	            isc_decode_sql_time((ISC_TIME *)var->sqldata, &times);
	            time_val = (times.tm_hour*60*60 + 
	                        times.tm_min*60 +
	                        times.tm_sec)*1000 + 
	                        ((*((ISC_TIME *)var->sqldata)) % 10000)/10;
	            js_field = Date::New(time_val);            
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
            js_field = FBResult::GetFieldValue((XSQLVAR *) &sqlda->sqlvar[i]);
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
     free(sqldap);
   }
   
  }
  
